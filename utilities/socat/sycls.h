/* source: sycls.h */
/* Copyright Gerhard Rieger */
/* Published under the GNU General Public License V.2, see file COPYING */

#ifndef __sycls_h_included
#define __sycls_h_included 1

#if WITH_SYCLS
struct termios;	/* prevent gcc from spitting silly warning */
struct utsname;
struct flock;
struct addrinfo;

mode_t Umask(mode_t mask);
int Open(const char *pathname, int flags, mode_t mode);
int Creat(const char *pathname, mode_t mode);
off_t Lseek(int fildes, off_t offset, int whence);
#if HAVE_LSEEK64
off64_t Lseek64(int fildes, off64_t offset, int whence);
#endif
pid_t Getpid(void);
pid_t Getppid(void);
pid_t Getpgrp(void);
int Getpgid(pid_t pid);
int Setpgid(pid_t pid, pid_t pgid);
int Setpgrp(void);
pid_t Tcgetpgrp(int fd);
int Tcsetpgrp(int fd, pid_t pgrpid);
pid_t Getsid(pid_t pid);
pid_t Setsid(void);
uid_t Getuid(void);
uid_t Geteuid(void);
int Setuid(uid_t uid);
gid_t Getgid(void);
gid_t Getegid(void);
int Setgid(gid_t gid);
int Initgroups(const char *user, gid_t group);
int Getgroups(int size, gid_t list[]);
int Setgroups(size_t size, const gid_t *list);
int Getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups);
int Chdir(const char *path);
int Chroot(const char *path);
int Gettimeofday(struct timeval *tv, struct timezone *tz);
int Mknod(const char *pathname, mode_t mode, dev_t dev);
int Mkfifo(const char *pathname, mode_t mode);
int Stat(const char *file_name, struct stat *buf);
int Fstat(int filedes, struct stat *buf);
int Lstat(const char *file_name, struct stat *buf);
#if HAVE_STAT64
int Stat64(const char *file_name, struct stat64 *buf);
int Fstat64(int filedes, struct stat64 *buf);
int Lstat64(const char *file_name, struct stat64 *buf);
#endif /* HAVE_STAT64 */
int Dup(int oldfd);
int Dup2(int oldfd, int newfd);
int Pipe(int filedes[2]);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
int Fcntl(int fd, int cmd);
int Fcntl_l(int fd, int cmd, long arg);
int Fcntl_lock(int fd, int cmd, struct flock *l);
int Ftruncate(int fd, off_t length);
#if HAVE_FTRUNCATE64
int Ftruncate64(int fd, off64_t length);
#endif /* HAVE_FTRUNCATE64 */
int Flock(int fd, int operation);
int Ioctl(int d, int request, void *argp);
int Ioctl_int(int d, int request, int arg);
int Close(int fd);
int Fchown(int fd, uid_t owner, gid_t group);
int Fchmod(int fd, mode_t mode);
int Unlink(const char *pathname);
int Symlink(const char *oldpath, const char *newpath);
int Readlink(const char *path, char *buf, size_t bufsiz);
int Chown(const char *path, uid_t owner, gid_t group);
int Chmod(const char *path, mode_t mode);
int Poll(struct pollfd *ufds, unsigned int nfds, int timeout);
int Select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	   struct timeval *timeout);
int Pselect(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    struct timespec *timeout, const sigset_t *sigmask);
pid_t Fork(void);
pid_t Waitpid(pid_t pid, int *status, int options);
#ifndef HAVE_TYPE_SIGHANDLER
typedef RETSIGTYPE (*sighandler_t)(int);
#endif
sighandler_t Signal(int signum, sighandler_t handler);
int Sigaction(int signum, const struct sigaction *act,
	      struct sigaction *oldact);
int Sigprocmask(int how, const sigset_t *set, sigset_t *oset);
unsigned int Alarm(unsigned int seconds);
int Kill(pid_t pid, int sig);
int Link(const char *oldpath, const char *newpath);
int Execvp(const char *file, char *const argv[]);
int System(const char *string);
int Socketpair(int d, int type, int protocol, int sv[2]);
#if _WITH_SOCKET
int Socket(int domain, int type, int protocol);
int Bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen);
int Connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
int Listen(int s, int backlog);
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int Getsockname(int s, struct sockaddr *name, socklen_t *namelen);
int Getpeername(int s, struct sockaddr *name, socklen_t *namelen);
int Getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int Setsockopt(int s, int level, int optname, const void *optval, int optlen);
int Recv(int s, void *buf, size_t len, int flags);
int Recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from,
	     socklen_t *fromlen);
