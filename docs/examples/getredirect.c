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
 * Show how to extract Location: header and URL to redirect to.
 * </DESC>
 */
#include <stdio.h>
#include <carl/carl.h>

int main(void)
{
  CARL *carl;
  CARLcode res;
  char *location;
  long response_code;

  carl = carl_easy_init();
  if(carl) {
    carl_easy_setopt(carl, CARLOPT_URL, "https://example.com");

    /* example.com is redirected, figure out the redirection! */

    /* Perform the request, res will get the return code */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));
    else {
      res = carl_easy_getinfo(carl, CARLINFO_RESPONSE_CODE, &response_code);
      if((res == CARLE_OK) &&
         ((response_code / 100) != 3)) {
        /* a redirect implies a 3xx response code */
        fprintf(stderr, "Not a redirect.\n");
      }
      else {
        res = carl_easy_getinfo(carl, CARLINFO_REDIRECT_URL, &location);

        if((res == CARLE_OK) && location) {
          /* This is the new absolute URL that you could redirect to, even if
           * the Location: response header may have been a relative URL. */
          printf("Redirected to: %s\n", location);
        }
      }
    }

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  return 0;
}
