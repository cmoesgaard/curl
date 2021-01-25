#ifndef CARLINC_URLAPI_H
#define CARLINC_URLAPI_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2018 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#include "carl.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* the error codes for the URL API */
typedef enum {
  CARLUE_OK,
  CARLUE_BAD_HANDLE,          /* 1 */
  CARLUE_BAD_PARTPOINTER,     /* 2 */
  CARLUE_MALFORMED_INPUT,     /* 3 */
  CARLUE_BAD_PORT_NUMBER,     /* 4 */
  CARLUE_UNSUPPORTED_SCHEME,  /* 5 */
  CARLUE_URLDECODE,           /* 6 */
  CARLUE_OUT_OF_MEMORY,       /* 7 */
  CARLUE_USER_NOT_ALLOWED,    /* 8 */
  CARLUE_UNKNOWN_PART,        /* 9 */
  CARLUE_NO_SCHEME,           /* 10 */
  CARLUE_NO_USER,             /* 11 */
  CARLUE_NO_PASSWORD,         /* 12 */
  CARLUE_NO_OPTIONS,          /* 13 */
  CARLUE_NO_HOST,             /* 14 */
  CARLUE_NO_PORT,             /* 15 */
  CARLUE_NO_QUERY,            /* 16 */
  CARLUE_NO_FRAGMENT          /* 17 */
} CARLUcode;

typedef enum {
  CARLUPART_URL,
  CARLUPART_SCHEME,
  CARLUPART_USER,
  CARLUPART_PASSWORD,
  CARLUPART_OPTIONS,
  CARLUPART_HOST,
  CARLUPART_PORT,
  CARLUPART_PATH,
  CARLUPART_QUERY,
  CARLUPART_FRAGMENT,
  CARLUPART_ZONEID /* added in 7.65.0 */
} CARLUPart;

#define CARLU_DEFAULT_PORT (1<<0)       /* return default port number */
#define CARLU_NO_DEFAULT_PORT (1<<1)    /* act as if no port number was set,
                                           if the port number matches the
                                           default for the scheme */
#define CARLU_DEFAULT_SCHEME (1<<2)     /* return default scheme if
                                           missing */
#define CARLU_NON_SUPPORT_SCHEME (1<<3) /* allow non-supported scheme */
#define CARLU_PATH_AS_IS (1<<4)         /* leave dot sequences */
#define CARLU_DISALLOW_USER (1<<5)      /* no user+password allowed */
#define CARLU_URLDECODE (1<<6)          /* URL decode on get */
#define CARLU_URLENCODE (1<<7)          /* URL encode on set */
#define CARLU_APPENDQUERY (1<<8)        /* append a form style part */
#define CARLU_GUESS_SCHEME (1<<9)       /* legacy carl-style guessing */
#define CARLU_NO_AUTHORITY (1<<10)      /* Allow empty authority when the
                                           scheme is unknown. */

typedef struct Curl_URL CARLU;

/*
 * carl_url() creates a new CARLU handle and returns a pointer to it.
 * Must be freed with carl_url_cleanup().
 */
CARL_EXTERN CARLU *carl_url(void);

/*
 * carl_url_cleanup() frees the CARLU handle and related resources used for
 * the URL parsing. It will not free strings previously returned with the URL
 * API.
 */
CARL_EXTERN void carl_url_cleanup(CARLU *handle);

/*
 * carl_url_dup() duplicates a CARLU handle and returns a new copy. The new
 * handle must also be freed with carl_url_cleanup().
 */
CARL_EXTERN CARLU *carl_url_dup(CARLU *in);

/*
 * carl_url_get() extracts a specific part of the URL from a CARLU
 * handle. Returns error code. The returned pointer MUST be freed with
 * carl_free() afterwards.
 */
CARL_EXTERN CARLUcode carl_url_get(CARLU *handle, CARLUPart what,
                                   char **part, unsigned int flags);

/*
 * carl_url_set() sets a specific part of the URL in a CARLU handle. Returns
 * error code. The passed in string will be copied. Passing a NULL instead of
 * a part string, clears that part.
 */
CARL_EXTERN CARLUcode carl_url_set(CARLU *handle, CARLUPart what,
                                   const char *part, unsigned int flags);


#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* CARLINC_URLAPI_H */
