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
/* <DESC>
 * Simple HTTP GET that stores the headers in a separate file
 * </DESC>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <carl/carl.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

int main(void)
{
  CARL *carl_handle;
  static const char *headerfilename = "head.out";
  FILE *headerfile;
  static const char *bodyfilename = "body.out";
  FILE *bodyfile;

  carl_global_init(CARL_GLOBAL_ALL);

  /* init the carl session */
  carl_handle = carl_easy_init();

  /* set URL to get */
  carl_easy_setopt(carl_handle, CARLOPT_URL, "https://example.com");

  /* no progress meter please */
  carl_easy_setopt(carl_handle, CARLOPT_NOPROGRESS, 1L);

  /* send all data to this function  */
  carl_easy_setopt(carl_handle, CARLOPT_WRITEFUNCTION, write_data);

  /* open the header file */
  headerfile = fopen(headerfilename, "wb");
  if(!headerfile) {
    carl_easy_cleanup(carl_handle);
    return -1;
  }

  /* open the body file */
  bodyfile = fopen(bodyfilename, "wb");
  if(!bodyfile) {
    carl_easy_cleanup(carl_handle);
    fclose(headerfile);
    return -1;
  }

  /* we want the headers be written to this file handle */
  carl_easy_setopt(carl_handle, CARLOPT_HEADERDATA, headerfile);

  /* we want the body be written to this file handle instead of stdout */
  carl_easy_setopt(carl_handle, CARLOPT_WRITEDATA, bodyfile);

  /* get it! */
  carl_easy_perform(carl_handle);

  /* close the header file */
  fclose(headerfile);

  /* close the body file */
  fclose(bodyfile);

  /* cleanup carl stuff */
  carl_easy_cleanup(carl_handle);

  return 0;
}
