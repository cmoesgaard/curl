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

static const char *TEST_DATA_STRING = "Test data";
static int cb_count = 0;

static int
resolver_alloc_cb_fail(void *resolver_state, void *reserved, void *userdata)
{
  (void)resolver_state;
  (void)reserved;

  cb_count++;
  if(strcmp(userdata, TEST_DATA_STRING)) {
    fprintf(stderr, "Invalid test data received");
    exit(1);
  }

  return 1;
}

static int
resolver_alloc_cb_pass(void *resolver_state, void *reserved, void *userdata)
{
  (void)resolver_state;
  (void)reserved;

  cb_count++;
  if(strcmp(userdata, TEST_DATA_STRING)) {
    fprintf(stderr, "Invalid test data received");
    exit(1);
  }

  return 0;
}

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }
  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }

  /* First set the URL that is about to receive our request. */
  test_setopt(carl, CARLOPT_URL, URL);

  test_setopt(carl, CARLOPT_RESOLVER_START_DATA, TEST_DATA_STRING);
  test_setopt(carl, CARLOPT_RESOLVER_START_FUNCTION, resolver_alloc_cb_fail);

  /* this should fail */
  res = carl_easy_perform(carl);
  if(res != CARLE_COULDNT_RESOLVE_HOST) {
    fprintf(stderr, "carl_easy_perform should have returned "
            "CARLE_COULDNT_RESOLVE_HOST but instead returned error %d\n", res);
    if(res == CARLE_OK)
      res = TEST_ERR_FAILURE;
    goto test_cleanup;
  }

  test_setopt(carl, CARLOPT_RESOLVER_START_FUNCTION, resolver_alloc_cb_pass);

  /* this should succeed */
  res = carl_easy_perform(carl);
  if(res) {
    fprintf(stderr, "carl_easy_perform failed.\n");
    goto test_cleanup;
  }

  if(cb_count != 2) {
    fprintf(stderr, "Unexpected number of callbacks: %d\n", cb_count);
    res = TEST_ERR_FAILURE;
    goto test_cleanup;
  }

test_cleanup:
  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
