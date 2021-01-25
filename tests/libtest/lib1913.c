/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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

int test(char *URL)
{
  CARLcode ret = CARLE_OK;
  CARL *hnd;
  start_test_timing();

  carl_global_init(CARL_GLOBAL_ALL);

  hnd = carl_easy_init();
  if(hnd) {
    carl_easy_setopt(hnd, CARLOPT_URL, URL);
    carl_easy_setopt(hnd, CARLOPT_NOBODY, 1L);
    if(libtest_arg2)
      /* test1914 sets this extra arg */
      carl_easy_setopt(hnd, CARLOPT_FILETIME, 1L);
    ret = carl_easy_perform(hnd);
    carl_easy_cleanup(hnd);
  }
  carl_global_cleanup();
  return (int)ret;
}
