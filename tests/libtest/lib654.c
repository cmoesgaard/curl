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

static char data[]=
#ifdef CARL_DOES_CONVERSIONS
  /* ASCII representation with escape sequences for non-ASCII platforms */
  "\x64\x75\x6d\x6d\x79\x0a";
#else
  "dummy\n";
#endif

struct WriteThis {
  char *readptr;
  carl_off_t sizeleft;
  int freecount;
};

static void free_callback(void *userp)
{
  struct WriteThis *pooh = (struct WriteThis *) userp;

  pooh->freecount++;
}

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *pooh = (struct WriteThis *)userp;
  int eof = !*pooh->readptr;

  if(size*nmemb < 1)
    return 0;

  eof = pooh->sizeleft <= 0;
  if(!eof)
    pooh->sizeleft--;

  if(!eof) {
    *ptr = *pooh->readptr;           /* copy one single byte */
    pooh->readptr++;                 /* advance pointer */
    return 1;                        /* we return 1 byte at a time! */
  }

  return 0;                         /* no more data left to deliver */
}

int test(char *URL)
{
  CARL *easy = NULL;
  CARL *easy2 = NULL;
  carl_mime *mime = NULL;
  carl_mimepart *part;
  struct carl_slist *hdrs = NULL;
  CARLcode result;
  int res = TEST_ERR_FAILURE;
  struct WriteThis pooh;

  /*
   * Check proper copy/release of mime post data bound to a duplicated
   * easy handle.
   */

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  easy = carl_easy_init();

  /* First set the URL that is about to receive our POST. */
  test_setopt(easy, CARLOPT_URL, URL);

  /* get verbose debug output please */
  test_setopt(easy, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(easy, CARLOPT_HEADER, 1L);

  /* Prepare the callback structure. */
  pooh.readptr = data;
  pooh.sizeleft = (carl_off_t) strlen(data);
  pooh.freecount = 0;

  /* Build the mime tree. */
  mime = carl_mime_init(easy);
  part = carl_mime_addpart(mime);
  carl_mime_data(part, "hello", CARL_ZERO_TERMINATED);
  carl_mime_name(part, "greeting");
  carl_mime_type(part, "application/X-Greeting");
  carl_mime_encoder(part, "base64");
  hdrs = carl_slist_append(hdrs, "X-Test-Number: 654");
  carl_mime_headers(part, hdrs, TRUE);
  part = carl_mime_addpart(mime);
  carl_mime_filedata(part, "log/file654.txt");
  part = carl_mime_addpart(mime);
  carl_mime_data_cb(part, (carl_off_t) -1, read_callback, NULL, free_callback,
                    &pooh);

  /* Bind mime data to its easy handle. */
  test_setopt(easy, CARLOPT_MIMEPOST, mime);

  /* Duplicate the handle. */
  easy2 = carl_easy_duphandle(easy);
  if(!easy2) {
    fprintf(stderr, "carl_easy_duphandle() failed\n");
    res = TEST_ERR_FAILURE;
    goto test_cleanup;
  }

  /* Now free the mime structure: it should unbind it from the first
     easy handle. */
  carl_mime_free(mime);
  mime = NULL;  /* Already cleaned up. */

  /* Perform on the first handle: should not send any data. */
  result = carl_easy_perform(easy);
  if(result) {
    fprintf(stderr, "carl_easy_perform(original) failed\n");
    res = (int) result;
    goto test_cleanup;
  }

  /* Perform on the second handle: if the bound mime structure has not been
     duplicated properly, it should cause a valgrind error. */
  result = carl_easy_perform(easy2);
  if(result) {
    fprintf(stderr, "carl_easy_perform(duplicated) failed\n");
    res = (int) result;
    goto test_cleanup;
  }

  /* Free the duplicated handle: it should call free_callback again.
     If the mime copy was bad or not automatically released, valgrind
     will signal it. */
  carl_easy_cleanup(easy2);
  easy2 = NULL;  /* Already cleaned up. */

  if(pooh.freecount != 2) {
    fprintf(stderr, "free_callback() called %d times instead of 2\n",
            pooh.freecount);
    res = TEST_ERR_FAILURE;
    goto test_cleanup;
  }

test_cleanup:
  carl_easy_cleanup(easy);
  carl_easy_cleanup(easy2);
  carl_mime_free(mime);
  carl_global_cleanup();
  return res;
}
