#ifndef CARLINC_TYPECHECK_GCC_H
#define CARLINC_TYPECHECK_GCC_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://carl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* wraps carl_easy_setopt() with typechecking */

/* To add a new kind of warning, add an
 *   if(carlcheck_sometype_option(_carl_opt))
 *     if(!carlcheck_sometype(value))
 *       _carl_easy_setopt_err_sometype();
 * block and define carlcheck_sometype_option, carlcheck_sometype and
 * _carl_easy_setopt_err_sometype below
 *
 * NOTE: We use two nested 'if' statements here instead of the && operator, in
 *       order to work around gcc bug #32061.  It affects only gcc 4.3.x/4.4.x
 *       when compiling with -Wlogical-op.
 *
 * To add an option that uses the same type as an existing option, you'll just
 * need to extend the appropriate _carl_*_option macro
 */
#define carl_easy_setopt(handle, option, value)                         \
  __extension__({                                                       \
      __typeof__(option) _carl_opt = option;                            \
      if(__builtin_constant_p(_carl_opt)) {                             \
        if(carlcheck_long_option(_carl_opt))                            \
          if(!carlcheck_long(value))                                    \
            _carl_easy_setopt_err_long();                               \
        if(carlcheck_off_t_option(_carl_opt))                           \
          if(!carlcheck_off_t(value))                                   \
            _carl_easy_setopt_err_carl_off_t();                         \
        if(carlcheck_string_option(_carl_opt))                          \
          if(!carlcheck_string(value))                                  \
            _carl_easy_setopt_err_string();                             \
        if(carlcheck_write_cb_option(_carl_opt))                        \
          if(!carlcheck_write_cb(value))                                \
            _carl_easy_setopt_err_write_callback();                     \
        if((_carl_opt) == CARLOPT_RESOLVER_START_FUNCTION)              \
          if(!carlcheck_resolver_start_callback(value))                 \
            _carl_easy_setopt_err_resolver_start_callback();            \
        if((_carl_opt) == CARLOPT_READFUNCTION)                         \
          if(!carlcheck_read_cb(value))                                 \
            _carl_easy_setopt_err_read_cb();                            \
        if((_carl_opt) == CARLOPT_IOCTLFUNCTION)                        \
          if(!carlcheck_ioctl_cb(value))                                \
            _carl_easy_setopt_err_ioctl_cb();                           \
        if((_carl_opt) == CARLOPT_SOCKOPTFUNCTION)                      \
          if(!carlcheck_sockopt_cb(value))                              \
            _carl_easy_setopt_err_sockopt_cb();                         \
        if((_carl_opt) == CARLOPT_OPENSOCKETFUNCTION)                   \
          if(!carlcheck_opensocket_cb(value))                           \
            _carl_easy_setopt_err_opensocket_cb();                      \
        if((_carl_opt) == CARLOPT_PROGRESSFUNCTION)                     \
          if(!carlcheck_progress_cb(value))                             \
            _carl_easy_setopt_err_progress_cb();                        \
        if((_carl_opt) == CARLOPT_DEBUGFUNCTION)                        \
          if(!carlcheck_debug_cb(value))                                \
            _carl_easy_setopt_err_debug_cb();                           \
        if((_carl_opt) == CARLOPT_SSL_CTX_FUNCTION)                     \
          if(!carlcheck_ssl_ctx_cb(value))                              \
            _carl_easy_setopt_err_ssl_ctx_cb();                         \
        if(carlcheck_conv_cb_option(_carl_opt))                         \
          if(!carlcheck_conv_cb(value))                                 \
            _carl_easy_setopt_err_conv_cb();                            \
        if((_carl_opt) == CARLOPT_SEEKFUNCTION)                         \
          if(!carlcheck_seek_cb(value))                                 \
            _carl_easy_setopt_err_seek_cb();                            \
        if(carlcheck_cb_data_option(_carl_opt))                         \
          if(!carlcheck_cb_data(value))                                 \
            _carl_easy_setopt_err_cb_data();                            \
        if((_carl_opt) == CARLOPT_ERRORBUFFER)                          \
          if(!carlcheck_error_buffer(value))                            \
            _carl_easy_setopt_err_error_buffer();                       \
        if((_carl_opt) == CARLOPT_STDERR)                               \
          if(!carlcheck_FILE(value))                                    \
            _carl_easy_setopt_err_FILE();                               \
        if(carlcheck_postfields_option(_carl_opt))                      \
          if(!carlcheck_postfields(value))                              \
            _carl_easy_setopt_err_postfields();                         \
        if((_carl_opt) == CARLOPT_HTTPPOST)                             \
          if(!carlcheck_arr((value), struct carl_httppost))             \
            _carl_easy_setopt_err_carl_httpost();                       \
        if((_carl_opt) == CARLOPT_MIMEPOST)                             \
          if(!carlcheck_ptr((value), carl_mime))                        \
            _carl_easy_setopt_err_carl_mimepost();                      \
        if(carlcheck_slist_option(_carl_opt))                           \
          if(!carlcheck_arr((value), struct carl_slist))                \
            _carl_easy_setopt_err_carl_slist();                         \
        if((_carl_opt) == CARLOPT_SHARE)                                \
          if(!carlcheck_ptr((value), CARLSH))                           \
            _carl_easy_setopt_err_CARLSH();                             \
      }                                                                 \
      carl_easy_setopt(handle, _carl_opt, value);                       \
    })

