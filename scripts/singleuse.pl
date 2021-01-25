#!/usr/bin/perl
#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 2019 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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
#
# This script is aimed to help scan for and detect globally declared functions
# that are not used from other source files.
#
# Use it like this:
#
# $ ./scripts/singleuse.pl lib/.libs/libcarl.a
#
# Be aware that it might cause false positives due to various build options.
#

my $file = $ARGV[0];

my %wl = (
    'Curl_none_cert_status_request' => 'multiple TLS backends',
    'Curl_none_check_cxn' => 'multiple TLS backends',
    'Curl_none_cleanup' => 'multiple TLS backends',
    'Curl_none_close_all' => 'multiple TLS backends',
    'Curl_none_data_pending' => 'multiple TLS backends',
    'Curl_none_engines_list' => 'multiple TLS backends',
    'Curl_none_init' => 'multiple TLS backends',
    'Curl_none_md5sum' => 'multiple TLS backends',
    'Curl_none_random' => 'multiple TLS backends',
    'Curl_none_session_free' => 'multiple TLS backends',
    'Curl_none_set_engine' => 'multiple TLS backends',
    'Curl_none_set_engine_default' => 'multiple TLS backends',
    'Curl_none_shutdown' => 'multiple TLS backends',
    'Curl_multi_dump' => 'debug build only',
    'Curl_parse_port' => 'UNITTEST',
    'Curl_shuffle_addr' => 'UNITTEST',
    'de_cleanup' => 'UNITTEST',
    'doh_decode' => 'UNITTEST',
    'doh_encode' => 'UNITTEST',
    'Curl_auth_digest_get_pair' => 'by digest_sspi',
    'carlx_uztoso' => 'cmdline tool use',
    'carlx_uztoul' => 'by krb5_sspi',
    'carlx_uitous' => 'by schannel',
    'Curl_islower' => 'by carl_fnmatch',
    'getaddressinfo' => 'UNITTEST',
    );

my %api = (
    'carl_easy_cleanup' => 'API',
    'carl_easy_duphandle' => 'API',
    'carl_easy_escape' => 'API',
    'carl_easy_getinfo' => 'API',
    'carl_easy_init' => 'API',
    'carl_easy_pause' => 'API',
    'carl_easy_perform' => 'API',
    'carl_easy_recv' => 'API',
    'carl_easy_reset' => 'API',
    'carl_easy_send' => 'API',
    'carl_easy_setopt' => 'API',
    'carl_easy_strerror' => 'API',
    'carl_easy_unescape' => 'API',
    'carl_easy_upkeep' => 'API',
    'carl_escape' => 'API',
    'carl_formadd' => 'API',
    'carl_formfree' => 'API',
    'carl_formget' => 'API',
    'carl_free' => 'API',
    'carl_getdate' => 'API',
    'carl_getenv' => 'API',
    'carl_global_cleanup' => 'API',
    'carl_global_init' => 'API',
    'carl_global_init_mem' => 'API',
    'carl_global_sslset' => 'API',
    'carl_maprintf' => 'API',
    'carl_mfprintf' => 'API',
    'carl_mime_addpart' => 'API',
    'carl_mime_data' => 'API',
    'carl_mime_data_cb' => 'API',
    'carl_mime_encoder' => 'API',
    'carl_mime_filedata' => 'API',
    'carl_mime_filename' => 'API',
    'carl_mime_free' => 'API',
    'carl_mime_headers' => 'API',
    'carl_mime_init' => 'API',
    'carl_mime_name' => 'API',
    'carl_mime_subparts' => 'API',
    'carl_mime_type' => 'API',
    'carl_mprintf' => 'API',
    'carl_msnprintf' => 'API',
    'carl_msprintf' => 'API',
    'carl_multi_add_handle' => 'API',
    'carl_multi_assign' => 'API',
    'carl_multi_cleanup' => 'API',
    'carl_multi_fdset' => 'API',
    'carl_multi_info_read' => 'API',
    'carl_multi_init' => 'API',
    'carl_multi_perform' => 'API',
    'carl_multi_remove_handle' => 'API',
    'carl_multi_setopt' => 'API',
    'carl_multi_socket' => 'API',
    'carl_multi_socket_action' => 'API',
    'carl_multi_socket_all' => 'API',
    'carl_multi_poll' => 'API',
    'carl_multi_strerror' => 'API',
    'carl_multi_timeout' => 'API',
    'carl_multi_wait' => 'API',
    'carl_multi_wakeup' => 'API',
    'carl_mvaprintf' => 'API',
    'carl_mvfprintf' => 'API',
    'carl_mvprintf' => 'API',
    'carl_mvsnprintf' => 'API',
    'carl_mvsprintf' => 'API',
    'carl_pushheader_byname' => 'API',
    'carl_pushheader_bynum' => 'API',
    'carl_share_cleanup' => 'API',
    'carl_share_init' => 'API',
    'carl_share_setopt' => 'API',
    'carl_share_strerror' => 'API',
    'carl_slist_append' => 'API',
    'carl_slist_free_all' => 'API',
    'carl_strequal' => 'API',
    'carl_strnequal' => 'API',
    'carl_unescape' => 'API',
    'carl_url' => 'API',
    'carl_url_cleanup' => 'API',
    'carl_url_dup' => 'API',
    'carl_url_get' => 'API',
    'carl_url_set' => 'API',
    'carl_version' => 'API',
    'carl_version_info' => 'API',

    # the following functions are provided globally in debug builds
    'carl_easy_perform_ev' => 'debug-build',
    );

open(N, "nm $file|") ||
    die;

my %exist;
my %uses;
my $file;
while (<N>) {
    my $l = $_;
    chomp $l;

    if($l =~ /^([0-9a-z_-]+)\.o:/) {
        $file = $1;
    }
    if($l =~ /^([0-9a-f]+) T (.*)/) {
        my ($name)=($2);
        #print "Define $name in $file\n";
        $file =~ s/^libcarl_la-//;
        $exist{$name} = $file;
    }
    elsif($l =~ /^                 U (.*)/) {
        my ($name)=($1);
        #print "Uses $name in $file\n";
        $uses{$name} .= "$file, ";
    }
}
close(N);

my $err;
for(sort keys %exist) {
    #printf "%s is defined in %s, used by: %s\n", $_, $exist{$_}, $uses{$_};
    if(!$uses{$_}) {
        # this is a symbol with no "global" user
        if($_ =~ /^carl_dbg_/) {
            # we ignore the memdebug symbols
        }
        elsif($_ =~ /^carl_/) {
            if(!$api{$_}) {
                # not present in the API, or for debug-builds
                print STDERR "Bad carl-prefix: $_\n";
                $err++;
            }
        }
        elsif($wl{$_}) {
            #print "$_ is WL\n";
        }
        else {
            printf "%s is defined in %s, but not used outside\n", $_, $exist{$_};
            $err++;
        }
    }
    elsif($_ =~ /^carl_/) {
        # global prefix, make sure it is "blessed"
        if(!$api{$_}) {
            # not present in the API, or for debug-builds
            if($_ !~ /^carl_dbg_/) {
                # ignore the memdebug symbols
                print STDERR "Bad carl-prefix $_\n";
                $err++;
            }
        }
    }
}

exit $err;
