#ifndef CARLINC_CARL_H
#define CARLINC_CARL_H
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

/*
 * If you have libcarl problems, all docs and details are found here:
 *   https://carl.se/libcarl/
 *
 * carl-library mailing list subscription and unsubscription web interface:
 *   https://cool.haxx.se/mailman/listinfo/carl-library/
 */

#ifdef CARL_NO_OLDIES
#define CARL_STRICTER
#endif

#include "carlver.h"         /* libcarl version defines   */
#include "system.h"          /* determine things run-time */

/*
 * Define CARL_WIN32 when build target is Win32 API
 */

#if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32)) &&        \
  !defined(__SYMBIAN32__)
#define CARL_WIN32
#endif

#include <stdio.h>
#include <limits.h>

#if defined(__FreeBSD__) && (__FreeBSD__ >= 2)
/* Needed for __FreeBSD_version symbol definition */
#include <osreldate.h>
#endif

/* The include stuff here below is mainly for time_t! */
#include <sys/types.h>
#include <time.h>

#if defined(CARL_WIN32) && !defined(_WIN32_WCE) && !defined(__CYGWIN__)
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H) || \
      defined(__LWIP_OPT_H__) || defined(LWIP_HDR_OPT_H))
/* The check above prevents the winsock2 inclusion if winsock.h already was
   included, since they can't co-exist without problems */
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#endif

/* HP-UX systems version 9, 10 and 11 lack sys/select.h and so does oldish
   libc5-based Linux systems. Only include it on systems that are known to
   require it! */
#if defined(_AIX) || defined(__NOVELL_LIBC__) || defined(__NetBSD__) || \
    defined(__minix) || defined(__SYMBIAN32__) || defined(__INTEGRITY) || \
    defined(ANDROID) || defined(__ANDROID__) || defined(__OpenBSD__) || \
    defined(__CYGWIN__) || defined(AMIGA) || \
   (defined(__FreeBSD_version) && (__FreeBSD_version < 800000))
#include <sys/select.h>
#endif

#if !defined(CARL_WIN32) && !defined(_WIN32_WCE)
#include <sys/socket.h>
#endif

#if !defined(CARL_WIN32) && !defined(__WATCOMC__) && !defined(__VXWORKS__)
#include <sys/time.h>
#endif

#ifdef __BEOS__
#include <support/SupportDefs.h>
#endif

/* Compatibility for non-Clang compilers */
#ifndef __has_declspec_attribute
#  define __has_declspec_attribute(x) 0
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(BUILDING_LIBCARL) || defined(CARL_STRICTER)
typedef struct Curl_easy CARL;
typedef struct Curl_share CARLSH;
#else
typedef void CARL;
typedef void CARLSH;
#endif

/*
 * libcarl external API function linkage decorations.
 */

#ifdef CARL_STATICLIB
#  define CARL_EXTERN
#elif defined(CARL_WIN32) || defined(__SYMBIAN32__) || \
     (__has_declspec_attribute(dllexport) && \
      __has_declspec_attribute(dllimport))
#  if defined(BUILDING_LIBCARL)
#    define CARL_EXTERN  __declspec(dllexport)
#  else
#    define CARL_EXTERN  __declspec(dllimport)
#  endif
#elif defined(BUILDING_LIBCARL) && defined(CARL_HIDDEN_SYMBOLS)
#  define CARL_EXTERN CARL_EXTERN_SYMBOL
#else
#  define CARL_EXTERN
#endif

#ifndef carl_socket_typedef
/* socket typedef */
#if defined(CARL_WIN32) && !defined(__LWIP_OPT_H__) && !defined(LWIP_HDR_OPT_H)
typedef SOCKET carl_socket_t;
#define CARL_SOCKET_BAD INVALID_SOCKET
#else
typedef int carl_socket_t;
#define CARL_SOCKET_BAD -1
#endif
#define carl_socket_typedef
#endif /* carl_socket_typedef */

/* enum for the different supported SSL backends */
typedef enum {
  CARLSSLBACKEND_NONE = 0,
  CARLSSLBACKEND_OPENSSL = 1,
  CARLSSLBACKEND_GNUTLS = 2,
  CARLSSLBACKEND_NSS = 3,
  CARLSSLBACKEND_OBSOLETE4 = 4,  /* Was QSOSSL. */
  CARLSSLBACKEND_GSKIT = 5,
  CARLSSLBACKEND_POLARSSL = 6,
  CARLSSLBACKEND_WOLFSSL = 7,
  CARLSSLBACKEND_SCHANNEL = 8,
  CARLSSLBACKEND_SECURETRANSPORT = 9,
  CARLSSLBACKEND_AXTLS = 10, /* never used since 7.63.0 */
  CARLSSLBACKEND_MBEDTLS = 11,
  CARLSSLBACKEND_MESALINK = 12,
  CARLSSLBACKEND_BEARSSL = 13
} carl_sslbackend;

/* aliases for library clones and renames */
#define CARLSSLBACKEND_LIBRESSL CARLSSLBACKEND_OPENSSL
#define CARLSSLBACKEND_BORINGSSL CARLSSLBACKEND_OPENSSL

/* deprecated names: */
#define CARLSSLBACKEND_CYASSL CARLSSLBACKEND_WOLFSSL
#define CARLSSLBACKEND_DARWINSSL CARLSSLBACKEND_SECURETRANSPORT

struct carl_httppost {
  struct carl_httppost *next;       /* next entry in the list */
  char *name;                       /* pointer to allocated name */
  long namelength;                  /* length of name length */
  char *contents;                   /* pointer to allocated data contents */
  long contentslength;              /* length of contents field, see also
                                       CARL_HTTPPOST_LARGE */
  char *buffer;                     /* pointer to allocated buffer contents */
  long bufferlength;                /* length of buffer field */
  char *contenttype;                /* Content-Type */
  struct carl_slist *contentheader; /* list of extra headers for this form */
  struct carl_httppost *more;       /* if one field name has more than one
                                       file, this link should link to following
                                       files */
  long flags;                       /* as defined below */

/* specified content is a file name */
#define CARL_HTTPPOST_FILENAME (1<<0)
/* specified content is a file name */
#define CARL_HTTPPOST_READFILE (1<<1)
/* name is only stored pointer do not free in formfree */
#define CARL_HTTPPOST_PTRNAME (1<<2)
/* contents is only stored pointer do not free in formfree */
#define CARL_HTTPPOST_PTRCONTENTS (1<<3)
/* upload file from buffer */
#define CARL_HTTPPOST_BUFFER (1<<4)
/* upload file from pointer contents */
#define CARL_HTTPPOST_PTRBUFFER (1<<5)
/* upload file contents by using the regular read callback to get the data and
   pass the given pointer as custom pointer */
#define CARL_HTTPPOST_CALLBACK (1<<6)
/* use size in 'contentlen', added in 7.46.0 */
#define CARL_HTTPPOST_LARGE (1<<7)

  char *showfilename;               /* The file name to show. If not set, the
                                       actual file name will be used (if this
                                       is a file part) */
  void *userp;                      /* custom pointer used for
                                       HTTPPOST_CALLBACK posts */
  carl_off_t contentlen;            /* alternative length of contents
                                       field. Used if CARL_HTTPPOST_LARGE is
                                       set. Added in 7.46.0 */
};


/* This is a return code for the progress callback that, when returned, will
   signal libcarl to continue executing the default progress function */
#define CARL_PROGRESSFUNC_CONTINUE 0x10000001

/* This is the CARLOPT_PROGRESSFUNCTION callback prototype. It is now
   considered deprecated but was the only choice up until 7.31.0 */
typedef int (*carl_progress_callback)(void *clientp,
                                      double dltotal,
                                      double dlnow,
                                      double ultotal,
                                      double ulnow);

/* This is the CARLOPT_XFERINFOFUNCTION callback prototype. It was introduced
   in 7.32.0, avoids the use of floating point numbers and provides more
   detailed information. */
typedef int (*carl_xferinfo_callback)(void *clientp,
                                      carl_off_t dltotal,
                                      carl_off_t dlnow,
                                      carl_off_t ultotal,
                                      carl_off_t ulnow);

#ifndef CARL_MAX_READ_SIZE
  /* The maximum receive buffer size configurable via CARLOPT_BUFFERSIZE. */
#define CARL_MAX_READ_SIZE 524288
#endif

#ifndef CARL_MAX_WRITE_SIZE
  /* Tests have proven that 20K is a very bad buffer size for uploads on
     Windows, while 16K for some odd reason performed a lot better.
     We do the ifndef check to allow this value to easier be changed at build
     time for those who feel adventurous. The practical minimum is about
     400 bytes since libcarl uses a buffer of this size as a scratch area
     (unrelated to network send operations). */
#define CARL_MAX_WRITE_SIZE 16384
#endif

#ifndef CARL_MAX_HTTP_HEADER
/* The only reason to have a max limit for this is to avoid the risk of a bad
   server feeding libcarl with a never-ending header that will cause reallocs
   infinitely */
#define CARL_MAX_HTTP_HEADER (100*1024)
#endif

/* This is a magic return code for the write callback that, when returned,
   will signal libcarl to pause receiving on the current transfer. */
#define CARL_WRITEFUNC_PAUSE 0x10000001

typedef size_t (*carl_write_callback)(char *buffer,
                                      size_t size,
                                      size_t nitems,
                                      void *outstream);

/* This callback will be called when a new resolver request is made */
typedef int (*carl_resolver_start_callback)(void *resolver_state,
                                            void *reserved, void *userdata);

/* enumeration of file types */
typedef enum {
  CARLFILETYPE_FILE = 0,
  CARLFILETYPE_DIRECTORY,
  CARLFILETYPE_SYMLINK,
  CARLFILETYPE_DEVICE_BLOCK,
  CARLFILETYPE_DEVICE_CHAR,
  CARLFILETYPE_NAMEDPIPE,
  CARLFILETYPE_SOCKET,
  CARLFILETYPE_DOOR, /* is possible only on Sun Solaris now */

  CARLFILETYPE_UNKNOWN /* should never occur */
} carlfiletype;

#define CARLFINFOFLAG_KNOWN_FILENAME    (1<<0)
#define CARLFINFOFLAG_KNOWN_FILETYPE    (1<<1)
#define CARLFINFOFLAG_KNOWN_TIME        (1<<2)
#define CARLFINFOFLAG_KNOWN_PERM        (1<<3)
#define CARLFINFOFLAG_KNOWN_UID         (1<<4)
#define CARLFINFOFLAG_KNOWN_GID         (1<<5)
#define CARLFINFOFLAG_KNOWN_SIZE        (1<<6)
#define CARLFINFOFLAG_KNOWN_HLINKCOUNT  (1<<7)

/* Information about a single file, used when doing FTP wildcard matching */
struct carl_fileinfo {
  char *filename;
  carlfiletype filetype;
  time_t time; /* always zero! */
  unsigned int perm;
  int uid;
  int gid;
  carl_off_t size;
  long int hardlinks;

  struct {
    /* If some of these fields is not NULL, it is a pointer to b_data. */
    char *time;
    char *perm;
    char *user;
    char *group;
    char *target; /* pointer to the target filename of a symlink */
  } strings;

  unsigned int flags;

  /* used internally */
  char *b_data;
  size_t b_size;
  size_t b_used;
};

/* return codes for CARLOPT_CHUNK_BGN_FUNCTION */
#define CARL_CHUNK_BGN_FUNC_OK      0
#define CARL_CHUNK_BGN_FUNC_FAIL    1 /* tell the lib to end the task */
#define CARL_CHUNK_BGN_FUNC_SKIP    2 /* skip this chunk over */

/* if splitting of data transfer is enabled, this callback is called before
   download of an individual chunk started. Note that parameter "remains" works
   only for FTP wildcard downloading (for now), otherwise is not used */
typedef long (*carl_chunk_bgn_callback)(const void *transfer_info,
                                        void *ptr,
                                        int remains);

/* return codes for CARLOPT_CHUNK_END_FUNCTION */
#define CARL_CHUNK_END_FUNC_OK      0
#define CARL_CHUNK_END_FUNC_FAIL    1 /* tell the lib to end the task */

/* If splitting of data transfer is enabled this callback is called after
   download of an individual chunk finished.
   Note! After this callback was set then it have to be called FOR ALL chunks.
   Even if downloading of this chunk was skipped in CHUNK_BGN_FUNC.
   This is the reason why we don't need "transfer_info" parameter in this
   callback and we are not interested in "remains" parameter too. */
typedef long (*carl_chunk_end_callback)(void *ptr);

/* return codes for FNMATCHFUNCTION */
#define CARL_FNMATCHFUNC_MATCH    0 /* string corresponds to the pattern */
#define CARL_FNMATCHFUNC_NOMATCH  1 /* pattern doesn't match the string */
#define CARL_FNMATCHFUNC_FAIL     2 /* an error occurred */

/* callback type for wildcard downloading pattern matching. If the
   string matches the pattern, return CARL_FNMATCHFUNC_MATCH value, etc. */
typedef int (*carl_fnmatch_callback)(void *ptr,
                                     const char *pattern,
                                     const char *string);

/* These are the return codes for the seek callbacks */
#define CARL_SEEKFUNC_OK       0
#define CARL_SEEKFUNC_FAIL     1 /* fail the entire transfer */
#define CARL_SEEKFUNC_CANTSEEK 2 /* tell libcarl seeking can't be done, so
                                    libcarl might try other means instead */
typedef int (*carl_seek_callback)(void *instream,
                                  carl_off_t offset,
                                  int origin); /* 'whence' */

/* This is a return code for the read callback that, when returned, will
   signal libcarl to immediately abort the current transfer. */
#define CARL_READFUNC_ABORT 0x10000000
/* This is a return code for the read callback that, when returned, will
   signal libcarl to pause sending data on the current transfer. */
#define CARL_READFUNC_PAUSE 0x10000001

/* Return code for when the trailing headers' callback has terminated
   without any errors*/
#define CARL_TRAILERFUNC_OK 0
/* Return code for when was an error in the trailing header's list and we
  want to abort the request */
#define CARL_TRAILERFUNC_ABORT 1

typedef size_t (*carl_read_callback)(char *buffer,
                                      size_t size,
                                      size_t nitems,
                                      void *instream);

typedef int (*carl_trailer_callback)(struct carl_slist **list,
                                      void *userdata);

typedef enum {
  CARLSOCKTYPE_IPCXN,  /* socket created for a specific IP connection */
  CARLSOCKTYPE_ACCEPT, /* socket created by accept() call */
  CARLSOCKTYPE_LAST    /* never use */
} carlsocktype;

/* The return code from the sockopt_callback can signal information back
   to libcarl: */
#define CARL_SOCKOPT_OK 0
#define CARL_SOCKOPT_ERROR 1 /* causes libcarl to abort and return
                                CARLE_ABORTED_BY_CALLBACK */
#define CARL_SOCKOPT_ALREADY_CONNECTED 2

typedef int (*carl_sockopt_callback)(void *clientp,
                                     carl_socket_t carlfd,
                                     carlsocktype purpose);

struct carl_sockaddr {
  int family;
  int socktype;
  int protocol;
  unsigned int addrlen; /* addrlen was a socklen_t type before 7.18.0 but it
                           turned really ugly and painful on the systems that
                           lack this type */
  struct sockaddr addr;
};

typedef carl_socket_t
(*carl_opensocket_callback)(void *clientp,
                            carlsocktype purpose,
                            struct carl_sockaddr *address);

typedef int
(*carl_closesocket_callback)(void *clientp, carl_socket_t item);

typedef enum {
  CARLIOE_OK,            /* I/O operation successful */
  CARLIOE_UNKNOWNCMD,    /* command was unknown to callback */
  CARLIOE_FAILRESTART,   /* failed to restart the read */
  CARLIOE_LAST           /* never use */
} carlioerr;

typedef enum {
  CARLIOCMD_NOP,         /* no operation */
  CARLIOCMD_RESTARTREAD, /* restart the read stream from start */
  CARLIOCMD_LAST         /* never use */
} carliocmd;

typedef carlioerr (*carl_ioctl_callback)(CARL *handle,
                                         int cmd,
                                         void *clientp);

#ifndef CARL_DID_MEMORY_FUNC_TYPEDEFS
/*
 * The following typedef's are signatures of malloc, free, realloc, strdup and
 * calloc respectively.  Function pointers of these types can be passed to the
 * carl_global_init_mem() function to set user defined memory management
 * callback routines.
 */
typedef void *(*carl_malloc_callback)(size_t size);
typedef void (*carl_free_callback)(void *ptr);
typedef void *(*carl_realloc_callback)(void *ptr, size_t size);
typedef char *(*carl_strdup_callback)(const char *str);
typedef void *(*carl_calloc_callback)(size_t nmemb, size_t size);

#define CARL_DID_MEMORY_FUNC_TYPEDEFS
#endif

/* the kind of data that is passed to information_callback*/
typedef enum {
  CARLINFO_TEXT = 0,
  CARLINFO_HEADER_IN,    /* 1 */
  CARLINFO_HEADER_OUT,   /* 2 */
  CARLINFO_DATA_IN,      /* 3 */
  CARLINFO_DATA_OUT,     /* 4 */
  CARLINFO_SSL_DATA_IN,  /* 5 */
  CARLINFO_SSL_DATA_OUT, /* 6 */
  CARLINFO_END
} carl_infotype;

typedef int (*carl_debug_callback)
       (CARL *handle,      /* the handle/transfer this concerns */
        carl_infotype type, /* what kind of data */
        char *data,        /* points to the data */
        size_t size,       /* size of the data pointed to */
        void *userptr);    /* whatever the user please */

/* All possible error codes from all sorts of carl functions. Future versions
   may return other values, stay prepared.

   Always add new return codes last. Never *EVER* remove any. The return
   codes must remain the same!
 */

