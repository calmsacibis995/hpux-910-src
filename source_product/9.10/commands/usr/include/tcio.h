/* @(#) $Revision: 64.1 $ */     
#ifndef _TCIO_INCLUDED /* allows multiple inclusion */
#define _TCIO_INCLUDED
#ifdef __hp9000s300
#include <sys/types.h>
#include <sys/cs80.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
extern int errno;
extern int errinfo;


#define BUFSIZE 1024
#define EQ(x,y)	(strcmp(x,y)==0)
#define IN	0
#define OUT	1
#define UTILITY	2
#define MAXINDEX (512 * BUFSIZE)
#define MININDEX (  4 * BUFSIZE)
#define DEFINDEX ( 64 * BUFSIZE)
#define YES	1
#define NO	0
/* exit errors */
#define BSIZERR		1
#define OPTERR		2
#define USEERR		3
#define BUFERR		4
#define DEVERR		5
#define NOINPERR	6
#define SUIDERR		7
#define BNUMERR		8
#define MNTERR		9
#define ORDERR		10
#define TAPERR		11
#define FILERR		12
#define KILLERR		13
#define FNUMERR		14
#define TFULLERR	15


/* GLOBALS */
extern char	*fname;
extern int	maxindex;
extern int	bufindex;
extern char	*buf;
extern unsigned	block_num;
extern int	fildes;
extern int	start_cart;
extern int	num_cart;

/* option flags to maintain option settings */
extern int	checkflag;
extern int	verifyflag;
extern int	bufflag;
extern int	releaseflag;
extern int	markflag;
extern int	verboseflag;
extern int	Systemflag;
extern int	singletapeflag;
extern int	eodmarkflag;
extern int	fileskipflag;
extern int	Merlinflag;
extern int	Merlinflag_n;

extern int	tape_length;
extern int	start_blk;
extern int	num_blk;
extern int	total_blk;
extern unsigned  checksum;

#endif /* __hp9000s300 */
#endif /* _TCIO_INCLUDED */
