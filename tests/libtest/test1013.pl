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
# Determine if carl-config --protocols/--features matches the
# carl --version protocols/features
if ( $#ARGV != 2 )
{
    print "Usage: $0 carl-config-script carl-version-output-file features|protocols\n";
    exit 3;
}

my $what=$ARGV[2];

# Read the output of carl --version
my $carl_protocols="";
open(CARL, "$ARGV[1]") || die "Can't get carl $what list\n";
while( <CARL> )
{
    $carl_protocols = lc($_) if ( /$what:/i );
}
close CARL;

$carl_protocols =~ s/\r//;
$carl_protocols =~ /\w+: (.*)$/;
@carl = split / /,$1;

# These features are not supported by carl-config
@carl = grep(!/^(Debug|TrackMemory|Metalink|Largefile|CharConv)$/i, @carl);
@carl = sort @carl;

# Read the output of carl-config
my @carl_config;
open(CARLCONFIG, "sh $ARGV[0] --$what|") || die "Can't get carl-config $what list\n";
while( <CARLCONFIG> )
{
    chomp;
    # ignore carl-config --features not in carl's feature list
    push @carl_config, lc($_);
}
close CARLCONFIG;

@carl_config = sort @carl_config;

my $carlproto = join ' ', @carl;
my $carlconfigproto = join ' ', @carl_config;

my $different = $carlproto ne $carlconfigproto;
if ($different) {
    print "Mismatch in $what lists:\n";
    print "carl:        $carlproto\n";
    print "carl-config: $carlconfigproto\n";
}
exit $different;
