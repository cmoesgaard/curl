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
 * This example shows an FTP upload, with a rename of the file just after
 * a successful upload.
 *
 * Example based on source code provided by Erick Nuwendam. Thanks!
 */

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  FILE *hd_src;
  int hd;
  struct_stat file_info;
  struct carl_slist *hl;

  struct carl_slist *headerlist = NULL;
  const char *buf_1 = "RNFR 505";
  const char *buf_2 = "RNTO 505-forreal";

  if(!libtest_arg2) {
    fprintf(stderr, "Usage: <url> <file-to-upload>\n");
    return TEST_ERR_USAGE;
  }

  hd_src = fopen(libtest_arg2, "rb");
  if(NULL == hd_src) {
    fprintf(stderr, "fopen failed with error: %d %s\n",
            errno, strerror(errno));
    fprintf(stderr, "Error opening file: %s\n", libtest_arg2);
    return TEST_ERR_MAJOR_BAD; /* if this happens things are major weird */
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

  /* build a list of commands to pass to libcarl */

  hl = carl_slist_append(headerlist, buf_1);
  if(!hl) {
    fprintf(stderr, "carl_slist_append() failed\n");
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    fclose(hd_src);
    return TEST_ERR_MAJOR_BAD;
  }
  headerlist = carl_slist_append(hl, buf_2);
  if(!headerlist) {
    fprintf(stderr, "carl_slist_append() failed\n");
    carl_slist_free_all(hl);
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    fclose(hd_src);
    return TEST_ERR_MAJOR_BAD;
  }
  headerlist = hl;

  /* enable uploading */
  test_setopt(carl, CARLOPT_UPLOAD, 1L);

  /* enable verbose */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* specify target */
  test_setopt(carl, CARLOPT_URL, URL);

  /* pass in that last of FTP commands to run after the transfer */
  test_setopt(carl, CARLOPT_POSTQUOTE, headerlist);

  /* now specify which file to upload */
  test_setopt(carl, CARLOPT_READDATA, hd_src);

  /* and give the size of the upload (optional) */
  test_setopt(carl, CARLOPT_INFILESIZE_LARGE,
                   (carl_off_t)file_info.st_size);

  /* Now run off and do what you've been told! */
  res = carl_easy_perform(carl);

test_cleanup:

  /* clean up the FTP commands list */
  carl_slist_free_all(headerlist);

  /* close the local file */
  fclose(hd_src);

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
