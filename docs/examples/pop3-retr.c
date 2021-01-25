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
 * POP3 example showing how to retrieve e-mails
 * </DESC>
 */

#include <stdio.h>
#include <carl/carl.h>

/* This is a simple example showing how to retrieve mail using libcarl's POP3
 * capabilities.
 *
 * Note that this example requires libcarl 7.20.0 or above.
 */

int main(void)
{
  CARL *carl;
  CARLcode res = CARLE_OK;

  carl = carl_easy_init();
  if(carl) {
    /* Set username and password */
    carl_easy_setopt(carl, CARLOPT_USERNAME, "user");
    carl_easy_setopt(carl, CARLOPT_PASSWORD, "secret");

    /* This will retrieve message 1 from the user's mailbox */
    carl_easy_setopt(carl, CARLOPT_URL, "pop3://pop.example.com/1");

    /* Perform the retr */
    res = carl_easy_perform(carl);

    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* Always cleanup */
    carl_easy_cleanup(carl);
  }

  return (int)res;
}
