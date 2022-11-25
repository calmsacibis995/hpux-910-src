/* @(#) $Revision: 66.2 $ */   

/****************************************************************************

 	NUNIX DEBUGGER - common definitions
 
*****************************************************************************
			debugger revisions for NUNIX by - J. Test  1/81
			debugger revisions for NUNIX by - MFM 5/82
****************************************************************************/


/* #include <a.out.h> ~user.h does include this file */
/* #include <sys/types.h>	not used? - SMT */
#include <sys/param.h>		/* used sys/user.h */
/* #ifndef MANX                 delete later - SMT
 * #include <sys/mknod.h>
 * #include <sys/page.h>
 * #endif
 */
/* #include <sys/vmparam.h>	not used? - SMT */
#include <fcntl.h>
/* #include <sys/dir.h>		not used? - SMT */
#include <machine/reg.h>
/* #include <sys/signal.h>	not used? - SMT */
#include <sys/user.h>
#include <sys/core.h>
#include <termio.h>
#include <stdio.h>
/* #include <signal.h>		not used? - SMT */
#include <sys/ptrace.h>		/* added  - SMT */

/* ----------------------------------------------------------------- */

#define W310KERNEL "9000/31"		/* uname.machine for WOPR 310 kernel */
#define W310KERNEL_LEN 7		/* length of W310KERNEL */

/* ----------------------------------------------------------------- */

#define	USRSTART	0x0
#define	PAGESIZE	ctob(1)
int	uarea_address;
int	float_soft;
int	mc68881;
int	mc68020;
int	umm_flag;
int	w310_flag;
int	Dragon_flag;


#define VARB	11
#define VARD	13
#define VARE	14
#define VARF	15
#define VARM	22
#define VARR	27
#define VARS	28
#define VART	29

#define COREMAGIC 0140000

#define NSP	0
#define	ISP	1
#define	DSP	2
#define STAR	4
#define STARCOM 0200
#define DSYM	7
#define ISYM	2
#define ASYM	1
#define NSYM	0
#define ESYM	(-1)
#define BKPTSET	1
#define BKPTEXEC 2
#define NCHNAM  SYMLENGTH	/* max length of a symbol MFM */
#define MAXHDIGITS 8	/* max length of a hex number address MFM */

#define STSIZE 2011
/* STSIZE should be the same as the loader's NSYM */

#define BKPTI	0x4E410000 	/* Trap 1, breakpoint instruction */

#define	EXIT	8
#define SINGLE	9

/* KERNEL_TUNE_DLB */
#define PS	SP+1
#define PC	SP+2
/* KERNEL_TUNE_DLB */

#define a6	AR6
#define sp	SP
#define ps	PS
#define pc	PC
#define fs	FSTATUS
#define fe	FERRBIT		/* register offset definitions (local)	*/

#define MAXPOS	80
#define MAXLIN	128
#define EOR	'\n'
#define TB	'\t'
#define QUOTE	0200
#define STRIP	0177
#define LOBYTE	0377
#define EVEN	-2


#define leng(a)		((long)((unsigned)(a)))
#define shorten(a)	((int)(a))

#define itol68(a,b)	((a << 16) | (b & 0xFFFF))

struct termio	adbtty, subtty;
extern int	adb_fcntl_flags, sub_fcntl_flags;

#define TRUE	 (-1)
#define FALSE	0
#define LOBYTE	0377
#define HIBYTE	0177400
#define STRIP	0177
#define HEXMSK	017

#define SPACE	' '
#define TB	'\t'
#define NL	'\n'

#define DBNAME "adb\n"

typedef	struct symb	SYMTAB;
typedef struct symb	*SYMPTR;
typedef	struct exec	BHDR;

/* this stuff is extracted from a.out.h, and should really
   be done with a #include */



struct symb	/* this struct parallels nlist_ in a.out.h */
{
	long	vals;			/* symbol value */
	unsigned char	symf;		/* symbol type */
	unsigned char   slength;	/* symbol length (no \0) MFM */
	int	smtp;			/* SYMTYPE */
	char	*symc;			/* pointer to symbol name */
};

#define SYMCHK 047
#define SYMTYPE(st)	((st>=041 || (st>=02 && st<=04))\
			? ((st&07)>=3 ? DSYM : (st&07)) : NSYM)
#define MAXCOM	64
#define MAXARG	32
#define LINSIZ	256
typedef	void		VOID;
typedef	float		REAL;
typedef	double		L_REAL;
typedef	unsigned	POS;
typedef	char		BOOL;
typedef	char		CHAR;
typedef	char		*STRING;
typedef	char		MSG[];
typedef	struct map	MAP;
typedef	MAP		*MAPPTR;
typedef	struct bkpt	BKPT;
typedef	BKPT		*BKPTR;
typedef struct proc_info PROC_REGS;
typedef struct proc_exec PROCEXEC;
typedef struct corehead  COREHEAD;

long		inkdot();
SYMPTR		lookupsym();
SYMPTR		symget();
POS		get();
POS		chkget();
STRING		exform();
long		round();
BKPTR		scanbkpt();
VOID		fault();

/* file address maps */
struct map {
	long	b1;	/* entry point */
	long	e1;	/* entry point + (0407 ? sizeof(data)+sizeof(text) :
						 sizeof(text) */
	long	f1;	/* a.out header size */
	long	b2;	/* data area base */
	long	e2;
	long	f2;
	int	ufd;
};

struct bkpt {
	int	loc;
	int	ins;
	int	count;
	int	initcnt;
	int	flag;
	CHAR	comm[MAXCOM];
	BKPT	*nxtbkpt;
};

typedef	struct reglist	REGLIST;

typedef	REGLIST		*REGPTR;
struct reglist {
	STRING	rname;
	int	roffs;
	long	rval;
};

typedef struct coremap COREMAP;

struct coremap {
	short type;
	long b;
	long e;
	long f;
	COREMAP *next_map;
};

typedef struct ptd_hdr_t TDHDR;

struct ptd_hdr_t {
  short	cnt;
  unsigned int	siz;
  unsigned int	bas;
};

