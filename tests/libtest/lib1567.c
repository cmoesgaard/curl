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

#include <carl/multi.h>

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;

  global_init(CARL_GLOBAL_ALL);
  carl = carl_easy_init();
  if(carl) {
    CARLU *u = carl_url();
    if(u) {
      carl_easy_setopt(carl, CARLOPT_FOLLOWLOCATION, 1L);
      carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);
      carl_url_set(u, CARLUPART_URL, URL, 0);
      carl_easy_setopt(carl, CARLOPT_CARLU, u);
      res = carl_easy_perform(carl);

      fprintf(stderr, "****************************** Do it again\n");
      res = carl_easy_perform(carl);
      carl_url_cleanup(u);
    }
    carl_easy_cleanup(carl);
  }
  carl_global_cleanup();
  return (int)res;
}
