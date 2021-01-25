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

/* Test CARLINFO_SCHEME */

int test(char *URL)
{
  CARL *carl, *dupe = NULL;
  char *scheme;
  int res = CARLE_OK;

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  /* Test that scheme is properly initialized on carl_easy_init.
  */

  res = carl_easy_getinfo(carl, CARLINFO_SCHEME, &scheme);
  if(res) {
    fprintf(stderr, "%s:%d carl_easy_getinfo() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }
  if(scheme != NULL) {
    fprintf(stderr, "%s:%d scheme init failed; expected NULL\n",
            __FILE__, __LINE__);
    res = CARLE_FAILED_INIT;
    goto test_cleanup;
  }

  easy_setopt(carl, CARLOPT_URL, URL);

  res = carl_easy_perform(carl);
  if(res) {
    fprintf(stderr, "%s:%d carl_easy_perform() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }

  /* Test that a scheme is properly set after receiving an HTTP resource.
  */

  res = carl_easy_getinfo(carl, CARLINFO_SCHEME, &scheme);
  if(res) {
    fprintf(stderr, "%s:%d carl_easy_getinfo() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }
  if(memcmp(scheme, "HTTP", 5) != 0) {
    fprintf(stderr, "%s:%d scheme of http resource is incorrect; "
            "expected 'HTTP' but is %s\n",
            __FILE__, __LINE__,
            (scheme == NULL ? "NULL" : "invalid"));
    res = CARLE_HTTP_RETURNED_ERROR;
    goto test_cleanup;
  }

  /* Test that a scheme is properly initialized on carl_easy_duphandle.
  */

  dupe = carl_easy_duphandle(carl);
  if(!dupe) {
    fprintf(stderr, "%s:%d carl_easy_duphandle() failed\n",
            __FILE__, __LINE__);
    res = CARLE_FAILED_INIT;
    goto test_cleanup;
  }

  res = carl_easy_getinfo(dupe, CARLINFO_SCHEME, &scheme);
  if(res) {
    fprintf(stderr, "%s:%d carl_easy_getinfo() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }
  if(scheme != 0) {
    fprintf(stderr, "%s:%d scheme init failed; expected NULL\n",
            __FILE__, __LINE__);
    res = CARLE_FAILED_INIT;
    goto test_cleanup;
  }


  /* Test that a scheme is properly initialized on carl_easy_reset.
  */

  carl_easy_reset(carl);

  res = carl_easy_getinfo(carl, CARLINFO_SCHEME, &scheme);
  if(res) {
    fprintf(stderr, "%s:%d carl_easy_getinfo() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }
  if(scheme != 0) {
    fprintf(stderr, "%s:%d scheme init failed; expected NULL\n",
            __FILE__, __LINE__);
    res = CARLE_FAILED_INIT;
    goto test_cleanup;
  }

test_cleanup:
  carl_easy_cleanup(carl);
  carl_easy_cleanup(dupe);
  carl_global_cleanup();
  return res;
}
