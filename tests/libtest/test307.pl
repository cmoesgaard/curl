#!/usr/bin/env perl
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
# Determine if the given carl executable supports the 'openssl' SSL engine
if ( $#ARGV != 0 )
{
    print "Usage: $0 carl-executable\n";
    exit 3;
}
if (!open(CARL, "@ARGV[0] -s --engine list|"))
{
    print "Can't get SSL engine list\n";
    exit 2;
}
while( <CARL> )
{
    exit 0 if ( /openssl/ );
}
close CARL;
print "openssl engine not supported\n";
exit 1;