typedef enum {
  CARLE_OK = 0,
  CARLE_UNSUPPORTED_PROTOCOL,    /* 1 */
  CARLE_FAILED_INIT,             /* 2 */
  CARLE_URL_MALFORMAT,           /* 3 */
  CARLE_NOT_BUILT_IN,            /* 4 - [was obsoleted in August 2007 for
                                    7.17.0, reused in April 2011 for 7.21.5] */
  CARLE_COULDNT_RESOLVE_PROXY,   /* 5 */
  CARLE_COULDNT_RESOLVE_HOST,    /* 6 */
  CARLE_COULDNT_CONNECT,         /* 7 */
  CARLE_WEIRD_SERVER_REPLY,      /* 8 */
  CARLE_REMOTE_ACCESS_DENIED,    /* 9 a service was denied by the server
                                    due to lack of access - when login fails
                                    this is not returned. */
  CARLE_FTP_ACCEPT_FAILED,       /* 10 - [was obsoleted in April 2006 for
                                    7.15.4, reused in Dec 2011 for 7.24.0]*/
  CARLE_FTP_WEIRD_PASS_REPLY,    /* 11 */
  CARLE_FTP_ACCEPT_TIMEOUT,      /* 12 - timeout occurred accepting server
                                    [was obsoleted in August 2007 for 7.17.0,
                                    reused in Dec 2011 for 7.24.0]*/
  CARLE_FTP_WEIRD_PASV_REPLY,    /* 13 */
  CARLE_FTP_WEIRD_227_FORMAT,    /* 14 */
  CARLE_FTP_CANT_GET_HOST,       /* 15 */
  CARLE_HTTP2,                   /* 16 - A problem in the http2 framing layer.
                                    [was obsoleted in August 2007 for 7.17.0,
                                    reused in July 2014 for 7.38.0] */
  CARLE_FTP_COULDNT_SET_TYPE,    /* 17 */
  CARLE_PARTIAL_FILE,            /* 18 */
  CARLE_FTP_COULDNT_RETR_FILE,   /* 19 */
  CARLE_OBSOLETE20,              /* 20 - NOT USED */
  CARLE_QUOTE_ERROR,             /* 21 - quote command failure */
  CARLE_HTTP_RETURNED_ERROR,     /* 22 */
  CARLE_WRITE_ERROR,             /* 23 */
  CARLE_OBSOLETE24,              /* 24 - NOT USED */
  CARLE_UPLOAD_FAILED,           /* 25 - failed upload "command" */
  CARLE_READ_ERROR,              /* 26 - couldn't open/read from file */
  CARLE_OUT_OF_MEMORY,           /* 27 */
  /* Note: CARLE_OUT_OF_MEMORY may sometimes indicate a conversion error
           instead of a memory allocation error if CARL_DOES_CONVERSIONS
           is defined
  */
  CARLE_OPERATION_TIMEDOUT,      /* 28 - the timeout time was reached */
  CARLE_OBSOLETE29,              /* 29 - NOT USED */
  CARLE_FTP_PORT_FAILED,         /* 30 - FTP PORT operation failed */
  CARLE_FTP_COULDNT_USE_REST,    /* 31 - the REST command failed */
  CARLE_OBSOLETE32,              /* 32 - NOT USED */
  CARLE_RANGE_ERROR,             /* 33 - RANGE "command" didn't work */
  CARLE_HTTP_POST_ERROR,         /* 34 */
  CARLE_SSL_CONNECT_ERROR,       /* 35 - wrong when connecting with SSL */
  CARLE_BAD_DOWNLOAD_RESUME,     /* 36 - couldn't resume download */
  CARLE_FILE_COULDNT_READ_FILE,  /* 37 */
  CARLE_LDAP_CANNOT_BIND,        /* 38 */
  CARLE_LDAP_SEARCH_FAILED,      /* 39 */
  CARLE_OBSOLETE40,              /* 40 - NOT USED */
  CARLE_FUNCTION_NOT_FOUND,      /* 41 - NOT USED starting with 7.53.0 */
  CARLE_ABORTED_BY_CALLBACK,     /* 42 */
  CARLE_BAD_FUNCTION_ARGUMENT,   /* 43 */
  CARLE_OBSOLETE44,              /* 44 - NOT USED */
  CARLE_INTERFACE_FAILED,        /* 45 - CARLOPT_INTERFACE failed */
  CARLE_OBSOLETE46,              /* 46 - NOT USED */
  CARLE_TOO_MANY_REDIRECTS,      /* 47 - catch endless re-direct loops */
  CARLE_UNKNOWN_OPTION,          /* 48 - User specified an unknown option */
  CARLE_TELNET_OPTION_SYNTAX,    /* 49 - Malformed telnet option */
  CARLE_OBSOLETE50,              /* 50 - NOT USED */
  CARLE_OBSOLETE51,              /* 51 - NOT USED */
  CARLE_GOT_NOTHING,             /* 52 - when this is a specific error */
  CARLE_SSL_ENGINE_NOTFOUND,     /* 53 - SSL crypto engine not found */
  CARLE_SSL_ENGINE_SETFAILED,    /* 54 - can not set SSL crypto engine as
                                    default */
  CARLE_SEND_ERROR,              /* 55 - failed sending network data */
  CARLE_RECV_ERROR,              /* 56 - failure in receiving network data */
  CARLE_OBSOLETE57,              /* 57 - NOT IN USE */
  CARLE_SSL_CERTPROBLEM,         /* 58 - problem with the local certificate */
  CARLE_SSL_CIPHER,              /* 59 - couldn't use specified cipher */
  CARLE_PEER_FAILED_VERIFICATION, /* 60 - peer's certificate or fingerprint
                                     wasn't verified fine */
  CARLE_BAD_CONTENT_ENCODING,    /* 61 - Unrecognized/bad encoding */
  CARLE_LDAP_INVALID_URL,        /* 62 - Invalid LDAP URL */
  CARLE_FILESIZE_EXCEEDED,       /* 63 - Maximum file size exceeded */
  CARLE_USE_SSL_FAILED,          /* 64 - Requested FTP SSL level failed */
  CARLE_SEND_FAIL_REWIND,        /* 65 - Sending the data requires a rewind
                                    that failed */
  CARLE_SSL_ENGINE_INITFAILED,   /* 66 - failed to initialise ENGINE */
  CARLE_LOGIN_DENIED,            /* 67 - user, password or similar was not
                                    accepted and we failed to login */
  CARLE_TFTP_NOTFOUND,           /* 68 - file not found on server */
  CARLE_TFTP_PERM,               /* 69 - permission problem on server */
  CARLE_REMOTE_DISK_FULL,        /* 70 - out of disk space on server */
  CARLE_TFTP_ILLEGAL,            /* 71 - Illegal TFTP operation */
  CARLE_TFTP_UNKNOWNID,          /* 72 - Unknown transfer ID */
  CARLE_REMOTE_FILE_EXISTS,      /* 73 - File already exists */
  CARLE_TFTP_NOSUCHUSER,         /* 74 - No such user */
  CARLE_CONV_FAILED,             /* 75 - conversion failed */
  CARLE_CONV_REQD,               /* 76 - caller must register conversion
                                    callbacks using carl_easy_setopt options
                                    CARLOPT_CONV_FROM_NETWORK_FUNCTION,
                                    CARLOPT_CONV_TO_NETWORK_FUNCTION, and
                                    CARLOPT_CONV_FROM_UTF8_FUNCTION */
  CARLE_SSL_CACERT_BADFILE,      /* 77 - could not load CACERT file, missing
                                    or wrong format */
  CARLE_REMOTE_FILE_NOT_FOUND,   /* 78 - remote file not found */
  CARLE_SSH,                     /* 79 - error from the SSH layer, somewhat
                                    generic so the error message will be of
                                    interest when this has happened */

  CARLE_SSL_SHUTDOWN_FAILED,     /* 80 - Failed to shut down the SSL
                                    connection */
  CARLE_AGAIN,                   /* 81 - socket is not ready for send/recv,
                                    wait till it's ready and try again (Added
                                    in 7.18.2) */
  CARLE_SSL_CRL_BADFILE,         /* 82 - could not load CRL file, missing or
                                    wrong format (Added in 7.19.0) */
  CARLE_SSL_ISSUER_ERROR,        /* 83 - Issuer check failed.  (Added in
                                    7.19.0) */
  CARLE_FTP_PRET_FAILED,         /* 84 - a PRET command failed */
  CARLE_RTSP_CSEQ_ERROR,         /* 85 - mismatch of RTSP CSeq numbers */
  CARLE_RTSP_SESSION_ERROR,      /* 86 - mismatch of RTSP Session Ids */
  CARLE_FTP_BAD_FILE_LIST,       /* 87 - unable to parse FTP file list */
  CARLE_CHUNK_FAILED,            /* 88 - chunk callback reported error */
  CARLE_NO_CONNECTION_AVAILABLE, /* 89 - No connection available, the
                                    session will be queued */
  CARLE_SSL_PINNEDPUBKEYNOTMATCH, /* 90 - specified pinned public key did not
                                     match */
  CARLE_SSL_INVALIDCERTSTATUS,   /* 91 - invalid certificate status */
  CARLE_HTTP2_STREAM,            /* 92 - stream error in HTTP/2 framing layer
                                    */
  CARLE_RECURSIVE_API_CALL,      /* 93 - an api function was called from
                                    inside a callback */
  CARLE_AUTH_ERROR,              /* 94 - an authentication function returned an
                                    error */
  CARLE_HTTP3,                   /* 95 - An HTTP/3 layer problem */
  CARLE_QUIC_CONNECT_ERROR,      /* 96 - QUIC connection error */
  CARLE_PROXY,                   /* 97 - proxy handshake error */
  CARL_LAST /* never use! */
} CARLcode;

#ifndef CARL_NO_OLDIES /* define this to test if your app builds with all
                          the obsolete stuff removed! */

/* Previously obsolete error code re-used in 7.38.0 */
#define CARLE_OBSOLETE16 CARLE_HTTP2

/* Previously obsolete error codes re-used in 7.24.0 */
#define CARLE_OBSOLETE10 CARLE_FTP_ACCEPT_FAILED
#define CARLE_OBSOLETE12 CARLE_FTP_ACCEPT_TIMEOUT

/*  compatibility with older names */
#define CARLOPT_ENCODING CARLOPT_ACCEPT_ENCODING
#define CARLE_FTP_WEIRD_SERVER_REPLY CARLE_WEIRD_SERVER_REPLY

/* The following were added in 7.62.0 */
#define CARLE_SSL_CACERT CARLE_PEER_FAILED_VERIFICATION

/* The following were added in 7.21.5, April 2011 */
#define CARLE_UNKNOWN_TELNET_OPTION CARLE_UNKNOWN_OPTION

/* The following were added in 7.17.1 */
/* These are scheduled to disappear by 2009 */
#define CARLE_SSL_PEER_CERTIFICATE CARLE_PEER_FAILED_VERIFICATION

/* The following were added in 7.17.0 */
/* These are scheduled to disappear by 2009 */
#define CARLE_OBSOLETE CARLE_OBSOLETE50 /* no one should be using this! */
#define CARLE_BAD_PASSWORD_ENTERED CARLE_OBSOLETE46
#define CARLE_BAD_CALLING_ORDER CARLE_OBSOLETE44
#define CARLE_FTP_USER_PASSWORD_INCORRECT CARLE_OBSOLETE10
#define CARLE_FTP_CANT_RECONNECT CARLE_OBSOLETE16
#define CARLE_FTP_COULDNT_GET_SIZE CARLE_OBSOLETE32
#define CARLE_FTP_COULDNT_SET_ASCII CARLE_OBSOLETE29
#define CARLE_FTP_WEIRD_USER_REPLY CARLE_OBSOLETE12
#define CARLE_FTP_WRITE_ERROR CARLE_OBSOLETE20
#define CARLE_LIBRARY_NOT_FOUND CARLE_OBSOLETE40
#define CARLE_MALFORMAT_USER CARLE_OBSOLETE24
#define CARLE_SHARE_IN_USE CARLE_OBSOLETE57
#define CARLE_URL_MALFORMAT_USER CARLE_NOT_BUILT_IN

#define CARLE_FTP_ACCESS_DENIED CARLE_REMOTE_ACCESS_DENIED
#define CARLE_FTP_COULDNT_SET_BINARY CARLE_FTP_COULDNT_SET_TYPE
#define CARLE_FTP_QUOTE_ERROR CARLE_QUOTE_ERROR
#define CARLE_TFTP_DISKFULL CARLE_REMOTE_DISK_FULL
#define CARLE_TFTP_EXISTS CARLE_REMOTE_FILE_EXISTS
#define CARLE_HTTP_RANGE_ERROR CARLE_RANGE_ERROR
#define CARLE_FTP_SSL_FAILED CARLE_USE_SSL_FAILED

/* The following were added earlier */

#define CARLE_OPERATION_TIMEOUTED CARLE_OPERATION_TIMEDOUT

#define CARLE_HTTP_NOT_FOUND CARLE_HTTP_RETURNED_ERROR
#define CARLE_HTTP_PORT_FAILED CARLE_INTERFACE_FAILED
#define CARLE_FTP_COULDNT_STOR_FILE CARLE_UPLOAD_FAILED

#define CARLE_FTP_PARTIAL_FILE CARLE_PARTIAL_FILE
#define CARLE_FTP_BAD_DOWNLOAD_RESUME CARLE_BAD_DOWNLOAD_RESUME

/* This was the error code 50 in 7.7.3 and a few earlier versions, this
   is no longer used by libcarl but is instead #defined here only to not
   make programs break */
#define CARLE_ALREADY_COMPLETE 99999

/* Provide defines for really old option names */
#define CARLOPT_FILE CARLOPT_WRITEDATA /* name changed in 7.9.7 */
#define CARLOPT_INFILE CARLOPT_READDATA /* name changed in 7.9.7 */
#define CARLOPT_WRITEHEADER CARLOPT_HEADERDATA

/* Since long deprecated options with no code in the lib that does anything
   with them. */
#define CARLOPT_WRITEINFO CARLOPT_OBSOLETE40
#define CARLOPT_CLOSEPOLICY CARLOPT_OBSOLETE72

#endif /*!CARL_NO_OLDIES*/

/*
 * Proxy error codes. Returned in CARLINFO_PROXY_ERROR if CARLE_PROXY was
 * return for the transfers.
 */
typedef enum {
  CARLPX_OK,
  CARLPX_BAD_ADDRESS_TYPE,
  CARLPX_BAD_VERSION,
  CARLPX_CLOSED,
  CARLPX_GSSAPI,
  CARLPX_GSSAPI_PERMSG,
  CARLPX_GSSAPI_PROTECTION,
  CARLPX_IDENTD,
  CARLPX_IDENTD_DIFFER,
  CARLPX_LONG_HOSTNAME,
  CARLPX_LONG_PASSWD,
  CARLPX_LONG_USER,
  CARLPX_NO_AUTH,
  CARLPX_RECV_ADDRESS,
  CARLPX_RECV_AUTH,
  CARLPX_RECV_CONNECT,
  CARLPX_RECV_REQACK,
  CARLPX_REPLY_ADDRESS_TYPE_NOT_SUPPORTED,
  CARLPX_REPLY_COMMAND_NOT_SUPPORTED,
  CARLPX_REPLY_CONNECTION_REFUSED,
  CARLPX_REPLY_GENERAL_SERVER_FAILURE,
  CARLPX_REPLY_HOST_UNREACHABLE,
  CARLPX_REPLY_NETWORK_UNREACHABLE,
  CARLPX_REPLY_NOT_ALLOWED,
  CARLPX_REPLY_TTL_EXPIRED,
  CARLPX_REPLY_UNASSIGNED,
  CARLPX_REQUEST_FAILED,
  CARLPX_RESOLVE_HOST,
  CARLPX_SEND_AUTH,
  CARLPX_SEND_CONNECT,
  CARLPX_SEND_REQUEST,
  CARLPX_UNKNOWN_FAIL,
  CARLPX_UNKNOWN_MODE,
  CARLPX_USER_REJECTED,
  CARLPX_LAST /* never use */
} CARLproxycode;

/* This prototype applies to all conversion callbacks */
typedef CARLcode (*carl_conv_callback)(char *buffer, size_t length);

typedef CARLcode (*carl_ssl_ctx_callback)(CARL *carl,    /* easy handle */
                                          void *ssl_ctx, /* actually an OpenSSL
                                                            or WolfSSL SSL_CTX,
                                                            or an mbedTLS
                                                          mbedtls_ssl_config */
                                          void *userptr);

typedef enum {
  CARLPROXY_HTTP = 0,   /* added in 7.10, new in 7.19.4 default is to use
                           CONNECT HTTP/1.1 */
  CARLPROXY_HTTP_1_0 = 1,   /* added in 7.19.4, force to use CONNECT
                               HTTP/1.0  */
  CARLPROXY_HTTPS = 2, /* added in 7.52.0 */
  CARLPROXY_SOCKS4 = 4, /* support added in 7.15.2, enum existed already
                           in 7.10 */
  CARLPROXY_SOCKS5 = 5, /* added in 7.10 */
  CARLPROXY_SOCKS4A = 6, /* added in 7.18.0 */
  CARLPROXY_SOCKS5_HOSTNAME = 7 /* Use the SOCKS5 protocol but pass along the
                                   host name rather than the IP address. added
                                   in 7.18.0 */
} carl_proxytype;  /* this enum was added in 7.10 */

/*
 * Bitmasks for CARLOPT_HTTPAUTH and CARLOPT_PROXYAUTH options:
 *
 * CARLAUTH_NONE         - No HTTP authentication
 * CARLAUTH_BASIC        - HTTP Basic authentication (default)
 * CARLAUTH_DIGEST       - HTTP Digest authentication
 * CARLAUTH_NEGOTIATE    - HTTP Negotiate (SPNEGO) authentication
 * CARLAUTH_GSSNEGOTIATE - Alias for CARLAUTH_NEGOTIATE (deprecated)
 * CARLAUTH_NTLM         - HTTP NTLM authentication
 * CARLAUTH_DIGEST_IE    - HTTP Digest authentication with IE flavour
 * CARLAUTH_NTLM_WB      - HTTP NTLM authentication delegated to winbind helper
 * CARLAUTH_BEARER       - HTTP Bearer token authentication
 * CARLAUTH_ONLY         - Use together with a single other type to force no
 *                         authentication or just that single type
 * CARLAUTH_ANY          - All fine types set
 * CARLAUTH_ANYSAFE      - All fine types except Basic
 */

