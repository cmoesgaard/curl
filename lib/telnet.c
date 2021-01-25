/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#include "carl_setup.h"

#ifndef CARL_DISABLE_TELNET

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "urldata.h"
#include <carl/carl.h>
#include "transfer.h"
#include "sendf.h"
#include "telnet.h"
#include "connect.h"
#include "progress.h"
#include "system_win32.h"
#include "arpa_telnet.h"
#include "select.h"
#include "strcase.h"
#include "warnless.h"

/* The last 3 #include files should be in this order */
#include "carl_printf.h"
#include "carl_memory.h"
#include "memdebug.h"

#define SUBBUFSIZE 512

#define CARL_SB_CLEAR(x)  x->subpointer = x->subbuffer
#define CARL_SB_TERM(x)                                 \
  do {                                                  \
    x->subend = x->subpointer;                          \
    CARL_SB_CLEAR(x);                                   \
  } while(0)
#define CARL_SB_ACCUM(x,c)                                      \
  do {                                                          \
    if(x->subpointer < (x->subbuffer + sizeof(x->subbuffer)))   \
      *x->subpointer++ = (c);                                   \
  } while(0)

#define  CARL_SB_GET(x) ((*x->subpointer++)&0xff)
#define  CARL_SB_LEN(x) (x->subend - x->subpointer)

/* For posterity:
#define  CARL_SB_PEEK(x) ((*x->subpointer)&0xff)
#define  CARL_SB_EOF(x) (x->subpointer >= x->subend) */

#ifdef CARL_DISABLE_VERBOSE_STRINGS
#define printoption(a,b,c,d)  Curl_nop_stmt
#endif

static
CARLcode telrcv(struct Curl_easy *data,
                const unsigned char *inbuf, /* Data received from socket */
                ssize_t count);             /* Number of bytes received */

#ifndef CARL_DISABLE_VERBOSE_STRINGS
static void printoption(struct Curl_easy *data,
                        const char *direction,
                        int cmd, int option);
#endif

static void negotiate(struct Curl_easy *data);
static void send_negotiation(struct Curl_easy *data, int cmd, int option);
static void set_local_option(struct Curl_easy *data,
                             int option, int newstate);
static void set_remote_option(struct Curl_easy *data,
                              int option, int newstate);

static void printsub(struct Curl_easy *data,
                     int direction, unsigned char *pointer,
                     size_t length);
static void suboption(struct Curl_easy *data);
static void sendsuboption(struct Curl_easy *data, int option);

static CARLcode telnet_do(struct Curl_easy *data, bool *done);
static CARLcode telnet_done(struct Curl_easy *data,
                                 CARLcode, bool premature);
static CARLcode send_telnet_data(struct Curl_easy *data,
                                 char *buffer, ssize_t nread);

/* For negotiation compliant to RFC 1143 */
#define CARL_NO          0
#define CARL_YES         1
#define CARL_WANTYES     2
#define CARL_WANTNO      3

#define CARL_EMPTY       0
#define CARL_OPPOSITE    1

/*
 * Telnet receiver states for fsm
 */
typedef enum
{
   CARL_TS_DATA = 0,
   CARL_TS_IAC,
   CARL_TS_WILL,
   CARL_TS_WONT,
   CARL_TS_DO,
   CARL_TS_DONT,
   CARL_TS_CR,
   CARL_TS_SB,   /* sub-option collection */
   CARL_TS_SE   /* looking for sub-option end */
} TelnetReceive;

struct TELNET {
  int please_negotiate;
  int already_negotiated;
  int us[256];
  int usq[256];
  int us_preferred[256];
  int him[256];
  int himq[256];
  int him_preferred[256];
  int subnegotiation[256];
  char subopt_ttype[32];             /* Set with suboption TTYPE */
  char subopt_xdisploc[128];         /* Set with suboption XDISPLOC */
  unsigned short subopt_wsx;         /* Set with suboption NAWS */
  unsigned short subopt_wsy;         /* Set with suboption NAWS */
  TelnetReceive telrcv_state;
  struct carl_slist *telnet_vars;    /* Environment variables */

  /* suboptions */
  unsigned char subbuffer[SUBBUFSIZE];
  unsigned char *subpointer, *subend;      /* buffer for sub-options */
};


/*
 * TELNET protocol handler.
 */

const struct Curl_handler Curl_handler_telnet = {
  "TELNET",                             /* scheme */
  ZERO_NULL,                            /* setup_connection */
  telnet_do,                            /* do_it */
  telnet_done,                          /* done */
  ZERO_NULL,                            /* do_more */
  ZERO_NULL,                            /* connect_it */
  ZERO_NULL,                            /* connecting */
  ZERO_NULL,                            /* doing */
  ZERO_NULL,                            /* proto_getsock */
  ZERO_NULL,                            /* doing_getsock */
  ZERO_NULL,                            /* domore_getsock */
  ZERO_NULL,                            /* perform_getsock */
  ZERO_NULL,                            /* disconnect */
  ZERO_NULL,                            /* readwrite */
  ZERO_NULL,                            /* connection_check */
  PORT_TELNET,                          /* defport */
  CARLPROTO_TELNET,                     /* protocol */
  CARLPROTO_TELNET,                     /* family */
  PROTOPT_NONE | PROTOPT_NOURLQUERY     /* flags */
};


