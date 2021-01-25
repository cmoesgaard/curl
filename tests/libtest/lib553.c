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

/* This test case and code is based on the bug recipe Joe Malicki provided for
 * bug report #1871269, fixed on Jan 14 2008 before the 7.18.0 release.
 */

#include "test.h"

#include "memdebug.h"

#define POSTLEN 40960

static size_t myreadfunc(char *ptr, size_t size, size_t nmemb, void *stream)
{
  static size_t total = POSTLEN;
  static char buf[1024];
  (void)stream;

  memset(buf, 'A', sizeof(buf));

  size *= nmemb;
  if(size > total)
    size = total;

  if(size > sizeof(buf))
    size = sizeof(buf);

  memcpy(ptr, buf, size);
  total -= size;
  return size;
}

#define NUM_HEADERS 8
#define SIZE_HEADERS 5000

static char buf[SIZE_HEADERS + 100];

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_FAILED_INIT;
  int i;
  struct carl_slist *headerlist = NULL, *hl;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  for(i = 0; i < NUM_HEADERS; i++) {
    int len = msnprintf(buf, sizeof(buf), "Header%d: ", i);
    memset(&buf[len], 'A', SIZE_HEADERS);
    buf[len + SIZE_HEADERS] = 0; /* null-terminate */
    hl = carl_slist_append(headerlist,  buf);
    if(!hl)
      goto test_cleanup;
    headerlist = hl;
  }

  hl = carl_slist_append(headerlist, "Expect: ");
  if(!hl)
    goto test_cleanup;
  headerlist = hl;

  test_setopt(carl, CARLOPT_URL, URL);
  test_setopt(carl, CARLOPT_HTTPHEADER, headerlist);
  test_setopt(carl, CARLOPT_POST, 1L);
#ifdef CARL_DOES_CONVERSIONS
  /* Convert the POST data to ASCII */
  test_setopt(carl, CARLOPT_TRANSFERTEXT, 1L);
#endif
  test_setopt(carl, CARLOPT_POSTFIELDSIZE, (long)POSTLEN);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);
  test_setopt(carl, CARLOPT_HEADER, 1L);
  test_setopt(carl, CARLOPT_READFUNCTION, myreadfunc);

  res = carl_easy_perform(carl);

test_cleanup:

  carl_easy_cleanup(carl);

  carl_slist_free_all(headerlist);

  carl_global_cleanup();

  return (int)res;
}
