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

#ifdef USE_GSKIT

#include <gskssl.h>
#include <qsoasync.h>
#undef HAVE_SOCKETPAIR /* because the native one isn't good enough */
#include "socketpair.h"

/* Some symbols are undefined/unsupported on OS400 versions < V7R1. */
#ifndef GSK_SSL_EXTN_SERVERNAME_REQUEST
#define GSK_SSL_EXTN_SERVERNAME_REQUEST         230
#endif

#ifndef GSK_TLSV10_CIPHER_SPECS
#define GSK_TLSV10_CIPHER_SPECS                 236
#endif

#ifndef GSK_TLSV11_CIPHER_SPECS
#define GSK_TLSV11_CIPHER_SPECS                 237
#endif

#ifndef GSK_TLSV12_CIPHER_SPECS
#define GSK_TLSV12_CIPHER_SPECS                 238
#endif

#ifndef GSK_PROTOCOL_TLSV11
#define GSK_PROTOCOL_TLSV11                     437
#endif

#ifndef GSK_PROTOCOL_TLSV12
#define GSK_PROTOCOL_TLSV12                     438
#endif

#ifndef GSK_FALSE
#define GSK_FALSE                               0
#endif

#ifndef GSK_TRUE
#define GSK_TRUE                                1
#endif


#include <limits.h>

#include <carl/carl.h>
#include "urldata.h"
#include "sendf.h"
#include "gskit.h"
#include "vtls.h"
#include "connect.h" /* for the connect timeout */
#include "select.h"
#include "strcase.h"
#include "x509asn1.h"
#include "carl_printf.h"

#include "carl_memory.h"
/* The last #include file should be: */
#include "memdebug.h"


/* Directions. */
#define SOS_READ        0x01
#define SOS_WRITE       0x02

/* SSL version flags. */
#define CARL_GSKPROTO_SSLV2     0
#define CARL_GSKPROTO_SSLV2_MASK        (1 << CARL_GSKPROTO_SSLV2)
#define CARL_GSKPROTO_SSLV3     1
#define CARL_GSKPROTO_SSLV3_MASK        (1 << CARL_GSKPROTO_SSLV3)
#define CARL_GSKPROTO_TLSV10    2
#define CARL_GSKPROTO_TLSV10_MASK        (1 << CARL_GSKPROTO_TLSV10)
#define CARL_GSKPROTO_TLSV11    3
#define CARL_GSKPROTO_TLSV11_MASK        (1 << CARL_GSKPROTO_TLSV11)
#define CARL_GSKPROTO_TLSV12    4
#define CARL_GSKPROTO_TLSV12_MASK        (1 << CARL_GSKPROTO_TLSV12)
#define CARL_GSKPROTO_LAST      5

struct ssl_backend_data {
  gsk_handle handle;
  int iocport;
  int localfd;
  int remotefd;
};

#define BACKEND connssl->backend

/* Supported ciphers. */
struct gskit_cipher {
  const char *name;            /* Cipher name. */
  const char *gsktoken;        /* Corresponding token for GSKit String. */
  unsigned int versions;       /* SSL version flags. */
};

static const struct gskit_cipher  ciphertable[] = {
  { "null-md5",         "01",
      CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK |
      CARL_GSKPROTO_TLSV11_MASK | CARL_GSKPROTO_TLSV12_MASK },
  { "null-sha",         "02",
      CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK |
      CARL_GSKPROTO_TLSV11_MASK | CARL_GSKPROTO_TLSV12_MASK },
  { "exp-rc4-md5",      "03",
      CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK },
  { "rc4-md5",          "04",
      CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK |
      CARL_GSKPROTO_TLSV11_MASK | CARL_GSKPROTO_TLSV12_MASK },
  { "rc4-sha",          "05",
      CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK |
      CARL_GSKPROTO_TLSV11_MASK | CARL_GSKPROTO_TLSV12_MASK },
  { "exp-rc2-cbc-md5",  "06",
      CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK },
  { "exp-des-cbc-sha",  "09",
      CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK |
      CARL_GSKPROTO_TLSV11_MASK },
  { "des-cbc3-sha",     "0A",
      CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK |
      CARL_GSKPROTO_TLSV11_MASK | CARL_GSKPROTO_TLSV12_MASK },
  { "aes128-sha",       "2F",
      CARL_GSKPROTO_TLSV10_MASK | CARL_GSKPROTO_TLSV11_MASK |
      CARL_GSKPROTO_TLSV12_MASK },
  { "aes256-sha",       "35",
      CARL_GSKPROTO_TLSV10_MASK | CARL_GSKPROTO_TLSV11_MASK |
      CARL_GSKPROTO_TLSV12_MASK },
  { "null-sha256",      "3B",   CARL_GSKPROTO_TLSV12_MASK },
  { "aes128-sha256",    "3C",   CARL_GSKPROTO_TLSV12_MASK },
  { "aes256-sha256",    "3D",   CARL_GSKPROTO_TLSV12_MASK },
  { "aes128-gcm-sha256",
                        "9C",   CARL_GSKPROTO_TLSV12_MASK },
  { "aes256-gcm-sha384",
                        "9D",   CARL_GSKPROTO_TLSV12_MASK },
  { "rc4-md5",          "1",    CARL_GSKPROTO_SSLV2_MASK },
  { "exp-rc4-md5",      "2",    CARL_GSKPROTO_SSLV2_MASK },
  { "rc2-md5",          "3",    CARL_GSKPROTO_SSLV2_MASK },
  { "exp-rc2-md5",      "4",    CARL_GSKPROTO_SSLV2_MASK },
  { "des-cbc-md5",      "6",    CARL_GSKPROTO_SSLV2_MASK },
  { "des-cbc3-md5",     "7",    CARL_GSKPROTO_SSLV2_MASK },
  { (const char *) NULL, (const char *) NULL, 0       }
};


