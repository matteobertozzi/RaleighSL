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
#if defined(TLS_OPENSSL)

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/engine.h>
#include <openssl/asn1.h>

#include <zcl/tls.h>

static void __tls_info_callback(const SSL *s, int where, int ret) {
  const char *str;
  int w;

  w = where& ~SSL_ST_MASK;
  if (w & SSL_ST_CONNECT)
    str="SSL_connect";
  else if (w & SSL_ST_ACCEPT)
    str="SSL_accept";
  else
    str="undefined";

  if (where & SSL_CB_LOOP) {
    fprintf(stderr, "%s:%s\n",str,SSL_state_string_long(s));
  } else if (where & SSL_CB_ALERT) {
    str=(where & SSL_CB_READ)?"read":"write";
    fprintf(stderr, "SSL3 alert %s:%s:%s\n", str,
               SSL_alert_type_string_long(ret),
               SSL_alert_desc_string_long(ret));
  } else if (where & SSL_CB_EXIT) {
    if (ret == 0) {
      fprintf(stderr, "%s:failed in %s\n", str,SSL_state_string_long(s));
    } else if (ret < 0) {
      fprintf(stderr, "%s:error in %s\n", str,SSL_state_string_long(s));
    }
  }
}

static int __tls_verify_peer (int preverify_ok, X509_STORE_CTX *ctx) {
    char buf[256];
    X509 *err_cert;
    int err, depth;

    err_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);

    X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

    /*
     * Catch a too long certificate chain. The depth limit set using
     * SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so
     * that whenever the "depth>verify_depth" condition is met, we
     * have violated the limit and want to log this error condition.
     * We must do it here, because the CHAIN_TOO_LONG error would not
     * be found explicitly; only errors introduced by cutting off the
     * additional certificates would be logged.
     */
     fprintf(stderr, " - DEPT %d\n", depth);
    if (depth > 2) {
        preverify_ok = 0;
        err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
        X509_STORE_CTX_set_error(ctx, err);
    }
    if (!preverify_ok) {
        fprintf(stderr, "verify error:num=%d:%s:depth=%d:%s\n", err,
                 X509_verify_cert_error_string(err), depth, buf);
    }
    fprintf(stderr, "depth=%d:%s\n", depth, buf);

    /*
     * At this point, err contains the last verification error. We can use
     * it for something special
     */
    if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
      X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
      fprintf(stderr, "issuer= %s\n", buf);
    }

    return preverify_ok;
}

int z_tls_lib_open (void) {
  SSL_library_init();
  SSL_load_error_strings();
  ERR_load_BIO_strings();
  OpenSSL_add_all_algorithms();
  ERR_load_crypto_strings();
  return(1);
}

void z_tls_lib_close (void) {
  ERR_remove_state(0);
  ENGINE_cleanup();
  CONF_modules_unload(1);
  ERR_free_strings();
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
}

z_tls_ctx_t *z_tls_ctx_open (void) {
  long ssloptions;
  SSL_CTX *ctx;

  ctx = SSL_CTX_new(TLSv1_server_method());
  if (Z_UNLIKELY(ctx == NULL))
    return(NULL);

  ssloptions = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_ALL |
               SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
#ifdef SSL_OP_NO_COMPRESSION
  ssloptions |= SSL_OP_NO_COMPRESSION;
#endif

  SSL_CTX_set_options(ctx, ssloptions);
  SSL_CTX_set_info_callback(ctx, __tls_info_callback);

  return(ctx);
}

void z_tls_ctx_close (z_tls_ctx_t *vctx) {
  SSL_CTX *ctx = Z_CAST(SSL_CTX, vctx);
  SSL_CTX_free(ctx);
}

z_tls_t *z_tls_accept (z_tls_ctx_t *vctx, int fd) {
  SSL *ssl = SSL_new(Z_CAST(SSL_CTX, vctx));
  long mode = SSL_MODE_ENABLE_PARTIAL_WRITE;
#if defined(SSL_MODE_RELEASE_BUFFERS)
  mode |= SSL_MODE_RELEASE_BUFFERS;
#endif
  SSL_set_mode(ssl, mode);
  SSL_set_accept_state(ssl);
  SSL_set_fd(ssl, fd);
  return(ssl);
}

void z_tls_disconnect (z_tls_t *vtls) {
  SSL *ssl = Z_CAST(SSL, vtls);
  SSL_shutdown(ssl);
  SSL_free(ssl);
}

#define TLS_DEFAULT_CIPHERS                                                    \
  "ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:ECDH+HIGH:"  \
  "DH+HIGH:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+HIGH:RSA+3DES:!aNULL:"     \
  "!eNULL:!MD5:!DSS:!RC4"

int z_tls_engine_inhale (z_tls_ctx_t *ctx,
                                const char *certfile,
                                const char *keyfile)
{
  int r;

  r = SSL_CTX_set_cipher_list(ctx, "ALL:!EXPORT:!LOW");
  if (Z_UNLIKELY(r != 1)) {
    return(-1);
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, __tls_verify_peer);

  r = SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM);
  if (Z_UNLIKELY(r != 1)) {
    return(-2);
  }

  r = SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM);
  if (Z_UNLIKELY(r != 1)) {
    return(-3);
  }

  r = SSL_CTX_check_private_key(ctx);
  if (Z_UNLIKELY(r != 1)) {
    return(-4);
  }

  return(0);
}

ssize_t z_tls_read (z_tls_t *tls, void *buffer, size_t bufsize) {
  ssize_t rd;
  int ret;

  if (Z_LIKELY(SSL_is_init_finished(tls))) {
    ret = SSL_read(tls, buffer, bufsize);
    rd = ret;
  } else {
    ret = SSL_do_handshake(tls);
    rd = 0;
  }

  switch (SSL_get_error(tls, ret)) {
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_SSL:
      return(-1);
  }
  return(rd);
}

ssize_t z_tls_write (z_tls_t *tls, const void *buffer, size_t bufsize) {
  ssize_t wr;
  int ret;

  if (Z_LIKELY(SSL_is_init_finished(tls))) {
    ret = SSL_write(tls, buffer, bufsize);
    wr = ret;
  } else {
    ret = SSL_do_handshake(tls);
    wr = 0;
  }

  switch (SSL_get_error(tls, ret)) {
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_SSL:
      return(-1);
  }
  return(wr);
}

#endif
