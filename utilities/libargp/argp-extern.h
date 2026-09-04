/* Compatibility shim. nexutil includes <argp-extern.h>; the previous gnulib
 * blend needed a pile of _GL_* / *_unlocked defines here before <argp.h> was
 * usable outside glibc. argp-standalone's <argp.h> is self-contained, so this
 * now just forwards to it. */
#include <argp.h>