static bool is_separator(char c)
{
  /* Return whether character is a cipher list separator. */
  switch(c) {
  case ' ':
  case '\t':
  case ':':
  case ',':
  case ';':
    return true;
  }
  return false;
}


static CARLcode gskit_status(struct Curl_easy *data, int rc,
                             const char *procname, CARLcode defcode)
{
  /* Process GSKit status and map it to a CARLcode. */
  switch(rc) {
  case GSK_OK:
  case GSK_OS400_ASYNCHRONOUS_SOC_INIT:
    return CARLE_OK;
  case GSK_KEYRING_OPEN_ERROR:
  case GSK_OS400_ERROR_NO_ACCESS:
    return CARLE_SSL_CACERT_BADFILE;
  case GSK_INSUFFICIENT_STORAGE:
    return CARLE_OUT_OF_MEMORY;
  case GSK_ERROR_BAD_V2_CIPHER:
  case GSK_ERROR_BAD_V3_CIPHER:
  case GSK_ERROR_NO_CIPHERS:
    return CARLE_SSL_CIPHER;
  case GSK_OS400_ERROR_NOT_TRUSTED_ROOT:
  case GSK_ERROR_CERT_VALIDATION:
    return CARLE_PEER_FAILED_VERIFICATION;
  case GSK_OS400_ERROR_TIMED_OUT:
    return CARLE_OPERATION_TIMEDOUT;
  case GSK_WOULD_BLOCK:
    return CARLE_AGAIN;
  case GSK_OS400_ERROR_NOT_REGISTERED:
    break;
  case GSK_ERROR_IO:
    switch(errno) {
    case ENOMEM:
      return CARLE_OUT_OF_MEMORY;
    default:
      failf(data, "%s I/O error: %s", procname, strerror(errno));
      break;
    }
    break;
  default:
    failf(data, "%s: %s", procname, gsk_strerror(rc));
    break;
  }
  return defcode;
}


static CARLcode set_enum(struct Curl_easy *data, gsk_handle h,
                GSK_ENUM_ID id, GSK_ENUM_VALUE value, bool unsupported_ok)
{
  int rc = gsk_attribute_set_enum(h, id, value);

  switch(rc) {
  case GSK_OK:
    return CARLE_OK;
  case GSK_ERROR_IO:
    failf(data, "gsk_attribute_set_enum() I/O error: %s", strerror(errno));
    break;
  case GSK_ATTRIBUTE_INVALID_ID:
    if(unsupported_ok)
      return CARLE_UNSUPPORTED_PROTOCOL;
  default:
    failf(data, "gsk_attribute_set_enum(): %s", gsk_strerror(rc));
    break;
  }
  return CARLE_SSL_CONNECT_ERROR;
}


static CARLcode set_buffer(struct Curl_easy *data, gsk_handle h,
                        GSK_BUF_ID id, const char *buffer, bool unsupported_ok)
{
  int rc = gsk_attribute_set_buffer(h, id, buffer, 0);

  switch(rc) {
  case GSK_OK:
    return CARLE_OK;
  case GSK_ERROR_IO:
    failf(data, "gsk_attribute_set_buffer() I/O error: %s", strerror(errno));
    break;
  case GSK_ATTRIBUTE_INVALID_ID:
    if(unsupported_ok)
      return CARLE_UNSUPPORTED_PROTOCOL;
  default:
    failf(data, "gsk_attribute_set_buffer(): %s", gsk_strerror(rc));
    break;
  }
  return CARLE_SSL_CONNECT_ERROR;
}


static CARLcode set_numeric(struct Curl_easy *data,
                            gsk_handle h, GSK_NUM_ID id, int value)
{
  int rc = gsk_attribute_set_numeric_value(h, id, value);

  switch(rc) {
  case GSK_OK:
    return CARLE_OK;
  case GSK_ERROR_IO:
    failf(data, "gsk_attribute_set_numeric_value() I/O error: %s",
          strerror(errno));
    break;
  default:
    failf(data, "gsk_attribute_set_numeric_value(): %s", gsk_strerror(rc));
    break;
  }
  return CARLE_SSL_CONNECT_ERROR;
}


static CARLcode set_callback(struct Curl_easy *data,
                             gsk_handle h, GSK_CALLBACK_ID id, void *info)
{
  int rc = gsk_attribute_set_callback(h, id, info);

  switch(rc) {
  case GSK_OK:
    return CARLE_OK;
  case GSK_ERROR_IO:
    failf(data, "gsk_attribute_set_callback() I/O error: %s", strerror(errno));
    break;
  default:
    failf(data, "gsk_attribute_set_callback(): %s", gsk_strerror(rc));
    break;
  }
  return CARLE_SSL_CONNECT_ERROR;
}


