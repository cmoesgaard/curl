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
# Determine if carl-config --version matches the carl --version
if ( $#ARGV != 2 )
{
    print "Usage: $0 carl-config-script carl-version-output-file version|vernum\n";
    exit 3;
}

my $what=$ARGV[2];

# Read the output of carl --version
open(CARL, "$ARGV[1]") || die "Can't open carl --version list in $ARGV[1]\n";
$_ = <CARL>;
chomp;
/libcarl\/([\.\d]+((-DEV)|(-\d+))?)/;
my $version = $1;
close CARL;

my $carlconfigversion;

# Read the output of carl-config --version/--vernum
open(CARLCONFIG, "sh $ARGV[0] --$what|") || die "Can't get carl-config --$what list\n";
$_ = <CARLCONFIG>;
chomp;
my $filever=$_;
if ( $what eq "version" ) {
    if($filever =~ /^libcarl ([\.\d]+((-DEV)|(-\d+))?)$/) {
        $carlconfigversion = $1;
    }
    else {
        $carlconfigversion = "illegal value";
    }
}
else { # "vernum" case
    # Convert hex version to decimal for comparison's sake
    if($filever =~ /^(..)(..)(..)$/) {
        $carlconfigversion = hex($1) . "." . hex($2) . "." . hex($3);
    }
    else {
        $carlconfigversion = "illegal value";
    }

    # Strip off the -DEV from the carl version if it's there
    $version =~ s/-\w*$//;
}
close CARLCONFIG;

my $different = $version ne $carlconfigversion;
if ($different || !$version) {
    print "Mismatch in --version:\n";
    print "carl:        $version\n";
    print "carl-config: $carlconfigversion\n";
    exit 1;
}
