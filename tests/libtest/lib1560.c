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

/*
 * Note:
 *
 * Since the URL parser by default only accepts schemes that *this instance*
 * of libcarl supports, make sure that the test1560 file lists all the schemes
 * that this test will assume to be present!
 */

#include "test.h"

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h" /* LAST include file */

struct part {
  CARLUPart part;
  const char *name;
};


static int checkparts(CARLU *u, const char *in, const char *wanted,
                      unsigned int getflags)
{
  int i;
  CARLUcode rc;
  char buf[256];
  char *bufp = &buf[0];
  size_t len = sizeof(buf);
  struct part parts[] = {
    {CARLUPART_SCHEME, "scheme"},
    {CARLUPART_USER, "user"},
    {CARLUPART_PASSWORD, "password"},
    {CARLUPART_OPTIONS, "options"},
    {CARLUPART_HOST, "host"},
    {CARLUPART_PORT, "port"},
    {CARLUPART_PATH, "path"},
    {CARLUPART_QUERY, "query"},
    {CARLUPART_FRAGMENT, "fragment"},
    {0, NULL}
  };
  memset(buf, 0, sizeof(buf));

  for(i = 0; parts[i].name; i++) {
    char *p = NULL;
    size_t n;
    rc = carl_url_get(u, parts[i].part, &p, getflags);
    if(!rc && p) {
      msnprintf(bufp, len, "%s%s", buf[0]?" | ":"", p);
    }
    else
      msnprintf(bufp, len, "%s[%d]", buf[0]?" | ":"", (int)rc);

    n = strlen(bufp);
    bufp += n;
    len -= n;
    carl_free(p);
  }
  if(strcmp(buf, wanted)) {
    fprintf(stderr, "in: %s\nwanted: %s\ngot:    %s\n", in, wanted, buf);
    return 1;
  }
  return 0;
}

struct redircase {
  const char *in;
  const char *set;
  const char *out;
  unsigned int urlflags;
  unsigned int setflags;
  CARLUcode ucode;
};

struct setcase {
  const char *in;
  const char *set;
  const char *out;
  unsigned int urlflags;
  unsigned int setflags;
  CARLUcode ucode; /* for the main URL set */
  CARLUcode pcode; /* for updating parts */
};

struct testcase {
  const char *in;
  const char *out;
  unsigned int urlflags;
  unsigned int getflags;
  CARLUcode ucode;
};

struct urltestcase {
  const char *in;
  const char *out;
  unsigned int urlflags; /* pass to carl_url() */
  unsigned int getflags; /* pass to carl_url_get() */
  CARLUcode ucode;
};

struct querycase {
  const char *in;
  const char *q;
  const char *out;
  unsigned int urlflags; /* pass to carl_url() */
  unsigned int qflags; /* pass to carl_url_get() */
  CARLUcode ucode;
};