static CARLcode set_ciphers(struct Curl_easy *data,
                                        gsk_handle h, unsigned int *protoflags)
{
  const char *cipherlist = SSL_CONN_CONFIG(cipher_list);
  const char *clp;
  const struct gskit_cipher *ctp;
  int i;
  int l;
  bool unsupported;
  CARLcode result;
  struct {
    char *buf;
    char *ptr;
  } ciphers[CARL_GSKPROTO_LAST];

  /* Compile cipher list into GSKit-compatible cipher lists. */

  if(!cipherlist)
    return CARLE_OK;
  while(is_separator(*cipherlist))     /* Skip initial separators. */
    cipherlist++;
  if(!*cipherlist)
    return CARLE_OK;

  /* We allocate GSKit buffers of the same size as the input string: since
     GSKit tokens are always shorter than their cipher names, allocated buffers
     will always be large enough to accommodate the result. */
  l = strlen(cipherlist) + 1;
  memset((char *) ciphers, 0, sizeof(ciphers));
  for(i = 0; i < CARL_GSKPROTO_LAST; i++) {
    ciphers[i].buf = malloc(l);
    if(!ciphers[i].buf) {
      while(i--)
        free(ciphers[i].buf);
      return CARLE_OUT_OF_MEMORY;
    }
    ciphers[i].ptr = ciphers[i].buf;
    *ciphers[i].ptr = '\0';
  }

  /* Process each cipher in input string. */
  unsupported = FALSE;
  result = CARLE_OK;
  for(;;) {
    for(clp = cipherlist; *cipherlist && !is_separator(*cipherlist);)
      cipherlist++;
    l = cipherlist - clp;
    if(!l)
      break;
    /* Search the cipher in our table. */
    for(ctp = ciphertable; ctp->name; ctp++)
      if(strncasecompare(ctp->name, clp, l) && !ctp->name[l])
        break;
    if(!ctp->name) {
      failf(data, "Unknown cipher %.*s", l, clp);
      result = CARLE_SSL_CIPHER;
    }
    else {
      unsupported |= !(ctp->versions & (CARL_GSKPROTO_SSLV2_MASK |
                        CARL_GSKPROTO_SSLV3_MASK | CARL_GSKPROTO_TLSV10_MASK));
      for(i = 0; i < CARL_GSKPROTO_LAST; i++) {
        if(ctp->versions & (1 << i)) {
          strcpy(ciphers[i].ptr, ctp->gsktoken);
          ciphers[i].ptr += strlen(ctp->gsktoken);
        }
      }
    }

   /* Advance to next cipher name or end of string. */
    while(is_separator(*cipherlist))
      cipherlist++;
  }

  /* Disable protocols with empty cipher lists. */
  for(i = 0; i < CARL_GSKPROTO_LAST; i++) {
    if(!(*protoflags & (1 << i)) || !ciphers[i].buf[0]) {
      *protoflags &= ~(1 << i);
      ciphers[i].buf[0] = '\0';
    }
  }

  /* Try to set-up TLSv1.1 and TLSv2.1 ciphers. */
  if(*protoflags & CARL_GSKPROTO_TLSV11_MASK) {
    result = set_buffer(data, h, GSK_TLSV11_CIPHER_SPECS,
                        ciphers[CARL_GSKPROTO_TLSV11].buf, TRUE);
    if(result == CARLE_UNSUPPORTED_PROTOCOL) {
      result = CARLE_OK;
      if(unsupported) {
        failf(data, "TLSv1.1-only ciphers are not yet supported");
        result = CARLE_SSL_CIPHER;
      }
    }
  }
  if(!result && (*protoflags & CARL_GSKPROTO_TLSV12_MASK)) {
    result = set_buffer(data, h, GSK_TLSV12_CIPHER_SPECS,
                        ciphers[CARL_GSKPROTO_TLSV12].buf, TRUE);
    if(result == CARLE_UNSUPPORTED_PROTOCOL) {
      result = CARLE_OK;
      if(unsupported) {
        failf(data, "TLSv1.2-only ciphers are not yet supported");
        result = CARLE_SSL_CIPHER;
      }
    }
  }

  /* Try to set-up TLSv1.0 ciphers. If not successful, concatenate them to
     the SSLv3 ciphers. OS/400 prior to version 7.1 will understand it. */
  if(!result && (*protoflags & CARL_GSKPROTO_TLSV10_MASK)) {
    result = set_buffer(data, h, GSK_TLSV10_CIPHER_SPECS,
                        ciphers[CARL_GSKPROTO_TLSV10].buf, TRUE);
    if(result == CARLE_UNSUPPORTED_PROTOCOL) {
      result = CARLE_OK;
      strcpy(ciphers[CARL_GSKPROTO_SSLV3].ptr,
             ciphers[CARL_GSKPROTO_TLSV10].ptr);
    }
  }

  /* Set-up other ciphers. */
  if(!result && (*protoflags & CARL_GSKPROTO_SSLV3_MASK))
    result = set_buffer(data, h, GSK_V3_CIPHER_SPECS,
                        ciphers[CARL_GSKPROTO_SSLV3].buf, FALSE);
  if(!result && (*protoflags & CARL_GSKPROTO_SSLV2_MASK))
    result = set_buffer(data, h, GSK_V2_CIPHER_SPECS,
                        ciphers[CARL_GSKPROTO_SSLV2].buf, FALSE);

  /* Clean-up. */
  for(i = 0; i < CARL_GSKPROTO_LAST; i++)
    free(ciphers[i].buf);

  return result;
}


static int gskit_init(void)
{
  /* No initialisation needed. */

  return 1;
}


static void gskit_cleanup(void)
{
  /* Nothing to do. */
}


static CARLcode init_environment(struct Curl_easy *data,
                                 gsk_handle *envir, const char *appid,
                                 const char *file, const char *label,
                                 const char *password)
{
  int rc;
  CARLcode result;
  gsk_handle h;

  /* Creates the GSKit environment. */

  rc = gsk_environment_open(&h);
  switch(rc) {
  case GSK_OK:
    break;
  case GSK_INSUFFICIENT_STORAGE:
    return CARLE_OUT_OF_MEMORY;
  default:
    failf(data, "gsk_environment_open(): %s", gsk_strerror(rc));
    return CARLE_SSL_CONNECT_ERROR;
  }

  result = set_enum(data, h, GSK_SESSION_TYPE, GSK_CLIENT_SESSION, FALSE);
  if(!result && appid)
    result = set_buffer(data, h, GSK_OS400_APPLICATION_ID, appid, FALSE);
  if(!result && file)
    result = set_buffer(data, h, GSK_KEYRING_FILE, file, FALSE);
  if(!result && label)
    result = set_buffer(data, h, GSK_KEYRING_LABEL, label, FALSE);
  if(!result && password)
    result = set_buffer(data, h, GSK_KEYRING_PW, password, FALSE);

  if(!result) {
    /* Locate CAs, Client certificate and key according to our settings.
       Note: this call may be blocking for some tenths of seconds. */
    result = gskit_status(data, gsk_environment_init(h),
                          "gsk_environment_init()", CARLE_SSL_CERTPROBLEM);
    if(!result) {
      *envir = h;
      return result;
    }
  }
  /* Error: rollback. */
  gsk_environment_close(&h);
  return result;
}


