/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#include "urldata.h"
#include "hsts.h"

static CARLcode
unit_setup(void)
{
  return CARLE_OK;
}

static void
unit_stop(void)
{
  carl_global_cleanup();
}

#if defined(CARL_DISABLE_HTTP) || !defined(USE_HSTS)
UNITTEST_START
{
  return 0; /* nothing to do when HTTP or HSTS are disabled */
}
UNITTEST_STOP
#else

struct testit {
  const char *host;
  const char *chost; /* if non-NULL, use to lookup with */
  const char *hdr; /* if NULL, just do the lookup */
  const CARLcode result; /* parse result */
};

static const struct testit headers[] = {
  /* two entries read from disk cache, verify first */
  { "-", "readfrom.example", NULL, CARLE_OK},
  { "-", "old.example", NULL, CARLE_OK},
  /* delete the remaining one read from disk */
  { "readfrom.example", NULL, "max-age=\"0\"", CARLE_OK},

  { "example.com", NULL, "max-age=\"31536000\"\r\n", CARLE_OK },
  { "example.com", NULL, "max-age=\"21536000\"\r\n", CARLE_OK },
  { "example.com", NULL, "max-age=\"21536000\"; \r\n", CARLE_OK },
  { "example.com", NULL, "max-age=\"21536000\"; includeSubDomains\r\n",
    CARLE_OK },
  { "example.org", NULL, "max-age=\"31536000\"\r\n", CARLE_OK },
  { "this.example", NULL, "max=\"31536\";", CARLE_BAD_FUNCTION_ARGUMENT },
  { "this.example", NULL, "max-age=\"31536", CARLE_BAD_FUNCTION_ARGUMENT },
  { "this.example", NULL, "max-age=31536\"", CARLE_OK },
  /* max-age=0 removes the entry */
  { "this.example", NULL, "max-age=0", CARLE_OK },
  { "another.example", NULL, "includeSubDomains; ",
    CARLE_BAD_FUNCTION_ARGUMENT },

  /* Two max-age is illegal */
  { "example.com", NULL,
    "max-age=\"21536000\"; includeSubDomains; max-age=\"3\";",
    CARLE_BAD_FUNCTION_ARGUMENT },
  /* Two includeSubDomains is illegal */
  { "2.example.com", NULL,
    "max-age=\"21536000\"; includeSubDomains; includeSubDomains;",
    CARLE_BAD_FUNCTION_ARGUMENT },
  /* use a unknown directive "include" that should be ignored */
  { "3.example.com", NULL, "max-age=\"21536000\"; include; includeSubDomains;",
    CARLE_OK },
  /* remove the "3.example.com" one, should still match the example.com */
  { "3.example.com", NULL, "max-age=\"0\"; includeSubDomains;",
    CARLE_OK },
  { "-", "foo.example.com", NULL, CARLE_OK},
  { "-", "foo.xample.com", NULL, CARLE_OK},

  /* should not match */
  { "example.net", "forexample.net", "max-age=\"31536000\"\r\n", CARLE_OK },

  /* should not match either, since forexample.net is not in the example.net
     domain */
  { "example.net", "forexample.net",
    "max-age=\"31536000\"; includeSubDomains\r\n", CARLE_OK },
  /* remove example.net again */
  { "example.net", NULL, "max-age=\"0\"; includeSubDomains\r\n", CARLE_OK },

  /* make this live for 7 seconds */
  { "expire.example", NULL, "max-age=\"7\"\r\n", CARLE_OK },
  { NULL, NULL, NULL, 0 }
};

static void showsts(struct stsentry *e, const char *chost)
{
  if(!e)
    printf("'%s' is not HSTS\n", chost);
  else {
    printf("%s [%s]: %" CARL_FORMAT_CARL_OFF_T "%s\n",
           chost, e->host, e->expires,
           e->includeSubDomains ? " includeSubDomains" : "");
  }
}

UNITTEST_START
{
  CARLcode result;
  struct stsentry *e;
  struct hsts *h = Curl_hsts_init();
  int i;
  const char *chost;
  CARL *easy;
  if(!h)
    return 1;

  carl_global_init(CARL_GLOBAL_ALL);
  easy = carl_easy_init();
  if(!easy) {
    Curl_hsts_cleanup(&h);
    carl_global_cleanup();
    return 1;
  }

  Curl_hsts_loadfile(easy, h, "log/input1660");

  for(i = 0; headers[i].host ; i++) {
    if(headers[i].hdr) {
      result = Curl_hsts_parse(h, headers[i].host, headers[i].hdr);

      if(result != headers[i].result) {
        fprintf(stderr, "Curl_hsts_parse(%s) failed: %d\n",
                headers[i].hdr, result);
        unitfail++;
        continue;
      }
      else if(result) {
        printf("Input %u: error %d\n", i, (int) result);
        continue;
      }
    }

    chost = headers[i].chost ? headers[i].chost : headers[i].host;
    e = Curl_hsts(h, chost, TRUE);
    showsts(e, chost);
  }

  printf("Number of entries: %zu\n", h->list.size);

  /* verify that it is exists for 7 seconds */
  chost = "expire.example";
  for(i = 100; i < 110; i++) {
    e = Curl_hsts(h, chost, TRUE);
    showsts(e, chost);
    deltatime++; /* another second passed */
  }

  (void)Curl_hsts_save(easy, h, "log/hsts1660");
  Curl_hsts_cleanup(&h);
  carl_easy_cleanup(easy);
  carl_global_cleanup();
  return unitfail;
}
UNITTEST_STOP
#endif
