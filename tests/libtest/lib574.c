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

static int new_fnmatch(void *ptr,
                       const char *pattern, const char *string)
{
  (void)ptr;
  (void)pattern;
  (void)string;
  return CARL_FNMATCHFUNC_MATCH;
}

int test(char *URL)
{
  int res;
  CARL *carl;

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

  test_setopt(carl, CARLOPT_URL, URL);
  test_setopt(carl, CARLOPT_WILDCARDMATCH, 1L);
  test_setopt(carl, CARLOPT_FNMATCH_FUNCTION, new_fnmatch);

  res = carl_easy_perform(carl);
  if(res) {
    fprintf(stderr, "carl_easy_perform() failed %d\n", res);
    goto test_cleanup;
  }
  res = carl_easy_perform(carl);
  if(res) {
    fprintf(stderr, "carl_easy_perform() failed %d\n", res);
    goto test_cleanup;
  }

test_cleanup:
  carl_easy_cleanup(carl);
  carl_global_cleanup();
  return res;
}
