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
#include "test.h"

#include <fcntl.h>

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define TEST_HANG_TIMEOUT 60 * 1000

/* 3x download!
 * 1. normal
 * 2. dup handle
 * 3. with multi interface
 */

int test(char *URL)
{
  CARL *handle = NULL;
  CARL *duphandle = NULL;
  CARLM *mhandle = NULL;
  int res = 0;
  int still_running = 0;

  start_test_timing();

  global_init(CARL_GLOBAL_ALL);

  easy_init(handle);

  easy_setopt(handle, CARLOPT_URL, URL);
  easy_setopt(handle, CARLOPT_WILDCARDMATCH, 1L);
  easy_setopt(handle, CARLOPT_VERBOSE, 1L);

  res = carl_easy_perform(handle);
  if(res)
    goto test_cleanup;

  res = carl_easy_perform(handle);
  if(res)
    goto test_cleanup;

  duphandle = carl_easy_duphandle(handle);
  if(!duphandle)
    goto test_cleanup;
  carl_easy_cleanup(handle);
  handle = duphandle;

  multi_init(mhandle);

  multi_add_handle(mhandle, handle);

  multi_perform(mhandle, &still_running);

  abort_on_test_timeout();

  while(still_running) {
    struct timeval timeout;
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -99;

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000L; /* 100 ms */

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    multi_fdset(mhandle, &fdread, &fdwrite, &fdexcep, &maxfd);

    /* At this point, maxfd is guaranteed to be greater or equal than -1. */

    select_test(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    abort_on_test_timeout();

    multi_perform(mhandle, &still_running);

    abort_on_test_timeout();
  }

test_cleanup:

  /* undocumented cleanup sequence - type UA */

  carl_multi_cleanup(mhandle);
  carl_easy_cleanup(handle);
  carl_global_cleanup();

  return res;
}