static
CARLcode init_telnet(struct Curl_easy *data)
{
  struct TELNET *tn;

  tn = calloc(1, sizeof(struct TELNET));
  if(!tn)
    return CARLE_OUT_OF_MEMORY;

  data->req.p.telnet = tn; /* make us known */

  tn->telrcv_state = CARL_TS_DATA;

  /* Init suboptions */
  CARL_SB_CLEAR(tn);

  /* Set the options we want by default */
  tn->us_preferred[CARL_TELOPT_SGA] = CARL_YES;
  tn->him_preferred[CARL_TELOPT_SGA] = CARL_YES;

  /* To be compliant with previous releases of libcarl
     we enable this option by default. This behavior
         can be changed thanks to the "BINARY" option in
         CARLOPT_TELNETOPTIONS
  */
  tn->us_preferred[CARL_TELOPT_BINARY] = CARL_YES;
  tn->him_preferred[CARL_TELOPT_BINARY] = CARL_YES;

  /* We must allow the server to echo what we sent
         but it is not necessary to request the server
         to do so (it might forces the server to close
         the connection). Hence, we ignore ECHO in the
         negotiate function
  */
  tn->him_preferred[CARL_TELOPT_ECHO] = CARL_YES;

  /* Set the subnegotiation fields to send information
    just after negotiation passed (do/will)

     Default values are (0,0) initialized by calloc.
     According to the RFC1013 it is valid:
     A value equal to zero is acceptable for the width (or height),
         and means that no character width (or height) is being sent.
         In this case, the width (or height) that will be assumed by the
         Telnet server is operating system specific (it will probably be
         based upon the terminal type information that may have been sent
         using the TERMINAL TYPE Telnet option). */
  tn->subnegotiation[CARL_TELOPT_NAWS] = CARL_YES;
  return CARLE_OK;
}

static void negotiate(struct Curl_easy *data)
{
  int i;
  struct TELNET *tn = data->req.p.telnet;

  for(i = 0; i < CARL_NTELOPTS; i++) {
    if(i == CARL_TELOPT_ECHO)
      continue;

    if(tn->us_preferred[i] == CARL_YES)
      set_local_option(data, i, CARL_YES);

    if(tn->him_preferred[i] == CARL_YES)
      set_remote_option(data, i, CARL_YES);
  }
}

#ifndef CARL_DISABLE_VERBOSE_STRINGS
static void printoption(struct Curl_easy *data,
                        const char *direction, int cmd, int option)
{
  if(data->set.verbose) {
    if(cmd == CARL_IAC) {
      if(CARL_TELCMD_OK(option))
        infof(data, "%s IAC %s\n", direction, CARL_TELCMD(option));
      else
        infof(data, "%s IAC %d\n", direction, option);
    }
    else {
      const char *fmt = (cmd == CARL_WILL) ? "WILL" :
                        (cmd == CARL_WONT) ? "WONT" :
                        (cmd == CARL_DO) ? "DO" :
                        (cmd == CARL_DONT) ? "DONT" : 0;
      if(fmt) {
        const char *opt;
        if(CARL_TELOPT_OK(option))
          opt = CARL_TELOPT(option);
        else if(option == CARL_TELOPT_EXOPL)
          opt = "EXOPL";
        else
          opt = NULL;

        if(opt)
          infof(data, "%s %s %s\n", direction, fmt, opt);
        else
          infof(data, "%s %s %d\n", direction, fmt, option);
      }
      else
        infof(data, "%s %d %d\n", direction, cmd, option);
    }
  }
}
#endif

static void send_negotiation(struct Curl_easy *data, int cmd, int option)
{
  unsigned char buf[3];
  ssize_t bytes_written;
  struct connectdata *conn = data->conn;

  buf[0] = CARL_IAC;
  buf[1] = (unsigned char)cmd;
  buf[2] = (unsigned char)option;

  bytes_written = swrite(conn->sock[FIRSTSOCKET], buf, 3);
  if(bytes_written < 0) {
    int err = SOCKERRNO;
    failf(data,"Sending data failed (%d)",err);
  }

  printoption(data, "SENT", cmd, option);
}

static
void set_remote_option(struct Curl_easy *data, int option, int newstate)
{
  struct TELNET *tn = data->req.p.telnet;
  if(newstate == CARL_YES) {
    switch(tn->him[option]) {
    case CARL_NO:
      tn->him[option] = CARL_WANTYES;
      send_negotiation(data, CARL_DO, option);
      break;

    case CARL_YES:
      /* Already enabled */
      break;

    case CARL_WANTNO:
      switch(tn->himq[option]) {
      case CARL_EMPTY:
        /* Already negotiating for CARL_YES, queue the request */
        tn->himq[option] = CARL_OPPOSITE;
        break;
      case CARL_OPPOSITE:
        /* Error: already queued an enable request */
        break;
      }
      break;

    case CARL_WANTYES:
      switch(tn->himq[option]) {
      case CARL_EMPTY:
        /* Error: already negotiating for enable */
        break;
      case CARL_OPPOSITE:
        tn->himq[option] = CARL_EMPTY;
        break;
      }
      break;
    }
  }
  else { /* NO */
    switch(tn->him[option]) {
    case CARL_NO:
      /* Already disabled */
      break;

    case CARL_YES:
      tn->him[option] = CARL_WANTNO;
      send_negotiation(data, CARL_DONT, option);
      break;

    case CARL_WANTNO:
      switch(tn->himq[option]) {
      case CARL_EMPTY:
        /* Already negotiating for NO */
        break;
      case CARL_OPPOSITE:
        tn->himq[option] = CARL_EMPTY;
        break;
      }
      break;

    case CARL_WANTYES:
      switch(tn->himq[option]) {
      case CARL_EMPTY:
        tn->himq[option] = CARL_OPPOSITE;
        break;
      case CARL_OPPOSITE:
        break;
      }
      break;
    }
  }
}

static
void rec_will(struct Curl_easy *data, int option)
{
  struct TELNET *tn = data->req.p.telnet;
  switch(tn->him[option]) {
  case CARL_NO:
    if(tn->him_preferred[option] == CARL_YES) {
      tn->him[option] = CARL_YES;
      send_negotiation(data, CARL_DO, option);
    }
    else
      send_negotiation(data, CARL_DONT, option);

    break;

  case CARL_YES:
    /* Already enabled */
    break;

  case CARL_WANTNO:
    switch(tn->himq[option]) {
    case CARL_EMPTY:
      /* Error: DONT answered by WILL */
      tn->him[option] = CARL_NO;
      break;
    case CARL_OPPOSITE:
      /* Error: DONT answered by WILL */
      tn->him[option] = CARL_YES;
      tn->himq[option] = CARL_EMPTY;
      break;
    }
    break;

  case CARL_WANTYES:
    switch(tn->himq[option]) {
    case CARL_EMPTY:
      tn->him[option] = CARL_YES;
      break;
    case CARL_OPPOSITE:
      tn->him[option] = CARL_WANTNO;
      tn->himq[option] = CARL_EMPTY;
      send_negotiation(data, CARL_DONT, option);
      break;
    }
    break;
  }
}

