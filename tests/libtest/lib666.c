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
  size_t i;

  /* Checks huge binary-encoded mime post. */

  /* Create a buffer with pseudo-binary data. */
  for(i = 0; i < sizeof(buffer); i++)
    if(i % 77 == 76)
      buffer[i] = '\n';
    else
      buffer[i] = (char) (0x41 + i % 26); /* A...Z */

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
  res = carl_mime_name(part, "upfile");
  if(res) {
    fprintf(stderr, "carl_mime_name() failed\n");
    goto test_cleanup;
  }
  res = carl_mime_filename(part, "myfile.txt");
  if(res) {
    fprintf(stderr, "carl_mime_filename() failed\n");
    goto test_cleanup;
  }
  res = carl_mime_data(part, buffer, sizeof(buffer));
  if(res) {
    fprintf(stderr, "carl_mime_data() failed\n");
    goto test_cleanup;
  }
  res = carl_mime_encoder(part, "binary");
  if(res) {
    fprintf(stderr, "carl_mime_encoder() failed\n");
    goto test_cleanup;
  }

  /* First set the URL that is about to receive our mime mail. */
  test_setopt(carl, CARLOPT_URL, URL);

  /* Post form */
  test_setopt(carl, CARLOPT_MIMEPOST, mime);

  /* Shorten upload buffer. */
  test_setopt(carl, CARLOPT_UPLOAD_BUFFERSIZE, 16411L);

  /* get verbose debug output please */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(carl, CARLOPT_HEADER, 1L);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);

  /* now cleanup the mime structure */
  carl_mime_free(mime);

  carl_global_cleanup();

  return res;
}
