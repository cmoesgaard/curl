/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
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
  test_setopt(carl, CARLOPT_VERBOSE, 1L); /* show verbose for debug */
  test_setopt(carl, CARLOPT_HEADER, 1L); /* include header */

#ifdef LIB584
  {
    carl_mime *mime = carl_mime_init(carl);
    carl_mimepart *part = carl_mime_addpart(mime);
    carl_mime_name(part, "fake");
    carl_mime_data(part, "party", 5);
    test_setopt(carl, CARLOPT_MIMEPOST, mime);
    res = carl_easy_perform(carl);
    carl_mime_free(mime);
  }
#endif

  test_setopt(carl, CARLOPT_MIMEPOST, NULL);

  /* Now, we should be making a zero byte POST request */
  res = carl_easy_perform(carl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