static
void rec_wont(struct Curl_easy *data, int option)
{
  struct TELNET *tn = data->req.p.telnet;
  switch(tn->him[option]) {
  case CARL_NO:
    /* Already disabled */
    break;

  case CARL_YES:
    tn->him[option] = CARL_NO;
    send_negotiation(data, CARL_DONT, option);
    break;

  case CARL_WANTNO:
    switch(tn->himq[option]) {
    case CARL_EMPTY:
      tn->him[option] = CARL_NO;
      break;

    case CARL_OPPOSITE:
      tn->him[option] = CARL_WANTYES;
      tn->himq[option] = CARL_EMPTY;
      send_negotiation(data, CARL_DO, option);
      break;
    }
    break;

  case CARL_WANTYES:
    switch(tn->himq[option]) {
    case CARL_EMPTY:
      tn->him[option] = CARL_NO;
      break;
    case CARL_OPPOSITE:
      tn->him[option] = CARL_NO;
      tn->himq[option] = CARL_EMPTY;
      break;
    }
    break;
  }
}

static void
set_local_option(struct Curl_easy *data, int option, int newstate)
{
  struct TELNET *tn = data->req.p.telnet;
  if(newstate == CARL_YES) {
    switch(tn->us[option]) {
    case CARL_NO:
      tn->us[option] = CARL_WANTYES;
      send_negotiation(data, CARL_WILL, option);
      break;

    case CARL_YES:
      /* Already enabled */
      break;

    case CARL_WANTNO:
      switch(tn->usq[option]) {
      case CARL_EMPTY:
        /* Already negotiating for CARL_YES, queue the request */
        tn->usq[option] = CARL_OPPOSITE;
        break;
      case CARL_OPPOSITE:
        /* Error: already queued an enable request */
        break;
      }
      break;

    case CARL_WANTYES:
      switch(tn->usq[option]) {
      case CARL_EMPTY:
        /* Error: already negotiating for enable */
        break;
      case CARL_OPPOSITE:
        tn->usq[option] = CARL_EMPTY;
        break;
      }
      break;
    }
  }
  else { /* NO */
    switch(tn->us[option]) {
    case CARL_NO:
      /* Already disabled */
      break;

    case CARL_YES:
      tn->us[option] = CARL_WANTNO;
      send_negotiation(data, CARL_WONT, option);
      break;

    case CARL_WANTNO:
      switch(tn->usq[option]) {
      case CARL_EMPTY:
        /* Already negotiating for NO */
        break;
      case CARL_OPPOSITE:
        tn->usq[option] = CARL_EMPTY;
        break;
      }
      break;

    case CARL_WANTYES:
      switch(tn->usq[option]) {
      case CARL_EMPTY:
        tn->usq[option] = CARL_OPPOSITE;
        break;
      case CARL_OPPOSITE:
        break;
      }
      break;
    }
  }
}

static
void rec_do(struct Curl_easy *data, int option)
{
  struct TELNET *tn = data->req.p.telnet;
  switch(tn->us[option]) {
  case CARL_NO:
    if(tn->us_preferred[option] == CARL_YES) {
      tn->us[option] = CARL_YES;
      send_negotiation(data, CARL_WILL, option);
      if(tn->subnegotiation[option] == CARL_YES)
        /* transmission of data option */
        sendsuboption(data, option);
    }
    else if(tn->subnegotiation[option] == CARL_YES) {
      /* send information to achieve this option*/
      tn->us[option] = CARL_YES;
      send_negotiation(data, CARL_WILL, option);
      sendsuboption(data, option);
    }
    else
      send_negotiation(data, CARL_WONT, option);
    break;

  case CARL_YES:
    /* Already enabled */
    break;

  case CARL_WANTNO:
    switch(tn->usq[option]) {
    case CARL_EMPTY:
      /* Error: DONT answered by WILL */
      tn->us[option] = CARL_NO;
      break;
    case CARL_OPPOSITE:
      /* Error: DONT answered by WILL */
      tn->us[option] = CARL_YES;
      tn->usq[option] = CARL_EMPTY;
      break;
    }
    break;

  case CARL_WANTYES:
    switch(tn->usq[option]) {
    case CARL_EMPTY:
      tn->us[option] = CARL_YES;
      if(tn->subnegotiation[option] == CARL_YES) {
        /* transmission of data option */
        sendsuboption(data, option);
      }
      break;
    case CARL_OPPOSITE:
      tn->us[option] = CARL_WANTNO;
      tn->himq[option] = CARL_EMPTY;
      send_negotiation(data, CARL_WONT, option);
      break;
    }
    break;
  }
}

static
void rec_dont(struct Curl_easy *data, int option)
{
  struct TELNET *tn = data->req.p.telnet;
  switch(tn->us[option]) {
  case CARL_NO:
    /* Already disabled */
    break;

  case CARL_YES:
    tn->us[option] = CARL_NO;
    send_negotiation(data, CARL_WONT, option);
    break;

  case CARL_WANTNO:
    switch(tn->usq[option]) {
    case CARL_EMPTY:
      tn->us[option] = CARL_NO;
      break;

    case CARL_OPPOSITE:
      tn->us[option] = CARL_WANTYES;
      tn->usq[option] = CARL_EMPTY;
      send_negotiation(data, CARL_WILL, option);
      break;
    }
    break;

  case CARL_WANTYES:
    switch(tn->usq[option]) {
    case CARL_EMPTY:
      tn->us[option] = CARL_NO;
      break;
    case CARL_OPPOSITE:
      tn->us[option] = CARL_NO;
      tn->usq[option] = CARL_EMPTY;
      break;
    }
    break;
  }
}