/* wraps carl_easy_getinfo() with typechecking */
#define carl_easy_getinfo(handle, info, arg)                            \
  __extension__({                                                      \
      __typeof__(info) _carl_info = info;                               \
      if(__builtin_constant_p(_carl_info)) {                            \
        if(carlcheck_string_info(_carl_info))                           \
          if(!carlcheck_arr((arg), char *))                             \
            _carl_easy_getinfo_err_string();                            \
        if(carlcheck_long_info(_carl_info))                             \
          if(!carlcheck_arr((arg), long))                               \
            _carl_easy_getinfo_err_long();                              \
        if(carlcheck_double_info(_carl_info))                           \
          if(!carlcheck_arr((arg), double))                             \
            _carl_easy_getinfo_err_double();                            \
        if(carlcheck_slist_info(_carl_info))                            \
          if(!carlcheck_arr((arg), struct carl_slist *))                \
            _carl_easy_getinfo_err_carl_slist();                        \
        if(carlcheck_tlssessioninfo_info(_carl_info))                   \
          if(!carlcheck_arr((arg), struct carl_tlssessioninfo *))       \
            _carl_easy_getinfo_err_carl_tlssesssioninfo();              \
        if(carlcheck_certinfo_info(_carl_info))                         \
          if(!carlcheck_arr((arg), struct carl_certinfo *))             \
            _carl_easy_getinfo_err_carl_certinfo();                     \
        if(carlcheck_socket_info(_carl_info))                           \
          if(!carlcheck_arr((arg), carl_socket_t))                      \
            _carl_easy_getinfo_err_carl_socket();                       \
        if(carlcheck_off_t_info(_carl_info))                            \
          if(!carlcheck_arr((arg), carl_off_t))                         \
            _carl_easy_getinfo_err_carl_off_t();                        \
      }                                                                 \
      carl_easy_getinfo(handle, _carl_info, arg);                       \
    })

/*
 * For now, just make sure that the functions are called with three arguments
 */
#define carl_share_setopt(share,opt,param) carl_share_setopt(share,opt,param)
#define carl_multi_setopt(handle,opt,param) carl_multi_setopt(handle,opt,param)


/* the actual warnings, triggered by calling the _carl_easy_setopt_err*
 * functions */

/* To define a new warning, use _CARL_WARNING(identifier, "message") */
#define CARLWARNING(id, message)                                        \
  static void __attribute__((__warning__(message)))                     \
  __attribute__((__unused__)) __attribute__((__noinline__))             \
  id(void) { __asm__(""); }

CARLWARNING(_carl_easy_setopt_err_long,
  "carl_easy_setopt expects a long argument for this option")
CARLWARNING(_carl_easy_setopt_err_carl_off_t,
  "carl_easy_setopt expects a carl_off_t argument for this option")
CARLWARNING(_carl_easy_setopt_err_string,
              "carl_easy_setopt expects a "
              "string ('char *' or char[]) argument for this option"
  )
CARLWARNING(_carl_easy_setopt_err_write_callback,
  "carl_easy_setopt expects a carl_write_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_resolver_start_callback,
              "carl_easy_setopt expects a "
              "carl_resolver_start_callback argument for this option"
  )
CARLWARNING(_carl_easy_setopt_err_read_cb,
  "carl_easy_setopt expects a carl_read_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_ioctl_cb,
  "carl_easy_setopt expects a carl_ioctl_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_sockopt_cb,
  "carl_easy_setopt expects a carl_sockopt_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_opensocket_cb,
              "carl_easy_setopt expects a "
              "carl_opensocket_callback argument for this option"
  )
CARLWARNING(_carl_easy_setopt_err_progress_cb,
  "carl_easy_setopt expects a carl_progress_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_debug_cb,
  "carl_easy_setopt expects a carl_debug_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_ssl_ctx_cb,
  "carl_easy_setopt expects a carl_ssl_ctx_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_conv_cb,
  "carl_easy_setopt expects a carl_conv_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_seek_cb,
  "carl_easy_setopt expects a carl_seek_callback argument for this option")
CARLWARNING(_carl_easy_setopt_err_cb_data,
              "carl_easy_setopt expects a "
              "private data pointer as argument for this option")
CARLWARNING(_carl_easy_setopt_err_error_buffer,
              "carl_easy_setopt expects a "
              "char buffer of CARL_ERROR_SIZE as argument for this option")
CARLWARNING(_carl_easy_setopt_err_FILE,
  "carl_easy_setopt expects a 'FILE *' argument for this option")
CARLWARNING(_carl_easy_setopt_err_postfields,
  "carl_easy_setopt expects a 'void *' or 'char *' argument for this option")
