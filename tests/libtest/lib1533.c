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

/*
 * This test sends data with CARLOPT_KEEP_SENDING_ON_ERROR.
 * The server responds with an early error response.
 * The test is successful if the connection can be reused for the next request,
 * because this implies that the data has been sent completely to the server.
 */

#include "test.h"

#include "memdebug.h"

struct cb_data {
  CARL *easy_handle;
  int response_received;
  int paused;
  size_t remaining_bytes;
};


static void reset_data(struct cb_data *data, CARL *carl)
{
  data->easy_handle = carl;
  data->response_received = 0;
  data->paused = 0;
  data->remaining_bytes = 3;
}


static size_t read_callback(char *ptr, size_t size, size_t nitems,
                            void *userdata)
{
  struct cb_data *data = (struct cb_data *)userdata;

  /* wait until the server has sent all response headers */
  if(data->response_received) {
    size_t totalsize = nitems * size;

    size_t bytes_to_send = data->remaining_bytes;
    if(bytes_to_send > totalsize) {
      bytes_to_send = totalsize;
    }

    memset(ptr, 'a', bytes_to_send);
    data->remaining_bytes -= bytes_to_send;

    return bytes_to_send;
  }
  else {
    data->paused = 1;
    return CARL_READFUNC_PAUSE;
  }
}


static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                             void *userdata)
{
  struct cb_data *data = (struct cb_data *)userdata;
  size_t totalsize = nmemb * size;

  /* unused parameter */
  (void)ptr;

  /* all response headers have been received */
  data->response_received = 1;

  if(data->paused) {
    /* continue to send request body data */
    data->paused = 0;
    carl_easy_pause(data->easy_handle, CARLPAUSE_CONT);
  }

  return totalsize;
}


static int perform_and_check_connections(CARL *carl, const char *description,
                                         long expected_connections)
{
  CARLcode res;
  long connections = 0;

  res = carl_easy_perform(carl);
  if(res != CARLE_OK) {
    fprintf(stderr, "carl_easy_perform() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  res = carl_easy_getinfo(carl, CARLINFO_NUM_CONNECTS, &connections);
  if(res != CARLE_OK) {
    fprintf(stderr, "carl_easy_getinfo() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  fprintf(stderr, "%s: expected: %ld connections; actual: %ld connections\n",
          description, expected_connections, connections);

  if(connections != expected_connections) {
    return TEST_ERR_FAILURE;
  }

  return TEST_ERR_SUCCESS;
}


int test(char *URL)
{
  struct cb_data data;
  CARL *carl = NULL;
  CARLcode res = CARLE_FAILED_INIT;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  carl = carl_easy_init();
  if(carl == NULL) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  reset_data(&data, carl);

  test_setopt(carl, CARLOPT_URL, URL);
  test_setopt(carl, CARLOPT_POST, 1L);
  test_setopt(carl, CARLOPT_POSTFIELDSIZE_LARGE,
              (carl_off_t)data.remaining_bytes);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);
  test_setopt(carl, CARLOPT_READFUNCTION, read_callback);
  test_setopt(carl, CARLOPT_READDATA, &data);
  test_setopt(carl, CARLOPT_WRITEFUNCTION, write_callback);
  test_setopt(carl, CARLOPT_WRITEDATA, &data);

  res = perform_and_check_connections(carl,
    "First request without CARLOPT_KEEP_SENDING_ON_ERROR", 1);
  if(res != TEST_ERR_SUCCESS) {
    goto test_cleanup;
  }

  reset_data(&data, carl);

  res = perform_and_check_connections(carl,
    "Second request without CARLOPT_KEEP_SENDING_ON_ERROR", 1);
  if(res != TEST_ERR_SUCCESS) {
    goto test_cleanup;
  }

  test_setopt(carl, CARLOPT_KEEP_SENDING_ON_ERROR, 1L);

  reset_data(&data, carl);

  res = perform_and_check_connections(carl,
    "First request with CARLOPT_KEEP_SENDING_ON_ERROR", 1);
  if(res != TEST_ERR_SUCCESS) {
    goto test_cleanup;
  }

  reset_data(&data, carl);

  res = perform_and_check_connections(carl,
    "Second request with CARLOPT_KEEP_SENDING_ON_ERROR", 0);
  if(res != TEST_ERR_SUCCESS) {
    goto test_cleanup;
  }

  res = TEST_ERR_SUCCESS;

test_cleanup:

  carl_easy_cleanup(carl);

  carl_global_cleanup();

  return (int)res;
}
