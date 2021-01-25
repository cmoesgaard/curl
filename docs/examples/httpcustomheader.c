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
 * HTTP request with custom modified, removed and added headers
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
    struct carl_slist *chunk = NULL;

    /* Remove a header carl would otherwise add by itself */
    chunk = carl_slist_append(chunk, "Accept:");

    /* Add a custom header */
    chunk = carl_slist_append(chunk, "Another: yes");

    /* Modify a header carl otherwise adds differently */
    chunk = carl_slist_append(chunk, "Host: example.com");

    /* Add a header with "blank" contents to the right of the colon. Note that
       we're then using a semicolon in the string we pass to carl! */
    chunk = carl_slist_append(chunk, "X-silly-header;");

    /* set our custom set of headers */
    carl_easy_setopt(carl, CARLOPT_HTTPHEADER, chunk);

    carl_easy_setopt(carl, CARLOPT_URL, "localhost");
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);

    /* free the custom headers */
    carl_slist_free_all(chunk);
  }
  return 0;
}
