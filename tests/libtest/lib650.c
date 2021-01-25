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

static char data[] =
#ifdef CARL_DOES_CONVERSIONS
  /* ASCII representation with escape sequences for non-ASCII platforms */
  "\x74\x68\x69\x73\x20\x69\x73\x20\x77\x68\x61\x74\x20\x77\x65\x20\x70"
  "\x6f\x73\x74\x20\x74\x6f\x20\x74\x68\x65\x20\x73\x69\x6c\x6c\x79\x20"
  "\x77\x65\x62\x20\x73\x65\x72\x76\x65\x72";
#else
  "this is what we post to the silly web server";
#endif

static const char name[] = "fieldname";


/* This test attempts to use all form API features that are not
 * used elsewhere.
 */

/* carl_formget callback to count characters. */
static size_t count_chars(void *userp, const char *buf, size_t len)
{
  size_t *pcounter = (size_t *) userp;

  (void) buf;
  *pcounter += len;
  return len;
}


int test(char *URL)
{
  CARL *carl = NULL;
  CARLcode res = TEST_ERR_MAJOR_BAD;
  CARLFORMcode formrc;
  struct carl_slist *headers, *headers2 = NULL;
  struct carl_httppost *formpost = NULL;
  struct carl_httppost *lastptr = NULL;
  struct carl_forms formarray[3];
  size_t formlength = 0;
  char flbuf[32];
  long contentlength = 0;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  /* Check proper name and data copying, as well as headers. */
  headers = carl_slist_append(NULL, "X-customheader-1: Header 1 data");
  if(!headers) {
    goto test_cleanup;
  }
  headers2 = carl_slist_append(headers, "X-customheader-2: Header 2 data");
  if(!headers2) {
    goto test_cleanup;
  }
  headers = headers2;
  headers2 = carl_slist_append(headers, "Content-Type: text/plain");
  if(!headers2) {
    goto test_cleanup;
  }
  headers = headers2;
  formrc = carl_formadd(&formpost, &lastptr,
                        CARLFORM_COPYNAME, &name,
                        CARLFORM_COPYCONTENTS, &data,
                        CARLFORM_CONTENTHEADER, headers,
                        CARLFORM_END);

  if(formrc) {
    printf("carl_formadd(1) = %d\n", (int) formrc);
    goto test_cleanup;
  }

  contentlength = (long)(strlen(data) - 1);

  /* Use a form array for the non-copy test. */
  formarray[0].option = CARLFORM_PTRCONTENTS;
  formarray[0].value = data;
  formarray[1].option = CARLFORM_CONTENTSLENGTH;
  formarray[1].value = (char *)(size_t)contentlength;
  formarray[2].option = CARLFORM_END;
  formarray[2].value = NULL;
  formrc = carl_formadd(&formpost,
                        &lastptr,
                        CARLFORM_PTRNAME, name,
                        CARLFORM_NAMELENGTH, strlen(name) - 1,
                        CARLFORM_ARRAY, formarray,
                        CARLFORM_FILENAME, "remotefile.txt",
                        CARLFORM_END);

  if(formrc) {
    printf("carl_formadd(2) = %d\n", (int) formrc);
    goto test_cleanup;
  }

  /* Now change in-memory data to affect CARLOPT_PTRCONTENTS value.
     Copied values (first field) must not be affected.
     CARLOPT_PTRNAME actually copies the name thus we do not test this here. */
  data[0]++;

  /* Check multi-files and content type propagation. */
  formrc = carl_formadd(&formpost,
                        &lastptr,
                        CARLFORM_COPYNAME, "multifile",
                        CARLFORM_FILE, libtest_arg2,    /* Set in first.c. */
                        CARLFORM_FILE, libtest_arg2,
                        CARLFORM_CONTENTTYPE, "text/whatever",
                        CARLFORM_FILE, libtest_arg2,
                        CARLFORM_END);

  if(formrc) {
    printf("carl_formadd(3) = %d\n", (int) formrc);
    goto test_cleanup;
  }

  /* Check data from file content. */
  formrc = carl_formadd(&formpost,
                        &lastptr,
                        CARLFORM_COPYNAME, "filecontents",
                        CARLFORM_FILECONTENT, libtest_arg2,
                        CARLFORM_END);

  if(formrc) {
    printf("carl_formadd(4) = %d\n", (int) formrc);
    goto test_cleanup;
  }

  /* Measure the current form length.
   * This is done before including stdin data because we want to reuse it
   * and stdin cannot be rewound.
   */
  carl_formget(formpost, (void *) &formlength, count_chars);

  /* Include length in data for external check. */
  carl_msnprintf(flbuf, sizeof(flbuf), "%lu", (unsigned long) formlength);
  formrc = carl_formadd(&formpost,
                        &lastptr,
                        CARLFORM_COPYNAME, "formlength",
                        CARLFORM_COPYCONTENTS, &flbuf,
                        CARLFORM_END);
  if(formrc) {
    printf("carl_formadd(5) = %d\n", (int) formrc);
    goto test_cleanup;
  }

  /* Check stdin (may be problematic on some platforms). */
  formrc = carl_formadd(&formpost,
                        &lastptr,
                        CARLFORM_COPYNAME, "standardinput",
                        CARLFORM_FILE, "-",
                        CARLFORM_END);
  if(formrc) {
    printf("carl_formadd(6) = %d\n", (int) formrc);
    goto test_cleanup;
  }

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    goto test_cleanup;
  }

  /* First set the URL that is about to receive our POST. */
  test_setopt(carl, CARLOPT_URL, URL);

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
  carl_slist_free_all(headers);

  carl_global_cleanup();

  return res;
}
