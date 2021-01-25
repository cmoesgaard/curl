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
 * SMTP example showing how to verify an e-mail address
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <carl/carl.h>

/* This is a simple example showing how to verify an e-mail address from an
 * SMTP server.
 *
 * Notes:
 *
 * 1) This example requires libcarl 7.34.0 or above.
 * 2) Not all email servers support this command and even if your email server
 *    does support it, it may respond with a 252 response code even though the
 *    address doesn't exist.
 */

int main(void)
{
  CARL *carl;
  CARLcode res;
  struct carl_slist *recipients = NULL;

  carl = carl_easy_init();
  if(carl) {
    /* This is the URL for your mailserver */
    carl_easy_setopt(carl, CARLOPT_URL, "smtp://mail.example.com");

    /* Note that the CARLOPT_MAIL_RCPT takes a list, not a char array  */
    recipients = carl_slist_append(recipients, "<recipient@example.com>");
    carl_easy_setopt(carl, CARLOPT_MAIL_RCPT, recipients);

    /* Perform the VRFY */
    res = carl_easy_perform(carl);

    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* Free the list of recipients */
    carl_slist_free_all(recipients);

    /* Curl won't send the QUIT command until you call cleanup, so you should
     * be able to re-use this connection for additional requests. It may not be
     * a good idea to keep the connection open for a very long time though
     * (more than a few minutes may result in the server timing out the
     * connection) and you do want to clean up in the end.
     */
    carl_easy_cleanup(carl);
  }

  return 0;
}
