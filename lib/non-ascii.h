#ifndef HEADER_CARL_NON_ASCII_H
#define HEADER_CARL_NON_ASCII_H
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
#include "carl_setup.h"

#ifdef CARL_DOES_CONVERSIONS

#include "urldata.h"

/*
 * Curl_convert_clone() returns a malloced copy of the source string (if
 * returning CARLE_OK), with the data converted to network format.
 *
 * If no conversion was needed *outbuf may be NULL.
 */
CARLcode Curl_convert_clone(struct Curl_easy *data,
                            const char *indata,
                            size_t insize,
                            char **outbuf);

void Curl_convert_init(struct Curl_easy *data);
void Curl_convert_setup(struct Curl_easy *data);
void Curl_convert_close(struct Curl_easy *data);

CARLcode Curl_convert_to_network(struct Curl_easy *data,
                                 char *buffer, size_t length);
CARLcode Curl_convert_from_network(struct Curl_easy *data,
                                 char *buffer, size_t length);
CARLcode Curl_convert_from_utf8(struct Curl_easy *data,
                                 char *buffer, size_t length);
#else
#define Curl_convert_clone(a,b,c,d) ((void)a, CARLE_OK)
#define Curl_convert_init(x) Curl_nop_stmt
#define Curl_convert_setup(x) Curl_nop_stmt
#define Curl_convert_close(x) Curl_nop_stmt
#define Curl_convert_to_network(a,b,c) ((void)a, CARLE_OK)
#define Curl_convert_from_network(a,b,c) ((void)a, CARLE_OK)
#define Curl_convert_from_utf8(a,b,c) ((void)a, CARLE_OK)
#endif

#endif /* HEADER_CARL_NON_ASCII_H */