static struct testcase get_parts_list[] ={
  {"[::1]",
   "http | [11] | [12] | [13] | [::1] | [15] | / | [16] | [17]",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK },
  {"[::]",
   "http | [11] | [12] | [13] | [::] | [15] | / | [16] | [17]",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK },
  {"https://[::1]",
   "https | [11] | [12] | [13] | [::1] | [15] | / | [16] | [17]",
   0, 0, CARLUE_OK },
  {"user:moo@ftp.example.com/color/#green?no-red",
   "ftp | user | moo | [13] | ftp.example.com | [15] | /color/ | [16] | "
   "green?no-red",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK },
  {"ftp.user:moo@example.com/color/#green?no-red",
   "http | ftp.user | moo | [13] | example.com | [15] | /color/ | [16] | "
   "green?no-red",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK },
#ifdef WIN32
  {"file:/C:\\programs\\foo",
   "file | [11] | [12] | [13] | [14] | [15] | C:\\programs\\foo | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"file://C:\\programs\\foo",
   "file | [11] | [12] | [13] | [14] | [15] | C:\\programs\\foo | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"file:///C:\\programs\\foo",
   "file | [11] | [12] | [13] | [14] | [15] | C:\\programs\\foo | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
#endif
  {"https://example.com/color/#green?no-red",
   "https | [11] | [12] | [13] | example.com | [15] | /color/ | [16] | "
   "green?no-red",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK },
  {"https://example.com/color/#green#no-red",
   "https | [11] | [12] | [13] | example.com | [15] | /color/ | [16] | "
   "green#no-red",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK },
  {"https://example.com/color/?green#no-red",
   "https | [11] | [12] | [13] | example.com | [15] | /color/ | green | "
   "no-red",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK },
  {"https://example.com/#color/?green#no-red",
   "https | [11] | [12] | [13] | example.com | [15] | / | [16] | "
   "color/?green#no-red",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK },
  {"https://example.#com/color/?green#no-red",
   "https | [11] | [12] | [13] | example. | [15] | / | [16] | "
   "com/color/?green#no-red",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK },
  {"http://[ab.be:1]/x", "",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_MALFORMED_INPUT},
  {"http://[ab.be]/x", "",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_MALFORMED_INPUT},
  /* URL without host name */
  {"http://a:b@/x", "",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_NO_HOST},
  {"boing:80",
   "https | [11] | [12] | [13] | boing | 80 | / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://[fd00:a41::50]:8080",
   "http | [11] | [12] | [13] | [fd00:a41::50] | 8080 | / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://[fd00:a41::50]/",
   "http | [11] | [12] | [13] | [fd00:a41::50] | [15] | / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://[fd00:a41::50]",
   "http | [11] | [12] | [13] | [fd00:a41::50] | [15] | / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"https://[::1%252]:1234",
   "https | [11] | [12] | [13] | [::1] | 1234 | / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},

  /* here's "bad" zone id */
  {"https://[fe80::20c:29ff:fe9c:409b%eth0]:1234",
   "https | [11] | [12] | [13] | [fe80::20c:29ff:fe9c:409b] | 1234 "
   "| / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"https://127.0.0.1:443",
   "https | [11] | [12] | [13] | 127.0.0.1 | [15] | / | [16] | [17]",
   0, CARLU_NO_DEFAULT_PORT, CARLUE_OK},
  {"http://%3a:%3a@ex%0ample/%3f+?+%3f+%23#+%23%3f%g7",
   "http | : | : | [13] | [6] | [15] | /?+ |  ? # | +#?%g7",
   0, CARLU_URLDECODE, CARLUE_OK},
  {"http://%3a:%3a@ex%0ample/%3f?%3f%35#%35%3f%g7",
   "http | %3a | %3a | [13] | ex%0ample | [15] | /%3f | %3f%35 | %35%3f%g7",
   0, 0, CARLUE_OK},
  {"http://HO0_-st%41/",
   "http | [11] | [12] | [13] | HO0_-st%41 | [15] | / | [16] | [17]",
   0, 0, CARLUE_OK},
  {"file://hello.html",
   "",
   0, 0, CARLUE_MALFORMED_INPUT},
  {"http://HO0_-st/",
   "http | [11] | [12] | [13] | HO0_-st | [15] | / | [16] | [17]",
   0, 0, CARLUE_OK},
  {"imap://user:pass;option@server/path",
   "imap | user | pass | option | server | [15] | /path | [16] | [17]",
   0, 0, CARLUE_OK},
  {"http://user:pass;option@server/path",
   "http | user | pass;option | [13] | server | [15] | /path | [16] | [17]",
   0, 0, CARLUE_OK},
  {"file:/hello.html",
   "file | [11] | [12] | [13] | [14] | [15] | /hello.html | [16] | [17]",
   0, 0, CARLUE_OK},
  {"file://127.0.0.1/hello.html",
   "file | [11] | [12] | [13] | [14] | [15] | /hello.html | [16] | [17]",
   0, 0, CARLUE_OK},
  {"file:////hello.html",
   "file | [11] | [12] | [13] | [14] | [15] | //hello.html | [16] | [17]",
   0, 0, CARLUE_OK},
  {"file:///hello.html",
   "file | [11] | [12] | [13] | [14] | [15] | /hello.html | [16] | [17]",
   0, 0, CARLUE_OK},
  {"https://127.0.0.1",
   "https | [11] | [12] | [13] | 127.0.0.1 | 443 | / | [16] | [17]",
   0, CARLU_DEFAULT_PORT, CARLUE_OK},
  {"https://127.0.0.1",
   "https | [11] | [12] | [13] | 127.0.0.1 | [15] | / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"https://[::1]:1234",
   "https | [11] | [12] | [13] | [::1] | 1234 | / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"https://127abc.com",
   "https | [11] | [12] | [13] | 127abc.com | [15] | / | [16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"https:// example.com?check",
   "",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_MALFORMED_INPUT},
  {"https://e x a m p l e.com?check",
   "",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_MALFORMED_INPUT},
  {"https://example.com?check",
   "https | [11] | [12] | [13] | example.com | [15] | / | check | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"https://example.com:65536",
   "",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_BAD_PORT_NUMBER},
  {"https://example.com:0#moo",
   "",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_BAD_PORT_NUMBER},
  {"https://example.com:01#moo",
   "https | [11] | [12] | [13] | example.com | 1 | / | "
   "[16] | moo",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"https://example.com:1#moo",
   "https | [11] | [12] | [13] | example.com | 1 | / | "
   "[16] | moo",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://example.com#moo",
   "http | [11] | [12] | [13] | example.com | [15] | / | "
   "[16] | moo",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://example.com",
   "http | [11] | [12] | [13] | example.com | [15] | / | "
   "[16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://example.com/path/html",
   "http | [11] | [12] | [13] | example.com | [15] | /path/html | "
   "[16] | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://example.com/path/html?query=name",
   "http | [11] | [12] | [13] | example.com | [15] | /path/html | "
   "query=name | [17]",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://example.com/path/html?query=name#anchor",
   "http | [11] | [12] | [13] | example.com | [15] | /path/html | "
   "query=name | anchor",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://example.com:1234/path/html?query=name#anchor",
   "http | [11] | [12] | [13] | example.com | 1234 | /path/html | "
   "query=name | anchor",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http:///user:password@example.com:1234/path/html?query=name#anchor",
   "http | user | password | [13] | example.com | 1234 | /path/html | "
   "query=name | anchor",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"https://user:password@example.com:1234/path/html?query=name#anchor",
   "https | user | password | [13] | example.com | 1234 | /path/html | "
   "query=name | anchor",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http://user:password@example.com:1234/path/html?query=name#anchor",
   "http | user | password | [13] | example.com | 1234 | /path/html | "
   "query=name | anchor",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http:/user:password@example.com:1234/path/html?query=name#anchor",
   "http | user | password | [13] | example.com | 1234 | /path/html | "
   "query=name | anchor",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"http:////user:password@example.com:1234/path/html?query=name#anchor",
   "",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_MALFORMED_INPUT},
  {NULL, NULL, 0, 0, CARLUE_OK},
};

static struct urltestcase get_url_list[] = {
  /* 40 bytes scheme is the max allowed */
  {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA://hostname/path",
   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa://hostname/path",
   CARLU_NON_SUPPORT_SCHEME, 0, CARLUE_OK},
  /* 41 bytes scheme is not allowed */
  {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA://hostname/path",
   "",
   CARLU_NON_SUPPORT_SCHEME, 0, CARLUE_MALFORMED_INPUT},
  {"https://[fe80::20c:29ff:fe9c:409b%]:1234",
   "",
   0, 0, CARLUE_MALFORMED_INPUT},
  {"https://[fe80::20c:29ff:fe9c:409b%25]:1234",
   "https://[fe80::20c:29ff:fe9c:409b%2525]:1234/",
   0, 0, CARLUE_OK},
  {"https://[fe80::20c:29ff:fe9c:409b%eth0]:1234",
   "https://[fe80::20c:29ff:fe9c:409b%25eth0]:1234/",
   0, 0, CARLUE_OK},
  {"https://[::%25fakeit]/moo",
   "https://[::%25fakeit]/moo",
   0, 0, CARLUE_OK},
  {"smtp.example.com/path/html",
   "smtp://smtp.example.com/path/html",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK},
  {"https.example.com/path/html",
   "http://https.example.com/path/html",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK},
  {"dict.example.com/path/html",
   "dict://dict.example.com/path/html",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK},
  {"pop3.example.com/path/html",
   "pop3://pop3.example.com/path/html",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK},
  {"ldap.example.com/path/html",
   "ldap://ldap.example.com/path/html",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK},
  {"imap.example.com/path/html",
   "imap://imap.example.com/path/html",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK},
  {"ftp.example.com/path/html",
   "ftp://ftp.example.com/path/html",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK},
  {"example.com/path/html",
   "http://example.com/path/html",
   CARLU_GUESS_SCHEME, 0, CARLUE_OK},
  {"HTTP://test/", "http://test/", 0, 0, CARLUE_OK},
  {"http://HO0_-st..~./", "http://HO0_-st..~./", 0, 0, CARLUE_OK},
  {"http:/@example.com: 123/", "", 0, 0, CARLUE_BAD_PORT_NUMBER},
  {"http:/@example.com:123 /", "", 0, 0, CARLUE_BAD_PORT_NUMBER},
  {"http:/@example.com:123a/", "", 0, 0, CARLUE_BAD_PORT_NUMBER},
  {"http://host/file\r", "", 0, 0, CARLUE_MALFORMED_INPUT},
  {"http://host/file\n\x03", "", 0, 0, CARLUE_MALFORMED_INPUT},
  {"htt\x02://host/file", "",
   CARLU_NON_SUPPORT_SCHEME, 0, CARLUE_MALFORMED_INPUT},
  {" http://host/file", "", 0, 0, CARLUE_MALFORMED_INPUT},
  /* here the password ends at the semicolon and options is 'word' */
  {"imap://user:pass;word@host/file",
   "imap://user:pass;word@host/file",
   0, 0, CARLUE_OK},
  /* here the password has the semicolon */
  {"http://user:pass;word@host/file",
   "http://user:pass;word@host/file",
   0, 0, CARLUE_OK},
  {"file:///file.txt#moo",
   "file:///file.txt#moo",
   0, 0, CARLUE_OK},
  {"file:////file.txt",
   "file:////file.txt",
   0, 0, CARLUE_OK},
  {"file:///file.txt",
   "file:///file.txt",
   0, 0, CARLUE_OK},
  {"file:./",
   "file://",
   0, 0, CARLUE_MALFORMED_INPUT},
  {"http://example.com/hello/../here",
   "http://example.com/hello/../here",
   CARLU_PATH_AS_IS, 0, CARLUE_OK},
  {"http://example.com/hello/../here",
   "http://example.com/here",
   0, 0, CARLUE_OK},
  {"http://example.com:80",
   "http://example.com/",
   0, CARLU_NO_DEFAULT_PORT, CARLUE_OK},
  {"tp://example.com/path/html",
   "",
   0, 0, CARLUE_UNSUPPORTED_SCHEME},
  {"http://hello:fool@example.com",
   "",
   CARLU_DISALLOW_USER, 0, CARLUE_USER_NOT_ALLOWED},
  {"http:/@example.com:123",
   "http://example.com:123/",
   0, 0, CARLUE_OK},
  {"http:/:password@example.com",
   "http://:password@example.com/",
   0, 0, CARLUE_OK},
  {"http://user@example.com?#",
   "http://user@example.com/",
   0, 0, CARLUE_OK},
  {"http://user@example.com?",
   "http://user@example.com/",
   0, 0, CARLUE_OK},
  {"http://user@example.com#anchor",
   "http://user@example.com/#anchor",
   0, 0, CARLUE_OK},
  {"example.com/path/html",
   "https://example.com/path/html",
   CARLU_DEFAULT_SCHEME, 0, CARLUE_OK},
  {"example.com/path/html",
   "",
   0, 0, CARLUE_MALFORMED_INPUT},
  {"http://user:password@example.com:1234/path/html?query=name#anchor",
   "http://user:password@example.com:1234/path/html?query=name#anchor",
   0, 0, CARLUE_OK},
  {"http://example.com:1234/path/html?query=name#anchor",
   "http://example.com:1234/path/html?query=name#anchor",
   0, 0, CARLUE_OK},
  {"http://example.com/path/html?query=name#anchor",
   "http://example.com/path/html?query=name#anchor",
   0, 0, CARLUE_OK},
  {"http://example.com/path/html?query=name",
   "http://example.com/path/html?query=name",
   0, 0, CARLUE_OK},
  {"http://example.com/path/html",
   "http://example.com/path/html",
   0, 0, CARLUE_OK},
  {"tp://example.com/path/html",
   "tp://example.com/path/html",
   CARLU_NON_SUPPORT_SCHEME, 0, CARLUE_OK},
  {"custom-scheme://host?expected=test-good",
   "custom-scheme://host/?expected=test-good",
   CARLU_NON_SUPPORT_SCHEME, 0, CARLUE_OK},
  {"custom-scheme://?expected=test-bad",
   "",
   CARLU_NON_SUPPORT_SCHEME, 0, CARLUE_MALFORMED_INPUT},
  {"custom-scheme://?expected=test-new-good",
   "custom-scheme:///?expected=test-new-good",
   CARLU_NON_SUPPORT_SCHEME | CARLU_NO_AUTHORITY, 0, CARLUE_OK},
  {"custom-scheme://host?expected=test-still-good",
   "custom-scheme://host/?expected=test-still-good",
   CARLU_NON_SUPPORT_SCHEME | CARLU_NO_AUTHORITY, 0, CARLUE_OK},
  {NULL, NULL, 0, 0, 0}
};

static int checkurl(const char *url, const char *out)
{
  if(strcmp(out, url)) {
    fprintf(stderr, "Wanted: %s\nGot   : %s\n",
            out, url);
    return 1;
  }
  return 0;
}

/* !checksrc! disable SPACEBEFORECOMMA 1 */
static struct setcase set_parts_list[] = {
  {"https://example.com/",
   "query=Al2cO3tDkcDZ3EWE5Lh+LX8TPHs,", /* contains '+' */
   "https://example.com/?Al2cO3tDkcDZ3EWE5Lh%2bLX8TPHs",
   CARLU_URLDECODE, /* decode on get */
   CARLU_URLENCODE, /* encode on set */
   CARLUE_OK, CARLUE_OK},

  {"https://example.com/",
   /* Set a 41 bytes scheme. That's too long so the old scheme remains set. */
   "scheme=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbc,",
   "https://example.com/",
   0, CARLU_NON_SUPPORT_SCHEME, CARLUE_OK, CARLUE_MALFORMED_INPUT},
  {"https://example.com/",
   /* set a 40 bytes scheme */
   "scheme=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,",
   "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb://example.com/",
   0, CARLU_NON_SUPPORT_SCHEME, CARLUE_OK, CARLUE_OK},
  {"https://[::1%25fake]:1234/",
   "zoneid=NULL,",
   "https://[::1]:1234/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"https://host:1234/",
   "port=NULL,",
   "https://host/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"https://host:1234/",
   "port=\"\",",
   "https://host:1234/",
   0, 0, CARLUE_OK, CARLUE_BAD_PORT_NUMBER},
  {"https://host:1234/",
   "port=56 78,",
   "https://host:1234/",
   0, 0, CARLUE_OK, CARLUE_MALFORMED_INPUT},
  {"https://host:1234/",
   "port=0,",
   "https://host:1234/",
   0, 0, CARLUE_OK, CARLUE_BAD_PORT_NUMBER},
  {"https://host:1234/",
   "port=65535,",
   "https://host:65535/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"https://host:1234/",
   "port=65536,",
   "https://host:1234/",
   0, 0, CARLUE_OK, CARLUE_BAD_PORT_NUMBER},
  {"https://host/",
   "path=%4A%4B%4C,",
   "https://host/%4a%4b%4c",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"https://host/mooo?q#f",
   "path=NULL,query=NULL,fragment=NULL,",
   "https://host/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"https://user:secret@host/",
   "user=NULL,password=NULL,",
   "https://host/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {NULL,
   "scheme=https,user=   @:,host=foobar,",
   "https://%20%20%20%40%3a@foobar/",
   0, CARLU_URLENCODE, CARLUE_OK, CARLUE_OK},
  {NULL,
   "scheme=https,host=  ,path= ,user= ,password= ,query= ,fragment= ,",
   "https://%20:%20@%20%20/%20?+#%20",
   0, CARLU_URLENCODE, CARLUE_OK, CARLUE_OK},
  {NULL,
   "scheme=https,host=foobar,path=/this /path /is /here,",
   "https://foobar/this%20/path%20/is%20/here",
   0, CARLU_URLENCODE, CARLUE_OK, CARLUE_OK},
  {NULL,
   "scheme=https,host=foobar,path=\xc3\xa4\xc3\xb6\xc3\xbc,",
   "https://foobar/%c3%a4%c3%b6%c3%bc",
   0, CARLU_URLENCODE, CARLUE_OK, CARLUE_OK},
  {"imap://user:secret;opt@host/",
   "options=updated,scheme=imaps,password=p4ssw0rd,",
   "imaps://user:p4ssw0rd;updated@host/",
   0, 0, CARLUE_NO_HOST, CARLUE_OK},
  {"imap://user:secret;optit@host/",
   "scheme=https,",
   "https://user:secret@host/",
   0, 0, CARLUE_NO_HOST, CARLUE_OK},
  {"file:///file#anchor",
   "scheme=https,host=example,",
   "https://example/file#anchor",
   0, 0, CARLUE_NO_HOST, CARLUE_OK},
  {NULL, /* start fresh! */
   "scheme=file,host=127.0.0.1,path=/no,user=anonymous,",
   "file:///no",
   0, 0, CARLUE_OK, CARLUE_OK},
  {NULL, /* start fresh! */
   "scheme=ftp,host=127.0.0.1,path=/no,user=anonymous,",
   "ftp://anonymous@127.0.0.1/no",
   0, 0, CARLUE_OK, CARLUE_OK},
  {NULL, /* start fresh! */
   "scheme=https,host=example.com,",
   "https://example.com/",
   0, CARLU_NON_SUPPORT_SCHEME, CARLUE_OK, CARLUE_OK},
  {"http://user:foo@example.com/path?query#frag",
   "fragment=changed,",
   "http://user:foo@example.com/path?query#changed",
   0, CARLU_NON_SUPPORT_SCHEME, CARLUE_OK, CARLUE_OK},
  {"http://example.com/",
   "scheme=foo,", /* not accepted */
   "http://example.com/",
   0, 0, CARLUE_OK, CARLUE_UNSUPPORTED_SCHEME},
  {"http://example.com/",
   "scheme=https,path=/hello,fragment=snippet,",
   "https://example.com/hello#snippet",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"http://example.com:80",
   "user=foo,port=1922,",
   "http://foo@example.com:1922/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"http://example.com:80",
   "user=foo,password=bar,",
   "http://foo:bar@example.com:80/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"http://example.com:80",
   "user=foo,",
   "http://foo@example.com:80/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"http://example.com",
   "host=www.example.com,",
   "http://www.example.com/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"http://example.com:80",
   "scheme=ftp,",
   "ftp://example.com:80/",
   0, 0, CARLUE_OK, CARLUE_OK},
  {"custom-scheme://host",
   "host=\"\",",
   "custom-scheme://host/",
   CARLU_NON_SUPPORT_SCHEME, CARLU_NON_SUPPORT_SCHEME, CARLUE_OK,
   CARLUE_MALFORMED_INPUT},
  {"custom-scheme://host",
   "host=\"\",",
   "custom-scheme:///",
   CARLU_NON_SUPPORT_SCHEME, CARLU_NON_SUPPORT_SCHEME | CARLU_NO_AUTHORITY,
   CARLUE_OK, CARLUE_OK},

  {NULL, NULL, NULL, 0, 0, 0, 0}
};

static CARLUPart part2id(char *part)
{
  if(!strcmp("url", part))
    return CARLUPART_URL;
  if(!strcmp("scheme", part))
    return CARLUPART_SCHEME;
  if(!strcmp("user", part))
    return CARLUPART_USER;
  if(!strcmp("password", part))
    return CARLUPART_PASSWORD;
  if(!strcmp("options", part))
    return CARLUPART_OPTIONS;
  if(!strcmp("host", part))
    return CARLUPART_HOST;
  if(!strcmp("port", part))
    return CARLUPART_PORT;
  if(!strcmp("path", part))
    return CARLUPART_PATH;
  if(!strcmp("query", part))
    return CARLUPART_QUERY;
  if(!strcmp("fragment", part))
    return CARLUPART_FRAGMENT;
  if(!strcmp("zoneid", part))
    return CARLUPART_ZONEID;
  return (CARLUPart)9999; /* bad input => bad output */
}

static CARLUcode updateurl(CARLU *u, const char *cmd, unsigned int setflags)
{
  const char *p = cmd;
  CARLUcode uc;

  /* make sure the last command ends with a comma too! */
  while(p) {
    char *e = strchr(p, ',');
    if(e) {
      size_t n = e-p;
      char buf[80];
      char part[80];
      char value[80];

      memset(part, 0, sizeof(part)); /* Avoid valgrind false positive. */
      memset(value, 0, sizeof(value)); /* Avoid valgrind false positive. */
      memcpy(buf, p, n);
      buf[n] = 0;
      if(2 == sscanf(buf, "%79[^=]=%79[^,]", part, value)) {
        CARLUPart what = part2id(part);
#if 0
        /* for debugging this */
        fprintf(stderr, "%s = %s [%d]\n", part, value, (int)what);
#endif
        if(what > CARLUPART_ZONEID)
          fprintf(stderr, "UNKNOWN part '%s'\n", part);

        if(!strcmp("NULL", value))
          uc = carl_url_set(u, what, NULL, setflags);
        else if(!strcmp("\"\"", value))
          uc = carl_url_set(u, what, "", setflags);
        else
          uc = carl_url_set(u, what, value, setflags);
        if(uc)
          return uc;
      }
      p = e + 1;
      continue;
    }
    break;
  }
  return CARLUE_OK;
}

static struct redircase set_url_list[] = {
  {"http://example.org/static/favicon/wikipedia.ico",
   "//fake.example.com/licenses/by-sa/3.0/",
   "http://fake.example.com/licenses/by-sa/3.0/",
   0, 0, 0},
  {"https://example.org/static/favicon/wikipedia.ico",
   "//fake.example.com/licenses/by-sa/3.0/",
   "https://fake.example.com/licenses/by-sa/3.0/",
   0, 0, 0},
  {"file://localhost/path?query#frag",
   "foo#another",
   "file:///foo#another",
   0, 0, 0},
  {"http://example.com/path?query#frag",
   "https://two.example.com/bradnew",
   "https://two.example.com/bradnew",
   0, 0, 0},
  {"http://example.com/path?query#frag",
   "../../newpage#foo",
   "http://example.com/newpage#foo",
   0, 0, 0},
  {"http://user:foo@example.com/path?query#frag",
   "../../newpage",
   "http://user:foo@example.com/newpage",
   0, 0, 0},
  {"http://user:foo@example.com/path?query#frag",
   "../newpage",
   "http://user:foo@example.com/newpage",
   0, 0, 0},
  {NULL, NULL, NULL, 0, 0, 0}
};

static int set_url(void)
{
  int i;
  int error = 0;

  for(i = 0; set_url_list[i].in && !error; i++) {
    CARLUcode rc;
    CARLU *urlp = carl_url();
    if(!urlp)
      break;
    rc = carl_url_set(urlp, CARLUPART_URL, set_url_list[i].in,
                      set_url_list[i].urlflags);
    if(!rc) {
      rc = carl_url_set(urlp, CARLUPART_URL, set_url_list[i].set,
                        set_url_list[i].setflags);
      if(rc) {
        fprintf(stderr, "%s:%d Set URL %s returned %d\n",
                __FILE__, __LINE__, set_url_list[i].set,
                (int)rc);
        error++;
      }
      else {
        char *url = NULL;
        rc = carl_url_get(urlp, CARLUPART_URL, &url, 0);
        if(rc) {
          fprintf(stderr, "%s:%d Get URL returned %d\n",
                  __FILE__, __LINE__, (int)rc);
          error++;
        }
        else {
          if(checkurl(url, set_url_list[i].out)) {
            error++;
          }
        }
        carl_free(url);
      }
    }
    else if(rc != set_url_list[i].ucode) {
      fprintf(stderr, "Set URL\nin: %s\nreturned %d (expected %d)\n",
              set_url_list[i].in, (int)rc, set_url_list[i].ucode);
      error++;
    }
    carl_url_cleanup(urlp);
  }
  return error;
}

static int set_parts(void)
{
  int i;
  int error = 0;

  for(i = 0; set_parts_list[i].set && !error; i++) {
    CARLUcode rc;
    CARLU *urlp = carl_url();
    if(!urlp) {
      error++;
      break;
    }
    if(set_parts_list[i].in)
      rc = carl_url_set(urlp, CARLUPART_URL, set_parts_list[i].in,
                        set_parts_list[i].urlflags);
    else
      rc = CARLUE_OK;
    if(!rc) {
      char *url = NULL;
      CARLUcode uc = updateurl(urlp, set_parts_list[i].set,
                               set_parts_list[i].setflags);

      if(uc != set_parts_list[i].pcode) {
        fprintf(stderr, "updateurl\nin: %s\nreturned %d (expected %d)\n",
                set_parts_list[i].set, (int)uc, set_parts_list[i].pcode);
        error++;
      }

      rc = carl_url_get(urlp, CARLUPART_URL, &url, 0);

      if(rc) {
        fprintf(stderr, "%s:%d Get URL returned %d\n",
                __FILE__, __LINE__, (int)rc);
        error++;
      }
      else if(checkurl(url, set_parts_list[i].out)) {
        error++;
      }
      carl_free(url);
    }
    else if(rc != set_parts_list[i].ucode) {
      fprintf(stderr, "Set parts\nin: %s\nreturned %d (expected %d)\n",
              set_parts_list[i].in, (int)rc, set_parts_list[i].ucode);
      error++;
    }
    carl_url_cleanup(urlp);
  }
  return error;
}

static int get_url(void)
{
  int i;
  int error = 0;
  for(i = 0; get_url_list[i].in && !error; i++) {
    CARLUcode rc;
    CARLU *urlp = carl_url();
    if(!urlp) {
      error++;
      break;
    }
    rc = carl_url_set(urlp, CARLUPART_URL, get_url_list[i].in,
                      get_url_list[i].urlflags);
    if(!rc) {
      char *url = NULL;
      rc = carl_url_get(urlp, CARLUPART_URL, &url, get_url_list[i].getflags);

      if(rc) {
        fprintf(stderr, "%s:%d returned %d\n",
                __FILE__, __LINE__, (int)rc);
        error++;
      }
      else {
        if(checkurl(url, get_url_list[i].out)) {
          error++;
        }
      }
      carl_free(url);
    }
    else if(rc != get_url_list[i].ucode) {
      fprintf(stderr, "Get URL\nin: %s\nreturned %d (expected %d)\n",
              get_url_list[i].in, (int)rc, get_url_list[i].ucode);
      error++;
    }
    carl_url_cleanup(urlp);
  }
  return error;
}

static int get_parts(void)
{
  int i;
  int error = 0;
  for(i = 0; get_parts_list[i].in && !error; i++) {
    CARLUcode rc;
    CARLU *urlp = carl_url();
    if(!urlp) {
      error++;
      break;
    }
    rc = carl_url_set(urlp, CARLUPART_URL,
                      get_parts_list[i].in,
                      get_parts_list[i].urlflags);
    if(rc != get_parts_list[i].ucode) {
      fprintf(stderr, "Get parts\nin: %s\nreturned %d (expected %d)\n",
              get_parts_list[i].in, (int)rc, get_parts_list[i].ucode);
      error++;
    }
    else if(get_parts_list[i].ucode) {
      /* the expected error happened */
    }
    else if(checkparts(urlp, get_parts_list[i].in, get_parts_list[i].out,
                       get_parts_list[i].getflags))
      error++;
    carl_url_cleanup(urlp);
  }
  return error;
}

static struct querycase append_list[] = {
  {"HTTP://test/?s", "name=joe\x02", "http://test/?s&name=joe%02",
   0, CARLU_URLENCODE, CARLUE_OK},
  {"HTTP://test/?size=2#f", "name=joe=", "http://test/?size=2&name=joe%3d#f",
   0, CARLU_URLENCODE, CARLUE_OK},
  {"HTTP://test/?size=2#f", "name=joe doe",
   "http://test/?size=2&name=joe+doe#f",
   0, CARLU_URLENCODE, CARLUE_OK},
  {"HTTP://test/", "name=joe", "http://test/?name=joe", 0, 0, CARLUE_OK},
  {"HTTP://test/?size=2", "name=joe", "http://test/?size=2&name=joe",
   0, 0, CARLUE_OK},
  {"HTTP://test/?size=2&", "name=joe", "http://test/?size=2&name=joe",
   0, 0, CARLUE_OK},
  {"HTTP://test/?size=2#f", "name=joe", "http://test/?size=2&name=joe#f",
   0, 0, CARLUE_OK},
  {NULL, NULL, NULL, 0, 0, 0}
};

static int append(void)
{
  int i;
  int error = 0;
  for(i = 0; append_list[i].in && !error; i++) {
    CARLUcode rc;
    CARLU *urlp = carl_url();
    if(!urlp) {
      error++;
      break;
    }
    rc = carl_url_set(urlp, CARLUPART_URL,
                      append_list[i].in,
                      append_list[i].urlflags);
    if(rc)
      error++;
    else
      rc = carl_url_set(urlp, CARLUPART_QUERY,
                        append_list[i].q,
                        append_list[i].qflags | CARLU_APPENDQUERY);
    if(error)
      ;
    else if(rc != append_list[i].ucode) {
      fprintf(stderr, "Append\nin: %s\nreturned %d (expected %d)\n",
              append_list[i].in, (int)rc, append_list[i].ucode);
      error++;
    }
    else if(append_list[i].ucode) {
      /* the expected error happened */
    }
    else {
      char *url;
      rc = carl_url_get(urlp, CARLUPART_URL, &url, 0);
      if(rc) {
        fprintf(stderr, "%s:%d Get URL returned %d\n",
                __FILE__, __LINE__, (int)rc);
        error++;
      }
      else {
        if(checkurl(url, append_list[i].out)) {
          error++;
        }
        carl_free(url);
      }
    }
    carl_url_cleanup(urlp);
  }
  return error;
}

static int scopeid(void)
{
  CARLU *u = carl_url();
  int error = 0;
  CARLUcode rc;
  char *url;

  rc = carl_url_set(u, CARLUPART_URL,
                    "https://[fe80::20c:29ff:fe9c:409b%25eth0]/hello.html", 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_set returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }

  rc = carl_url_get(u, CARLUPART_HOST, &url, 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_get CARLUPART_HOST returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }
  else {
    printf("we got %s\n", url);
    carl_free(url);
  }

  rc = carl_url_set(u, CARLUPART_HOST, "[::1]", 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_set CARLUPART_HOST returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }

  rc = carl_url_get(u, CARLUPART_URL, &url, 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_get CARLUPART_URL returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }
  else {
    printf("we got %s\n", url);
    carl_free(url);
  }

  rc = carl_url_set(u, CARLUPART_HOST, "example.com", 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_set CARLUPART_HOST returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }

  rc = carl_url_get(u, CARLUPART_URL, &url, 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_get CARLUPART_URL returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }
  else {
    printf("we got %s\n", url);
    carl_free(url);
  }

  rc = carl_url_set(u, CARLUPART_HOST,
                    "[fe80::20c:29ff:fe9c:409b%25eth0]", 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_set CARLUPART_HOST returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }

  rc = carl_url_get(u, CARLUPART_URL, &url, 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_get CARLUPART_URL returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }
  else {
    printf("we got %s\n", url);
    carl_free(url);
  }

  rc = carl_url_get(u, CARLUPART_HOST, &url, 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_get CARLUPART_HOST returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }
  else {
    printf("we got %s\n", url);
    carl_free(url);
  }

  rc = carl_url_get(u, CARLUPART_ZONEID, &url, 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_get CARLUPART_ZONEID returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }
  else {
    printf("we got %s\n", url);
    carl_free(url);
  }

  rc = carl_url_set(u, CARLUPART_ZONEID, "clown", 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_set CARLUPART_ZONEID returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }

  rc = carl_url_get(u, CARLUPART_URL, &url, 0);
  if(rc != CARLUE_OK) {
    fprintf(stderr, "%s:%d carl_url_get CARLUPART_URL returned %d\n",
            __FILE__, __LINE__, (int)rc);
    error++;
  }
  else {
    printf("we got %s\n", url);
    carl_free(url);
  }

  carl_url_cleanup(u);

  return error;
}

int test(char *URL)
{
  (void)URL; /* not used */

  if(scopeid())
    return 6;

  if(append())
    return 5;

  if(set_url())
    return 1;

  if(set_parts())
    return 2;

  if(get_url())
    return 3;

  if(get_parts())
    return 4;

  printf("success\n");
  return 0;
}