CARLWARNING(_carl_easy_setopt_err_carl_httpost,
              "carl_easy_setopt expects a 'struct carl_httppost *' "
              "argument for this option")
CARLWARNING(_carl_easy_setopt_err_carl_mimepost,
              "carl_easy_setopt expects a 'carl_mime *' "
              "argument for this option")
CARLWARNING(_carl_easy_setopt_err_carl_slist,
  "carl_easy_setopt expects a 'struct carl_slist *' argument for this option")
CARLWARNING(_carl_easy_setopt_err_CARLSH,
  "carl_easy_setopt expects a CARLSH* argument for this option")

CARLWARNING(_carl_easy_getinfo_err_string,
  "carl_easy_getinfo expects a pointer to 'char *' for this info")
CARLWARNING(_carl_easy_getinfo_err_long,
  "carl_easy_getinfo expects a pointer to long for this info")
CARLWARNING(_carl_easy_getinfo_err_double,
  "carl_easy_getinfo expects a pointer to double for this info")
CARLWARNING(_carl_easy_getinfo_err_carl_slist,
  "carl_easy_getinfo expects a pointer to 'struct carl_slist *' for this info")
CARLWARNING(_carl_easy_getinfo_err_carl_tlssesssioninfo,
              "carl_easy_getinfo expects a pointer to "
              "'struct carl_tlssessioninfo *' for this info")
CARLWARNING(_carl_easy_getinfo_err_carl_certinfo,
              "carl_easy_getinfo expects a pointer to "
              "'struct carl_certinfo *' for this info")
CARLWARNING(_carl_easy_getinfo_err_carl_socket,
  "carl_easy_getinfo expects a pointer to carl_socket_t for this info")
CARLWARNING(_carl_easy_getinfo_err_carl_off_t,
  "carl_easy_getinfo expects a pointer to carl_off_t for this info")

/* groups of carl_easy_setops options that take the same type of argument */

/* To add a new option to one of the groups, just add
 *   (option) == CARLOPT_SOMETHING
 * to the or-expression. If the option takes a long or carl_off_t, you don't
 * have to do anything
 */

/* evaluates to true if option takes a long argument */
#define carlcheck_long_option(option)                   \
  (0 < (option) && (option) < CARLOPTTYPE_OBJECTPOINT)

#define carlcheck_off_t_option(option)          \
  (((option) > CARLOPTTYPE_OFF_T) && ((option) < CARLOPTTYPE_BLOB))

