/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Directory for data */
#define DATAFILE_DIR "/usr/local/share/wireshark"

/* Define if dladdr can be used to find the path of the executable */
#define DLADDR_FINDS_EXECUTABLE_PATH 1

/* Directory for docs */
#define DOC_DIR "/usr/local/share/doc/wireshark"

/* Link plugins statically into Wireshark */
#define ENABLE_STATIC 1

/* Enable AirPcap */
/* #undef HAVE_AIRPCAP */

/* Enable AirPDcap (WPA/WPA2 decryption) */
#define HAVE_AIRPDCAP 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the <arpa/nameser.h> header file. */
#define HAVE_ARPA_NAMESER_H 1

/* Define to 1 if you have the `bpf_image' function. */
/* #undef HAVE_BPF_IMAGE */

/* Define to use c-ares library */
/* #undef HAVE_C_ARES */

/* Define to 1 if you have the <direct.h> header file. */
/* #undef HAVE_DIRECT_H */

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to use GeoIP library */
/* #undef HAVE_GEOIP */

/* Define to 1 if you have the `gethostbyname2' function. */
#define HAVE_GETHOSTBYNAME2 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getprotobynumber' function. */
#define HAVE_GETPROTOBYNUMBER 1

/* Define to use GNU ADNS library */
/* #undef HAVE_GNU_ADNS */

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if compiling with GTK */
/* #undef HAVE_GTK */

/* Define to 1 if -ligemacintegration includes the GtkOSXApplication
   Integration functions. */
/* #undef HAVE_GTKOSXAPPLICATION */

/* Define if we have gzclearerr */
#define HAVE_GZCLEARERR 1

/* Define to use heimdal kerberos */
/* #undef HAVE_HEIMDAL_KERBEROS */

/* Define to 1 if the the Gtk+ framework or a separate library includes the
   Imendio IGE Mac OS X Integration functions. */
/* #undef HAVE_IGE_MAC_INTEGRATION */

/* Define if inet_ntop() prototype exists */
#define HAVE_INET_NTOP_PROTO 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `issetugid' function. */
#define HAVE_ISSETUGID 1

/* Define to use kerberos */
/* #undef HAVE_KERBEROS */

/* Define if krb5.h defines KEYTYPE_ARCFOUR_56 */
/* #undef HAVE_KEYTYPE_ARCFOUR_56 */

/* Define to 1 if you have the <lauxlib.h> header file. */
/* #undef HAVE_LAUXLIB_H */

/* Define to use the libcap library */
/* #undef HAVE_LIBCAP */

/* Define to use libgcrypt */
/* #undef HAVE_LIBGCRYPT */

/* Define to use gnutls library */
/* #undef HAVE_LIBGNUTLS */

/* Define to use libpcap library */
/* #undef HAVE_LIBPCAP */

/* Define to use libpcre library */
/* #undef HAVE_LIBPCRE */

/* Define to use libportaudio library */
/* #undef HAVE_LIBPORTAUDIO */

/* Define to 1 if you have the `smi' library (-lsmi). */
/* #undef HAVE_LIBSMI */

/* Define to use libz library */
#define HAVE_LIBZ 1

/* Define to 1 if you have the <lua5.1/lauxlib.h> header file. */
/* #undef HAVE_LUA5_1_LAUXLIB_H */

/* Define to 1 if you have the <lua5.1/lualib.h> header file. */
/* #undef HAVE_LUA5_1_LUALIB_H */

/* Define to 1 if you have the <lua5.1/lua.h> header file. */
/* #undef HAVE_LUA5_1_LUA_H */

/* Define to 1 if you have the <lualib.h> header file. */
/* #undef HAVE_LUALIB_H */

/* Define to use Lua 5.1 */
/* #undef HAVE_LUA_5_1 */

/* Define to 1 if you have the <lua.h> header file. */
/* #undef HAVE_LUA_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to use MIT kerberos */
/* #undef HAVE_MIT_KERBEROS */

/* Define to 1 if you have the `mkdtemp' function. */
#define HAVE_MKDTEMP 1

/* Define to 1 if you have the `mkstemp' function. */
#define HAVE_MKSTEMP 1

/* Define to 1 if you have the `mmap' function. */
#define HAVE_MMAP 1

/* Define to 1 if you have the `mprotect' function. */
#define HAVE_MPROTECT 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have OS X frameworks */
/* #undef HAVE_OS_X_FRAMEWORKS */

/* Define if pcap_breakloop is known */
/* #undef HAVE_PCAP_BREAKLOOP */

/* Define to 1 if you have the `pcap_create' function. */
/* #undef HAVE_PCAP_CREATE */

/* Define to 1 if you have the `pcap_createsrcstr' function. */
/* #undef HAVE_PCAP_CREATESRCSTR */

