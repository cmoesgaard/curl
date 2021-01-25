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
  "\x74\x68\x69\x73\x20\x69\x73\x20\x77\x68\x61\x74\x20\x77\x65\x20\x70"
  "\x6f\x73\x74\x20\x74\x6f\x20\x74\x68\x65\x20\x73\x69\x6c\x6c\x79\x20"
  "\x77\x65\x62\x20\x73\x65\x72\x76\x65\x72\x0a";
#else
  "this is what we post to the silly web server\n";
#endif

struct WriteThis {
  char *readptr;
  size_t sizeleft;
};

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
#ifdef LIB587
  (void)ptr;
  (void)size;
  (void)nmemb;
  (void)userp;
  return CARL_READFUNC_ABORT;
#else

  struct WriteThis *pooh = (struct WriteThis *)userp;

  if(size*nmemb < 1)
    return 0;

  if(pooh->sizeleft) {
    *ptr = pooh->readptr[0]; /* copy one single byte */
    pooh->readptr++;                 /* advance pointer */
    pooh->sizeleft--;                /* less data left */
    return 1;                        /* we return 1 byte at a time! */
  }

  return 0;                         /* no more data left to deliver */
#endif
}

static int once(char *URL, bool oldstyle)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  CARLFORMcode formrc;

  struct carl_httppost *formpost = NULL;
  struct carl_httppost *lastptr = NULL;
  struct WriteThis pooh;
  struct WriteThis pooh2;

  pooh.readptr = data;
  pooh.sizeleft = strlen(data);

  /* Fill in the file upload field */
  if(oldstyle) {
    formrc = carl_formadd(&formpost,
                          &lastptr,
                          CARLFORM_COPYNAME, "sendfile",
                          CARLFORM_STREAM, &pooh,
                          CARLFORM_CONTENTSLENGTH, (long)pooh.sizeleft,
                          CARLFORM_FILENAME, "postit2.c",
                          CARLFORM_END);
  }
  else {
    /* new style */
    formrc = carl_formadd(&formpost,
                          &lastptr,
                          CARLFORM_COPYNAME, "sendfile alternative",
                          CARLFORM_STREAM, &pooh,
                          CARLFORM_CONTENTLEN, (carl_off_t)pooh.sizeleft,
                          CARLFORM_FILENAME, "file name 2",
                          CARLFORM_END);
  }

  if(formrc)
    printf("carl_formadd(1) = %d\n", (int)formrc);

  /* Now add the same data with another name and make it not look like
     a file upload but still using the callback */

  pooh2.readptr = data;
  pooh2.sizeleft = strlen(data);

  /* Fill in the file upload field */
  formrc = carl_formadd(&formpost,
                        &lastptr,
                        CARLFORM_COPYNAME, "callbackdata",
                        CARLFORM_STREAM, &pooh2,
                        CARLFORM_CONTENTSLENGTH, (long)pooh2.sizeleft,
                        CARLFORM_END);

  if(formrc)
    printf("carl_formadd(2) = %d\n", (int)formrc);

  /* Fill in the filename field */
  formrc = carl_formadd(&formpost,
                        &lastptr,
                        CARLFORM_COPYNAME, "filename",
#ifdef CARL_DOES_CONVERSIONS
                        /* ASCII representation with escape
                           sequences for non-ASCII platforms */
                        CARLFORM_COPYCONTENTS,
                           "\x70\x6f\x73\x74\x69\x74\x32\x2e\x63",
#else
                        CARLFORM_COPYCONTENTS, "postit2.c",
#endif
                        CARLFORM_END);

  if(formrc)
    printf("carl_formadd(3) = %d\n", (int)formrc);

  /* Fill in a submit field too */
  formrc = carl_formadd(&formpost,
                        &lastptr,
                        CARLFORM_COPYNAME, "submit",
#ifdef CARL_DOES_CONVERSIONS
                        /* ASCII representation with escape
                           sequences for non-ASCII platforms */
                        CARLFORM_COPYCONTENTS, "\x73\x65\x6e\x64",
#else
                        CARLFORM_COPYCONTENTS, "send",
#endif
                        CARLFORM_CONTENTTYPE, "text/plain",
                        CARLFORM_END);

  if(formrc)
    printf("carl_formadd(4) = %d\n", (int)formrc);

  formrc = carl_formadd(&formpost, &lastptr,
                        CARLFORM_COPYNAME, "somename",
                        CARLFORM_BUFFER, "somefile.txt",
                        CARLFORM_BUFFERPTR, "blah blah",
                        CARLFORM_BUFFERLENGTH, (long)9,
                        CARLFORM_END);

  if(formrc)
    printf("carl_formadd(5) = %d\n", (int)formrc);

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_formfree(formpost);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* First set the URL that is about to receive our POST. */
  test_setopt(carl, CARLOPT_URL, URL);

  /* Now specify we want to POST data */
  test_setopt(carl, CARLOPT_POST, 1L);

  /* Set the expected POST size */
  test_setopt(carl, CARLOPT_POSTFIELDSIZE, (long)pooh.sizeleft);

  /* we want to use our own read function */
  test_setopt(carl, CARLOPT_READFUNCTION, read_callback);

  /* send a multi-part formpost */
  test_setopt(carl, CARLOPT_HTTPPOST, formpost);

  /* get verbose debug output please */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(carl, CARLOPT_HEADER, 1L);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);

  /* now cleanup the formpost chain */
  carl_formfree(formpost);

  return res;
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

  carl_global_cleanup();

  return res;
}