/* evaluates to true if option takes a char* argument */
#define carlcheck_string_option(option)                                       \
  ((option) == CARLOPT_ABSTRACT_UNIX_SOCKET ||                                \
   (option) == CARLOPT_ACCEPT_ENCODING ||                                     \
   (option) == CARLOPT_ALTSVC ||                                              \
   (option) == CARLOPT_CAINFO ||                                              \
   (option) == CARLOPT_CAPATH ||                                              \
   (option) == CARLOPT_COOKIE ||                                              \
   (option) == CARLOPT_COOKIEFILE ||                                          \
   (option) == CARLOPT_COOKIEJAR ||                                           \
   (option) == CARLOPT_COOKIELIST ||                                          \
   (option) == CARLOPT_CRLFILE ||                                             \
   (option) == CARLOPT_CUSTOMREQUEST ||                                       \
   (option) == CARLOPT_DEFAULT_PROTOCOL ||                                    \
   (option) == CARLOPT_DNS_INTERFACE ||                                       \
   (option) == CARLOPT_DNS_LOCAL_IP4 ||                                       \
   (option) == CARLOPT_DNS_LOCAL_IP6 ||                                       \
   (option) == CARLOPT_DNS_SERVERS ||                                         \
   (option) == CARLOPT_DOH_URL ||                                             \
   (option) == CARLOPT_EGDSOCKET ||                                           \
   (option) == CARLOPT_FTPPORT ||                                             \
   (option) == CARLOPT_FTP_ACCOUNT ||                                         \
   (option) == CARLOPT_FTP_ALTERNATIVE_TO_USER ||                             \
   (option) == CARLOPT_HSTS ||                                                \
   (option) == CARLOPT_INTERFACE ||                                           \
   (option) == CARLOPT_ISSUERCERT ||                                          \
   (option) == CARLOPT_KEYPASSWD ||                                           \
   (option) == CARLOPT_KRBLEVEL ||                                            \
   (option) == CARLOPT_LOGIN_OPTIONS ||                                       \
   (option) == CARLOPT_MAIL_AUTH ||                                           \
   (option) == CARLOPT_MAIL_FROM ||                                           \
   (option) == CARLOPT_NETRC_FILE ||                                          \
   (option) == CARLOPT_NOPROXY ||                                             \
   (option) == CARLOPT_PASSWORD ||                                            \
   (option) == CARLOPT_PINNEDPUBLICKEY ||                                     \
   (option) == CARLOPT_PRE_PROXY ||                                           \
   (option) == CARLOPT_PROXY ||                                               \
   (option) == CARLOPT_PROXYPASSWORD ||                                       \
   (option) == CARLOPT_PROXYUSERNAME ||                                       \
   (option) == CARLOPT_PROXYUSERPWD ||                                        \
   (option) == CARLOPT_PROXY_CAINFO ||                                        \
   (option) == CARLOPT_PROXY_CAPATH ||                                        \
   (option) == CARLOPT_PROXY_CRLFILE ||                                       \
   (option) == CARLOPT_PROXY_ISSUERCERT ||                                    \
   (option) == CARLOPT_PROXY_KEYPASSWD ||                                     \
   (option) == CARLOPT_PROXY_PINNEDPUBLICKEY ||                               \
   (option) == CARLOPT_PROXY_SERVICE_NAME ||                                  \
   (option) == CARLOPT_PROXY_SSLCERT ||                                       \
   (option) == CARLOPT_PROXY_SSLCERTTYPE ||                                   \
   (option) == CARLOPT_PROXY_SSLKEY ||                                        \
   (option) == CARLOPT_PROXY_SSLKEYTYPE ||                                    \
   (option) == CARLOPT_PROXY_SSL_CIPHER_LIST ||                               \
   (option) == CARLOPT_PROXY_TLS13_CIPHERS ||                                 \
   (option) == CARLOPT_PROXY_TLSAUTH_PASSWORD ||                              \
   (option) == CARLOPT_PROXY_TLSAUTH_TYPE ||                                  \
   (option) == CARLOPT_PROXY_TLSAUTH_USERNAME ||                              \
   (option) == CARLOPT_RANDOM_FILE ||                                         \
   (option) == CARLOPT_RANGE ||                                               \
   (option) == CARLOPT_REFERER ||                                             \
   (option) == CARLOPT_REQUEST_TARGET ||                                      \
   (option) == CARLOPT_RTSP_SESSION_ID ||                                     \
   (option) == CARLOPT_RTSP_STREAM_URI ||                                     \
   (option) == CARLOPT_RTSP_TRANSPORT ||                                      \
   (option) == CARLOPT_SASL_AUTHZID ||                                        \
   (option) == CARLOPT_SERVICE_NAME ||                                        \
   (option) == CARLOPT_SOCKS5_GSSAPI_SERVICE ||                               \
   (option) == CARLOPT_SSH_HOST_PUBLIC_KEY_MD5 ||                             \
   (option) == CARLOPT_SSH_KNOWNHOSTS ||                                      \
   (option) == CARLOPT_SSH_PRIVATE_KEYFILE ||                                 \
   (option) == CARLOPT_SSH_PUBLIC_KEYFILE ||                                  \
   (option) == CARLOPT_SSLCERT ||                                             \
   (option) == CARLOPT_SSLCERTTYPE ||                                         \
   (option) == CARLOPT_SSLENGINE ||                                           \
   (option) == CARLOPT_SSLKEY ||                                              \
   (option) == CARLOPT_SSLKEYTYPE ||                                          \
   (option) == CARLOPT_SSL_CIPHER_LIST ||                                     \
   (option) == CARLOPT_TLS13_CIPHERS ||                                       \
   (option) == CARLOPT_TLSAUTH_PASSWORD ||                                    \
   (option) == CARLOPT_TLSAUTH_TYPE ||                                        \
   (option) == CARLOPT_TLSAUTH_USERNAME ||                                    \
   (option) == CARLOPT_UNIX_SOCKET_PATH ||                                    \
   (option) == CARLOPT_URL ||                                                 \
   (option) == CARLOPT_USERAGENT ||                                           \
   (option) == CARLOPT_USERNAME ||                                            \
   (option) == CARLOPT_AWS_SIGV4 ||                                           \
   (option) == CARLOPT_USERPWD ||                                             \
   (option) == CARLOPT_XOAUTH2_BEARER ||                                      \
   (option) == CARLOPT_SSL_EC_CURVES ||                                       \
   0)

/* evaluates to true if option takes a carl_write_callback argument */
#define carlcheck_write_cb_option(option)                               \
  ((option) == CARLOPT_HEADERFUNCTION ||                                \
   (option) == CARLOPT_WRITEFUNCTION)

/* evaluates to true if option takes a carl_conv_callback argument */
#define carlcheck_conv_cb_option(option)                                \
  ((option) == CARLOPT_CONV_TO_NETWORK_FUNCTION ||                      \
   (option) == CARLOPT_CONV_FROM_NETWORK_FUNCTION ||                    \
   (option) == CARLOPT_CONV_FROM_UTF8_FUNCTION)

