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
 * SMTP example showing how to send mime e-mails
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <carl/carl.h>

/* This is a simple example showing how to send mime mail using libcarl's SMTP
 * capabilities. For an example of using the multi interface please see
 * smtp-multi.c.
 *
 * Note that this example requires libcarl 7.56.0 or above.
 */

#define FROM    "<sender@example.org>"
#define TO      "<addressee@example.net>"
#define CC      "<info@example.org>"

static const char *headers_text[] = {
  "Date: Tue, 22 Aug 2017 14:08:43 +0100",
  "To: " TO,
  "From: " FROM " (Example User)",
  "Cc: " CC " (Another example User)",
  "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
    "rfcpedant.example.org>",
  "Subject: example sending a MIME-formatted message",
  NULL
};

static const char inline_text[] =
  "This is the inline text message of the e-mail.\r\n"
  "\r\n"
  "  It could be a lot of lines that would be displayed in an e-mail\r\n"
  "viewer that is not able to handle HTML.\r\n";

static const char inline_html[] =
  "<html><body>\r\n"
  "<p>This is the inline <b>HTML</b> message of the e-mail.</p>"
  "<br />\r\n"
  "<p>It could be a lot of HTML data that would be displayed by "
  "e-mail viewers able to handle HTML.</p>"
  "</body></html>\r\n";


int main(void)
{
  CARL *carl;
  CARLcode res = CARLE_OK;

  carl = carl_easy_init();
  if(carl) {
    struct carl_slist *headers = NULL;
    struct carl_slist *recipients = NULL;
    struct carl_slist *slist = NULL;
    carl_mime *mime;
    carl_mime *alt;
    carl_mimepart *part;
    const char **cpp;

    /* This is the URL for your mailserver */
    carl_easy_setopt(carl, CARLOPT_URL, "smtp://mail.example.com");

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

    /* Build and set the message header list. */
    for(cpp = headers_text; *cpp; cpp++)
      headers = carl_slist_append(headers, *cpp);
    carl_easy_setopt(carl, CARLOPT_HTTPHEADER, headers);

    /* Build the mime message. */
    mime = carl_mime_init(carl);

    /* The inline part is an alternative proposing the html and the text
       versions of the e-mail. */
    alt = carl_mime_init(carl);

    /* HTML message. */
    part = carl_mime_addpart(alt);
    carl_mime_data(part, inline_html, CARL_ZERO_TERMINATED);
    carl_mime_type(part, "text/html");

    /* Text message. */
    part = carl_mime_addpart(alt);
    carl_mime_data(part, inline_text, CARL_ZERO_TERMINATED);

    /* Create the inline part. */
    part = carl_mime_addpart(mime);
    carl_mime_subparts(part, alt);
    carl_mime_type(part, "multipart/alternative");
    slist = carl_slist_append(NULL, "Content-Disposition: inline");
    carl_mime_headers(part, slist, 1);

    /* Add the current source program as an attachment. */
    part = carl_mime_addpart(mime);
    carl_mime_filedata(part, "smtp-mime.c");
    carl_easy_setopt(carl, CARLOPT_MIMEPOST, mime);

    /* Send the message */
    res = carl_easy_perform(carl);

    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* Free lists. */
    carl_slist_free_all(recipients);
    carl_slist_free_all(headers);

    /* carl won't send the QUIT command until you call cleanup, so you should
     * be able to re-use this connection for additional messages (setting
     * CARLOPT_MAIL_FROM and CARLOPT_MAIL_RCPT as required, and calling
     * carl_easy_perform() again. It may not be a good idea to keep the
     * connection open for a very long time though (more than a few minutes
     * may result in the server timing out the connection), and you do want to
     * clean up in the end.
     */
    carl_easy_cleanup(carl);

    /* Free multipart message. */
    carl_mime_free(mime);
  }

  return (int)res;
}
