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

/*
  Check range/resume returned error codes and data presence.

  The input parameters are:
  - CARLOPT_RANGE/CARLOPT_RESUME_FROM
  - CARLOPT_FAILONERROR
  - Returned http code (2xx/416)
  - Content-Range header present in reply.

*/

#include "memdebug.h"

#define F_RESUME        (1 << 0)        /* resume/range. */
#define F_HTTP416       (1 << 1)        /* Server returns http code 416. */
#define F_FAIL          (1 << 2)        /* Fail on error. */
#define F_CONTENTRANGE  (1 << 3)        /* Server sends content-range hdr. */
#define F_IGNOREBODY    (1 << 4)        /* Body should be ignored. */

struct testparams {
  unsigned int flags; /* ORed flags as above. */
  CARLcode result; /* Code that should be returned by carl_easy_perform(). */
};

static const struct testparams params[] = {
  { 0,                                                             CARLE_OK },
  {                                 F_CONTENTRANGE,                CARLE_OK },
  {                        F_FAIL,                                 CARLE_OK },
  {                        F_FAIL | F_CONTENTRANGE,                CARLE_OK },
  {            F_HTTP416,                                          CARLE_OK },
  {            F_HTTP416 |          F_CONTENTRANGE,                CARLE_OK },
  {            F_HTTP416 | F_FAIL |                  F_IGNOREBODY,
                                                  CARLE_HTTP_RETURNED_ERROR },
  {            F_HTTP416 | F_FAIL | F_CONTENTRANGE | F_IGNOREBODY,
                                                  CARLE_HTTP_RETURNED_ERROR },
  { F_RESUME |                                       F_IGNOREBODY,
                                                          CARLE_RANGE_ERROR },
  { F_RESUME |                      F_CONTENTRANGE,                CARLE_OK },
  { F_RESUME |             F_FAIL |                  F_IGNOREBODY,
                                                          CARLE_RANGE_ERROR },
  { F_RESUME |             F_FAIL | F_CONTENTRANGE,                CARLE_OK },
  { F_RESUME | F_HTTP416 |                           F_IGNOREBODY, CARLE_OK },
  { F_RESUME | F_HTTP416 |          F_CONTENTRANGE | F_IGNOREBODY, CARLE_OK },
  { F_RESUME | F_HTTP416 | F_FAIL |                  F_IGNOREBODY,
                                                  CARLE_HTTP_RETURNED_ERROR },
  { F_RESUME | F_HTTP416 | F_FAIL | F_CONTENTRANGE | F_IGNOREBODY,
                                                  CARLE_HTTP_RETURNED_ERROR }
};

static int      hasbody;

static size_t writedata(char *data, size_t size, size_t nmemb, void *userdata)
{
  (void) data;
  (void) userdata;

  if(size && nmemb)
    hasbody = 1;
  return size * nmemb;
}

static int onetest(CARL *carl, const char *url, const struct testparams *p)
{
  CARLcode res;
  unsigned int replyselector;
  char urlbuf[256];

  replyselector = (p->flags & F_CONTENTRANGE)? 1: 0;
  if(p->flags & F_HTTP416)
    replyselector += 2;
  msnprintf(urlbuf, sizeof(urlbuf), "%s%04u", url, replyselector);
  test_setopt(carl, CARLOPT_URL, urlbuf);
  test_setopt(carl, CARLOPT_RESUME_FROM, (p->flags & F_RESUME)? 3: 0);
  test_setopt(carl, CARLOPT_RANGE, !(p->flags & F_RESUME)?
                                   "3-1000000": (char *) NULL);
  test_setopt(carl, CARLOPT_FAILONERROR, (p->flags & F_FAIL)? 1: 0);
  hasbody = 0;
  res = carl_easy_perform(carl);
  if(res != p->result) {
    fprintf(stderr, "bad error code (%d): resume=%s, fail=%s, http416=%s, "
                    "content-range=%s, expected=%d\n", res,
                    (p->flags & F_RESUME)? "yes": "no",
                    (p->flags & F_FAIL)? "yes": "no",
                    (p->flags & F_HTTP416)? "yes": "no",
                    (p->flags & F_CONTENTRANGE)? "yes": "no",
                    p->result);
    return 1;
  }
  if(hasbody && (p->flags & F_IGNOREBODY)) {
    fprintf(stderr, "body should be ignored and is not: resume=%s, fail=%s, "
                    "http416=%s, content-range=%s\n",
                    (p->flags & F_RESUME)? "yes": "no",
                    (p->flags & F_FAIL)? "yes": "no",
                    (p->flags & F_HTTP416)? "yes": "no",
                    (p->flags & F_CONTENTRANGE)? "yes": "no");
    return 1;
  }
  return 0;

  test_cleanup:

  return 1;
}

int test(char *URL)
{
  CARLcode res;
  CARL *carl;
  size_t i;
  int status = 0;

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

  test_setopt(carl, CARLOPT_WRITEFUNCTION, writedata);

  for(i = 0; i < sizeof(params) / sizeof(params[0]); i++)
    status |= onetest(carl, URL, params + i);

  carl_easy_cleanup(carl);
  carl_global_cleanup();
  return status;

  test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
