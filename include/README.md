# include

Public include files for libcarl, external users.

They're all placed in the carl subdirectory here for better fit in any kind of
environment. You must include files from here using...

    #include <carl/carl.h>

... style and point the compiler's include path to the directory holding the
carl subdirectory. It makes it more likely to survive future modifications.

The public carl include files can be shared freely between different platforms
and different architectures.
