#ifndef HEADER_CARL_ARPA_TELNET_H
#define HEADER_CARL_ARPA_TELNET_H
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
#ifndef CARL_DISABLE_TELNET
/*
 * Telnet option defines. Add more here if in need.
 */
#define CARL_TELOPT_BINARY   0  /* binary 8bit data */
#define CARL_TELOPT_ECHO     1  /* just echo! */
#define CARL_TELOPT_SGA      3  /* Suppress Go Ahead */
#define CARL_TELOPT_EXOPL  255  /* EXtended OPtions List */
#define CARL_TELOPT_TTYPE   24  /* Terminal TYPE */
#define CARL_TELOPT_NAWS    31  /* Negotiate About Window Size */
#define CARL_TELOPT_XDISPLOC 35 /* X DISPlay LOCation */

#define CARL_TELOPT_NEW_ENVIRON 39  /* NEW ENVIRONment variables */
#define CARL_NEW_ENV_VAR   0
#define CARL_NEW_ENV_VALUE 1

#ifndef CARL_DISABLE_VERBOSE_STRINGS
/*
 * The telnet options represented as strings
 */
static const char * const telnetoptions[]=
{
  "BINARY",      "ECHO",           "RCP",           "SUPPRESS GO AHEAD",
  "NAME",        "STATUS",         "TIMING MARK",   "RCTE",
  "NAOL",        "NAOP",           "NAOCRD",        "NAOHTS",
  "NAOHTD",      "NAOFFD",         "NAOVTS",        "NAOVTD",
  "NAOLFD",      "EXTEND ASCII",   "LOGOUT",        "BYTE MACRO",
  "DE TERMINAL", "SUPDUP",         "SUPDUP OUTPUT", "SEND LOCATION",
  "TERM TYPE",   "END OF RECORD",  "TACACS UID",    "OUTPUT MARKING",
  "TTYLOC",      "3270 REGIME",    "X3 PAD",        "NAWS",
  "TERM SPEED",  "LFLOW",          "LINEMODE",      "XDISPLOC",
  "OLD-ENVIRON", "AUTHENTICATION", "ENCRYPT",       "NEW-ENVIRON"
};
#endif

#define CARL_TELOPT_MAXIMUM CARL_TELOPT_NEW_ENVIRON

#define CARL_TELOPT_OK(x) ((x) <= CARL_TELOPT_MAXIMUM)
#define CARL_TELOPT(x)    telnetoptions[x]

#define CARL_NTELOPTS 40

/*
 * First some defines
 */
#define CARL_xEOF 236 /* End Of File */
#define CARL_SE   240 /* Sub negotiation End */
#define CARL_NOP  241 /* No OPeration */
#define CARL_DM   242 /* Data Mark */
#define CARL_GA   249 /* Go Ahead, reverse the line */
#define CARL_SB   250 /* SuBnegotiation */
#define CARL_WILL 251 /* Our side WILL use this option */
#define CARL_WONT 252 /* Our side WON'T use this option */
#define CARL_DO   253 /* DO use this option! */
#define CARL_DONT 254 /* DON'T use this option! */
#define CARL_IAC  255 /* Interpret As Command */

#ifndef CARL_DISABLE_VERBOSE_STRINGS
/*
 * Then those numbers represented as strings:
 */
static const char * const telnetcmds[]=
{
  "EOF",  "SUSP",  "ABORT", "EOR",  "SE",
  "NOP",  "DMARK", "BRK",   "IP",   "AO",
  "AYT",  "EC",    "EL",    "GA",   "SB",
  "WILL", "WONT",  "DO",    "DONT", "IAC"
};
#endif

#define CARL_TELCMD_MINIMUM CARL_xEOF /* the first one */
#define CARL_TELCMD_MAXIMUM CARL_IAC  /* surprise, 255 is the last one! ;-) */

#define CARL_TELQUAL_IS   0
#define CARL_TELQUAL_SEND 1
#define CARL_TELQUAL_INFO 2
#define CARL_TELQUAL_NAME 3

#define CARL_TELCMD_OK(x) ( ((unsigned int)(x) >= CARL_TELCMD_MINIMUM) && \
                       ((unsigned int)(x) <= CARL_TELCMD_MAXIMUM) )
#define CARL_TELCMD(x)    telnetcmds[(x)-CARL_TELCMD_MINIMUM]

#endif /* CARL_DISABLE_TELNET */

#endif /* HEADER_CARL_ARPA_TELNET_H */
