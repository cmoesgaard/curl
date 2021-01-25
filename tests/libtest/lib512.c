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

/* Test case code based on source in a bug report filed by James Bursa on
   28 Apr 2004 */

int test(char *URL)
{
  CARLcode code;
  int rc = 99;

  code = carl_global_init(CARL_GLOBAL_ALL);
  if(code == CARLE_OK) {
    CARL *carl = carl_easy_init();
    if(carl) {
      CARL *carl2;

      carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);
      carl_easy_setopt(carl, CARLOPT_HEADER, 1L);

      carl2 = carl_easy_duphandle(carl);
      if(carl2) {

        code = carl_easy_setopt(carl2, CARLOPT_URL, URL);
        if(code == CARLE_OK) {

          code = carl_easy_perform(carl2);
          if(code == CARLE_OK)
            rc = 0;
          else
            rc = 1;
        }
        else
          rc = 2;

        carl_easy_cleanup(carl2);
      }
      else
        rc = 3;

      carl_easy_cleanup(carl);
    }
    else
      rc = 4;

    carl_global_cleanup();
  }
  else
    rc = 5;

  return rc;
}