/* evaluates to true if option takes a data argument to pass to a callback */
#define carlcheck_cb_data_option(option)                                      \
  ((option) == CARLOPT_CHUNK_DATA ||                                          \
   (option) == CARLOPT_CLOSESOCKETDATA ||                                     \
   (option) == CARLOPT_DEBUGDATA ||                                           \
   (option) == CARLOPT_FNMATCH_DATA ||                                        \
   (option) == CARLOPT_HEADERDATA ||                                          \
   (option) == CARLOPT_HSTSREADDATA ||                                        \
   (option) == CARLOPT_HSTSWRITEDATA ||                                       \
   (option) == CARLOPT_INTERLEAVEDATA ||                                      \
   (option) == CARLOPT_IOCTLDATA ||                                           \
   (option) == CARLOPT_OPENSOCKETDATA ||                                      \
   (option) == CARLOPT_PROGRESSDATA ||                                        \
   (option) == CARLOPT_READDATA ||                                            \
   (option) == CARLOPT_SEEKDATA ||                                            \
   (option) == CARLOPT_SOCKOPTDATA ||                                         \
   (option) == CARLOPT_SSH_KEYDATA ||                                         \
   (option) == CARLOPT_SSL_CTX_DATA ||                                        \
   (option) == CARLOPT_WRITEDATA ||                                           \
   (option) == CARLOPT_RESOLVER_START_DATA ||                                 \
   (option) == CARLOPT_TRAILERDATA ||                                         \
   0)

/* evaluates to true if option takes a POST data argument (void* or char*) */
#define carlcheck_postfields_option(option)                                   \
  ((option) == CARLOPT_POSTFIELDS ||                                          \
   (option) == CARLOPT_COPYPOSTFIELDS ||                                      \
   0)

/* evaluates to true if option takes a struct carl_slist * argument */
#define carlcheck_slist_option(option)                                        \
  ((option) == CARLOPT_HTTP200ALIASES ||                                      \
   (option) == CARLOPT_HTTPHEADER ||                                          \
   (option) == CARLOPT_MAIL_RCPT ||                                           \
   (option) == CARLOPT_POSTQUOTE ||                                           \
   (option) == CARLOPT_PREQUOTE ||                                            \
   (option) == CARLOPT_PROXYHEADER ||                                         \
   (option) == CARLOPT_QUOTE ||                                               \
   (option) == CARLOPT_RESOLVE ||                                             \
   (option) == CARLOPT_TELNETOPTIONS ||                                       \
   (option) == CARLOPT_CONNECT_TO ||                                          \
   0)

/* groups of carl_easy_getinfo infos that take the same type of argument */

/* evaluates to true if info expects a pointer to char * argument */
#define carlcheck_string_info(info)                             \
  (CARLINFO_STRING < (info) && (info) < CARLINFO_LONG &&        \
   (info) != CARLINFO_PRIVATE)

/* evaluates to true if info expects a pointer to long argument */
#define carlcheck_long_info(info)                       \
  (CARLINFO_LONG < (info) && (info) < CARLINFO_DOUBLE)

/* evaluates to true if info expects a pointer to double argument */
#define carlcheck_double_info(info)                     \
  (CARLINFO_DOUBLE < (info) && (info) < CARLINFO_SLIST)

/* true if info expects a pointer to struct carl_slist * argument */
#define carlcheck_slist_info(info)                                      \
  (((info) == CARLINFO_SSL_ENGINES) || ((info) == CARLINFO_COOKIELIST))

/* true if info expects a pointer to struct carl_tlssessioninfo * argument */
#define carlcheck_tlssessioninfo_info(info)                              \
  (((info) == CARLINFO_TLS_SSL_PTR) || ((info) == CARLINFO_TLS_SESSION))

/* true if info expects a pointer to struct carl_certinfo * argument */
#define carlcheck_certinfo_info(info) ((info) == CARLINFO_CERTINFO)

/* true if info expects a pointer to struct carl_socket_t argument */
#define carlcheck_socket_info(info)                     \
  (CARLINFO_SOCKET < (info) && (info) < CARLINFO_OFF_T)

/* true if info expects a pointer to carl_off_t argument */
#define carlcheck_off_t_info(info)              \
  (CARLINFO_OFF_T < (info))


/* typecheck helpers -- check whether given expression has requested type*/

/* For pointers, you can use the carlcheck_ptr/carlcheck_arr macros,
 * otherwise define a new macro. Search for __builtin_types_compatible_p
 * in the GCC manual.
 * NOTE: these macros MUST NOT EVALUATE their arguments! The argument is
 * the actual expression passed to the carl_easy_setopt macro. This
 * means that you can only apply the sizeof and __typeof__ operators, no
 * == or whatsoever.
 */

/* XXX: should evaluate to true if expr is a pointer */
#define carlcheck_any_ptr(expr)                 \
  (sizeof(expr) == sizeof(void *))

/* evaluates to true if expr is NULL */
/* XXX: must not evaluate expr, so this check is not accurate */
#define carlcheck_NULL(expr)                                            \
  (__builtin_types_compatible_p(__typeof__(expr), __typeof__(NULL)))

/* evaluates to true if expr is type*, const type* or NULL */
#define carlcheck_ptr(expr, type)                                       \
  (carlcheck_NULL(expr) ||                                              \
   __builtin_types_compatible_p(__typeof__(expr), type *) ||            \
   __builtin_types_compatible_p(__typeof__(expr), const type *))

