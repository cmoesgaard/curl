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

/* Test suppressing the If-Modified-Since header */

#include "test.h"

#include "memdebug.h"

int test(char *URL)
{
  struct carl_slist *header = NULL;
  long unmet;
  CARL *carl = NULL;
  int res = 0;

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);
  easy_setopt(carl, CARLOPT_TIMECONDITION, (long)CARL_TIMECOND_IFMODSINCE);
  /* Some TIMEVALUE; it doesn't matter. */
  easy_setopt(carl, CARLOPT_TIMEVALUE, 1566210680L);

  header = carl_slist_append(NULL, "If-Modified-Since:");
  if(!header) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }

  easy_setopt(carl, CARLOPT_HTTPHEADER, header);

  res = carl_easy_perform(carl);
  if(res)
    goto test_cleanup;

  /* Confirm that the condition checking still worked, even though we
   * suppressed the actual header.
   * The server returns 304, which means the condition is "unmet".
   */

  res = carl_easy_getinfo(carl, CARLINFO_CONDITION_UNMET, &unmet);
  if(res)
    goto test_cleanup;

  if(unmet != 1L) {
    res = TEST_ERR_FAILURE;
    goto test_cleanup;
  }

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_slist_free_all(header);
  carl_global_cleanup();

  return res;
}
