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

/* Testing Retry-After header parser */

#include "test.h"

#include "memdebug.h"

int test(char *URL)
{
  struct carl_slist *header = NULL;
  carl_off_t retry;
  CARL *carl = NULL;
  int res = 0;

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);

  res = carl_easy_perform(carl);
  if(res)
    goto test_cleanup;

  res = carl_easy_getinfo(carl, CARLINFO_RETRY_AFTER, &retry);
  if(res)
    goto test_cleanup;

#ifdef LIB1596
  /* we get a relative number of seconds, so add the number of seconds
     we're at to make it a somewhat stable number. Then remove accuracy. */
  retry += time(NULL);
  retry /= 10000;
#endif
  printf("Retry-After: %" CARL_FORMAT_CARL_OFF_T "\n", retry);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_slist_free_all(header);
  carl_global_cleanup();

  return res;
}