/* evaluates to true if expr is one of type[], type*, NULL or const type* */
#define carlcheck_arr(expr, type)                                       \
  (carlcheck_ptr((expr), type) ||                                       \
   __builtin_types_compatible_p(__typeof__(expr), type []))

/* evaluates to true if expr is a string */
#define carlcheck_string(expr)                                          \
  (carlcheck_arr((expr), char) ||                                       \
   carlcheck_arr((expr), signed char) ||                                \
   carlcheck_arr((expr), unsigned char))

/* evaluates to true if expr is a long (no matter the signedness)
 * XXX: for now, int is also accepted (and therefore short and char, which
 * are promoted to int when passed to a variadic function) */
#define carlcheck_long(expr)                                                  \
  (__builtin_types_compatible_p(__typeof__(expr), long) ||                    \
   __builtin_types_compatible_p(__typeof__(expr), signed long) ||             \
   __builtin_types_compatible_p(__typeof__(expr), unsigned long) ||           \
   __builtin_types_compatible_p(__typeof__(expr), int) ||                     \
   __builtin_types_compatible_p(__typeof__(expr), signed int) ||              \
   __builtin_types_compatible_p(__typeof__(expr), unsigned int) ||            \
   __builtin_types_compatible_p(__typeof__(expr), short) ||                   \
   __builtin_types_compatible_p(__typeof__(expr), signed short) ||            \
   __builtin_types_compatible_p(__typeof__(expr), unsigned short) ||          \
   __builtin_types_compatible_p(__typeof__(expr), char) ||                    \
   __builtin_types_compatible_p(__typeof__(expr), signed char) ||             \
   __builtin_types_compatible_p(__typeof__(expr), unsigned char))

/* evaluates to true if expr is of type carl_off_t */
#define carlcheck_off_t(expr)                                   \
  (__builtin_types_compatible_p(__typeof__(expr), carl_off_t))

/* evaluates to true if expr is abuffer suitable for CARLOPT_ERRORBUFFER */
/* XXX: also check size of an char[] array? */
#define carlcheck_error_buffer(expr)                                    \
  (carlcheck_NULL(expr) ||                                              \
   __builtin_types_compatible_p(__typeof__(expr), char *) ||            \
   __builtin_types_compatible_p(__typeof__(expr), char[]))

/* evaluates to true if expr is of type (const) void* or (const) FILE* */
#if 0
#define carlcheck_cb_data(expr)                                         \
  (carlcheck_ptr((expr), void) ||                                       \
   carlcheck_ptr((expr), FILE))
#else /* be less strict */
#define carlcheck_cb_data(expr)                 \
  carlcheck_any_ptr(expr)
#endif

/* evaluates to true if expr is of type FILE* */
#define carlcheck_FILE(expr)                                            \
  (carlcheck_NULL(expr) ||                                              \
   (__builtin_types_compatible_p(__typeof__(expr), FILE *)))

/* evaluates to true if expr can be passed as POST data (void* or char*) */
#define carlcheck_postfields(expr)                                      \
  (carlcheck_ptr((expr), void) ||                                       \
   carlcheck_arr((expr), char) ||                                       \
   carlcheck_arr((expr), unsigned char))

/* helper: __builtin_types_compatible_p distinguishes between functions and
 * function pointers, hide it */
#define carlcheck_cb_compatible(func, type)                             \
  (__builtin_types_compatible_p(__typeof__(func), type) ||              \
   __builtin_types_compatible_p(__typeof__(func) *, type))

/* evaluates to true if expr is of type carl_resolver_start_callback */
#define carlcheck_resolver_start_callback(expr)       \
  (carlcheck_NULL(expr) || \
   carlcheck_cb_compatible((expr), carl_resolver_start_callback))

/* evaluates to true if expr is of type carl_read_callback or "similar" */
#define carlcheck_read_cb(expr)                                         \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), __typeof__(fread) *) ||              \
   carlcheck_cb_compatible((expr), carl_read_callback) ||               \
   carlcheck_cb_compatible((expr), _carl_read_callback1) ||             \
   carlcheck_cb_compatible((expr), _carl_read_callback2) ||             \
   carlcheck_cb_compatible((expr), _carl_read_callback3) ||             \
   carlcheck_cb_compatible((expr), _carl_read_callback4) ||             \
   carlcheck_cb_compatible((expr), _carl_read_callback5) ||             \
   carlcheck_cb_compatible((expr), _carl_read_callback6))
typedef size_t (*_carl_read_callback1)(char *, size_t, size_t, void *);
typedef size_t (*_carl_read_callback2)(char *, size_t, size_t, const void *);
typedef size_t (*_carl_read_callback3)(char *, size_t, size_t, FILE *);
typedef size_t (*_carl_read_callback4)(void *, size_t, size_t, void *);
typedef size_t (*_carl_read_callback5)(void *, size_t, size_t, const void *);
typedef size_t (*_carl_read_callback6)(void *, size_t, size_t, FILE *);

