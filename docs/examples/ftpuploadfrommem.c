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
 * FTP upload a file from memory
 * </DESC>
 */
#include <stdio.h>
#include <string.h>
#include <carl/carl.h>

static const char data[]=
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
  "Nam rhoncus odio id venenatis volutpat. Vestibulum dapibus "
  "bibendum ullamcorper. Maecenas finibus elit augue, vel "
  "condimentum odio maximus nec. In hac habitasse platea dictumst. "
  "Vestibulum vel dolor et turpis rutrum finibus ac at nulla. "
  "Vivamus nec neque ac elit blandit pretium vitae maximus ipsum. "
  "Quisque sodales magna vel erat auctor, sed pellentesque nisi "
  "rhoncus. Donec vehicula maximus pretium. Aliquam eu tincidunt "
  "lorem.";

struct WriteThis {
  const char *readptr;
  size_t sizeleft;
};

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *upload = (struct WriteThis *)userp;
  size_t max = size*nmemb;

  if(max < 1)
    return 0;

  if(upload->sizeleft) {
    size_t copylen = max;
    if(copylen > upload->sizeleft)
      copylen = upload->sizeleft;
    memcpy(ptr, upload->readptr, copylen);
    upload->readptr += copylen;
    upload->sizeleft -= copylen;
    return copylen;
  }

  return 0;                          /* no more data left to deliver */
}

int main(void)
{
  CARL *carl;
  CARLcode res;

  struct WriteThis upload;

  upload.readptr = data;
  upload.sizeleft = strlen(data);

  /* In windows, this will init the winsock stuff */
  res = carl_global_init(CARL_GLOBAL_DEFAULT);
  /* Check for errors */
  if(res != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed: %s\n",
            carl_easy_strerror(res));
    return 1;
  }

  /* get a carl handle */
  carl = carl_easy_init();
  if(carl) {
    /* First set the URL, the target file */
    carl_easy_setopt(carl, CARLOPT_URL,
                     "ftp://example.com/path/to/upload/file");

    /* User and password for the FTP login */
    carl_easy_setopt(carl, CARLOPT_USERPWD, "login:secret");

    /* Now specify we want to UPLOAD data */
    carl_easy_setopt(carl, CARLOPT_UPLOAD, 1L);

    /* we want to use our own read function */
    carl_easy_setopt(carl, CARLOPT_READFUNCTION, read_callback);

    /* pointer to pass to our read function */
    carl_easy_setopt(carl, CARLOPT_READDATA, &upload);

    /* get verbose debug output please */
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

    /* Set the expected upload size. */
    carl_easy_setopt(carl, CARLOPT_INFILESIZE_LARGE,
                     (carl_off_t)upload.sizeleft);

    /* Perform the request, res will get the return code */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  carl_global_cleanup();
  return 0;
}
