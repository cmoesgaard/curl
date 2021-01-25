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
 * Send e-mail with SMTP
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <carl/carl.h>

/*
 * For an SMTP example using the multi interface please see smtp-multi.c.
 */

/* The libcarl options want plain addresses, the viewable headers in the mail
 * can very well get a full name as well.
 */
#define FROM_ADDR    "<sender@example.org>"
#define TO_ADDR      "<addressee@example.net>"
#define CC_ADDR      "<info@example.org>"

#define FROM_MAIL "Sender Person " FROM_ADDR
#define TO_MAIL   "A Receiver " TO_ADDR
#define CC_MAIL   "John CC Smith " CC_ADDR

static const char *payload_text[] = {
  "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
  "To: " TO_MAIL "\r\n",
  "From: " FROM_MAIL "\r\n",
  "Cc: " CC_MAIL "\r\n",
  "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
  "rfcpedant.example.org>\r\n",
  "Subject: SMTP example message\r\n",
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
    /* This is the URL for your mailserver */
    carl_easy_setopt(carl, CARLOPT_URL, "smtp://mail.example.com");

    /* Note that this option isn't strictly required, omitting it will result
     * in libcarl sending the MAIL FROM command with empty sender data. All
     * autoresponses should have an empty reverse-path, and should be directed
     * to the address in the reverse-path which triggered them. Otherwise,
     * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
     * details.
     */
    carl_easy_setopt(carl, CARLOPT_MAIL_FROM, FROM_ADDR);

    /* Add two recipients, in this particular case they correspond to the
     * To: and Cc: addressees in the header, but they could be any kind of
     * recipient. */
    recipients = carl_slist_append(recipients, TO_ADDR);
    recipients = carl_slist_append(recipients, CC_ADDR);
    carl_easy_setopt(carl, CARLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CARLOPT_READDATA option to
     * specify a FILE pointer to read from. */
    carl_easy_setopt(carl, CARLOPT_READFUNCTION, payload_source);
    carl_easy_setopt(carl, CARLOPT_READDATA, &upload_ctx);
    carl_easy_setopt(carl, CARLOPT_UPLOAD, 1L);

    /* Send the message */
    res = carl_easy_perform(carl);

    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* Free the list of recipients */
    carl_slist_free_all(recipients);

    /* carl won't send the QUIT command until you call cleanup, so you should
     * be able to re-use this connection for additional messages (setting
     * CARLOPT_MAIL_FROM and CARLOPT_MAIL_RCPT as required, and calling
     * carl_easy_perform() again. It may not be a good idea to keep the
     * connection open for a very long time though (more than a few minutes
     * may result in the server timing out the connection), and you do want to
     * clean up in the end.
     */
    carl_easy_cleanup(carl);
  }

  return (int)res;
}
