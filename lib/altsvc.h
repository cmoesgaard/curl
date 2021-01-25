#ifndef HEADER_CARL_ALTSVC_H
#define HEADER_CARL_ALTSVC_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2019 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#if !defined(CARL_DISABLE_HTTP) && !defined(CARL_DISABLE_ALTSVC)
#include <carl/carl.h>
#include "llist.h"

enum alpnid {
  ALPN_none = 0,
  ALPN_h1 = CARLALTSVC_H1,
  ALPN_h2 = CARLALTSVC_H2,
  ALPN_h3 = CARLALTSVC_H3
};

struct althost {
  char *host;
  unsigned short port;
  enum alpnid alpnid;
};

struct altsvc {
  struct althost src;
  struct althost dst;
  time_t expires;
  bool persist;
  int prio;
  struct Curl_llist_element node;
};

struct altsvcinfo {
  char *filename;
  struct Curl_llist list; /* list of entries */
  long flags; /* the publicly set bitmask */
};

const char *Curl_alpnid2str(enum alpnid id);
struct altsvcinfo *Curl_altsvc_init(void);
CARLcode Curl_altsvc_load(struct altsvcinfo *asi, const char *file);
CARLcode Curl_altsvc_save(struct Curl_easy *data,
                          struct altsvcinfo *asi, const char *file);
CARLcode Curl_altsvc_ctrl(struct altsvcinfo *asi, const long ctrl);
void Curl_altsvc_cleanup(struct altsvcinfo **altsvc);
CARLcode Curl_altsvc_parse(struct Curl_easy *data,
                           struct altsvcinfo *altsvc, const char *value,
                           enum alpnid srcalpn, const char *srchost,
                           unsigned short srcport);
bool Curl_altsvc_lookup(struct altsvcinfo *asi,
                        enum alpnid srcalpnid, const char *srchost,
                        int srcport,
                        struct altsvc **dstentry,
                        const int versions); /* CARLALTSVC_H* bits */
#else
/* disabled */
#define Curl_altsvc_save(a,b,c)
#define Curl_altsvc_cleanup(x)
#endif /* !CARL_DISABLE_HTTP && !CARL_DISABLE_ALTSVC */
#endif /* HEADER_CARL_ALTSVC_H */
