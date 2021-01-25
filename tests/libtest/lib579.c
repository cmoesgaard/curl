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

static const char * const post[]={
  "one",
  "two",
  "three",
  "and a final longer crap: four",
  NULL
};


struct WriteThis {
  int counter;
};

static int progress_callback(void *clientp, double dltotal, double dlnow,
                             double ultotal, double ulnow)
{
  static int prev_ultotal = -1;
  static int prev_ulnow = -1;
  (void)clientp; /* UNUSED */
  (void)dltotal; /* UNUSED */
  (void)dlnow; /* UNUSED */

  /* to avoid depending on timing, which will cause this progress function to
     get called a different number of times depending on circumstances, we
     only log these lines if the numbers are different from the previous
     invoke */
  if((prev_ultotal != (int)ultotal) ||
     (prev_ulnow != (int)ulnow)) {

    FILE *moo = fopen(libtest_arg2, "ab");
    if(moo) {
      fprintf(moo, "Progress callback called with UL %d out of %d\n",
              (int)ulnow, (int)ultotal);
      fclose(moo);
    }
    prev_ulnow = (int) ulnow;
    prev_ultotal = (int) ultotal;
  }
  return 0;
}

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *pooh = (struct WriteThis *)userp;
  const char *data;

  if(size*nmemb < 1)
    return 0;

  data = post[pooh->counter];

  if(data) {
    size_t len = strlen(data);
    memcpy(ptr, data, len);
    pooh->counter++; /* advance pointer */
    return len;
  }
  return 0;                         /* no more data left to deliver */
}

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  struct carl_slist *slist = NULL;
  struct WriteThis pooh;
  pooh.counter = 0;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  slist = carl_slist_append(slist, "Transfer-Encoding: chunked");
  if(slist == NULL) {
    fprintf(stderr, "carl_slist_append() failed\n");
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* First set the URL that is about to receive our POST. */
  test_setopt(carl, CARLOPT_URL, URL);

  /* Now specify we want to POST data */
  test_setopt(carl, CARLOPT_POST, 1L);

#ifdef CARL_DOES_CONVERSIONS
  /* Convert the POST data to ASCII */
  test_setopt(carl, CARLOPT_TRANSFERTEXT, 1L);
#endif

  /* we want to use our own read function */
  test_setopt(carl, CARLOPT_READFUNCTION, read_callback);

  /* pointer to pass to our read function */
  test_setopt(carl, CARLOPT_READDATA, &pooh);

  /* get verbose debug output please */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(carl, CARLOPT_HEADER, 1L);

  /* enforce chunked transfer by setting the header */
  test_setopt(carl, CARLOPT_HTTPHEADER, slist);

  test_setopt(carl, CARLOPT_HTTPAUTH, (long)CARLAUTH_DIGEST);
  test_setopt(carl, CARLOPT_USERPWD, "foo:bar");

  /* we want to use our own progress function */
  test_setopt(carl, CARLOPT_NOPROGRESS, 0L);
  test_setopt(carl, CARLOPT_PROGRESSFUNCTION, progress_callback);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

test_cleanup:

  /* clean up the headers list */
  if(slist)
    carl_slist_free_all(slist);

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
