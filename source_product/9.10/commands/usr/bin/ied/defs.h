/* @(#) $Revision: 66.1 $ */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/times.h>

/* #include "io.h" */

#define ERRIO 2
#define TMPSIZ 20
#define EOF (-1)
#define STRIP 0377
#define MAXFILES 10
#define MAXTRY 12
#define IOBSIZE 1024
#define TO_PRINT	0100	/* to make a control-character print */

#define 	GMACS	1<<0
#define 	EMACS	1<<1
#define		VIRAW	1<<2
#define		EDITVI	1<<3
#define		NOHIST	1<<4
#define		EDITMASK	GMACS|EMACS|VIRAW|EDITVI

#define FIXFLG 1
#define SIGSET 1

#define new_of(type,x)   ((type*)malloc((unsigned)sizeof(type)+(x)))

#define ESH 1
#define VSH 1
#define SIG_NORESTART 1

struct {
	unsigned states;
} st;

struct {
	unsigned trapnote;
} sh;

#   define is_option(m)	(opt_flags&(m))
extern int opt_flags;

/*
#define io_renumber(x,y) dup2((x),(y))
#define p_setout()
*/