#define CARLAUTH_NONE         ((unsigned long)0)
#define CARLAUTH_BASIC        (((unsigned long)1)<<0)
#define CARLAUTH_DIGEST       (((unsigned long)1)<<1)
#define CARLAUTH_NEGOTIATE    (((unsigned long)1)<<2)
/* Deprecated since the advent of CARLAUTH_NEGOTIATE */
#define CARLAUTH_GSSNEGOTIATE CARLAUTH_NEGOTIATE
/* Used for CARLOPT_SOCKS5_AUTH to stay terminologically correct */
#define CARLAUTH_GSSAPI CARLAUTH_NEGOTIATE
#define CARLAUTH_NTLM         (((unsigned long)1)<<3)
#define CARLAUTH_DIGEST_IE    (((unsigned long)1)<<4)
#define CARLAUTH_NTLM_WB      (((unsigned long)1)<<5)
#define CARLAUTH_BEARER       (((unsigned long)1)<<6)
#define CARLAUTH_AWS_SIGV4 (((unsigned long)1)<<7)
#define CARLAUTH_ONLY         (((unsigned long)1)<<31)
#define CARLAUTH_ANY          (~CARLAUTH_DIGEST_IE)
#define CARLAUTH_ANYSAFE      (~(CARLAUTH_BASIC|CARLAUTH_DIGEST_IE))

#define CARLSSH_AUTH_ANY       ~0     /* all types supported by the server */
#define CARLSSH_AUTH_NONE      0      /* none allowed, silly but complete */
#define CARLSSH_AUTH_PUBLICKEY (1<<0) /* public/private key files */
#define CARLSSH_AUTH_PASSWORD  (1<<1) /* password */
#define CARLSSH_AUTH_HOST      (1<<2) /* host key files */
#define CARLSSH_AUTH_KEYBOARD  (1<<3) /* keyboard interactive */
#define CARLSSH_AUTH_AGENT     (1<<4) /* agent (ssh-agent, pageant...) */
#define CARLSSH_AUTH_GSSAPI    (1<<5) /* gssapi (kerberos, ...) */
#define CARLSSH_AUTH_DEFAULT CARLSSH_AUTH_ANY

#define CARLGSSAPI_DELEGATION_NONE        0      /* no delegation (default) */
#define CARLGSSAPI_DELEGATION_POLICY_FLAG (1<<0) /* if permitted by policy */
#define CARLGSSAPI_DELEGATION_FLAG        (1<<1) /* delegate always */

#define CARL_ERROR_SIZE 256

enum carl_khtype {
  CARLKHTYPE_UNKNOWN,
  CARLKHTYPE_RSA1,
  CARLKHTYPE_RSA,
  CARLKHTYPE_DSS,
  CARLKHTYPE_ECDSA,
  CARLKHTYPE_ED25519
};

struct carl_khkey {
  const char *key; /* points to a null-terminated string encoded with base64
                      if len is zero, otherwise to the "raw" data */
  size_t len;
  enum carl_khtype keytype;
};

/* this is the set of return values expected from the carl_sshkeycallback
   callback */
enum carl_khstat {
  CARLKHSTAT_FINE_ADD_TO_FILE,
  CARLKHSTAT_FINE,
  CARLKHSTAT_REJECT, /* reject the connection, return an error */
  CARLKHSTAT_DEFER,  /* do not accept it, but we can't answer right now so
                        this causes a CARLE_DEFER error but otherwise the
                        connection will be left intact etc */
  CARLKHSTAT_FINE_REPLACE, /* accept and replace the wrong key*/
  CARLKHSTAT_LAST    /* not for use, only a marker for last-in-list */
};

/* this is the set of status codes pass in to the callback */
enum carl_khmatch {
  CARLKHMATCH_OK,       /* match */
  CARLKHMATCH_MISMATCH, /* host found, key mismatch! */
  CARLKHMATCH_MISSING,  /* no matching host/key found */
  CARLKHMATCH_LAST      /* not for use, only a marker for last-in-list */
};

typedef int
  (*carl_sshkeycallback) (CARL *easy,     /* easy handle */
                          const struct carl_khkey *knownkey, /* known */
                          const struct carl_khkey *foundkey, /* found */
                          enum carl_khmatch, /* libcarl's view on the keys */
                          void *clientp); /* custom pointer passed from app */

/* parameter for the CARLOPT_USE_SSL option */
typedef enum {
  CARLUSESSL_NONE,    /* do not attempt to use SSL */
  CARLUSESSL_TRY,     /* try using SSL, proceed anyway otherwise */
  CARLUSESSL_CONTROL, /* SSL for the control connection or fail */
  CARLUSESSL_ALL,     /* SSL for all communication or fail */
  CARLUSESSL_LAST     /* not an option, never use */
} carl_usessl;

/* Definition of bits for the CARLOPT_SSL_OPTIONS argument: */

/* - ALLOW_BEAST tells libcarl to allow the BEAST SSL vulnerability in the
   name of improving interoperability with older servers. Some SSL libraries
   have introduced work-arounds for this flaw but those work-arounds sometimes
   make the SSL communication fail. To regain functionality with those broken
   servers, a user can this way allow the vulnerability back. */
#define CARLSSLOPT_ALLOW_BEAST (1<<0)

/* - NO_REVOKE tells libcarl to disable certificate revocation checks for those
   SSL backends where such behavior is present. */
#define CARLSSLOPT_NO_REVOKE (1<<1)

/* - NO_PARTIALCHAIN tells libcarl to *NOT* accept a partial certificate chain
   if possible. The OpenSSL backend has this ability. */
#define CARLSSLOPT_NO_PARTIALCHAIN (1<<2)

/* - REVOKE_BEST_EFFORT tells libcarl to ignore certificate revocation offline
   checks and ignore missing revocation list for those SSL backends where such
   behavior is present. */
#define CARLSSLOPT_REVOKE_BEST_EFFORT (1<<3)

/* - CARLSSLOPT_NATIVE_CA tells libcarl to use standard certificate store of
   operating system. Currently implemented under MS-Windows. */
#define CARLSSLOPT_NATIVE_CA (1<<4)

/* The default connection attempt delay in milliseconds for happy eyeballs.
   CARLOPT_HAPPY_EYEBALLS_TIMEOUT_MS.3 and happy-eyeballs-timeout-ms.d document
   this value, keep them in sync. */
#define CARL_HET_DEFAULT 200L

/* The default connection upkeep interval in milliseconds. */
#define CARL_UPKEEP_INTERVAL_DEFAULT 60000L

#ifndef CARL_NO_OLDIES /* define this to test if your app builds with all
                          the obsolete stuff removed! */

/* Backwards compatibility with older names */
/* These are scheduled to disappear by 2009 */

#define CARLFTPSSL_NONE CARLUSESSL_NONE
#define CARLFTPSSL_TRY CARLUSESSL_TRY
#define CARLFTPSSL_CONTROL CARLUSESSL_CONTROL
#define CARLFTPSSL_ALL CARLUSESSL_ALL
#define CARLFTPSSL_LAST CARLUSESSL_LAST
#define carl_ftpssl carl_usessl
#endif /*!CARL_NO_OLDIES*/

/* parameter for the CARLOPT_FTP_SSL_CCC option */
typedef enum {
  CARLFTPSSL_CCC_NONE,    /* do not send CCC */
  CARLFTPSSL_CCC_PASSIVE, /* Let the server initiate the shutdown */
  CARLFTPSSL_CCC_ACTIVE,  /* Initiate the shutdown */
  CARLFTPSSL_CCC_LAST     /* not an option, never use */
} carl_ftpccc;

/* parameter for the CARLOPT_FTPSSLAUTH option */
typedef enum {
  CARLFTPAUTH_DEFAULT, /* let libcarl decide */
  CARLFTPAUTH_SSL,     /* use "AUTH SSL" */
  CARLFTPAUTH_TLS,     /* use "AUTH TLS" */
  CARLFTPAUTH_LAST /* not an option, never use */
} carl_ftpauth;

/* parameter for the CARLOPT_FTP_CREATE_MISSING_DIRS option */
typedef enum {
  CARLFTP_CREATE_DIR_NONE,  /* do NOT create missing dirs! */
  CARLFTP_CREATE_DIR,       /* (FTP/SFTP) if CWD fails, try MKD and then CWD
                               again if MKD succeeded, for SFTP this does
                               similar magic */
  CARLFTP_CREATE_DIR_RETRY, /* (FTP only) if CWD fails, try MKD and then CWD
                               again even if MKD failed! */
  CARLFTP_CREATE_DIR_LAST   /* not an option, never use */
} carl_ftpcreatedir;

/* parameter for the CARLOPT_FTP_FILEMETHOD option */
typedef enum {
  CARLFTPMETHOD_DEFAULT,   /* let libcarl pick */
  CARLFTPMETHOD_MULTICWD,  /* single CWD operation for each path part */
  CARLFTPMETHOD_NOCWD,     /* no CWD at all */
  CARLFTPMETHOD_SINGLECWD, /* one CWD to full dir, then work on file */
  CARLFTPMETHOD_LAST       /* not an option, never use */
} carl_ftpmethod;

/* bitmask defines for CARLOPT_HEADEROPT */
#define CARLHEADER_UNIFIED  0
#define CARLHEADER_SEPARATE (1<<0)

/* CARLALTSVC_* are bits for the CARLOPT_ALTSVC_CTRL option */
#define CARLALTSVC_READONLYFILE (1<<2)
#define CARLALTSVC_H1           (1<<3)
#define CARLALTSVC_H2           (1<<4)
#define CARLALTSVC_H3           (1<<5)


struct carl_hstsentry {
  char *name;
  size_t namelen;
  unsigned int includeSubDomains:1;
  char expire[18]; /* YYYYMMDD HH:MM:SS [null-terminated] */
};

struct carl_index {
  size_t index; /* the provided entry's "index" or count */
  size_t total; /* total number of entries to save */
};

typedef enum {
  CARLSTS_OK,
  CARLSTS_DONE,
  CARLSTS_FAIL
} CARLSTScode;

typedef CARLSTScode (*carl_hstsread_callback)(CARL *easy,
                                              struct carl_hstsentry *e,
                                              void *userp);
typedef CARLSTScode (*carl_hstswrite_callback)(CARL *easy,
                                               struct carl_hstsentry *e,
                                               struct carl_index *i,
                                               void *userp);

/* CARLHSTS_* are bits for the CARLOPT_HSTS option */
#define CARLHSTS_ENABLE       (long)(1<<0)
#define CARLHSTS_READONLYFILE (long)(1<<1)

/* CARLPROTO_ defines are for the CARLOPT_*PROTOCOLS options */
#define CARLPROTO_HTTP   (1<<0)
#define CARLPROTO_HTTPS  (1<<1)
#define CARLPROTO_FTP    (1<<2)
#define CARLPROTO_FTPS   (1<<3)
#define CARLPROTO_SCP    (1<<4)
#define CARLPROTO_SFTP   (1<<5)
#define CARLPROTO_TELNET (1<<6)
#define CARLPROTO_LDAP   (1<<7)
#define CARLPROTO_LDAPS  (1<<8)
#define CARLPROTO_DICT   (1<<9)
#define CARLPROTO_FILE   (1<<10)
#define CARLPROTO_TFTP   (1<<11)
#define CARLPROTO_IMAP   (1<<12)
#define CARLPROTO_IMAPS  (1<<13)
#define CARLPROTO_POP3   (1<<14)
#define CARLPROTO_POP3S  (1<<15)
#define CARLPROTO_SMTP   (1<<16)
#define CARLPROTO_SMTPS  (1<<17)
#define CARLPROTO_RTSP   (1<<18)
#define CARLPROTO_RTMP   (1<<19)
#define CARLPROTO_RTMPT  (1<<20)
#define CARLPROTO_RTMPE  (1<<21)
#define CARLPROTO_RTMPTE (1<<22)
#define CARLPROTO_RTMPS  (1<<23)
#define CARLPROTO_RTMPTS (1<<24)
#define CARLPROTO_GOPHER (1<<25)
#define CARLPROTO_SMB    (1<<26)
#define CARLPROTO_SMBS   (1<<27)
#define CARLPROTO_MQTT   (1<<28)
#define CARLPROTO_GOPHERS (1<<29)
#define CARLPROTO_ALL    (~0) /* enable everything */

/* long may be 32 or 64 bits, but we should never depend on anything else
   but 32 */
#define CARLOPTTYPE_LONG          0
#define CARLOPTTYPE_OBJECTPOINT   10000
#define CARLOPTTYPE_FUNCTIONPOINT 20000
#define CARLOPTTYPE_OFF_T         30000
#define CARLOPTTYPE_BLOB          40000

/* *STRINGPOINT is an alias for OBJECTPOINT to allow tools to extract the
   string options from the header file */


#define CARLOPT(na,t,nu) na = t + nu

/* CARLOPT aliases that make no run-time difference */

/* 'char *' argument to a string with a trailing zero */
#define CARLOPTTYPE_STRINGPOINT CARLOPTTYPE_OBJECTPOINT

/* 'struct carl_slist *' argument */
#define CARLOPTTYPE_SLISTPOINT  CARLOPTTYPE_OBJECTPOINT

/* 'void *' argument passed untouched to callback */
#define CARLOPTTYPE_CBPOINT     CARLOPTTYPE_OBJECTPOINT

/* 'long' argument with a set of values/bitmask */
#define CARLOPTTYPE_VALUES      CARLOPTTYPE_LONG

/*
 * All CARLOPT_* values.
 */

