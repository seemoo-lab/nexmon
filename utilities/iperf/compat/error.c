/*---------------------------------------------------------------
 * Copyright (c) 1999,2000,2001,2002,2003
 * The Board of Trustees of the University of Illinois
 * All Rights Reserved.
 *---------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software (Iperf) and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and
 * the following disclaimers.
 *
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimers in the documentation and/or other materials
 * provided with the distribution.
 *
 *
 * Neither the names of the University of Illinois, NCSA,
 * nor the names of its contributors may be used to endorse
 * or promote products derived from this Software without
 * specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ________________________________________________________________
 * National Laboratory for Applied Network Research
 * National Center for Supercomputing Applications
 * University of Illinois at Urbana-Champaign
 * http://www.ncsa.uiuc.edu
 * ________________________________________________________________
 *
 * error.c
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * error handlers
 * ------------------------------------------------------------------- */

#include "headers.h"
#include "util.h"

/*---------------------------------------------------------------
 * Linux errors per [root@fedora iperf-2.1.9-rc2]# uname -r
 * 5.11.12-300.fc34.x86_64
 *
 * [root@fedora iperf-2.1.9-rc2]# errno -l
 * EPERM 1 Operation not permitted
 * ENOENT 2 No such file or directory
 * ESRCH 3 No such process
 * EINTR 4 Interrupted system call
 * EIO 5 Input/output error
 * ENXIO 6 No such device or address
 * E2BIG 7 Argument list too long
 * ENOEXEC 8 Exec format error
 * EBADF 9 Bad file descriptor
 * ECHILD 10 No child processes
 * EAGAIN 11 Resource temporarily unavailable
 * ENOMEM 12 Cannot allocate memory
 * EACCES 13 Permission denied
 * EFAULT 14 Bad address
 * ENOTBLK 15 Block device required
 * EBUSY 16 Device or resource busy
 * EEXIST 17 File exists
 * EXDEV 18 Invalid cross-device link
 * ENODEV 19 No such device
 * ENOTDIR 20 Not a directory
 * EISDIR 21 Is a directory
 * EINVAL 22 Invalid argument
 * ENFILE 23 Too many open files in system
 * EMFILE 24 Too many open files
 * ENOTTY 25 Inappropriate ioctl for device
 * ETXTBSY 26 Text file busy
 * EFBIG 27 File too large
 * ENOSPC 28 No space left on device
 * ESPIPE 29 Illegal seek
 * EROFS 30 Read-only file system
 * EMLINK 31 Too many links
 * EPIPE 32 Broken pipe
 * EDOM 33 Numerical argument out of domain
 * ERANGE 34 Numerical result out of range
 * EDEADLK 35 Resource deadlock avoided
 * ENAMETOOLONG 36 File name too long
 * ENOLCK 37 No locks available
 * ENOSYS 38 Function not implemented
 * ENOTEMPTY 39 Directory not empty
 * ELOOP 40 Too many levels of symbolic links
 * EWOULDBLOCK 11 Resource temporarily unavailable
 * ENOMSG 42 No message of desired type
 * EIDRM 43 Identifier removed
 * ECHRNG 44 Channel number out of range
 * EL2NSYNC 45 Level 2 not synchronized
 * EL3HLT 46 Level 3 halted
 * EL3RST 47 Level 3 reset
 * ELNRNG 48 Link number out of range
 * EUNATCH 49 Protocol driver not attached
 * ENOCSI 50 No CSI structure available
 * EL2HLT 51 Level 2 halted
 * EBADE 52 Invalid exchange
 * EBADR 53 Invalid request descriptor
 * EXFULL 54 Exchange full
 * ENOANO 55 No anode
 * EBADRQC 56 Invalid request code
 * EBADSLT 57 Invalid slot
 * EDEADLOCK 35 Resource deadlock avoided
 * EBFONT 59 Bad font file format
 * ENOSTR 60 Device not a stream
 * ENODATA 61 No data available
 * ETIME 62 Timer expired
 * ENOSR 63 Out of streams resources
 * ENONET 64 Machine is not on the network
 * ENOPKG 65 Package not installed
 * EREMOTE 66 Object is remote
 * ENOLINK 67 Link has been severed
 * EADV 68 Advertise error
 * ESRMNT 69 Srmount error
 * ECOMM 70 Communication error on send
 * EPROTO 71 Protocol error
 * EMULTIHOP 72 Multihop attempted
 * EDOTDOT 73 RFS specific error
 * EBADMSG 74 Bad message
 * EOVERFLOW 75 Value too large for defined data type
 * ENOTUNIQ 76 Name not unique on network
 * EBADFD 77 File descriptor in bad state
 * EREMCHG 78 Remote address changed
 * ELIBACC 79 Can not access a needed shared library
 * ELIBBAD 80 Accessing a corrupted shared library
 * ELIBSCN 81 .lib section in a.out corrupted
 * ELIBMAX 82 Attempting to link in too many shared libraries
 * ELIBEXEC 83 Cannot exec a shared library directly
 * EILSEQ 84 Invalid or incomplete multibyte or wide character
 * ERESTART 85 Interrupted system call should be restarted
 * ESTRPIPE 86 Streams pipe error
 * EUSERS 87 Too many users
 * ENOTSOCK 88 Socket operation on non-socket
 * EDESTADDRREQ 89 Destination address required
 * EMSGSIZE 90 Message too long
 * EPROTOTYPE 91 Protocol wrong type for socket
 * ENOPROTOOPT 92 Protocol not available
 * EPROTONOSUPPORT 93 Protocol not supported
 * ESOCKTNOSUPPORT 94 Socket type not supported
 * EOPNOTSUPP 95 Operation not supported
 * EPFNOSUPPORT 96 Protocol family not supported
 * EAFNOSUPPORT 97 Address family not supported by protocol
 * EADDRINUSE 98 Address already in use
 * EADDRNOTAVAIL 99 Cannot assign requested address
 * ENETDOWN 100 Network is down
 * ENETUNREACH 101 Network is unreachable
 * ENETRESET 102 Network dropped connection on reset
 * ECONNABORTED 103 Software caused connection abort
 * ECONNRESET 104 Connection reset by peer
 * ENOBUFS 105 No buffer space available
 * EISCONN 106 Transport endpoint is already connected
 * ENOTCONN 107 Transport endpoint is not connected
 * ESHUTDOWN 108 Cannot send after transport endpoint shutdown
 * ETOOMANYREFS 109 Too many references: cannot splice
 * ETIMEDOUT 110 Connection timed out
 * ECONNREFUSED 111 Connection refused
 * EHOSTDOWN 112 Host is down
 * EHOSTUNREACH 113 No route to host
 * EALREADY 114 Operation already in progress
 * EINPROGRESS 115 Operation now in progress
 * ESTALE 116 Stale file handle
 * EUCLEAN 117 Structure needs cleaning
 * ENOTNAM 118 Not a XENIX named type file
 * ENAVAIL 119 No XENIX semaphores available
 * EISNAM 120 Is a named type file
 * EREMOTEIO 121 Remote I/O error
 * EDQUOT 122 Disk quota exceeded
 * ENOMEDIUM 123 No medium found
 * EMEDIUMTYPE 124 Wrong medium type
 * ECANCELED 125 Operation canceled
 * ENOKEY 126 Required key not available
 * EKEYEXPIRED 127 Key has expired
 * EKEYREVOKED 128 Key has been revoked
 * EKEYREJECTED 129 Key was rejected by service
 * EOWNERDEAD 130 Owner died
 * ENOTRECOVERABLE 131 State not recoverable
 * ERFKILL 132 Operation not possible due to RF-kill
 * EHWPOISON 133 Memory page has hardware error
 * ENOTSUP 95 Operation not supported
 * ------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32

/* -------------------------------------------------------------------
 * Implement a simple Win32 strerror function for our purposes.
 * These error values weren't handled by FormatMessage;
 * any particular reason why not??
 * ------------------------------------------------------------------- */