/* Define to 1 if you have the `pcap_datalink_name_to_val' function. */
/* #undef HAVE_PCAP_DATALINK_NAME_TO_VAL */

/* Define to 1 if you have the `pcap_datalink_val_to_description' function. */
/* #undef HAVE_PCAP_DATALINK_VAL_TO_DESCRIPTION */

/* Define to 1 if you have the `pcap_datalink_val_to_name' function. */
/* #undef HAVE_PCAP_DATALINK_VAL_TO_NAME */

/* Define to 1 if you have the `pcap_findalldevs' function and a pcap.h that
   declares pcap_if_t. */
/* #undef HAVE_PCAP_FINDALLDEVS */

/* Define to 1 if you have the `pcap_findalldevs_ex' function. */
/* #undef HAVE_PCAP_FINDALLDEVS_EX */

/* Define to 1 if you have the `pcap_freecode' function. */
/* #undef HAVE_PCAP_FREECODE */

/* Define to 1 if you have the `pcap_free_datalinks' function. */
/* #undef HAVE_PCAP_FREE_DATALINKS */

/* Define to 1 if you have the `pcap_get_selectable_fd' function. */
/* #undef HAVE_PCAP_GET_SELECTABLE_FD */

/* Define to 1 if you have the `pcap_lib_version' function. */
/* #undef HAVE_PCAP_LIB_VERSION */

/* Define to 1 if you have the `pcap_list_datalinks' function. */
/* #undef HAVE_PCAP_LIST_DATALINKS */

/* Define to 1 if you have the `pcap_open' function. */
/* #undef HAVE_PCAP_OPEN */

/* Define to 1 if you have the `pcap_open_dead' function. */
/* #undef HAVE_PCAP_OPEN_DEAD */

/* Define to 1 if you have WinPcap remote capturing support and prefer to use
   these new API features. */
/* #undef HAVE_PCAP_REMOTE */

/* Define to 1 if you have the `pcap_setsampling' function. */
/* #undef HAVE_PCAP_SETSAMPLING */

/* Define to 1 if you have the `pcap_set_datalink' function. */
/* #undef HAVE_PCAP_SET_DATALINK */

/* Define if libpcap version is known */
/* #undef HAVE_PCAP_VERSION */

/* Define if plugins are enabled */
#define HAVE_PLUGINS 1

/* Define to 1 if you have the <portaudio.h> header file. */
/* #undef HAVE_PORTAUDIO_H */

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define if python devel package available */
/* #undef HAVE_PYTHON */

/* Define to 1 to enable remote capturing feature in WinPcap library */
/* #undef HAVE_REMOTE */

/* Define if sa_len field exists in struct sockaddr */
/* #undef HAVE_SA_LEN */

/* Define to 1 if you have the `setresgid' function. */
#define HAVE_SETRESGID 1

/* Define to 1 if you have the `setresuid' function. */
#define HAVE_SETRESUID 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtoll' function. */
#define HAVE_STRTOLL 1

/* Define to 1 if you have the `sysconf' function. */
#define HAVE_SYSCONF 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/utsname.h> header file. */
#define HAVE_SYS_UTSNAME_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define if tm_zone field exists in struct tm */
#define HAVE_TM_ZONE 1

/* Define if tzname array exists */
/* #undef HAVE_TZNAME */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if we have xdg-open */
#define HAVE_XDG_OPEN 1

/* HTML viewer, e.g. mozilla */
#define HTML_VIEWER "xdg-open"

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define if inet/aton.h needs to be included */
/* #undef NEED_INET_ATON_H */

/* Define if inet/v6defs.h needs to be included */
/* #undef NEED_INET_V6DEFS_H */

/* Define if strerror.h needs to be included */
/* #undef NEED_STRERROR_H */

/* Define if strptime.h needs to be included */
/* #undef NEED_STRPTIME_H */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
//#define PACKAGE "wireshark"
//
///* Define to the address where bug reports for this package should be sent. */
//#define PACKAGE_BUGREPORT ""
//
///* Define to the full name of this package. */
//#define PACKAGE_NAME "wireshark"
//
///* Define to the full name and version of this package. */
//#define PACKAGE_STRING "wireshark 1.5.2"
//
///* Define to the one symbol short name of this package. */
//#define PACKAGE_TARNAME "wireshark"
//
///* Define to the home page for this package. */
//#define PACKAGE_URL ""
//
///* Define to the version of this package. */
//#define PACKAGE_VERSION "1.5.2"

/* Define if we are using version of of the Portaudio library API */
/* #undef PORTAUDIO_API_1 */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.5.2"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define as the string to precede declarations of routines that never return
   */
#define WS_MSVC_NORETURN /**/

/* Define as the string to precede external variable declarations in
   dynamically-linked libraries */
#define WS_VAR_IMPORT extern

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
/* #undef YYTEXT_POINTER */

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */
