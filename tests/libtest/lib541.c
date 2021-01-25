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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "memdebug.h"

/*
 * Two FTP uploads, the second with no content sent.
 */

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  FILE *hd_src;
  int hd;
  struct_stat file_info;

  if(!libtest_arg2) {
    fprintf(stderr, "Usage: <url> <file-to-upload>\n");
    return TEST_ERR_USAGE;
  }

  hd_src = fopen(libtest_arg2, "rb");
  if(NULL == hd_src) {
    fprintf(stderr, "fopen failed with error: %d %s\n",
            errno, strerror(errno));
    fprintf(stderr, "Error opening file: %s\n", libtest_arg2);
    return -2; /* if this happens things are major weird */
  }

  /* get the file size of the local file */
  hd = fstat(fileno(hd_src), &file_info);
  if(hd == -1) {
    /* can't open file, bail out */
    fprintf(stderr, "fstat() failed with error: %d %s\n",
            errno, strerror(errno));
    fprintf(stderr, "ERROR: cannot open file %s\n", libtest_arg2);
    fclose(hd_src);
    return TEST_ERR_MAJOR_BAD;
  }

  if(!file_info.st_size) {
    fprintf(stderr, "ERROR: file %s has zero size!\n", libtest_arg2);
    fclose(hd_src);
    return TEST_ERR_MAJOR_BAD;
  }

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    fclose(hd_src);
    return TEST_ERR_MAJOR_BAD;
  }

  /* get a carl handle */
  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    fclose(hd_src);
    return TEST_ERR_MAJOR_BAD;
  }

  /* enable uploading */
  test_setopt(carl, CARLOPT_UPLOAD, 1L);

  /* enable verbose */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* specify target */
  test_setopt(carl, CARLOPT_URL, URL);

  /* now specify which file to upload */
  test_setopt(carl, CARLOPT_READDATA, hd_src);

  /* Now run off and do what you've been told! */
  carl_easy_perform(carl);

  /* and now upload the exact same again, but without rewinding so it already
     is at end of file */
  res = carl_easy_perform(carl);

test_cleanup:

  /* close the local file */
  fclose(hd_src);

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
