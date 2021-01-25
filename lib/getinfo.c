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

#include "carl_setup.h"

#include <carl/carl.h>

#include "urldata.h"
#include "getinfo.h"

#include "vtls/vtls.h"
#include "connect.h" /* Curl_getconnectinfo() */
#include "progress.h"

/* The last #include files should be: */
#include "carl_memory.h"
#include "memdebug.h"

/*
 * Initialize statistical and informational data.
 *
 * This function is called in carl_easy_reset, carl_easy_duphandle and at the
 * beginning of a perform session. It must reset the session-info variables,
 * in particular all variables in struct PureInfo.
 */
CARLcode Curl_initinfo(struct Curl_easy *data)
{
  struct Progress *pro = &data->progress;
  struct PureInfo *info = &data->info;

  pro->t_nslookup = 0;
  pro->t_connect = 0;
  pro->t_appconnect = 0;
  pro->t_pretransfer = 0;
  pro->t_starttransfer = 0;
  pro->timespent = 0;
  pro->t_redirect = 0;
  pro->is_t_startransfer_set = false;

  info->httpcode = 0;
  info->httpproxycode = 0;
  info->httpversion = 0;
  info->filetime = -1; /* -1 is an illegal time and thus means unknown */
  info->timecond = FALSE;

  info->header_size = 0;
  info->request_size = 0;
  info->proxyauthavail = 0;
  info->httpauthavail = 0;
  info->numconnects = 0;

  free(info->contenttype);
  info->contenttype = NULL;

  free(info->wouldredirect);
  info->wouldredirect = NULL;

  info->conn_primary_ip[0] = '\0';
  info->conn_local_ip[0] = '\0';
  info->conn_primary_port = 0;
  info->conn_local_port = 0;
  info->retry_after = 0;

  info->conn_scheme = 0;
  info->conn_protocol = 0;

#ifdef USE_SSL
  Curl_ssl_free_certinfo(data);
#endif
  return CARLE_OK;
}

static CARLcode getinfo_char(struct Curl_easy *data, CARLINFO info,
                             const char **param_charp)
{
  switch(info) {
  case CARLINFO_EFFECTIVE_URL:
    *param_charp = data->change.url?data->change.url:(char *)"";
    break;
  case CARLINFO_EFFECTIVE_METHOD: {
    const char *m = data->set.str[STRING_CUSTOMREQUEST];
    if(!m) {
      if(data->set.opt_no_body)
        m = "HEAD";
#ifndef CARL_DISABLE_HTTP
      else {
        switch(data->state.httpreq) {
        case HTTPREQ_POST:
        case HTTPREQ_POST_FORM:
        case HTTPREQ_POST_MIME:
          m = "POST";
          break;
        case HTTPREQ_PUT:
          m = "PUT";
          break;
        default: /* this should never happen */
        case HTTPREQ_GET:
          m = "GET";
          break;
        case HTTPREQ_HEAD:
          m = "HEAD";
          break;
        }
      }
#endif
    }
    *param_charp = m;
  }
    break;
  case CARLINFO_CONTENT_TYPE:
    *param_charp = data->info.contenttype;
    break;
  case CARLINFO_PRIVATE:
    *param_charp = (char *) data->set.private_data;
    break;
  case CARLINFO_FTP_ENTRY_PATH:
    /* Return the entrypath string from the most recent connection.
       This pointer was copied from the connectdata structure by FTP.
       The actual string may be free()ed by subsequent libcarl calls so
       it must be copied to a safer area before the next libcarl call.
       Callers must never free it themselves. */
    *param_charp = data->state.most_recent_ftp_entrypath;
    break;
  case CARLINFO_REDIRECT_URL:
    /* Return the URL this request would have been redirected to if that
       option had been enabled! */
    *param_charp = data->info.wouldredirect;
    break;
  case CARLINFO_PRIMARY_IP:
    /* Return the ip address of the most recent (primary) connection */
    *param_charp = data->info.conn_primary_ip;
    break;
  case CARLINFO_LOCAL_IP:
    /* Return the source/local ip address of the most recent (primary)
       connection */
    *param_charp = data->info.conn_local_ip;
    break;
  case CARLINFO_RTSP_SESSION_ID:
    *param_charp = data->set.str[STRING_RTSP_SESSION_ID];
    break;
  case CARLINFO_SCHEME:
    *param_charp = data->info.conn_scheme;
    break;

  default:
    return CARLE_UNKNOWN_OPTION;
  }

  return CARLE_OK;
}

