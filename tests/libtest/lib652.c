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
#include "test.h"

#include "memdebug.h"

static char buffer[17000]; /* more than 16K */

int test(char *URL)
{
  CARL *carl = NULL;
  CARLcode res = CARLE_OK;
  carl_mime *mime = NULL;
  carl_mimepart *part;
  struct carl_slist *recipients = NULL;

  /* create a buffer with AAAA...BBBBB...CCCC...etc */
  int i;
  int size = (int)sizeof(buffer) / 10;

  for(i = 0; i < size ; i++)
    memset(&buffer[i * 10], 65 + (i % 26), 10);

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    res = (CARLcode) TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }

  /* Build mime structure. */
  mime = carl_mime_init(carl);
  if(!mime) {
    fprintf(stderr, "carl_mime_init() failed\n");
    res = (CARLcode) TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  part = carl_mime_addpart(mime);
  if(!part) {
    fprintf(stderr, "carl_mime_addpart() failed\n");
    res = (CARLcode) TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  res = carl_mime_filename(part, "myfile.jpg");
  if(res) {
    fprintf(stderr, "carl_mime_filename() failed\n");
    goto test_cleanup;
  }
  res = carl_mime_type(part, "image/jpeg");
  if(res) {
    fprintf(stderr, "carl_mime_type() failed\n");
    goto test_cleanup;
  }
  res = carl_mime_data(part, buffer, sizeof(buffer));
  if(res) {
    fprintf(stderr, "carl_mime_data() failed\n");
    goto test_cleanup;
  }
  res = carl_mime_encoder(part, "base64");
  if(res) {
    fprintf(stderr, "carl_mime_encoder() failed\n");
    goto test_cleanup;
  }

  /* Prepare recipients. */
  recipients = carl_slist_append(NULL, "someone@example.com");
  if(!recipients) {
    fprintf(stderr, "carl_slist_append() failed\n");
    goto test_cleanup;
  }

  /* First set the URL that is about to receive our mime mail. */
  test_setopt(carl, CARLOPT_URL, URL);

  /* Set sender. */
  test_setopt(carl, CARLOPT_MAIL_FROM, "somebody@example.com");

  /* Set recipients. */
  test_setopt(carl, CARLOPT_MAIL_RCPT, recipients);

  /* send a multi-part mail */
  test_setopt(carl, CARLOPT_MIMEPOST, mime);

  /* Shorten upload buffer. */
  test_setopt(carl, CARLOPT_UPLOAD_BUFFERSIZE, 16411L);

  /* get verbose debug output please */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);

  /* now cleanup the mime structure */
  carl_mime_free(mime);

  /* cleanup the recipients. */
  carl_slist_free_all(recipients);

  carl_global_cleanup();

  return res;
}
