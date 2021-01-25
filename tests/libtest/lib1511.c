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

#include "memdebug.h"

int test(char *URL)
{
  long unmet;
  CARL *carl = NULL;
  int res = 0;

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);
  easy_setopt(carl, CARLOPT_HEADER, 1L);
  easy_setopt(carl, CARLOPT_TIMECONDITION, (long)CARL_TIMECOND_IFMODSINCE);

  /* TIMEVALUE in the future */
  easy_setopt(carl, CARLOPT_TIMEVALUE, 1566210680L);

  res = carl_easy_perform(carl);
  if(res)
    goto test_cleanup;

  carl_easy_getinfo(carl, CARLINFO_CONDITION_UNMET, &unmet);
  if(unmet != 1L) {
    res = TEST_ERR_FAILURE; /* not correct */
    goto test_cleanup;
  }

  /* TIMEVALUE in the past */
  easy_setopt(carl, CARLOPT_TIMEVALUE, 1L);

  res = carl_easy_perform(carl);
  if(res)
    goto test_cleanup;

  carl_easy_getinfo(carl, CARLINFO_CONDITION_UNMET, &unmet);
  if(unmet != 0L) {
    res = TEST_ERR_FAILURE; /* not correct */
    goto test_cleanup;
  }

  res = TEST_ERR_SUCCESS; /* this is where we should be */

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
