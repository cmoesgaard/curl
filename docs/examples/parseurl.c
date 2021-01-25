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
 * Basic URL API use.
 * </DESC>
 */
#include <stdio.h>
#include <carl/carl.h>

#if !CARL_AT_LEAST_VERSION(7, 62, 0)
#error "this example requires carl 7.62.0 or later"
#endif

int main(void)
{
  CARLU *h;
  CARLUcode uc;
  char *host;
  char *path;

  h = carl_url(); /* get a handle to work with */
  if(!h)
    return 1;

  /* parse a full URL */
  uc = carl_url_set(h, CARLUPART_URL, "http://example.com/path/index.html", 0);
  if(uc)
    goto fail;

  /* extract host name from the parsed URL */
  uc = carl_url_get(h, CARLUPART_HOST, &host, 0);
  if(!uc) {
    printf("Host name: %s\n", host);
    carl_free(host);
  }

  /* extract the path from the parsed URL */
  uc = carl_url_get(h, CARLUPART_PATH, &path, 0);
  if(!uc) {
    printf("Path: %s\n", path);
    carl_free(path);
  }

  /* redirect with a relative URL */
  uc = carl_url_set(h, CARLUPART_URL, "../another/second.html", 0);
  if(uc)
    goto fail;

  /* extract the new, updated path */
  uc = carl_url_get(h, CARLUPART_PATH, &path, 0);
  if(!uc) {
    printf("Path: %s\n", path);
    carl_free(path);
  }

  fail:
  carl_url_cleanup(h); /* free url handle */
  return 0;
}
