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

/* build request url */
static char *suburl(const char *base, int i)
{
  return carl_maprintf("%s%.4d", base, i);
}

/*
 * Test Session ID capture
 */
int test(char *URL)
{
  int res;
  CARL *carl;
  char *stream_uri = NULL;
  char *rtsp_session_id;
  int request = 1;
  int i;

  FILE *idfile = fopen(libtest_arg2, "wb");
  if(idfile == NULL) {
    fprintf(stderr, "couldn't open the Session ID File\n");
    return TEST_ERR_MAJOR_BAD;
  }

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    fclose(idfile);
    return TEST_ERR_MAJOR_BAD;
  }

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    fclose(idfile);
    return TEST_ERR_MAJOR_BAD;
  }

  test_setopt(carl, CARLOPT_HEADERDATA, stdout);
  test_setopt(carl, CARLOPT_WRITEDATA, stdout);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  test_setopt(carl, CARLOPT_URL, URL);

  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_SETUP);
  res = carl_easy_perform(carl);
  if(res != (int)CARLE_BAD_FUNCTION_ARGUMENT) {
    fprintf(stderr, "This should have failed. "
            "Cannot setup without a Transport: header");
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }

  /* Go through the various Session IDs */
  for(i = 0; i < 3; i++) {
    stream_uri = suburl(URL, request++);
    if(!stream_uri) {
      res = TEST_ERR_MAJOR_BAD;
      goto test_cleanup;
    }
    test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
    free(stream_uri);
    stream_uri = NULL;

    test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_SETUP);
    test_setopt(carl, CARLOPT_RTSP_TRANSPORT,
                "Fake/NotReal/JustATest;foo=baz");
    res = carl_easy_perform(carl);
    if(res)
      goto test_cleanup;

    carl_easy_getinfo(carl, CARLINFO_RTSP_SESSION_ID, &rtsp_session_id);
    fprintf(idfile, "Got Session ID: [%s]\n", rtsp_session_id);
    rtsp_session_id = NULL;

    stream_uri = suburl(URL, request++);
    if(!stream_uri) {
      res = TEST_ERR_MAJOR_BAD;
      goto test_cleanup;
    }
    test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
    free(stream_uri);
    stream_uri = NULL;

    test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_TEARDOWN);
    res = carl_easy_perform(carl);

    /* Clear for the next go-round */
    test_setopt(carl, CARLOPT_RTSP_SESSION_ID, NULL);
  }

test_cleanup:

  if(idfile)
    fclose(idfile);

  free(stream_uri);
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
