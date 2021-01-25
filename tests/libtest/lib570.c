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

int test(char *URL)
{
  int res;
  CARL *carl;
  int request = 1;
  char *stream_uri = NULL;

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

  test_setopt(carl, CARLOPT_HEADERDATA, stdout);
  test_setopt(carl, CARLOPT_WRITEDATA, stdout);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  test_setopt(carl, CARLOPT_URL, URL);

  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_OPTIONS);

  stream_uri = suburl(URL, request++);
  if(!stream_uri) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
  free(stream_uri);
  stream_uri = NULL;

  res = carl_easy_perform(carl);
  if(res != (int)CARLE_RTSP_CSEQ_ERROR) {
    fprintf(stderr, "Failed to detect CSeq mismatch");
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }

  test_setopt(carl, CARLOPT_RTSP_CLIENT_CSEQ, 999L);
  test_setopt(carl, CARLOPT_RTSP_TRANSPORT,
                    "RAW/RAW/UDP;unicast;client_port=3056-3057");
  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_SETUP);

  stream_uri = suburl(URL, request++);
  if(!stream_uri) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
  free(stream_uri);
  stream_uri = NULL;

  res = carl_easy_perform(carl);
  if(res)
    goto test_cleanup;

  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_PLAY);

  stream_uri = suburl(URL, request++);
  if(!stream_uri) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
  free(stream_uri);
  stream_uri = NULL;

  res = carl_easy_perform(carl);
  if(res == CARLE_RTSP_SESSION_ERROR) {
    res = 0;
  }
  else {
    fprintf(stderr, "Failed to detect a Session ID mismatch");
    res = 1;
  }

test_cleanup:
  free(stream_uri);

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