/* evaluates to true if expr is of type carl_write_callback or "similar" */
#define carlcheck_write_cb(expr)                                        \
  (carlcheck_read_cb(expr) ||                                           \
   carlcheck_cb_compatible((expr), __typeof__(fwrite) *) ||             \
   carlcheck_cb_compatible((expr), carl_write_callback) ||              \
   carlcheck_cb_compatible((expr), _carl_write_callback1) ||            \
   carlcheck_cb_compatible((expr), _carl_write_callback2) ||            \
   carlcheck_cb_compatible((expr), _carl_write_callback3) ||            \
   carlcheck_cb_compatible((expr), _carl_write_callback4) ||            \
   carlcheck_cb_compatible((expr), _carl_write_callback5) ||            \
   carlcheck_cb_compatible((expr), _carl_write_callback6))
typedef size_t (*_carl_write_callback1)(const char *, size_t, size_t, void *);
typedef size_t (*_carl_write_callback2)(const char *, size_t, size_t,
                                       const void *);
typedef size_t (*_carl_write_callback3)(const char *, size_t, size_t, FILE *);
typedef size_t (*_carl_write_callback4)(const void *, size_t, size_t, void *);
typedef size_t (*_carl_write_callback5)(const void *, size_t, size_t,
                                       const void *);
typedef size_t (*_carl_write_callback6)(const void *, size_t, size_t, FILE *);

/* evaluates to true if expr is of type carl_ioctl_callback or "similar" */
#define carlcheck_ioctl_cb(expr)                                        \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), carl_ioctl_callback) ||              \
   carlcheck_cb_compatible((expr), _carl_ioctl_callback1) ||            \
   carlcheck_cb_compatible((expr), _carl_ioctl_callback2) ||            \
   carlcheck_cb_compatible((expr), _carl_ioctl_callback3) ||            \
   carlcheck_cb_compatible((expr), _carl_ioctl_callback4))
typedef carlioerr (*_carl_ioctl_callback1)(CARL *, int, void *);
typedef carlioerr (*_carl_ioctl_callback2)(CARL *, int, const void *);
typedef carlioerr (*_carl_ioctl_callback3)(CARL *, carliocmd, void *);
typedef carlioerr (*_carl_ioctl_callback4)(CARL *, carliocmd, const void *);

/* evaluates to true if expr is of type carl_sockopt_callback or "similar" */
#define carlcheck_sockopt_cb(expr)                                      \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), carl_sockopt_callback) ||            \
   carlcheck_cb_compatible((expr), _carl_sockopt_callback1) ||          \
   carlcheck_cb_compatible((expr), _carl_sockopt_callback2))
typedef int (*_carl_sockopt_callback1)(void *, carl_socket_t, carlsocktype);
typedef int (*_carl_sockopt_callback2)(const void *, carl_socket_t,
                                      carlsocktype);

/* evaluates to true if expr is of type carl_opensocket_callback or
   "similar" */
#define carlcheck_opensocket_cb(expr)                                   \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), carl_opensocket_callback) ||         \
   carlcheck_cb_compatible((expr), _carl_opensocket_callback1) ||       \
   carlcheck_cb_compatible((expr), _carl_opensocket_callback2) ||       \
   carlcheck_cb_compatible((expr), _carl_opensocket_callback3) ||       \
   carlcheck_cb_compatible((expr), _carl_opensocket_callback4))
typedef carl_socket_t (*_carl_opensocket_callback1)
  (void *, carlsocktype, struct carl_sockaddr *);
typedef carl_socket_t (*_carl_opensocket_callback2)
  (void *, carlsocktype, const struct carl_sockaddr *);
typedef carl_socket_t (*_carl_opensocket_callback3)
  (const void *, carlsocktype, struct carl_sockaddr *);
typedef carl_socket_t (*_carl_opensocket_callback4)
  (const void *, carlsocktype, const struct carl_sockaddr *);

/* evaluates to true if expr is of type carl_progress_callback or "similar" */
#define carlcheck_progress_cb(expr)                                     \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), carl_progress_callback) ||           \
   carlcheck_cb_compatible((expr), _carl_progress_callback1) ||         \
   carlcheck_cb_compatible((expr), _carl_progress_callback2))
typedef int (*_carl_progress_callback1)(void *,
    double, double, double, double);
typedef int (*_carl_progress_callback2)(const void *,
    double, double, double, double);

/* evaluates to true if expr is of type carl_debug_callback or "similar" */
#define carlcheck_debug_cb(expr)                                        \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), carl_debug_callback) ||              \
   carlcheck_cb_compatible((expr), _carl_debug_callback1) ||            \
   carlcheck_cb_compatible((expr), _carl_debug_callback2) ||            \
   carlcheck_cb_compatible((expr), _carl_debug_callback3) ||            \
   carlcheck_cb_compatible((expr), _carl_debug_callback4) ||            \
   carlcheck_cb_compatible((expr), _carl_debug_callback5) ||            \
   carlcheck_cb_compatible((expr), _carl_debug_callback6) ||            \
   carlcheck_cb_compatible((expr), _carl_debug_callback7) ||            \
   carlcheck_cb_compatible((expr), _carl_debug_callback8))
