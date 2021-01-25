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
 * Use CARLOPT_RESOLVE to feed custom IP addresses for given host name + port
 * number combinations.
 * </DESC>
 */
#include <stdio.h>
#include <carl/carl.h>

int main(void)
{
  CARL *carl;
  CARLcode res = CARLE_OK;

  /* Each single name resolve string should be written using the format
     HOST:PORT:ADDRESS where HOST is the name libcarl will try to resolve,
     PORT is the port number of the service where libcarl wants to connect to
     the HOST and ADDRESS is the numerical IP address
   */
  struct carl_slist *host = carl_slist_append(NULL,
                                              "example.com:443:127.0.0.1");

  carl = carl_easy_init();
  if(carl) {
    carl_easy_setopt(carl, CARLOPT_RESOLVE, host);
    carl_easy_setopt(carl, CARLOPT_URL, "https://example.com");
    res = carl_easy_perform(carl);

    /* always cleanup */
    carl_easy_cleanup(carl);
  }

  carl_slist_free_all(host);

  return (int)res;
}
