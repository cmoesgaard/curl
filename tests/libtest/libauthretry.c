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
/* argv1 = URL
 * argv2 = main auth type
 * argv3 = second auth type
 */

#include "test.h"
#include "memdebug.h"

static CARLcode send_request(CARL *carl, const char *url, int seq,
                             long auth_scheme, const char *userpwd)
{
  CARLcode res;
  size_t len = strlen(url) + 4 + 1;
  char *full_url = malloc(len);
  if(!full_url) {
    fprintf(stderr, "Not enough memory for full url\n");
    return CARLE_OUT_OF_MEMORY;
  }

  msnprintf(full_url, len, "%s%04d", url, seq);
  fprintf(stderr, "Sending new request %d to %s with credential %s "
          "(auth %ld)\n", seq, full_url, userpwd, auth_scheme);
  test_setopt(carl, CARLOPT_URL, full_url);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);
  test_setopt(carl, CARLOPT_HEADER, 1L);
  test_setopt(carl, CARLOPT_HTTPGET, 1L);
  test_setopt(carl, CARLOPT_USERPWD, userpwd);
  test_setopt(carl, CARLOPT_HTTPAUTH, auth_scheme);

  res = carl_easy_perform(carl);

test_cleanup:
  free(full_url);
  return res;
}

static CARLcode send_wrong_password(CARL *carl, const char *url, int seq,
                                    long auth_scheme)
{
    return send_request(carl, url, seq, auth_scheme, "testuser:wrongpass");
}

static CARLcode send_right_password(CARL *carl, const char *url, int seq,
                                    long auth_scheme)
{
    return send_request(carl, url, seq, auth_scheme, "testuser:testpass");
}

static long parse_auth_name(const char *arg)
{
  if(!arg)
    return CARLAUTH_NONE;
  if(carl_strequal(arg, "basic"))
    return CARLAUTH_BASIC;
  if(carl_strequal(arg, "digest"))
    return CARLAUTH_DIGEST;
  if(carl_strequal(arg, "ntlm"))
    return CARLAUTH_NTLM;
  return CARLAUTH_NONE;
}

int test(char *url)
{
  CARLcode res;
  CARL *carl = NULL;

  long main_auth_scheme = parse_auth_name(libtest_arg2);
  long fallback_auth_scheme = parse_auth_name(libtest_arg3);

  if(main_auth_scheme == CARLAUTH_NONE ||
      fallback_auth_scheme == CARLAUTH_NONE) {
    fprintf(stderr, "auth schemes not found on commandline\n");
    return TEST_ERR_MAJOR_BAD;
  }

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  /* Send wrong password, then right password */

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  res = send_wrong_password(carl, url, 100, main_auth_scheme);
  if(res != CARLE_OK)
    goto test_cleanup;

  res = send_right_password(carl, url, 200, fallback_auth_scheme);
  if(res != CARLE_OK)
    goto test_cleanup;

  carl_easy_cleanup(carl);

  /* Send wrong password twice, then right password */
  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  res = send_wrong_password(carl, url, 300, main_auth_scheme);
  if(res != CARLE_OK)
    goto test_cleanup;

  res = send_wrong_password(carl, url, 400, fallback_auth_scheme);
  if(res != CARLE_OK)
    goto test_cleanup;

  res = send_right_password(carl, url, 500, fallback_auth_scheme);
  if(res != CARLE_OK)
    goto test_cleanup;

test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
