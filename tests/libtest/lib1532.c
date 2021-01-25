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

/* Test CARLINFO_RESPONSE_CODE */

int test(char *URL)
{
  CARL *carl;
  long httpcode;
  int res = CARLE_OK;

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);

  res = carl_easy_perform(carl);
  if(res) {
    fprintf(stderr, "%s:%d carl_easy_perform() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }

  res = carl_easy_getinfo(carl, CARLINFO_RESPONSE_CODE, &httpcode);
  if(res) {
    fprintf(stderr, "%s:%d carl_easy_getinfo() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }
  if(httpcode != 200) {
    fprintf(stderr, "%s:%d unexpected response code %ld\n",
            __FILE__, __LINE__, httpcode);
    res = CARLE_HTTP_RETURNED_ERROR;
    goto test_cleanup;
  }

  /* Test for a regression of github bug 1017 (response code does not reset) */
  carl_easy_reset(carl);

  res = carl_easy_getinfo(carl, CARLINFO_RESPONSE_CODE, &httpcode);
  if(res) {
    fprintf(stderr, "%s:%d carl_easy_getinfo() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }
  if(httpcode != 0) {
    fprintf(stderr, "%s:%d carl_easy_reset failed to zero the response code\n"
            "possible regression of github bug 1017\n", __FILE__, __LINE__);
    res = CARLE_HTTP_RETURNED_ERROR;
    goto test_cleanup;
  }

test_cleanup:
  carl_easy_cleanup(carl);
  carl_global_cleanup();
  return res;
}
