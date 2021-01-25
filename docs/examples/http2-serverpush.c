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
 * HTTP/2 server push
 * </DESC>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* somewhat unix-specific */
#include <sys/time.h>
#include <unistd.h>

/* carl stuff */
#include <carl/carl.h>

#ifndef CARLPIPE_MULTIPLEX
#error "too old libcarl, can't do HTTP/2 server push!"
#endif

static
void dump(const char *text, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;

  unsigned int width = 0x10;

  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;

  fprintf(stderr, "%s, %lu bytes (0x%lx)\n",
          text, (unsigned long)size, (unsigned long)size);

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
  (void)handle; /* prevent compiler warning */
  (void)userp;
  switch(type) {
  case CARLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
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

  dump(text, (unsigned char *)data, size, 1);
  return 0;
}

#define OUTPUTFILE "dl"

static int setup(CARL *hnd)
{
  FILE *out = fopen(OUTPUTFILE, "wb");
  if(!out)
    /* failed */
    return 1;

  /* write to this file */
  carl_easy_setopt(hnd, CARLOPT_WRITEDATA, out);

  /* set the same URL */
  carl_easy_setopt(hnd, CARLOPT_URL, "https://localhost:8443/index.html");

  /* please be verbose */
  carl_easy_setopt(hnd, CARLOPT_VERBOSE, 1L);
  carl_easy_setopt(hnd, CARLOPT_DEBUGFUNCTION, my_trace);

  /* HTTP/2 please */
  carl_easy_setopt(hnd, CARLOPT_HTTP_VERSION, CARL_HTTP_VERSION_2_0);

  /* we use a self-signed test server, skip verification during debugging */
  carl_easy_setopt(hnd, CARLOPT_SSL_VERIFYPEER, 0L);
  carl_easy_setopt(hnd, CARLOPT_SSL_VERIFYHOST, 0L);

#if (CARLPIPE_MULTIPLEX > 0)
  /* wait for pipe connection to confirm */
  carl_easy_setopt(hnd, CARLOPT_PIPEWAIT, 1L);
#endif
  return 0; /* all is good */
}

/* called when there's an incoming push */
static int server_push_callback(CARL *parent,
                                CARL *easy,
                                size_t num_headers,
                                struct carl_pushheaders *headers,
                                void *userp)
{
  char *headp;
  size_t i;
  int *transfers = (int *)userp;
  char filename[128];
  FILE *out;
  static unsigned int count = 0;

  (void)parent; /* we have no use for this */

  snprintf(filename, 128, "push%u", count++);

  /* here's a new stream, save it in a new file for each new push */
  out = fopen(filename, "wb");
  if(!out) {
    /* if we can't save it, deny it */
    fprintf(stderr, "Failed to create output file for push\n");
    return CARL_PUSH_DENY;
  }

  /* write to this file */
  carl_easy_setopt(easy, CARLOPT_WRITEDATA, out);

  fprintf(stderr, "**** push callback approves stream %u, got %lu headers!\n",
          count, (unsigned long)num_headers);

  for(i = 0; i<num_headers; i++) {
    headp = carl_pushheader_bynum(headers, i);
    fprintf(stderr, "**** header %lu: %s\n", (unsigned long)i, headp);
  }

  headp = carl_pushheader_byname(headers, ":path");
  if(headp) {
    fprintf(stderr, "**** The PATH is %s\n", headp /* skip :path + colon */);
  }

  (*transfers)++; /* one more */
  return CARL_PUSH_OK;
}


/*
 * Download a file over HTTP/2, take care of server push.
 */
int main(void)
{
  CARL *easy;
  CARLM *multi_handle;
  int still_running; /* keep number of running handles */
  int transfers = 1; /* we start with one */
  struct CARLMsg *m;

  /* init a multi stack */
  multi_handle = carl_multi_init();

  easy = carl_easy_init();

  /* set options */
  if(setup(easy)) {
    fprintf(stderr, "failed\n");
    return 1;
  }

  /* add the easy transfer */
  carl_multi_add_handle(multi_handle, easy);

  carl_multi_setopt(multi_handle, CARLMOPT_PIPELINING, CARLPIPE_MULTIPLEX);
  carl_multi_setopt(multi_handle, CARLMOPT_PUSHFUNCTION, server_push_callback);
  carl_multi_setopt(multi_handle, CARLMOPT_PUSHDATA, &transfers);

  /* we start some action by calling perform right away */
  carl_multi_perform(multi_handle, &still_running);

  do {
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

    /*
     * A little caution when doing server push is that libcarl itself has
     * created and added one or more easy handles but we need to clean them up
     * when we are done.
     */

    do {
      int msgq = 0;
      m = carl_multi_info_read(multi_handle, &msgq);
      if(m && (m->msg == CARLMSG_DONE)) {
        CARL *e = m->easy_handle;
        transfers--;
        carl_multi_remove_handle(multi_handle, e);
        carl_easy_cleanup(e);
      }
    } while(m);

  } while(transfers); /* as long as we have transfers going */

  carl_multi_cleanup(multi_handle);


  return 0;
}