struct mesg {
    DWORD       err;
    const char* str;
};

const struct mesg error_mesgs[] =
{
    { WSAEACCES,             "Permission denied"},
    { WSAEADDRINUSE,         "Address already in use"},
    { WSAEADDRNOTAVAIL,      "Cannot assign requested address"},
    { WSAEAFNOSUPPORT,       "Address family not supported by protocol family"},
    { WSAEALREADY,           "Operation already in progress"},
    { WSAECONNABORTED,       "Software caused connection abort"},
    { WSAECONNREFUSED,       "Connection refused"},
    { WSAECONNRESET,         "Connection reset by peer"},
    { WSAEDESTADDRREQ,       "Destination address required"},
    { WSAEFAULT,             "Bad address"},
    { WSAEHOSTDOWN,          "Host is down"},
    { WSAEHOSTUNREACH,       "No route to host"},
    { WSAEINPROGRESS,        "Operation now in progress"},
    { WSAEINTR,              "Interrupted function call."},
    { WSAEINVAL,             "Invalid argument."},
    { WSAEISCONN,            "Socket is already connected."},
    { WSAEMFILE,             "Too many open files."},
    { WSAEMSGSIZE,           "Message too long"},
    { WSAENETDOWN,           "Network is down"},
    { WSAENETRESET,          "Network dropped connection on reset"},
    { WSAENETUNREACH,        "Network is unreachable"},
    { WSAENOBUFS,            "No buffer space available."},
    { WSAENOPROTOOPT,        "Bad protocol option."},
    { WSAENOTCONN,           "Socket is not connected"},
    { WSAENOTSOCK,           "Socket operation on non-socket."},
    { WSAEOPNOTSUPP,         "Operation not supported"},
    { WSAEPFNOSUPPORT,       "Protocol family not supported"},
    { WSAEPROCLIM,           "Too many processes."},
    { WSAEPROTONOSUPPORT,    "Protocol not supported"},
    { WSAEPROTOTYPE,         "Protocol wrong type for socket"},
    { WSAESHUTDOWN,          "Cannot send after socket shutdown"},
    { WSAESOCKTNOSUPPORT,    "Socket type not supported."},
    { WSAETIMEDOUT,          "Connection timed out."},
    { WSATYPE_NOT_FOUND,     "Class type not found."},
    { WSAEWOULDBLOCK,        "Resource temporarily unavailable"},
    { WSAHOST_NOT_FOUND,     "Host not found."},
    { WSA_INVALID_HANDLE,    "Specified event object handle is invalid."},
    { WSA_INVALID_PARAMETER, "One or more parameters are invalid."},
    { WSA_IO_INCOMPLETE,     "Overlapped I/O event object not in signaled state."},
    { WSA_IO_PENDING,        "Overlapped operations will complete later."},
    { WSA_NOT_ENOUGH_MEMORY, "Insufficient memory available."},
    { WSANOTINITIALISED,     "Successful WSAStartup not yet performed."},
    { WSANO_DATA,            "Valid name, no data record of requested type."},
    { WSANO_RECOVERY,        "This is a non-recoverable error."},
    { WSASYSCALLFAILURE,     "System call failure."},
    { WSASYSNOTREADY,        "Network subsystem is unavailable."},
    { WSATRY_AGAIN,          "Non-authoritative host not found."},
    { WSAVERNOTSUPPORTED,    "WINSOCK.DLL version out of range."},
    { WSAEDISCON,            "Graceful shutdown in progress."},
    { WSA_OPERATION_ABORTED, "Overlapped operation aborted."},
    { 0,                     "No error."}

    /* These appeared in the documentation, but didn't compile.
     * { WSAINVALIDPROCTABLE,   "Invalid procedure table from service provider." },
     * { WSAINVALIDPROVIDER,    "Invalid service provider version number." },
     * { WSAPROVIDERFAILEDINIT, "Unable to initialize a service provider." },
     */

}; /* end error_mesgs[] */

