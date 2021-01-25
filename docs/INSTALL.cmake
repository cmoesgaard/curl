                                  _   _ ____  _
                              ___| | | |  _ \| |
                             / __| | | | |_) | |
                            | (__| |_| |  _ <| |___
                             \___|\___/|_| \_\_____|

                                How To Compile with CMake

Building with CMake
==========================
   This document describes how to compile, build and install carl and libcarl
   from source code using the CMake build tool. To build with CMake, you will
   of course have to first install CMake.  The minimum required version of
   CMake is specified in the file CMakeLists.txt found in the top of the carl
   source tree. Once the correct version of CMake is installed you can follow
   the instructions below for the platform you are building on.

   CMake builds can be configured either from the command line, or from one
   of CMake's GUI's.

Current flaws in the carl CMake build
=====================================

   Missing features in the cmake build:

   - Builds libcarl without large file support
   - Does not support all SSL libraries (only OpenSSL, Schannel,
     Secure Transport, and mbed TLS, NSS, WolfSSL)
   - Doesn't allow different resolver backends (no c-ares build support)
   - No RTMP support built
   - Doesn't allow build carl and libcarl debug enabled
   - Doesn't allow a custom CA bundle path
   - Doesn't allow you to disable specific protocols from the build
   - Doesn't find or use krb4 or GSS
   - Rebuilds test files too eagerly, but still can't run the tests
   - Doesn't detect the correct strerror_r flavor when cross-compiling (issue #1123)


Command Line CMake
==================
   A CMake build of carl is similar to the autotools build of carl. It
   consists of the following steps after you have unpacked the source.

    1. Create an out of source build tree parallel to the carl source
       tree and change into that directory

    $ mkdir carl-build
    $ cd carl-build

    2. Run CMake from the build tree, giving it the path to the top of
       the carl source tree.  CMake will pick a compiler for you. If you
       want to specify the compile, you can set the CC environment
       variable prior to running CMake.

    $ cmake ../carl
    $ make

    3. Install to default location:

    $ make install

    (The test suite does not work with the cmake build)

ccmake
=========
     CMake comes with a curses based interface called ccmake.  To run ccmake on
     a carl use the instructions for the command line cmake, but substitute
     ccmake ../carl for cmake ../carl.  This will bring up a curses interface
     with instructions on the bottom of the screen. You can press the "c" key
     to configure the project, and the "g" key to generate the project. After
     the project is generated, you can run make.

cmake-gui
=========
     CMake also comes with a Qt based GUI called cmake-gui. To configure with
     cmake-gui, you run cmake-gui and follow these steps:
        1. Fill in the "Where is the source code" combo box with the path to
        the carl source tree.
        2. Fill in the "Where to build the binaries" combo box with the path
        to the directory for your build tree, ideally this should not be the
        same as the source tree, but a parallel directory called carl-build or
        something similar.
        3. Once the source and binary directories are specified, press the
        "Configure" button.
        4. Select the native build tool that you want to use.
        5. At this point you can change any of the options presented in the
        GUI.  Once you have selected all the options you want, click the
        "Generate" button.
        6. Run the native build tool that you used CMake to generate.