typedef enum {
  /* This is the FILE * or void * the regular output should be written to. */
  CARLOPT(CARLOPT_WRITEDATA, CARLOPTTYPE_CBPOINT, 1),

  /* The full URL to get/put */
  CARLOPT(CARLOPT_URL, CARLOPTTYPE_STRINGPOINT, 2),

  /* Port number to connect to, if other than default. */
  CARLOPT(CARLOPT_PORT, CARLOPTTYPE_LONG, 3),

  /* Name of proxy to use. */
  CARLOPT(CARLOPT_PROXY, CARLOPTTYPE_STRINGPOINT, 4),

  /* "user:password;options" to use when fetching. */
  CARLOPT(CARLOPT_USERPWD, CARLOPTTYPE_STRINGPOINT, 5),

  /* "user:password" to use with proxy. */
  CARLOPT(CARLOPT_PROXYUSERPWD, CARLOPTTYPE_STRINGPOINT, 6),

  /* Range to get, specified as an ASCII string. */
  CARLOPT(CARLOPT_RANGE, CARLOPTTYPE_STRINGPOINT, 7),

  /* not used */

  /* Specified file stream to upload from (use as input): */
  CARLOPT(CARLOPT_READDATA, CARLOPTTYPE_CBPOINT, 9),

  /* Buffer to receive error messages in, must be at least CARL_ERROR_SIZE
   * bytes big. */
  CARLOPT(CARLOPT_ERRORBUFFER, CARLOPTTYPE_OBJECTPOINT, 10),

  /* Function that will be called to store the output (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CARLOPT(CARLOPT_WRITEFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 11),

  /* Function that will be called to read the input (instead of fread). The
   * parameters will use fread() syntax, make sure to follow them. */
  CARLOPT(CARLOPT_READFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 12),

  /* Time-out the read operation after this amount of seconds */
  CARLOPT(CARLOPT_TIMEOUT, CARLOPTTYPE_LONG, 13),

  /* If the CARLOPT_INFILE is used, this can be used to inform libcarl about
   * how large the file being sent really is. That allows better error
   * checking and better verifies that the upload was successful. -1 means
   * unknown size.
   *
   * For large file support, there is also a _LARGE version of the key
   * which takes an off_t type, allowing platforms with larger off_t
   * sizes to handle larger files.  See below for INFILESIZE_LARGE.
   */
  CARLOPT(CARLOPT_INFILESIZE, CARLOPTTYPE_LONG, 14),

  /* POST static input fields. */
  CARLOPT(CARLOPT_POSTFIELDS, CARLOPTTYPE_OBJECTPOINT, 15),

  /* Set the referrer page (needed by some CGIs) */
  CARLOPT(CARLOPT_REFERER, CARLOPTTYPE_STRINGPOINT, 16),

  /* Set the FTP PORT string (interface name, named or numerical IP address)
     Use i.e '-' to use default address. */
  CARLOPT(CARLOPT_FTPPORT, CARLOPTTYPE_STRINGPOINT, 17),

  /* Set the User-Agent string (examined by some CGIs) */
  CARLOPT(CARLOPT_USERAGENT, CARLOPTTYPE_STRINGPOINT, 18),

  /* If the download receives less than "low speed limit" bytes/second
   * during "low speed time" seconds, the operations is aborted.
   * You could i.e if you have a pretty high speed connection, abort if
   * it is less than 2000 bytes/sec during 20 seconds.
   */

  /* Set the "low speed limit" */
  CARLOPT(CARLOPT_LOW_SPEED_LIMIT, CARLOPTTYPE_LONG, 19),

  /* Set the "low speed time" */
  CARLOPT(CARLOPT_LOW_SPEED_TIME, CARLOPTTYPE_LONG, 20),

  /* Set the continuation offset.
   *
   * Note there is also a _LARGE version of this key which uses
   * off_t types, allowing for large file offsets on platforms which
   * use larger-than-32-bit off_t's.  Look below for RESUME_FROM_LARGE.
   */
  CARLOPT(CARLOPT_RESUME_FROM, CARLOPTTYPE_LONG, 21),

  /* Set cookie in request: */
  CARLOPT(CARLOPT_COOKIE, CARLOPTTYPE_STRINGPOINT, 22),

  /* This points to a linked list of headers, struct carl_slist kind. This
     list is also used for RTSP (in spite of its name) */
  CARLOPT(CARLOPT_HTTPHEADER, CARLOPTTYPE_SLISTPOINT, 23),

  /* This points to a linked list of post entries, struct carl_httppost */
  CARLOPT(CARLOPT_HTTPPOST, CARLOPTTYPE_OBJECTPOINT, 24),

  /* name of the file keeping your private SSL-certificate */
  CARLOPT(CARLOPT_SSLCERT, CARLOPTTYPE_STRINGPOINT, 25),

  /* password for the SSL or SSH private key */
  CARLOPT(CARLOPT_KEYPASSWD, CARLOPTTYPE_STRINGPOINT, 26),

  /* send TYPE parameter? */
  CARLOPT(CARLOPT_CRLF, CARLOPTTYPE_LONG, 27),

  /* send linked-list of QUOTE commands */
  CARLOPT(CARLOPT_QUOTE, CARLOPTTYPE_SLISTPOINT, 28),

  /* send FILE * or void * to store headers to, if you use a callback it
     is simply passed to the callback unmodified */
  CARLOPT(CARLOPT_HEADERDATA, CARLOPTTYPE_CBPOINT, 29),

  /* point to a file to read the initial cookies from, also enables
     "cookie awareness" */
  CARLOPT(CARLOPT_COOKIEFILE, CARLOPTTYPE_STRINGPOINT, 31),

  /* What version to specifically try to use.
     See CARL_SSLVERSION defines below. */
  CARLOPT(CARLOPT_SSLVERSION, CARLOPTTYPE_VALUES, 32),

  /* What kind of HTTP time condition to use, see defines */
  CARLOPT(CARLOPT_TIMECONDITION, CARLOPTTYPE_VALUES, 33),

  /* Time to use with the above condition. Specified in number of seconds
     since 1 Jan 1970 */
  CARLOPT(CARLOPT_TIMEVALUE, CARLOPTTYPE_LONG, 34),

  /* 35 = OBSOLETE */

  /* Custom request, for customizing the get command like
     HTTP: DELETE, TRACE and others
     FTP: to use a different list command
     */
  CARLOPT(CARLOPT_CUSTOMREQUEST, CARLOPTTYPE_STRINGPOINT, 36),

  /* FILE handle to use instead of stderr */
  CARLOPT(CARLOPT_STDERR, CARLOPTTYPE_OBJECTPOINT, 37),

  /* 38 is not used */

  /* send linked-list of post-transfer QUOTE commands */
  CARLOPT(CARLOPT_POSTQUOTE, CARLOPTTYPE_SLISTPOINT, 39),

   /* OBSOLETE, do not use! */
  CARLOPT(CARLOPT_OBSOLETE40, CARLOPTTYPE_OBJECTPOINT, 40),

  /* talk a lot */
  CARLOPT(CARLOPT_VERBOSE, CARLOPTTYPE_LONG, 41),

  /* throw the header out too */
  CARLOPT(CARLOPT_HEADER, CARLOPTTYPE_LONG, 42),

  /* shut off the progress meter */
  CARLOPT(CARLOPT_NOPROGRESS, CARLOPTTYPE_LONG, 43),

  /* use HEAD to get http document */
  CARLOPT(CARLOPT_NOBODY, CARLOPTTYPE_LONG, 44),

  /* no output on http error codes >= 400 */
  CARLOPT(CARLOPT_FAILONERROR, CARLOPTTYPE_LONG, 45),

  /* this is an upload */
  CARLOPT(CARLOPT_UPLOAD, CARLOPTTYPE_LONG, 46),

  /* HTTP POST method */
  CARLOPT(CARLOPT_POST, CARLOPTTYPE_LONG, 47),

  /* bare names when listing directories */
  CARLOPT(CARLOPT_DIRLISTONLY, CARLOPTTYPE_LONG, 48),

  /* Append instead of overwrite on upload! */
  CARLOPT(CARLOPT_APPEND, CARLOPTTYPE_LONG, 50),

  /* Specify whether to read the user+password from the .netrc or the URL.
   * This must be one of the CARL_NETRC_* enums below. */
  CARLOPT(CARLOPT_NETRC, CARLOPTTYPE_VALUES, 51),

  /* use Location: Luke! */
  CARLOPT(CARLOPT_FOLLOWLOCATION, CARLOPTTYPE_LONG, 52),

   /* transfer data in text/ASCII format */
  CARLOPT(CARLOPT_TRANSFERTEXT, CARLOPTTYPE_LONG, 53),

  /* HTTP PUT */
  CARLOPT(CARLOPT_PUT, CARLOPTTYPE_LONG, 54),

  /* 55 = OBSOLETE */

  /* DEPRECATED
   * Function that will be called instead of the internal progress display
   * function. This function should be defined as the carl_progress_callback
   * prototype defines. */
  CARLOPT(CARLOPT_PROGRESSFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 56),

  /* Data passed to the CARLOPT_PROGRESSFUNCTION and CARLOPT_XFERINFOFUNCTION
     callbacks */
  CARLOPT(CARLOPT_XFERINFODATA, CARLOPTTYPE_CBPOINT, 57),
#define CARLOPT_PROGRESSDATA CARLOPT_XFERINFODATA

  /* We want the referrer field set automatically when following locations */
  CARLOPT(CARLOPT_AUTOREFERER, CARLOPTTYPE_LONG, 58),

  /* Port of the proxy, can be set in the proxy string as well with:
     "[host]:[port]" */
  CARLOPT(CARLOPT_PROXYPORT, CARLOPTTYPE_LONG, 59),

  /* size of the POST input data, if strlen() is not good to use */
  CARLOPT(CARLOPT_POSTFIELDSIZE, CARLOPTTYPE_LONG, 60),

  /* tunnel non-http operations through a HTTP proxy */
  CARLOPT(CARLOPT_HTTPPROXYTUNNEL, CARLOPTTYPE_LONG, 61),

  /* Set the interface string to use as outgoing network interface */
  CARLOPT(CARLOPT_INTERFACE, CARLOPTTYPE_STRINGPOINT, 62),

  /* Set the krb4/5 security level, this also enables krb4/5 awareness.  This
   * is a string, 'clear', 'safe', 'confidential' or 'private'.  If the string
   * is set but doesn't match one of these, 'private' will be used.  */
  CARLOPT(CARLOPT_KRBLEVEL, CARLOPTTYPE_STRINGPOINT, 63),

  /* Set if we should verify the peer in ssl handshake, set 1 to verify. */
  CARLOPT(CARLOPT_SSL_VERIFYPEER, CARLOPTTYPE_LONG, 64),

  /* The CApath or CAfile used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CARLOPT(CARLOPT_CAINFO, CARLOPTTYPE_STRINGPOINT, 65),

  /* 66 = OBSOLETE */
  /* 67 = OBSOLETE */

  /* Maximum number of http redirects to follow */
  CARLOPT(CARLOPT_MAXREDIRS, CARLOPTTYPE_LONG, 68),

  /* Pass a long set to 1 to get the date of the requested document (if
     possible)! Pass a zero to shut it off. */
  CARLOPT(CARLOPT_FILETIME, CARLOPTTYPE_LONG, 69),

  /* This points to a linked list of telnet options */
  CARLOPT(CARLOPT_TELNETOPTIONS, CARLOPTTYPE_SLISTPOINT, 70),

  /* Max amount of cached alive connections */
  CARLOPT(CARLOPT_MAXCONNECTS, CARLOPTTYPE_LONG, 71),

  /* OBSOLETE, do not use! */
  CARLOPT(CARLOPT_OBSOLETE72, CARLOPTTYPE_LONG, 72),

  /* 73 = OBSOLETE */

  /* Set to explicitly use a new connection for the upcoming transfer.
     Do not use this unless you're absolutely sure of this, as it makes the
     operation slower and is less friendly for the network. */
  CARLOPT(CARLOPT_FRESH_CONNECT, CARLOPTTYPE_LONG, 74),

  /* Set to explicitly forbid the upcoming transfer's connection to be re-used
     when done. Do not use this unless you're absolutely sure of this, as it
     makes the operation slower and is less friendly for the network. */
  CARLOPT(CARLOPT_FORBID_REUSE, CARLOPTTYPE_LONG, 75),

  /* Set to a file name that contains random data for libcarl to use to
     seed the random engine when doing SSL connects. */
  CARLOPT(CARLOPT_RANDOM_FILE, CARLOPTTYPE_STRINGPOINT, 76),

  /* Set to the Entropy Gathering Daemon socket pathname */
  CARLOPT(CARLOPT_EGDSOCKET, CARLOPTTYPE_STRINGPOINT, 77),

  /* Time-out connect operations after this amount of seconds, if connects are
     OK within this time, then fine... This only aborts the connect phase. */
  CARLOPT(CARLOPT_CONNECTTIMEOUT, CARLOPTTYPE_LONG, 78),

  /* Function that will be called to store headers (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CARLOPT(CARLOPT_HEADERFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 79),

  /* Set this to force the HTTP request to get back to GET. Only really usable
     if POST, PUT or a custom request have been used first.
   */
  CARLOPT(CARLOPT_HTTPGET, CARLOPTTYPE_LONG, 80),

  /* Set if we should verify the Common name from the peer certificate in ssl
   * handshake, set 1 to check existence, 2 to ensure that it matches the
   * provided hostname. */
  CARLOPT(CARLOPT_SSL_VERIFYHOST, CARLOPTTYPE_LONG, 81),

  /* Specify which file name to write all known cookies in after completed
     operation. Set file name to "-" (dash) to make it go to stdout. */
  CARLOPT(CARLOPT_COOKIEJAR, CARLOPTTYPE_STRINGPOINT, 82),

  /* Specify which SSL ciphers to use */
  CARLOPT(CARLOPT_SSL_CIPHER_LIST, CARLOPTTYPE_STRINGPOINT, 83),

  /* Specify which HTTP version to use! This must be set to one of the
     CARL_HTTP_VERSION* enums set below. */
  CARLOPT(CARLOPT_HTTP_VERSION, CARLOPTTYPE_VALUES, 84),

  /* Specifically switch on or off the FTP engine's use of the EPSV command. By
     default, that one will always be attempted before the more traditional
     PASV command. */
  CARLOPT(CARLOPT_FTP_USE_EPSV, CARLOPTTYPE_LONG, 85),

  /* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") */
  CARLOPT(CARLOPT_SSLCERTTYPE, CARLOPTTYPE_STRINGPOINT, 86),

  /* name of the file keeping your private SSL-key */
  CARLOPT(CARLOPT_SSLKEY, CARLOPTTYPE_STRINGPOINT, 87),

  /* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") */
  CARLOPT(CARLOPT_SSLKEYTYPE, CARLOPTTYPE_STRINGPOINT, 88),

  /* crypto engine for the SSL-sub system */
  CARLOPT(CARLOPT_SSLENGINE, CARLOPTTYPE_STRINGPOINT, 89),

  /* set the crypto engine for the SSL-sub system as default
     the param has no meaning...
   */
  CARLOPT(CARLOPT_SSLENGINE_DEFAULT, CARLOPTTYPE_LONG, 90),

  /* Non-zero value means to use the global dns cache */
  /* DEPRECATED, do not use! */
  CARLOPT(CARLOPT_DNS_USE_GLOBAL_CACHE, CARLOPTTYPE_LONG, 91),

  /* DNS cache timeout */
  CARLOPT(CARLOPT_DNS_CACHE_TIMEOUT, CARLOPTTYPE_LONG, 92),

  /* send linked-list of pre-transfer QUOTE commands */
  CARLOPT(CARLOPT_PREQUOTE, CARLOPTTYPE_SLISTPOINT, 93),

  /* set the debug function */
  CARLOPT(CARLOPT_DEBUGFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 94),

  /* set the data for the debug function */
  CARLOPT(CARLOPT_DEBUGDATA, CARLOPTTYPE_CBPOINT, 95),

  /* mark this as start of a cookie session */
  CARLOPT(CARLOPT_COOKIESESSION, CARLOPTTYPE_LONG, 96),

  /* The CApath directory used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CARLOPT(CARLOPT_CAPATH, CARLOPTTYPE_STRINGPOINT, 97),

  /* Instruct libcarl to use a smaller receive buffer */
  CARLOPT(CARLOPT_BUFFERSIZE, CARLOPTTYPE_LONG, 98),

  /* Instruct libcarl to not use any signal/alarm handlers, even when using
     timeouts. This option is useful for multi-threaded applications.
     See libcarl-the-guide for more background information. */
  CARLOPT(CARLOPT_NOSIGNAL, CARLOPTTYPE_LONG, 99),

  /* Provide a CARLShare for mutexing non-ts data */
  CARLOPT(CARLOPT_SHARE, CARLOPTTYPE_OBJECTPOINT, 100),

  /* indicates type of proxy. accepted values are CARLPROXY_HTTP (default),
     CARLPROXY_HTTPS, CARLPROXY_SOCKS4, CARLPROXY_SOCKS4A and
     CARLPROXY_SOCKS5. */
  CARLOPT(CARLOPT_PROXYTYPE, CARLOPTTYPE_VALUES, 101),

  /* Set the Accept-Encoding string. Use this to tell a server you would like
     the response to be compressed. Before 7.21.6, this was known as
     CARLOPT_ENCODING */
  CARLOPT(CARLOPT_ACCEPT_ENCODING, CARLOPTTYPE_STRINGPOINT, 102),

  /* Set pointer to private data */
  CARLOPT(CARLOPT_PRIVATE, CARLOPTTYPE_OBJECTPOINT, 103),

  /* Set aliases for HTTP 200 in the HTTP Response header */
  CARLOPT(CARLOPT_HTTP200ALIASES, CARLOPTTYPE_SLISTPOINT, 104),

  /* Continue to send authentication (user+password) when following locations,
     even when hostname changed. This can potentially send off the name
     and password to whatever host the server decides. */
  CARLOPT(CARLOPT_UNRESTRICTED_AUTH, CARLOPTTYPE_LONG, 105),

  /* Specifically switch on or off the FTP engine's use of the EPRT command (
     it also disables the LPRT attempt). By default, those ones will always be
     attempted before the good old traditional PORT command. */
  CARLOPT(CARLOPT_FTP_USE_EPRT, CARLOPTTYPE_LONG, 106),

  /* Set this to a bitmask value to enable the particular authentications
     methods you like. Use this in combination with CARLOPT_USERPWD.
     Note that setting multiple bits may cause extra network round-trips. */
  CARLOPT(CARLOPT_HTTPAUTH, CARLOPTTYPE_VALUES, 107),

  /* Set the ssl context callback function, currently only for OpenSSL or
     WolfSSL ssl_ctx, or mbedTLS mbedtls_ssl_config in the second argument.
     The function must match the carl_ssl_ctx_callback prototype. */
  CARLOPT(CARLOPT_SSL_CTX_FUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 108),

  /* Set the userdata for the ssl context callback function's third
     argument */
  CARLOPT(CARLOPT_SSL_CTX_DATA, CARLOPTTYPE_CBPOINT, 109),

  /* FTP Option that causes missing dirs to be created on the remote server.
     In 7.19.4 we introduced the convenience enums for this option using the
     CARLFTP_CREATE_DIR prefix.
  */
  CARLOPT(CARLOPT_FTP_CREATE_MISSING_DIRS, CARLOPTTYPE_LONG, 110),

  /* Set this to a bitmask value to enable the particular authentications
     methods you like. Use this in combination with CARLOPT_PROXYUSERPWD.
     Note that setting multiple bits may cause extra network round-trips. */
  CARLOPT(CARLOPT_PROXYAUTH, CARLOPTTYPE_VALUES, 111),

  /* FTP option that changes the timeout, in seconds, associated with
     getting a response.  This is different from transfer timeout time and
     essentially places a demand on the FTP server to acknowledge commands
     in a timely manner. */
  CARLOPT(CARLOPT_FTP_RESPONSE_TIMEOUT, CARLOPTTYPE_LONG, 112),
#define CARLOPT_SERVER_RESPONSE_TIMEOUT CARLOPT_FTP_RESPONSE_TIMEOUT

  /* Set this option to one of the CARL_IPRESOLVE_* defines (see below) to
     tell libcarl to resolve names to those IP versions only. This only has
     affect on systems with support for more than one, i.e IPv4 _and_ IPv6. */
  CARLOPT(CARLOPT_IPRESOLVE, CARLOPTTYPE_VALUES, 113),

  /* Set this option to limit the size of a file that will be downloaded from
     an HTTP or FTP server.

     Note there is also _LARGE version which adds large file support for
     platforms which have larger off_t sizes.  See MAXFILESIZE_LARGE below. */
  CARLOPT(CARLOPT_MAXFILESIZE, CARLOPTTYPE_LONG, 114),

  /* See the comment for INFILESIZE above, but in short, specifies
   * the size of the file being uploaded.  -1 means unknown.
   */
  CARLOPT(CARLOPT_INFILESIZE_LARGE, CARLOPTTYPE_OFF_T, 115),

  /* Sets the continuation offset.  There is also a CARLOPTTYPE_LONG version
   * of this; look above for RESUME_FROM.
   */
  CARLOPT(CARLOPT_RESUME_FROM_LARGE, CARLOPTTYPE_OFF_T, 116),

  /* Sets the maximum size of data that will be downloaded from
   * an HTTP or FTP server.  See MAXFILESIZE above for the LONG version.
   */
  CARLOPT(CARLOPT_MAXFILESIZE_LARGE, CARLOPTTYPE_OFF_T, 117),

  /* Set this option to the file name of your .netrc file you want libcarl
     to parse (using the CARLOPT_NETRC option). If not set, libcarl will do
     a poor attempt to find the user's home directory and check for a .netrc
     file in there. */
  CARLOPT(CARLOPT_NETRC_FILE, CARLOPTTYPE_STRINGPOINT, 118),

  /* Enable SSL/TLS for FTP, pick one of:
     CARLUSESSL_TRY     - try using SSL, proceed anyway otherwise
     CARLUSESSL_CONTROL - SSL for the control connection or fail
     CARLUSESSL_ALL     - SSL for all communication or fail
  */
  CARLOPT(CARLOPT_USE_SSL, CARLOPTTYPE_VALUES, 119),

  /* The _LARGE version of the standard POSTFIELDSIZE option */
  CARLOPT(CARLOPT_POSTFIELDSIZE_LARGE, CARLOPTTYPE_OFF_T, 120),

  /* Enable/disable the TCP Nagle algorithm */
  CARLOPT(CARLOPT_TCP_NODELAY, CARLOPTTYPE_LONG, 121),

  /* 122 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 123 OBSOLETE. Gone in 7.16.0 */
  /* 124 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 125 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 126 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 127 OBSOLETE. Gone in 7.16.0 */
  /* 128 OBSOLETE. Gone in 7.16.0 */

  /* When FTP over SSL/TLS is selected (with CARLOPT_USE_SSL), this option
     can be used to change libcarl's default action which is to first try
     "AUTH SSL" and then "AUTH TLS" in this order, and proceed when a OK
     response has been received.

     Available parameters are:
     CARLFTPAUTH_DEFAULT - let libcarl decide
     CARLFTPAUTH_SSL     - try "AUTH SSL" first, then TLS
     CARLFTPAUTH_TLS     - try "AUTH TLS" first, then SSL
  */
  CARLOPT(CARLOPT_FTPSSLAUTH, CARLOPTTYPE_VALUES, 129),

  CARLOPT(CARLOPT_IOCTLFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 130),
  CARLOPT(CARLOPT_IOCTLDATA, CARLOPTTYPE_CBPOINT, 131),

  /* 132 OBSOLETE. Gone in 7.16.0 */
  /* 133 OBSOLETE. Gone in 7.16.0 */

  /* null-terminated string for pass on to the FTP server when asked for
     "account" info */
  CARLOPT(CARLOPT_FTP_ACCOUNT, CARLOPTTYPE_STRINGPOINT, 134),

  /* feed cookie into cookie engine */
  CARLOPT(CARLOPT_COOKIELIST, CARLOPTTYPE_STRINGPOINT, 135),

  /* ignore Content-Length */
  CARLOPT(CARLOPT_IGNORE_CONTENT_LENGTH, CARLOPTTYPE_LONG, 136),

  /* Set to non-zero to skip the IP address received in a 227 PASV FTP server
     response. Typically used for FTP-SSL purposes but is not restricted to
     that. libcarl will then instead use the same IP address it used for the
     control connection. */
  CARLOPT(CARLOPT_FTP_SKIP_PASV_IP, CARLOPTTYPE_LONG, 137),

  /* Select "file method" to use when doing FTP, see the carl_ftpmethod
     above. */
  CARLOPT(CARLOPT_FTP_FILEMETHOD, CARLOPTTYPE_VALUES, 138),

  /* Local port number to bind the socket to */
  CARLOPT(CARLOPT_LOCALPORT, CARLOPTTYPE_LONG, 139),

  /* Number of ports to try, including the first one set with LOCALPORT.
     Thus, setting it to 1 will make no additional attempts but the first.
  */
  CARLOPT(CARLOPT_LOCALPORTRANGE, CARLOPTTYPE_LONG, 140),

  /* no transfer, set up connection and let application use the socket by
     extracting it with CARLINFO_LASTSOCKET */
  CARLOPT(CARLOPT_CONNECT_ONLY, CARLOPTTYPE_LONG, 141),

  /* Function that will be called to convert from the
     network encoding (instead of using the iconv calls in libcarl) */
  CARLOPT(CARLOPT_CONV_FROM_NETWORK_FUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 142),

  /* Function that will be called to convert to the
     network encoding (instead of using the iconv calls in libcarl) */
  CARLOPT(CARLOPT_CONV_TO_NETWORK_FUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 143),

  /* Function that will be called to convert from UTF8
     (instead of using the iconv calls in libcarl)
     Note that this is used only for SSL certificate processing */
  CARLOPT(CARLOPT_CONV_FROM_UTF8_FUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 144),

  /* if the connection proceeds too quickly then need to slow it down */
  /* limit-rate: maximum number of bytes per second to send or receive */
  CARLOPT(CARLOPT_MAX_SEND_SPEED_LARGE, CARLOPTTYPE_OFF_T, 145),
  CARLOPT(CARLOPT_MAX_RECV_SPEED_LARGE, CARLOPTTYPE_OFF_T, 146),

  /* Pointer to command string to send if USER/PASS fails. */
  CARLOPT(CARLOPT_FTP_ALTERNATIVE_TO_USER, CARLOPTTYPE_STRINGPOINT, 147),

  /* callback function for setting socket options */
  CARLOPT(CARLOPT_SOCKOPTFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 148),
  CARLOPT(CARLOPT_SOCKOPTDATA, CARLOPTTYPE_CBPOINT, 149),

  /* set to 0 to disable session ID re-use for this transfer, default is
     enabled (== 1) */
  CARLOPT(CARLOPT_SSL_SESSIONID_CACHE, CARLOPTTYPE_LONG, 150),

  /* allowed SSH authentication methods */
  CARLOPT(CARLOPT_SSH_AUTH_TYPES, CARLOPTTYPE_VALUES, 151),

  /* Used by scp/sftp to do public/private key authentication */
  CARLOPT(CARLOPT_SSH_PUBLIC_KEYFILE, CARLOPTTYPE_STRINGPOINT, 152),
  CARLOPT(CARLOPT_SSH_PRIVATE_KEYFILE, CARLOPTTYPE_STRINGPOINT, 153),

  /* Send CCC (Clear Command Channel) after authentication */
  CARLOPT(CARLOPT_FTP_SSL_CCC, CARLOPTTYPE_LONG, 154),

  /* Same as TIMEOUT and CONNECTTIMEOUT, but with ms resolution */
  CARLOPT(CARLOPT_TIMEOUT_MS, CARLOPTTYPE_LONG, 155),
  CARLOPT(CARLOPT_CONNECTTIMEOUT_MS, CARLOPTTYPE_LONG, 156),

  /* set to zero to disable the libcarl's decoding and thus pass the raw body
     data to the application even when it is encoded/compressed */
  CARLOPT(CARLOPT_HTTP_TRANSFER_DECODING, CARLOPTTYPE_LONG, 157),
  CARLOPT(CARLOPT_HTTP_CONTENT_DECODING, CARLOPTTYPE_LONG, 158),

  /* Permission used when creating new files and directories on the remote
     server for protocols that support it, SFTP/SCP/FILE */
  CARLOPT(CARLOPT_NEW_FILE_PERMS, CARLOPTTYPE_LONG, 159),
  CARLOPT(CARLOPT_NEW_DIRECTORY_PERMS, CARLOPTTYPE_LONG, 160),

  /* Set the behavior of POST when redirecting. Values must be set to one
     of CARL_REDIR* defines below. This used to be called CARLOPT_POST301 */
  CARLOPT(CARLOPT_POSTREDIR, CARLOPTTYPE_VALUES, 161),

  /* used by scp/sftp to verify the host's public key */
  CARLOPT(CARLOPT_SSH_HOST_PUBLIC_KEY_MD5, CARLOPTTYPE_STRINGPOINT, 162),

  /* Callback function for opening socket (instead of socket(2)). Optionally,
     callback is able change the address or refuse to connect returning
     CARL_SOCKET_BAD.  The callback should have type
     carl_opensocket_callback */
  CARLOPT(CARLOPT_OPENSOCKETFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 163),
  CARLOPT(CARLOPT_OPENSOCKETDATA, CARLOPTTYPE_CBPOINT, 164),

  /* POST volatile input fields. */
  CARLOPT(CARLOPT_COPYPOSTFIELDS, CARLOPTTYPE_OBJECTPOINT, 165),

  /* set transfer mode (;type=<a|i>) when doing FTP via an HTTP proxy */
  CARLOPT(CARLOPT_PROXY_TRANSFER_MODE, CARLOPTTYPE_LONG, 166),

  /* Callback function for seeking in the input stream */
  CARLOPT(CARLOPT_SEEKFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 167),
  CARLOPT(CARLOPT_SEEKDATA, CARLOPTTYPE_CBPOINT, 168),

  /* CRL file */
  CARLOPT(CARLOPT_CRLFILE, CARLOPTTYPE_STRINGPOINT, 169),

  /* Issuer certificate */
  CARLOPT(CARLOPT_ISSUERCERT, CARLOPTTYPE_STRINGPOINT, 170),

  /* (IPv6) Address scope */
  CARLOPT(CARLOPT_ADDRESS_SCOPE, CARLOPTTYPE_LONG, 171),

  /* Collect certificate chain info and allow it to get retrievable with
     CARLINFO_CERTINFO after the transfer is complete. */
  CARLOPT(CARLOPT_CERTINFO, CARLOPTTYPE_LONG, 172),

  /* "name" and "pwd" to use when fetching. */
  CARLOPT(CARLOPT_USERNAME, CARLOPTTYPE_STRINGPOINT, 173),
  CARLOPT(CARLOPT_PASSWORD, CARLOPTTYPE_STRINGPOINT, 174),

    /* "name" and "pwd" to use with Proxy when fetching. */
  CARLOPT(CARLOPT_PROXYUSERNAME, CARLOPTTYPE_STRINGPOINT, 175),
  CARLOPT(CARLOPT_PROXYPASSWORD, CARLOPTTYPE_STRINGPOINT, 176),

  /* Comma separated list of hostnames defining no-proxy zones. These should
     match both hostnames directly, and hostnames within a domain. For
     example, local.com will match local.com and www.local.com, but NOT
     notlocal.com or www.notlocal.com. For compatibility with other
     implementations of this, .local.com will be considered to be the same as
     local.com. A single * is the only valid wildcard, and effectively
     disables the use of proxy. */
  CARLOPT(CARLOPT_NOPROXY, CARLOPTTYPE_STRINGPOINT, 177),

  /* block size for TFTP transfers */
  CARLOPT(CARLOPT_TFTP_BLKSIZE, CARLOPTTYPE_LONG, 178),

  /* Socks Service */
  /* DEPRECATED, do not use! */
  CARLOPT(CARLOPT_SOCKS5_GSSAPI_SERVICE, CARLOPTTYPE_STRINGPOINT, 179),

  /* Socks Service */
  CARLOPT(CARLOPT_SOCKS5_GSSAPI_NEC, CARLOPTTYPE_LONG, 180),

  /* set the bitmask for the protocols that are allowed to be used for the
     transfer, which thus helps the app which takes URLs from users or other
     external inputs and want to restrict what protocol(s) to deal
     with. Defaults to CARLPROTO_ALL. */
  CARLOPT(CARLOPT_PROTOCOLS, CARLOPTTYPE_LONG, 181),

  /* set the bitmask for the protocols that libcarl is allowed to follow to,
     as a subset of the CARLOPT_PROTOCOLS ones. That means the protocol needs
     to be set in both bitmasks to be allowed to get redirected to. */
  CARLOPT(CARLOPT_REDIR_PROTOCOLS, CARLOPTTYPE_LONG, 182),

  /* set the SSH knownhost file name to use */
  CARLOPT(CARLOPT_SSH_KNOWNHOSTS, CARLOPTTYPE_STRINGPOINT, 183),

  /* set the SSH host key callback, must point to a carl_sshkeycallback
     function */
  CARLOPT(CARLOPT_SSH_KEYFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 184),

  /* set the SSH host key callback custom pointer */
  CARLOPT(CARLOPT_SSH_KEYDATA, CARLOPTTYPE_CBPOINT, 185),

  /* set the SMTP mail originator */
  CARLOPT(CARLOPT_MAIL_FROM, CARLOPTTYPE_STRINGPOINT, 186),

  /* set the list of SMTP mail receiver(s) */
  CARLOPT(CARLOPT_MAIL_RCPT, CARLOPTTYPE_SLISTPOINT, 187),

  /* FTP: send PRET before PASV */
  CARLOPT(CARLOPT_FTP_USE_PRET, CARLOPTTYPE_LONG, 188),

  /* RTSP request method (OPTIONS, SETUP, PLAY, etc...) */
  CARLOPT(CARLOPT_RTSP_REQUEST, CARLOPTTYPE_VALUES, 189),

  /* The RTSP session identifier */
  CARLOPT(CARLOPT_RTSP_SESSION_ID, CARLOPTTYPE_STRINGPOINT, 190),

  /* The RTSP stream URI */
  CARLOPT(CARLOPT_RTSP_STREAM_URI, CARLOPTTYPE_STRINGPOINT, 191),

  /* The Transport: header to use in RTSP requests */
  CARLOPT(CARLOPT_RTSP_TRANSPORT, CARLOPTTYPE_STRINGPOINT, 192),

  /* Manually initialize the client RTSP CSeq for this handle */
  CARLOPT(CARLOPT_RTSP_CLIENT_CSEQ, CARLOPTTYPE_LONG, 193),

  /* Manually initialize the server RTSP CSeq for this handle */
  CARLOPT(CARLOPT_RTSP_SERVER_CSEQ, CARLOPTTYPE_LONG, 194),

  /* The stream to pass to INTERLEAVEFUNCTION. */
  CARLOPT(CARLOPT_INTERLEAVEDATA, CARLOPTTYPE_CBPOINT, 195),

  /* Let the application define a custom write method for RTP data */
  CARLOPT(CARLOPT_INTERLEAVEFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 196),

  /* Turn on wildcard matching */
  CARLOPT(CARLOPT_WILDCARDMATCH, CARLOPTTYPE_LONG, 197),

  /* Directory matching callback called before downloading of an
     individual file (chunk) started */
  CARLOPT(CARLOPT_CHUNK_BGN_FUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 198),

  /* Directory matching callback called after the file (chunk)
     was downloaded, or skipped */
  CARLOPT(CARLOPT_CHUNK_END_FUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 199),

  /* Change match (fnmatch-like) callback for wildcard matching */
  CARLOPT(CARLOPT_FNMATCH_FUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 200),

  /* Let the application define custom chunk data pointer */
  CARLOPT(CARLOPT_CHUNK_DATA, CARLOPTTYPE_CBPOINT, 201),

  /* FNMATCH_FUNCTION user pointer */
  CARLOPT(CARLOPT_FNMATCH_DATA, CARLOPTTYPE_CBPOINT, 202),

  /* send linked-list of name:port:address sets */
  CARLOPT(CARLOPT_RESOLVE, CARLOPTTYPE_SLISTPOINT, 203),

  /* Set a username for authenticated TLS */
  CARLOPT(CARLOPT_TLSAUTH_USERNAME, CARLOPTTYPE_STRINGPOINT, 204),

  /* Set a password for authenticated TLS */
  CARLOPT(CARLOPT_TLSAUTH_PASSWORD, CARLOPTTYPE_STRINGPOINT, 205),

  /* Set authentication type for authenticated TLS */
  CARLOPT(CARLOPT_TLSAUTH_TYPE, CARLOPTTYPE_STRINGPOINT, 206),

  /* Set to 1 to enable the "TE:" header in HTTP requests to ask for
     compressed transfer-encoded responses. Set to 0 to disable the use of TE:
     in outgoing requests. The current default is 0, but it might change in a
     future libcarl release.

     libcarl will ask for the compressed methods it knows of, and if that
     isn't any, it will not ask for transfer-encoding at all even if this
     option is set to 1.

  */
  CARLOPT(CARLOPT_TRANSFER_ENCODING, CARLOPTTYPE_LONG, 207),

  /* Callback function for closing socket (instead of close(2)). The callback
     should have type carl_closesocket_callback */
  CARLOPT(CARLOPT_CLOSESOCKETFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 208),
  CARLOPT(CARLOPT_CLOSESOCKETDATA, CARLOPTTYPE_CBPOINT, 209),

  /* allow GSSAPI credential delegation */
  CARLOPT(CARLOPT_GSSAPI_DELEGATION, CARLOPTTYPE_VALUES, 210),

  /* Set the name servers to use for DNS resolution */
  CARLOPT(CARLOPT_DNS_SERVERS, CARLOPTTYPE_STRINGPOINT, 211),

  /* Time-out accept operations (currently for FTP only) after this amount
     of milliseconds. */
  CARLOPT(CARLOPT_ACCEPTTIMEOUT_MS, CARLOPTTYPE_LONG, 212),

  /* Set TCP keepalive */
  CARLOPT(CARLOPT_TCP_KEEPALIVE, CARLOPTTYPE_LONG, 213),

  /* non-universal keepalive knobs (Linux, AIX, HP-UX, more) */
  CARLOPT(CARLOPT_TCP_KEEPIDLE, CARLOPTTYPE_LONG, 214),
  CARLOPT(CARLOPT_TCP_KEEPINTVL, CARLOPTTYPE_LONG, 215),

  /* Enable/disable specific SSL features with a bitmask, see CARLSSLOPT_* */
  CARLOPT(CARLOPT_SSL_OPTIONS, CARLOPTTYPE_VALUES, 216),

  /* Set the SMTP auth originator */
  CARLOPT(CARLOPT_MAIL_AUTH, CARLOPTTYPE_STRINGPOINT, 217),

  /* Enable/disable SASL initial response */
  CARLOPT(CARLOPT_SASL_IR, CARLOPTTYPE_LONG, 218),

  /* Function that will be called instead of the internal progress display
   * function. This function should be defined as the carl_xferinfo_callback
   * prototype defines. (Deprecates CARLOPT_PROGRESSFUNCTION) */
  CARLOPT(CARLOPT_XFERINFOFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 219),

  /* The XOAUTH2 bearer token */
  CARLOPT(CARLOPT_XOAUTH2_BEARER, CARLOPTTYPE_STRINGPOINT, 220),

  /* Set the interface string to use as outgoing network
   * interface for DNS requests.
   * Only supported by the c-ares DNS backend */
  CARLOPT(CARLOPT_DNS_INTERFACE, CARLOPTTYPE_STRINGPOINT, 221),

  /* Set the local IPv4 address to use for outgoing DNS requests.
   * Only supported by the c-ares DNS backend */
  CARLOPT(CARLOPT_DNS_LOCAL_IP4, CARLOPTTYPE_STRINGPOINT, 222),

  /* Set the local IPv6 address to use for outgoing DNS requests.
   * Only supported by the c-ares DNS backend */
  CARLOPT(CARLOPT_DNS_LOCAL_IP6, CARLOPTTYPE_STRINGPOINT, 223),

  /* Set authentication options directly */
  CARLOPT(CARLOPT_LOGIN_OPTIONS, CARLOPTTYPE_STRINGPOINT, 224),

  /* Enable/disable TLS NPN extension (http2 over ssl might fail without) */
  CARLOPT(CARLOPT_SSL_ENABLE_NPN, CARLOPTTYPE_LONG, 225),

  /* Enable/disable TLS ALPN extension (http2 over ssl might fail without) */
  CARLOPT(CARLOPT_SSL_ENABLE_ALPN, CARLOPTTYPE_LONG, 226),

  /* Time to wait for a response to a HTTP request containing an
   * Expect: 100-continue header before sending the data anyway. */
  CARLOPT(CARLOPT_EXPECT_100_TIMEOUT_MS, CARLOPTTYPE_LONG, 227),

  /* This points to a linked list of headers used for proxy requests only,
     struct carl_slist kind */
  CARLOPT(CARLOPT_PROXYHEADER, CARLOPTTYPE_SLISTPOINT, 228),

  /* Pass in a bitmask of "header options" */
  CARLOPT(CARLOPT_HEADEROPT, CARLOPTTYPE_VALUES, 229),

  /* The public key in DER form used to validate the peer public key
     this option is used only if SSL_VERIFYPEER is true */
  CARLOPT(CARLOPT_PINNEDPUBLICKEY, CARLOPTTYPE_STRINGPOINT, 230),

  /* Path to Unix domain socket */
  CARLOPT(CARLOPT_UNIX_SOCKET_PATH, CARLOPTTYPE_STRINGPOINT, 231),

  /* Set if we should verify the certificate status. */
  CARLOPT(CARLOPT_SSL_VERIFYSTATUS, CARLOPTTYPE_LONG, 232),

  /* Set if we should enable TLS false start. */
  CARLOPT(CARLOPT_SSL_FALSESTART, CARLOPTTYPE_LONG, 233),

  /* Do not squash dot-dot sequences */
  CARLOPT(CARLOPT_PATH_AS_IS, CARLOPTTYPE_LONG, 234),

  /* Proxy Service Name */
  CARLOPT(CARLOPT_PROXY_SERVICE_NAME, CARLOPTTYPE_STRINGPOINT, 235),

  /* Service Name */
  CARLOPT(CARLOPT_SERVICE_NAME, CARLOPTTYPE_STRINGPOINT, 236),

  /* Wait/don't wait for pipe/mutex to clarify */
  CARLOPT(CARLOPT_PIPEWAIT, CARLOPTTYPE_LONG, 237),

  /* Set the protocol used when carl is given a URL without a protocol */
  CARLOPT(CARLOPT_DEFAULT_PROTOCOL, CARLOPTTYPE_STRINGPOINT, 238),

  /* Set stream weight, 1 - 256 (default is 16) */
  CARLOPT(CARLOPT_STREAM_WEIGHT, CARLOPTTYPE_LONG, 239),

  /* Set stream dependency on another CARL handle */
  CARLOPT(CARLOPT_STREAM_DEPENDS, CARLOPTTYPE_OBJECTPOINT, 240),

  /* Set E-xclusive stream dependency on another CARL handle */
  CARLOPT(CARLOPT_STREAM_DEPENDS_E, CARLOPTTYPE_OBJECTPOINT, 241),

  /* Do not send any tftp option requests to the server */
  CARLOPT(CARLOPT_TFTP_NO_OPTIONS, CARLOPTTYPE_LONG, 242),

  /* Linked-list of host:port:connect-to-host:connect-to-port,
     overrides the URL's host:port (only for the network layer) */
  CARLOPT(CARLOPT_CONNECT_TO, CARLOPTTYPE_SLISTPOINT, 243),

  /* Set TCP Fast Open */
  CARLOPT(CARLOPT_TCP_FASTOPEN, CARLOPTTYPE_LONG, 244),

  /* Continue to send data if the server responds early with an
   * HTTP status code >= 300 */
  CARLOPT(CARLOPT_KEEP_SENDING_ON_ERROR, CARLOPTTYPE_LONG, 245),

  /* The CApath or CAfile used to validate the proxy certificate
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CARLOPT(CARLOPT_PROXY_CAINFO, CARLOPTTYPE_STRINGPOINT, 246),

  /* The CApath directory used to validate the proxy certificate
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CARLOPT(CARLOPT_PROXY_CAPATH, CARLOPTTYPE_STRINGPOINT, 247),

  /* Set if we should verify the proxy in ssl handshake,
     set 1 to verify. */
  CARLOPT(CARLOPT_PROXY_SSL_VERIFYPEER, CARLOPTTYPE_LONG, 248),

  /* Set if we should verify the Common name from the proxy certificate in ssl
   * handshake, set 1 to check existence, 2 to ensure that it matches
   * the provided hostname. */
  CARLOPT(CARLOPT_PROXY_SSL_VERIFYHOST, CARLOPTTYPE_LONG, 249),

  /* What version to specifically try to use for proxy.
     See CARL_SSLVERSION defines below. */
  CARLOPT(CARLOPT_PROXY_SSLVERSION, CARLOPTTYPE_VALUES, 250),

  /* Set a username for authenticated TLS for proxy */
  CARLOPT(CARLOPT_PROXY_TLSAUTH_USERNAME, CARLOPTTYPE_STRINGPOINT, 251),

  /* Set a password for authenticated TLS for proxy */
  CARLOPT(CARLOPT_PROXY_TLSAUTH_PASSWORD, CARLOPTTYPE_STRINGPOINT, 252),

  /* Set authentication type for authenticated TLS for proxy */
  CARLOPT(CARLOPT_PROXY_TLSAUTH_TYPE, CARLOPTTYPE_STRINGPOINT, 253),

  /* name of the file keeping your private SSL-certificate for proxy */
  CARLOPT(CARLOPT_PROXY_SSLCERT, CARLOPTTYPE_STRINGPOINT, 254),

  /* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") for
     proxy */
  CARLOPT(CARLOPT_PROXY_SSLCERTTYPE, CARLOPTTYPE_STRINGPOINT, 255),

  /* name of the file keeping your private SSL-key for proxy */
  CARLOPT(CARLOPT_PROXY_SSLKEY, CARLOPTTYPE_STRINGPOINT, 256),

  /* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") for
     proxy */
  CARLOPT(CARLOPT_PROXY_SSLKEYTYPE, CARLOPTTYPE_STRINGPOINT, 257),

  /* password for the SSL private key for proxy */
  CARLOPT(CARLOPT_PROXY_KEYPASSWD, CARLOPTTYPE_STRINGPOINT, 258),

  /* Specify which SSL ciphers to use for proxy */
  CARLOPT(CARLOPT_PROXY_SSL_CIPHER_LIST, CARLOPTTYPE_STRINGPOINT, 259),

  /* CRL file for proxy */
  CARLOPT(CARLOPT_PROXY_CRLFILE, CARLOPTTYPE_STRINGPOINT, 260),

  /* Enable/disable specific SSL features with a bitmask for proxy, see
     CARLSSLOPT_* */
  CARLOPT(CARLOPT_PROXY_SSL_OPTIONS, CARLOPTTYPE_LONG, 261),

  /* Name of pre proxy to use. */
  CARLOPT(CARLOPT_PRE_PROXY, CARLOPTTYPE_STRINGPOINT, 262),

  /* The public key in DER form used to validate the proxy public key
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CARLOPT(CARLOPT_PROXY_PINNEDPUBLICKEY, CARLOPTTYPE_STRINGPOINT, 263),

  /* Path to an abstract Unix domain socket */
  CARLOPT(CARLOPT_ABSTRACT_UNIX_SOCKET, CARLOPTTYPE_STRINGPOINT, 264),

  /* Suppress proxy CONNECT response headers from user callbacks */
  CARLOPT(CARLOPT_SUPPRESS_CONNECT_HEADERS, CARLOPTTYPE_LONG, 265),

  /* The request target, instead of extracted from the URL */
  CARLOPT(CARLOPT_REQUEST_TARGET, CARLOPTTYPE_STRINGPOINT, 266),

  /* bitmask of allowed auth methods for connections to SOCKS5 proxies */
  CARLOPT(CARLOPT_SOCKS5_AUTH, CARLOPTTYPE_LONG, 267),

  /* Enable/disable SSH compression */
  CARLOPT(CARLOPT_SSH_COMPRESSION, CARLOPTTYPE_LONG, 268),

  /* Post MIME data. */
  CARLOPT(CARLOPT_MIMEPOST, CARLOPTTYPE_OBJECTPOINT, 269),

  /* Time to use with the CARLOPT_TIMECONDITION. Specified in number of
     seconds since 1 Jan 1970. */
  CARLOPT(CARLOPT_TIMEVALUE_LARGE, CARLOPTTYPE_OFF_T, 270),

  /* Head start in milliseconds to give happy eyeballs. */
  CARLOPT(CARLOPT_HAPPY_EYEBALLS_TIMEOUT_MS, CARLOPTTYPE_LONG, 271),

  /* Function that will be called before a resolver request is made */
  CARLOPT(CARLOPT_RESOLVER_START_FUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 272),

  /* User data to pass to the resolver start callback. */
  CARLOPT(CARLOPT_RESOLVER_START_DATA, CARLOPTTYPE_CBPOINT, 273),

  /* send HAProxy PROXY protocol header? */
  CARLOPT(CARLOPT_HAPROXYPROTOCOL, CARLOPTTYPE_LONG, 274),

  /* shuffle addresses before use when DNS returns multiple */
  CARLOPT(CARLOPT_DNS_SHUFFLE_ADDRESSES, CARLOPTTYPE_LONG, 275),

  /* Specify which TLS 1.3 ciphers suites to use */
  CARLOPT(CARLOPT_TLS13_CIPHERS, CARLOPTTYPE_STRINGPOINT, 276),
  CARLOPT(CARLOPT_PROXY_TLS13_CIPHERS, CARLOPTTYPE_STRINGPOINT, 277),

  /* Disallow specifying username/login in URL. */
  CARLOPT(CARLOPT_DISALLOW_USERNAME_IN_URL, CARLOPTTYPE_LONG, 278),

  /* DNS-over-HTTPS URL */
  CARLOPT(CARLOPT_DOH_URL, CARLOPTTYPE_STRINGPOINT, 279),

  /* Preferred buffer size to use for uploads */
  CARLOPT(CARLOPT_UPLOAD_BUFFERSIZE, CARLOPTTYPE_LONG, 280),

  /* Time in ms between connection upkeep calls for long-lived connections. */
  CARLOPT(CARLOPT_UPKEEP_INTERVAL_MS, CARLOPTTYPE_LONG, 281),

  /* Specify URL using CARL URL API. */
  CARLOPT(CARLOPT_CARLU, CARLOPTTYPE_OBJECTPOINT, 282),

  /* add trailing data just after no more data is available */
  CARLOPT(CARLOPT_TRAILERFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 283),

  /* pointer to be passed to HTTP_TRAILER_FUNCTION */
  CARLOPT(CARLOPT_TRAILERDATA, CARLOPTTYPE_CBPOINT, 284),

  /* set this to 1L to allow HTTP/0.9 responses or 0L to disallow */
  CARLOPT(CARLOPT_HTTP09_ALLOWED, CARLOPTTYPE_LONG, 285),

  /* alt-svc control bitmask */
  CARLOPT(CARLOPT_ALTSVC_CTRL, CARLOPTTYPE_LONG, 286),

  /* alt-svc cache file name to possibly read from/write to */
  CARLOPT(CARLOPT_ALTSVC, CARLOPTTYPE_STRINGPOINT, 287),

  /* maximum age of a connection to consider it for reuse (in seconds) */
  CARLOPT(CARLOPT_MAXAGE_CONN, CARLOPTTYPE_LONG, 288),

  /* SASL authorisation identity */
  CARLOPT(CARLOPT_SASL_AUTHZID, CARLOPTTYPE_STRINGPOINT, 289),

  /* allow RCPT TO command to fail for some recipients */
  CARLOPT(CARLOPT_MAIL_RCPT_ALLLOWFAILS, CARLOPTTYPE_LONG, 290),

  /* the private SSL-certificate as a "blob" */
  CARLOPT(CARLOPT_SSLCERT_BLOB, CARLOPTTYPE_BLOB, 291),
  CARLOPT(CARLOPT_SSLKEY_BLOB, CARLOPTTYPE_BLOB, 292),
  CARLOPT(CARLOPT_PROXY_SSLCERT_BLOB, CARLOPTTYPE_BLOB, 293),
  CARLOPT(CARLOPT_PROXY_SSLKEY_BLOB, CARLOPTTYPE_BLOB, 294),
  CARLOPT(CARLOPT_ISSUERCERT_BLOB, CARLOPTTYPE_BLOB, 295),

  /* Issuer certificate for proxy */
  CARLOPT(CARLOPT_PROXY_ISSUERCERT, CARLOPTTYPE_STRINGPOINT, 296),
  CARLOPT(CARLOPT_PROXY_ISSUERCERT_BLOB, CARLOPTTYPE_BLOB, 297),

  /* the EC curves requested by the TLS client (RFC 8422, 5.1);
   * OpenSSL support via 'set_groups'/'set_curves':
   * https://www.openssl.org/docs/manmaster/man3/SSL_CTX_set1_groups.html
   */
  CARLOPT(CARLOPT_SSL_EC_CURVES, CARLOPTTYPE_STRINGPOINT, 298),

  /* HSTS bitmask */
  CARLOPT(CARLOPT_HSTS_CTRL, CARLOPTTYPE_LONG, 299),
  /* HSTS file name */
  CARLOPT(CARLOPT_HSTS, CARLOPTTYPE_STRINGPOINT, 300),

  /* HSTS read callback */
  CARLOPT(CARLOPT_HSTSREADFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 301),
  CARLOPT(CARLOPT_HSTSREADDATA, CARLOPTTYPE_CBPOINT, 302),

  /* HSTS write callback */
  CARLOPT(CARLOPT_HSTSWRITEFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 303),
  CARLOPT(CARLOPT_HSTSWRITEDATA, CARLOPTTYPE_CBPOINT, 304),

  /* Provider for V4 signature */
  CARLOPT(CARLOPT_AWS_SIGV4, CARLOPTTYPE_STRINGPOINT, 305),

  CARLOPT_LASTENTRY /* the last unused */
} CARLoption;

