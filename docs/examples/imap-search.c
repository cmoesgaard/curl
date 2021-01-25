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
 * IMAP example showing how to search for new e-mails
 * </DESC>
 */

#include <stdio.h>
#include <carl/carl.h>

/* This is a simple example showing how to search for new messages using
 * libcarl's IMAP capabilities.
 *
 * Note that this example requires libcarl 7.30.0 or above.
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

    /* This is mailbox folder to select */
    carl_easy_setopt(carl, CARLOPT_URL, "imap://imap.example.com/INBOX");

    /* Set the SEARCH command specifying what we want to search for. Note that
     * this can contain a message sequence set and a number of search criteria
     * keywords including flags such as ANSWERED, DELETED, DRAFT, FLAGGED, NEW,
     * RECENT and SEEN. For more information about the search criteria please
     * see RFC-3501 section 6.4.4.   */
    carl_easy_setopt(carl, CARLOPT_CUSTOMREQUEST, "SEARCH NEW");

    /* Perform the custom request */
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
