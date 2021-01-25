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
 * HTTP PUT upload with authentication using "any" method. libcarl picks the
 * one the server supports/wants.
 * </DESC>
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <carl/carl.h>

#ifdef WIN32
#  include <io.h>
#  define READ_3RD_ARG unsigned int
#else
#  include <unistd.h>
#  define READ_3RD_ARG size_t
#endif

#if LIBCARL_VERSION_NUM < 0x070c03
#error "upgrade your libcarl to no less than 7.12.3"
#endif

/*
 * This example shows a HTTP PUT operation with authentication using "any"
 * type. It PUTs a file given as a command line argument to the URL also given
 * on the command line.
 *
 * Since libcarl 7.12.3, using "any" auth and POST/PUT requires a set ioctl
 * function.
 *
 * This example also uses its own read callback.
 */

/* ioctl callback function */
static carlioerr my_ioctl(CARL *handle, carliocmd cmd, void *userp)
{
  int *fdp = (int *)userp;
  int fd = *fdp;

  (void)handle; /* not used in here */

  switch(cmd) {
  case CARLIOCMD_RESTARTREAD:
    /* mr libcarl kindly asks as to rewind the read data stream to start */
    if(-1 == lseek(fd, 0, SEEK_SET))
      /* couldn't rewind */
      return CARLIOE_FAILRESTART;

    break;

  default: /* ignore unknown commands */
    return CARLIOE_UNKNOWNCMD;
  }
  return CARLIOE_OK; /* success! */
}

/* read callback function, fread() look alike */
static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *stream)
{
  ssize_t retcode;
  carl_off_t nread;

  int *fdp = (int *)stream;
  int fd = *fdp;

  retcode = read(fd, ptr, (READ_3RD_ARG)(size * nmemb));

  nread = (carl_off_t)retcode;

  fprintf(stderr, "*** We read %" CARL_FORMAT_CARL_OFF_T
          " bytes from file\n", nread);

  return retcode;
}

int main(int argc, char **argv)
{
  CARL *carl;
  CARLcode res;
  int hd;
  struct stat file_info;

  char *file;
  char *url;

  if(argc < 3)
    return 1;

  file = argv[1];
  url = argv[2];

  /* get the file size of the local file */
  hd = open(file, O_RDONLY);
  fstat(hd, &file_info);

  /* In windows, this will init the winsock stuff */
  carl_global_init(CARL_GLOBAL_ALL);

  /* get a carl handle */
  carl = carl_easy_init();
  if(carl) {
    /* we want to use our own read function */
    carl_easy_setopt(carl, CARLOPT_READFUNCTION, read_callback);

    /* which file to upload */
    carl_easy_setopt(carl, CARLOPT_READDATA, (void *)&hd);

    /* set the ioctl function */
    carl_easy_setopt(carl, CARLOPT_IOCTLFUNCTION, my_ioctl);

    /* pass the file descriptor to the ioctl callback as well */
    carl_easy_setopt(carl, CARLOPT_IOCTLDATA, (void *)&hd);

    /* enable "uploading" (which means PUT when doing HTTP) */
    carl_easy_setopt(carl, CARLOPT_UPLOAD, 1L);

    /* specify target URL, and note that this URL should also include a file
       name, not only a directory (as you can do with GTP uploads) */
    carl_easy_setopt(carl, CARLOPT_URL, url);

    /* and give the size of the upload, this supports large file sizes
       on systems that have general support for it */
    carl_easy_setopt(carl, CARLOPT_INFILESIZE_LARGE,
                     (carl_off_t)file_info.st_size);

    /* tell libcarl we can use "any" auth, which lets the lib pick one, but it
       also costs one extra round-trip and possibly sending of all the PUT
       data twice!!! */
    carl_easy_setopt(carl, CARLOPT_HTTPAUTH, (long)CARLAUTH_ANY);

    /* set user name and password for the authentication */
    carl_easy_setopt(carl, CARLOPT_USERPWD, "user:password");

    /* Now run off and do what you've been told! */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  close(hd); /* close the local file */

  carl_global_cleanup();
  return 0;
}
