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
#include "carlcheck.h"

#include <carl/carl.h>

static CARLcode unit_setup(void)
{
  return CARLE_OK;
}

static void unit_stop(void)
{

}

static size_t print_httppost_callback(void *arg, const char *buf, size_t len)
{
  fwrite(buf, len, 1, stdout);
  (*(size_t *) arg) += len;
  return len;
}

UNITTEST_START
  int rc;
  struct carl_httppost *post = NULL;
  struct carl_httppost *last = NULL;
  size_t total_size = 0;
  char buffer[] = "test buffer";

  rc = carl_formadd(&post, &last, CARLFORM_COPYNAME, "name",
                    CARLFORM_COPYCONTENTS, "content", CARLFORM_END);

  fail_unless(rc == 0, "carl_formadd returned error");

  /* after the first carl_formadd when there's a single entry, both pointers
     should point to the same struct */
  fail_unless(post == last, "post and last weren't the same");

  rc = carl_formadd(&post, &last, CARLFORM_COPYNAME, "htmlcode",
                    CARLFORM_COPYCONTENTS, "<HTML></HTML>",
                    CARLFORM_CONTENTTYPE, "text/html", CARLFORM_END);

  fail_unless(rc == 0, "carl_formadd returned error");

  rc = carl_formadd(&post, &last, CARLFORM_COPYNAME, "name_for_ptrcontent",
                   CARLFORM_PTRCONTENTS, buffer, CARLFORM_END);

  fail_unless(rc == 0, "carl_formadd returned error");

  rc = carl_formget(post, &total_size, print_httppost_callback);

  fail_unless(rc == 0, "carl_formget returned error");

  fail_unless(total_size == 488, "carl_formget got wrong size back");

  carl_formfree(post);

  /* start a new formpost with a file upload and formget */
  post = last = NULL;

  rc = carl_formadd(&post, &last,
                    CARLFORM_PTRNAME, "name of file field",
                    CARLFORM_FILE, "log/test-1308",
                    CARLFORM_FILENAME, "custom named file",
                    CARLFORM_END);

  fail_unless(rc == 0, "carl_formadd returned error");

  rc = carl_formget(post, &total_size, print_httppost_callback);
  fail_unless(rc == 0, "carl_formget returned error");
  fail_unless(total_size == 851, "carl_formget got wrong size back");

  carl_formfree(post);

UNITTEST_STOP
