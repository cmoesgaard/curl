/*
 * Copyright (c) 2011 - 2020, Jim Hollinger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Jim Hollinger nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* <DESC>
 * A basic RTSP transfer
 * </DESC>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (WIN32)
#  include <conio.h>  /* _getch() */
#else
#  include <termios.h>
#  include <unistd.h>

static int _getch(void)
{
  struct termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}
#endif

#include <carl/carl.h>

#define VERSION_STR  "V1.0"

/* error handling macros */
#define my_carl_easy_setopt(A, B, C)                               \
  do {                                                             \
    res = carl_easy_setopt((A), (B), (C));                         \
    if(res != CARLE_OK)                                            \
      fprintf(stderr, "carl_easy_setopt(%s, %s, %s) failed: %d\n", \
              #A, #B, #C, res);                                    \
  } while(0)

#define my_carl_easy_perform(A)                                         \
  do {                                                                  \
    res = carl_easy_perform(A);                                         \
    if(res != CARLE_OK)                                                 \
      fprintf(stderr, "carl_easy_perform(%s) failed: %d\n", #A, res);   \
  } while(0)

/* send RTSP OPTIONS request */
static void rtsp_options(CARL *carl, const char *uri)
{
  CARLcode res = CARLE_OK;
  printf("\nRTSP: OPTIONS %s\n", uri);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_STREAM_URI, uri);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_REQUEST, (long)CARL_RTSPREQ_OPTIONS);
  my_carl_easy_perform(carl);
}


/* send RTSP DESCRIBE request and write sdp response to a file */
static void rtsp_describe(CARL *carl, const char *uri,
                          const char *sdp_filename)
{
  CARLcode res = CARLE_OK;
  FILE *sdp_fp = fopen(sdp_filename, "wb");
  printf("\nRTSP: DESCRIBE %s\n", uri);
  if(sdp_fp == NULL) {
    fprintf(stderr, "Could not open '%s' for writing\n", sdp_filename);
    sdp_fp = stdout;
  }
  else {
    printf("Writing SDP to '%s'\n", sdp_filename);
  }
  my_carl_easy_setopt(carl, CARLOPT_WRITEDATA, sdp_fp);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_REQUEST, (long)CARL_RTSPREQ_DESCRIBE);
  my_carl_easy_perform(carl);
  my_carl_easy_setopt(carl, CARLOPT_WRITEDATA, stdout);
  if(sdp_fp != stdout) {
    fclose(sdp_fp);
  }
}

/* send RTSP SETUP request */
static void rtsp_setup(CARL *carl, const char *uri, const char *transport)
{
  CARLcode res = CARLE_OK;
  printf("\nRTSP: SETUP %s\n", uri);
  printf("      TRANSPORT %s\n", transport);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_STREAM_URI, uri);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_TRANSPORT, transport);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_REQUEST, (long)CARL_RTSPREQ_SETUP);
  my_carl_easy_perform(carl);
}


/* send RTSP PLAY request */
static void rtsp_play(CARL *carl, const char *uri, const char *range)
{
  CARLcode res = CARLE_OK;
  printf("\nRTSP: PLAY %s\n", uri);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_STREAM_URI, uri);
  my_carl_easy_setopt(carl, CARLOPT_RANGE, range);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_REQUEST, (long)CARL_RTSPREQ_PLAY);
  my_carl_easy_perform(carl);

  /* switch off using range again */
  my_carl_easy_setopt(carl, CARLOPT_RANGE, NULL);
}


/* send RTSP TEARDOWN request */
static void rtsp_teardown(CARL *carl, const char *uri)
{
  CARLcode res = CARLE_OK;
  printf("\nRTSP: TEARDOWN %s\n", uri);
  my_carl_easy_setopt(carl, CARLOPT_RTSP_REQUEST, (long)CARL_RTSPREQ_TEARDOWN);
  my_carl_easy_perform(carl);
}


/* convert url into an sdp filename */
static void get_sdp_filename(const char *url, char *sdp_filename,
                             size_t namelen)
{
  const char *s = strrchr(url, '/');
  strcpy(sdp_filename, "video.sdp");
  if(s != NULL) {
    s++;
    if(s[0] != '\0') {
      snprintf(sdp_filename, namelen, "%s.sdp", s);
    }
  }
}


