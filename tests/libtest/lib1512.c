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

/*
 * Use global DNS cache (while deprecated it should still work), populate it
 * with CARLOPT_RESOLVE in the first request and then make sure a subsequent
 * easy transfer finds and uses the populated stuff.
 */

#include "test.h"

#include "memdebug.h"

#define NUM_HANDLES 2

int test(char *URL)
{
  int res = 0;
  CARL *carl[NUM_HANDLES] = {NULL, NULL};
  char *port = libtest_arg3;
  char *address = libtest_arg2;
  char dnsentry[256];
  struct carl_slist *slist = NULL;
  int i;
  char target_url[256];
  (void)URL; /* URL is setup in the code */

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  msnprintf(dnsentry, sizeof(dnsentry), "server.example.carl:%s:%s",
            port, address);
  printf("%s\n", dnsentry);
  slist = carl_slist_append(slist, dnsentry);

  /* get NUM_HANDLES easy handles */
  for(i = 0; i < NUM_HANDLES; i++) {
    /* get an easy handle */
    easy_init(carl[i]);
    /* specify target */
    msnprintf(target_url, sizeof(target_url),
              "http://server.example.carl:%s/path/1512%04i",
              port, i + 1);
    target_url[sizeof(target_url) - 1] = '\0';
    easy_setopt(carl[i], CARLOPT_URL, target_url);
    /* go verbose */
    easy_setopt(carl[i], CARLOPT_VERBOSE, 1L);
    /* include headers */
    easy_setopt(carl[i], CARLOPT_HEADER, 1L);

    easy_setopt(carl[i], CARLOPT_DNS_USE_GLOBAL_CACHE, 1L);
  }

  /* make the first one populate the GLOBAL cache */
  easy_setopt(carl[0], CARLOPT_RESOLVE, slist);

  /* run NUM_HANDLES transfers */
  for(i = 0; (i < NUM_HANDLES) && !res; i++)
    res = carl_easy_perform(carl[i]);

test_cleanup:

  carl_easy_cleanup(carl[0]);
  carl_easy_cleanup(carl[1]);
  carl_slist_free_all(slist);
  carl_global_cleanup();

  return res;
}
