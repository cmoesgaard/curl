/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#include "warnless.h"
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
  if(carl) {
    carl_easy_setopt(carl, CARLOPT_URL, URL);
#ifdef LIB1917
    /* without any postfields set! */
    carl_easy_setopt(carl, CARLOPT_POST, 1L);
#else
    carl_easy_setopt(carl, CARLOPT_POSTFIELDS, "");
#endif
    res = carl_easy_perform(carl);
    if(res) {
      printf("res: %d\n", res);
    }
    carl_easy_cleanup(carl);
  }
  carl_global_cleanup();
  return (int)res;
}
