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
 * Multiplexed HTTP/2 downloads over a single connection
 * </DESC>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* somewhat unix-specific */
#include <sys/time.h>
#include <unistd.h>

/* carl stuff */
#include <carl/carl.h>
#include <carl/mprintf.h>

#ifndef CARLPIPE_MULTIPLEX
/* This little trick will just make sure that we don't enable pipelining for
   libcarls old enough to not have this symbol. It is _not_ defined to zero in
   a recent libcarl header. */
#define CARLPIPE_MULTIPLEX 0
#endif

struct transfer {
  CARL *easy;
  unsigned int num;
  FILE *out;
};

#define NUM_HANDLES 1000

static
void dump(const char *text, int num, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;

  unsigned int width = 0x10;

  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;

  fprintf(stderr, "%d %s, %lu bytes (0x%lx)\n",
          num, text, (unsigned long)size, (unsigned long)size);

  for(i = 0; i<size; i += width) {

    fprintf(stderr, "%4.4lx: ", (unsigned long)i);

    if(!nohex) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i + c < size)
          fprintf(stderr, "%02x ", ptr[i + c]);
        else
          fputs("   ", stderr);
    }

    for(c = 0; (c < width) && (i + c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
         ptr[i + c + 1] == 0x0A) {
        i += (c + 2 - width);
        break;
      }
      fprintf(stderr, "%c",
              (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
         ptr[i + c + 2] == 0x0A) {
        i += (c + 3 - width);
        break;
      }
    }
    fputc('\n', stderr); /* newline */
  }
}

static
int my_trace(CARL *handle, carl_infotype type,
             char *data, size_t size,
             void *userp)
{
  const char *text;
  struct transfer *t = (struct transfer *)userp;
  unsigned int num = t->num;
  (void)handle; /* prevent compiler warning */

  switch(type) {
  case CARLINFO_TEXT:
    fprintf(stderr, "== %u Info: %s", num, data);
    /* FALLTHROUGH */
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CARLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CARLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CARLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CARLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CARLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CARLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  dump(text, num, (unsigned char *)data, size, 1);
  return 0;
}

static void setup(struct transfer *t, int num)
{
  char filename[128];
  CARL *hnd;

  hnd = t->easy = carl_easy_init();

  carl_msnprintf(filename, 128, "dl-%d", num);

  t->out = fopen(filename, "wb");
  if(!t->out) {
    fprintf(stderr, "error: could not open file %s for writing: %s\n",
            filename, strerror(errno));
    exit(1);
  }

  /* write to this file */
  carl_easy_setopt(hnd, CARLOPT_WRITEDATA, t->out);

  /* set the same URL */
  carl_easy_setopt(hnd, CARLOPT_URL, "https://localhost:8443/index.html");

  /* please be verbose */
  carl_easy_setopt(hnd, CARLOPT_VERBOSE, 1L);
  carl_easy_setopt(hnd, CARLOPT_DEBUGFUNCTION, my_trace);
  carl_easy_setopt(hnd, CARLOPT_DEBUGDATA, t);

  /* HTTP/2 please */
  carl_easy_setopt(hnd, CARLOPT_HTTP_VERSION, CARL_HTTP_VERSION_2_0);

  /* we use a self-signed test server, skip verification during debugging */
  carl_easy_setopt(hnd, CARLOPT_SSL_VERIFYPEER, 0L);
  carl_easy_setopt(hnd, CARLOPT_SSL_VERIFYHOST, 0L);

#if (CARLPIPE_MULTIPLEX > 0)
  /* wait for pipe connection to confirm */
  carl_easy_setopt(hnd, CARLOPT_PIPEWAIT, 1L);
#endif
}

/*
 * Download many transfers over HTTP/2, using the same connection!
 */
int main(int argc, char **argv)
{
  struct transfer trans[NUM_HANDLES];
  CARLM *multi_handle;
  int i;
  int still_running = 0; /* keep number of running handles */
  int num_transfers;
  if(argc > 1) {
    /* if given a number, do that many transfers */
    num_transfers = atoi(argv[1]);
    if((num_transfers < 1) || (num_transfers > NUM_HANDLES))
      num_transfers = 3; /* a suitable low default */
  }
  else
    num_transfers = 3; /* suitable default */

  /* init a multi stack */
  multi_handle = carl_multi_init();

  for(i = 0; i < num_transfers; i++) {
    setup(&trans[i], i);

    /* add the individual transfer */
    carl_multi_add_handle(multi_handle, trans[i].easy);
  }

  carl_multi_setopt(multi_handle, CARLMOPT_PIPELINING, CARLPIPE_MULTIPLEX);

  /* we start some action by calling perform right away */
  carl_multi_perform(multi_handle, &still_running);

  while(still_running) {
    struct timeval timeout;
    int rc; /* select() return code */
    CARLMcode mc; /* carl_multi_fdset() return code */

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;

    long carl_timeo = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    /* set a suitable timeout to play around with */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    carl_multi_timeout(multi_handle, &carl_timeo);
    if(carl_timeo >= 0) {
      timeout.tv_sec = carl_timeo / 1000;
      if(timeout.tv_sec > 1)
        timeout.tv_sec = 1;
      else
        timeout.tv_usec = (carl_timeo % 1000) * 1000;
    }

    /* get file descriptors from the transfers */
    mc = carl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

    if(mc != CARLM_OK) {
      fprintf(stderr, "carl_multi_fdset() failed, code %d.\n", mc);
      break;
    }

    /* On success the value of maxfd is guaranteed to be >= -1. We call
       select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
       no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
       to sleep 100ms, which is the minimum suggested value in the
       carl_multi_fdset() doc. */

    if(maxfd == -1) {
#ifdef _WIN32
      Sleep(100);
      rc = 0;
#else
      /* Portable sleep for platforms other than Windows. */
      struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
      rc = select(0, NULL, NULL, NULL, &wait);
#endif
    }
    else {
      /* Note that on some platforms 'timeout' may be modified by select().
         If you need access to the original value save a copy beforehand. */
      rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    }

    switch(rc) {
    case -1:
      /* select error */
      break;
    case 0:
    default:
      /* timeout or readable/writable sockets */
      carl_multi_perform(multi_handle, &still_running);
      break;
    }
  }

  for(i = 0; i < num_transfers; i++) {
    carl_multi_remove_handle(multi_handle, trans[i].easy);
    carl_easy_cleanup(trans[i].easy);
  }

  carl_multi_cleanup(multi_handle);

  return 0;
}
