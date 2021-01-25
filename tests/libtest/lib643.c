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
};

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
#ifdef LIB644
  static int count = 0;
  (void)ptr;
  (void)size;
  (void)nmemb;
  (void)userp;
  switch(count++) {
  case 0: /* Return a single byte. */
    *ptr = '\n';
    return 1;
  case 1: /* Request abort. */
    return CARL_READFUNC_ABORT;
  }
  printf("Wrongly called >2 times\n");
  exit(1); /* trigger major failure */
#else

  struct WriteThis *pooh = (struct WriteThis *)userp;
  int eof = !*pooh->readptr;

  if(size*nmemb < 1)
    return 0;

#ifndef LIB645
  eof = pooh->sizeleft <= 0;
  if(!eof)
    pooh->sizeleft--;
#endif

  if(!eof) {
    *ptr = *pooh->readptr;           /* copy one single byte */
    pooh->readptr++;                 /* advance pointer */
    return 1;                        /* we return 1 byte at a time! */
  }

  return 0;                         /* no more data left to deliver */
#endif
}

static int once(char *URL, bool oldstyle)
{
  CARL *carl;
  CARLcode res = CARLE_OK;

  carl_mime *mime = NULL;
  carl_mimepart *part = NULL;
  struct WriteThis pooh;
  struct WriteThis pooh2;
  carl_off_t datasize = -1;

  pooh.readptr = data;
#ifndef LIB645
  datasize = (carl_off_t)strlen(data);
#endif
  pooh.sizeleft = datasize;

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  mime = carl_mime_init(carl);
  if(!mime) {
    fprintf(stderr, "carl_mime_init() failed\n");
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  part = carl_mime_addpart(mime);
  if(!part) {
    fprintf(stderr, "carl_mime_addpart(1) failed\n");
    carl_mime_free(mime);
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* Fill in the file upload part */
  if(oldstyle) {
    res = carl_mime_name(part, "sendfile");
    if(!res)
      res = carl_mime_data_cb(part, datasize, read_callback,
                              NULL, NULL, &pooh);
    if(!res)
      res = carl_mime_filename(part, "postit2.c");
  }
  else {
    /* new style */
    res = carl_mime_name(part, "sendfile alternative");
    if(!res)
      res = carl_mime_data_cb(part, datasize, read_callback,
                              NULL, NULL, &pooh);
    if(!res)
      res = carl_mime_filename(part, "file name 2");
  }

  if(res)
    printf("carl_mime_xxx(1) = %s\n", carl_easy_strerror(res));

  /* Now add the same data with another name and make it not look like
     a file upload but still using the callback */

  pooh2.readptr = data;
#ifndef LIB645
  datasize = (carl_off_t)strlen(data);
#endif
  pooh2.sizeleft = datasize;

  part = carl_mime_addpart(mime);
  if(!part) {
    fprintf(stderr, "carl_mime_addpart(2) failed\n");
    carl_mime_free(mime);
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }
  /* Fill in the file upload part */
  res = carl_mime_name(part, "callbackdata");
  if(!res)
    res = carl_mime_data_cb(part, datasize, read_callback,
                            NULL, NULL, &pooh2);

  if(res)
    printf("carl_mime_xxx(2) = %s\n", carl_easy_strerror(res));

  part = carl_mime_addpart(mime);
  if(!part) {
    fprintf(stderr, "carl_mime_addpart(3) failed\n");
    carl_mime_free(mime);
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* Fill in the filename field */
  res = carl_mime_name(part, "filename");
  if(!res)
    res = carl_mime_data(part,
#ifdef CARL_DOES_CONVERSIONS
                         /* ASCII representation with escape
                            sequences for non-ASCII platforms */
                         "\x70\x6f\x73\x74\x69\x74\x32\x2e\x63",
#else
                          "postit2.c",
#endif
                          CARL_ZERO_TERMINATED);

  if(res)
    printf("carl_mime_xxx(3) = %s\n", carl_easy_strerror(res));

  /* Fill in a submit field too */
  part = carl_mime_addpart(mime);
  if(!part) {
    fprintf(stderr, "carl_mime_addpart(4) failed\n");
    carl_mime_free(mime);
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }
  res = carl_mime_name(part, "submit");
  if(!res)
    res = carl_mime_data(part,
#ifdef CARL_DOES_CONVERSIONS
                         /* ASCII representation with escape
                            sequences for non-ASCII platforms */
                         "\x73\x65\x6e\x64",
#else
                          "send",
#endif
                          CARL_ZERO_TERMINATED);

  if(res)
    printf("carl_mime_xxx(4) = %s\n", carl_easy_strerror(res));

  part = carl_mime_addpart(mime);
  if(!part) {
    fprintf(stderr, "carl_mime_addpart(5) failed\n");
    carl_mime_free(mime);
    carl_easy_cleanup(carl);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }
  res = carl_mime_name(part, "somename");
  if(!res)
    res = carl_mime_filename(part, "somefile.txt");
  if(!res)
    res = carl_mime_data(part, "blah blah", 9);

  if(res)
    printf("carl_mime_xxx(5) = %s\n", carl_easy_strerror(res));

  /* First set the URL that is about to receive our POST. */
  test_setopt(carl, CARLOPT_URL, URL);

  /* send a multi-part mimepost */
  test_setopt(carl, CARLOPT_MIMEPOST, mime);

  /* get verbose debug output please */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(carl, CARLOPT_HEADER, 1L);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);

  /* now cleanup the mimepost structure */
  carl_mime_free(mime);

  return res;
}

static int cyclic_add(void)
{
  CARL *easy = carl_easy_init();
  carl_mime *mime = carl_mime_init(easy);
  carl_mimepart *part = carl_mime_addpart(mime);
  CARLcode a1 = carl_mime_subparts(part, mime);

  if(a1 == CARLE_BAD_FUNCTION_ARGUMENT) {
    carl_mime *submime = carl_mime_init(easy);
    carl_mimepart *subpart = carl_mime_addpart(submime);

    carl_mime_subparts(part, submime);
    a1 = carl_mime_subparts(subpart, mime);
  }

  carl_mime_free(mime);
  carl_easy_cleanup(easy);
  if(a1 != CARLE_BAD_FUNCTION_ARGUMENT)
    /* that should have failed */
    return 1;

  return 0;
}

int test(char *URL)
{
  int res;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  res = once(URL, TRUE); /* old */
  if(!res)
    res = once(URL, FALSE); /* new */

  if(!res)
    res = cyclic_add();

  carl_global_cleanup();

  return res;
}
