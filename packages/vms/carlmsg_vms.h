#ifndef HEADER_CARLMSG_VMS_H
#define HEADER_CARLMSG_VMS_H
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

/*                                                                          */
/* CARLMSG_VMS.H                                                            */
/*                                                                          */
/* This defines the necessary bits to change CARLE_* error codes to VMS     */
/* style error codes.  CARLMSG.H is built from CARLMSG.SDL which is built   */
/* from CARLMSG.MSG.  The vms_cond array is used to return VMS errors by    */
/* putting the VMS error codes into the array offset based on CARLE_* code. */
/*                                                                          */
/* If you update CARLMSG.MSG make sure to update this file to match.        */
/*                                                                          */

#include "carlmsg.h"

/*
#define   FAC_CARL      0xC01
#define   FAC_SYSTEM    0
#define   MSG_NORMAL    0
*/

/*
#define   SEV_WARNING   0
#define   SEV_SUCCESS   1
#define   SEV_ERROR     2
#define   SEV_INFO      3
#define   SEV_FATAL     4
*/

static const long vms_cond[] =
        {
        CARL_OK,
	CARL_UNSUPPORTED_PROTOCOL,
	CARL_FAILED_INIT,
	CARL_URL_MALFORMAT,
	CARL_OBSOLETE4,
	CARL_COULDNT_RESOLVE_PROXY,
	CARL_COULDNT_RESOLVE_HOST,
	CARL_COULDNT_CONNECT,
	CARL_WEIRD_SERVER_REPLY,
	CARL_FTP_ACCESS_DENIED,
	CARL_OBSOLETE10,
	CARL_FTP_WEIRD_PASS_REPLY,
	CARL_OBSOLETE12,
	CARL_FTP_WEIRD_PASV_REPLY,
	CARL_FTP_WEIRD_227_FORMAT,
	CARL_FTP_CANT_GET_HOST,
	CARL_OBSOLETE16,
	CARL_FTP_COULDNT_SET_TYPE,
	CARL_PARTIAL_FILE,
	CARL_FTP_COULDNT_RETR_FILE,
	CARL_OBSOLETE20,
	CARL_QUOTE_ERROR,
	CARL_HTTP_RETURNED_ERROR,
	CARL_WRITE_ERROR,
	CARL_OBSOLETE24,
	CARL_UPLOAD_FAILED,
	CARL_READ_ERROR,
	CARL_OUT_OF_MEMORY,
	CARL_OPERATION_TIMEOUTED,
	CARL_OBSOLETE29,
	CARL_FTP_PORT_FAILED,
	CARL_FTP_COULDNT_USE_REST,
	CARL_OBSOLETE32,
	CARL_RANGE_ERROR,
	CARL_HTTP_POST_ERROR,
	CARL_SSL_CONNECT_ERROR,
	CARL_BAD_DOWNLOAD_RESUME,
	CARL_FILE_COULDNT_READ_FILE,
	CARL_LDAP_CANNOT_BIND,
	CARL_LDAP_SEARCH_FAILED,
	CARL_OBSOLETE40,
	CARL_FUNCTION_NOT_FOUND,
	CARL_ABORTED_BY_CALLBACK,
	CARL_BAD_FUNCTION_ARGUMENT,
	CARL_OBSOLETE44,
	CARL_INTERFACE_FAILED,
	CARL_OBSOLETE46,
	CARL_TOO_MANY_REDIRECTS,
	CARL_UNKNOWN_TELNET_OPTION,
	CARL_TELNET_OPTION_SYNTAX,
	CARL_OBSOLETE50,
	CARL_PEER_FAILED_VERIF,
	CARL_GOT_NOTHING,
	CARL_SSL_ENGINE_NOTFOUND,
	CARL_SSL_ENGINE_SETFAILED,
	CARL_SEND_ERROR,
	CARL_RECV_ERROR,
	CARL_OBSOLETE57,
	CARL_SSL_CERTPROBLEM,
	CARL_SSL_CIPHER,
	CARL_SSL_CACERT,
	CARL_BAD_CONTENT_ENCODING,
	CARL_LDAP_INVALID_URL,
	CARL_FILESIZE_EXCEEDED,
	CARL_USE_SSL_FAILED,
	CARL_SEND_FAIL_REWIND,
	CARL_SSL_ENGINE_INITFAILED,
	CARL_LOGIN_DENIED,
	CARL_TFTP_NOTFOUND,
	CARL_TFTP_PERM,
	CARL_REMOTE_DISK_FULL,
	CARL_TFTP_ILLEGAL,
	CARL_TFTP_UNKNOWNID,
	CARL_REMOTE_FILE_EXISTS,
	CARL_TFTP_NOSUCHUSER,
	CARL_CONV_FAILED,
	CARL_CONV_REQD,
	CARL_SSL_CACERT_BADFILE,
	CARL_REMOTE_FILE_NOT_FOUND,
	CARL_SSH,
	CARL_SSL_SHUTDOWN_FAILED,
	CARL_AGAIN,
	CARLE_SSL_CRL_BADFILE,
	CARLE_SSL_ISSUER_ERROR,
        CARL_CARL_LAST
        };

#endif /* HEADER_CARLMSG_VMS_H */
