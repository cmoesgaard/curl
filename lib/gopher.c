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

#ifndef CARL_DISABLE_GOPHER

#include "urldata.h"
#include <carl/carl.h>
#include "transfer.h"
#include "sendf.h"
#include "connect.h"
#include "progress.h"
#include "gopher.h"
#include "select.h"
#include "strdup.h"
#include "vtls/vtls.h"
#include "url.h"
#include "escape.h"
#include "warnless.h"
#include "carl_printf.h"
#include "carl_memory.h"
/* The last #include file should be: */
#include "memdebug.h"

/*
 * Forward declarations.
 */

static CARLcode gopher_do(struct Curl_easy *data, bool *done);
#ifdef USE_SSL
static CARLcode gopher_connect(struct Curl_easy *data, bool *done);
static CARLcode gopher_connecting(struct Curl_easy *data, bool *done);
#endif

/*
 * Gopher protocol handler.
 * This is also a nice simple template to build off for simple
 * connect-command-download protocols.
 */

const struct Curl_handler Curl_handler_gopher = {
  "GOPHER",                             /* scheme */
  ZERO_NULL,                            /* setup_connection */
  gopher_do,                            /* do_it */
  ZERO_NULL,                            /* done */
  ZERO_NULL,                            /* do_more */
  ZERO_NULL,                            /* connect_it */
  ZERO_NULL,                            /* connecting */
  ZERO_NULL,                            /* doing */
  ZERO_NULL,                            /* proto_getsock */
  ZERO_NULL,                            /* doing_getsock */
  ZERO_NULL,                            /* domore_getsock */
  ZERO_NULL,                            /* perform_getsock */
  ZERO_NULL,                            /* disconnect */
  ZERO_NULL,                            /* readwrite */
  ZERO_NULL,                            /* connection_check */
  PORT_GOPHER,                          /* defport */
  CARLPROTO_GOPHER,                     /* protocol */
  CARLPROTO_GOPHER,                     /* family */
  PROTOPT_NONE                          /* flags */
};

#ifdef USE_SSL
const struct Curl_handler Curl_handler_gophers = {
  "GOPHERS",                            /* scheme */
  ZERO_NULL,                            /* setup_connection */
  gopher_do,                            /* do_it */
  ZERO_NULL,                            /* done */
  ZERO_NULL,                            /* do_more */
  gopher_connect,                       /* connect_it */
  gopher_connecting,                    /* connecting */
  ZERO_NULL,                            /* doing */
  ZERO_NULL,                            /* proto_getsock */
  ZERO_NULL,                            /* doing_getsock */
  ZERO_NULL,                            /* domore_getsock */
  ZERO_NULL,                            /* perform_getsock */
  ZERO_NULL,                            /* disconnect */
  ZERO_NULL,                            /* readwrite */
  ZERO_NULL,                            /* connection_check */
  PORT_GOPHER,                          /* defport */
  CARLPROTO_GOPHERS,                    /* protocol */
  CARLPROTO_GOPHER,                     /* family */
  PROTOPT_SSL                           /* flags */
};

static CARLcode gopher_connect(struct Curl_easy *data, bool *done)
{
  (void)data;
  (void)done;
  return CARLE_OK;
}

static CARLcode gopher_connecting(struct Curl_easy *data, bool *done)
{
  struct connectdata *conn = data->conn;
  CARLcode result = Curl_ssl_connect(data, conn, FIRSTSOCKET);
  if(result)
    connclose(conn, "Failed TLS connection");
  *done = TRUE;
  return result;
}
#endif

static CARLcode gopher_do(struct Curl_easy *data, bool *done)
{
  CARLcode result = CARLE_OK;
  struct connectdata *conn = data->conn;
  carl_socket_t sockfd = conn->sock[FIRSTSOCKET];
  char *gopherpath;
  char *path = data->state.up.path;
  char *query = data->state.up.query;
  char *sel = NULL;
  char *sel_org = NULL;
  timediff_t timeout_ms;
  ssize_t amount, k;
  size_t len;
  int what;

  *done = TRUE; /* unconditionally */

  /* path is guaranteed non-NULL */
  DEBUGASSERT(path);

  if(query)
    gopherpath = aprintf("%s?%s", path, query);
  else
    gopherpath = strdup(path);

  if(!gopherpath)
    return CARLE_OUT_OF_MEMORY;

  /* Create selector. Degenerate cases: / and /1 => convert to "" */
  if(strlen(gopherpath) <= 2) {
    sel = (char *)"";
    len = strlen(sel);
    free(gopherpath);
  }
  else {
    char *newp;

    /* Otherwise, drop / and the first character (i.e., item type) ... */
    newp = gopherpath;
    newp += 2;

    /* ... and finally unescape */
    result = Curl_urldecode(data, newp, 0, &sel, &len, REJECT_ZERO);
    free(gopherpath);
    if(result)
      return result;
    sel_org = sel;
  }

  k = carlx_uztosz(len);

  for(;;) {
    /* Break out of the loop if the selector is empty because OpenSSL and/or
       LibreSSL fail with errno 0 if this is the case. */
    if(strlen(sel) < 1)
      break;

    result = Curl_write(data, sockfd, sel, k, &amount);
    if(!result) { /* Which may not have written it all! */
      result = Curl_client_write(data, CLIENTWRITE_HEADER, sel, amount);
      if(result)
        break;

      k -= amount;
      sel += amount;
      if(k < 1)
        break; /* but it did write it all */
    }
    else
      break;

    timeout_ms = Curl_timeleft(data, NULL, FALSE);
    if(timeout_ms < 0) {
      result = CARLE_OPERATION_TIMEDOUT;
      break;
    }
    if(!timeout_ms)
      timeout_ms = TIMEDIFF_T_MAX;

    /* Don't busyloop. The entire loop thing is a work-around as it causes a
       BLOCKING behavior which is a NO-NO. This function should rather be
       split up in a do and a doing piece where the pieces that aren't
       possible to send now will be sent in the doing function repeatedly
       until the entire request is sent.
    */
    what = SOCKET_WRITABLE(sockfd, timeout_ms);
    if(what < 0) {
      result = CARLE_SEND_ERROR;
      break;
    }
    else if(!what) {
      result = CARLE_OPERATION_TIMEDOUT;
      break;
    }
  }

  free(sel_org);

  if(!result)
    result = Curl_write(data, sockfd, "\r\n", 2, &amount);
  if(result) {
    failf(data, "Failed sending Gopher request");
    return result;
  }
  result = Curl_client_write(data, CLIENTWRITE_HEADER, (char *)"\r\n", 2);
  if(result)
    return result;

  Curl_setup_transfer(data, FIRSTSOCKET, -1, FALSE, -1);
  return CARLE_OK;
}
#endif /*CARL_DISABLE_GOPHER*/
