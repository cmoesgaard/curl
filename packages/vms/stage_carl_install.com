$! File: stage_carl_install.com
$!
$! $Id$
$!
$! This updates or removes the GNV$CARL.EXE and related files for the
$! new_gnu:[*...] directory tree for running the self tests.
$!
$! The files installed/removed are:
$!     [usr.bin]gnv$carl.exe
$!     [usr.bin]carl-config.
$!     [usr.lib]gnv$libcarl.exe
$!     [usr.bin]carl. hard link for [usr.bin]gnv$carl.exe
$!     [usr.include.carl]carl.h
$!     [usr.include.carl]carlver.h
$!     [usr.include.carl]easy.h
$!     [usr.include.carl]mprintf.h
$!     [usr.include.carl]multi.h
$!     [usr.include.carl]stdcheaders.h
$!     [usr.include.carl]typecheck-gcc.h
$!     [usr.lib.pkgconfig]libcarl.pc
$!     [usr.share.man.man1]carl-config.1
$!     [usr.share.man.man1]carl.1
$!     [usr.share.man.man3]carl*.3
$!     [usr.share.man.man3]libcarl*.3
$! Future: A symbolic link to the release notes?
$!
$! Copyright 2012 - 2020, John Malmberg
$!
$! Permission to use, copy, modify, and/or distribute this software for any
$! purpose with or without fee is hereby granted, provided that the above
$! copyright notice and this permission notice appear in all copies.
$!
$! THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
$! WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
$! MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
$! ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
$! WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
$! ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
$! OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
$!
$!
$! 20-Aug-2012  J. Malmberg
$!
$!===========================================================================
$!
$ arch_type = f$getsyi("ARCH_NAME")
$ arch_code = f$extract(0, 1, arch_type)
$!
$ if arch_code .nes. "V"
$ then
$   set proc/parse=extended
$ endif
$!
$!
$! If the first parameter begins with "r" or "R" then this is to
$! remove the files instead of installing them.
$ remove_filesq = f$edit(p1, "upcase,trim")
$ remove_filesq = f$extract(0, 1, remove_filesq)
$ remove_files = 0
$ if remove_filesq .eqs. "R" then remove_files = 1
$!
$!
$! If we are staging files, make sure that the libcarl.pc and carl-config
$! files are present.
$ if remove_files .eq. 0
$ then
$   if f$search("[--]libcarl.pc") .eqs. ""
$   then
$       @build_libcarl_pc.com
$   endif
$   if f$search("[--]carl-config") .eqs. ""
$   then
$       @build_carl-config_script.com
$   endif
$ endif
$!
$!
$! Dest dirs
$!------------------
$ dest_dirs1 = "[usr],[usr.bin],[usr.include],[usr.include.carl]"
$ dest_dirs2 = ",[usr.bin],[usr.lib.pkgconfig],[usr.share]"
$ dest_dirs3 = ",[usr.share.man],[usr.share.man.man1],[usr.share.man.man3]"
$ dest_dirs = dest_dirs1 + dest_dirs2 + dest_dirs3
$!
$!
$!   Alias links needed.
$!-------------------------
$ source_carl = "gnv$carl.exe"
$ dest_carl = "[bin]gnv$carl.exe"
$ carl_links = "[bin]carl."
$ new_gnu = "new_gnu:"
$!
$!
$! Create the directories if they do not exist
$!---------------------------------------------
$ i = 0
$carl_dir_loop:
$   this_dir = f$element(i, ",", dest_dirs)
$   i = i + 1
$   if this_dir .eqs. "" then goto carl_dir_loop
$   if this_dir .eqs. "," then goto carl_dir_loop_end
$!  Just create the directories, do not delete them.
$!  --------------------------------------------------
$   if remove_files .eq. 0
$   then
$       create/dir 'new_gnu''this_dir'/prot=(o:rwed)
$   endif
$   goto carl_dir_loop
$carl_dir_loop_end:
$!
$!
$! Need to add in the executable file
$!-----------------------------------
$ if remove_files .eq. 0
$ then
$   copy [--.src]carl.exe 'new_gnu'[usr.bin]gnv$carl.exe/prot=w:re
$   copy [--]carl-config. 'new_gnu'[usr.bin]carl-config./prot=w:re
$   copy sys$disk:[]gnv$libcarl.exe 'new_gnu'[usr.lib]gnv$libcarl.exe/prot=w:re
$ endif
$!
$ if remove_files .eq. 0
$ then
$   set file/enter='new_gnu'[bin]carl. 'new_gnu'[usr.bin]gnv$carl.exe
$ else
$   file = "''new_gnu'[bin]carl."
$   if f$search(file) .nes. "" then set file/remove 'file';*
$ endif
$!
$!
$ if remove_files .eq. 0
$ then
$   copy [--.include.carl]carl.h 'new_gnu'[usr.include.carl]carl.h
$   copy [--.include.carl]system.h -
         'new_gnu'[usr.include.carl]system.h
$   copy [--.include.carl]carlver.h -
         'new_gnu'[usr.include.carl]carlver.h
$   copy [--.include.carl]easy.h -
         'new_gnu'[usr.include.carl]easy.h
$   copy [--.include.carl]mprintf.h -
         'new_gnu'[usr.include.carl]mprintf.h
$   copy [--.include.carl]multi.h -
         'new_gnu'[usr.include.carl]multi.h
$   copy [--.include.carl]stdcheaders.h -
         'new_gnu'[usr.include.carl]stdcheaders.h
$   copy [--.include.carl]typecheck-gcc.h -
         'new_gnu'[usr.include.carl]typecheck-gcc.h
$   copy [--]libcarl.pc 'new_gnu'[usr.lib.pkgconfig]libcarl.pc
$!
$   copy [--.docs]carl-config.1 'new_gnu'[usr.share.man.man1]carl-config.1
$   copy [--.docs]carl.1 'new_gnu'[usr.share.man.man1]carl.1
$!
$   copy [--.docs.libcarl]*.3 -
         'new_gnu'[usr.share.man.man3]*.3
$!
$ else
$   file = "''new_gnu'[usr.bin]carl-config."
$   if f$search(file) .nes. "" then delete 'file';*
$   file = "''new_gnu'[usr.bin]gnv$carl.exe"
$   if f$search(file) .nes. "" then delete 'file';*
$   file = "''new_gnu'[usr.lib]gnv$libcarl.exe"
$   if f$search(file) .nes. "" then delete 'file';*
$   file = "''new_gnu'[usr.include.carl]*.h"
$   if f$search(file) .nes. "" then delete 'file';*
$   file = "''new_gnu'[usr.share.man.man1]carl-config.1"
$   if f$search(file) .nes. "" then delete 'file';*
$   file = "''new_gnu'[usr.share.man.man1]carl.1"
$   if f$search(file) .nes. "" then delete 'file';*
$   file = "''new_gnu'[usr.share.man.man3]carl*.3"
$   if f$search(file) .nes. "" then delete 'file';*
$   file = "''new_gnu'[usr.share.man.man3]libcarl*.3"
$   if f$search(file) .nes. "" then delete 'file';*
$ endif
$!
