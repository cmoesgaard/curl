/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2013 - 2020, Linus Nielsen Feltzing <linus@haxx.se>
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
#include "test.h"

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define TEST_HANG_TIMEOUT 60 * 1000

#define NUM_URLS 4

int test(char *URL)
{
  int res = 0;
  CARL *carl = NULL;
  int i;
  char target_url[256];
  char dnsentry[256];
  struct carl_slist *slist = NULL, *slist2;
  char *port = libtest_arg3;
  char *address = libtest_arg2;

  (void)URL;

  /* Create fake DNS entries for serverX.example.com for all handles */
  for(i = 0; i < NUM_URLS; i++) {
    msnprintf(dnsentry, sizeof(dnsentry), "server%d.example.com:%s:%s", i + 1,
              port, address);
    printf("%s\n", dnsentry);
    slist2 = carl_slist_append(slist, dnsentry);
    if(!slist2) {
      fprintf(stderr, "carl_slist_append() failed\n");
      goto test_cleanup;
    }
    slist = slist2;
  }

  start_test_timing();

  global_init(CARL_GLOBAL_ALL);

  /* get an easy handle */
  easy_init(carl);

  /* go verbose */
  easy_setopt(carl, CARLOPT_VERBOSE, 1L);
  /* include headers */
  easy_setopt(carl, CARLOPT_HEADER, 1L);

  easy_setopt(carl, CARLOPT_RESOLVE, slist);

  easy_setopt(carl, CARLOPT_MAXCONNECTS, 3L);

  /* get NUM_HANDLES easy handles */
  for(i = 0; i < NUM_URLS; i++) {
    /* specify target */
    msnprintf(target_url, sizeof(target_url),
              "http://server%d.example.com:%s/path/1510%04i",
              i + 1, port, i + 1);
    target_url[sizeof(target_url) - 1] = '\0';
    easy_setopt(carl, CARLOPT_URL, target_url);

    res = carl_easy_perform(carl);

    abort_on_test_timeout();
  }

test_cleanup:

  /* proper cleanup sequence - type PB */

  carl_easy_cleanup(carl);

  carl_slist_free_all(slist);

  carl_global_cleanup();

  return res;
}
