/*	$OpenBSD: unistd.h,v 1.67 2012/01/13 13:16:44 nigel Exp $ */
/*	$NetBSD: unistd.h,v 1.26.4.1 1996/05/28 02:31:51 mrg Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)unistd.h	5.13 (Berkeley) 6/17/91
 */

#ifndef _UNISTD_H_
#define	_UNISTD_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/unistd.h>

#define	STDIN_FILENO	0	/* standard input file descriptor */
#define	STDOUT_FILENO	1	/* standard output file descriptor */
#define	STDERR_FILENO	2	/* standard error file descriptor */

#if __XPG_VISIBLE || __POSIX_VISIBLE >= 200112
#define F_ULOCK         0	/* unlock locked section */
#define F_LOCK          1	/* lock a section for exclusive use */
#define F_TLOCK         2	/* test and lock a section for exclusive use */
#define F_TEST          3	/* test a section for locks by other procs */
#endif

#if __POSIX_VISIBLE
#define _POSIX_REENTRANT_FUNCTIONS	1
#define _POSIX_THREAD_SAFE_FUNCTIONS	200112L
#endif

#ifndef NULL
#ifdef 	__GNUG__
#define	NULL	__null
#elif defined(__cplusplus)
#define	NULL	0L
#else
#define	NULL	((void *)0)
#endif
#endif

__BEGIN_DECLS
//__dead void	 _exit(int);
//int	 access(const char *, int);
unsigned int alarm(unsigned int);
//int	 chdir(const char *);
//int	 chown(const char *, uid_t, gid_t);
int	 close(int);
int	 dup(int);
int	 dup2(int, int);
//int	 execl(const char *, const char *, ...)
//	    __attribute__((sentinel));
//int	 execle(const char *, const char *, ...);
//int	 execlp(const char *, const char *, ...)
//	    __attribute__((sentinel));
//int	 execv(const char *, char * const *);
//int	 execve(const char *, char * const *, char * const *);
//int	 execvp(const char *, char * const *);
pid_t	 fork(void);
long	 fpathconf(int, int);
char	*getcwd(char *, size_t)
		__attribute__((__bounded__(__string__,1,2)));
gid_t	 getegid(void);
uid_t	 geteuid(void);
gid_t	 getgid(void);
int	 getgroups(int, gid_t *);
char	*getlogin(void);
pid_t	 getpgrp(void);
pid_t	 getpid(void);
pid_t	 getppid(void);
uid_t	 getuid(void);
int	 isatty(int);
//int	 link(const char *, const char *);
//off_t	 lseek(int, off_t, int);
//long	 pathconf(const char *, int);
int	 pause(void);
int	 pipe(int *);
//ssize_t	 read(int, void *, size_t)
//		__attribute__((__bounded__(__buffer__,2,3)));
//int	 rmdir(const char *);
int	 setgid(gid_t);
int	 setuid(uid_t);
unsigned int sleep(unsigned int);
long	 sysconf(int);
pid_t	 tcgetpgrp(int);
int	 tcsetpgrp(int, pid_t);
char	*ttyname(int);
//int	 unlink(const char *);
//ssize_t	 write(int, const void *, size_t)
//		__attribute__((__bounded__(__buffer__,2,3)));

#if __POSIX_VISIBLE || __XPG_VISIBLE >= 300
pid_t	 setsid(void);
int	 setpgid(pid_t, pid_t);
#endif

#if __POSIX_VISIBLE >= 199209 || __XPG_VISIBLE
size_t	 confstr(int, char *, size_t)
		__attribute__((__bounded__(__string__,2,3)));
#ifndef _GETOPT_DEFINED_
#define _GETOPT_DEFINED_
int	 getopt(int, char * const *, const char *);
extern	 char *optarg;			/* getopt(3) external variables */
extern	 int opterr, optind, optopt, optreset;
/* XXX - getsubopt does not belong here */
int	 getsubopt(char **, char * const *, char **);
extern	 char *suboptarg;		/* getsubopt(3) external variable */
#endif /* _GETOPT_DEFINED_ */
#endif

#if __POSIX_VISIBLE >= 199506 || __XPG_VISIBLE
int	 fsync(int);
int	 ftruncate(int, off_t);
int	 getlogin_r(char *, size_t)
		__attribute__((__bounded__(__string__,1,2)));
#endif

#if __XPG_VISIBLE || __BSD_VISIBLE
char	*crypt(const char *, const char *);
int	 encrypt(char *, int);
int	 fchdir(int);
int	 fchown(int, uid_t, gid_t);
long	 gethostid(void);
char	*getwd(char *)
		__attribute__ ((__bounded__(__minbytes__,1,1024)));
int	 lchown(const char *, uid_t, gid_t);
int	 mkstemp(char *);
char	*mktemp(char *);
int	 nice(int);
int	 readlink(const char *, char *, size_t)
		__attribute__ ((__bounded__(__string__,2,3)));