static void printsub(struct Curl_easy *data,
                     int direction,             /* '<' or '>' */
                     unsigned char *pointer,    /* where suboption data is */
                     size_t length)             /* length of suboption data */
{
  if(data->set.verbose) {
    unsigned int i = 0;
    if(direction) {
      infof(data, "%s IAC SB ", (direction == '<')? "RCVD":"SENT");
      if(length >= 3) {
        int j;

        i = pointer[length-2];
        j = pointer[length-1];

        if(i != CARL_IAC || j != CARL_SE) {
          infof(data, "(terminated by ");
          if(CARL_TELOPT_OK(i))
            infof(data, "%s ", CARL_TELOPT(i));
          else if(CARL_TELCMD_OK(i))
            infof(data, "%s ", CARL_TELCMD(i));
          else
            infof(data, "%u ", i);
          if(CARL_TELOPT_OK(j))
            infof(data, "%s", CARL_TELOPT(j));
          else if(CARL_TELCMD_OK(j))
            infof(data, "%s", CARL_TELCMD(j));
          else
            infof(data, "%d", j);
          infof(data, ", not IAC SE!) ");
        }
      }
      length -= 2;
    }
    if(length < 1) {
      infof(data, "(Empty suboption?)");
      return;
    }

    if(CARL_TELOPT_OK(pointer[0])) {
      switch(pointer[0]) {
      case CARL_TELOPT_TTYPE:
      case CARL_TELOPT_XDISPLOC:
      case CARL_TELOPT_NEW_ENVIRON:
      case CARL_TELOPT_NAWS:
        infof(data, "%s", CARL_TELOPT(pointer[0]));
        break;
      default:
        infof(data, "%s (unsupported)", CARL_TELOPT(pointer[0]));
        break;
      }
    }
    else
      infof(data, "%d (unknown)", pointer[i]);

    switch(pointer[0]) {
    case CARL_TELOPT_NAWS:
      if(length > 4)
        infof(data, "Width: %d ; Height: %d", (pointer[1]<<8) | pointer[2],
              (pointer[3]<<8) | pointer[4]);
      break;
    default:
      switch(pointer[1]) {
      case CARL_TELQUAL_IS:
        infof(data, " IS");
        break;
      case CARL_TELQUAL_SEND:
        infof(data, " SEND");
        break;
      case CARL_TELQUAL_INFO:
        infof(data, " INFO/REPLY");
        break;
      case CARL_TELQUAL_NAME:
        infof(data, " NAME");
        break;
      }

      switch(pointer[0]) {
      case CARL_TELOPT_TTYPE:
      case CARL_TELOPT_XDISPLOC:
        pointer[length] = 0;
        infof(data, " \"%s\"", &pointer[2]);
        break;
      case CARL_TELOPT_NEW_ENVIRON:
        if(pointer[1] == CARL_TELQUAL_IS) {
          infof(data, " ");
          for(i = 3; i < length; i++) {
            switch(pointer[i]) {
            case CARL_NEW_ENV_VAR:
              infof(data, ", ");
              break;
            case CARL_NEW_ENV_VALUE:
              infof(data, " = ");
              break;
            default:
              infof(data, "%c", pointer[i]);
              break;
            }
          }
        }
        break;
      default:
        for(i = 2; i < length; i++)
          infof(data, " %.2x", pointer[i]);
        break;
      }
    }
    if(direction)
      infof(data, "\n");
  }
}

static CARLcode check_telnet_options(struct Curl_easy *data)
{
  struct carl_slist *head;
  struct carl_slist *beg;
  char option_keyword[128] = "";
  char option_arg[256] = "";
  struct TELNET *tn = data->req.p.telnet;
  struct connectdata *conn = data->conn;
  CARLcode result = CARLE_OK;
  int binary_option;

  /* Add the user name as an environment variable if it
     was given on the command line */
  if(conn->bits.user_passwd) {
    msnprintf(option_arg, sizeof(option_arg), "USER,%s", conn->user);
    beg = carl_slist_append(tn->telnet_vars, option_arg);
    if(!beg) {
      carl_slist_free_all(tn->telnet_vars);
      tn->telnet_vars = NULL;
      return CARLE_OUT_OF_MEMORY;
    }
    tn->telnet_vars = beg;
    tn->us_preferred[CARL_TELOPT_NEW_ENVIRON] = CARL_YES;
  }

  for(head = data->set.telnet_options; head; head = head->next) {
    if(sscanf(head->data, "%127[^= ]%*[ =]%255s",
              option_keyword, option_arg) == 2) {

      /* Terminal type */
      if(strcasecompare(option_keyword, "TTYPE")) {
        strncpy(tn->subopt_ttype, option_arg, 31);
        tn->subopt_ttype[31] = 0; /* String termination */
        tn->us_preferred[CARL_TELOPT_TTYPE] = CARL_YES;
        continue;
      }

      /* Display variable */
      if(strcasecompare(option_keyword, "XDISPLOC")) {
        strncpy(tn->subopt_xdisploc, option_arg, 127);
        tn->subopt_xdisploc[127] = 0; /* String termination */
        tn->us_preferred[CARL_TELOPT_XDISPLOC] = CARL_YES;
        continue;
      }

      /* Environment variable */
      if(strcasecompare(option_keyword, "NEW_ENV")) {
        beg = carl_slist_append(tn->telnet_vars, option_arg);
        if(!beg) {
          result = CARLE_OUT_OF_MEMORY;
          break;
        }
        tn->telnet_vars = beg;
        tn->us_preferred[CARL_TELOPT_NEW_ENVIRON] = CARL_YES;
        continue;
      }

      /* Window Size */
      if(strcasecompare(option_keyword, "WS")) {
        if(sscanf(option_arg, "%hu%*[xX]%hu",
                  &tn->subopt_wsx, &tn->subopt_wsy) == 2)
          tn->us_preferred[CARL_TELOPT_NAWS] = CARL_YES;
        else {
          failf(data, "Syntax error in telnet option: %s", head->data);
          result = CARLE_TELNET_OPTION_SYNTAX;
          break;
        }
        continue;
      }

      /* To take care or not of the 8th bit in data exchange */
      if(strcasecompare(option_keyword, "BINARY")) {
        binary_option = atoi(option_arg);
        if(binary_option != 1) {
          tn->us_preferred[CARL_TELOPT_BINARY] = CARL_NO;
          tn->him_preferred[CARL_TELOPT_BINARY] = CARL_NO;
        }
        continue;
      }

      failf(data, "Unknown telnet option %s", head->data);
      result = CARLE_UNKNOWN_OPTION;
      break;
    }
    failf(data, "Syntax error in telnet option: %s", head->data);
    result = CARLE_TELNET_OPTION_SYNTAX;
    break;
  }

  if(result) {
    carl_slist_free_all(tn->telnet_vars);
    tn->telnet_vars = NULL;
  }

  return result;
}

