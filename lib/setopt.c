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

#include <limits.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_LINUX_TCP_H
#include <linux/tcp.h>
#elif defined(HAVE_NETINET_TCP_H)
#include <netinet/tcp.h>
#endif

#include "urldata.h"
#include "url.h"
#include "progress.h"
#include "content_encoding.h"
#include "strcase.h"
#include "share.h"
#include "vtls/vtls.h"
#include "warnless.h"
#include "sendf.h"
#include "http2.h"
#include "setopt.h"
#include "multiif.h"
#include "altsvc.h"
#include "hsts.h"

/* The last 3 #include files should be in this order */
#include "carl_printf.h"
#include "carl_memory.h"
#include "memdebug.h"

CARLcode Curl_setstropt(char **charp, const char *s)
{
  /* Release the previous storage at `charp' and replace by a dynamic storage
     copy of `s'. Return CARLE_OK or CARLE_OUT_OF_MEMORY. */

  Curl_safefree(*charp);

  if(s) {
    char *str = strdup(s);

    if(str) {
      size_t len = strlen(str);
      if(len > CARL_MAX_INPUT_LENGTH) {
        free(str);
        return CARLE_BAD_FUNCTION_ARGUMENT;
      }
    }
    if(!str)
      return CARLE_OUT_OF_MEMORY;

    *charp = str;
  }

  return CARLE_OK;
}

CARLcode Curl_setblobopt(struct carl_blob **blobp,
                         const struct carl_blob *blob)
{
  /* free the previous storage at `blobp' and replace by a dynamic storage
     copy of blob. If CARL_BLOB_COPY is set, the data is copied. */

  Curl_safefree(*blobp);

  if(blob) {
    struct carl_blob *nblob;
    if(blob->len > CARL_MAX_INPUT_LENGTH)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    nblob = (struct carl_blob *)
      malloc(sizeof(struct carl_blob) +
             ((blob->flags & CARL_BLOB_COPY) ? blob->len : 0));
    if(!nblob)
      return CARLE_OUT_OF_MEMORY;
    *nblob = *blob;
    if(blob->flags & CARL_BLOB_COPY) {
      /* put the data after the blob struct in memory */
      nblob->data = (char *)nblob + sizeof(struct carl_blob);
      memcpy(nblob->data, blob->data, blob->len);
    }

    *blobp = nblob;
    return CARLE_OK;
  }

  return CARLE_OK;
}

static CARLcode setstropt_userpwd(char *option, char **userp, char **passwdp)
{
  CARLcode result = CARLE_OK;
  char *user = NULL;
  char *passwd = NULL;

  /* Parse the login details if specified. It not then we treat NULL as a hint
     to clear the existing data */
  if(option) {
    result = Curl_parse_login_details(option, strlen(option),
                                      (userp ? &user : NULL),
                                      (passwdp ? &passwd : NULL),
                                      NULL);
  }

  if(!result) {
    /* Store the username part of option if required */
    if(userp) {
      if(!user && option && option[0] == ':') {
        /* Allocate an empty string instead of returning NULL as user name */
        user = strdup("");
        if(!user)
          result = CARLE_OUT_OF_MEMORY;
      }

      Curl_safefree(*userp);
      *userp = user;
    }

    /* Store the password part of option if required */
    if(passwdp) {
      Curl_safefree(*passwdp);
      *passwdp = passwd;
    }
  }

  return result;
}

#define C_SSLVERSION_VALUE(x) (x & 0xffff)
#define C_SSLVERSION_MAX_VALUE(x) (x & 0xffff0000)

/*
 * Do not make Curl_vsetopt() static: it is called from
 * packages/OS400/ccsidcarl.c.
 */