int	 setkey(const char *);
int	 setpgrp(pid_t pid, pid_t pgrp);	/* obsoleted by setpgid() */
int	 setregid(gid_t, gid_t);
int	 setreuid(uid_t, uid_t);
void	 swab(const void *, void *, size_t);
void	 sync(void);
int	 truncate(const char *, off_t);
unsigned int	 ualarm(unsigned int, unsigned int);
int	 usleep(useconds_t);
pid_t	 vfork(void);
#endif

#if __XPG_VISIBLE >= 420
pid_t	 getpgid(pid_t);
pid_t	 getsid(pid_t);
#endif

#if __XPG_VISIBLE >= 500
ssize_t  pread(int, void *, size_t, off_t);
ssize_t  pwrite(int, const void *, size_t, off_t);
int	 ttyname_r(int, char *, size_t)
	    __attribute__((__bounded__(__string__,2,3)));
#endif

#if __BSD_VISIBLE ||  __XPG_VISIBLE <= 500
/* Interfaces withdrawn by X/Open Issue 5 Version 0 */
int	 brk(void *);
//int	 chroot(const char *);
int	 getdtablesize(void);
int	 getpagesize(void);
//char	*getpass(const char *);
void	*sbrk(int);
#endif

#if __POSIX_VISIBLE >= 200112 || __XPG_VISIBLE >= 420
int     lockf(int, int, off_t);
#endif

#if __POSIX_VISIBLE >= 200112 || __XPG_VISIBLE >= 420 || __BSD_VISIBLE
int	 symlink(const char *, const char *);
int	 gethostname(char *, size_t)
		__attribute__ ((__bounded__(__string__,1,2)));
int	 setegid(gid_t);
int	 seteuid(uid_t);
#endif

#if __POSIX_VISIBLE >= 200809
int	faccessat(int, const char *, int, int);
int	fchownat(int, const char *, uid_t, gid_t, int);
int	linkat(int, const char *, int, const char *, int);
ssize_t	readlinkat(int, const char *, char *, size_t);
int	symlinkat(const char *, int, const char *);
int	unlinkat(int, const char *, int);
#endif

#if __BSD_VISIBLE
int	 acct(const char *);
int	 closefrom(int);
int	 des_cipher(const char *, char *, int32_t, int);
int	 des_setkey(const char *);
void	 endusershell(void);
int	 exect(const char *, char * const *, char * const *);
char	*fflagstostr(u_int32_t);
int	 getdomainname(char *, size_t)
		__attribute__ ((__bounded__(__string__,1,2)));
int	 getgrouplist(const char *, gid_t, gid_t *, int *);
mode_t	 getmode(const void *, mode_t);
int	 getresgid(gid_t *, gid_t *, gid_t *);
int	 getresuid(uid_t *, uid_t *, uid_t *);
char	*getusershell(void);
int	 initgroups(const char *, gid_t);
int	 iruserok(u_int32_t, int, const char *, const char *);
int	 iruserok_sa(const void *, int, int, const char *, const char *);
int	 issetugid(void);
char	*mkdtemp(char *);
int	 mkstemps(char *, int);
int	 nfssvc(int, void *);
int	 profil(char *, size_t, unsigned long, unsigned int)
		__attribute__ ((__bounded__(__string__,1,2)));
int	 quotactl(const char *, int, int, char *);
int	 rcmd(char **, int, const char *,
	    const char *, const char *, int *);
int	 rcmd_af(char **, int, const char *,
	    const char *, const char *, int *, int);
int	 rcmdsh(char **, int, const char *,
	    const char *, const char *, char *);
char	*re_comp(const char *);
int	 re_exec(const char *);
int	 reboot(int);
int	 revoke(const char *);
int	 rfork(int opts);
int	 rresvport(int *);
int	 rresvport_af(int *, int);
int	 ruserok(const char *, int, const char *, const char *);
#ifndef _SELECT_DEFINED_
#define _SELECT_DEFINED_
struct timeval;
int	 select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#endif
int	 setdomainname(const char *, size_t);
int	 setgroups(int, const gid_t *);
int	 sethostid(long);
int	 sethostname(const char *, size_t);
int	 setlogin(const char *);
void	*setmode(const char *);
int	 setresgid(gid_t, gid_t, gid_t);
int	 setresuid(uid_t, uid_t, uid_t);
int	 setrgid(gid_t);
int	 setruid(uid_t);
void	 setusershell(void);
int	 strtofflags(char **, u_int32_t *, u_int32_t *);
int	 swapctl(int cmd, const void *arg, int misc);
int	 syscall(int, ...);
#endif /* __BSD_VISIBLE */
__END_DECLS

#endif /* !_UNISTD_H_ */
