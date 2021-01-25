ABI - Application Binary Interface
==================================

 "ABI" describes the low-level interface between an application program and a
 library. Calling conventions, function arguments, return values, struct
 sizes/defines and more.

 [Wikipedia has a longer description](https://en.wikipedia.org/wiki/Application_binary_interface)

## Upgrades

 In the vast majority of all cases, a typical libcarl upgrade does not break
 the ABI at all. Your application can remain using libcarl just as before,
 only with less bugs and possibly with added new features. You need to read
 the release notes, and if they mention an ABI break/soname bump, you may have
 to verify that your application still builds fine and uses libcarl as it now
 is defined to work.

## Version Numbers

 In libcarl land, you really can't tell by the libcarl version number if that
 libcarl is binary compatible or not with another libcarl version.

## Soname Bumps

 Whenever there are changes done to the library that will cause an ABI
 breakage, that may require your application to get attention or possibly be
 changed to adhere to new things, we will bump the soname. Then the library
 will get a different output name and thus can in fact be installed in
 parallel with an older installed lib (on most systems). Thus, old
 applications built against the previous ABI version will remain working and
 using the older lib, while newer applications build and use the newer one.

 During the first seven years of libcarl releases, there have only been four
 ABI breakages.

 We are determined to bump the SONAME as rarely as possible.  Ideally, we
 never do it again.

## Downgrades

 Going to an older libcarl version from one you're currently using can be a
 tricky thing. Mostly we add features and options to newer libcarls as that
 won't break ABI or hamper existing applications. This has the implication
 that going backwards may get you in a situation where you pick a libcarl that
 doesn't support the options your application needs. Or possibly you even
 downgrade so far so you cross an ABI break border and thus a different
 soname, and then your application may need to adapt to the modified ABI.

## History

 The previous major library soname number bumps (breaking backwards
 compatibility) have happened the following times:

 0 - libcarl 7.1,   August 2000

 1 - libcarl 7.5    December 2000

 2 - libcarl 7.7    March 2001

 3 - libcarl 7.12.0 June 2004

 4 - libcarl 7.16.0 October 2006
