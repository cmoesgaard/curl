#ifndef HEADER_CARL_URL_H
#define HEADER_CARL_URL_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
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
#include "carl_setup.h"

/*
 * Prototypes for library-wide functions provided by url.c
 */

CARLcode Curl_init_do(struct Curl_easy *data, struct connectdata *conn);
CARLcode Curl_open(struct Curl_easy **carl);
CARLcode Curl_init_userdefined(struct Curl_easy *data);

void Curl_freeset(struct Curl_easy *data);
CARLcode Curl_uc_to_carlcode(CARLUcode uc);
CARLcode Curl_close(struct Curl_easy **datap); /* opposite of carl_open() */
CARLcode Curl_connect(struct Curl_easy *, bool *async, bool *protocol_connect);
CARLcode Curl_disconnect(struct Curl_easy *data,
                         struct connectdata *, bool dead_connection);
CARLcode Curl_setup_conn(struct connectdata *conn,
                         bool *protocol_done);
void Curl_free_request_state(struct Curl_easy *data);
CARLcode Curl_parse_login_details(const char *login, const size_t len,
                                  char **userptr, char **passwdptr,
                                  char **optionsptr);

const struct Curl_handler *Curl_builtin_scheme(const char *scheme);

bool Curl_is_ASCII_name(const char *hostname);
CARLcode Curl_idnconvert_hostname(struct connectdata *conn,
                                  struct hostname *host);
void Curl_free_idnconverted_hostname(struct hostname *host);

#define CARL_DEFAULT_PROXY_PORT 1080 /* default proxy port unless specified */
#define CARL_DEFAULT_HTTPS_PROXY_PORT 443 /* default https proxy port unless
                                             specified */

#ifdef CARL_DISABLE_VERBOSE_STRINGS
#define Curl_verboseconnect(x,y)  Curl_nop_stmt
#else
void Curl_verboseconnect(struct Curl_easy *data, struct connectdata *conn);
#endif

#ifdef CARL_DISABLE_PROXY
#define CONNECT_PROXY_SSL() FALSE
#else

#define CONNECT_PROXY_SSL()\
  (conn->http_proxy.proxytype == CARLPROXY_HTTPS &&\
  !conn->bits.proxy_ssl_connected[sockindex])

#define CONNECT_FIRSTSOCKET_PROXY_SSL()\
  (conn->http_proxy.proxytype == CARLPROXY_HTTPS &&\
  !conn->bits.proxy_ssl_connected[FIRSTSOCKET])

#define CONNECT_SECONDARYSOCKET_PROXY_SSL()\
  (conn->http_proxy.proxytype == CARLPROXY_HTTPS &&\
  !conn->bits.proxy_ssl_connected[SECONDARYSOCKET])
#endif /* !CARL_DISABLE_PROXY */

#endif /* HEADER_CARL_URL_H */