int Recvmsg(int s, struct msghdr *msg, int flags);
int Send(int s, const void *mesg, size_t len, int flags);
int Sendto(int s, const void *msg, size_t len, int flags,
	   const struct sockaddr *to, socklen_t tolen);
int Shutdown(int fd, int how);
#endif /* _WITH_SOCKET */
unsigned int Sleep(unsigned int seconds);
void Usleep(unsigned long usec);
unsigned int Nanosleep(const struct timespec *req, struct timespec *rem);
int Pause(void);
struct hostent *Gethostbyname(const char *name);
int Getaddrinfo(const char *node, const char *service,
		const struct addrinfo *hints, struct addrinfo **res);
struct hostent *Getipnodebyname(const char *name, int af, int flags,
                                int *error_num);
void *Malloc(size_t size);
void *Calloc(size_t nmemb, size_t size);
void *Realloc(void *ptr, size_t size);
int Tcgetattr(int fd, struct termios *termios_p);
int Tcsetattr(int fd, int optional_actions, struct termios *termios_p);
char *Ttyname(int fd);
int Isatty(int fd);
struct winsize;	/* avoid warnings */
int Openpty(int *ptyfd, int *ttyfd, char *ptyname, struct termios *termp,
	    struct winsize *winp);
char *Ptsname(int fd);
int Grantpt(int fd);
int Unlockpt(int fd);
int Gethostname(char *name, size_t len);
int Uname(struct utsname *buf);
int Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int Atexit(void (*func)(void));
void Exit(int status);
void Abort(void);
int Pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		   void *(*start_routine)(void *), void *arg);
int Pthread_join(pthread_t thread, void **value_ptr);
int Mkstemp(char *template);
int Setenv(const char *name, const char *value, int overwrite);
void Unsetenv(const char *name);