#ifndef CARL_NO_OLDIES /* define this to test if your app builds with all
                          the obsolete stuff removed! */

/* Backwards compatibility with older names */
/* These are scheduled to disappear by 2011 */

/* This was added in version 7.19.1 */
#define CARLOPT_POST301 CARLOPT_POSTREDIR

/* These are scheduled to disappear by 2009 */

/* The following were added in 7.17.0 */
#define CARLOPT_SSLKEYPASSWD CARLOPT_KEYPASSWD
#define CARLOPT_FTPAPPEND CARLOPT_APPEND
#define CARLOPT_FTPLISTONLY CARLOPT_DIRLISTONLY
#define CARLOPT_FTP_SSL CARLOPT_USE_SSL

/* The following were added earlier */

#define CARLOPT_SSLCERTPASSWD CARLOPT_KEYPASSWD
#define CARLOPT_KRB4LEVEL CARLOPT_KRBLEVEL

#else
/* This is set if CARL_NO_OLDIES is defined at compile-time */
#undef CARLOPT_DNS_USE_GLOBAL_CACHE /* soon obsolete */
#endif


  /* Below here follows defines for the CARLOPT_IPRESOLVE option. If a host
     name resolves addresses using more than one IP protocol version, this
     option might be handy to force libcarl to use a specific IP version. */
