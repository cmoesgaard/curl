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
 * IMAP example using TLS
 * </DESC>
 */

#include <stdio.h>
#include <carl/carl.h>

/* This is a simple example showing how to fetch mail using libcarl's IMAP
 * capabilities. It builds on the imap-fetch.c example adding transport
 * security to protect the authentication details from being snooped.
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

    /* This will fetch message 1 from the user's inbox */
    carl_easy_setopt(carl, CARLOPT_URL,
                     "imap://imap.example.com/INBOX/;UID=1");

    /* In this example, we'll start with a plain text connection, and upgrade
     * to Transport Layer Security (TLS) using the STARTTLS command. Be careful
     * of using CARLUSESSL_TRY here, because if TLS upgrade fails, the transfer
     * will continue anyway - see the security discussion in the libcarl
     * tutorial for more details. */
    carl_easy_setopt(carl, CARLOPT_USE_SSL, (long)CARLUSESSL_ALL);

    /* If your server doesn't have a valid certificate, then you can disable
     * part of the Transport Layer Security protection by setting the
     * CARLOPT_SSL_VERIFYPEER and CARLOPT_SSL_VERIFYHOST options to 0 (false).
     *   carl_easy_setopt(carl, CARLOPT_SSL_VERIFYPEER, 0L);
     *   carl_easy_setopt(carl, CARLOPT_SSL_VERIFYHOST, 0L);
     *
     * That is, in general, a bad idea. It is still better than sending your
     * authentication details in plain text though.  Instead, you should get
     * the issuer certificate (or the host certificate if the certificate is
     * self-signed) and add it to the set of certificates that are known to
     * libcarl using CARLOPT_CAINFO and/or CARLOPT_CAPATH. See docs/SSLCERTS
     * for more information. */
    carl_easy_setopt(carl, CARLOPT_CAINFO, "/path/to/certificate.pem");

    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcarl to see what is happening during the
     * transfer */
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

    /* Perform the fetch */
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