static CARLcode getinfo_long(struct Curl_easy *data, CARLINFO info,
                             long *param_longp)
{
  carl_socket_t sockfd;

  union {
    unsigned long *to_ulong;
    long          *to_long;
  } lptr;

#ifdef DEBUGBUILD
  char *timestr = getenv("CARL_TIME");
  if(timestr) {
    unsigned long val = strtol(timestr, NULL, 10);
    switch(info) {
    case CARLINFO_LOCAL_PORT:
      *param_longp = (long)val;
      return CARLE_OK;
    default:
      break;
    }
  }
  /* use another variable for this to allow different values */
  timestr = getenv("CARL_DEBUG_SIZE");
  if(timestr) {
    unsigned long val = strtol(timestr, NULL, 10);
    switch(info) {
    case CARLINFO_HEADER_SIZE:
    case CARLINFO_REQUEST_SIZE:
      *param_longp = (long)val;
      return CARLE_OK;
    default:
      break;
    }
  }
#endif

  switch(info) {
  case CARLINFO_RESPONSE_CODE:
    *param_longp = data->info.httpcode;
    break;
  case CARLINFO_HTTP_CONNECTCODE:
    *param_longp = data->info.httpproxycode;
    break;
  case CARLINFO_FILETIME:
    if(data->info.filetime > LONG_MAX)
      *param_longp = LONG_MAX;
    else if(data->info.filetime < LONG_MIN)
      *param_longp = LONG_MIN;
    else
      *param_longp = (long)data->info.filetime;
    break;
  case CARLINFO_HEADER_SIZE:
    *param_longp = (long)data->info.header_size;
    break;
  case CARLINFO_REQUEST_SIZE:
    *param_longp = (long)data->info.request_size;
    break;
  case CARLINFO_SSL_VERIFYRESULT:
    *param_longp = data->set.ssl.certverifyresult;
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLINFO_PROXY_SSL_VERIFYRESULT:
    *param_longp = data->set.proxy_ssl.certverifyresult;
    break;
#endif
  case CARLINFO_REDIRECT_COUNT:
    *param_longp = data->set.followlocation;
    break;
  case CARLINFO_HTTPAUTH_AVAIL:
    lptr.to_long = param_longp;
    *lptr.to_ulong = data->info.httpauthavail;
    break;
  case CARLINFO_PROXYAUTH_AVAIL:
    lptr.to_long = param_longp;
    *lptr.to_ulong = data->info.proxyauthavail;
    break;
  case CARLINFO_OS_ERRNO:
    *param_longp = data->state.os_errno;
    break;
  case CARLINFO_NUM_CONNECTS:
    *param_longp = data->info.numconnects;
    break;
  case CARLINFO_LASTSOCKET:
    sockfd = Curl_getconnectinfo(data, NULL);

    /* note: this is not a good conversion for systems with 64 bit sockets and
       32 bit longs */
    if(sockfd != CARL_SOCKET_BAD)
      *param_longp = (long)sockfd;
    else
      /* this interface is documented to return -1 in case of badness, which
         may not be the same as the CARL_SOCKET_BAD value */
      *param_longp = -1;
    break;
  case CARLINFO_PRIMARY_PORT:
    /* Return the (remote) port of the most recent (primary) connection */
    *param_longp = data->info.conn_primary_port;
    break;
  case CARLINFO_LOCAL_PORT:
    /* Return the local port of the most recent (primary) connection */
    *param_longp = data->info.conn_local_port;
    break;
  case CARLINFO_PROXY_ERROR:
    *param_longp = (long)data->info.pxcode;
    break;
  case CARLINFO_CONDITION_UNMET:
    if(data->info.httpcode == 304)
      *param_longp = 1L;
    else
      /* return if the condition prevented the document to get transferred */
      *param_longp = data->info.timecond ? 1L : 0L;
    break;
  case CARLINFO_RTSP_CLIENT_CSEQ:
    *param_longp = data->state.rtsp_next_client_CSeq;
    break;
  case CARLINFO_RTSP_SERVER_CSEQ:
    *param_longp = data->state.rtsp_next_server_CSeq;
    break;
  case CARLINFO_RTSP_CSEQ_RECV:
    *param_longp = data->state.rtsp_CSeq_recv;
    break;
  case CARLINFO_HTTP_VERSION:
    switch(data->info.httpversion) {
    case 10:
      *param_longp = CARL_HTTP_VERSION_1_0;
      break;
    case 11:
      *param_longp = CARL_HTTP_VERSION_1_1;
      break;
    case 20:
      *param_longp = CARL_HTTP_VERSION_2_0;
      break;
    case 30:
      *param_longp = CARL_HTTP_VERSION_3;
      break;
    default:
      *param_longp = CARL_HTTP_VERSION_NONE;
      break;
    }
    break;
  case CARLINFO_PROTOCOL:
    *param_longp = data->info.conn_protocol;
    break;
  default:
    return CARLE_UNKNOWN_OPTION;
  }

  return CARLE_OK;
}

