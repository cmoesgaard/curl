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

/* The size of data should be kept below MAX_INITIAL_POST_SIZE! */
static char data[]="this is a short string.\n";

static size_t data_size = sizeof(data) / sizeof(char);

static int progress_callback(void *clientp, double dltotal, double dlnow,
                             double ultotal, double ulnow)
{
  FILE *moo = fopen(libtest_arg2, "wb");

  (void)clientp; /* UNUSED */
  (void)dltotal; /* UNUSED */
  (void)dlnow; /* UNUSED */

  if(moo) {
    if((size_t)ultotal == data_size && (size_t)ulnow == data_size)
      fprintf(moo, "PASSED, UL data matched data size\n");
    else
      fprintf(moo, "Progress callback called with UL %f out of %f\n",
              ulnow, ultotal);
    fclose(moo);
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
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* First set the URL that is about to receive our POST. */
  test_setopt(carl, CARLOPT_URL, URL);

  /* Now specify we want to POST data */
  test_setopt(carl, CARLOPT_POST, 1L);

#ifdef CARL_DOES_CONVERSIONS
  /* Convert the POST data to ASCII */
  test_setopt(carl, CARLOPT_TRANSFERTEXT, 1L);
#endif

  /* Set the expected POST size */
  test_setopt(carl, CARLOPT_POSTFIELDSIZE, (long)data_size);
  test_setopt(carl, CARLOPT_POSTFIELDS, data);

  /* we want to use our own progress function */
  test_setopt(carl, CARLOPT_NOPROGRESS, 0L);
  test_setopt(carl, CARLOPT_PROGRESSFUNCTION, progress_callback);

  /* pointer to pass to our read function */

  /* get verbose debug output please */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(carl, CARLOPT_HEADER, 1L);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