const char* winsock_strerror( DWORD inErrno );

/* -------------------------------------------------------------------
 * winsock_strerror
 *
 * returns a string representing the error code. The error messages
 * were taken from Microsoft's online developer library.
 * ------------------------------------------------------------------- */

const char* winsock_strerror( DWORD inErrno ) {
    const char* str = "Unknown error";
    int i;
    for ( i = 0; i < sizeof(error_mesgs); i++ ) {
        if ( error_mesgs[i].err == inErrno ) {
            str = error_mesgs[i].str;
            break;
        }
    }

    return str;
} /* end winsock_strerror */

#endif /* WIN32 */

/* -------------------------------------------------------------------
 * warn
 *
 * Prints message and return
 * ------------------------------------------------------------------- */

void warn( const char *inMessage, const char *inFile, int inLine ) {
    fflush( 0 );

#ifdef NDEBUG
    fprintf( stderr, "%s failed\n", inMessage );
#else

    /* while debugging output file/line number also */
    fprintf( stderr, "%s failed (%s:%d)\n", inMessage, inFile, inLine );
#endif
} /* end warn */

/* -------------------------------------------------------------------
 * warn_errno
 *
 * Prints message and errno message, and return.
 * ------------------------------------------------------------------- */

void warn_errno( const char *inMessage, const char *inFile, int inLine ) {
    int my_err;
    const char* my_str;

    /* get platform's errno and error message */
#ifdef WIN32
    my_err = WSAGetLastError();
    my_str = winsock_strerror( my_err );
#else
    my_err = errno;
    my_str = strerror( my_err );
#endif

    fflush( 0 );

#ifdef NDEBUG
    fprintf( stderr, "%s failed: %s\n", inMessage, my_str );
#else

    /* while debugging output file/line number and errno value also */
    fprintf( stderr, "%s failed (%s:%d): %s (%d)\n",
             inMessage, inFile, inLine, my_str, my_err );
#endif
} /* end warn_errno */

int errno_decode (char *text, size_t len) {
    int my_err;
    const char* my_str;
    /* get platform's errno and error message */
#ifdef WIN32
    my_err = WSAGetLastError();
    my_str = winsock_strerror( my_err );
#else
    my_err = errno;
    my_str = strerror( my_err );
#endif
    strncpy(text, my_str, len);
    return my_err;
}

#ifdef __cplusplus
} /* end extern "C" */
#endif