static void cancel_async_handshake(struct connectdata *conn, int sockindex)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  Qso_OverlappedIO_t cstat;

  if(QsoCancelOperation(conn->sock[sockindex], 0) > 0)
    QsoWaitForIOCompletion(BACKEND->iocport, &cstat, (struct timeval *) NULL);
}


static void close_async_handshake(struct ssl_connect_data *connssl)
{
  QsoDestroyIOCompletionPort(BACKEND->iocport);
  BACKEND->iocport = -1;
}

static int pipe_ssloverssl(struct connectdata *conn, int sockindex,
                           int directions)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_connect_data *connproxyssl = &conn->proxy_ssl[sockindex];
  fd_set fds_read;
  fd_set fds_write;
  int n;
  int m;
  int i;
  int ret = 0;
  char buf[CARL_MAX_WRITE_SIZE];

  if(!connssl->use || !connproxyssl->use)
    return 0;   /* No SSL over SSL: OK. */

  FD_ZERO(&fds_read);
  FD_ZERO(&fds_write);
  n = -1;
  if(directions & SOS_READ) {
    FD_SET(BACKEND->remotefd, &fds_write);
    n = BACKEND->remotefd;
  }
  if(directions & SOS_WRITE) {
    FD_SET(BACKEND->remotefd, &fds_read);
    n = BACKEND->remotefd;
    FD_SET(conn->sock[sockindex], &fds_write);
    if(n < conn->sock[sockindex])
      n = conn->sock[sockindex];
  }
  i = Curl_select(n + 1, &fds_read, &fds_write, NULL, 0);
  if(i < 0)
    return -1;  /* Select error. */

  if(FD_ISSET(BACKEND->remotefd, &fds_write)) {
    /* Try getting data from HTTPS proxy and pipe it upstream. */
    n = 0;
    i = gsk_secure_soc_read(connproxyssl->backend->handle,
                            buf, sizeof(buf), &n);
    switch(i) {
    case GSK_OK:
      if(n) {
        i = write(BACKEND->remotefd, buf, n);
        if(i < 0)
          return -1;
        ret = 1;
      }
      break;
    case GSK_OS400_ERROR_TIMED_OUT:
    case GSK_WOULD_BLOCK:
      break;
    default:
      return -1;
    }
  }

  if(FD_ISSET(BACKEND->remotefd, &fds_read) &&
     FD_ISSET(conn->sock[sockindex], &fds_write)) {
    /* Pipe data to HTTPS proxy. */
    n = read(BACKEND->remotefd, buf, sizeof(buf));
    if(n < 0)
      return -1;
    if(n) {
      i = gsk_secure_soc_write(connproxyssl->backend->handle, buf, n, &m);
      if(i != GSK_OK || n != m)
        return -1;
      ret = 1;
    }
  }

  return ret;  /* OK */
}


static void close_one(struct ssl_connect_data *connssl, struct Curl_easy *data,
                      struct connectdata *conn, int sockindex)
{
  if(BACKEND->handle) {
    gskit_status(data, gsk_secure_soc_close(&BACKEND->handle),
              "gsk_secure_soc_close()", 0);
    /* Last chance to drain output. */
    while(pipe_ssloverssl(conn, sockindex, SOS_WRITE) > 0)
      ;
    BACKEND->handle = (gsk_handle) NULL;
    if(BACKEND->localfd >= 0) {
      close(BACKEND->localfd);
      BACKEND->localfd = -1;
    }
    if(BACKEND->remotefd >= 0) {
      close(BACKEND->remotefd);
      BACKEND->remotefd = -1;
    }
  }
  if(BACKEND->iocport >= 0)
    close_async_handshake(connssl);
}


static ssize_t gskit_send(struct connectdata *conn, int sockindex,
                          const void *mem, size_t len, CARLcode *carlcode)
{
  struct connectdata *conn = data->conn;
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  CARLcode cc = CARLE_SEND_ERROR;
  int written;

  if(pipe_ssloverssl(conn, sockindex, SOS_WRITE) >= 0) {
    cc = gskit_status(data,
                      gsk_secure_soc_write(BACKEND->handle,
                                           (char *) mem, (int) len, &written),
                      "gsk_secure_soc_write()", CARLE_SEND_ERROR);
    if(cc == CARLE_OK)
      if(pipe_ssloverssl(conn, sockindex, SOS_WRITE) < 0)
        cc = CARLE_SEND_ERROR;
  }
  if(cc != CARLE_OK) {
    *carlcode = cc;
    written = -1;
  }
  return (ssize_t) written; /* number of bytes */
}


static ssize_t gskit_recv(struct Curl_easy *data, int num, char *buf,
                               size_t buffersize, CARLcode *carlcode)
{
  struct connectdata *conn = data->conn;
  struct ssl_connect_data *connssl = &conn->ssl[num];
  int nread;
  CARLcode cc = CARLE_RECV_ERROR;

  if(pipe_ssloverssl(conn, num, SOS_READ) >= 0) {
    int buffsize = buffersize > (size_t) INT_MAX? INT_MAX: (int) buffersize;
    cc = gskit_status(data, gsk_secure_soc_read(BACKEND->handle,
                                                buf, buffsize, &nread),
                      "gsk_secure_soc_read()", CARLE_RECV_ERROR);
  }
  switch(cc) {
  case CARLE_OK:
    break;
  case CARLE_OPERATION_TIMEDOUT:
    cc = CARLE_AGAIN;
  default:
    *carlcode = cc;
    nread = -1;
    break;
  }
  return (ssize_t) nread;
}

static CARLcode
set_ssl_version_min_max(unsigned int *protoflags, struct Curl_easy *data)
{
  long ssl_version = SSL_CONN_CONFIG(version);
  long ssl_version_max = SSL_CONN_CONFIG(version_max);
  long i = ssl_version;
  switch(ssl_version_max) {
    case CARL_SSLVERSION_MAX_NONE:
    case CARL_SSLVERSION_MAX_DEFAULT:
      ssl_version_max = CARL_SSLVERSION_TLSv1_2;
      break;
  }
  for(; i <= (ssl_version_max >> 16); ++i) {
    switch(i) {
      case CARL_SSLVERSION_TLSv1_0:
        *protoflags |= CARL_GSKPROTO_TLSV10_MASK;
        break;
      case CARL_SSLVERSION_TLSv1_1:
        *protoflags |= CARL_GSKPROTO_TLSV11_MASK;
        break;
      case CARL_SSLVERSION_TLSv1_2:
        *protoflags |= CARL_GSKPROTO_TLSV11_MASK;
        break;
      case CARL_SSLVERSION_TLSv1_3:
        failf(data, "GSKit: TLS 1.3 is not yet supported");
        return CARLE_SSL_CONNECT_ERROR;
    }
  }

  return CARLE_OK;
}

static CARLcode gskit_connect_step1(struct Curl_easy *data,
                                    struct connectdata *conn, int sockindex)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  gsk_handle envir;
  CARLcode result;
  int rc;
  const char * const keyringfile = SSL_CONN_CONFIG(CAfile);
  const char * const keyringpwd = SSL_SET_OPTION(key_passwd);
  const char * const keyringlabel = SSL_SET_OPTION(primary.clientcert);
  const long int ssl_version = SSL_CONN_CONFIG(version);
  const bool verifypeer = SSL_CONN_CONFIG(verifypeer);
  const char * const hostname = SSL_IS_PROXY()? conn->http_proxy.host.name:
    conn->host.name;
  const char *sni;
  unsigned int protoflags = 0;
  Qso_OverlappedIO_t commarea;
  int sockpair[2];
  static const int sobufsize = CARL_MAX_WRITE_SIZE;

  /* Create SSL environment, start (preferably asynchronous) handshake. */

  BACKEND->handle = (gsk_handle) NULL;
  BACKEND->iocport = -1;
  BACKEND->localfd = -1;
  BACKEND->remotefd = -1;

  /* GSKit supports two ways of specifying an SSL context: either by
   *  application identifier (that should have been defined at the system
   *  level) or by keyring file, password and certificate label.
   * Local certificate name (CARLOPT_SSLCERT) is used to hold either the
   *  application identifier of the certificate label.
   * Key password (CARLOPT_KEYPASSWD) holds the keyring password.
   * It is not possible to have different keyrings for the CAs and the
   *  local certificate. We thus use the CA file (CARLOPT_CAINFO) to identify
   *  the keyring file.
   * If no key password is given and the keyring is the system keyring,
   *  application identifier mode is tried first, as recommended in IBM doc.
   */

  envir = (gsk_handle) NULL;

  if(keyringlabel && *keyringlabel && !keyringpwd &&
      !strcmp(keyringfile, CARL_CA_BUNDLE)) {
    /* Try application identifier mode. */
    init_environment(data, &envir, keyringlabel, (const char *) NULL,
                     (const char *) NULL, (const char *) NULL);
  }

  if(!envir) {
    /* Use keyring mode. */
    result = init_environment(data, &envir, (const char *) NULL,
                              keyringfile, keyringlabel, keyringpwd);
    if(result)
      return result;
  }

  /* Create secure session. */
  result = gskit_status(data, gsk_secure_soc_open(envir, &BACKEND->handle),
                        "gsk_secure_soc_open()", CARLE_SSL_CONNECT_ERROR);
  gsk_environment_close(&envir);
  if(result)
    return result;

  /* Establish a pipelining socket pair for SSL over SSL. */
  if(conn->proxy_ssl[sockindex].use) {
    if(Curl_socketpair(0, 0, 0, sockpair))
      return CARLE_SSL_CONNECT_ERROR;
    BACKEND->localfd = sockpair[0];
    BACKEND->remotefd = sockpair[1];
    setsockopt(BACKEND->localfd, SOL_SOCKET, SO_RCVBUF,
               (void *) sobufsize, sizeof(sobufsize));
    setsockopt(BACKEND->remotefd, SOL_SOCKET, SO_RCVBUF,
               (void *) sobufsize, sizeof(sobufsize));
    setsockopt(BACKEND->localfd, SOL_SOCKET, SO_SNDBUF,
               (void *) sobufsize, sizeof(sobufsize));
    setsockopt(BACKEND->remotefd, SOL_SOCKET, SO_SNDBUF,
               (void *) sobufsize, sizeof(sobufsize));
    carlx_nonblock(BACKEND->localfd, TRUE);
    carlx_nonblock(BACKEND->remotefd, TRUE);
  }

  /* Determine which SSL/TLS version should be enabled. */
  sni = hostname;
  switch(ssl_version) {
  case CARL_SSLVERSION_SSLv2:
    protoflags = CARL_GSKPROTO_SSLV2_MASK;
    sni = NULL;
    break;
  case CARL_SSLVERSION_SSLv3:
    protoflags = CARL_GSKPROTO_SSLV3_MASK;
    sni = NULL;
    break;
  case CARL_SSLVERSION_DEFAULT:
  case CARL_SSLVERSION_TLSv1:
    protoflags = CARL_GSKPROTO_TLSV10_MASK |
                 CARL_GSKPROTO_TLSV11_MASK | CARL_GSKPROTO_TLSV12_MASK;
    break;
  case CARL_SSLVERSION_TLSv1_0:
  case CARL_SSLVERSION_TLSv1_1:
  case CARL_SSLVERSION_TLSv1_2:
  case CARL_SSLVERSION_TLSv1_3:
    result = set_ssl_version_min_max(&protoflags, data);
    if(result != CARLE_OK)
      return result;
    break;
  default:
    failf(data, "Unrecognized parameter passed via CARLOPT_SSLVERSION");
    return CARLE_SSL_CONNECT_ERROR;
  }

  /* Process SNI. Ignore if not supported (on OS400 < V7R1). */
  if(sni) {
    result = set_buffer(data, BACKEND->handle,
                        GSK_SSL_EXTN_SERVERNAME_REQUEST, sni, TRUE);
    if(result == CARLE_UNSUPPORTED_PROTOCOL)
      result = CARLE_OK;
  }

  /* Set session parameters. */
  if(!result) {
    /* Compute the handshake timeout. Since GSKit granularity is 1 second,
       we round up the required value. */
    timediff_t timeout = Curl_timeleft(data, NULL, TRUE);
    if(timeout < 0)
      result = CARLE_OPERATION_TIMEDOUT;
    else
      result = set_numeric(data, BACKEND->handle, GSK_HANDSHAKE_TIMEOUT,
                           (timeout + 999) / 1000);
  }
  if(!result)
    result = set_numeric(data, BACKEND->handle, GSK_OS400_READ_TIMEOUT, 1);
  if(!result)
    result = set_numeric(data, BACKEND->handle, GSK_FD, BACKEND->localfd >= 0?
                         BACKEND->localfd: conn->sock[sockindex]);
  if(!result)
    result = set_ciphers(data, BACKEND->handle, &protoflags);
  if(!protoflags) {
    failf(data, "No SSL protocol/cipher combination enabled");
    result = CARLE_SSL_CIPHER;
  }
  if(!result)
    result = set_enum(data, BACKEND->handle, GSK_PROTOCOL_SSLV2,
                      (protoflags & CARL_GSKPROTO_SSLV2_MASK)?
                      GSK_PROTOCOL_SSLV2_ON: GSK_PROTOCOL_SSLV2_OFF, FALSE);
  if(!result)
    result = set_enum(data, BACKEND->handle, GSK_PROTOCOL_SSLV3,
                      (protoflags & CARL_GSKPROTO_SSLV3_MASK)?
                      GSK_PROTOCOL_SSLV3_ON: GSK_PROTOCOL_SSLV3_OFF, FALSE);
  if(!result)
    result = set_enum(data, BACKEND->handle, GSK_PROTOCOL_TLSV1,
                      (protoflags & CARL_GSKPROTO_TLSV10_MASK)?
                      GSK_PROTOCOL_TLSV1_ON: GSK_PROTOCOL_TLSV1_OFF, FALSE);
  if(!result) {
    result = set_enum(data, BACKEND->handle, GSK_PROTOCOL_TLSV11,
                      (protoflags & CARL_GSKPROTO_TLSV11_MASK)?
                      GSK_TRUE: GSK_FALSE, TRUE);
    if(result == CARLE_UNSUPPORTED_PROTOCOL) {
      result = CARLE_OK;
      if(protoflags == CARL_GSKPROTO_TLSV11_MASK) {
        failf(data, "TLS 1.1 not yet supported");
        result = CARLE_SSL_CIPHER;
      }
    }
  }
  if(!result) {
    result = set_enum(data, BACKEND->handle, GSK_PROTOCOL_TLSV12,
                      (protoflags & CARL_GSKPROTO_TLSV12_MASK)?
                      GSK_TRUE: GSK_FALSE, TRUE);
    if(result == CARLE_UNSUPPORTED_PROTOCOL) {
      result = CARLE_OK;
      if(protoflags == CARL_GSKPROTO_TLSV12_MASK) {
        failf(data, "TLS 1.2 not yet supported");
        result = CARLE_SSL_CIPHER;
      }
    }
  }
  if(!result)
    result = set_enum(data, BACKEND->handle, GSK_SERVER_AUTH_TYPE,
                      verifypeer? GSK_SERVER_AUTH_FULL:
                      GSK_SERVER_AUTH_PASSTHRU, FALSE);

  if(!result) {
    /* Start handshake. Try asynchronous first. */
    memset(&commarea, 0, sizeof(commarea));
    BACKEND->iocport = QsoCreateIOCompletionPort();
    if(BACKEND->iocport != -1) {
      result = gskit_status(data,
                            gsk_secure_soc_startInit(BACKEND->handle,
                                                     BACKEND->iocport,
                                                     &commarea),
                            "gsk_secure_soc_startInit()",
                            CARLE_SSL_CONNECT_ERROR);
      if(!result) {
        connssl->connecting_state = ssl_connect_2;
        return CARLE_OK;
      }
      else
        close_async_handshake(connssl);
    }
    else if(errno != ENOBUFS)
      result = gskit_status(data, GSK_ERROR_IO,
                            "QsoCreateIOCompletionPort()", 0);
    else if(conn->proxy_ssl[sockindex].use) {
      /* Cannot pipeline while handshaking synchronously. */
      result = CARLE_SSL_CONNECT_ERROR;
    }
    else {
      /* No more completion port available. Use synchronous IO. */
      result = gskit_status(data, gsk_secure_soc_init(BACKEND->handle),
                            "gsk_secure_soc_init()", CARLE_SSL_CONNECT_ERROR);
      if(!result) {
        connssl->connecting_state = ssl_connect_3;
        return CARLE_OK;
      }
    }
  }

  /* Error: rollback. */
  close_one(connssl, data, conn, sockindex);
  return result;
}


static CARLcode gskit_connect_step2(struct Curl_easy *data,
                                    struct connectdata *conn, int sockindex,
                                    bool nonblocking)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  Qso_OverlappedIO_t cstat;
  struct timeval stmv;
  CARLcode result;

  /* Poll or wait for end of SSL asynchronous handshake. */

  for(;;) {
    timediff_t timeout_ms = nonblocking? 0: Curl_timeleft(data, NULL, TRUE);
    if(timeout_ms < 0)
      timeout_ms = 0;
    stmv.tv_sec = timeout_ms / 1000;
    stmv.tv_usec = (timeout_ms - stmv.tv_sec * 1000) * 1000;
    switch(QsoWaitForIOCompletion(BACKEND->iocport, &cstat, &stmv)) {
    case 1:             /* Operation complete. */
      break;
    case -1:            /* An error occurred: handshake still in progress. */
      if(errno == EINTR) {
        if(nonblocking)
          return CARLE_OK;
        continue;       /* Retry. */
      }
      if(errno != ETIME) {
        failf(data, "QsoWaitForIOCompletion() I/O error: %s", strerror(errno));
        cancel_async_handshake(conn, sockindex);
        close_async_handshake(connssl);
        return CARLE_SSL_CONNECT_ERROR;
      }
      /* FALL INTO... */
    case 0:             /* Handshake in progress, timeout occurred. */
      if(nonblocking)
        return CARLE_OK;
      cancel_async_handshake(conn, sockindex);
      close_async_handshake(connssl);
      return CARLE_OPERATION_TIMEDOUT;
    }
    break;
  }
  result = gskit_status(data, cstat.returnValue, "SSL handshake",
                        CARLE_SSL_CONNECT_ERROR);
  if(!result)
    connssl->connecting_state = ssl_connect_3;
  close_async_handshake(connssl);
  return result;
}


