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
 * Set working URL with CARLU *.
 * </DESC>
 */
#include <stdio.h>
#include <carl/carl.h>

#if !CARL_AT_LEAST_VERSION(7, 62, 0)
#error "this example requires carl 7.62.0 or later"
#endif

int main(void)
{
  CARL *carl;
  CARLcode res;

  CARLU *urlp;
  CARLUcode uc;

  /* get a carl handle */
  carl = carl_easy_init();

  /* init Curl URL */
  urlp = carl_url();
  uc = carl_url_set(urlp, CARLUPART_URL,
                    "http://example.com/path/index.html", 0);

  if(uc) {
    fprintf(stderr, "carl_url_set() failed: %in", uc);
    goto cleanup;
  }

  if(carl) {
    /* set urlp to use as working URL */
    carl_easy_setopt(carl, CARLOPT_CARLU, urlp);
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    goto cleanup;
  }

  cleanup:
  carl_url_cleanup(urlp);
  carl_easy_cleanup(carl);
  return 0;
}
