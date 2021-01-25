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
 * POP3 example using SSL
 * </DESC>
 */

#include <stdio.h>
#include <carl/carl.h>

/* This is a simple example showing how to retrieve mail using libcarl's POP3
 * capabilities. It builds on the pop3-retr.c example adding transport
 * security to protect the authentication details from being snooped.
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

    /* This will retrieve message 1 from the user's mailbox. Note the use of
     * pop3s:// rather than pop3:// to request a SSL based connection. */
    carl_easy_setopt(carl, CARLOPT_URL, "pop3s://pop.example.com/1");

    /* If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CARLOPT_CAPATH option might come handy for
     * you. */
#ifdef SKIP_PEER_VERIFICATION
    carl_easy_setopt(carl, CARLOPT_SSL_VERIFYPEER, 0L);
#endif

    /* If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcarl will refuse to connect. You can skip
     * this check, but this will make the connection less secure. */
#ifdef SKIP_HOSTNAME_VERIFICATION
    carl_easy_setopt(carl, CARLOPT_SSL_VERIFYHOST, 0L);
#endif

    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcarl to see what is happening during the
     * transfer */
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

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
