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
 * Upload to a file:// URL
 * </DESC>
 */
#include <stdio.h>
#include <carl/carl.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{
  CARL *carl;
  CARLcode res;
  struct stat file_info;
  carl_off_t speed_upload, total_time;
  FILE *fd;

  fd = fopen("debugit", "rb"); /* open file to upload */
  if(!fd)
    return 1; /* can't continue */

  /* to get the file size */
  if(fstat(fileno(fd), &file_info) != 0)
    return 1; /* can't continue */

  carl = carl_easy_init();
  if(carl) {
    /* upload to this place */
    carl_easy_setopt(carl, CARLOPT_URL,
                     "file:///home/dast/src/carl/debug/new");

    /* tell it to "upload" to the URL */
    carl_easy_setopt(carl, CARLOPT_UPLOAD, 1L);

    /* set where to read from (on Windows you need to use READFUNCTION too) */
    carl_easy_setopt(carl, CARLOPT_READDATA, fd);

    /* and give the size of the upload (optional) */
    carl_easy_setopt(carl, CARLOPT_INFILESIZE_LARGE,
                     (carl_off_t)file_info.st_size);

    /* enable verbose for easier tracing */
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK) {
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    }
    else {
      /* now extract transfer info */
      carl_easy_getinfo(carl, CARLINFO_SPEED_UPLOAD_T, &speed_upload);
      carl_easy_getinfo(carl, CARLINFO_TOTAL_TIME_T, &total_time);

      fprintf(stderr, "Speed: %" CARL_FORMAT_CARL_OFF_T " bytes/sec during %"
              CARL_FORMAT_CARL_OFF_T ".%06ld seconds\n",
              speed_upload,
              (total_time / 1000000), (long)(total_time % 1000000));

    }
    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  fclose(fd);
  return 0;
}
