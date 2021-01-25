#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 2008 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution. The terms
# are also available at https://carl.se/docs/copyright.html.
#
# You may opt to use, copy, modify, merge, publish, distribute and/or sell
# copies of the Software, and permit persons to whom the Software is
# furnished to do so, under the terms of the COPYING file.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
###########################################################################

# Shared between Makefile.am and CMakeLists.txt

man_MANS = \
 carl_easy_cleanup.3 \
 carl_easy_duphandle.3 \
 carl_easy_escape.3 \
 carl_easy_getinfo.3 \
 carl_easy_init.3 \
 carl_easy_option_by_id.3 \
 carl_easy_option_by_name.3 \
 carl_easy_option_next.3 \
 carl_easy_pause.3 \
 carl_easy_perform.3 \
 carl_easy_recv.3 \
 carl_easy_reset.3 \
 carl_easy_send.3 \
 carl_easy_setopt.3 \
 carl_easy_strerror.3 \
 carl_easy_unescape.3 \
 carl_easy_upkeep.3 \
 carl_escape.3 \
 carl_formadd.3 \
 carl_formfree.3 \
 carl_formget.3 \
 carl_free.3 \
 carl_getdate.3 \
 carl_getenv.3 \
 carl_global_cleanup.3 \
 carl_global_init.3 \
 carl_global_init_mem.3 \
 carl_global_sslset.3 \
 carl_mime_addpart.3 \
 carl_mime_data.3 \
 carl_mime_data_cb.3 \
 carl_mime_encoder.3 \
 carl_mime_filedata.3 \
 carl_mime_filename.3 \
 carl_mime_free.3 \
 carl_mime_headers.3 \
 carl_mime_init.3 \
 carl_mime_name.3 \
 carl_mime_subparts.3 \
 carl_mime_type.3 \
 carl_mprintf.3 \
 carl_multi_add_handle.3 \
 carl_multi_assign.3 \
 carl_multi_cleanup.3 \
 carl_multi_fdset.3 \
 carl_multi_info_read.3 \
 carl_multi_init.3 \
 carl_multi_perform.3 \
 carl_multi_poll.3 \
 carl_multi_remove_handle.3 \
 carl_multi_setopt.3 \
 carl_multi_socket.3 \
 carl_multi_socket_action.3 \
 carl_multi_socket_all.3 \
 carl_multi_strerror.3 \
 carl_multi_timeout.3 \
 carl_multi_wakeup.3 \
 carl_multi_wait.3 \
 carl_share_cleanup.3 \
 carl_share_init.3 \
 carl_share_setopt.3 \
 carl_share_strerror.3 \
 carl_slist_append.3 \
 carl_slist_free_all.3 \
 carl_strequal.3 \
 carl_strnequal.3 \
 carl_unescape.3 \
 carl_url.3 \
 carl_url_cleanup.3 \
 carl_url_dup.3 \
 carl_url_get.3 \
 carl_url_set.3 \
 carl_version.3 \
 carl_version_info.3 \
 libcarl-easy.3 \
 libcarl-env.3 \
 libcarl-errors.3 \
 libcarl-multi.3 \
 libcarl-security.3 \
 libcarl-share.3 \
 libcarl-symbols.3 \
 libcarl-thread.3 \
 libcarl-tutorial.3 \
 libcarl-url.3 \
 libcarl.3
