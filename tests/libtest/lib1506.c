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

#define NUM_HANDLES 4

int test(char *URL)
{
  int res = 0;
  CARL *carl[NUM_HANDLES] = {0};
  int running;
  CARLM *m = NULL;
  int i;
  char target_url[256];
  char dnsentry[256];
  struct carl_slist *slist = NULL, *slist2;
  char *port = libtest_arg3;
  char *address = libtest_arg2;

  (void)URL;

  /* Create fake DNS entries for serverX.example.com for all handles */
  for(i = 0; i < NUM_HANDLES; i++) {
    msnprintf(dnsentry, sizeof(dnsentry), "server%d.example.com:%s:%s",
              i + 1, port, address);
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

  multi_init(m);

  multi_setopt(m, CARLMOPT_MAXCONNECTS, 3L);

  /* get NUM_HANDLES easy handles */
  for(i = 0; i < NUM_HANDLES; i++) {
    /* get an easy handle */
    easy_init(carl[i]);
    /* specify target */
    msnprintf(target_url, sizeof(target_url),
              "http://server%d.example.com:%s/path/1506%04i",
              i + 1, port, i + 1);
    target_url[sizeof(target_url) - 1] = '\0';
    easy_setopt(carl[i], CARLOPT_URL, target_url);
    /* go verbose */
    easy_setopt(carl[i], CARLOPT_VERBOSE, 1L);
    /* include headers */
    easy_setopt(carl[i], CARLOPT_HEADER, 1L);

    easy_setopt(carl[i], CARLOPT_RESOLVE, slist);
  }

  fprintf(stderr, "Start at URL 0\n");

  for(i = 0; i < NUM_HANDLES; i++) {
    /* add handle to multi */
    multi_add_handle(m, carl[i]);

    for(;;) {
      struct timeval interval;
      fd_set rd, wr, exc;
      int maxfd = -99;

      interval.tv_sec = 1;
      interval.tv_usec = 0;

      multi_perform(m, &running);

      abort_on_test_timeout();

      if(!running)
        break; /* done */

      FD_ZERO(&rd);
      FD_ZERO(&wr);
      FD_ZERO(&exc);

      multi_fdset(m, &rd, &wr, &exc, &maxfd);

      /* At this point, maxfd is guaranteed to be greater or equal than -1. */

      select_test(maxfd + 1, &rd, &wr, &exc, &interval);

      abort_on_test_timeout();
    }
    wait_ms(1); /* to ensure different end times */
  }

test_cleanup:

  /* proper cleanup sequence - type PB */

  for(i = 0; i < NUM_HANDLES; i++) {
    carl_multi_remove_handle(m, carl[i]);
    carl_easy_cleanup(carl[i]);
  }

  carl_slist_free_all(slist);

  carl_multi_cleanup(m);
  carl_global_cleanup();

  return res;
}
