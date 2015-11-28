/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <zcl/config.h>
//#define TLS_SECURE_TRANSPORT
#if defined(TLS_SECURE_TRANSPORT)

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <Security/SecureTransport.h>

#include <zcl/tls.h>

#ifndef ioErr
  #define ioErr -36
#endif

static OSStatus __tls_read (SSLConnectionRef connection,
                            void *buffer,
                            size_t *bufsize)
{
  int fd = *((int *)connection);
  size_t bytesToGo = *bufsize;
  size_t initLen = bytesToGo;
  UInt8 *currData = (UInt8 *)buffer;
  OSStatus retValue = noErr;
  ssize_t val;

  for (;;) {
    val = read(fd, currData, bytesToGo);
    if (val <= 0) {
      if (val == 0) {
        retValue = errSSLClosedGraceful;
      } else { /* do the switch */
        switch (errno) {
          case ENOENT:
            /* connection closed */
            retValue = errSSLClosedGraceful;
            break;
          case ECONNRESET:
            retValue = errSSLClosedAbort;
            break;
          case EAGAIN:
            retValue = errSSLWouldBlock;
            break;
          default:
            retValue = ioErr;
            break;
        }
      }
      break;
    } else {
      bytesToGo -= val;
      currData += val;
    }

    if (bytesToGo == 0) {
      /* filled buffer with incoming data, done */
      break;
    }
  }
  *bufsize = initLen - bytesToGo;
  return retValue;
}

static OSStatus __tls_write (SSLConnectionRef connection,
                             const void *buffer,
                             size_t *bufsize)
{
  int fd = *((int *)connection);
  size_t bytesSent = 0;
  size_t dataLen = *bufsize;
  OSStatus retValue = noErr;
  ssize_t val;

  do {
    val = write(fd, (char *)buffer + bytesSent, dataLen - bytesSent);
  } while (val >= 0 && (bytesSent += val) < dataLen);

  if (val < 0) {
    switch(errno) {
      case EAGAIN:
        retValue = errSSLWouldBlock;
        break;
      case EPIPE:
      case ECONNRESET:
        retValue = errSSLClosedAbort;
        break;
      default:
        retValue = ioErr;
        break;
    }
  }
  *bufsize = bytesSent;
  return retValue;
}

int z_tls_lib_open (void) {
  return(0);
}

void z_tls_lib_close (void) {
}

z_tls_ctx_t *z_tls_ctx_open (void) {
  SSLContextRef ctx;
  int ret;

  ctx = SSLCreateContext(kCFAllocatorDefault, kSSLServerSide, kSSLStreamType);
  if (Z_UNLIKELY(ctx == NULL))
    return(NULL);

  ret = SSLSetIOFuncs(ctx, __tls_read, __tls_write);
  if (Z_UNLIKELY(ret != noErr)) {
    CFRelease(ctx);
    return(NULL);
  }

  SSLSetProtocolVersionMin(ctx, kTLSProtocol1);

  return(ctx);
}

void z_tls_ctx_close (z_tls_ctx_t *vctx) {
  SSLContextRef ctx = (SSLContextRef)vctx;
  CFRelease(ctx);
}

z_tls_t *z_tls_accept (z_tls_ctx_t *vctx, int fd) {
  SSLContextRef ctx = (SSLContextRef)vctx;
  SSLSetConnection(ctx, &fd);
  return(NULL);
}

void z_tls_disconnect (z_tls_t *vtls) {
}

int z_tls_engine_inhale (z_tls_ctx_t *ctx,
                                const char *cert, const char *key)
{
  return(-1);
}

ssize_t z_tls_read (z_tls_t *tls, void *buffer, size_t bufsize) {
  size_t actualSize;

  OSStatus ret = SSLRead(tls, buffer, bufsize, &actualSize);
  if (ret == errSSLWouldBlock && actualSize)
    return actualSize;

  /* peer performed shutdown */
  if (ret == errSSLClosedNoNotify || ret == errSSLClosedGraceful) {
    return 0;
  }

  return ret != noErr ? -1 : actualSize;
}

ssize_t z_tls_write (z_tls_t *tls, const void *buffer, size_t bufsize) {
  return -1;
}


#endif
