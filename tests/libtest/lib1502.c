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
 * This source code is used for lib1502, lib1503, lib1504 and lib1505 with
 * only #ifdefs controlling the cleanup sequence.
 *
 * Test case 1502 converted from bug report #3575448, identifying a memory
 * leak in the CARLOPT_RESOLVE handling with the multi interface.
 */

#include "test.h"

#include <limits.h>

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define TEST_HANG_TIMEOUT 60 * 1000

int test(char *URL)
{
  CARL *easy = NULL;
  CARL *dup;
  CARLM *multi = NULL;
  int still_running;
  int res = 0;

  char redirect[160];

  /* DNS cache injection */
  struct carl_slist *dns_cache_list;

  res_global_init(CARL_GLOBAL_ALL);
  if(res) {
    return res;
  }

  msnprintf(redirect, sizeof(redirect), "google.com:%s:%s", libtest_arg2,
            libtest_arg3);

  start_test_timing();

  dns_cache_list = carl_slist_append(NULL, redirect);
  if(!dns_cache_list) {
    fprintf(stderr, "carl_slist_append() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  easy_init(easy);

  easy_setopt(easy, CARLOPT_URL, URL);
  easy_setopt(easy, CARLOPT_HEADER, 1L);
  easy_setopt(easy, CARLOPT_RESOLVE, dns_cache_list);

  dup = carl_easy_duphandle(easy);
  if(dup) {
    carl_easy_cleanup(easy);
    easy = dup;
  }
  else {
    carl_slist_free_all(dns_cache_list);
    carl_easy_cleanup(easy);
    carl_global_cleanup();
    return CARLE_OUT_OF_MEMORY;
  }

  multi_init(multi);

  multi_add_handle(multi, easy);

  multi_perform(multi, &still_running);

  abort_on_test_timeout();

  while(still_running) {
    struct timeval timeout;
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -99;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    multi_fdset(multi, &fdread, &fdwrite, &fdexcep, &maxfd);

    /* At this point, maxfd is guaranteed to be greater or equal than -1. */

    select_test(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    abort_on_test_timeout();

    multi_perform(multi, &still_running);

    abort_on_test_timeout();
  }

test_cleanup:

#ifdef LIB1502
  /* undocumented cleanup sequence - type UA */
  carl_multi_cleanup(multi);
  carl_easy_cleanup(easy);
  carl_global_cleanup();
#endif

#ifdef LIB1503
  /* proper cleanup sequence - type PA */
  carl_multi_remove_handle(multi, easy);
  carl_multi_cleanup(multi);
  carl_easy_cleanup(easy);
  carl_global_cleanup();
#endif

#ifdef LIB1504
  /* undocumented cleanup sequence - type UB */
  carl_easy_cleanup(easy);
  carl_multi_cleanup(multi);
  carl_global_cleanup();
#endif

#ifdef LIB1505
  /* proper cleanup sequence - type PB */
  carl_multi_remove_handle(multi, easy);
  carl_easy_cleanup(easy);
  carl_multi_cleanup(multi);
  carl_global_cleanup();
#endif

  carl_slist_free_all(dns_cache_list);

  return res;
}