CARLcode Curl_vsetopt(struct Curl_easy *data, CARLoption option, va_list param)
{
  char *argptr;
  CARLcode result = CARLE_OK;
  long arg;
  unsigned long uarg;
  carl_off_t bigsize;

  switch(option) {
  case CARLOPT_DNS_CACHE_TIMEOUT:
    arg = va_arg(param, long);
    if(arg < -1)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.dns_cache_timeout = arg;
    break;
  case CARLOPT_DNS_USE_GLOBAL_CACHE:
    /* deprecated */
    break;
  case CARLOPT_SSL_CIPHER_LIST:
    /* set a list of cipher we want to use in the SSL connection */
    result = Curl_setstropt(&data->set.str[STRING_SSL_CIPHER_LIST_ORIG],
                            va_arg(param, char *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSL_CIPHER_LIST:
    /* set a list of cipher we want to use in the SSL connection for proxy */
    result = Curl_setstropt(&data->set.str[STRING_SSL_CIPHER_LIST_PROXY],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_TLS13_CIPHERS:
    if(Curl_ssl_tls13_ciphersuites()) {
      /* set preferred list of TLS 1.3 cipher suites */
      result = Curl_setstropt(&data->set.str[STRING_SSL_CIPHER13_LIST_ORIG],
                              va_arg(param, char *));
    }
    else
      return CARLE_NOT_BUILT_IN;
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_TLS13_CIPHERS:
    if(Curl_ssl_tls13_ciphersuites()) {
      /* set preferred list of TLS 1.3 cipher suites for proxy */
      result = Curl_setstropt(&data->set.str[STRING_SSL_CIPHER13_LIST_PROXY],
                              va_arg(param, char *));
    }
    else
      return CARLE_NOT_BUILT_IN;
    break;
#endif
  case CARLOPT_RANDOM_FILE:
    /*
     * This is the path name to a file that contains random data to seed
     * the random SSL stuff with. The file is only used for reading.
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_RANDOM_FILE],
                            va_arg(param, char *));
    break;
  case CARLOPT_EGDSOCKET:
    /*
     * The Entropy Gathering Daemon socket pathname
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_EGDSOCKET],
                            va_arg(param, char *));
    break;
  case CARLOPT_MAXCONNECTS:
    /*
     * Set the absolute number of maximum simultaneous alive connection that
     * libcarl is allowed to have.
     */
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.maxconnects = arg;
    break;
  case CARLOPT_FORBID_REUSE:
    /*
     * When this transfer is done, it must not be left to be reused by a
     * subsequent transfer but shall be closed immediately.
     */
    data->set.reuse_forbid = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_FRESH_CONNECT:
    /*
     * This transfer shall not use a previously cached connection but
     * should be made with a fresh new connect!
     */
    data->set.reuse_fresh = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_VERBOSE:
    /*
     * Verbose means infof() calls that give a lot of information about
     * the connection and transfer procedures as well as internal choices.
     */
    data->set.verbose = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_HEADER:
    /*
     * Set to include the header in the general data output stream.
     */
    data->set.include_header = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_NOPROGRESS:
    /*
     * Shut off the internal supported progress meter
     */
    data->set.hide_progress = (0 != va_arg(param, long)) ? TRUE : FALSE;
    if(data->set.hide_progress)
      data->progress.flags |= PGRS_HIDE;
    else
      data->progress.flags &= ~PGRS_HIDE;
    break;
  case CARLOPT_NOBODY:
    /*
     * Do not include the body part in the output data stream.
     */
    data->set.opt_no_body = (0 != va_arg(param, long)) ? TRUE : FALSE;
#ifndef CARL_DISABLE_HTTP
    if(data->set.opt_no_body)
      /* in HTTP lingo, no body means using the HEAD request... */
      data->set.method = HTTPREQ_HEAD;
    else if(data->set.method == HTTPREQ_HEAD)
      data->set.method = HTTPREQ_GET;
#endif
    break;
  case CARLOPT_FAILONERROR:
    /*
     * Don't output the >=400 error code HTML-page, but instead only
     * return error.
     */
    data->set.http_fail_on_error = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_KEEP_SENDING_ON_ERROR:
    data->set.http_keep_sending_on_error = (0 != va_arg(param, long)) ?
      TRUE : FALSE;
    break;
  case CARLOPT_UPLOAD:
  case CARLOPT_PUT:
    /*
     * We want to sent data to the remote host. If this is HTTP, that equals
     * using the PUT request.
     */
    data->set.upload = (0 != va_arg(param, long)) ? TRUE : FALSE;
    if(data->set.upload) {
      /* If this is HTTP, PUT is what's needed to "upload" */
      data->set.method = HTTPREQ_PUT;
      data->set.opt_no_body = FALSE; /* this is implied */
    }
    else
      /* In HTTP, the opposite of upload is GET (unless NOBODY is true as
         then this can be changed to HEAD later on) */
      data->set.method = HTTPREQ_GET;
    break;
  case CARLOPT_REQUEST_TARGET:
    result = Curl_setstropt(&data->set.str[STRING_TARGET],
                            va_arg(param, char *));
    break;
  case CARLOPT_FILETIME:
    /*
     * Try to get the file time of the remote document. The time will
     * later (possibly) become available using carl_easy_getinfo().
     */
    data->set.get_filetime = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_SERVER_RESPONSE_TIMEOUT:
    /*
     * Option that specifies how quickly an server response must be obtained
     * before it is considered failure. For pingpong protocols.
     */
    arg = va_arg(param, long);
    if((arg >= 0) && (arg <= (INT_MAX/1000)))
      data->set.server_response_timeout = arg * 1000;
    else
      return CARLE_BAD_FUNCTION_ARGUMENT;
    break;
#ifndef CARL_DISABLE_TFTP
  case CARLOPT_TFTP_NO_OPTIONS:
    /*
     * Option that prevents libcarl from sending TFTP option requests to the
     * server.
     */
    data->set.tftp_no_options = va_arg(param, long) != 0;
    break;
  case CARLOPT_TFTP_BLKSIZE:
    /*
     * TFTP option that specifies the block size to use for data transmission.
     */
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.tftp_blksize = arg;
    break;
#endif
#ifndef CARL_DISABLE_NETRC
  case CARLOPT_NETRC:
    /*
     * Parse the $HOME/.netrc file
     */
    arg = va_arg(param, long);
    if((arg < CARL_NETRC_IGNORED) || (arg >= CARL_NETRC_LAST))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.use_netrc = (enum CARL_NETRC_OPTION)arg;
    break;
  case CARLOPT_NETRC_FILE:
    /*
     * Use this file instead of the $HOME/.netrc file
     */
    result = Curl_setstropt(&data->set.str[STRING_NETRC_FILE],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_TRANSFERTEXT:
    /*
     * This option was previously named 'FTPASCII'. Renamed to work with
     * more protocols than merely FTP.
     *
     * Transfer using ASCII (instead of BINARY).
     */
    data->set.prefer_ascii = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_TIMECONDITION:
    /*
     * Set HTTP time condition. This must be one of the defines in the
     * carl/carl.h header file.
     */
    arg = va_arg(param, long);
    if((arg < CARL_TIMECOND_NONE) || (arg >= CARL_TIMECOND_LAST))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.timecondition = (carl_TimeCond)arg;
    break;
  case CARLOPT_TIMEVALUE:
    /*
     * This is the value to compare with the remote document with the
     * method set with CARLOPT_TIMECONDITION
     */
    data->set.timevalue = (time_t)va_arg(param, long);
    break;

  case CARLOPT_TIMEVALUE_LARGE:
    /*
     * This is the value to compare with the remote document with the
     * method set with CARLOPT_TIMECONDITION
     */
    data->set.timevalue = (time_t)va_arg(param, carl_off_t);
    break;

  case CARLOPT_SSLVERSION:
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSLVERSION:
#endif
    /*
     * Set explicit SSL version to try to connect with, as some SSL
     * implementations are lame.
     */
#ifdef USE_SSL
    {
      long version, version_max;
      struct ssl_primary_config *primary = &data->set.ssl.primary;
#ifndef CARL_DISABLE_PROXY
      if(option != CARLOPT_SSLVERSION)
        primary = &data->set.proxy_ssl.primary;
#endif

      arg = va_arg(param, long);

      version = C_SSLVERSION_VALUE(arg);
      version_max = C_SSLVERSION_MAX_VALUE(arg);

      if(version < CARL_SSLVERSION_DEFAULT ||
         version >= CARL_SSLVERSION_LAST ||
         version_max < CARL_SSLVERSION_MAX_NONE ||
         version_max >= CARL_SSLVERSION_MAX_LAST)
        return CARLE_BAD_FUNCTION_ARGUMENT;

      primary->version = version;
      primary->version_max = version_max;
    }
#else
    result = CARLE_NOT_BUILT_IN;
#endif
    break;

    /* MQTT "borrows" some of the HTTP options */
#if !defined(CARL_DISABLE_HTTP) || !defined(CARL_DISABLE_MQTT)
  case CARLOPT_COPYPOSTFIELDS:
    /*
     * A string with POST data. Makes carl HTTP POST. Even if it is NULL.
     * If needed, CARLOPT_POSTFIELDSIZE must have been set prior to
     *  CARLOPT_COPYPOSTFIELDS and not altered later.
     */
    argptr = va_arg(param, char *);

    if(!argptr || data->set.postfieldsize == -1)
      result = Curl_setstropt(&data->set.str[STRING_COPYPOSTFIELDS], argptr);
    else {
      /*
       *  Check that requested length does not overflow the size_t type.
       */

      if((data->set.postfieldsize < 0) ||
         ((sizeof(carl_off_t) != sizeof(size_t)) &&
          (data->set.postfieldsize > (carl_off_t)((size_t)-1))))
        result = CARLE_OUT_OF_MEMORY;
      else {
        char *p;

        (void) Curl_setstropt(&data->set.str[STRING_COPYPOSTFIELDS], NULL);

        /* Allocate even when size == 0. This satisfies the need of possible
           later address compare to detect the COPYPOSTFIELDS mode, and
           to mark that postfields is used rather than read function or
           form data.
        */
        p = malloc((size_t)(data->set.postfieldsize?
                            data->set.postfieldsize:1));

        if(!p)
          result = CARLE_OUT_OF_MEMORY;
        else {
          if(data->set.postfieldsize)
            memcpy(p, argptr, (size_t)data->set.postfieldsize);

          data->set.str[STRING_COPYPOSTFIELDS] = p;
        }
      }
    }

    data->set.postfields = data->set.str[STRING_COPYPOSTFIELDS];
    data->set.method = HTTPREQ_POST;
    break;

  case CARLOPT_POSTFIELDS:
    /*
     * Like above, but use static data instead of copying it.
     */
    data->set.postfields = va_arg(param, void *);
    /* Release old copied data. */
    (void) Curl_setstropt(&data->set.str[STRING_COPYPOSTFIELDS], NULL);
    data->set.method = HTTPREQ_POST;
    break;

  case CARLOPT_POSTFIELDSIZE:
    /*
     * The size of the POSTFIELD data to prevent libcarl to do strlen() to
     * figure it out. Enables binary posts.
     */
    bigsize = va_arg(param, long);
    if(bigsize < -1)
      return CARLE_BAD_FUNCTION_ARGUMENT;

    if(data->set.postfieldsize < bigsize &&
       data->set.postfields == data->set.str[STRING_COPYPOSTFIELDS]) {
      /* Previous CARLOPT_COPYPOSTFIELDS is no longer valid. */
      (void) Curl_setstropt(&data->set.str[STRING_COPYPOSTFIELDS], NULL);
      data->set.postfields = NULL;
    }

    data->set.postfieldsize = bigsize;
    break;

  case CARLOPT_POSTFIELDSIZE_LARGE:
    /*
     * The size of the POSTFIELD data to prevent libcarl to do strlen() to
     * figure it out. Enables binary posts.
     */
    bigsize = va_arg(param, carl_off_t);
    if(bigsize < -1)
      return CARLE_BAD_FUNCTION_ARGUMENT;

    if(data->set.postfieldsize < bigsize &&
       data->set.postfields == data->set.str[STRING_COPYPOSTFIELDS]) {
      /* Previous CARLOPT_COPYPOSTFIELDS is no longer valid. */
      (void) Curl_setstropt(&data->set.str[STRING_COPYPOSTFIELDS], NULL);
      data->set.postfields = NULL;
    }

    data->set.postfieldsize = bigsize;
    break;
#endif
#ifndef CARL_DISABLE_HTTP
  case CARLOPT_AUTOREFERER:
    /*
     * Switch on automatic referer that gets set if carl follows locations.
     */
    data->set.http_auto_referer = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_ACCEPT_ENCODING:
    /*
     * String to use at the value of Accept-Encoding header.
     *
     * If the encoding is set to "" we use an Accept-Encoding header that
     * encompasses all the encodings we support.
     * If the encoding is set to NULL we don't send an Accept-Encoding header
     * and ignore an received Content-Encoding header.
     *
     */
    argptr = va_arg(param, char *);
    if(argptr && !*argptr) {
      argptr = Curl_all_content_encodings();
      if(!argptr)
        result = CARLE_OUT_OF_MEMORY;
      else {
        result = Curl_setstropt(&data->set.str[STRING_ENCODING], argptr);
        free(argptr);
      }
    }
    else
      result = Curl_setstropt(&data->set.str[STRING_ENCODING], argptr);
    break;

  case CARLOPT_TRANSFER_ENCODING:
    data->set.http_transfer_encoding = (0 != va_arg(param, long)) ?
      TRUE : FALSE;
    break;

  case CARLOPT_FOLLOWLOCATION:
    /*
     * Follow Location: header hints on a HTTP-server.
     */
    data->set.http_follow_location = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_UNRESTRICTED_AUTH:
    /*
     * Send authentication (user+password) when following locations, even when
     * hostname changed.
     */
    data->set.allow_auth_to_other_hosts =
      (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_MAXREDIRS:
    /*
     * The maximum amount of hops you allow carl to follow Location:
     * headers. This should mostly be used to detect never-ending loops.
     */
    arg = va_arg(param, long);
    if(arg < -1)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.maxredirs = arg;
    break;

  case CARLOPT_POSTREDIR:
    /*
     * Set the behavior of POST when redirecting
     * CARL_REDIR_GET_ALL - POST is changed to GET after 301 and 302
     * CARL_REDIR_POST_301 - POST is kept as POST after 301
     * CARL_REDIR_POST_302 - POST is kept as POST after 302
     * CARL_REDIR_POST_303 - POST is kept as POST after 303
     * CARL_REDIR_POST_ALL - POST is kept as POST after 301, 302 and 303
     * other - POST is kept as POST after 301 and 302
     */
    arg = va_arg(param, long);
    if(arg < CARL_REDIR_GET_ALL)
      /* no return error on too high numbers since the bitmask could be
         extended in a future */
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.keep_post = arg & CARL_REDIR_POST_ALL;
    break;

  case CARLOPT_POST:
    /* Does this option serve a purpose anymore? Yes it does, when
       CARLOPT_POSTFIELDS isn't used and the POST data is read off the
       callback! */
    if(va_arg(param, long)) {
      data->set.method = HTTPREQ_POST;
      data->set.opt_no_body = FALSE; /* this is implied */
    }
    else
      data->set.method = HTTPREQ_GET;
    break;

  case CARLOPT_HTTPPOST:
    /*
     * Set to make us do HTTP POST
     */
    data->set.httppost = va_arg(param, struct carl_httppost *);
    data->set.method = HTTPREQ_POST_FORM;
    data->set.opt_no_body = FALSE; /* this is implied */
    break;

  case CARLOPT_AWS_SIGV4:
    /*
     * String that holds file type of the SSL certificate to use
     */
    result = Curl_setstropt(&data->set.str[STRING_AWS_SIGV4],
                            va_arg(param, char *));
    /*
     * Basic been set by default it need to be unset here
     */
    if(data->set.str[STRING_AWS_SIGV4])
      data->set.httpauth = CARLAUTH_AWS_SIGV4;
    break;

#endif   /* CARL_DISABLE_HTTP */

  case CARLOPT_MIMEPOST:
    /*
     * Set to make us do MIME/form POST
     */
    result = Curl_mime_set_subparts(&data->set.mimepost,
                                    va_arg(param, carl_mime *), FALSE);
    if(!result) {
      data->set.method = HTTPREQ_POST_MIME;
      data->set.opt_no_body = FALSE; /* this is implied */
    }
    break;

  case CARLOPT_REFERER:
    /*
     * String to set in the HTTP Referer: field.
     */
    if(data->change.referer_alloc) {
      Curl_safefree(data->change.referer);
      data->change.referer_alloc = FALSE;
    }
    result = Curl_setstropt(&data->set.str[STRING_SET_REFERER],
                            va_arg(param, char *));
    data->change.referer = data->set.str[STRING_SET_REFERER];
    break;

  case CARLOPT_USERAGENT:
    /*
     * String to use in the HTTP User-Agent field
     */
    result = Curl_setstropt(&data->set.str[STRING_USERAGENT],
                            va_arg(param, char *));
    break;

  case CARLOPT_HTTPHEADER:
    /*
     * Set a list with HTTP headers to use (or replace internals with)
     */
    data->set.headers = va_arg(param, struct carl_slist *);
    break;

#ifndef CARL_DISABLE_HTTP
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXYHEADER:
    /*
     * Set a list with proxy headers to use (or replace internals with)
     *
     * Since CARLOPT_HTTPHEADER was the only way to set HTTP headers for a
     * long time we remain doing it this way until CARLOPT_PROXYHEADER is
     * used. As soon as this option has been used, if set to anything but
     * NULL, custom headers for proxies are only picked from this list.
     *
     * Set this option to NULL to restore the previous behavior.
     */
    data->set.proxyheaders = va_arg(param, struct carl_slist *);
    break;
#endif
  case CARLOPT_HEADEROPT:
    /*
     * Set header option.
     */
    arg = va_arg(param, long);
    data->set.sep_headers = (bool)((arg & CARLHEADER_SEPARATE)? TRUE: FALSE);
    break;

  case CARLOPT_HTTP200ALIASES:
    /*
     * Set a list of aliases for HTTP 200 in response header
     */
    data->set.http200aliases = va_arg(param, struct carl_slist *);
    break;

#if !defined(CARL_DISABLE_COOKIES)
  case CARLOPT_COOKIE:
    /*
     * Cookie string to send to the remote server in the request.
     */
    result = Curl_setstropt(&data->set.str[STRING_COOKIE],
                            va_arg(param, char *));
    break;

  case CARLOPT_COOKIEFILE:
    /*
     * Set cookie file to read and parse. Can be used multiple times.
     */
    argptr = (char *)va_arg(param, void *);
    if(argptr) {
      struct carl_slist *cl;
      /* general protection against mistakes and abuse */
      if(strlen(argptr) > CARL_MAX_INPUT_LENGTH)
        return CARLE_BAD_FUNCTION_ARGUMENT;
      /* append the cookie file name to the list of file names, and deal with
         them later */
      cl = carl_slist_append(data->change.cookielist, argptr);
      if(!cl) {
        carl_slist_free_all(data->change.cookielist);
        data->change.cookielist = NULL;
        return CARLE_OUT_OF_MEMORY;
      }
      data->change.cookielist = cl; /* store the list for later use */
    }
    break;

  case CARLOPT_COOKIEJAR:
    /*
     * Set cookie file name to dump all cookies to when we're done.
     */
  {
    struct CookieInfo *newcookies;
    result = Curl_setstropt(&data->set.str[STRING_COOKIEJAR],
                            va_arg(param, char *));

    /*
     * Activate the cookie parser. This may or may not already
     * have been made.
     */
    newcookies = Curl_cookie_init(data, NULL, data->cookies,
                                  data->set.cookiesession);
    if(!newcookies)
      result = CARLE_OUT_OF_MEMORY;
    data->cookies = newcookies;
  }
  break;

  case CARLOPT_COOKIESESSION:
    /*
     * Set this option to TRUE to start a new "cookie session". It will
     * prevent the forthcoming read-cookies-from-file actions to accept
     * cookies that are marked as being session cookies, as they belong to a
     * previous session.
     *
     * In the original Netscape cookie spec, "session cookies" are cookies
     * with no expire date set. RFC2109 describes the same action if no
     * 'Max-Age' is set and RFC2965 includes the RFC2109 description and adds
     * a 'Discard' action that can enforce the discard even for cookies that
     * have a Max-Age.
     *
     * We run mostly with the original cookie spec, as hardly anyone implements
     * anything else.
     */
    data->set.cookiesession = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_COOKIELIST:
    argptr = va_arg(param, char *);

    if(argptr == NULL)
      break;

    if(strcasecompare(argptr, "ALL")) {
      /* clear all cookies */
      Curl_share_lock(data, CARL_LOCK_DATA_COOKIE, CARL_LOCK_ACCESS_SINGLE);
      Curl_cookie_clearall(data->cookies);
      Curl_share_unlock(data, CARL_LOCK_DATA_COOKIE);
    }
    else if(strcasecompare(argptr, "SESS")) {
      /* clear session cookies */
      Curl_share_lock(data, CARL_LOCK_DATA_COOKIE, CARL_LOCK_ACCESS_SINGLE);
      Curl_cookie_clearsess(data->cookies);
      Curl_share_unlock(data, CARL_LOCK_DATA_COOKIE);
    }
    else if(strcasecompare(argptr, "FLUSH")) {
      /* flush cookies to file, takes care of the locking */
      Curl_flush_cookies(data, FALSE);
    }
    else if(strcasecompare(argptr, "RELOAD")) {
      /* reload cookies from file */
      Curl_cookie_loadfiles(data);
      break;
    }
    else {
      if(!data->cookies)
        /* if cookie engine was not running, activate it */
        data->cookies = Curl_cookie_init(data, NULL, NULL, TRUE);

      /* general protection against mistakes and abuse */
      if(strlen(argptr) > CARL_MAX_INPUT_LENGTH)
        return CARLE_BAD_FUNCTION_ARGUMENT;
      argptr = strdup(argptr);
      if(!argptr || !data->cookies) {
        result = CARLE_OUT_OF_MEMORY;
        free(argptr);
      }
      else {
        Curl_share_lock(data, CARL_LOCK_DATA_COOKIE, CARL_LOCK_ACCESS_SINGLE);

        if(checkprefix("Set-Cookie:", argptr))
          /* HTTP Header format line */
          Curl_cookie_add(data, data->cookies, TRUE, FALSE, argptr + 11, NULL,
                          NULL, TRUE);

        else
          /* Netscape format line */
          Curl_cookie_add(data, data->cookies, FALSE, FALSE, argptr, NULL,
                          NULL, TRUE);

        Curl_share_unlock(data, CARL_LOCK_DATA_COOKIE);
        free(argptr);
      }
    }

    break;
#endif /* !CARL_DISABLE_COOKIES */

  case CARLOPT_HTTPGET:
    /*
     * Set to force us do HTTP GET
     */
    if(va_arg(param, long)) {
      data->set.method = HTTPREQ_GET;
      data->set.upload = FALSE; /* switch off upload */
      data->set.opt_no_body = FALSE; /* this is implied */
    }
    break;

  case CARLOPT_HTTP_VERSION:
    /*
     * This sets a requested HTTP version to be used. The value is one of
     * the listed enums in carl/carl.h.
     */
    arg = va_arg(param, long);
    if(arg < CARL_HTTP_VERSION_NONE)
      return CARLE_BAD_FUNCTION_ARGUMENT;
#ifdef ENABLE_QUIC
    if(arg == CARL_HTTP_VERSION_3)
      ;
    else
#endif
#if !defined(USE_NGHTTP2) && !defined(USE_HYPER)
    if(arg >= CARL_HTTP_VERSION_2)
      return CARLE_UNSUPPORTED_PROTOCOL;
#else
    if(arg >= CARL_HTTP_VERSION_LAST)
      return CARLE_UNSUPPORTED_PROTOCOL;
    if(arg == CARL_HTTP_VERSION_NONE)
      arg = CARL_HTTP_VERSION_2TLS;
#endif
    data->set.httpversion = arg;
    break;

  case CARLOPT_EXPECT_100_TIMEOUT_MS:
    /*
     * Time to wait for a response to a HTTP request containing an
     * Expect: 100-continue header before sending the data anyway.
     */
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.expect_100_timeout = arg;
    break;

  case CARLOPT_HTTP09_ALLOWED:
    arg = va_arg(param, unsigned long);
    if(arg > 1L)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.http09_allowed = arg ? TRUE : FALSE;
    break;
#endif   /* CARL_DISABLE_HTTP */

  case CARLOPT_HTTPAUTH:
    /*
     * Set HTTP Authentication type BITMASK.
     */
  {
    int bitcheck;
    bool authbits;
    unsigned long auth = va_arg(param, unsigned long);

    if(auth == CARLAUTH_NONE) {
      data->set.httpauth = auth;
      break;
    }

    /* the DIGEST_IE bit is only used to set a special marker, for all the
       rest we need to handle it as normal DIGEST */
    data->state.authhost.iestyle =
      (bool)((auth & CARLAUTH_DIGEST_IE) ? TRUE : FALSE);

    if(auth & CARLAUTH_DIGEST_IE) {
      auth |= CARLAUTH_DIGEST; /* set standard digest bit */
      auth &= ~CARLAUTH_DIGEST_IE; /* unset ie digest bit */
    }

    /* switch off bits we can't support */
#ifndef USE_NTLM
    auth &= ~CARLAUTH_NTLM;    /* no NTLM support */
    auth &= ~CARLAUTH_NTLM_WB; /* no NTLM_WB support */
#elif !defined(NTLM_WB_ENABLED)
    auth &= ~CARLAUTH_NTLM_WB; /* no NTLM_WB support */
#endif
#ifndef USE_SPNEGO
    auth &= ~CARLAUTH_NEGOTIATE; /* no Negotiate (SPNEGO) auth without
                                    GSS-API or SSPI */
#endif

    /* check if any auth bit lower than CARLAUTH_ONLY is still set */
    bitcheck = 0;
    authbits = FALSE;
    while(bitcheck < 31) {
      if(auth & (1UL << bitcheck++)) {
        authbits = TRUE;
        break;
      }
    }
    if(!authbits)
      return CARLE_NOT_BUILT_IN; /* no supported types left! */

    data->set.httpauth = auth;
  }
  break;

  case CARLOPT_CUSTOMREQUEST:
    /*
     * Set a custom string to use as request
     */
    result = Curl_setstropt(&data->set.str[STRING_CUSTOMREQUEST],
                            va_arg(param, char *));

    /* we don't set
       data->set.method = HTTPREQ_CUSTOM;
       here, we continue as if we were using the already set type
       and this just changes the actual request keyword */
    break;

#ifndef CARL_DISABLE_PROXY
  case CARLOPT_HTTPPROXYTUNNEL:
    /*
     * Tunnel operations through the proxy instead of normal proxy use
     */
    data->set.tunnel_thru_httpproxy = (0 != va_arg(param, long)) ?
      TRUE : FALSE;
    break;

  case CARLOPT_PROXYPORT:
    /*
     * Explicitly set HTTP proxy port number.
     */
    arg = va_arg(param, long);
    if((arg < 0) || (arg > 65535))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.proxyport = arg;
    break;

  case CARLOPT_PROXYAUTH:
    /*
     * Set HTTP Authentication type BITMASK.
     */
  {
    int bitcheck;
    bool authbits;
    unsigned long auth = va_arg(param, unsigned long);

    if(auth == CARLAUTH_NONE) {
      data->set.proxyauth = auth;
      break;
    }

    /* the DIGEST_IE bit is only used to set a special marker, for all the
       rest we need to handle it as normal DIGEST */
    data->state.authproxy.iestyle =
      (bool)((auth & CARLAUTH_DIGEST_IE) ? TRUE : FALSE);

    if(auth & CARLAUTH_DIGEST_IE) {
      auth |= CARLAUTH_DIGEST; /* set standard digest bit */
      auth &= ~CARLAUTH_DIGEST_IE; /* unset ie digest bit */
    }
    /* switch off bits we can't support */
#ifndef USE_NTLM
    auth &= ~CARLAUTH_NTLM;    /* no NTLM support */
    auth &= ~CARLAUTH_NTLM_WB; /* no NTLM_WB support */
#elif !defined(NTLM_WB_ENABLED)
    auth &= ~CARLAUTH_NTLM_WB; /* no NTLM_WB support */
#endif
#ifndef USE_SPNEGO
    auth &= ~CARLAUTH_NEGOTIATE; /* no Negotiate (SPNEGO) auth without
                                    GSS-API or SSPI */
#endif

    /* check if any auth bit lower than CARLAUTH_ONLY is still set */
    bitcheck = 0;
    authbits = FALSE;
    while(bitcheck < 31) {
      if(auth & (1UL << bitcheck++)) {
        authbits = TRUE;
        break;
      }
    }
    if(!authbits)
      return CARLE_NOT_BUILT_IN; /* no supported types left! */

    data->set.proxyauth = auth;
  }
  break;

  case CARLOPT_PROXY:
    /*
     * Set proxy server:port to use as proxy.
     *
     * If the proxy is set to "" (and CARLOPT_SOCKS_PROXY is set to "" or NULL)
     * we explicitly say that we don't want to use a proxy
     * (even though there might be environment variables saying so).
     *
     * Setting it to NULL, means no proxy but allows the environment variables
     * to decide for us (if CARLOPT_SOCKS_PROXY setting it to NULL).
     */
    result = Curl_setstropt(&data->set.str[STRING_PROXY],
                            va_arg(param, char *));
    break;

  case CARLOPT_PRE_PROXY:
    /*
     * Set proxy server:port to use as SOCKS proxy.
     *
     * If the proxy is set to "" or NULL we explicitly say that we don't want
     * to use the socks proxy.
     */
    result = Curl_setstropt(&data->set.str[STRING_PRE_PROXY],
                            va_arg(param, char *));
    break;

  case CARLOPT_PROXYTYPE:
    /*
     * Set proxy type. HTTP/HTTP_1_0/SOCKS4/SOCKS4a/SOCKS5/SOCKS5_HOSTNAME
     */
    arg = va_arg(param, long);
    if((arg < CARLPROXY_HTTP) || (arg > CARLPROXY_SOCKS5_HOSTNAME))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.proxytype = (carl_proxytype)arg;
    break;

  case CARLOPT_PROXY_TRANSFER_MODE:
    /*
     * set transfer mode (;type=<a|i>) when doing FTP via an HTTP proxy
     */
    switch(va_arg(param, long)) {
    case 0:
      data->set.proxy_transfer_mode = FALSE;
      break;
    case 1:
      data->set.proxy_transfer_mode = TRUE;
      break;
    default:
      /* reserve other values for future use */
      result = CARLE_BAD_FUNCTION_ARGUMENT;
      break;
    }
    break;
#endif   /* CARL_DISABLE_PROXY */

  case CARLOPT_SOCKS5_AUTH:
    data->set.socks5auth = va_arg(param, unsigned long);
    if(data->set.socks5auth & ~(CARLAUTH_BASIC | CARLAUTH_GSSAPI))
      result = CARLE_NOT_BUILT_IN;
    break;
#if defined(HAVE_GSSAPI) || defined(USE_WINDOWS_SSPI)
  case CARLOPT_SOCKS5_GSSAPI_NEC:
    /*
     * Set flag for NEC SOCK5 support
     */
    data->set.socks5_gssapi_nec = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
#endif
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_SOCKS5_GSSAPI_SERVICE:
  case CARLOPT_PROXY_SERVICE_NAME:
    /*
     * Set proxy authentication service name for Kerberos 5 and SPNEGO
     */
    result = Curl_setstropt(&data->set.str[STRING_PROXY_SERVICE_NAME],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_SERVICE_NAME:
    /*
     * Set authentication service name for DIGEST-MD5, Kerberos 5 and SPNEGO
     */
    result = Curl_setstropt(&data->set.str[STRING_SERVICE_NAME],
                            va_arg(param, char *));
    break;

  case CARLOPT_HEADERDATA:
    /*
     * Custom pointer to pass the header write callback function
     */
    data->set.writeheader = (void *)va_arg(param, void *);
    break;
  case CARLOPT_ERRORBUFFER:
    /*
     * Error buffer provided by the caller to get the human readable
     * error string in.
     */
    data->set.errorbuffer = va_arg(param, char *);
    break;
  case CARLOPT_WRITEDATA:
    /*
     * FILE pointer to write to. Or possibly
     * used as argument to the write callback.
     */
    data->set.out = va_arg(param, void *);
    break;

  case CARLOPT_DIRLISTONLY:
    /*
     * An option that changes the command to one that asks for a list only, no
     * file info details. Used for FTP, POP3 and SFTP.
     */
    data->set.ftp_list_only = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_APPEND:
    /*
     * We want to upload and append to an existing file. Used for FTP and
     * SFTP.
     */
    data->set.ftp_append = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

#ifndef CARL_DISABLE_FTP
  case CARLOPT_FTP_FILEMETHOD:
    /*
     * How do access files over FTP.
     */
    arg = va_arg(param, long);
    if((arg < CARLFTPMETHOD_DEFAULT) || (arg >= CARLFTPMETHOD_LAST))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.ftp_filemethod = (carl_ftpfile)arg;
    break;
  case CARLOPT_FTPPORT:
    /*
     * Use FTP PORT, this also specifies which IP address to use
     */
    result = Curl_setstropt(&data->set.str[STRING_FTPPORT],
                            va_arg(param, char *));
    data->set.ftp_use_port = (data->set.str[STRING_FTPPORT]) ? TRUE : FALSE;
    break;

  case CARLOPT_FTP_USE_EPRT:
    data->set.ftp_use_eprt = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_FTP_USE_EPSV:
    data->set.ftp_use_epsv = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_FTP_USE_PRET:
    data->set.ftp_use_pret = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_FTP_SSL_CCC:
    arg = va_arg(param, long);
    if((arg < CARLFTPSSL_CCC_NONE) || (arg >= CARLFTPSSL_CCC_LAST))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.ftp_ccc = (carl_ftpccc)arg;
    break;

  case CARLOPT_FTP_SKIP_PASV_IP:
    /*
     * Enable or disable FTP_SKIP_PASV_IP, which will disable/enable the
     * bypass of the IP address in PASV responses.
     */
    data->set.ftp_skip_ip = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_FTP_ACCOUNT:
    result = Curl_setstropt(&data->set.str[STRING_FTP_ACCOUNT],
                            va_arg(param, char *));
    break;

  case CARLOPT_FTP_ALTERNATIVE_TO_USER:
    result = Curl_setstropt(&data->set.str[STRING_FTP_ALTERNATIVE_TO_USER],
                            va_arg(param, char *));
    break;

  case CARLOPT_FTPSSLAUTH:
    /*
     * Set a specific auth for FTP-SSL transfers.
     */
    arg = va_arg(param, long);
    if((arg < CARLFTPAUTH_DEFAULT) || (arg >= CARLFTPAUTH_LAST))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.ftpsslauth = (carl_ftpauth)arg;
    break;
  case CARLOPT_KRBLEVEL:
    /*
     * A string that defines the kerberos security level.
     */
    result = Curl_setstropt(&data->set.str[STRING_KRB_LEVEL],
                            va_arg(param, char *));
    data->set.krb = (data->set.str[STRING_KRB_LEVEL]) ? TRUE : FALSE;
    break;
#endif
  case CARLOPT_FTP_CREATE_MISSING_DIRS:
    /*
     * An FTP/SFTP option that modifies an upload to create missing
     * directories on the server.
     */
    arg = va_arg(param, long);
    /* reserve other values for future use */
    if((arg < CARLFTP_CREATE_DIR_NONE) ||
       (arg > CARLFTP_CREATE_DIR_RETRY))
      result = CARLE_BAD_FUNCTION_ARGUMENT;
    else
      data->set.ftp_create_missing_dirs = (int)arg;
    break;
  case CARLOPT_READDATA:
    /*
     * FILE pointer to read the file to be uploaded from. Or possibly
     * used as argument to the read callback.
     */
    data->set.in_set = va_arg(param, void *);
    break;
  case CARLOPT_INFILESIZE:
    /*
     * If known, this should inform carl about the file size of the
     * to-be-uploaded file.
     */
    arg = va_arg(param, long);
    if(arg < -1)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.filesize = arg;
    break;
  case CARLOPT_INFILESIZE_LARGE:
    /*
     * If known, this should inform carl about the file size of the
     * to-be-uploaded file.
     */
    bigsize = va_arg(param, carl_off_t);
    if(bigsize < -1)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.filesize = bigsize;
    break;
  case CARLOPT_LOW_SPEED_LIMIT:
    /*
     * The low speed limit that if transfers are below this for
     * CARLOPT_LOW_SPEED_TIME, the transfer is aborted.
     */
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.low_speed_limit = arg;
    break;
  case CARLOPT_MAX_SEND_SPEED_LARGE:
    /*
     * When transfer uploads are faster then CARLOPT_MAX_SEND_SPEED_LARGE
     * bytes per second the transfer is throttled..
     */
    bigsize = va_arg(param, carl_off_t);
    if(bigsize < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.max_send_speed = bigsize;
    break;
  case CARLOPT_MAX_RECV_SPEED_LARGE:
    /*
     * When receiving data faster than CARLOPT_MAX_RECV_SPEED_LARGE bytes per
     * second the transfer is throttled..
     */
    bigsize = va_arg(param, carl_off_t);
    if(bigsize < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.max_recv_speed = bigsize;
    break;
  case CARLOPT_LOW_SPEED_TIME:
    /*
     * The low speed time that if transfers are below the set
     * CARLOPT_LOW_SPEED_LIMIT during this time, the transfer is aborted.
     */
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.low_speed_time = arg;
    break;
  case CARLOPT_CARLU:
    /*
     * pass CARLU to set URL
     */
    data->set.uh = va_arg(param, CARLU *);
    break;
  case CARLOPT_URL:
    /*
     * The URL to fetch.
     */
    if(data->change.url_alloc) {
      /* the already set URL is allocated, free it first! */
      Curl_safefree(data->change.url);
      data->change.url_alloc = FALSE;
    }
    result = Curl_setstropt(&data->set.str[STRING_SET_URL],
                            va_arg(param, char *));
    data->change.url = data->set.str[STRING_SET_URL];
    break;
  case CARLOPT_PORT:
    /*
     * The port number to use when getting the URL
     */
    arg = va_arg(param, long);
    if((arg < 0) || (arg > 65535))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.use_port = arg;
    break;
  case CARLOPT_TIMEOUT:
    /*
     * The maximum time you allow carl to use for a single transfer
     * operation.
     */
    arg = va_arg(param, long);
    if((arg >= 0) && (arg <= (INT_MAX/1000)))
      data->set.timeout = arg * 1000;
    else
      return CARLE_BAD_FUNCTION_ARGUMENT;
    break;

  case CARLOPT_TIMEOUT_MS:
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.timeout = arg;
    break;

  case CARLOPT_CONNECTTIMEOUT:
    /*
     * The maximum time you allow carl to use to connect.
     */
    arg = va_arg(param, long);
    if((arg >= 0) && (arg <= (INT_MAX/1000)))
      data->set.connecttimeout = arg * 1000;
    else
      return CARLE_BAD_FUNCTION_ARGUMENT;
    break;

  case CARLOPT_CONNECTTIMEOUT_MS:
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.connecttimeout = arg;
    break;

  case CARLOPT_ACCEPTTIMEOUT_MS:
    /*
     * The maximum time you allow carl to wait for server connect
     */
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.accepttimeout = arg;
    break;

  case CARLOPT_USERPWD:
    /*
     * user:password to use in the operation
     */
    result = setstropt_userpwd(va_arg(param, char *),
                               &data->set.str[STRING_USERNAME],
                               &data->set.str[STRING_PASSWORD]);
    break;

  case CARLOPT_USERNAME:
    /*
     * authentication user name to use in the operation
     */
    result = Curl_setstropt(&data->set.str[STRING_USERNAME],
                            va_arg(param, char *));
    break;

  case CARLOPT_PASSWORD:
    /*
     * authentication password to use in the operation
     */
    result = Curl_setstropt(&data->set.str[STRING_PASSWORD],
                            va_arg(param, char *));
    break;

  case CARLOPT_LOGIN_OPTIONS:
    /*
     * authentication options to use in the operation
     */
    result = Curl_setstropt(&data->set.str[STRING_OPTIONS],
                            va_arg(param, char *));
    break;

  case CARLOPT_XOAUTH2_BEARER:
    /*
     * OAuth 2.0 bearer token to use in the operation
     */
    result = Curl_setstropt(&data->set.str[STRING_BEARER],
                            va_arg(param, char *));
    break;

  case CARLOPT_POSTQUOTE:
    /*
     * List of RAW FTP commands to use after a transfer
     */
    data->set.postquote = va_arg(param, struct carl_slist *);
    break;
  case CARLOPT_PREQUOTE:
    /*
     * List of RAW FTP commands to use prior to RETR (Wesley Laxton)
     */
    data->set.prequote = va_arg(param, struct carl_slist *);
    break;
  case CARLOPT_QUOTE:
    /*
     * List of RAW FTP commands to use before a transfer
     */
    data->set.quote = va_arg(param, struct carl_slist *);
    break;
  case CARLOPT_RESOLVE:
    /*
     * List of HOST:PORT:[addresses] strings to populate the DNS cache with
     * Entries added this way will remain in the cache until explicitly
     * removed or the handle is cleaned up.
     *
     * Prefix the HOST with plus sign (+) to have the entry expire just like
     * automatically added entries.
     *
     * Prefix the HOST with dash (-) to _remove_ the entry from the cache.
     *
     * This API can remove any entry from the DNS cache, but only entries
     * that aren't actually in use right now will be pruned immediately.
     */
    data->set.resolve = va_arg(param, struct carl_slist *);
    data->change.resolve = data->set.resolve;
    break;
  case CARLOPT_PROGRESSFUNCTION:
    /*
     * Progress callback function
     */
    data->set.fprogress = va_arg(param, carl_progress_callback);
    if(data->set.fprogress)
      data->progress.callback = TRUE; /* no longer internal */
    else
      data->progress.callback = FALSE; /* NULL enforces internal */
    break;

  case CARLOPT_XFERINFOFUNCTION:
    /*
     * Transfer info callback function
     */
    data->set.fxferinfo = va_arg(param, carl_xferinfo_callback);
    if(data->set.fxferinfo)
      data->progress.callback = TRUE; /* no longer internal */
    else
      data->progress.callback = FALSE; /* NULL enforces internal */

    break;

  case CARLOPT_PROGRESSDATA:
    /*
     * Custom client data to pass to the progress callback
     */
    data->set.progress_client = va_arg(param, void *);
    break;

#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXYUSERPWD:
    /*
     * user:password needed to use the proxy
     */
    result = setstropt_userpwd(va_arg(param, char *),
                               &data->set.str[STRING_PROXYUSERNAME],
                               &data->set.str[STRING_PROXYPASSWORD]);
    break;
  case CARLOPT_PROXYUSERNAME:
    /*
     * authentication user name to use in the operation
     */
    result = Curl_setstropt(&data->set.str[STRING_PROXYUSERNAME],
                            va_arg(param, char *));
    break;
  case CARLOPT_PROXYPASSWORD:
    /*
     * authentication password to use in the operation
     */
    result = Curl_setstropt(&data->set.str[STRING_PROXYPASSWORD],
                            va_arg(param, char *));
    break;
  case CARLOPT_NOPROXY:
    /*
     * proxy exception list
     */
    result = Curl_setstropt(&data->set.str[STRING_NOPROXY],
                            va_arg(param, char *));
    break;
#endif

  case CARLOPT_RANGE:
    /*
     * What range of the file you want to transfer
     */
    result = Curl_setstropt(&data->set.str[STRING_SET_RANGE],
                            va_arg(param, char *));
    break;
  case CARLOPT_RESUME_FROM:
    /*
     * Resume transfer at the given file position
     */
    arg = va_arg(param, long);
    if(arg < -1)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.set_resume_from = arg;
    break;
  case CARLOPT_RESUME_FROM_LARGE:
    /*
     * Resume transfer at the given file position
     */
    bigsize = va_arg(param, carl_off_t);
    if(bigsize < -1)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.set_resume_from = bigsize;
    break;
  case CARLOPT_DEBUGFUNCTION:
    /*
     * stderr write callback.
     */
    data->set.fdebug = va_arg(param, carl_debug_callback);
    /*
     * if the callback provided is NULL, it'll use the default callback
     */
    break;
  case CARLOPT_DEBUGDATA:
    /*
     * Set to a void * that should receive all error writes. This
     * defaults to CARLOPT_STDERR for normal operations.
     */
    data->set.debugdata = va_arg(param, void *);
    break;
  case CARLOPT_STDERR:
    /*
     * Set to a FILE * that should receive all error writes. This
     * defaults to stderr for normal operations.
     */
    data->set.err = va_arg(param, FILE *);
    if(!data->set.err)
      data->set.err = stderr;
    break;
  case CARLOPT_HEADERFUNCTION:
    /*
     * Set header write callback
     */
    data->set.fwrite_header = va_arg(param, carl_write_callback);
    break;
  case CARLOPT_WRITEFUNCTION:
    /*
     * Set data write callback
     */
    data->set.fwrite_func = va_arg(param, carl_write_callback);
    if(!data->set.fwrite_func) {
      data->set.is_fwrite_set = 0;
      /* When set to NULL, reset to our internal default function */
      data->set.fwrite_func = (carl_write_callback)fwrite;
    }
    else
      data->set.is_fwrite_set = 1;
    break;
  case CARLOPT_READFUNCTION:
    /*
     * Read data callback
     */
    data->set.fread_func_set = va_arg(param, carl_read_callback);
    if(!data->set.fread_func_set) {
      data->set.is_fread_set = 0;
      /* When set to NULL, reset to our internal default function */
      data->set.fread_func_set = (carl_read_callback)fread;
    }
    else
      data->set.is_fread_set = 1;
    break;
  case CARLOPT_SEEKFUNCTION:
    /*
     * Seek callback. Might be NULL.
     */
    data->set.seek_func = va_arg(param, carl_seek_callback);
    break;
  case CARLOPT_SEEKDATA:
    /*
     * Seek control callback. Might be NULL.
     */
    data->set.seek_client = va_arg(param, void *);
    break;
  case CARLOPT_CONV_FROM_NETWORK_FUNCTION:
    /*
     * "Convert from network encoding" callback
     */
    data->set.convfromnetwork = va_arg(param, carl_conv_callback);
    break;
  case CARLOPT_CONV_TO_NETWORK_FUNCTION:
    /*
     * "Convert to network encoding" callback
     */
    data->set.convtonetwork = va_arg(param, carl_conv_callback);
    break;
  case CARLOPT_CONV_FROM_UTF8_FUNCTION:
    /*
     * "Convert from UTF-8 encoding" callback
     */
    data->set.convfromutf8 = va_arg(param, carl_conv_callback);
    break;
  case CARLOPT_IOCTLFUNCTION:
    /*
     * I/O control callback. Might be NULL.
     */
    data->set.ioctl_func = va_arg(param, carl_ioctl_callback);
    break;
  case CARLOPT_IOCTLDATA:
    /*
     * I/O control data pointer. Might be NULL.
     */
    data->set.ioctl_client = va_arg(param, void *);
    break;
  case CARLOPT_SSLCERT:
    /*
     * String that holds file name of the SSL certificate to use
     */
    result = Curl_setstropt(&data->set.str[STRING_CERT_ORIG],
                            va_arg(param, char *));
    break;
  case CARLOPT_SSLCERT_BLOB:
    /*
     * Blob that holds file name of the SSL certificate to use
     */
    result = Curl_setblobopt(&data->set.blobs[BLOB_CERT_ORIG],
                             va_arg(param, struct carl_blob *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSLCERT:
    /*
     * String that holds file name of the SSL certificate to use for proxy
     */
    result = Curl_setstropt(&data->set.str[STRING_CERT_PROXY],
                            va_arg(param, char *));
    break;
  case CARLOPT_PROXY_SSLCERT_BLOB:
    /*
     * Blob that holds file name of the SSL certificate to use for proxy
     */
    result = Curl_setblobopt(&data->set.blobs[BLOB_CERT_PROXY],
                             va_arg(param, struct carl_blob *));
    break;
#endif
  case CARLOPT_SSLCERTTYPE:
    /*
     * String that holds file type of the SSL certificate to use
     */
    result = Curl_setstropt(&data->set.str[STRING_CERT_TYPE_ORIG],
                            va_arg(param, char *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSLCERTTYPE:
    /*
     * String that holds file type of the SSL certificate to use for proxy
     */
    result = Curl_setstropt(&data->set.str[STRING_CERT_TYPE_PROXY],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_SSLKEY:
    /*
     * String that holds file name of the SSL key to use
     */
    result = Curl_setstropt(&data->set.str[STRING_KEY_ORIG],
                            va_arg(param, char *));
    break;
  case CARLOPT_SSLKEY_BLOB:
    /*
     * Blob that holds file name of the SSL key to use
     */
    result = Curl_setblobopt(&data->set.blobs[BLOB_KEY_ORIG],
                             va_arg(param, struct carl_blob *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSLKEY:
    /*
     * String that holds file name of the SSL key to use for proxy
     */
    result = Curl_setstropt(&data->set.str[STRING_KEY_PROXY],
                            va_arg(param, char *));
    break;
  case CARLOPT_PROXY_SSLKEY_BLOB:
    /*
     * Blob that holds file name of the SSL key to use for proxy
     */
    result = Curl_setblobopt(&data->set.blobs[BLOB_KEY_PROXY],
                             va_arg(param, struct carl_blob *));
    break;
#endif
  case CARLOPT_SSLKEYTYPE:
    /*
     * String that holds file type of the SSL key to use
     */
    result = Curl_setstropt(&data->set.str[STRING_KEY_TYPE_ORIG],
                            va_arg(param, char *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSLKEYTYPE:
    /*
     * String that holds file type of the SSL key to use for proxy
     */
    result = Curl_setstropt(&data->set.str[STRING_KEY_TYPE_PROXY],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_KEYPASSWD:
    /*
     * String that holds the SSL or SSH private key password.
     */
    result = Curl_setstropt(&data->set.str[STRING_KEY_PASSWD_ORIG],
                            va_arg(param, char *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_KEYPASSWD:
    /*
     * String that holds the SSL private key password for proxy.
     */
    result = Curl_setstropt(&data->set.str[STRING_KEY_PASSWD_PROXY],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_SSLENGINE:
    /*
     * String that holds the SSL crypto engine.
     */
    argptr = va_arg(param, char *);
    if(argptr && argptr[0]) {
      result = Curl_setstropt(&data->set.str[STRING_SSL_ENGINE], argptr);
      if(!result) {
        result = Curl_ssl_set_engine(data, argptr);
      }
    }
    break;

  case CARLOPT_SSLENGINE_DEFAULT:
    /*
     * flag to set engine as default.
     */
    Curl_setstropt(&data->set.str[STRING_SSL_ENGINE], NULL);
    result = Curl_ssl_set_engine_default(data);
    break;
  case CARLOPT_CRLF:
    /*
     * Kludgy option to enable CRLF conversions. Subject for removal.
     */
    data->set.crlf = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_HAPROXYPROTOCOL:
    /*
     * Set to send the HAProxy Proxy Protocol header
     */
    data->set.haproxyprotocol = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
#endif
  case CARLOPT_INTERFACE:
    /*
     * Set what interface or address/hostname to bind the socket to when
     * performing an operation and thus what from-IP your connection will use.
     */
    result = Curl_setstropt(&data->set.str[STRING_DEVICE],
                            va_arg(param, char *));
    break;
  case CARLOPT_LOCALPORT:
    /*
     * Set what local port to bind the socket to when performing an operation.
     */
    arg = va_arg(param, long);
    if((arg < 0) || (arg > 65535))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.localport = carlx_sltous(arg);
    break;
  case CARLOPT_LOCALPORTRANGE:
    /*
     * Set number of local ports to try, starting with CARLOPT_LOCALPORT.
     */
    arg = va_arg(param, long);
    if((arg < 0) || (arg > 65535))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.localportrange = carlx_sltosi(arg);
    break;
  case CARLOPT_GSSAPI_DELEGATION:
    /*
     * GSS-API credential delegation bitmask
     */
    arg = va_arg(param, long);
    if(arg < CARLGSSAPI_DELEGATION_NONE)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.gssapi_delegation = arg;
    break;
  case CARLOPT_SSL_VERIFYPEER:
    /*
     * Enable peer SSL verifying.
     */
    data->set.ssl.primary.verifypeer = (0 != va_arg(param, long)) ?
      TRUE : FALSE;

    /* Update the current connection ssl_config. */
    if(data->conn) {
      data->conn->ssl_config.verifypeer =
        data->set.ssl.primary.verifypeer;
    }
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSL_VERIFYPEER:
    /*
     * Enable peer SSL verifying for proxy.
     */
    data->set.proxy_ssl.primary.verifypeer =
      (0 != va_arg(param, long))?TRUE:FALSE;

    /* Update the current connection proxy_ssl_config. */
    if(data->conn) {
      data->conn->proxy_ssl_config.verifypeer =
        data->set.proxy_ssl.primary.verifypeer;
    }
    break;
#endif
  case CARLOPT_SSL_VERIFYHOST:
    /*
     * Enable verification of the host name in the peer certificate
     */
    arg = va_arg(param, long);

    /* Obviously people are not reading documentation and too many thought
       this argument took a boolean when it wasn't and misused it.
       Treat 1 and 2 the same */
    data->set.ssl.primary.verifyhost = (bool)((arg & 3) ? TRUE : FALSE);

    /* Update the current connection ssl_config. */
    if(data->conn) {
      data->conn->ssl_config.verifyhost =
        data->set.ssl.primary.verifyhost;
    }
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSL_VERIFYHOST:
    /*
     * Enable verification of the host name in the peer certificate for proxy
     */
    arg = va_arg(param, long);

    /* Treat both 1 and 2 as TRUE */
    data->set.proxy_ssl.primary.verifyhost = (bool)((arg & 3)?TRUE:FALSE);

    /* Update the current connection proxy_ssl_config. */
    if(data->conn) {
      data->conn->proxy_ssl_config.verifyhost =
        data->set.proxy_ssl.primary.verifyhost;
    }
    break;
#endif
  case CARLOPT_SSL_VERIFYSTATUS:
    /*
     * Enable certificate status verifying.
     */
    if(!Curl_ssl_cert_status_request()) {
      result = CARLE_NOT_BUILT_IN;
      break;
    }

    data->set.ssl.primary.verifystatus = (0 != va_arg(param, long)) ?
      TRUE : FALSE;

    /* Update the current connection ssl_config. */
    if(data->conn) {
      data->conn->ssl_config.verifystatus =
        data->set.ssl.primary.verifystatus;
    }
    break;
  case CARLOPT_SSL_CTX_FUNCTION:
    /*
     * Set a SSL_CTX callback
     */
#ifdef USE_SSL
    if(Curl_ssl->supports & SSLSUPP_SSL_CTX)
      data->set.ssl.fsslctx = va_arg(param, carl_ssl_ctx_callback);
    else
#endif
      result = CARLE_NOT_BUILT_IN;
    break;
  case CARLOPT_SSL_CTX_DATA:
    /*
     * Set a SSL_CTX callback parameter pointer
     */
#ifdef USE_SSL
    if(Curl_ssl->supports & SSLSUPP_SSL_CTX)
      data->set.ssl.fsslctxp = va_arg(param, void *);
    else
#endif
      result = CARLE_NOT_BUILT_IN;
    break;
  case CARLOPT_SSL_FALSESTART:
    /*
     * Enable TLS false start.
     */
    if(!Curl_ssl_false_start()) {
      result = CARLE_NOT_BUILT_IN;
      break;
    }

    data->set.ssl.falsestart = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_CERTINFO:
#ifdef USE_SSL
    if(Curl_ssl->supports & SSLSUPP_CERTINFO)
      data->set.ssl.certinfo = (0 != va_arg(param, long)) ? TRUE : FALSE;
    else
#endif
      result = CARLE_NOT_BUILT_IN;
        break;
  case CARLOPT_PINNEDPUBLICKEY:
    /*
     * Set pinned public key for SSL connection.
     * Specify file name of the public key in DER format.
     */
#ifdef USE_SSL
    if(Curl_ssl->supports & SSLSUPP_PINNEDPUBKEY)
      result = Curl_setstropt(&data->set.str[STRING_SSL_PINNEDPUBLICKEY_ORIG],
                              va_arg(param, char *));
    else
#endif
      result = CARLE_NOT_BUILT_IN;
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_PINNEDPUBLICKEY:
    /*
     * Set pinned public key for SSL connection.
     * Specify file name of the public key in DER format.
     */
#ifdef USE_SSL
    if(Curl_ssl->supports & SSLSUPP_PINNEDPUBKEY)
      result = Curl_setstropt(&data->set.str[STRING_SSL_PINNEDPUBLICKEY_PROXY],
                              va_arg(param, char *));
    else
#endif
      result = CARLE_NOT_BUILT_IN;
    break;
#endif
  case CARLOPT_CAINFO:
    /*
     * Set CA info for SSL connection. Specify file name of the CA certificate
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_CAFILE_ORIG],
                            va_arg(param, char *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_CAINFO:
    /*
     * Set CA info SSL connection for proxy. Specify file name of the
     * CA certificate
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_CAFILE_PROXY],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_CAPATH:
    /*
     * Set CA path info for SSL connection. Specify directory name of the CA
     * certificates which have been prepared using openssl c_rehash utility.
     */
#ifdef USE_SSL
    if(Curl_ssl->supports & SSLSUPP_CA_PATH)
      /* This does not work on windows. */
      result = Curl_setstropt(&data->set.str[STRING_SSL_CAPATH_ORIG],
                              va_arg(param, char *));
    else
#endif
      result = CARLE_NOT_BUILT_IN;
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_CAPATH:
    /*
     * Set CA path info for SSL connection proxy. Specify directory name of the
     * CA certificates which have been prepared using openssl c_rehash utility.
     */
#ifdef USE_SSL
    if(Curl_ssl->supports & SSLSUPP_CA_PATH)
      /* This does not work on windows. */
      result = Curl_setstropt(&data->set.str[STRING_SSL_CAPATH_PROXY],
                              va_arg(param, char *));
    else
#endif
      result = CARLE_NOT_BUILT_IN;
    break;
#endif
  case CARLOPT_CRLFILE:
    /*
     * Set CRL file info for SSL connection. Specify file name of the CRL
     * to check certificates revocation
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_CRLFILE_ORIG],
                            va_arg(param, char *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_CRLFILE:
    /*
     * Set CRL file info for SSL connection for proxy. Specify file name of the
     * CRL to check certificates revocation
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_CRLFILE_PROXY],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_ISSUERCERT:
    /*
     * Set Issuer certificate file
     * to check certificates issuer
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_ISSUERCERT_ORIG],
                            va_arg(param, char *));
    break;
  case CARLOPT_ISSUERCERT_BLOB:
    /*
     * Blob that holds Issuer certificate to check certificates issuer
     */
    result = Curl_setblobopt(&data->set.blobs[BLOB_SSL_ISSUERCERT_ORIG],
                             va_arg(param, struct carl_blob *));
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_ISSUERCERT:
    /*
     * Set Issuer certificate file
     * to check certificates issuer
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_ISSUERCERT_PROXY],
                            va_arg(param, char *));
    break;
  case CARLOPT_PROXY_ISSUERCERT_BLOB:
    /*
     * Blob that holds Issuer certificate to check certificates issuer
     */
    result = Curl_setblobopt(&data->set.blobs[BLOB_SSL_ISSUERCERT_PROXY],
                             va_arg(param, struct carl_blob *));
    break;
#endif
#ifndef CARL_DISABLE_TELNET
  case CARLOPT_TELNETOPTIONS:
    /*
     * Set a linked list of telnet options
     */
    data->set.telnet_options = va_arg(param, struct carl_slist *);
    break;
#endif
  case CARLOPT_BUFFERSIZE:
    /*
     * The application kindly asks for a differently sized receive buffer.
     * If it seems reasonable, we'll use it.
     */
    if(data->state.buffer)
      return CARLE_BAD_FUNCTION_ARGUMENT;

    arg = va_arg(param, long);

    if(arg > READBUFFER_MAX)
      arg = READBUFFER_MAX;
    else if(arg < 1)
      arg = READBUFFER_SIZE;
    else if(arg < READBUFFER_MIN)
      arg = READBUFFER_MIN;

    data->set.buffer_size = arg;
    break;

  case CARLOPT_UPLOAD_BUFFERSIZE:
    /*
     * The application kindly asks for a differently sized upload buffer.
     * Cap it to sensible.
     */
    arg = va_arg(param, long);

    if(arg > UPLOADBUFFER_MAX)
      arg = UPLOADBUFFER_MAX;
    else if(arg < UPLOADBUFFER_MIN)
      arg = UPLOADBUFFER_MIN;

    data->set.upload_buffer_size = arg;
    Curl_safefree(data->state.ulbuf); /* force a realloc next opportunity */
    break;

  case CARLOPT_NOSIGNAL:
    /*
     * The application asks not to set any signal() or alarm() handlers,
     * even when using a timeout.
     */
    data->set.no_signal = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_SHARE:
  {
    struct Curl_share *set;
    set = va_arg(param, struct Curl_share *);

    /* disconnect from old share, if any */
    if(data->share) {
      Curl_share_lock(data, CARL_LOCK_DATA_SHARE, CARL_LOCK_ACCESS_SINGLE);

      if(data->dns.hostcachetype == HCACHE_SHARED) {
        data->dns.hostcache = NULL;
        data->dns.hostcachetype = HCACHE_NONE;
      }

#if !defined(CARL_DISABLE_HTTP) && !defined(CARL_DISABLE_COOKIES)
      if(data->share->cookies == data->cookies)
        data->cookies = NULL;
#endif

      if(data->share->sslsession == data->state.session)
        data->state.session = NULL;

#ifdef USE_LIBPSL
      if(data->psl == &data->share->psl)
        data->psl = data->multi? &data->multi->psl: NULL;
#endif

      data->share->dirty--;

      Curl_share_unlock(data, CARL_LOCK_DATA_SHARE);
      data->share = NULL;
    }

    if(GOOD_SHARE_HANDLE(set))
      /* use new share if it set */
      data->share = set;
    if(data->share) {

      Curl_share_lock(data, CARL_LOCK_DATA_SHARE, CARL_LOCK_ACCESS_SINGLE);

      data->share->dirty++;

      if(data->share->specifier & (1<< CARL_LOCK_DATA_DNS)) {
        /* use shared host cache */
        data->dns.hostcache = &data->share->hostcache;
        data->dns.hostcachetype = HCACHE_SHARED;
      }
#if !defined(CARL_DISABLE_HTTP) && !defined(CARL_DISABLE_COOKIES)
      if(data->share->cookies) {
        /* use shared cookie list, first free own one if any */
        Curl_cookie_cleanup(data->cookies);
        /* enable cookies since we now use a share that uses cookies! */
        data->cookies = data->share->cookies;
      }
#endif   /* CARL_DISABLE_HTTP */
      if(data->share->sslsession) {
        data->set.general_ssl.max_ssl_sessions = data->share->max_ssl_sessions;
        data->state.session = data->share->sslsession;
      }
#ifdef USE_LIBPSL
      if(data->share->specifier & (1 << CARL_LOCK_DATA_PSL))
        data->psl = &data->share->psl;
#endif

      Curl_share_unlock(data, CARL_LOCK_DATA_SHARE);
    }
    /* check for host cache not needed,
     * it will be done by carl_easy_perform */
  }
  break;

  case CARLOPT_PRIVATE:
    /*
     * Set private data pointer.
     */
    data->set.private_data = va_arg(param, void *);
    break;

  case CARLOPT_MAXFILESIZE:
    /*
     * Set the maximum size of a file to download.
     */
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.max_filesize = arg;
    break;

#ifdef USE_SSL
  case CARLOPT_USE_SSL:
    /*
     * Make transfers attempt to use SSL/TLS.
     */
    arg = va_arg(param, long);
    if((arg < CARLUSESSL_NONE) || (arg >= CARLUSESSL_LAST))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.use_ssl = (carl_usessl)arg;
    break;

  case CARLOPT_SSL_OPTIONS:
    arg = va_arg(param, long);
    data->set.ssl.enable_beast =
      (bool)((arg&CARLSSLOPT_ALLOW_BEAST) ? TRUE : FALSE);
    data->set.ssl.no_revoke = !!(arg & CARLSSLOPT_NO_REVOKE);
    data->set.ssl.no_partialchain = !!(arg & CARLSSLOPT_NO_PARTIALCHAIN);
    data->set.ssl.revoke_best_effort = !!(arg & CARLSSLOPT_REVOKE_BEST_EFFORT);
    data->set.ssl.native_ca_store = !!(arg & CARLSSLOPT_NATIVE_CA);
    break;

#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_SSL_OPTIONS:
    arg = va_arg(param, long);
    data->set.proxy_ssl.enable_beast =
      (bool)((arg&CARLSSLOPT_ALLOW_BEAST) ? TRUE : FALSE);
    data->set.proxy_ssl.no_revoke = !!(arg & CARLSSLOPT_NO_REVOKE);
    data->set.proxy_ssl.no_partialchain = !!(arg & CARLSSLOPT_NO_PARTIALCHAIN);
    data->set.proxy_ssl.native_ca_store = !!(arg & CARLSSLOPT_NATIVE_CA);
    data->set.proxy_ssl.revoke_best_effort =
      !!(arg & CARLSSLOPT_REVOKE_BEST_EFFORT);
    break;
#endif

  case CARLOPT_SSL_EC_CURVES:
    /*
     * Set accepted curves in SSL connection setup.
     * Specify colon-delimited list of curve algorithm names.
     */
    result = Curl_setstropt(&data->set.str[STRING_SSL_EC_CURVES],
                            va_arg(param, char *));
    break;
#endif
  case CARLOPT_IPRESOLVE:
    arg = va_arg(param, long);
    if((arg < CARL_IPRESOLVE_WHATEVER) || (arg > CARL_IPRESOLVE_V6))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.ipver = arg;
    break;

  case CARLOPT_MAXFILESIZE_LARGE:
    /*
     * Set the maximum size of a file to download.
     */
    bigsize = va_arg(param, carl_off_t);
    if(bigsize < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.max_filesize = bigsize;
    break;

  case CARLOPT_TCP_NODELAY:
    /*
     * Enable or disable TCP_NODELAY, which will disable/enable the Nagle
     * algorithm
     */
    data->set.tcp_nodelay = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_IGNORE_CONTENT_LENGTH:
    data->set.ignorecl = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_CONNECT_ONLY:
    /*
     * No data transfer, set up connection and let application use the socket
     */
    data->set.connect_only = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_SOCKOPTFUNCTION:
    /*
     * socket callback function: called after socket() but before connect()
     */
    data->set.fsockopt = va_arg(param, carl_sockopt_callback);
    break;

  case CARLOPT_SOCKOPTDATA:
    /*
     * socket callback data pointer. Might be NULL.
     */
    data->set.sockopt_client = va_arg(param, void *);
    break;

  case CARLOPT_OPENSOCKETFUNCTION:
    /*
     * open/create socket callback function: called instead of socket(),
     * before connect()
     */
    data->set.fopensocket = va_arg(param, carl_opensocket_callback);
    break;

  case CARLOPT_OPENSOCKETDATA:
    /*
     * socket callback data pointer. Might be NULL.
     */
    data->set.opensocket_client = va_arg(param, void *);
    break;

  case CARLOPT_CLOSESOCKETFUNCTION:
    /*
     * close socket callback function: called instead of close()
     * when shutting down a connection
     */
    data->set.fclosesocket = va_arg(param, carl_closesocket_callback);
    break;

  case CARLOPT_RESOLVER_START_FUNCTION:
    /*
     * resolver start callback function: called before a new resolver request
     * is started
     */
    data->set.resolver_start = va_arg(param, carl_resolver_start_callback);
    break;

  case CARLOPT_RESOLVER_START_DATA:
    /*
     * resolver start callback data pointer. Might be NULL.
     */
    data->set.resolver_start_client = va_arg(param, void *);
    break;

  case CARLOPT_CLOSESOCKETDATA:
    /*
     * socket callback data pointer. Might be NULL.
     */
    data->set.closesocket_client = va_arg(param, void *);
    break;

  case CARLOPT_SSL_SESSIONID_CACHE:
    data->set.ssl.primary.sessionid = (0 != va_arg(param, long)) ?
      TRUE : FALSE;
#ifndef CARL_DISABLE_PROXY
    data->set.proxy_ssl.primary.sessionid = data->set.ssl.primary.sessionid;
#endif
    break;

#ifdef USE_SSH
    /* we only include SSH options if explicitly built to support SSH */
  case CARLOPT_SSH_AUTH_TYPES:
    data->set.ssh_auth_types = va_arg(param, long);
    break;

  case CARLOPT_SSH_PUBLIC_KEYFILE:
    /*
     * Use this file instead of the $HOME/.ssh/id_dsa.pub file
     */
    result = Curl_setstropt(&data->set.str[STRING_SSH_PUBLIC_KEY],
                            va_arg(param, char *));
    break;

  case CARLOPT_SSH_PRIVATE_KEYFILE:
    /*
     * Use this file instead of the $HOME/.ssh/id_dsa file
     */
    result = Curl_setstropt(&data->set.str[STRING_SSH_PRIVATE_KEY],
                            va_arg(param, char *));
    break;
  case CARLOPT_SSH_HOST_PUBLIC_KEY_MD5:
    /*
     * Option to allow for the MD5 of the host public key to be checked
     * for validation purposes.
     */
    result = Curl_setstropt(&data->set.str[STRING_SSH_HOST_PUBLIC_KEY_MD5],
                            va_arg(param, char *));
    break;

  case CARLOPT_SSH_KNOWNHOSTS:
    /*
     * Store the file name to read known hosts from.
     */
    result = Curl_setstropt(&data->set.str[STRING_SSH_KNOWNHOSTS],
                            va_arg(param, char *));
    break;

  case CARLOPT_SSH_KEYFUNCTION:
    /* setting to NULL is fine since the ssh.c functions themselves will
       then revert to use the internal default */
    data->set.ssh_keyfunc = va_arg(param, carl_sshkeycallback);
    break;

  case CARLOPT_SSH_KEYDATA:
    /*
     * Custom client data to pass to the SSH keyfunc callback
     */
    data->set.ssh_keyfunc_userp = va_arg(param, void *);
    break;

  case CARLOPT_SSH_COMPRESSION:
    data->set.ssh_compression = (0 != va_arg(param, long))?TRUE:FALSE;
    break;
#endif /* USE_SSH */

  case CARLOPT_HTTP_TRANSFER_DECODING:
    /*
     * disable libcarl transfer encoding is used
     */
    data->set.http_te_skip = (0 == va_arg(param, long)) ? TRUE : FALSE;
    break;

  case CARLOPT_HTTP_CONTENT_DECODING:
    /*
     * raw data passed to the application when content encoding is used
     */
    data->set.http_ce_skip = (0 == va_arg(param, long)) ? TRUE : FALSE;
    break;

#if !defined(CARL_DISABLE_FTP) || defined(USE_SSH)
  case CARLOPT_NEW_FILE_PERMS:
    /*
     * Uses these permissions instead of 0644
     */
    arg = va_arg(param, long);
    if((arg < 0) || (arg > 0777))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.new_file_perms = arg;
    break;

  case CARLOPT_NEW_DIRECTORY_PERMS:
    /*
     * Uses these permissions instead of 0755
     */
    arg = va_arg(param, long);
    if((arg < 0) || (arg > 0777))
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.new_directory_perms = arg;
    break;
#endif

  case CARLOPT_ADDRESS_SCOPE:
    /*
     * Use this scope id when using IPv6
     * We always get longs when passed plain numericals so we should check
     * that the value fits into an unsigned 32 bit integer.
     */
    uarg = va_arg(param, unsigned long);
#if SIZEOF_LONG > 4
    if(uarg > UINT_MAX)
      return CARLE_BAD_FUNCTION_ARGUMENT;
#endif
    data->set.scope_id = (unsigned int)uarg;
    break;

  case CARLOPT_PROTOCOLS:
    /* set the bitmask for the protocols that are allowed to be used for the
       transfer, which thus helps the app which takes URLs from users or other
       external inputs and want to restrict what protocol(s) to deal
       with. Defaults to CARLPROTO_ALL. */
    data->set.allowed_protocols = va_arg(param, long);
    break;

  case CARLOPT_REDIR_PROTOCOLS:
    /* set the bitmask for the protocols that libcarl is allowed to follow to,
       as a subset of the CARLOPT_PROTOCOLS ones. That means the protocol needs
       to be set in both bitmasks to be allowed to get redirected to. */
    data->set.redir_protocols = va_arg(param, long);
    break;

  case CARLOPT_DEFAULT_PROTOCOL:
    /* Set the protocol to use when the URL doesn't include any protocol */
    result = Curl_setstropt(&data->set.str[STRING_DEFAULT_PROTOCOL],
                            va_arg(param, char *));
    break;
#ifndef CARL_DISABLE_SMTP
  case CARLOPT_MAIL_FROM:
    /* Set the SMTP mail originator */
    result = Curl_setstropt(&data->set.str[STRING_MAIL_FROM],
                            va_arg(param, char *));
    break;

  case CARLOPT_MAIL_AUTH:
    /* Set the SMTP auth originator */
    result = Curl_setstropt(&data->set.str[STRING_MAIL_AUTH],
                            va_arg(param, char *));
    break;

  case CARLOPT_MAIL_RCPT:
    /* Set the list of mail recipients */
    data->set.mail_rcpt = va_arg(param, struct carl_slist *);
    break;
  case CARLOPT_MAIL_RCPT_ALLLOWFAILS:
    /* allow RCPT TO command to fail for some recipients */
    data->set.mail_rcpt_allowfails = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
#endif

  case CARLOPT_SASL_AUTHZID:
    /* Authorisation identity (identity to act as) */
    result = Curl_setstropt(&data->set.str[STRING_SASL_AUTHZID],
                            va_arg(param, char *));
    break;

  case CARLOPT_SASL_IR:
    /* Enable/disable SASL initial response */
    data->set.sasl_ir = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
#ifndef CARL_DISABLE_RTSP
  case CARLOPT_RTSP_REQUEST:
  {
    /*
     * Set the RTSP request method (OPTIONS, SETUP, PLAY, etc...)
     * Would this be better if the RTSPREQ_* were just moved into here?
     */
    long in_rtspreq = va_arg(param, long);
    Curl_RtspReq rtspreq = RTSPREQ_NONE;
    switch(in_rtspreq) {
    case CARL_RTSPREQ_OPTIONS:
      rtspreq = RTSPREQ_OPTIONS;
      break;

    case CARL_RTSPREQ_DESCRIBE:
      rtspreq = RTSPREQ_DESCRIBE;
      break;

    case CARL_RTSPREQ_ANNOUNCE:
      rtspreq = RTSPREQ_ANNOUNCE;
      break;

    case CARL_RTSPREQ_SETUP:
      rtspreq = RTSPREQ_SETUP;
      break;

    case CARL_RTSPREQ_PLAY:
      rtspreq = RTSPREQ_PLAY;
      break;

    case CARL_RTSPREQ_PAUSE:
      rtspreq = RTSPREQ_PAUSE;
      break;

    case CARL_RTSPREQ_TEARDOWN:
      rtspreq = RTSPREQ_TEARDOWN;
      break;

    case CARL_RTSPREQ_GET_PARAMETER:
      rtspreq = RTSPREQ_GET_PARAMETER;
      break;

    case CARL_RTSPREQ_SET_PARAMETER:
      rtspreq = RTSPREQ_SET_PARAMETER;
      break;

    case CARL_RTSPREQ_RECORD:
      rtspreq = RTSPREQ_RECORD;
      break;

    case CARL_RTSPREQ_RECEIVE:
      rtspreq = RTSPREQ_RECEIVE;
      break;
    default:
      rtspreq = RTSPREQ_NONE;
    }

    data->set.rtspreq = rtspreq;
    break;
  }


  case CARLOPT_RTSP_SESSION_ID:
    /*
     * Set the RTSP Session ID manually. Useful if the application is
     * resuming a previously established RTSP session
     */
    result = Curl_setstropt(&data->set.str[STRING_RTSP_SESSION_ID],
                            va_arg(param, char *));
    break;

  case CARLOPT_RTSP_STREAM_URI:
    /*
     * Set the Stream URI for the RTSP request. Unless the request is
     * for generic server options, the application will need to set this.
     */
    result = Curl_setstropt(&data->set.str[STRING_RTSP_STREAM_URI],
                            va_arg(param, char *));
    break;

  case CARLOPT_RTSP_TRANSPORT:
    /*
     * The content of the Transport: header for the RTSP request
     */
    result = Curl_setstropt(&data->set.str[STRING_RTSP_TRANSPORT],
                            va_arg(param, char *));
    break;

  case CARLOPT_RTSP_CLIENT_CSEQ:
    /*
     * Set the CSEQ number to issue for the next RTSP request. Useful if the
     * application is resuming a previously broken connection. The CSEQ
     * will increment from this new number henceforth.
     */
    data->state.rtsp_next_client_CSeq = va_arg(param, long);
    break;

  case CARLOPT_RTSP_SERVER_CSEQ:
    /* Same as the above, but for server-initiated requests */
    data->state.rtsp_next_server_CSeq = va_arg(param, long);
    break;

  case CARLOPT_INTERLEAVEDATA:
    data->set.rtp_out = va_arg(param, void *);
    break;
  case CARLOPT_INTERLEAVEFUNCTION:
    /* Set the user defined RTP write function */
    data->set.fwrite_rtp = va_arg(param, carl_write_callback);
    break;
#endif
#ifndef CARL_DISABLE_FTP
  case CARLOPT_WILDCARDMATCH:
    data->set.wildcard_enabled = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_CHUNK_BGN_FUNCTION:
    data->set.chunk_bgn = va_arg(param, carl_chunk_bgn_callback);
    break;
  case CARLOPT_CHUNK_END_FUNCTION:
    data->set.chunk_end = va_arg(param, carl_chunk_end_callback);
    break;
  case CARLOPT_FNMATCH_FUNCTION:
    data->set.fnmatch = va_arg(param, carl_fnmatch_callback);
    break;
  case CARLOPT_CHUNK_DATA:
    data->wildcard.customptr = va_arg(param, void *);
    break;
  case CARLOPT_FNMATCH_DATA:
    data->set.fnmatch_data = va_arg(param, void *);
    break;
#endif
#ifdef USE_TLS_SRP
  case CARLOPT_TLSAUTH_USERNAME:
    result = Curl_setstropt(&data->set.str[STRING_TLSAUTH_USERNAME_ORIG],
                            va_arg(param, char *));
    if(data->set.str[STRING_TLSAUTH_USERNAME_ORIG] && !data->set.ssl.authtype)
      data->set.ssl.authtype = CARL_TLSAUTH_SRP; /* default to SRP */
    break;
  case CARLOPT_PROXY_TLSAUTH_USERNAME:
    result = Curl_setstropt(&data->set.str[STRING_TLSAUTH_USERNAME_PROXY],
                            va_arg(param, char *));
#ifndef CARL_DISABLE_PROXY
    if(data->set.str[STRING_TLSAUTH_USERNAME_PROXY] &&
       !data->set.proxy_ssl.authtype)
      data->set.proxy_ssl.authtype = CARL_TLSAUTH_SRP; /* default to SRP */
#endif
    break;
  case CARLOPT_TLSAUTH_PASSWORD:
    result = Curl_setstropt(&data->set.str[STRING_TLSAUTH_PASSWORD_ORIG],
                            va_arg(param, char *));
    if(data->set.str[STRING_TLSAUTH_USERNAME_ORIG] && !data->set.ssl.authtype)
      data->set.ssl.authtype = CARL_TLSAUTH_SRP; /* default to SRP */
    break;
  case CARLOPT_PROXY_TLSAUTH_PASSWORD:
    result = Curl_setstropt(&data->set.str[STRING_TLSAUTH_PASSWORD_PROXY],
                            va_arg(param, char *));
#ifndef CARL_DISABLE_PROXY
    if(data->set.str[STRING_TLSAUTH_USERNAME_PROXY] &&
       !data->set.proxy_ssl.authtype)
      data->set.proxy_ssl.authtype = CARL_TLSAUTH_SRP; /* default to SRP */
#endif
    break;
  case CARLOPT_TLSAUTH_TYPE:
    argptr = va_arg(param, char *);
    if(!argptr ||
       strncasecompare(argptr, "SRP", strlen("SRP")))
      data->set.ssl.authtype = CARL_TLSAUTH_SRP;
    else
      data->set.ssl.authtype = CARL_TLSAUTH_NONE;
    break;
#ifndef CARL_DISABLE_PROXY
  case CARLOPT_PROXY_TLSAUTH_TYPE:
    argptr = va_arg(param, char *);
    if(!argptr ||
       strncasecompare(argptr, "SRP", strlen("SRP")))
      data->set.proxy_ssl.authtype = CARL_TLSAUTH_SRP;
    else
      data->set.proxy_ssl.authtype = CARL_TLSAUTH_NONE;
    break;
#endif
#endif
#ifdef USE_ARES
  case CARLOPT_DNS_SERVERS:
    result = Curl_setstropt(&data->set.str[STRING_DNS_SERVERS],
                            va_arg(param, char *));
    if(result)
      return result;
    result = Curl_set_dns_servers(data, data->set.str[STRING_DNS_SERVERS]);
    break;
  case CARLOPT_DNS_INTERFACE:
    result = Curl_setstropt(&data->set.str[STRING_DNS_INTERFACE],
                            va_arg(param, char *));
    if(result)
      return result;
    result = Curl_set_dns_interface(data, data->set.str[STRING_DNS_INTERFACE]);
    break;
  case CARLOPT_DNS_LOCAL_IP4:
    result = Curl_setstropt(&data->set.str[STRING_DNS_LOCAL_IP4],
                            va_arg(param, char *));
    if(result)
      return result;
    result = Curl_set_dns_local_ip4(data, data->set.str[STRING_DNS_LOCAL_IP4]);
    break;
  case CARLOPT_DNS_LOCAL_IP6:
    result = Curl_setstropt(&data->set.str[STRING_DNS_LOCAL_IP6],
                            va_arg(param, char *));
    if(result)
      return result;
    result = Curl_set_dns_local_ip6(data, data->set.str[STRING_DNS_LOCAL_IP6]);
    break;
#endif
  case CARLOPT_TCP_KEEPALIVE:
    data->set.tcp_keepalive = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_TCP_KEEPIDLE:
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.tcp_keepidle = arg;
    break;
  case CARLOPT_TCP_KEEPINTVL:
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.tcp_keepintvl = arg;
    break;
  case CARLOPT_TCP_FASTOPEN:
#if defined(CONNECT_DATA_IDEMPOTENT) || defined(MSG_FASTOPEN) || \
   defined(TCP_FASTOPEN_CONNECT)
    data->set.tcp_fastopen = (0 != va_arg(param, long))?TRUE:FALSE;
#else
    result = CARLE_NOT_BUILT_IN;
#endif
    break;
  case CARLOPT_SSL_ENABLE_NPN:
    data->set.ssl_enable_npn = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_SSL_ENABLE_ALPN:
    data->set.ssl_enable_alpn = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
#ifdef USE_UNIX_SOCKETS
  case CARLOPT_UNIX_SOCKET_PATH:
    data->set.abstract_unix_socket = FALSE;
    result = Curl_setstropt(&data->set.str[STRING_UNIX_SOCKET_PATH],
                            va_arg(param, char *));
    break;
  case CARLOPT_ABSTRACT_UNIX_SOCKET:
    data->set.abstract_unix_socket = TRUE;
    result = Curl_setstropt(&data->set.str[STRING_UNIX_SOCKET_PATH],
                            va_arg(param, char *));
    break;
#endif

  case CARLOPT_PATH_AS_IS:
    data->set.path_as_is = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_PIPEWAIT:
    data->set.pipewait = (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
  case CARLOPT_STREAM_WEIGHT:
#ifndef USE_NGHTTP2
    return CARLE_NOT_BUILT_IN;
#else
    arg = va_arg(param, long);
    if((arg >= 1) && (arg <= 256))
      data->set.stream_weight = (int)arg;
    break;
#endif
  case CARLOPT_STREAM_DEPENDS:
  case CARLOPT_STREAM_DEPENDS_E:
  {
#ifndef USE_NGHTTP2
    return CARLE_NOT_BUILT_IN;
#else
    struct Curl_easy *dep = va_arg(param, struct Curl_easy *);
    if(!dep || GOOD_EASY_HANDLE(dep)) {
      if(data->set.stream_depends_on) {
        Curl_http2_remove_child(data->set.stream_depends_on, data);
      }
      Curl_http2_add_child(dep, data, (option == CARLOPT_STREAM_DEPENDS_E));
    }
    break;
#endif
  }
  case CARLOPT_CONNECT_TO:
    data->set.connect_to = va_arg(param, struct carl_slist *);
    break;
  case CARLOPT_SUPPRESS_CONNECT_HEADERS:
    data->set.suppress_connect_headers = (0 != va_arg(param, long))?TRUE:FALSE;
    break;
  case CARLOPT_HAPPY_EYEBALLS_TIMEOUT_MS:
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.happy_eyeballs_timeout = arg;
    break;
#ifndef CARL_DISABLE_SHUFFLE_DNS
  case CARLOPT_DNS_SHUFFLE_ADDRESSES:
    data->set.dns_shuffle_addresses = (0 != va_arg(param, long)) ? TRUE:FALSE;
    break;
#endif
  case CARLOPT_DISALLOW_USERNAME_IN_URL:
    data->set.disallow_username_in_url =
      (0 != va_arg(param, long)) ? TRUE : FALSE;
    break;
#ifndef CARL_DISABLE_DOH
  case CARLOPT_DOH_URL:
    result = Curl_setstropt(&data->set.str[STRING_DOH],
                            va_arg(param, char *));
    data->set.doh = data->set.str[STRING_DOH]?TRUE:FALSE;
    break;
#endif
  case CARLOPT_UPKEEP_INTERVAL_MS:
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.upkeep_interval_ms = arg;
    break;
  case CARLOPT_MAXAGE_CONN:
    arg = va_arg(param, long);
    if(arg < 0)
      return CARLE_BAD_FUNCTION_ARGUMENT;
    data->set.maxage_conn = arg;
    break;
  case CARLOPT_TRAILERFUNCTION:
#ifndef CARL_DISABLE_HTTP
    data->set.trailer_callback = va_arg(param, carl_trailer_callback);
#endif
    break;
  case CARLOPT_TRAILERDATA:
#ifndef CARL_DISABLE_HTTP
    data->set.trailer_data = va_arg(param, void *);
#endif
    break;
#ifdef USE_HSTS
  case CARLOPT_HSTSREADFUNCTION:
    data->set.hsts_read = va_arg(param, carl_hstsread_callback);
    break;
  case CARLOPT_HSTSREADDATA:
    data->set.hsts_read_userp = va_arg(param, void *);
    break;
  case CARLOPT_HSTSWRITEFUNCTION:
    data->set.hsts_write = va_arg(param, carl_hstswrite_callback);
    break;
  case CARLOPT_HSTSWRITEDATA:
    data->set.hsts_write_userp = va_arg(param, void *);
    break;
  case CARLOPT_HSTS:
    if(!data->hsts) {
      data->hsts = Curl_hsts_init();
      if(!data->hsts)
        return CARLE_OUT_OF_MEMORY;
    }
    argptr = va_arg(param, char *);
    result = Curl_setstropt(&data->set.str[STRING_HSTS], argptr);
    if(result)
      return result;
    if(argptr)
      (void)Curl_hsts_loadfile(data, data->hsts, argptr);
    break;
  case CARLOPT_HSTS_CTRL:
    arg = va_arg(param, long);
    if(arg & CARLHSTS_ENABLE) {
      if(!data->hsts) {
        data->hsts = Curl_hsts_init();
        if(!data->hsts)
          return CARLE_OUT_OF_MEMORY;
      }
    }
    else
      Curl_hsts_cleanup(&data->hsts);
    break;
#endif
#ifndef CARL_DISABLE_ALTSVC
  case CARLOPT_ALTSVC:
    if(!data->asi) {
      data->asi = Curl_altsvc_init();
      if(!data->asi)
        return CARLE_OUT_OF_MEMORY;
    }
    argptr = va_arg(param, char *);
    result = Curl_setstropt(&data->set.str[STRING_ALTSVC], argptr);
    if(result)
      return result;
    if(argptr)
      (void)Curl_altsvc_load(data->asi, argptr);
    break;
  case CARLOPT_ALTSVC_CTRL:
    if(!data->asi) {
      data->asi = Curl_altsvc_init();
      if(!data->asi)
        return CARLE_OUT_OF_MEMORY;
    }
    arg = va_arg(param, long);
    result = Curl_altsvc_ctrl(data->asi, arg);
    if(result)
      return result;
    break;
#endif
  default:
    /* unknown tag and its companion, just ignore: */
    result = CARLE_UNKNOWN_OPTION;
    break;
  }

  return result;
}

/*
 * carl_easy_setopt() is the external interface for setting options on an
 * easy handle.
 *
 * NOTE: This is one of few API functions that are allowed to be called from
 * within a callback.
 */

#undef carl_easy_setopt
CARLcode carl_easy_setopt(struct Curl_easy *data, CARLoption tag, ...)
{
  va_list arg;
  CARLcode result;

  if(!data)
    return CARLE_BAD_FUNCTION_ARGUMENT;

  va_start(arg, tag);

  result = Curl_vsetopt(data, tag, arg);

  va_end(arg);
  return result;
}