typedef int (*_carl_debug_callback1) (CARL *,
    carl_infotype, char *, size_t, void *);
typedef int (*_carl_debug_callback2) (CARL *,
    carl_infotype, char *, size_t, const void *);
typedef int (*_carl_debug_callback3) (CARL *,
    carl_infotype, const char *, size_t, void *);
typedef int (*_carl_debug_callback4) (CARL *,
    carl_infotype, const char *, size_t, const void *);
typedef int (*_carl_debug_callback5) (CARL *,
    carl_infotype, unsigned char *, size_t, void *);
typedef int (*_carl_debug_callback6) (CARL *,
    carl_infotype, unsigned char *, size_t, const void *);
typedef int (*_carl_debug_callback7) (CARL *,
    carl_infotype, const unsigned char *, size_t, void *);
typedef int (*_carl_debug_callback8) (CARL *,
    carl_infotype, const unsigned char *, size_t, const void *);

/* evaluates to true if expr is of type carl_ssl_ctx_callback or "similar" */
/* this is getting even messier... */
#define carlcheck_ssl_ctx_cb(expr)                                      \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), carl_ssl_ctx_callback) ||            \
   carlcheck_cb_compatible((expr), _carl_ssl_ctx_callback1) ||          \
   carlcheck_cb_compatible((expr), _carl_ssl_ctx_callback2) ||          \
   carlcheck_cb_compatible((expr), _carl_ssl_ctx_callback3) ||          \
   carlcheck_cb_compatible((expr), _carl_ssl_ctx_callback4) ||          \
   carlcheck_cb_compatible((expr), _carl_ssl_ctx_callback5) ||          \
   carlcheck_cb_compatible((expr), _carl_ssl_ctx_callback6) ||          \
   carlcheck_cb_compatible((expr), _carl_ssl_ctx_callback7) ||          \
   carlcheck_cb_compatible((expr), _carl_ssl_ctx_callback8))
typedef CARLcode (*_carl_ssl_ctx_callback1)(CARL *, void *, void *);
typedef CARLcode (*_carl_ssl_ctx_callback2)(CARL *, void *, const void *);
typedef CARLcode (*_carl_ssl_ctx_callback3)(CARL *, const void *, void *);
typedef CARLcode (*_carl_ssl_ctx_callback4)(CARL *, const void *,
                                            const void *);
#ifdef HEADER_SSL_H
/* hack: if we included OpenSSL's ssl.h, we know about SSL_CTX
 * this will of course break if we're included before OpenSSL headers...
 */
typedef CARLcode (*_carl_ssl_ctx_callback5)(CARL *, SSL_CTX, void *);
typedef CARLcode (*_carl_ssl_ctx_callback6)(CARL *, SSL_CTX, const void *);
typedef CARLcode (*_carl_ssl_ctx_callback7)(CARL *, const SSL_CTX, void *);
typedef CARLcode (*_carl_ssl_ctx_callback8)(CARL *, const SSL_CTX,
                                           const void *);
#else
typedef _carl_ssl_ctx_callback1 _carl_ssl_ctx_callback5;
typedef _carl_ssl_ctx_callback1 _carl_ssl_ctx_callback6;
typedef _carl_ssl_ctx_callback1 _carl_ssl_ctx_callback7;
typedef _carl_ssl_ctx_callback1 _carl_ssl_ctx_callback8;
#endif

/* evaluates to true if expr is of type carl_conv_callback or "similar" */
#define carlcheck_conv_cb(expr)                                         \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), carl_conv_callback) ||               \
   carlcheck_cb_compatible((expr), _carl_conv_callback1) ||             \
   carlcheck_cb_compatible((expr), _carl_conv_callback2) ||             \
   carlcheck_cb_compatible((expr), _carl_conv_callback3) ||             \
   carlcheck_cb_compatible((expr), _carl_conv_callback4))
typedef CARLcode (*_carl_conv_callback1)(char *, size_t length);
typedef CARLcode (*_carl_conv_callback2)(const char *, size_t length);
typedef CARLcode (*_carl_conv_callback3)(void *, size_t length);
typedef CARLcode (*_carl_conv_callback4)(const void *, size_t length);

/* evaluates to true if expr is of type carl_seek_callback or "similar" */
#define carlcheck_seek_cb(expr)                                         \
  (carlcheck_NULL(expr) ||                                              \
   carlcheck_cb_compatible((expr), carl_seek_callback) ||               \
   carlcheck_cb_compatible((expr), _carl_seek_callback1) ||             \
   carlcheck_cb_compatible((expr), _carl_seek_callback2))
typedef CARLcode (*_carl_seek_callback1)(void *, carl_off_t, int);
typedef CARLcode (*_carl_seek_callback2)(const void *, carl_off_t, int);


#endif /* CARLINC_TYPECHECK_GCC_H */