/*
 * suboption()
 *
 * Look at the sub-option buffer, and try to be helpful to the other
 * side.
 */

static void suboption(struct Curl_easy *data)
{
  struct carl_slist *v;
  unsigned char temp[2048];
  ssize_t bytes_written;
  size_t len;
  int err;
  char varname[128] = "";
  char varval[128] = "";
  struct TELNET *tn = data->req.p.telnet;
  struct connectdata *conn = data->conn;

  printsub(data, '<', (unsigned char *)tn->subbuffer, CARL_SB_LEN(tn) + 2);
  switch(CARL_SB_GET(tn)) {
    case CARL_TELOPT_TTYPE:
      len = strlen(tn->subopt_ttype) + 4 + 2;
      msnprintf((char *)temp, sizeof(temp),
                "%c%c%c%c%s%c%c", CARL_IAC, CARL_SB, CARL_TELOPT_TTYPE,
                CARL_TELQUAL_IS, tn->subopt_ttype, CARL_IAC, CARL_SE);
      bytes_written = swrite(conn->sock[FIRSTSOCKET], temp, len);
      if(bytes_written < 0) {
        err = SOCKERRNO;
        failf(data,"Sending data failed (%d)",err);
      }
      printsub(data, '>', &temp[2], len-2);
      break;
    case CARL_TELOPT_XDISPLOC:
      len = strlen(tn->subopt_xdisploc) + 4 + 2;
      msnprintf((char *)temp, sizeof(temp),
                "%c%c%c%c%s%c%c", CARL_IAC, CARL_SB, CARL_TELOPT_XDISPLOC,
                CARL_TELQUAL_IS, tn->subopt_xdisploc, CARL_IAC, CARL_SE);
      bytes_written = swrite(conn->sock[FIRSTSOCKET], temp, len);
      if(bytes_written < 0) {
        err = SOCKERRNO;
        failf(data,"Sending data failed (%d)",err);
      }
      printsub(data, '>', &temp[2], len-2);
      break;
    case CARL_TELOPT_NEW_ENVIRON:
      msnprintf((char *)temp, sizeof(temp),
                "%c%c%c%c", CARL_IAC, CARL_SB, CARL_TELOPT_NEW_ENVIRON,
                CARL_TELQUAL_IS);
      len = 4;

      for(v = tn->telnet_vars; v; v = v->next) {
        size_t tmplen = (strlen(v->data) + 1);
        /* Add the variable only if it fits */
        if(len + tmplen < (int)sizeof(temp)-6) {
          if(sscanf(v->data, "%127[^,],%127s", varname, varval)) {
            msnprintf((char *)&temp[len], sizeof(temp) - len,
                      "%c%s%c%s", CARL_NEW_ENV_VAR, varname,
                      CARL_NEW_ENV_VALUE, varval);
            len += tmplen;
          }
        }
      }
      msnprintf((char *)&temp[len], sizeof(temp) - len,
                "%c%c", CARL_IAC, CARL_SE);
      len += 2;
      bytes_written = swrite(conn->sock[FIRSTSOCKET], temp, len);
      if(bytes_written < 0) {
        err = SOCKERRNO;
        failf(data,"Sending data failed (%d)",err);
      }
      printsub(data, '>', &temp[2], len-2);
      break;
  }
  return;
}


/*
 * sendsuboption()
 *
 * Send suboption information to the server side.
 */

static void sendsuboption(struct Curl_easy *data, int option)
{
  ssize_t bytes_written;
  int err;
  unsigned short x, y;
  unsigned char *uc1, *uc2;
  struct TELNET *tn = data->req.p.telnet;
  struct connectdata *conn = data->conn;

  switch(option) {
  case CARL_TELOPT_NAWS:
    /* We prepare data to be sent */
    CARL_SB_CLEAR(tn);
    CARL_SB_ACCUM(tn, CARL_IAC);
    CARL_SB_ACCUM(tn, CARL_SB);
    CARL_SB_ACCUM(tn, CARL_TELOPT_NAWS);
    /* We must deal either with little or big endian processors */
    /* Window size must be sent according to the 'network order' */
    x = htons(tn->subopt_wsx);
    y = htons(tn->subopt_wsy);
    uc1 = (unsigned char *)&x;
    uc2 = (unsigned char *)&y;
    CARL_SB_ACCUM(tn, uc1[0]);
    CARL_SB_ACCUM(tn, uc1[1]);
    CARL_SB_ACCUM(tn, uc2[0]);
    CARL_SB_ACCUM(tn, uc2[1]);

    CARL_SB_ACCUM(tn, CARL_IAC);
    CARL_SB_ACCUM(tn, CARL_SE);
    CARL_SB_TERM(tn);
    /* data suboption is now ready */

    printsub(data, '>', (unsigned char *)tn->subbuffer + 2,
             CARL_SB_LEN(tn)-2);

    /* we send the header of the suboption... */
    bytes_written = swrite(conn->sock[FIRSTSOCKET], tn->subbuffer, 3);
    if(bytes_written < 0) {
      err = SOCKERRNO;
      failf(data, "Sending data failed (%d)", err);
    }
    /* ... then the window size with the send_telnet_data() function
       to deal with 0xFF cases ... */
    send_telnet_data(data, (char *)tn->subbuffer + 3, 4);
    /* ... and the footer */
    bytes_written = swrite(conn->sock[FIRSTSOCKET], tn->subbuffer + 7, 2);
    if(bytes_written < 0) {
      err = SOCKERRNO;
      failf(data, "Sending data failed (%d)", err);
    }
    break;
  }
}