#define DOUBLE_SECS(x) (double)(x)/1000000

static CARLcode getinfo_offt(struct Curl_easy *data, CARLINFO info,
                             carl_off_t *param_offt)
{
#ifdef DEBUGBUILD
  char *timestr = getenv("CARL_TIME");
  if(timestr) {
    unsigned long val = strtol(timestr, NULL, 10);
    switch(info) {
    case CARLINFO_TOTAL_TIME_T:
    case CARLINFO_NAMELOOKUP_TIME_T:
    case CARLINFO_CONNECT_TIME_T:
    case CARLINFO_APPCONNECT_TIME_T:
    case CARLINFO_PRETRANSFER_TIME_T:
    case CARLINFO_STARTTRANSFER_TIME_T:
    case CARLINFO_REDIRECT_TIME_T:
    case CARLINFO_SPEED_DOWNLOAD_T:
    case CARLINFO_SPEED_UPLOAD_T:
      *param_offt = (carl_off_t)val;
      return CARLE_OK;
    default:
      break;
    }
  }
#endif
  switch(info) {
  case CARLINFO_FILETIME_T:
    *param_offt = (carl_off_t)data->info.filetime;
    break;
  case CARLINFO_SIZE_UPLOAD_T:
    *param_offt = data->progress.uploaded;
    break;
  case CARLINFO_SIZE_DOWNLOAD_T:
    *param_offt = data->progress.downloaded;
    break;
  case CARLINFO_SPEED_DOWNLOAD_T:
    *param_offt = data->progress.dlspeed;
    break;
  case CARLINFO_SPEED_UPLOAD_T:
    *param_offt = data->progress.ulspeed;
    break;
  case CARLINFO_CONTENT_LENGTH_DOWNLOAD_T:
    *param_offt = (data->progress.flags & PGRS_DL_SIZE_KNOWN)?
      data->progress.size_dl:-1;
    break;
  case CARLINFO_CONTENT_LENGTH_UPLOAD_T:
    *param_offt = (data->progress.flags & PGRS_UL_SIZE_KNOWN)?
      data->progress.size_ul:-1;
    break;
   case CARLINFO_TOTAL_TIME_T:
    *param_offt = data->progress.timespent;
    break;
  case CARLINFO_NAMELOOKUP_TIME_T:
    *param_offt = data->progress.t_nslookup;
    break;
  case CARLINFO_CONNECT_TIME_T:
    *param_offt = data->progress.t_connect;
    break;
  case CARLINFO_APPCONNECT_TIME_T:
    *param_offt = data->progress.t_appconnect;
    break;
  case CARLINFO_PRETRANSFER_TIME_T:
    *param_offt = data->progress.t_pretransfer;
    break;
  case CARLINFO_STARTTRANSFER_TIME_T:
    *param_offt = data->progress.t_starttransfer;
    break;
  case CARLINFO_REDIRECT_TIME_T:
    *param_offt = data->progress.t_redirect;
    break;
  case CARLINFO_RETRY_AFTER:
    *param_offt = data->info.retry_after;
    break;
  default:
    return CARLE_UNKNOWN_OPTION;
  }

  return CARLE_OK;
}