#define CARL_IPRESOLVE_WHATEVER 0 /* default, resolves addresses to all IP
                                     versions that your system allows */
#define CARL_IPRESOLVE_V4       1 /* resolve to IPv4 addresses */
#define CARL_IPRESOLVE_V6       2 /* resolve to IPv6 addresses */

  /* three convenient "aliases" that follow the name scheme better */
#define CARLOPT_RTSPHEADER CARLOPT_HTTPHEADER

  /* These enums are for use with the CARLOPT_HTTP_VERSION option. */
enum {
  CARL_HTTP_VERSION_NONE, /* setting this means we don't care, and that we'd
                             like the library to choose the best possible
                             for us! */
  CARL_HTTP_VERSION_1_0,  /* please use HTTP 1.0 in the request */
  CARL_HTTP_VERSION_1_1,  /* please use HTTP 1.1 in the request */
  CARL_HTTP_VERSION_2_0,  /* please use HTTP 2 in the request */
  CARL_HTTP_VERSION_2TLS, /* use version 2 for HTTPS, version 1.1 for HTTP */
  CARL_HTTP_VERSION_2_PRIOR_KNOWLEDGE,  /* please use HTTP 2 without HTTP/1.1
                                           Upgrade */
  CARL_HTTP_VERSION_3 = 30, /* Makes use of explicit HTTP/3 without fallback.
                               Use CARLOPT_ALTSVC to enable HTTP/3 upgrade */
  CARL_HTTP_VERSION_LAST /* *ILLEGAL* http version */
};

/* Convenience definition simple because the name of the version is HTTP/2 and
   not 2.0. The 2_0 version of the enum name was set while the version was
   still planned to be 2.0 and we stick to it for compatibility. */
#define CARL_HTTP_VERSION_2 CARL_HTTP_VERSION_2_0

/*
 * Public API enums for RTSP requests
 */
enum {
    CARL_RTSPREQ_NONE, /* first in list */
    CARL_RTSPREQ_OPTIONS,
    CARL_RTSPREQ_DESCRIBE,
    CARL_RTSPREQ_ANNOUNCE,
    CARL_RTSPREQ_SETUP,
    CARL_RTSPREQ_PLAY,
    CARL_RTSPREQ_PAUSE,
    CARL_RTSPREQ_TEARDOWN,
    CARL_RTSPREQ_GET_PARAMETER,
    CARL_RTSPREQ_SET_PARAMETER,
    CARL_RTSPREQ_RECORD,
    CARL_RTSPREQ_RECEIVE,
    CARL_RTSPREQ_LAST /* last in list */
};

  /* These enums are for use with the CARLOPT_NETRC option. */
