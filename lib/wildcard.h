#ifndef HEADER_CARL_WILDCARD_H
#define HEADER_CARL_WILDCARD_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2010 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#ifndef CARL_DISABLE_FTP
#include "llist.h"

/* list of wildcard process states */
typedef enum {
  CARLWC_CLEAR = 0,
  CARLWC_INIT = 1,
  CARLWC_MATCHING, /* library is trying to get list of addresses for
                      downloading */
  CARLWC_DOWNLOADING,
  CARLWC_CLEAN, /* deallocate resources and reset settings */
  CARLWC_SKIP,  /* skip over concrete file */
  CARLWC_ERROR, /* error cases */
  CARLWC_DONE   /* if is wildcard->state == CARLWC_DONE wildcard loop
                   will end */
} wildcard_states;

typedef void (*wildcard_dtor)(void *ptr);

/* struct keeping information about wildcard download process */
struct WildcardData {
  wildcard_states state;
  char *path; /* path to the directory, where we trying wildcard-match */
  char *pattern; /* wildcard pattern */
  struct Curl_llist filelist; /* llist with struct Curl_fileinfo */
  void *protdata; /* pointer to protocol specific temporary data */
  wildcard_dtor dtor;
  void *customptr;  /* for CARLOPT_CHUNK_DATA pointer */
};

CARLcode Curl_wildcard_init(struct WildcardData *wc);
void Curl_wildcard_dtor(struct WildcardData *wc);

struct Curl_easy;

#else
/* FTP is disabled */
#define Curl_wildcard_dtor(x)
#endif

#endif /* HEADER_CARL_WILDCARD_H */
