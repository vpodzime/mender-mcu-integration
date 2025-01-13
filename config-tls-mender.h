/* Zephyr's config-tls-generic.h defines this macro to the value of
   CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN. However, that config value is used for
   both input and output content and we can only use smaller output content
   length because how much data is sent to us is not under our control (the
   max_fragment_len TLS 1.2 extension seems to be ignored by the webservers we
   communicate with). Prevent redefinition warnings by undefining the macro
   first. */
#ifdef MBEDTLS_SSL_OUT_CONTENT_LEN
#undef MBEDTLS_SSL_OUT_CONTENT_LEN
#endif
#define MBEDTLS_SSL_OUT_CONTENT_LEN 2048
