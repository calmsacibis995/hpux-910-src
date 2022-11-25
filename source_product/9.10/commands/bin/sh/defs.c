/* @(#) $Revision: 66.1 $ */      
/*
 *	UNIX shell
 */

#include 		<setjmp.h>
#include		"mode.h"
#include		"name.h"
#include		<sys/param.h>

/* temp files and io */

int				output = 2;
int				ioset;
struct ionod	*iotemp;	/* files to be deleted sometime */
struct ionod	*fiotemp;	/* function files to be deleted sometime */
struct ionod	*iopend;	/* documents waiting to be read at NL */

/* substitution */
int				dolc;
tchar			**dolv;
struct dolnod	*argfor;
struct argnod	*gchain;

#ifdef NLS
char		*langpath[128];
#endif NLS

/* name tree and words */
int				wdval;
int				wdnum;
int				fndef;
int				nohash;
struct argnod	*wdarg;
int				wdset;
BOOL			reserv;

/* special names */
tchar			*pcsadr;
tchar			*pidadr;
tchar			*cmdadr;

/* transput */ 
char 			*tmpnam;
int 			serial; 
int 			peekc;
int 			peekn;
tchar 			*comdiv;

long			flags;
int				rwait;	/* flags read waiting */

/* error exits from various parts of shell */
jmp_buf			subshell;
jmp_buf			errshell;

/* fault handling */
BOOL			trapnote;

/* execflgs */
int				exitval;
int				retval;
BOOL			execbrk;
int				loopcnt;
int				breakcnt;
int 			funcnt;

int				wasintr;	/* used to tell if break or delete is hit
				   			   while executing a wait
							*/

int				eflag;

/* The following stuff is from stak.h	*/

tchar 			*stakbas;
tchar			*staktop;
tchar			*stakbot;
tchar			*stakbsy;
tchar 			*brkend;

