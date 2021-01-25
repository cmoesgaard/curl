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

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "memdebug.h"

/* build request url */
static char *suburl(const char *base, int i)
{
  return carl_maprintf("%s%.4d", base, i);
}

/*
 * Test GET_PARAMETER: PUT, HEARTBEAT, and POST
 */
int test(char *URL)
{
  int res;
  CARL *carl;
  int params;
  FILE *paramsf = NULL;
  struct_stat file_info;
  char *stream_uri = NULL;
  int request = 1;
  struct carl_slist *custom_headers = NULL;

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

  /* SETUP */
  stream_uri = suburl(URL, request++);
  if(!stream_uri) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
  free(stream_uri);
  stream_uri = NULL;

  test_setopt(carl, CARLOPT_RTSP_TRANSPORT, "Planes/Trains/Automobiles");
  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_SETUP);
  res = carl_easy_perform(carl);
  if(res)
    goto test_cleanup;

  stream_uri = suburl(URL, request++);
  if(!stream_uri) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
  free(stream_uri);
  stream_uri = NULL;

  /* PUT style GET_PARAMETERS */
  params = open("log/file572.txt", O_RDONLY);
  fstat(params, &file_info);
  close(params);

  paramsf = fopen("log/file572.txt", "rb");
  if(paramsf == NULL) {
    fprintf(stderr, "can't open log/file572.txt\n");
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_GET_PARAMETER);

  test_setopt(carl, CARLOPT_READDATA, paramsf);
  test_setopt(carl, CARLOPT_UPLOAD, 1L);
  test_setopt(carl, CARLOPT_INFILESIZE_LARGE, (carl_off_t) file_info.st_size);

  res = carl_easy_perform(carl);
  if(res)
    goto test_cleanup;

  test_setopt(carl, CARLOPT_UPLOAD, 0L);
  fclose(paramsf);
  paramsf = NULL;

  /* Heartbeat GET_PARAMETERS */
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

  /* POST GET_PARAMETERS */

  stream_uri = suburl(URL, request++);
  if(!stream_uri) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
  free(stream_uri);
  stream_uri = NULL;

  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_GET_PARAMETER);
  test_setopt(carl, CARLOPT_POSTFIELDS, "packets_received\njitter\n");

  res = carl_easy_perform(carl);
  if(res)
    goto test_cleanup;

  test_setopt(carl, CARLOPT_POSTFIELDS, NULL);

  /* Make sure we can do a normal request now */
  stream_uri = suburl(URL, request++);
  if(!stream_uri) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  test_setopt(carl, CARLOPT_RTSP_STREAM_URI, stream_uri);
  free(stream_uri);
  stream_uri = NULL;

  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_OPTIONS);
  res = carl_easy_perform(carl);

test_cleanup:

  if(paramsf)
    fclose(paramsf);

  free(stream_uri);

  if(custom_headers)
    carl_slist_free_all(custom_headers);

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
