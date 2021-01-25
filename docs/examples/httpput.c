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
 * HTTP PUT with easy interface and read callback
 * </DESC>
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <carl/carl.h>

/*
 * This example shows a HTTP PUT operation. PUTs a file given as a command
 * line argument to the URL also given on the command line.
 *
 * This example also uses its own read callback.
 *
 * Here's an article on how to setup a PUT handler for Apache:
 * http://www.apacheweek.com/features/put
 */

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t retcode;
  carl_off_t nread;

  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */
  retcode = fread(ptr, size, nmemb, stream);

  nread = (carl_off_t)retcode;

  fprintf(stderr, "*** We read %" CARL_FORMAT_CARL_OFF_T
          " bytes from file\n", nread);

  return retcode;
}

int main(int argc, char **argv)
{
  CARL *carl;
  CARLcode res;
  FILE * hd_src;
  struct stat file_info;

  char *file;
  char *url;

  if(argc < 3)
    return 1;

  file = argv[1];
  url = argv[2];

  /* get the file size of the local file */
  stat(file, &file_info);

  /* get a FILE * of the same file, could also be made with
     fdopen() from the previous descriptor, but hey this is just
     an example! */
  hd_src = fopen(file, "rb");

  /* In windows, this will init the winsock stuff */
  carl_global_init(CARL_GLOBAL_ALL);

  /* get a carl handle */
  carl = carl_easy_init();
  if(carl) {
    /* we want to use our own read function */
    carl_easy_setopt(carl, CARLOPT_READFUNCTION, read_callback);

    /* enable uploading (implies PUT over HTTP) */
    carl_easy_setopt(carl, CARLOPT_UPLOAD, 1L);

    /* specify target URL, and note that this URL should include a file
       name, not only a directory */
    carl_easy_setopt(carl, CARLOPT_URL, url);

    /* now specify which file to upload */
    carl_easy_setopt(carl, CARLOPT_READDATA, hd_src);

    /* provide the size of the upload, we specicially typecast the value
       to carl_off_t since we must be sure to use the correct data size */
    carl_easy_setopt(carl, CARLOPT_INFILESIZE_LARGE,
                     (carl_off_t)file_info.st_size);

    /* Now run off and do what you've been told! */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  fclose(hd_src); /* close the local file */

  carl_global_cleanup();
  return 0;
}
