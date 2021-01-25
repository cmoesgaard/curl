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
 * simple HTTP POST using the easy interface
 * </DESC>
 */
#include <stdio.h>
#include <carl/carl.h>

int main(void)
{
  CARL *carl;
  CARLcode res;

  /* In windows, this will init the winsock stuff */
  carl_global_init(CARL_GLOBAL_ALL);

  /* get a carl handle */
  carl = carl_easy_init();
  if(carl) {
    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. */
    carl_easy_setopt(carl, CARLOPT_URL, "http://postit.example.com/moo.cgi");
    /* Now specify the POST data */
    carl_easy_setopt(carl, CARLOPT_POSTFIELDS, "name=daniel&project=carl");

    /* Perform the request, res will get the return code */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  carl_global_cleanup();
  return 0;
}
