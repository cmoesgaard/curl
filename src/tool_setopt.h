#ifndef HEADER_CARL_TOOL_SETOPT_H
#define HEADER_CARL_TOOL_SETOPT_H
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
#include "tool_setup.h"

#include "tool_formparse.h"

/*
 * Macros used in operate()
 */

#define SETOPT_CHECK(v,opt) do {                \
    if(!tool_setopt_skip(opt)) {                \
      result = (v);                             \
      if(result)                                \
        break;                                  \
    }                                           \
  } while(0)

/* allow removed features to simulate success: */
bool tool_setopt_skip(CARLoption tag);

#ifndef CARL_DISABLE_LIBCARL_OPTION

/* Associate symbolic names with option values */
struct NameValue {
  const char *name;
  long value;
};

struct NameValueUnsigned {
  const char *name;
  unsigned long value;
};

extern const struct NameValue setopt_nv_CARLPROXY[];
extern const struct NameValue setopt_nv_CARL_SOCKS_PROXY[];
extern const struct NameValue setopt_nv_CARL_HTTP_VERSION[];
extern const struct NameValue setopt_nv_CARL_SSLVERSION[];
extern const struct NameValue setopt_nv_CARL_TIMECOND[];
extern const struct NameValue setopt_nv_CARLFTPSSL_CCC[];
extern const struct NameValue setopt_nv_CARLUSESSL[];
extern const struct NameValueUnsigned setopt_nv_CARLSSLOPT[];
extern const struct NameValue setopt_nv_CARL_NETRC[];
extern const struct NameValue setopt_nv_CARLPROTO[];
extern const struct NameValueUnsigned setopt_nv_CARLAUTH[];
extern const struct NameValueUnsigned setopt_nv_CARLHSTS[];

/* Map options to NameValue sets */
#define setopt_nv_CARLOPT_HSTS_CTRL setopt_nv_CARLHSTS
#define setopt_nv_CARLOPT_HTTP_VERSION setopt_nv_CARL_HTTP_VERSION
#define setopt_nv_CARLOPT_HTTPAUTH setopt_nv_CARLAUTH
#define setopt_nv_CARLOPT_SSLVERSION setopt_nv_CARL_SSLVERSION
#define setopt_nv_CARLOPT_PROXY_SSLVERSION setopt_nv_CARL_SSLVERSION
#define setopt_nv_CARLOPT_TIMECONDITION setopt_nv_CARL_TIMECOND
#define setopt_nv_CARLOPT_FTP_SSL_CCC setopt_nv_CARLFTPSSL_CCC
#define setopt_nv_CARLOPT_USE_SSL setopt_nv_CARLUSESSL
#define setopt_nv_CARLOPT_SSL_OPTIONS setopt_nv_CARLSSLOPT
#define setopt_nv_CARLOPT_NETRC setopt_nv_CARL_NETRC
#define setopt_nv_CARLOPT_PROTOCOLS setopt_nv_CARLPROTO
#define setopt_nv_CARLOPT_REDIR_PROTOCOLS setopt_nv_CARLPROTO
#define setopt_nv_CARLOPT_PROXYTYPE setopt_nv_CARLPROXY
#define setopt_nv_CARLOPT_PROXYAUTH setopt_nv_CARLAUTH
#define setopt_nv_CARLOPT_SOCKS5_AUTH setopt_nv_CARLAUTH

/* Intercept setopt calls for --libcarl */

CARLcode tool_setopt_enum(CARL *carl, struct GlobalConfig *config,
                          const char *name, CARLoption tag,
                          const struct NameValue *nv, long lval);
CARLcode tool_setopt_flags(CARL *carl, struct GlobalConfig *config,
                           const char *name, CARLoption tag,
                           const struct NameValue *nv, long lval);
CARLcode tool_setopt_bitmask(CARL *carl, struct GlobalConfig *config,
                             const char *name, CARLoption tag,
                             const struct NameValueUnsigned *nv, long lval);
CARLcode tool_setopt_mimepost(CARL *carl, struct GlobalConfig *config,
                              const char *name, CARLoption tag,
                              carl_mime *mimepost);
CARLcode tool_setopt_slist(CARL *carl, struct GlobalConfig *config,
                           const char *name, CARLoption tag,
                           struct carl_slist *list);
CARLcode tool_setopt(CARL *carl, bool str, struct GlobalConfig *global,
                     struct OperationConfig *config,
                     const char *name, CARLoption tag, ...);

#define my_setopt(x,y,z) \
  SETOPT_CHECK(tool_setopt(x, FALSE, global, config, #y, y, z), y)

#define my_setopt_str(x,y,z) \
  SETOPT_CHECK(tool_setopt(x, TRUE, global, config, #y, y, z), y)

#define my_setopt_enum(x,y,z) \
  SETOPT_CHECK(tool_setopt_enum(x, global, #y, y, setopt_nv_ ## y, z), y)

#define my_setopt_flags(x,y,z) \
  SETOPT_CHECK(tool_setopt_flags(x, global, #y, y, setopt_nv_ ## y, z), y)

#define my_setopt_bitmask(x,y,z) \
  SETOPT_CHECK(tool_setopt_bitmask(x, global, #y, y, setopt_nv_ ## y, z), y)

#define my_setopt_mimepost(x,y,z) \
  SETOPT_CHECK(tool_setopt_mimepost(x, global, #y, y, z), y)

#define my_setopt_slist(x,y,z) \
  SETOPT_CHECK(tool_setopt_slist(x, global, #y, y, z), y)

#define res_setopt(x,y,z) tool_setopt(x, FALSE, global, config, #y, y, z)

#define res_setopt_str(x,y,z) tool_setopt(x, TRUE, global, config, #y, y, z)

#else /* CARL_DISABLE_LIBCARL_OPTION */

/* No --libcarl, so pass options directly to library */

#define my_setopt(x,y,z) \
  SETOPT_CHECK(carl_easy_setopt(x, y, z), y)

#define my_setopt_str(x,y,z) \
  SETOPT_CHECK(carl_easy_setopt(x, y, z), y)

#define my_setopt_enum(x,y,z) \
  SETOPT_CHECK(carl_easy_setopt(x, y, z), y)

#define my_setopt_flags(x,y,z) \
  SETOPT_CHECK(carl_easy_setopt(x, y, z), y)

#define my_setopt_bitmask(x,y,z) \
  SETOPT_CHECK(carl_easy_setopt(x, y, z), y)

#define my_setopt_mimepost(x,y,z) \
  SETOPT_CHECK(carl_easy_setopt(x, y, z), y)

#define my_setopt_slist(x,y,z) \
  SETOPT_CHECK(carl_easy_setopt(x, y, z), y)

#define res_setopt(x,y,z) carl_easy_setopt(x,y,z)

#define res_setopt_str(x,y,z) carl_easy_setopt(x,y,z)

#endif /* CARL_DISABLE_LIBCARL_OPTION */

#endif /* HEADER_CARL_TOOL_SETOPT_H */