static
CARLcode telrcv(struct Curl_easy *data,
                const unsigned char *inbuf, /* Data received from socket */
                ssize_t count)              /* Number of bytes received */
{
  unsigned char c;
  CARLcode result;
  int in = 0;
  int startwrite = -1;
  struct TELNET *tn = data->req.p.telnet;

#define startskipping()                                       \
  if(startwrite >= 0) {                                       \
    result = Curl_client_write(data,                          \
                               CLIENTWRITE_BODY,              \
                               (char *)&inbuf[startwrite],    \
                               in-startwrite);                \
    if(result)                                                \
      return result;                                          \
  }                                                           \
  startwrite = -1

#define writebyte() \
    if(startwrite < 0) \
      startwrite = in

#define bufferflush() startskipping()

  while(count--) {
    c = inbuf[in];

    switch(tn->telrcv_state) {
    case CARL_TS_CR:
      tn->telrcv_state = CARL_TS_DATA;
      if(c == '\0') {
        startskipping();
        break;   /* Ignore \0 after CR */
      }
      writebyte();
      break;

    case CARL_TS_DATA:
      if(c == CARL_IAC) {
        tn->telrcv_state = CARL_TS_IAC;
        startskipping();
        break;
      }
      else if(c == '\r')
        tn->telrcv_state = CARL_TS_CR;
      writebyte();
      break;

    case CARL_TS_IAC:
    process_iac:
      DEBUGASSERT(startwrite < 0);
      switch(c) {
      case CARL_WILL:
        tn->telrcv_state = CARL_TS_WILL;
        break;
      case CARL_WONT:
        tn->telrcv_state = CARL_TS_WONT;
        break;
      case CARL_DO:
        tn->telrcv_state = CARL_TS_DO;
        break;
      case CARL_DONT:
        tn->telrcv_state = CARL_TS_DONT;
        break;
      case CARL_SB:
        CARL_SB_CLEAR(tn);
        tn->telrcv_state = CARL_TS_SB;
        break;
      case CARL_IAC:
        tn->telrcv_state = CARL_TS_DATA;
        writebyte();
        break;
      case CARL_DM:
      case CARL_NOP:
      case CARL_GA:
      default:
        tn->telrcv_state = CARL_TS_DATA;
        printoption(data, "RCVD", CARL_IAC, c);
        break;
      }
      break;

      case CARL_TS_WILL:
        printoption(data, "RCVD", CARL_WILL, c);
        tn->please_negotiate = 1;
        rec_will(data, c);
        tn->telrcv_state = CARL_TS_DATA;
        break;

      case CARL_TS_WONT:
        printoption(data, "RCVD", CARL_WONT, c);
        tn->please_negotiate = 1;
        rec_wont(data, c);
        tn->telrcv_state = CARL_TS_DATA;
        break;

      case CARL_TS_DO:
        printoption(data, "RCVD", CARL_DO, c);
        tn->please_negotiate = 1;
        rec_do(data, c);
        tn->telrcv_state = CARL_TS_DATA;
        break;

      case CARL_TS_DONT:
        printoption(data, "RCVD", CARL_DONT, c);
        tn->please_negotiate = 1;
        rec_dont(data, c);
        tn->telrcv_state = CARL_TS_DATA;
        break;

      case CARL_TS_SB:
        if(c == CARL_IAC)
          tn->telrcv_state = CARL_TS_SE;
        else
          CARL_SB_ACCUM(tn, c);
        break;

      case CARL_TS_SE:
        if(c != CARL_SE) {
          if(c != CARL_IAC) {
            /*
             * This is an error.  We only expect to get "IAC IAC" or "IAC SE".
             * Several things may have happened.  An IAC was not doubled, the
             * IAC SE was left off, or another option got inserted into the
             * suboption are all possibilities.  If we assume that the IAC was
             * not doubled, and really the IAC SE was left off, we could get
             * into an infinite loop here.  So, instead, we terminate the
             * suboption, and process the partial suboption if we can.
             */
            CARL_SB_ACCUM(tn, CARL_IAC);
            CARL_SB_ACCUM(tn, c);
            tn->subpointer -= 2;
            CARL_SB_TERM(tn);

            printoption(data, "In SUBOPTION processing, RCVD", CARL_IAC, c);
            suboption(data);   /* handle sub-option */
            tn->telrcv_state = CARL_TS_IAC;
            goto process_iac;
          }
          CARL_SB_ACCUM(tn, c);
          tn->telrcv_state = CARL_TS_SB;
        }
        else {
          CARL_SB_ACCUM(tn, CARL_IAC);
          CARL_SB_ACCUM(tn, CARL_SE);
          tn->subpointer -= 2;
          CARL_SB_TERM(tn);
          suboption(data);   /* handle sub-option */
          tn->telrcv_state = CARL_TS_DATA;
        }
        break;
    }
    ++in;
  }
  bufferflush();
  return CARLE_OK;
}