char *Readline(const char *prompt);
void Using_history(void);
int Read_history(const char *filename);
int Write_history(const char *filename);
int Append_history(int nelements, const char *filename);
int Read_history(const char *filename);
void Add_history(const char *string);
#if WITH_GZIP
gzFile Gzdopen(int fd, const char *mode) {
int Gzread(gzFile file, voidp buf, unsigned len) {
int Gzwrite(gzFile file, const voidp buf, unsigned len) {
int Gzclose(gzFile file) {
#endif /* WITH_GZIP */

#else /* !WITH_SYCLS */

#define Umask(m) umask(m)
#define Open(p,f,m) open(p,f,m)
#define Creat(p,m) creat(p,m)
#define Lseek(f,o,w) lseek(f,o,w)
#define Lseek64(f,o,w) lseek64(f,o,w)
#define Getpid() getpid()
#define Getppid() getppid()
#define Getpgrp() getpgrp()
#define Getpgid(p) getpgid(p)
#define Setpgid(p,g) setpgid(p,g)
#define Setpgrp() setpgrp()
#define Tcgetpgrp(f) tcgetpgrp(f)
#define Tcsetpgrp(f,p) tcsetpgrp(f,p)
#define Getsid(p) getsid(p)
#define Setsid() setsid()
#define Getuid() getuid()
#define Geteuid() geteuid()
#define Setuid(u) setuid(u)
#define Getgid() getgid()
#define Getegid() getegid()
#define Setgid(g) setgid(g)
#define Initgroups(u,g) initgroups(u,g)
#define Getgroups(s,l) getgroups(s,l)
#define Setgroups(s,l) setgroups(s,l)
#define Getgrouplist(u,g,gs,n) getgrouplist(u,g,gs,n)
#define Chdir(p) chdir(p)
#define Chroot(p) chroot(p)
#define Gettimeofday(tv,tz) gettimeofday(tv,tz)
#define Mknod(p,m,d) mknod(p,m,d)
#define Mkfifo(p,m) mkfifo(p,m)
#define Stat(f,b) stat(f,b)
#define Stat64(f,b) stat64(f,b)
#define Fstat(f,b) fstat(f,b)
#define Fstat64(f,b) fstat64(f,b)
#define Lstat(f,b) lstat(f,b)
#define Lstat64(f,b) lstat64(f,b)
#define Dup(o) dup(o)
#define Dup2(o,n) dup2(o,n)
#define Pipe(f) pipe(f)
#define Read(f,b,c) read(f,b,c)
#define Write(f,b,c) write(f,b,c)
#define Fcntl(f,c) fcntl(f,c)
#define Fcntl_l(f,c,a) fcntl(f,c,a)
#define Fcntl_lock(f,c,l) fcntl(f,c,l)
#define Ftruncate(f,l) ftruncate(f,l)
#define Ftruncate64(f,l) ftruncate64(f,l)
#define Flock(f,o) flock(f,o)
#define Ioctl(d,r,a) ioctl(d,r,a)
#define Ioctl_int(d,r,a) ioctl(d,r,a)
#define Close(f) close(f)
#define Fchown(f,o,g) fchown(f,o,g)
#define Fchmod(f,m) fchmod(f,m)
#define Unlink(p) unlink(p)
#define Symlink(op,np) symlink(op,np)
#define Readlink(p,b,s) readlink(p,b,s)
#define Chown(p,o,g) chown(p,o,g)
#define Chmod(p,m) chmod(p,m)
#define Poll(u, n, t) poll(u, n, t)
#define Select(n,r,w,e,t) select(n,r,w,e,t)
#define Pselect(n,r,w,e,t,s) select(n,r,w,e,t,s)
#define Fork() fork()
#define Waitpid(p,s,o) waitpid(p,s,o)
#define Signal(s,h) signal(s,h)
#define Sigaction(s,a,o) sigaction(s,a,o)
#define Sigprocmask(h,s,o) sigprocmask(h,s,o)
#define Alarm(s) alarm(s)
#define Kill(p,s) kill(p,s)
#define Link(o,n) link(o,n)
#define Execvp(f,a) execvp(f,a)
#define System(s) system(s)
#define Socketpair(d,t,p,s) socketpair(d,t,p,s)
#define Socket(d,t,p) socket(d,t,p)
#define Bind(s,m,a) bind(s,m,a)
#define Connect(s,a,l) connect(s,a,l)
#define Listen(s,b) listen(s,b)
#define Accept(s,a,l) accept(s,a,l)
#define Getsockname(s,n,l) getsockname(s,n,l)
#define Getpeername(s,n,l) getpeername(s,n,l)
#define Getsockopt(s,d,n,v,l) getsockopt(s,d,n,v,l)
#define Setsockopt(s,d,n,v,l) setsockopt(s,d,n,v,l)
#define Recv(s,b,l,f) recv(s,b,l,f)
#define Recvfrom(s,b,bl,f,fr,fl) recvfrom(s,b,bl,f,fr,fl)
#define Recvmsg(s,m,f) recvmsg(s,m,f)
#define Send(s,m,l,f) send(s,m,l,f)
#define Sendto(s,b,bl,f,t,tl) sendto(s,b,bl,f,t,tl)
#define Shutdown(f,h) shutdown(f,h)
#define Sleep(s) sleep(s)
#define Usleep(u) usleep(u)
#define Nanosleep(req,rem) nanosleep(req,rem)
#define Pause() pause()
#define Gethostbyname(n) gethostbyname(n)
#define Getaddrinfo(n,s,h,r) getaddrinfo(n,s,h,r)
#define Getipnodebyname(n,a,f,e) getipnodebyname(n,a,f,e)
#define Malloc(s) malloc(s)
#define Calloc(n,s) calloc(n,s)
#define Realloc(p,s) realloc(p,s)
#define Tcgetattr(f,t) tcgetattr(f,t)
#define Tcsetattr(f,o,t) tcsetattr(f,o,t)
#define Ttyname(f) ttyname(f)
#define Isatty(f) isatty(f)
#define Openpty(p,t,n,i,f) openpty(p,t,n,i,f)
#define Ptsname(f) ptsname(f)
#define Grantpt(f) grantpt(f)
#define Unlockpt(f) unlockpt(f)
#define Getpgid(p) getpgid(p)
#define Gethostname(n,l) gethostname(n,l)
#define Uname(b) uname(b)
#define Sigprocmask(h,s,o) sigprocmask(h,s,o)
#define Atexit(f) atexit(f)
#define Exit(s) exit(s)
#define Abort() abort()
#define Pthread_create(t,attr,s,arg) pthread_create(t,attr,s,arg)
#define Pthread_join(t,ptr) pthread_join(t,ptr)
#define Mkstemp(t) mkstemp(t)
#define Setenv(n,v,o) setenv(n,v,o)
#define Unsetenv(n) unsetenv(n)

#define Readline(p) readline(p)
#define Using_history() using_history()
#define Read_history(f) read_history(f)
#define Write_history(f) write_history(f)
#define Append_history(n,f) append_history(n,f)
#define Read_history(f) read_history(f)
#define Add_history(s) add_history(s)

#define Gzdopen(f,m) gzdopen(f,m)
#define Gzread(f,b,l) gzread(f,b,l)
#define Gzwrite(f,b,l) gzwrite(f,b,l)
#define Gzclose(f) gzclose(f)

#endif /* !WITH_SYCLS */

#endif /* !defined(__sycls_h_included) */
