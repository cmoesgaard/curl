# File: make_gnv_carl_install.sh
#
# $Id$
#
# Set up and run the make script for Curl.
#
# This makes the library, the carl binary and attempts an install.
# A search list should be set up for GNU (GNV$GNU).
#
# Copyright 2009 - 2020, John Malmberg
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
# OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# 06-Jun-2009	J. Malmberg
#==========================================================================
#
#
# Needed VMS build setups for GNV.
export GNV_OPT_DIR=.
export GNV_CC_QUALIFIERS=/DEBUG/OPTIMIZE/STANDARD=RELAXED\
/float=ieee_float/ieee_mode=denorm_results
export GNV_CXX_QUALIFIERS=/DEBUG/OPTIMIZE/float=ieee/ieee_mode=denorm_results
export GNV_CC_NO_INC_PRIMARY=1
#
#
# POSIX exit mode is needed for Unix shells.
export GNV_CC_MAIN_POSIX_EXIT=1
make
cd ../..
# adjust the libcarl.pc file, GNV currently ignores the Lib: line.
# but is noisy about it, so we just remove it.
sed -e 's/^Libs:/#Libs:/g' libcarl.pc > libcarl.pc_new
rm libcarl.pc
mv libcarl.pc_new libcarl.pc
make install
