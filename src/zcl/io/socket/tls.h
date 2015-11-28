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

#ifndef _Z_TLS_H_
#define _Z_TLS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef void z_tls_ctx_t;
typedef void z_tls_t;

#if defined(TLS_OPENSSL)
  #define Z_SYS_HAS_TLS     1
#elif defined(TLS_SECURE_TRANSPORT)
  #define Z_SYS_HAS_TLS     1
#else
  #define Z_SYS_HAS_TLS     0
#endif

#if (Z_SYS_HAS_TLS)
  #define z_tls_is_supported()        (1)

  int   z_tls_lib_open      (void);
  void  z_tls_lib_close     (void);

  z_tls_ctx_t *z_tls_ctx_open       (void);
  void         z_tls_ctx_close      (z_tls_ctx_t *ctx);
  z_tls_t *    z_tls_accept         (z_tls_ctx_t *ctx, int fd);
  void         z_tls_disconnect     (z_tls_t *tls);
  int          z_tls_engine_inhale  (z_tls_ctx_t *ctx,
                                     const char *cert,
                                     const char *key);

  ssize_t     z_tls_read   (z_tls_t *tls,
                            void *buffer,
                            size_t bufsize);
  ssize_t     z_tls_write  (z_tls_t *tls,
                            const void *buffer,
                            size_t bufsize);
#else
  #define z_tls_is_supported()              (0)
  #define z_tls_lib_open()                  (0)
  #define z_tls_lib_close()                 while (0)
  #define z_tls_ctx_open()                  (NULL)
  #define z_tls_ctx_close(ctx)              while (0)
  #define z_tls_accept(ctx, fd)             (NULL)
  #define z_tls_disconnect(tls)             while (0)
  #define z_tls_engine_inhale(ctx, c, k)    (1)
  #define z_tls_read(tls, buf, bufsize)     (-1)
  #define z_tls_write(tls, buf, bufsize)     (-1)
#endif

__Z_END_DECLS__

#endif /* !_Z_TLS_H_ */
