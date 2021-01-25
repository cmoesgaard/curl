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
 * Download a given URL into a local file named page.out.
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

int main(int argc, char *argv[])
{
  CARL *carl_handle;
  static const char *pagefilename = "page.out";
  FILE *pagefile;

  if(argc < 2) {
    printf("Usage: %s <URL>\n", argv[0]);
    return 1;
  }

  carl_global_init(CARL_GLOBAL_ALL);

  /* init the carl session */
  carl_handle = carl_easy_init();

  /* set URL to get here */
  carl_easy_setopt(carl_handle, CARLOPT_URL, argv[1]);

  /* Switch on full protocol/debug output while testing */
  carl_easy_setopt(carl_handle, CARLOPT_VERBOSE, 1L);

  /* disable progress meter, set to 0L to enable it */
  carl_easy_setopt(carl_handle, CARLOPT_NOPROGRESS, 1L);

  /* send all data to this function  */
  carl_easy_setopt(carl_handle, CARLOPT_WRITEFUNCTION, write_data);

  /* open the file */
  pagefile = fopen(pagefilename, "wb");
  if(pagefile) {

    /* write the page body to this file handle */
    carl_easy_setopt(carl_handle, CARLOPT_WRITEDATA, pagefile);

    /* get it! */
    carl_easy_perform(carl_handle);

    /* close the header file */
    fclose(pagefile);
  }

  /* cleanup carl stuff */
  carl_easy_cleanup(carl_handle);

  carl_global_cleanup();

  return 0;
}
