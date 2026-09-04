#ifndef GLIB_TEST_SYMBOL_VISIBILITY
#define GLIB_TEST_SYMBOL_VISIBILITY

/* This is the same check that's done in configure to create config.h */
#ifdef _WIN32
#ifdef GLIB_TEST_STATIC_COMPILATION
#define GLIB_TEST_EXPORT_SYMBOL extern
#else
#ifdef _MSC_VER
#define GLIB_TEST_EXPORT_SYMBOL __declspec(dllexport) extern
#else
#define GLIB_TEST_EXPORT_SYMBOL __attribute__ ((visibility ("default"))) __declspec(dllexport) extern
#endif
#endif
/* Matches GCC and Clang */
#elif defined(__GNUC__) && (__GNUC__ >= 4)
# define GLIB_TEST_EXPORT_SYMBOL __attribute__((visibility("default"))) extern
#endif

#endif /* GLIB_TEST_SYMBOL_VISIBILITY */