static CARLcode getinfo_double(struct Curl_easy *data, CARLINFO info,
                               double *param_doublep)
{
#ifdef DEBUGBUILD
  char *timestr = getenv("CARL_TIME");
  if(timestr) {
    unsigned long val = strtol(timestr, NULL, 10);
    switch(info) {
    case CARLINFO_TOTAL_TIME:
    case CARLINFO_NAMELOOKUP_TIME:
    case CARLINFO_CONNECT_TIME:
    case CARLINFO_APPCONNECT_TIME:
    case CARLINFO_PRETRANSFER_TIME:
    case CARLINFO_STARTTRANSFER_TIME:
    case CARLINFO_REDIRECT_TIME:
    case CARLINFO_SPEED_DOWNLOAD:
    case CARLINFO_SPEED_UPLOAD:
      *param_doublep = (double)val;
      return CARLE_OK;
    default:
      break;
    }
  }
#endif
  switch(info) {
  case CARLINFO_TOTAL_TIME:
    *param_doublep = DOUBLE_SECS(data->progress.timespent);
    break;
  case CARLINFO_NAMELOOKUP_TIME:
    *param_doublep = DOUBLE_SECS(data->progress.t_nslookup);
    break;
  case CARLINFO_CONNECT_TIME:
    *param_doublep = DOUBLE_SECS(data->progress.t_connect);
    break;
  case CARLINFO_APPCONNECT_TIME:
    *param_doublep = DOUBLE_SECS(data->progress.t_appconnect);
    break;
  case CARLINFO_PRETRANSFER_TIME:
    *param_doublep = DOUBLE_SECS(data->progress.t_pretransfer);
    break;
  case CARLINFO_STARTTRANSFER_TIME:
    *param_doublep = DOUBLE_SECS(data->progress.t_starttransfer);
    break;
  case CARLINFO_SIZE_UPLOAD:
    *param_doublep = (double)data->progress.uploaded;
    break;
  case CARLINFO_SIZE_DOWNLOAD:
    *param_doublep = (double)data->progress.downloaded;
    break;
  case CARLINFO_SPEED_DOWNLOAD:
    *param_doublep = (double)data->progress.dlspeed;
    break;
  case CARLINFO_SPEED_UPLOAD:
    *param_doublep = (double)data->progress.ulspeed;
    break;
  case CARLINFO_CONTENT_LENGTH_DOWNLOAD:
    *param_doublep = (data->progress.flags & PGRS_DL_SIZE_KNOWN)?
      (double)data->progress.size_dl:-1;
    break;
  case CARLINFO_CONTENT_LENGTH_UPLOAD:
    *param_doublep = (data->progress.flags & PGRS_UL_SIZE_KNOWN)?
      (double)data->progress.size_ul:-1;
    break;
  case CARLINFO_REDIRECT_TIME:
    *param_doublep = DOUBLE_SECS(data->progress.t_redirect);
    break;

  default:
    return CARLE_UNKNOWN_OPTION;
  }

  return CARLE_OK;
}