static CARLcode gskit_connect_step3(struct Curl_easy *data,
                                    struct connectdata *conn, int sockindex)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  const gsk_cert_data_elem *cdev;
  int cdec;
  const gsk_cert_data_elem *p;
  const char *cert = (const char *) NULL;
  const char *certend;
  const char *ptr;
  CARLcode result;

  /* SSL handshake done: gather certificate info and verify host. */

  if(gskit_status(data, gsk_attribute_get_cert_info(BACKEND->handle,
                                                    GSK_PARTNER_CERT_INFO,
                                                    &cdev, &cdec),
                  "gsk_attribute_get_cert_info()", CARLE_SSL_CONNECT_ERROR) ==
     CARLE_OK) {
    int i;

    infof(data, "Server certificate:\n");
    p = cdev;
    for(i = 0; i++ < cdec; p++)
      switch(p->cert_data_id) {
      case CERT_BODY_DER:
        cert = p->cert_data_p;
        certend = cert + cdev->cert_data_l;
        break;
      case CERT_DN_PRINTABLE:
        infof(data, "\t subject: %.*s\n", p->cert_data_l, p->cert_data_p);
        break;
      case CERT_ISSUER_DN_PRINTABLE:
        infof(data, "\t issuer: %.*s\n", p->cert_data_l, p->cert_data_p);
        break;
      case CERT_VALID_FROM:
        infof(data, "\t start date: %.*s\n", p->cert_data_l, p->cert_data_p);
        break;
      case CERT_VALID_TO:
        infof(data, "\t expire date: %.*s\n", p->cert_data_l, p->cert_data_p);
        break;
    }
  }

  /* Verify host. */
  result = Curl_verifyhost(data, conn, cert, certend);
  if(result)
    return result;

  /* The only place GSKit can get the whole CA chain is a validation
     callback where no user data pointer is available. Therefore it's not
     possible to copy this chain into our structures for CAINFO.
     However the server certificate may be available, thus we can return
     info about it. */
  if(data->set.ssl.certinfo) {
    result = Curl_ssl_init_certinfo(data, 1);
    if(result)
      return result;

    if(cert) {
      result = Curl_extract_certinfo(data, 0, cert, certend);
      if(result)
        return result;
    }
  }

  /* Check pinned public key. */
  ptr = SSL_IS_PROXY() ? data->set.str[STRING_SSL_PINNEDPUBLICKEY_PROXY] :
                         data->set.str[STRING_SSL_PINNEDPUBLICKEY_ORIG];
  if(!result && ptr) {
    carl_X509certificate x509;
    carl_asn1Element *p;

    if(Curl_parseX509(&x509, cert, certend))
      return CARLE_SSL_PINNEDPUBKEYNOTMATCH;
    p = &x509.subjectPublicKeyInfo;
    result = Curl_pin_peer_pubkey(data, ptr, p->header, p->end - p->header);
    if(result) {
      failf(data, "SSL: public key does not match pinned public key!");
      return result;
    }
  }

  connssl->connecting_state = ssl_connect_done;
  return CARLE_OK;
}


