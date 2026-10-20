// Example for use of GNU gettext.
// This file is in the public domain.

// Source code of the ISO C++ 20 program.

// Note: The API has changed three years after ISO C++ 20. Code that was working
// fine with g++ 13.1, 13.2 and clang++ 17, 18 (with option -std=gnu++20)
// no longer compiles with g++ 13.3 or newer and clang++ 19 or newer.  Thus the
// need to test __cpp_lib_format, whose value is 202106L for the older compilers
// and 202110L for the newer compilers.  See
// <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2905r2.html>.
// The replacement API, presented in
// <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2918r2.html>,
// uses a new symbol std::runtime_format, that
//   - does not exist in g++ 13.3,
//   - exists in g++ 14 or newer and clang++ 19 or newer, but requires the
//     option -std=gnu++26.

#include <format>
#include <iostream>
using namespace std;

// Get setlocale() declaration.
#include <locale.h>

// Get getpid() declaration.
#if defined _WIN32 && !defined __CYGWIN__
/* native Windows API */
# include <process.h>
# define getpid _getpid
#else
/* POSIX API */
# include <unistd.h>
#endif

// Get gettext(), textdomain(), bindtextdomain() declaration.
#include "gettext.h"
// Define shortcut for gettext().
#define _(string) gettext (string)

int
main ()
{
  setlocale (LC_ALL, "");
  textdomain ("hello-c++20");
  bindtextdomain ("hello-c++20", LOCALEDIR);

  cout << _("Hello, world!") << endl;
#if __cpp_lib_format <= 202106L
  cout << vformat (_("This program is running as process number {:d}."),
                   make_format_args (getpid ()))
#else
  cout << format (runtime_format (_("This program is running as process number {:d}.")),
                  getpid ())
#endif
       << endl;
}
