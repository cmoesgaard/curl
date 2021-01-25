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
 * HTTP with Alt-Svc support
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
    carl_easy_setopt(carl, CARLOPT_URL, "https://example.com");

    /* cache the alternatives in this file */
    carl_easy_setopt(carl, CARLOPT_ALTSVC, "altsvc.txt");

    /* restrict which HTTP versions to use alternatives */
    carl_easy_setopt(carl, CARLOPT_ALTSVC_CTRL, (long)
                     CARLALTSVC_H1|CARLALTSVC_H2|CARLALTSVC_H3);

    /* Perform the request, res will get the return code */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  return 0;
}
