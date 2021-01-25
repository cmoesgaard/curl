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
 * From "KNOWN_BUGS" April 2009:

 59. If the CARLOPT_PORT option is used on an FTP URL like
 "ftp://example.com/file;type=A" the ";type=A" is stripped off.

 */

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  /* get a carl handle */
  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* enable verbose */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* set port number */
  test_setopt(carl, CARLOPT_PORT, strtol(libtest_arg2, NULL, 10));

  /* specify target */
  test_setopt(carl, CARLOPT_URL, URL);

  /* Now run off and do what you've been told! */
  res = carl_easy_perform(carl);

test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