static CARLcode getinfo_slist(struct Curl_easy *data, CARLINFO info,
                              struct carl_slist **param_slistp)
{
  union {
    struct carl_certinfo *to_certinfo;
    struct carl_slist    *to_slist;
  } ptr;

  switch(info) {
  case CARLINFO_SSL_ENGINES:
    *param_slistp = Curl_ssl_engines_list(data);
    break;
  case CARLINFO_COOKIELIST:
    *param_slistp = Curl_cookie_list(data);
    break;
  case CARLINFO_CERTINFO:
    /* Return the a pointer to the certinfo struct. Not really an slist
       pointer but we can pretend it is here */
    ptr.to_certinfo = &data->info.certs;
    *param_slistp = ptr.to_slist;
    break;
  case CARLINFO_TLS_SESSION:
  case CARLINFO_TLS_SSL_PTR:
    {
      struct carl_tlssessioninfo **tsip = (struct carl_tlssessioninfo **)
                                          param_slistp;
      struct carl_tlssessioninfo *tsi = &data->tsi;
#ifdef USE_SSL
      struct connectdata *conn = data->conn;
#endif

      *tsip = tsi;
      tsi->backend = Curl_ssl_backend();
      tsi->internals = NULL;

#ifdef USE_SSL
      if(conn && tsi->backend != CARLSSLBACKEND_NONE) {
        unsigned int i;
        for(i = 0; i < (sizeof(conn->ssl) / sizeof(conn->ssl[0])); ++i) {
          if(conn->ssl[i].use) {
            tsi->internals = Curl_ssl->get_internals(&conn->ssl[i], info);
            break;
          }
        }
      }
#endif
    }
    break;
  default:
    return CARLE_UNKNOWN_OPTION;
  }

  return CARLE_OK;
}

static CARLcode getinfo_socket(struct Curl_easy *data, CARLINFO info,
                               carl_socket_t *param_socketp)
{
  switch(info) {
  case CARLINFO_ACTIVESOCKET:
    *param_socketp = Curl_getconnectinfo(data, NULL);
    break;
  default:
    return CARLE_UNKNOWN_OPTION;
  }

  return CARLE_OK;
}

CARLcode Curl_getinfo(struct Curl_easy *data, CARLINFO info, ...)
{
  va_list arg;
  long *param_longp = NULL;
  double *param_doublep = NULL;
  carl_off_t *param_offt = NULL;
  const char **param_charp = NULL;
  struct carl_slist **param_slistp = NULL;
  carl_socket_t *param_socketp = NULL;
  int type;
  CARLcode result = CARLE_UNKNOWN_OPTION;

  if(!data)
    return result;

  va_start(arg, info);

  type = CARLINFO_TYPEMASK & (int)info;
  switch(type) {
  case CARLINFO_STRING:
    param_charp = va_arg(arg, const char **);
    if(param_charp)
      result = getinfo_char(data, info, param_charp);
    break;
  case CARLINFO_LONG:
    param_longp = va_arg(arg, long *);
    if(param_longp)
      result = getinfo_long(data, info, param_longp);
    break;
  case CARLINFO_DOUBLE:
    param_doublep = va_arg(arg, double *);
    if(param_doublep)
      result = getinfo_double(data, info, param_doublep);
    break;
  case CARLINFO_OFF_T:
    param_offt = va_arg(arg, carl_off_t *);
    if(param_offt)
      result = getinfo_offt(data, info, param_offt);
    break;
  case CARLINFO_SLIST:
    param_slistp = va_arg(arg, struct carl_slist **);
    if(param_slistp)
      result = getinfo_slist(data, info, param_slistp);
    break;
  case CARLINFO_SOCKET:
    param_socketp = va_arg(arg, carl_socket_t *);
    if(param_socketp)
      result = getinfo_socket(data, info, param_socketp);
    break;
  default:
    break;
  }

  va_end(arg);

  return result;
}
