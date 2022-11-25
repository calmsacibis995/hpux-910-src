/*
 * $Header: unix.h,v 1.1.109.5 92/02/28 16:02:20 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


/* system function type declarations */

#ifdef	USE_PROTOTYPES
extern void free(caddr_t ptr);
extern int send(int s, caddr_t msg, int len, int flags);
extern int getpeername(int s, struct sockaddr * name, int *namelen);
extern int fcntl(int fd, int cmd, int arg);
extern int setsockopt(int s, int level, int optname, caddr_t optval, int optlen);
extern int close(int fd);
extern int connect(int s, struct sockaddr * name, int namelen);
extern int accept(int s, struct sockaddr * addr, int *addrlen);
extern int bind(int s, struct sockaddr * name, int namelen);
extern int listen(int s, int n);
extern int recv(int s, caddr_t buff, int len, int flags);
extern void qsort(caddr_t base, int nel, int width, int (*compar) ());
extern int gettimeofday(struct timeval * tp, struct timezone * tzp);
extern int sendto(int s, caddr_t msg, int len, int flags, struct sockaddr * to, int tolen);
extern int ioctl(int d, unsigned long request, caddr_t argp);
extern int nlist(const char *filename, struct nlist * nl);
extern int open(const char *path, int flags, int mode);
extern int read(int d, caddr_t buf, int nbytes);
extern int gethostname(char *name, int namelen);
extern int fork(void);
extern void exit(int status);
extern int getdtablesize(void);
extern int getpid(void);
extern void openlog(const char *ident, int logopt, int facility);
extern void setlogmask(int maskpri);
extern void srandom(int seed);
extern long random();
extern int chdir(const char *path);
extern void abort(void);
extern void insque(struct qelem * elem, struct qelem * pred);
extern void remque(struct qelem * elem);
extern int fputs(const char *s, FILE * stream);
extern int kill(int pid, int sig);
extern int setitimer(int which, struct itimerval * value, struct itimerval * ovalue);
extern int recvmsg(int s, struct msghdr * msg, int flags);
extern int select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout);
extern int sigblock(int mask);
extern int sigsetmask(int mask);
extern int wait3(union wait * status, int options, struct rusage * rusage);
extern int sigvec(int sig, struct sigvec * vec, struct sigvec * ovec);
extern int socket(int domain, int type, int protocol);
extern void sleep(unsigned seconds);
extern int fclose(FILE * fp);
extern int stat(const char *path, struct stat * buf);
extern int setlinebuf(FILE * stream);
extern int fflush(FILE * fp);
extern int fputc(char c, FILE * stream);
extern int syslog();			/* VARARGS */
extern caddr_t calloc(unsigned nelem, unsigned elsize);
extern caddr_t malloc(unsigned size);
extern caddr_t realloc((caddr_t), unsigned size);
extern caddr_t alloca(int size);
extern int atoi(const char *nptr);
extern char *index(), *rindex();
extern char *getwd(char *pathname);
extern sethostent(int stayopen);
extern endhostent(void);
extern endnetent(void);
extern setnetent(int stayopen);

#else				/* USE_PROTOTYPES */
extern void free();
extern int send();
extern int getpeername();
extern int fcntl();
extern int setsockopt();
extern int close();
extern int connect();
extern int accept();
extern int bind();
extern int listen();
extern int recv();
extern void qsort();
extern int gettimeofday();
extern int sendto();
extern int ioctl();
extern int nlist();
extern int open();
extern int read();
extern int gethostname();
extern int fork();
extern void exit();
extern int getdtablesize();
extern int getpid();
extern void openlog();
extern void setlogmask();
extern void srandom();
extern long random();
extern int chdir();
extern void abort();
extern void insque();
extern void remque();
extern int fputs();
extern int kill();
extern int setitimer();
extern int recvmsg();
extern int select();
extern int sigblock();
extern int sigsetmask();
extern int wait3();
extern int sigvec();
extern int socket();
extern void sleep();
extern int fclose();
extern int stat();
extern int setlinebuf();
extern int fflush();
extern int fputc();
extern int syslog();
extern caddr_t calloc();
extern caddr_t malloc();
extern caddr_t realloc();
extern caddr_t alloca();
extern int atoi();
extern long random();
extern char *index(), *rindex();
extern char *getwd();
extern sethostent();
extern endhostent();
extern endnetent();
extern setnetent();

#endif				/* USE_PROTOTYPES */
