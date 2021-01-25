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

/* Test inspired by github issue 3340 */

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  long carlResponseCode;
  long carlRedirectCount;
  char *effectiveUrl = NULL;
  char *redirectUrl = NULL;

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  test_setopt(carl, CARLOPT_URL, URL);
  /* just to make it explicit and visible in this test: */
  test_setopt(carl, CARLOPT_FOLLOWLOCATION, 0L);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

  carl_easy_getinfo(carl, CARLINFO_RESPONSE_CODE, &carlResponseCode);
  carl_easy_getinfo(carl, CARLINFO_REDIRECT_COUNT, &carlRedirectCount);
  carl_easy_getinfo(carl, CARLINFO_EFFECTIVE_URL, &effectiveUrl);
  carl_easy_getinfo(carl, CARLINFO_REDIRECT_URL, &redirectUrl);

  printf("res: %d\n"
         "status: %d\n"
         "redirects: %d\n"
         "effectiveurl: %s\n"
         "redirecturl: %s\n",
         (int)res,
         (int)carlResponseCode,
         (int)carlRedirectCount,
         effectiveUrl,
         redirectUrl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