static CARLcode gskit_connect_common(struct Curl_easy *data,
                                     struct connectdata *conn, int sockindex,
                                     bool nonblocking, bool *done)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  timediff_t timeout_ms;
  CARLcode result = CARLE_OK;

  *done = connssl->state == ssl_connection_complete;
  if(*done)
    return CARLE_OK;

  /* Step 1: create session, start handshake. */
  if(connssl->connecting_state == ssl_connect_1) {
    /* check allowed time left */
    timeout_ms = Curl_timeleft(data, NULL, TRUE);

    if(timeout_ms < 0) {
      /* no need to continue if time already is up */
      failf(data, "SSL connection timeout");
      result = CARLE_OPERATION_TIMEDOUT;
    }
    else
      result = gskit_connect_step1(data, conn, sockindex);
  }

  /* Handle handshake pipelining. */
  if(!result)
    if(pipe_ssloverssl(conn, sockindex, SOS_READ | SOS_WRITE) < 0)
      result = CARLE_SSL_CONNECT_ERROR;

  /* Step 2: check if handshake is over. */
  if(!result && connssl->connecting_state == ssl_connect_2) {
    /* check allowed time left */
    timeout_ms = Curl_timeleft(data, NULL, TRUE);

    if(timeout_ms < 0) {
      /* no need to continue if time already is up */
      failf(data, "SSL connection timeout");
      result = CARLE_OPERATION_TIMEDOUT;
    }
    else
      result = gskit_connect_step2(data, conn, sockindex, nonblocking);
  }

  /* Handle handshake pipelining. */
  if(!result)
    if(pipe_ssloverssl(conn, sockindex, SOS_READ | SOS_WRITE) < 0)
      result = CARLE_SSL_CONNECT_ERROR;

  /* Step 3: gather certificate info, verify host. */
  if(!result && connssl->connecting_state == ssl_connect_3)
    result = gskit_connect_step3(data, conn, sockindex);

  if(result)
    close_one(connssl, data, conn, sockindex);
  else if(connssl->connecting_state == ssl_connect_done) {
    connssl->state = ssl_connection_complete;
    connssl->connecting_state = ssl_connect_1;
    conn->recv[sockindex] = gskit_recv;
    conn->send[sockindex] = gskit_send;
    *done = TRUE;
  }

  return result;
}


static CARLcode gskit_connect_nonblocking(struct Curl_easy *data,
                                          struct connectdata *conn,
                                          int sockindex, bool *done)
{
  CARLcode result;

  result = gskit_connect_common(data, conn, sockindex, TRUE, done);
  if(*done || result)
    conn->ssl[sockindex].connecting_state = ssl_connect_1;
  return result;
}


static CARLcode gskit_connect(struct Curl_easy *data,
                              struct connectdata *conn, int sockindex)
{
  CARLcode result;
  bool done;

  conn->ssl[sockindex].connecting_state = ssl_connect_1;
  result = gskit_connect_common(data, conn, sockindex, FALSE, &done);
  if(result)
    return result;

  DEBUGASSERT(done);

  return CARLE_OK;
}


static void gskit_close(struct Curl_easy *data, struct connectdata *conn,
                        int sockindex)
{
  close_one(&conn->ssl[sockindex], data, conn, sockindex);
  close_one(&conn->proxy_ssl[sockindex], data, conn, sockindex);
}


static int gskit_shutdown(struct Curl_easy *data,
                          struct connectdata *conn, int sockindex)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  int what;
  int rc;
  char buf[120];

  if(!BACKEND->handle)
    return 0;

#ifndef CARL_DISABLE_FTP
  if(data->set.ftp_ccc != CARLFTPSSL_CCC_ACTIVE)
    return 0;
#endif

  close_one(connssl, data, conn, sockindex);
  rc = 0;
  what = SOCKET_READABLE(conn->sock[sockindex],
                         SSL_SHUTDOWN_TIMEOUT);

  for(;;) {
    ssize_t nread;

    if(what < 0) {
      /* anything that gets here is fatally bad */
      failf(data, "select/poll on SSL socket, errno: %d", SOCKERRNO);
      rc = -1;
      break;
    }

    if(!what) {                                /* timeout */
      failf(data, "SSL shutdown timeout");
      break;
    }

    /* Something to read, let's do it and hope that it is the close
       notify alert from the server. No way to gsk_secure_soc_read() now, so
       use read(). */

    nread = read(conn->sock[sockindex], buf, sizeof(buf));

    if(nread < 0) {
      failf(data, "read: %s", strerror(errno));
      rc = -1;
    }

    if(nread <= 0)
      break;

    what = SOCKET_READABLE(conn->sock[sockindex], 0);
  }

  return rc;
}


static size_t gskit_version(char *buffer, size_t size)
{
  return msnprintf(buffer, size, "GSKit");
}


static int gskit_check_cxn(struct connectdata *cxn)
{
  struct ssl_connect_data *connssl = &cxn->ssl[FIRSTSOCKET];
  int err;
  int errlen;

  /* The only thing that can be tested here is at the socket level. */

  if(!BACKEND->handle)
    return 0; /* connection has been closed */

  err = 0;
  errlen = sizeof(err);

  if(getsockopt(cxn->sock[FIRSTSOCKET], SOL_SOCKET, SO_ERROR,
                 (unsigned char *) &err, &errlen) ||
     errlen != sizeof(err) || err)
    return 0; /* connection has been closed */

  return -1;  /* connection status unknown */
}

static void *gskit_get_internals(struct ssl_connect_data *connssl,
                                 CARLINFO info UNUSED_PARAM)
{
  (void)info;
  return BACKEND->handle;
}

const struct Curl_ssl Curl_ssl_gskit = {
  { CARLSSLBACKEND_GSKIT, "gskit" }, /* info */

  SSLSUPP_CERTINFO |
  SSLSUPP_PINNEDPUBKEY,

  sizeof(struct ssl_backend_data),

  gskit_init,                     /* init */
  gskit_cleanup,                  /* cleanup */
  gskit_version,                  /* version */
  gskit_check_cxn,                /* check_cxn */
  gskit_shutdown,                 /* shutdown */
  Curl_none_data_pending,         /* data_pending */
  Curl_none_random,               /* random */
  Curl_none_cert_status_request,  /* cert_status_request */
  gskit_connect,                  /* connect */
  gskit_connect_nonblocking,      /* connect_nonblocking */
  gskit_get_internals,            /* get_internals */
  gskit_close,                    /* close_one */
  Curl_none_close_all,            /* close_all */
  /* No session handling for GSKit */
  Curl_none_session_free,         /* session_free */
  Curl_none_set_engine,           /* set_engine */
  Curl_none_set_engine_default,   /* set_engine_default */
  Curl_none_engines_list,         /* engines_list */
  Curl_none_false_start,          /* false_start */
  Curl_none_md5sum,               /* md5sum */
  NULL                            /* sha256sum */
};

#endif /* USE_GSKIT */
