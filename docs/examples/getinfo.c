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
/* <DESC>
 * Use getinfo to get content-type after completed transfer.
 * </DESC>
 */
#include <stdio.h>
#include <carl/carl.h>

int main(void)
{
  CARL *carl;
  CARLcode res;

  carl = carl_easy_init();
  if(carl) {
    carl_easy_setopt(carl, CARLOPT_URL, "https://www.example.com/");
    res = carl_easy_perform(carl);

    if(CARLE_OK == res) {
      char *ct;
      /* ask for the content-type */
      res = carl_easy_getinfo(carl, CARLINFO_CONTENT_TYPE, &ct);

      if((CARLE_OK == res) && ct)
        printf("We received Content-Type: %s\n", ct);
    }

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  return 0;
}