enum CARL_NETRC_OPTION {
  CARL_NETRC_IGNORED,     /* The .netrc will never be read.
                           * This is the default. */
  CARL_NETRC_OPTIONAL,    /* A user:password in the URL will be preferred
                           * to one in the .netrc. */
  CARL_NETRC_REQUIRED,    /* A user:password in the URL will be ignored.
                           * Unless one is set programmatically, the .netrc
                           * will be queried. */
  CARL_NETRC_LAST
};

enum {
  CARL_SSLVERSION_DEFAULT,
  CARL_SSLVERSION_TLSv1, /* TLS 1.x */
  CARL_SSLVERSION_SSLv2,
  CARL_SSLVERSION_SSLv3,
  CARL_SSLVERSION_TLSv1_0,
  CARL_SSLVERSION_TLSv1_1,
  CARL_SSLVERSION_TLSv1_2,
  CARL_SSLVERSION_TLSv1_3,

  CARL_SSLVERSION_LAST /* never use, keep last */
};

enum {
  CARL_SSLVERSION_MAX_NONE =     0,
  CARL_SSLVERSION_MAX_DEFAULT =  (CARL_SSLVERSION_TLSv1   << 16),
  CARL_SSLVERSION_MAX_TLSv1_0 =  (CARL_SSLVERSION_TLSv1_0 << 16),
  CARL_SSLVERSION_MAX_TLSv1_1 =  (CARL_SSLVERSION_TLSv1_1 << 16),
  CARL_SSLVERSION_MAX_TLSv1_2 =  (CARL_SSLVERSION_TLSv1_2 << 16),
  CARL_SSLVERSION_MAX_TLSv1_3 =  (CARL_SSLVERSION_TLSv1_3 << 16),

  /* never use, keep last */
  CARL_SSLVERSION_MAX_LAST =     (CARL_SSLVERSION_LAST    << 16)
};

enum CARL_TLSAUTH {
  CARL_TLSAUTH_NONE,
  CARL_TLSAUTH_SRP,
  CARL_TLSAUTH_LAST /* never use, keep last */
};

/* symbols to use with CARLOPT_POSTREDIR.
   CARL_REDIR_POST_301, CARL_REDIR_POST_302 and CARL_REDIR_POST_303
   can be bitwise ORed so that CARL_REDIR_POST_301 | CARL_REDIR_POST_302
   | CARL_REDIR_POST_303 == CARL_REDIR_POST_ALL */

#define CARL_REDIR_GET_ALL  0
#define CARL_REDIR_POST_301 1
#define CARL_REDIR_POST_302 2
#define CARL_REDIR_POST_303 4
#define CARL_REDIR_POST_ALL \
    (CARL_REDIR_POST_301|CARL_REDIR_POST_302|CARL_REDIR_POST_303)

typedef enum {
  CARL_TIMECOND_NONE,

  CARL_TIMECOND_IFMODSINCE,
  CARL_TIMECOND_IFUNMODSINCE,
  CARL_TIMECOND_LASTMOD,

  CARL_TIMECOND_LAST
} carl_TimeCond;

/* Special size_t value signaling a null-terminated string. */
#define CARL_ZERO_TERMINATED ((size_t) -1)

/* carl_strequal() and carl_strnequal() are subject for removal in a future
   release */
CARL_EXTERN int carl_strequal(const char *s1, const char *s2);
CARL_EXTERN int carl_strnequal(const char *s1, const char *s2, size_t n);

/* Mime/form handling support. */
typedef struct carl_mime      carl_mime;      /* Mime context. */
typedef struct carl_mimepart  carl_mimepart;  /* Mime part context. */

/*
 * NAME carl_mime_init()
 *
 * DESCRIPTION
 *
 * Create a mime context and return its handle. The easy parameter is the
 * target handle.
 */
CARL_EXTERN carl_mime *carl_mime_init(CARL *easy);

/*
 * NAME carl_mime_free()
 *
 * DESCRIPTION
 *
 * release a mime handle and its substructures.
 */
CARL_EXTERN void carl_mime_free(carl_mime *mime);

/*
 * NAME carl_mime_addpart()
 *
 * DESCRIPTION
 *
 * Append a new empty part to the given mime context and return a handle to
 * the created part.
 */
CARL_EXTERN carl_mimepart *carl_mime_addpart(carl_mime *mime);

/*
 * NAME carl_mime_name()
 *
 * DESCRIPTION
 *
 * Set mime/form part name.
 */
CARL_EXTERN CARLcode carl_mime_name(carl_mimepart *part, const char *name);

/*
 * NAME carl_mime_filename()
 *
 * DESCRIPTION
 *
 * Set mime part remote file name.
 */
CARL_EXTERN CARLcode carl_mime_filename(carl_mimepart *part,
                                        const char *filename);

/*
 * NAME carl_mime_type()
 *
 * DESCRIPTION
 *
 * Set mime part type.
 */
CARL_EXTERN CARLcode carl_mime_type(carl_mimepart *part, const char *mimetype);

/*
 * NAME carl_mime_encoder()
 *
 * DESCRIPTION
 *
 * Set mime data transfer encoder.
 */
CARL_EXTERN CARLcode carl_mime_encoder(carl_mimepart *part,
                                       const char *encoding);

/*
 * NAME carl_mime_data()
 *
 * DESCRIPTION
 *
 * Set mime part data source from memory data,
 */
CARL_EXTERN CARLcode carl_mime_data(carl_mimepart *part,
                                    const char *data, size_t datasize);

/*
 * NAME carl_mime_filedata()
 *
 * DESCRIPTION
 *
 * Set mime part data source from named file.
 */
CARL_EXTERN CARLcode carl_mime_filedata(carl_mimepart *part,
                                        const char *filename);

/*
 * NAME carl_mime_data_cb()
 *
 * DESCRIPTION
 *
 * Set mime part data source from callback function.
 */
CARL_EXTERN CARLcode carl_mime_data_cb(carl_mimepart *part,
                                       carl_off_t datasize,
                                       carl_read_callback readfunc,
                                       carl_seek_callback seekfunc,
                                       carl_free_callback freefunc,
                                       void *arg);

/*
 * NAME carl_mime_subparts()
 *
 * DESCRIPTION
 *
 * Set mime part data source from subparts.
 */
CARL_EXTERN CARLcode carl_mime_subparts(carl_mimepart *part,
                                        carl_mime *subparts);
/*
 * NAME carl_mime_headers()
 *
 * DESCRIPTION
 *
 * Set mime part headers.
 */
CARL_EXTERN CARLcode carl_mime_headers(carl_mimepart *part,
                                       struct carl_slist *headers,
                                       int take_ownership);

typedef enum {
  CARLFORM_NOTHING,        /********* the first one is unused ************/
  CARLFORM_COPYNAME,
  CARLFORM_PTRNAME,
  CARLFORM_NAMELENGTH,
  CARLFORM_COPYCONTENTS,
  CARLFORM_PTRCONTENTS,
  CARLFORM_CONTENTSLENGTH,
  CARLFORM_FILECONTENT,
  CARLFORM_ARRAY,
  CARLFORM_OBSOLETE,
  CARLFORM_FILE,

  CARLFORM_BUFFER,
  CARLFORM_BUFFERPTR,
  CARLFORM_BUFFERLENGTH,

  CARLFORM_CONTENTTYPE,
  CARLFORM_CONTENTHEADER,
  CARLFORM_FILENAME,
  CARLFORM_END,
  CARLFORM_OBSOLETE2,

  CARLFORM_STREAM,
  CARLFORM_CONTENTLEN, /* added in 7.46.0, provide a carl_off_t length */

  CARLFORM_LASTENTRY /* the last unused */
} CARLformoption;

/* structure to be used as parameter for CARLFORM_ARRAY */
struct carl_forms {
  CARLformoption option;
  const char     *value;
};

/* use this for multipart formpost building */
/* Returns code for carl_formadd()
 *
 * Returns:
 * CARL_FORMADD_OK             on success
 * CARL_FORMADD_MEMORY         if the FormInfo allocation fails
 * CARL_FORMADD_OPTION_TWICE   if one option is given twice for one Form
 * CARL_FORMADD_NULL           if a null pointer was given for a char
 * CARL_FORMADD_MEMORY         if the allocation of a FormInfo struct failed
 * CARL_FORMADD_UNKNOWN_OPTION if an unknown option was used
 * CARL_FORMADD_INCOMPLETE     if the some FormInfo is not complete (or error)
 * CARL_FORMADD_MEMORY         if a carl_httppost struct cannot be allocated
 * CARL_FORMADD_MEMORY         if some allocation for string copying failed.
 * CARL_FORMADD_ILLEGAL_ARRAY  if an illegal option is used in an array
 *
 ***************************************************************************/
typedef enum {
  CARL_FORMADD_OK, /* first, no error */

  CARL_FORMADD_MEMORY,
  CARL_FORMADD_OPTION_TWICE,
  CARL_FORMADD_NULL,
  CARL_FORMADD_UNKNOWN_OPTION,
  CARL_FORMADD_INCOMPLETE,
  CARL_FORMADD_ILLEGAL_ARRAY,
  CARL_FORMADD_DISABLED, /* libcarl was built with this disabled */

  CARL_FORMADD_LAST /* last */
} CARLFORMcode;

/*
 * NAME carl_formadd()
 *
 * DESCRIPTION
 *
 * Pretty advanced function for building multi-part formposts. Each invoke
 * adds one part that together construct a full post. Then use
 * CARLOPT_HTTPPOST to send it off to libcarl.
 */
CARL_EXTERN CARLFORMcode carl_formadd(struct carl_httppost **httppost,
                                      struct carl_httppost **last_post,
                                      ...);

/*
 * callback function for carl_formget()
 * The void *arg pointer will be the one passed as second argument to
 *   carl_formget().
 * The character buffer passed to it must not be freed.
 * Should return the buffer length passed to it as the argument "len" on
 *   success.
 */
typedef size_t (*carl_formget_callback)(void *arg, const char *buf,
                                        size_t len);

/*
 * NAME carl_formget()
 *
 * DESCRIPTION
 *
 * Serialize a carl_httppost struct built with carl_formadd().
 * Accepts a void pointer as second argument which will be passed to
 * the carl_formget_callback function.
 * Returns 0 on success.
 */
CARL_EXTERN int carl_formget(struct carl_httppost *form, void *arg,
                             carl_formget_callback append);
/*
 * NAME carl_formfree()
 *
 * DESCRIPTION
 *
 * Free a multipart formpost previously built with carl_formadd().
 */
CARL_EXTERN void carl_formfree(struct carl_httppost *form);

/*
 * NAME carl_getenv()
 *
 * DESCRIPTION
 *
 * Returns a malloc()'ed string that MUST be carl_free()ed after usage is
 * complete. DEPRECATED - see lib/README.carlx
 */
CARL_EXTERN char *carl_getenv(const char *variable);

/*
 * NAME carl_version()
 *
 * DESCRIPTION
 *
 * Returns a static ascii string of the libcarl version.
 */
CARL_EXTERN char *carl_version(void);

/*
 * NAME carl_easy_escape()
 *
 * DESCRIPTION
 *
 * Escapes URL strings (converts all letters consider illegal in URLs to their
 * %XX versions). This function returns a new allocated string or NULL if an
 * error occurred.
 */
CARL_EXTERN char *carl_easy_escape(CARL *handle,
                                   const char *string,
                                   int length);

/* the previous version: */
CARL_EXTERN char *carl_escape(const char *string,
                              int length);


/*
 * NAME carl_easy_unescape()
 *
 * DESCRIPTION
 *
 * Unescapes URL encoding in strings (converts all %XX codes to their 8bit
 * versions). This function returns a new allocated string or NULL if an error
 * occurred.
 * Conversion Note: On non-ASCII platforms the ASCII %XX codes are
 * converted into the host encoding.
 */
CARL_EXTERN char *carl_easy_unescape(CARL *handle,
                                     const char *string,
                                     int length,
                                     int *outlength);

/* the previous version */
CARL_EXTERN char *carl_unescape(const char *string,
                                int length);

/*
 * NAME carl_free()
 *
 * DESCRIPTION
 *
 * Provided for de-allocation in the same translation unit that did the
 * allocation. Added in libcarl 7.10
 */
CARL_EXTERN void carl_free(void *p);

/*
 * NAME carl_global_init()
 *
 * DESCRIPTION
 *
 * carl_global_init() should be invoked exactly once for each application that
 * uses libcarl and before any call of other libcarl functions.
 *
 * This function is not thread-safe!
 */
CARL_EXTERN CARLcode carl_global_init(long flags);

/*
 * NAME carl_global_init_mem()
 *
 * DESCRIPTION
 *
 * carl_global_init() or carl_global_init_mem() should be invoked exactly once
 * for each application that uses libcarl.  This function can be used to
 * initialize libcarl and set user defined memory management callback
 * functions.  Users can implement memory management routines to check for
 * memory leaks, check for mis-use of the carl library etc.  User registered
 * callback routines will be invoked by this library instead of the system
 * memory management routines like malloc, free etc.
 */
CARL_EXTERN CARLcode carl_global_init_mem(long flags,
                                          carl_malloc_callback m,
                                          carl_free_callback f,
                                          carl_realloc_callback r,
                                          carl_strdup_callback s,
                                          carl_calloc_callback c);

/*
 * NAME carl_global_cleanup()
 *
 * DESCRIPTION
 *
 * carl_global_cleanup() should be invoked exactly once for each application
 * that uses libcarl
 */
CARL_EXTERN void carl_global_cleanup(void);

/* linked-list structure for the CARLOPT_QUOTE option (and other) */
struct carl_slist {
  char *data;
  struct carl_slist *next;
};

/*
 * NAME carl_global_sslset()
 *
 * DESCRIPTION
 *
 * When built with multiple SSL backends, carl_global_sslset() allows to
 * choose one. This function can only be called once, and it must be called
 * *before* carl_global_init().
 *
 * The backend can be identified by the id (e.g. CARLSSLBACKEND_OPENSSL). The
 * backend can also be specified via the name parameter (passing -1 as id).
 * If both id and name are specified, the name will be ignored. If neither id
 * nor name are specified, the function will fail with
 * CARLSSLSET_UNKNOWN_BACKEND and set the "avail" pointer to the
 * NULL-terminated list of available backends.
 *
 * Upon success, the function returns CARLSSLSET_OK.
 *
 * If the specified SSL backend is not available, the function returns
 * CARLSSLSET_UNKNOWN_BACKEND and sets the "avail" pointer to a NULL-terminated
 * list of available SSL backends.
 *
 * The SSL backend can be set only once. If it has already been set, a
 * subsequent attempt to change it will result in a CARLSSLSET_TOO_LATE.
 */

struct carl_ssl_backend {
  carl_sslbackend id;
  const char *name;
};
typedef struct carl_ssl_backend carl_ssl_backend;

typedef enum {
  CARLSSLSET_OK = 0,
  CARLSSLSET_UNKNOWN_BACKEND,
  CARLSSLSET_TOO_LATE,
  CARLSSLSET_NO_BACKENDS /* libcarl was built without any SSL support */
} CARLsslset;

CARL_EXTERN CARLsslset carl_global_sslset(carl_sslbackend id, const char *name,
                                          const carl_ssl_backend ***avail);

/*
 * NAME carl_slist_append()
 *
 * DESCRIPTION
 *
 * Appends a string to a linked list. If no list exists, it will be created
 * first. Returns the new list, after appending.
 */
CARL_EXTERN struct carl_slist *carl_slist_append(struct carl_slist *,
                                                 const char *);

/*
 * NAME carl_slist_free_all()
 *
 * DESCRIPTION
 *
 * free a previously built carl_slist.
 */
CARL_EXTERN void carl_slist_free_all(struct carl_slist *);

/*
 * NAME carl_getdate()
 *
 * DESCRIPTION
 *
 * Returns the time, in seconds since 1 Jan 1970 of the time string given in
 * the first argument. The time argument in the second parameter is unused
 * and should be set to NULL.
 */
CARL_EXTERN time_t carl_getdate(const char *p, const time_t *unused);

/* info about the certificate chain, only for OpenSSL, GnuTLS, Schannel, NSS
   and GSKit builds. Asked for with CARLOPT_CERTINFO / CARLINFO_CERTINFO */
struct carl_certinfo {
  int num_of_certs;             /* number of certificates with information */
  struct carl_slist **certinfo; /* for each index in this array, there's a
                                   linked list with textual information in the
                                   format "name: value" */
};

/* Information about the SSL library used and the respective internal SSL
   handle, which can be used to obtain further information regarding the
   connection. Asked for with CARLINFO_TLS_SSL_PTR or CARLINFO_TLS_SESSION. */
struct carl_tlssessioninfo {
  carl_sslbackend backend;
  void *internals;
};

#define CARLINFO_STRING   0x100000
#define CARLINFO_LONG     0x200000
#define CARLINFO_DOUBLE   0x300000
#define CARLINFO_SLIST    0x400000
#define CARLINFO_PTR      0x400000 /* same as SLIST */
#define CARLINFO_SOCKET   0x500000
#define CARLINFO_OFF_T    0x600000
#define CARLINFO_MASK     0x0fffff
#define CARLINFO_TYPEMASK 0xf00000

