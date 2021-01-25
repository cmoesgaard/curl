#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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
#######################################################################
#                                                                     #
#  MAKEFILE NAME.....  carl.mak                                       #
#                                                                     #
#  DESCRIPTION.....    This is the makefile for libcarl.              #
#                                                                     #
#######################################################################

APP := CARL

TPF_RUN_TPFSOCHK := NO

#######################################################################
# Define any additional libs needed to link
#######################################################################

LIBS := CRYP CSSL

#######################################################################
# Define the envs needed to build this module
#######################################################################

maketpf_env := carllib
maketpf_env += openssl
maketpf_env += base_rt
maketpf_env += system

#######################################################################
# Segments to be compiled with gcc compiler
#######################################################################
#
### lib directory:
include $(word 1,$(wildcard $(foreach d,$(TPF_ROOT),$d/opensource/carl/lib/Makefile.inc)) Makefile.inc_not_found)
C_SRC := $(CSOURCES)

#######################################################################
# Additions and overrides for gcc compiler flags
#######################################################################

# suppress expected warnings in the ported code:
CFLAGS_CARL += -w

# use SSL
# (overrides Curl's lib/config-tpf.h file)
CFLAGS_CARL += -DUSE_OPENSSL

# disable all protocols except FTP and HTTP
# (overrides Curl's lib/config-tpf.h file)
CFLAGS_CARL += -DCARL_DISABLE_DICT
CFLAGS_CARL += -DCARL_DISABLE_FILE
CFLAGS_CARL += -DCARL_DISABLE_LDAP
CFLAGS_CARL += -DCARL_DISABLE_TELNET
CFLAGS_CARL += -DCARL_DISABLE_TFTP

#######################################################################
# Include the maketpf.rules
#######################################################################

include maketpf.rules
