#ifndef CARLINC_CCSIDCARL_H
#define CARLINC_CCSIDCARL_H
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
 *
 ***************************************************************************/
#include "carl.h"
#include "easy.h"
#include "multi.h"


CARL_EXTERN char *carl_version_ccsid(unsigned int ccsid);
CARL_EXTERN char *carl_easy_escape_ccsid(CARL *handle,
                                         const char *string, int length,
                                         unsigned int sccsid,
                                         unsigned int dccsid);
CARL_EXTERN char *carl_easy_unescape_ccsid(CARL *handle, const char *string,
                                           int length, int *outlength,
                                           unsigned int sccsid,
                                           unsigned int dccsid);
CARL_EXTERN struct carl_slist *carl_slist_append_ccsid(struct carl_slist *l,
                                                       const char *data,
                                                       unsigned int ccsid);
CARL_EXTERN time_t carl_getdate_ccsid(const char *p, const time_t *unused,
                                      unsigned int ccsid);
CARL_EXTERN carl_version_info_data *carl_version_info_ccsid(CARLversion stamp,
                                                            unsigned int cid);
CARL_EXTERN const char *carl_easy_strerror_ccsid(CARLcode error,
                                                 unsigned int ccsid);
CARL_EXTERN const char *carl_share_strerror_ccsid(CARLSHcode error,
                                                  unsigned int ccsid);
CARL_EXTERN const char *carl_multi_strerror_ccsid(CARLMcode error,
                                                  unsigned int ccsid);
CARL_EXTERN CARLcode carl_easy_getinfo_ccsid(CARL *carl, CARLINFO info, ...);
CARL_EXTERN CARLFORMcode carl_formadd_ccsid(struct carl_httppost **httppost,
                                            struct carl_httppost **last_post,
                                            ...);
CARL_EXTERN char *carl_form_long_value(long value);
CARL_EXTERN int carl_formget_ccsid(struct carl_httppost *form, void *arg,
                                   carl_formget_callback append,
                                   unsigned int ccsid);
CARL_EXTERN CARLcode carl_easy_setopt_ccsid(CARL *carl, CARLoption tag, ...);
CARL_EXTERN void carl_certinfo_free_all(struct carl_certinfo *info);
CARL_EXTERN char *carl_pushheader_bynum_cssid(struct carl_pushheaders *h,
                                              size_t num, unsigned int ccsid);
CARL_EXTERN char *carl_pushheader_byname_ccsid(struct carl_pushheaders *h,
                                               const char *header,
                                               unsigned int ccsidin,
                                               unsigned int ccsidout);
CARL_EXTERN CARLcode carl_mime_name_ccsid(carl_mimepart *part,
                                          const char *name,
                                          unsigned int ccsid);
CARL_EXTERN CARLcode carl_mime_filename_ccsid(carl_mimepart *part,
                                              const char *filename,
                                              unsigned int ccsid);
CARL_EXTERN CARLcode carl_mime_type_ccsid(carl_mimepart *part,
                                          const char *mimetype,
                                          unsigned int ccsid);
CARL_EXTERN CARLcode carl_mime_encoder_ccsid(carl_mimepart *part,
                                             const char *encoding,
                                             unsigned int ccsid);
CARL_EXTERN CARLcode carl_mime_filedata_ccsid(carl_mimepart *part,
                                              const char *filename,
                                              unsigned int ccsid);
CARL_EXTERN CARLcode carl_mime_data_ccsid(carl_mimepart *part,
                                          const char *data, size_t datasize,
                                          unsigned int ccsid);
CARL_EXTERN CARLUcode carl_url_get_ccsid(CARLU *handle, CARLUPart what,
                                         char **part, unsigned int flags,
                                         unsigned int ccsid);
CARL_EXTERN CARLUcode carl_url_set_ccsid(CARLU *handle, CARLUPart what,
                                         const char *part, unsigned int flags,
                                         unsigned int ccsid);

#endif
