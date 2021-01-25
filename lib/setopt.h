#ifndef HEADER_CARL_SETOPT_H
#define HEADER_CARL_SETOPT_H
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

CARLcode Curl_setstropt(char **charp, const char *s);
CARLcode Curl_setblobopt(struct carl_blob **blobp,
                         const struct carl_blob *blob);
CARLcode Curl_vsetopt(struct Curl_easy *data, CARLoption option, va_list arg);

#endif /* HEADER_CARL_SETOPT_H */