/* Escape and send a telnet data block */
static CARLcode send_telnet_data(struct Curl_easy *data,
                                 char *buffer, ssize_t nread)
{
  ssize_t escapes, i, outlen;
  unsigned char *outbuf = NULL;
  CARLcode result = CARLE_OK;
  ssize_t bytes_written, total_written;
  struct connectdata *conn = data->conn;

  /* Determine size of new buffer after escaping */
  escapes = 0;
  for(i = 0; i < nread; i++)
    if((unsigned char)buffer[i] == CARL_IAC)
      escapes++;
  outlen = nread + escapes;

  if(outlen == nread)
    outbuf = (unsigned char *)buffer;
  else {
    ssize_t j;
    outbuf = malloc(nread + escapes + 1);
    if(!outbuf)
      return CARLE_OUT_OF_MEMORY;

    j = 0;
    for(i = 0; i < nread; i++) {
      outbuf[j++] = buffer[i];
      if((unsigned char)buffer[i] == CARL_IAC)
        outbuf[j++] = CARL_IAC;
    }
    outbuf[j] = '\0';
  }

  total_written = 0;
  while(!result && total_written < outlen) {
    /* Make sure socket is writable to avoid EWOULDBLOCK condition */
    struct pollfd pfd[1];
    pfd[0].fd = conn->sock[FIRSTSOCKET];
    pfd[0].events = POLLOUT;
    switch(Curl_poll(pfd, 1, -1)) {
      case -1:                    /* error, abort writing */
      case 0:                     /* timeout (will never happen) */
        result = CARLE_SEND_ERROR;
        break;
      default:                    /* write! */
        bytes_written = 0;
        result = Curl_write(data, conn->sock[FIRSTSOCKET],
                            outbuf + total_written,
                            outlen - total_written,
                            &bytes_written);
        total_written += bytes_written;
        break;
    }
  }

  /* Free malloc copy if escaped */
  if(outbuf != (unsigned char *)buffer)
    free(outbuf);

  return result;
}

static CARLcode telnet_done(struct Curl_easy *data,
                            CARLcode status, bool premature)
{
  struct TELNET *tn = data->req.p.telnet;
  (void)status; /* unused */
  (void)premature; /* not used */

  if(!tn)
    return CARLE_OK;

  carl_slist_free_all(tn->telnet_vars);
  tn->telnet_vars = NULL;

  Curl_safefree(data->req.p.telnet);

  return CARLE_OK;
}