/* scan sdp file for media control attribute */
static void get_media_control_attribute(const char *sdp_filename,
                                        char *control)
{
  int max_len = 256;
  char *s = malloc(max_len);
  FILE *sdp_fp = fopen(sdp_filename, "rb");
  control[0] = '\0';
  if(sdp_fp != NULL) {
    while(fgets(s, max_len - 2, sdp_fp) != NULL) {
      sscanf(s, " a = control: %s", control);
    }
    fclose(sdp_fp);
  }
  free(s);
}


/* main app */
int main(int argc, char * const argv[])
{
#if 1
  const char *transport = "RTP/AVP;unicast;client_port=1234-1235";  /* UDP */
#else
  /* TCP */
  const char *transport = "RTP/AVP/TCP;unicast;client_port=1234-1235";
#endif
  const char *range = "0.000-";
  int rc = EXIT_SUCCESS;
  char *base_name = NULL;

  printf("\nRTSP request %s\n", VERSION_STR);
  printf("    Project website: "
    "https://github.com/BackupGGCode/rtsprequest\n");
  printf("    Requires carl V7.20 or greater\n\n");

  /* check command line */
  if((argc != 2) && (argc != 3)) {
    base_name = strrchr(argv[0], '/');
    if(base_name == NULL) {
      base_name = strrchr(argv[0], '\\');
    }
    if(base_name == NULL) {
      base_name = argv[0];
    }
    else {
      base_name++;
    }
    printf("Usage:   %s url [transport]\n", base_name);
    printf("         url of video server\n");
    printf("         transport (optional) specifier for media stream"
           " protocol\n");
    printf("         default transport: %s\n", transport);
    printf("Example: %s rtsp://192.168.0.2/media/video1\n\n", base_name);
    rc = EXIT_FAILURE;
  }
  else {
    const char *url = argv[1];
    char *uri = malloc(strlen(url) + 32);
    char *sdp_filename = malloc(strlen(url) + 32);
    char *control = malloc(strlen(url) + 32);
    CARLcode res;
    get_sdp_filename(url, sdp_filename, strlen(url) + 32);
    if(argc == 3) {
      transport = argv[2];
    }

    /* initialize carl */
    res = carl_global_init(CARL_GLOBAL_ALL);
    if(res == CARLE_OK) {
      carl_version_info_data *data = carl_version_info(CARLVERSION_NOW);
      CARL *carl;
      fprintf(stderr, "    carl V%s loaded\n", data->version);

      /* initialize this carl session */
      carl = carl_easy_init();
      if(carl != NULL) {
        my_carl_easy_setopt(carl, CARLOPT_VERBOSE, 0L);
        my_carl_easy_setopt(carl, CARLOPT_NOPROGRESS, 1L);
        my_carl_easy_setopt(carl, CARLOPT_HEADERDATA, stdout);
        my_carl_easy_setopt(carl, CARLOPT_URL, url);

        /* request server options */
        snprintf(uri, strlen(url) + 32, "%s", url);
        rtsp_options(carl, uri);

        /* request session description and write response to sdp file */
        rtsp_describe(carl, uri, sdp_filename);

        /* get media control attribute from sdp file */
        get_media_control_attribute(sdp_filename, control);

        /* setup media stream */
        snprintf(uri, strlen(url) + 32, "%s/%s", url, control);
        rtsp_setup(carl, uri, transport);

        /* start playing media stream */
        snprintf(uri, strlen(url) + 32, "%s/", url);
        rtsp_play(carl, uri, range);
        printf("Playing video, press any key to stop ...");
        _getch();
        printf("\n");

        /* teardown session */
        rtsp_teardown(carl, uri);

        /* cleanup */
        carl_easy_cleanup(carl);
        carl = NULL;
      }
      else {
        fprintf(stderr, "carl_easy_init() failed\n");
      }
      carl_global_cleanup();
    }
    else {
      fprintf(stderr, "carl_global_init(%s) failed: %d\n",
              "CARL_GLOBAL_ALL", res);
    }
    free(control);
    free(sdp_filename);
    free(uri);
  }

  return rc;
}
