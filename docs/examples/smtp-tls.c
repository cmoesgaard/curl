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
 * SMTP example using TLS
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <carl/carl.h>

/* This is a simple example showing how to send mail using libcarl's SMTP
 * capabilities. It builds on the smtp-mail.c example to add authentication
 * and, more importantly, transport security to protect the authentication
 * details from being snooped.
 *
 * Note that this example requires libcarl 7.20.0 or above.
 */

#define FROM    "<sender@example.org>"
#define TO      "<addressee@example.net>"
#define CC      "<info@example.org>"

static const char *payload_text[] = {
  "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
  "To: " TO "\r\n",
  "From: " FROM " (Example User)\r\n",
  "Cc: " CC " (Another example User)\r\n",
  "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
  "rfcpedant.example.org>\r\n",
  "Subject: SMTP TLS example message\r\n",
  "\r\n", /* empty line to divide headers from body, see RFC5322 */
  "The body of the message starts here.\r\n",
  "\r\n",
  "It could be a lot of lines, could be MIME encoded, whatever.\r\n",
  "Check RFC5322.\r\n",
  NULL
};

struct upload_status {
  int lines_read;
};

static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;

  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }

  data = payload_text[upload_ctx->lines_read];

  if(data) {
    size_t len = strlen(data);
    memcpy(ptr, data, len);
    upload_ctx->lines_read++;

    return len;
  }

  return 0;
}

int main(void)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  struct carl_slist *recipients = NULL;
  struct upload_status upload_ctx;

  upload_ctx.lines_read = 0;

  carl = carl_easy_init();
  if(carl) {
    /* Set username and password */
    carl_easy_setopt(carl, CARLOPT_USERNAME, "user");
    carl_easy_setopt(carl, CARLOPT_PASSWORD, "secret");

    /* This is the URL for your mailserver. Note the use of port 587 here,
     * instead of the normal SMTP port (25). Port 587 is commonly used for
     * secure mail submission (see RFC4403), but you should use whatever
     * matches your server configuration. */
    carl_easy_setopt(carl, CARLOPT_URL, "smtp://mainserver.example.net:587");

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
     * That is, in general, a bad idea. It is still better than sending your
     * authentication details in plain text though.  Instead, you should get
     * the issuer certificate (or the host certificate if the certificate is
     * self-signed) and add it to the set of certificates that are known to
     * libcarl using CARLOPT_CAINFO and/or CARLOPT_CAPATH. See docs/SSLCERTS
     * for more information. */
    carl_easy_setopt(carl, CARLOPT_CAINFO, "/path/to/certificate.pem");

    /* Note that this option isn't strictly required, omitting it will result
     * in libcarl sending the MAIL FROM command with empty sender data. All
     * autoresponses should have an empty reverse-path, and should be directed
     * to the address in the reverse-path which triggered them. Otherwise,
     * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
     * details.
     */
    carl_easy_setopt(carl, CARLOPT_MAIL_FROM, FROM);

    /* Add two recipients, in this particular case they correspond to the
     * To: and Cc: addressees in the header, but they could be any kind of
     * recipient. */
    recipients = carl_slist_append(recipients, TO);
    recipients = carl_slist_append(recipients, CC);
    carl_easy_setopt(carl, CARLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CARLOPT_READDATA option to
     * specify a FILE pointer to read from. */
    carl_easy_setopt(carl, CARLOPT_READFUNCTION, payload_source);
    carl_easy_setopt(carl, CARLOPT_READDATA, &upload_ctx);
    carl_easy_setopt(carl, CARLOPT_UPLOAD, 1L);

    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcarl to see what is happening during the transfer.
     */
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

    /* Send the message */
    res = carl_easy_perform(carl);

    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* Free the list of recipients */
    carl_slist_free_all(recipients);

    /* Always cleanup */
    carl_easy_cleanup(carl);
  }

  return (int)res;
}