static CARLcode telnet_do(struct Curl_easy *data, bool *done)
{
  CARLcode result;
  struct connectdata *conn = data->conn;
  carl_socket_t sockfd = conn->sock[FIRSTSOCKET];
#ifdef USE_WINSOCK
  WSAEVENT event_handle;
  WSANETWORKEVENTS events;
  HANDLE stdin_handle;
  HANDLE objs[2];
  DWORD  obj_count;
  DWORD  wait_timeout;
  DWORD readfile_read;
  int err;
#else
  timediff_t interval_ms;
  struct pollfd pfd[2];
  int poll_cnt;
  carl_off_t total_dl = 0;
  carl_off_t total_ul = 0;
#endif
  ssize_t nread;
  struct carltime now;
  bool keepon = TRUE;
  char *buf = data->state.buffer;
  struct TELNET *tn;

  *done = TRUE; /* unconditionally */

  result = init_telnet(data);
  if(result)
    return result;

  tn = data->req.p.telnet;

  result = check_telnet_options(data);
  if(result)
    return result;

#ifdef USE_WINSOCK
  /* We want to wait for both stdin and the socket. Since
  ** the select() function in winsock only works on sockets
  ** we have to use the WaitForMultipleObjects() call.
  */

  /* First, create a sockets event object */
  event_handle = WSACreateEvent();
  if(event_handle == WSA_INVALID_EVENT) {
    failf(data, "WSACreateEvent failed (%d)", SOCKERRNO);
    return CARLE_FAILED_INIT;
  }

  /* Tell winsock what events we want to listen to */
  if(WSAEventSelect(sockfd, event_handle, FD_READ|FD_CLOSE) == SOCKET_ERROR) {
    WSACloseEvent(event_handle);
    return CARLE_OK;
  }

  /* The get the Windows file handle for stdin */
  stdin_handle = GetStdHandle(STD_INPUT_HANDLE);

  /* Create the list of objects to wait for */
  objs[0] = event_handle;
  objs[1] = stdin_handle;

  /* If stdin_handle is a pipe, use PeekNamedPipe() method to check it,
     else use the old WaitForMultipleObjects() way */
  if(GetFileType(stdin_handle) == FILE_TYPE_PIPE ||
     data->set.is_fread_set) {
    /* Don't wait for stdin_handle, just wait for event_handle */
    obj_count = 1;
    /* Check stdin_handle per 100 milliseconds */
    wait_timeout = 100;
  }
  else {
    obj_count = 2;
    wait_timeout = 1000;
  }

  /* Keep on listening and act on events */
  while(keepon) {
    const DWORD buf_size = (DWORD)data->set.buffer_size;
    DWORD waitret = WaitForMultipleObjects(obj_count, objs,
                                           FALSE, wait_timeout);
    switch(waitret) {

    case WAIT_TIMEOUT:
    {
      for(;;) {
        if(data->set.is_fread_set) {
          size_t n;
          /* read from user-supplied method */
          n = data->state.fread_func(buf, 1, buf_size, data->state.in);
          if(n == CARL_READFUNC_ABORT) {
            keepon = FALSE;
            result = CARLE_READ_ERROR;
            break;
          }

          if(n == CARL_READFUNC_PAUSE)
            break;

          if(n == 0)                        /* no bytes */
            break;

          /* fall through with number of bytes read */
          readfile_read = (DWORD)n;
        }
        else {
          /* read from stdin */
          if(!PeekNamedPipe(stdin_handle, NULL, 0, NULL,
                            &readfile_read, NULL)) {
            keepon = FALSE;
            result = CARLE_READ_ERROR;
            break;
          }

          if(!readfile_read)
            break;

          if(!ReadFile(stdin_handle, buf, buf_size,
                       &readfile_read, NULL)) {
            keepon = FALSE;
            result = CARLE_READ_ERROR;
            break;
          }
        }

        result = send_telnet_data(data, buf, readfile_read);
        if(result) {
          keepon = FALSE;
          break;
        }
      }
    }
    break;

    case WAIT_OBJECT_0 + 1:
    {
      if(!ReadFile(stdin_handle, buf, buf_size,
                   &readfile_read, NULL)) {
        keepon = FALSE;
        result = CARLE_READ_ERROR;
        break;
      }

      result = send_telnet_data(data, buf, readfile_read);
      if(result) {
        keepon = FALSE;
        break;
      }
    }
    break;

    case WAIT_OBJECT_0:
    {
      events.lNetworkEvents = 0;
      if(WSAEnumNetworkEvents(sockfd, event_handle, &events) == SOCKET_ERROR) {
        err = SOCKERRNO;
        if(err != EINPROGRESS) {
          infof(data, "WSAEnumNetworkEvents failed (%d)", err);
          keepon = FALSE;
          result = CARLE_READ_ERROR;
        }
        break;
      }
      if(events.lNetworkEvents & FD_READ) {
        /* read data from network */
        result = Curl_read(data, sockfd, buf, data->set.buffer_size, &nread);
        /* read would've blocked. Loop again */
        if(result == CARLE_AGAIN)
          break;
        /* returned not-zero, this an error */
        else if(result) {
          keepon = FALSE;
          break;
        }
        /* returned zero but actually received 0 or less here,
           the server closed the connection and we bail out */
        else if(nread <= 0) {
          keepon = FALSE;
          break;
        }

        result = telrcv(data, (unsigned char *) buf, nread);
        if(result) {
          keepon = FALSE;
          break;
        }

        /* Negotiate if the peer has started negotiating,
           otherwise don't. We don't want to speak telnet with
           non-telnet servers, like POP or SMTP. */
        if(tn->please_negotiate && !tn->already_negotiated) {
          negotiate(data);
          tn->already_negotiated = 1;
        }
      }
      if(events.lNetworkEvents & FD_CLOSE) {
        keepon = FALSE;
      }
    }
    break;

    }

    if(data->set.timeout) {
      now = Curl_now();
      if(Curl_timediff(now, conn->created) >= data->set.timeout) {
        failf(data, "Time-out");
        result = CARLE_OPERATION_TIMEDOUT;
        keepon = FALSE;
      }
    }
  }

  /* We called WSACreateEvent, so call WSACloseEvent */
  if(!WSACloseEvent(event_handle)) {
    infof(data, "WSACloseEvent failed (%d)", SOCKERRNO);
  }
#else
  pfd[0].fd = sockfd;
  pfd[0].events = POLLIN;

  if(data->set.is_fread_set) {
    poll_cnt = 1;
    interval_ms = 100; /* poll user-supplied read function */
  }
  else {
    /* really using fread, so infile is a FILE* */
    pfd[1].fd = fileno((FILE *)data->state.in);
    pfd[1].events = POLLIN;
    poll_cnt = 2;
    interval_ms = 1 * 1000;
  }

  while(keepon) {
    switch(Curl_poll(pfd, poll_cnt, interval_ms)) {
    case -1:                    /* error, stop reading */
      keepon = FALSE;
      continue;
    case 0:                     /* timeout */
      pfd[0].revents = 0;
      pfd[1].revents = 0;
      /* FALLTHROUGH */
    default:                    /* read! */
      if(pfd[0].revents & POLLIN) {
        /* read data from network */
        result = Curl_read(data, sockfd, buf, data->set.buffer_size, &nread);
        /* read would've blocked. Loop again */
        if(result == CARLE_AGAIN)
          break;
        /* returned not-zero, this an error */
        if(result) {
          keepon = FALSE;
          break;
        }
        /* returned zero but actually received 0 or less here,
           the server closed the connection and we bail out */
        else if(nread <= 0) {
          keepon = FALSE;
          break;
        }

        total_dl += nread;
        Curl_pgrsSetDownloadCounter(data, total_dl);
        result = telrcv(data, (unsigned char *)buf, nread);
        if(result) {
          keepon = FALSE;
          break;
        }

        /* Negotiate if the peer has started negotiating,
           otherwise don't. We don't want to speak telnet with
           non-telnet servers, like POP or SMTP. */
        if(tn->please_negotiate && !tn->already_negotiated) {
          negotiate(data);
          tn->already_negotiated = 1;
        }
      }

      nread = 0;
      if(poll_cnt == 2) {
        if(pfd[1].revents & POLLIN) { /* read from in file */
          nread = read(pfd[1].fd, buf, data->set.buffer_size);
        }
      }
      else {
        /* read from user-supplied method */
        nread = (int)data->state.fread_func(buf, 1, data->set.buffer_size,
                                            data->state.in);
        if(nread == CARL_READFUNC_ABORT) {
          keepon = FALSE;
          break;
        }
        if(nread == CARL_READFUNC_PAUSE)
          break;
      }

      if(nread > 0) {
        result = send_telnet_data(data, buf, nread);
        if(result) {
          keepon = FALSE;
          break;
        }
        total_ul += nread;
        Curl_pgrsSetUploadCounter(data, total_ul);
      }
      else if(nread < 0)
        keepon = FALSE;

      break;
    } /* poll switch statement */

    if(data->set.timeout) {
      now = Curl_now();
      if(Curl_timediff(now, conn->created) >= data->set.timeout) {
        failf(data, "Time-out");
        result = CARLE_OPERATION_TIMEDOUT;
        keepon = FALSE;
      }
    }

    if(Curl_pgrsUpdate(data)) {
      result = CARLE_ABORTED_BY_CALLBACK;
      break;
    }
  }
#endif
  /* mark this as "no further transfer wanted" */
  Curl_setup_transfer(data, -1, -1, FALSE, -1);

  return result;
}
#endif