typedef enum {
  CARLINFO_NONE, /* first, never use this */
  CARLINFO_EFFECTIVE_URL    = CARLINFO_STRING + 1,
  CARLINFO_RESPONSE_CODE    = CARLINFO_LONG   + 2,
  CARLINFO_TOTAL_TIME       = CARLINFO_DOUBLE + 3,
  CARLINFO_NAMELOOKUP_TIME  = CARLINFO_DOUBLE + 4,
  CARLINFO_CONNECT_TIME     = CARLINFO_DOUBLE + 5,
  CARLINFO_PRETRANSFER_TIME = CARLINFO_DOUBLE + 6,
  CARLINFO_SIZE_UPLOAD      = CARLINFO_DOUBLE + 7,
  CARLINFO_SIZE_UPLOAD_T    = CARLINFO_OFF_T  + 7,
  CARLINFO_SIZE_DOWNLOAD    = CARLINFO_DOUBLE + 8,
  CARLINFO_SIZE_DOWNLOAD_T  = CARLINFO_OFF_T  + 8,
  CARLINFO_SPEED_DOWNLOAD   = CARLINFO_DOUBLE + 9,
  CARLINFO_SPEED_DOWNLOAD_T = CARLINFO_OFF_T  + 9,
  CARLINFO_SPEED_UPLOAD     = CARLINFO_DOUBLE + 10,
  CARLINFO_SPEED_UPLOAD_T   = CARLINFO_OFF_T  + 10,
  CARLINFO_HEADER_SIZE      = CARLINFO_LONG   + 11,
  CARLINFO_REQUEST_SIZE     = CARLINFO_LONG   + 12,
  CARLINFO_SSL_VERIFYRESULT = CARLINFO_LONG   + 13,
  CARLINFO_FILETIME         = CARLINFO_LONG   + 14,
  CARLINFO_FILETIME_T       = CARLINFO_OFF_T  + 14,
  CARLINFO_CONTENT_LENGTH_DOWNLOAD   = CARLINFO_DOUBLE + 15,
  CARLINFO_CONTENT_LENGTH_DOWNLOAD_T = CARLINFO_OFF_T  + 15,
  CARLINFO_CONTENT_LENGTH_UPLOAD     = CARLINFO_DOUBLE + 16,
  CARLINFO_CONTENT_LENGTH_UPLOAD_T   = CARLINFO_OFF_T  + 16,
  CARLINFO_STARTTRANSFER_TIME = CARLINFO_DOUBLE + 17,
  CARLINFO_CONTENT_TYPE     = CARLINFO_STRING + 18,
  CARLINFO_REDIRECT_TIME    = CARLINFO_DOUBLE + 19,
  CARLINFO_REDIRECT_COUNT   = CARLINFO_LONG   + 20,
  CARLINFO_PRIVATE          = CARLINFO_STRING + 21,
  CARLINFO_HTTP_CONNECTCODE = CARLINFO_LONG   + 22,
  CARLINFO_HTTPAUTH_AVAIL   = CARLINFO_LONG   + 23,
  CARLINFO_PROXYAUTH_AVAIL  = CARLINFO_LONG   + 24,
  CARLINFO_OS_ERRNO         = CARLINFO_LONG   + 25,
  CARLINFO_NUM_CONNECTS     = CARLINFO_LONG   + 26,
  CARLINFO_SSL_ENGINES      = CARLINFO_SLIST  + 27,
  CARLINFO_COOKIELIST       = CARLINFO_SLIST  + 28,
  CARLINFO_LASTSOCKET       = CARLINFO_LONG   + 29,
  CARLINFO_FTP_ENTRY_PATH   = CARLINFO_STRING + 30,
  CARLINFO_REDIRECT_URL     = CARLINFO_STRING + 31,
  CARLINFO_PRIMARY_IP       = CARLINFO_STRING + 32,
  CARLINFO_APPCONNECT_TIME  = CARLINFO_DOUBLE + 33,
  CARLINFO_CERTINFO         = CARLINFO_PTR    + 34,
  CARLINFO_CONDITION_UNMET  = CARLINFO_LONG   + 35,
  CARLINFO_RTSP_SESSION_ID  = CARLINFO_STRING + 36,
  CARLINFO_RTSP_CLIENT_CSEQ = CARLINFO_LONG   + 37,
  CARLINFO_RTSP_SERVER_CSEQ = CARLINFO_LONG   + 38,
  CARLINFO_RTSP_CSEQ_RECV   = CARLINFO_LONG   + 39,
  CARLINFO_PRIMARY_PORT     = CARLINFO_LONG   + 40,
  CARLINFO_LOCAL_IP         = CARLINFO_STRING + 41,
  CARLINFO_LOCAL_PORT       = CARLINFO_LONG   + 42,
  CARLINFO_TLS_SESSION      = CARLINFO_PTR    + 43,
  CARLINFO_ACTIVESOCKET     = CARLINFO_SOCKET + 44,
  CARLINFO_TLS_SSL_PTR      = CARLINFO_PTR    + 45,
  CARLINFO_HTTP_VERSION     = CARLINFO_LONG   + 46,
  CARLINFO_PROXY_SSL_VERIFYRESULT = CARLINFO_LONG + 47,
  CARLINFO_PROTOCOL         = CARLINFO_LONG   + 48,
  CARLINFO_SCHEME           = CARLINFO_STRING + 49,
  CARLINFO_TOTAL_TIME_T     = CARLINFO_OFF_T + 50,
  CARLINFO_NAMELOOKUP_TIME_T = CARLINFO_OFF_T + 51,
  CARLINFO_CONNECT_TIME_T   = CARLINFO_OFF_T + 52,
  CARLINFO_PRETRANSFER_TIME_T = CARLINFO_OFF_T + 53,
  CARLINFO_STARTTRANSFER_TIME_T = CARLINFO_OFF_T + 54,
  CARLINFO_REDIRECT_TIME_T  = CARLINFO_OFF_T + 55,
  CARLINFO_APPCONNECT_TIME_T = CARLINFO_OFF_T + 56,
  CARLINFO_RETRY_AFTER      = CARLINFO_OFF_T + 57,
  CARLINFO_EFFECTIVE_METHOD = CARLINFO_STRING + 58,
  CARLINFO_PROXY_ERROR      = CARLINFO_LONG + 59,

  CARLINFO_LASTONE          = 59
} CARLINFO;

/* CARLINFO_RESPONSE_CODE is the new name for the option previously known as
   CARLINFO_HTTP_CODE */
#define CARLINFO_HTTP_CODE CARLINFO_RESPONSE_CODE

typedef enum {
  CARLCLOSEPOLICY_NONE, /* first, never use this */

  CARLCLOSEPOLICY_OLDEST,
  CARLCLOSEPOLICY_LEAST_RECENTLY_USED,
  CARLCLOSEPOLICY_LEAST_TRAFFIC,
  CARLCLOSEPOLICY_SLOWEST,
  CARLCLOSEPOLICY_CALLBACK,

  CARLCLOSEPOLICY_LAST /* last, never use this */
} carl_closepolicy;

#define CARL_GLOBAL_SSL (1<<0) /* no purpose since since 7.57.0 */
#define CARL_GLOBAL_WIN32 (1<<1)
#define CARL_GLOBAL_ALL (CARL_GLOBAL_SSL|CARL_GLOBAL_WIN32)
#define CARL_GLOBAL_NOTHING 0
#define CARL_GLOBAL_DEFAULT CARL_GLOBAL_ALL
#define CARL_GLOBAL_ACK_EINTR (1<<2)


/*****************************************************************************
 * Setup defines, protos etc for the sharing stuff.
 */

/* Different data locks for a single share */
typedef enum {
  CARL_LOCK_DATA_NONE = 0,
  /*  CARL_LOCK_DATA_SHARE is used internally to say that
   *  the locking is just made to change the internal state of the share
   *  itself.
   */
  CARL_LOCK_DATA_SHARE,
  CARL_LOCK_DATA_COOKIE,
  CARL_LOCK_DATA_DNS,
  CARL_LOCK_DATA_SSL_SESSION,
  CARL_LOCK_DATA_CONNECT,
  CARL_LOCK_DATA_PSL,
  CARL_LOCK_DATA_LAST
} carl_lock_data;

/* Different lock access types */
typedef enum {
  CARL_LOCK_ACCESS_NONE = 0,   /* unspecified action */
  CARL_LOCK_ACCESS_SHARED = 1, /* for read perhaps */
  CARL_LOCK_ACCESS_SINGLE = 2, /* for write perhaps */
  CARL_LOCK_ACCESS_LAST        /* never use */
} carl_lock_access;

typedef void (*carl_lock_function)(CARL *handle,
                                   carl_lock_data data,
                                   carl_lock_access locktype,
                                   void *userptr);
typedef void (*carl_unlock_function)(CARL *handle,
                                     carl_lock_data data,
                                     void *userptr);


typedef enum {
  CARLSHE_OK,  /* all is fine */
  CARLSHE_BAD_OPTION, /* 1 */
  CARLSHE_IN_USE,     /* 2 */
  CARLSHE_INVALID,    /* 3 */
  CARLSHE_NOMEM,      /* 4 out of memory */
  CARLSHE_NOT_BUILT_IN, /* 5 feature not present in lib */
  CARLSHE_LAST        /* never use */
} CARLSHcode;

typedef enum {
  CARLSHOPT_NONE,  /* don't use */
  CARLSHOPT_SHARE,   /* specify a data type to share */
  CARLSHOPT_UNSHARE, /* specify which data type to stop sharing */
  CARLSHOPT_LOCKFUNC,   /* pass in a 'carl_lock_function' pointer */
  CARLSHOPT_UNLOCKFUNC, /* pass in a 'carl_unlock_function' pointer */
  CARLSHOPT_USERDATA,   /* pass in a user data pointer used in the lock/unlock
                           callback functions */
  CARLSHOPT_LAST  /* never use */
} CARLSHoption;

CARL_EXTERN CARLSH *carl_share_init(void);
CARL_EXTERN CARLSHcode carl_share_setopt(CARLSH *, CARLSHoption option, ...);
CARL_EXTERN CARLSHcode carl_share_cleanup(CARLSH *);

/****************************************************************************
 * Structures for querying information about the carl library at runtime.
 */

typedef enum {
  CARLVERSION_FIRST,
  CARLVERSION_SECOND,
  CARLVERSION_THIRD,
  CARLVERSION_FOURTH,
  CARLVERSION_FIFTH,
  CARLVERSION_SIXTH,
  CARLVERSION_SEVENTH,
  CARLVERSION_EIGHTH,
  CARLVERSION_NINTH,
  CARLVERSION_LAST /* never actually use this */
} CARLversion;

/* The 'CARLVERSION_NOW' is the symbolic name meant to be used by
   basically all programs ever that want to get version information. It is
   meant to be a built-in version number for what kind of struct the caller
   expects. If the struct ever changes, we redefine the NOW to another enum
   from above. */
#define CARLVERSION_NOW CARLVERSION_NINTH

struct carl_version_info_data {
  CARLversion age;          /* age of the returned struct */
  const char *version;      /* LIBCARL_VERSION */
  unsigned int version_num; /* LIBCARL_VERSION_NUM */
  const char *host;         /* OS/host/cpu/machine when configured */
  int features;             /* bitmask, see defines below */
  const char *ssl_version;  /* human readable string */
  long ssl_version_num;     /* not used anymore, always 0 */
  const char *libz_version; /* human readable string */
  /* protocols is terminated by an entry with a NULL protoname */
  const char * const *protocols;

  /* The fields below this were added in CARLVERSION_SECOND */
  const char *ares;
  int ares_num;

  /* This field was added in CARLVERSION_THIRD */
  const char *libidn;

  /* These field were added in CARLVERSION_FOURTH */

  /* Same as '_libiconv_version' if built with HAVE_ICONV */
  int iconv_ver_num;

  const char *libssh_version; /* human readable string */

  /* These fields were added in CARLVERSION_FIFTH */
  unsigned int brotli_ver_num; /* Numeric Brotli version
                                  (MAJOR << 24) | (MINOR << 12) | PATCH */
  const char *brotli_version; /* human readable string. */

  /* These fields were added in CARLVERSION_SIXTH */
  unsigned int nghttp2_ver_num; /* Numeric nghttp2 version
                                   (MAJOR << 16) | (MINOR << 8) | PATCH */
  const char *nghttp2_version; /* human readable string. */
  const char *quic_version;    /* human readable quic (+ HTTP/3) library +
                                  version or NULL */

  /* These fields were added in CARLVERSION_SEVENTH */
  const char *cainfo;          /* the built-in default CARLOPT_CAINFO, might
                                  be NULL */
  const char *capath;          /* the built-in default CARLOPT_CAPATH, might
                                  be NULL */

  /* These fields were added in CARLVERSION_EIGHTH */
  unsigned int zstd_ver_num; /* Numeric Zstd version
                                  (MAJOR << 24) | (MINOR << 12) | PATCH */
  const char *zstd_version; /* human readable string. */

  /* These fields were added in CARLVERSION_NINTH */
  const char *hyper_version; /* human readable string. */
};
typedef struct carl_version_info_data carl_version_info_data;

#define CARL_VERSION_IPV6         (1<<0)  /* IPv6-enabled */
#define CARL_VERSION_KERBEROS4    (1<<1)  /* Kerberos V4 auth is supported
                                             (deprecated) */
#define CARL_VERSION_SSL          (1<<2)  /* SSL options are present */
#define CARL_VERSION_LIBZ         (1<<3)  /* libz features are present */
#define CARL_VERSION_NTLM         (1<<4)  /* NTLM auth is supported */
#define CARL_VERSION_GSSNEGOTIATE (1<<5)  /* Negotiate auth is supported
                                             (deprecated) */
#define CARL_VERSION_DEBUG        (1<<6)  /* Built with debug capabilities */
#define CARL_VERSION_ASYNCHDNS    (1<<7)  /* Asynchronous DNS resolves */
#define CARL_VERSION_SPNEGO       (1<<8)  /* SPNEGO auth is supported */
#define CARL_VERSION_LARGEFILE    (1<<9)  /* Supports files larger than 2GB */
#define CARL_VERSION_IDN          (1<<10) /* Internationized Domain Names are
                                             supported */
#define CARL_VERSION_SSPI         (1<<11) /* Built against Windows SSPI */
#define CARL_VERSION_CONV         (1<<12) /* Character conversions supported */
#define CARL_VERSION_CARLDEBUG    (1<<13) /* Debug memory tracking supported */
#define CARL_VERSION_TLSAUTH_SRP  (1<<14) /* TLS-SRP auth is supported */
#define CARL_VERSION_NTLM_WB      (1<<15) /* NTLM delegation to winbind helper
                                             is supported */
#define CARL_VERSION_HTTP2        (1<<16) /* HTTP2 support built-in */
#define CARL_VERSION_GSSAPI       (1<<17) /* Built against a GSS-API library */
#define CARL_VERSION_KERBEROS5    (1<<18) /* Kerberos V5 auth is supported */
#define CARL_VERSION_UNIX_SOCKETS (1<<19) /* Unix domain sockets support */
#define CARL_VERSION_PSL          (1<<20) /* Mozilla's Public Suffix List, used
                                             for cookie domain verification */
#define CARL_VERSION_HTTPS_PROXY  (1<<21) /* HTTPS-proxy support built-in */
#define CARL_VERSION_MULTI_SSL    (1<<22) /* Multiple SSL backends available */
#define CARL_VERSION_BROTLI       (1<<23) /* Brotli features are present. */
#define CARL_VERSION_ALTSVC       (1<<24) /* Alt-Svc handling built-in */
#define CARL_VERSION_HTTP3        (1<<25) /* HTTP3 support built-in */
#define CARL_VERSION_ZSTD         (1<<26) /* zstd features are present */
#define CARL_VERSION_UNICODE      (1<<27) /* Unicode support on Windows */
#define CARL_VERSION_HSTS         (1<<28) /* HSTS is supported */

 /*
 * NAME carl_version_info()
 *
 * DESCRIPTION
 *
 * This function returns a pointer to a static copy of the version info
 * struct. See above.
 */
CARL_EXTERN carl_version_info_data *carl_version_info(CARLversion);

/*
 * NAME carl_easy_strerror()
 *
 * DESCRIPTION
 *
 * The carl_easy_strerror function may be used to turn a CARLcode value
 * into the equivalent human readable error string.  This is useful
 * for printing meaningful error messages.
 */
CARL_EXTERN const char *carl_easy_strerror(CARLcode);

/*
 * NAME carl_share_strerror()
 *
 * DESCRIPTION
 *
 * The carl_share_strerror function may be used to turn a CARLSHcode value
 * into the equivalent human readable error string.  This is useful
 * for printing meaningful error messages.
 */
CARL_EXTERN const char *carl_share_strerror(CARLSHcode);

/*
 * NAME carl_easy_pause()
 *
 * DESCRIPTION
 *
 * The carl_easy_pause function pauses or unpauses transfers. Select the new
 * state by setting the bitmask, use the convenience defines below.
 *
 */
CARL_EXTERN CARLcode carl_easy_pause(CARL *handle, int bitmask);

#define CARLPAUSE_RECV      (1<<0)
#define CARLPAUSE_RECV_CONT (0)

#define CARLPAUSE_SEND      (1<<2)
#define CARLPAUSE_SEND_CONT (0)

#define CARLPAUSE_ALL       (CARLPAUSE_RECV|CARLPAUSE_SEND)
#define CARLPAUSE_CONT      (CARLPAUSE_RECV_CONT|CARLPAUSE_SEND_CONT)

#ifdef  __cplusplus
}
#endif

/* unfortunately, the easy.h and multi.h include files need options and info
  stuff before they can be included! */
#include "easy.h" /* nothing in carl is fun without the easy stuff */
#include "multi.h"
#include "urlapi.h"
#include "options.h"

/* the typechecker doesn't work in C++ (yet) */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && \
    ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && \
    !defined(__cplusplus) && !defined(CARL_DISABLE_TYPECHECK)
#include "typecheck-gcc.h"
#else
#if defined(__STDC__) && (__STDC__ >= 1)
/* This preprocessor magic that replaces a call with the exact same call is
   only done to make sure application authors pass exactly three arguments
   to these functions. */
#define carl_easy_setopt(handle,opt,param) carl_easy_setopt(handle,opt,param)
#define carl_easy_getinfo(handle,info,arg) carl_easy_getinfo(handle,info,arg)
#define carl_share_setopt(share,opt,param) carl_share_setopt(share,opt,param)
#define carl_multi_setopt(handle,opt,param) carl_multi_setopt(handle,opt,param)
#endif /* __STDC__ >= 1 */
#endif /* gcc >= 4.3 && !__cplusplus */

#endif /* CARLINC_CARL_H */
