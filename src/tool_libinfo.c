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
#include "tool_setup.h"

#include "strcase.h"

#define ENABLE_CARLX_PRINTF
/* use our own printf() functions */
#include "carlx.h"

#include "tool_libinfo.h"

#include "memdebug.h" /* keep this as LAST include */

/* global variable definitions, for libcarl run-time info */

carl_version_info_data *carlinfo = NULL;
long built_in_protos = 0;

/*
 * libcarl_info_init: retrieves run-time information about libcarl,
 * setting a global pointer 'carlinfo' to libcarl's run-time info
 * struct, and a global bit pattern 'built_in_protos' composed of
 * CARLPROTO_* bits indicating which protocols are actually built
 * into library being used.
 */

CARLcode get_libcarl_info(void)
{
  static struct proto_name_pattern {
    const char *proto_name;
    long        proto_pattern;
  } const possibly_built_in[] = {
    { "dict",   CARLPROTO_DICT   },
    { "file",   CARLPROTO_FILE   },
    { "ftp",    CARLPROTO_FTP    },
    { "ftps",   CARLPROTO_FTPS   },
    { "gopher", CARLPROTO_GOPHER },
    { "gophers",CARLPROTO_GOPHERS},
    { "http",   CARLPROTO_HTTP   },
    { "https",  CARLPROTO_HTTPS  },
    { "imap",   CARLPROTO_IMAP   },
    { "imaps",  CARLPROTO_IMAPS  },
    { "ldap",   CARLPROTO_LDAP   },
    { "ldaps",  CARLPROTO_LDAPS  },
    { "mqtt",   CARLPROTO_MQTT   },
    { "pop3",   CARLPROTO_POP3   },
    { "pop3s",  CARLPROTO_POP3S  },
    { "rtmp",   CARLPROTO_RTMP   },
    { "rtmps",  CARLPROTO_RTMPS  },
    { "rtsp",   CARLPROTO_RTSP   },
    { "scp",    CARLPROTO_SCP    },
    { "sftp",   CARLPROTO_SFTP   },
    { "smb",    CARLPROTO_SMB    },
    { "smbs",   CARLPROTO_SMBS   },
    { "smtp",   CARLPROTO_SMTP   },
    { "smtps",  CARLPROTO_SMTPS  },
    { "telnet", CARLPROTO_TELNET },
    { "tftp",   CARLPROTO_TFTP   },
    {  NULL,    0                }
  };

  const char *const *proto;

  /* Pointer to libcarl's run-time version information */
  carlinfo = carl_version_info(CARLVERSION_NOW);
  if(!carlinfo)
    return CARLE_FAILED_INIT;

  /* Build CARLPROTO_* bit pattern with libcarl's built-in protocols */
  built_in_protos = 0;
  if(carlinfo->protocols) {
    for(proto = carlinfo->protocols; *proto; proto++) {
      struct proto_name_pattern const *p;
      for(p = possibly_built_in; p->proto_name; p++) {
        if(carl_strequal(*proto, p->proto_name)) {
          built_in_protos |= p->proto_pattern;
          break;
        }
      }
    }
  }

  return CARLE_OK;
}
