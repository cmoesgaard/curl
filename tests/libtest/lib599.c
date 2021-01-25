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

static int progress_callback(void *clientp, double dltotal,
                             double dlnow, double ultotal, double ulnow)
{
  (void)clientp;
  (void)ulnow;
  (void)ultotal;

  if((dltotal > 0.0) && (dlnow > dltotal)) {
    /* this should not happen with test case 599 */
    printf("%.0f > %.0f !!\n", dltotal, dlnow);
    return -1;
  }

  return 0;
}

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  double content_length = 0.0;

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

  /* First set the URL that is about to receive our POST. */
  test_setopt(carl, CARLOPT_URL, URL);

  /* we want to use our own progress function */
  test_setopt(carl, CARLOPT_NOPROGRESS, 0L);
  test_setopt(carl, CARLOPT_PROGRESSFUNCTION, progress_callback);

  /* get verbose debug output please */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* follow redirects */
  test_setopt(carl, CARLOPT_FOLLOWLOCATION, 1L);

  /* include headers in the output */
  test_setopt(carl, CARLOPT_HEADER, 1L);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

  if(!res) {
    FILE *moo;
    res = carl_easy_getinfo(carl, CARLINFO_CONTENT_LENGTH_DOWNLOAD,
                            &content_length);
    moo = fopen(libtest_arg2, "wb");
    if(moo) {
      fprintf(moo, "CL: %.0f\n", content_length);
      fclose(moo);
    }
  }

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
