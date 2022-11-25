/*
 * @(#)cdb.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 16:22:11 $
 * $Locker:  $
 */

/*
 * Original version based on:
 * Revision 63.2  88/05/19  10:57:06  10:57:06  sjl (Steve Lilker)
 */

/*
 * Copyright Third Eye Software, 1983.
 * Copyright Hewlett Packard Company 1985.
 *
 * This module is part of the CDB/XDB symbolic debugger.  It is available
 * to Hewlett-Packard Company under an explicit source and binary license
 * agreement.  DO NOT COPY IT WITHOUT PERMISSION FROM AN APPROPRIATE HP
 * SOURCE ADMINISTRATOR.
 */

/*
 * This file declares types, macros, and structures used widely throughout
 * the debugger.
 */


/*
 * LIBRARY ROUTINE TYPES:
 *
 * Note that malloc() and free() are NOT here; all calls to them
 * must go through Malloc() and Free().
 */

extern long	lseek();
extern int	errno;
extern char	*strchr();
extern char	*strrchr();
extern char	*strcpy();
extern char	*strncpy();
extern char	*strcat();
extern char	*strncat();
extern int	strlen();
extern int	strspn();
extern long	time();
extern char	*getenv();

/*
 * Macro to capture all printf() output for record-all and paging:
 */

#define	printf	Printf


#ifdef HPE
/* Macros to intercept read, open and lseek calls for the early kludged
 * version of libc.  Read, Open, and Lseek are found in hpeio.c. 
 */

#ifdef KLUDGELIBC 

#define read  	Read
#define open 	Open
#define lseek	Lseek

#endif

/* _setjmp and _longjmp are part of libc and are used instead of 
 * setjmp and longjmp because they do not use signals.
 */

#define setjmp  _setjmp
#define longjmp _longjmp

#endif


/*
 * The rest of XDB's include files:
 * --------------------------------
 */

#include <stdio.h>
#include <ndir.h>
#define SYS_UINT 1
#define SYS_USHORT 1
#include "basic.h"
#include "sym.h"
#include "macdefs.h"
#include "tk.h"
#include "ty.h"


#ifdef OLDSYMTAB
/***********************************************************************
 * SOURCE FILE DESCRIPTOR:
 *
 * This builds the source file quick reference array in memory.
 */

typedef struct FDS {
	    long	isym;		/* first symbol for file	   */
	    ADRT	adrStart;	/* mem adr of start of file's code */
	    ADRT	adrEnd;		/* mem adr of end of file's code   */
	    char	*sbFile;	/* name of source file		   */
	    bits	fHasDecl : 1;	/* do we have a .d file?	   */
	    bits	fWarned	 : 1;	/* have warned about age problems? */
	    ushort	ilnMac;		/* lines in file (0 if don't know) */
	    int		ipd;		/* first proc for file, in PD []   */
	    uint	*rgLn;		/* line pointer array, if any	   */
	} FDR, *pFDR;
#endif

#define cbFDR	(sizeof (FDR))
#define ilnNil	0


#ifdef OLDSYMTAB
/***********************************************************************
 * PROCEDURE DESCRIPTOR:
 *
 * This builds the procedure quick reference array in memory.
 */

typedef struct PDS {
	    long	isym;		/* first symbol for proc	*/
	    ADRT	adrStart;	/* memory adr of start of proc	*/
#ifdef HPSYMTABII
	    ADRT	adrEntry;	/* memory adr of end of proc	*/
#endif
	    ADRT	adrEnd;		/* memory adr of end of proc	*/
	    char	*sbAlias;	/* alias name of procedure	*/
	    char	*sbProc;	/* real name of procedure	*/
	    ADRT	adrBp;		/* address of breakpoint	 */
#if (FOCUS || M68000)
	    short	inst;		/* instruction that lives there	 */
#else
#ifdef SPECTRUM
	    uint	inst;		/* instruction that lives there	 */
#endif
#endif
#ifdef HPSYMTAB
	    ushort	isd;		/* corresponding scope record	*/
#endif
#ifdef HPSYMTABII
            long	level;		/* nesting level (top=0)	*/
#endif

	} PDR, *pPDR;
#endif

#define cbPDR	(sizeof (PDR))
#define ipdNil	(-1)

#ifdef HPSYMTABII
#ifdef OLDSYMTAB
/***********************************************************************
 * MODULE DESCRIPTOR:
 *
 * This builds the module quick reference array in memory.
 */

typedef struct MDS {
	    long	isym;		/* first symbol for module	*/
	    ADRT	adrStart;	/* memory adr of start of mod.	*/
	    ADRT	adrEnd;		/* memory adr of end of mod.	*/
	    char	*sbAlias;	/* alias name of module   	*/
	    char	*sbMod;		/* real name of module		*/
            uint        imports: 1; 	/* does module have any imports?   */
            uint        vars_in_front: 1; /* module globals in front?  */
            uint        vars_in_gaps:  1; /* module globals in gaps?   */
            uint        unused      : 29;
            uint        unused2;	/* space for future stuff	*/
	} MDR, *pMDR;
#endif

#define cbMDR	(sizeof (MDR))
#define imdNil	(-1)

#else  /* HPSYMTAB */

#ifdef OLDSYMTAB
/***********************************************************************
 * SCOPE DESCRIPTOR:
 *
 * This builds the scope descriptor array in memory.
 */

typedef struct SDS {
	    ushort 	sdTy;		/* type of SDR	*/
	    ushort	isdParent;	/* SDR index of parent SDR */
	    ushort	isdSibling;	/* SDR index of sibling SDR */
	    ushort	isdChild;	/* SDR index of first child */
	    char	*sdName;	/* name of proc, func, or module */
	    long	sdIpd;		/* Pd index for proc-type SDR's */
	    long	sdIsymStart;	/* start isym of scope	*/
	    long	sdIsymEnd;	/* isym of scope END	*/
	} SDR, *pSDR;
#endif

#define cbSDR 	(sizeof (SDR))
#define isdNil	0


/***********************************************************************
 * LST PROCEDURE DESCRIPTOR:
 *
 * This builds the LST procedure quick reference array in memory.
 * Needed in memory to find values for non-active register variables.
 * S200 only.
 */

typedef struct LSTPDS {
	    ADRT	adrStart;	/* memory adr of start of proc	*/
	    char	*sbProc;	/* name of procedure		*/
	} LSTPDR, *pLSTPDR;

#define cbLSTPDR	(sizeof (LSTPDR))
#endif /* HPSYMTAB */

/***********************************************************************
 * MACRO DESCRIPTOR:
 *
 * This builds the macro data structure in memory.
 */

typedef struct MACS {
	    char	*sbMacro;	/* macro name                   */
	    char	*sbReplace;	/* macro replacement string     */
            struct MACS *pmacnext;      /* ptr to next defined macro    */
	} MACR, *pMACR;

#define cbMACR	(sizeof (MACR))

#define macNil		0	/* macros not being used		 */
#define macActive	1	/* macros are being used		 */
#define macSuspended	2	/* being used, but temporarily suspended */

#ifdef SPECTRUM
/***********************************************************************
 * STATES FOR ADOPTING PROCESS INDICATOR (SUPPORTS -P OPTION)
 *
 */

#define NOT_ADOPTING    0
#define ADOPTING        1
#define ADOPTED         2
#define DONE_ADOPTING   3


/***********************************************************************
 * CODE STACK-UNWIND DESCRIPTOR:
 *
 *  See the "Stack Unwinding design document"
 */

typedef struct UWS {
	    ADRT	adrstart;	/* low address of range         */
	    ADRT	adrend;		/* high address of range        */
            uint	cannot_unwind  : 1;
            uint	millicode      : 1;
            uint	milli_save_sr0 : 1;
            uint	region_descr   : 2;
            uint	save_srs       : 2;
            uint	entry_fr       : 4;
            uint	entry_gr       : 5;
            uint        args_stored    : 1;
            uint	call_fr        : 5;
            uint	call_gr        : 5;
            uint	save_sp        : 1;
            uint	save_rp        : 1;
            uint	save_mrp       : 1;
            uint	reserved       : 1;
            uint	cleanup_def    : 1;
            uint	hpe_int_mark   : 1;
            uint	hpux_int_mark  : 1;
            uint	large_frame_r3 : 1;
            uint	reserved2      : 2;
            uint	frame_size     :27;
	} SUR, *pSUR;

#define cbSUR	(sizeof (SUR))
#define isuNil	-1

/***********************************************************************
 * STUB STACK-UNWIND DESCRIPTOR:
 *
 *  See the "Stack Unwinding design document"
 */

typedef struct STUBUWS {
	    ADRT	adrstart;	/* low address of range         */
            uint	reserved1      : 5;
            uint	stub_type      : 3;
            uint	reserved2      : 3;
            uint	instr_num      : 5;
            uint	length         :16;
	} STUBSUR, *pSTUBSUR;

#define cbSTUBSUR  (sizeof (STUBSUR))

#define  long_branch_rp  1
#define  relocation      2
#define  import          3
#define  export          4
#define  export_reloc    6
#define  long_branch_mrp 7

#endif /* SPECTRUM */

/***********************************************************************
 * ASSERTION DESCRIPTOR:
 *
 * This builds the assertion array in memory.
 */

typedef int ASE, *pASE;

typedef struct ADS {
	    char	*sbCheck;	/* command list for assertion */
	    ASE		as;		/* state of this assertion    */
	} ADR, *pADR;

#define cbADR (sizeof (ADR))

#define asNil		0	/* auto checking not being used		 */
#define asActive	1	/* auto checking is being used		 */
#define asSuspended	2	/* being used, but temporarily suspended */


/***********************************************************************
 * BREAKPOINT DESCRIPTOR:
 *
 * This builds the breakpoint array in memory.
 */

typedef int BSE;

typedef struct BPS {
	    ADRT	adr;		/* address of breakpoint	 */
#if (FOCUS || M68000)
	    short	inst;		/* instruction that lives there	 */
#else
#ifdef SPECTRUM
	    uint	inst;		/* instruction that lives there	 */
#endif
#endif
            int         countmax;       /* original/reset count value    */
            BSE         bs;             /* state of this breakpoint      */
	    int		count;		/* times to hit before recognize */
	    char	*sbBp;		/* command list for bp, if any	 */
	    FLAGT	fTell;		/* don't report, regardless	 */
	} BPR, *pBPR;

#define cbBPR	(sizeof (BPR))

#define	ibpTemp	      0		/* where to save temporary bps for exec	 */
#define ibpNil	    (-1)	/* no breakpoint hit, or no current bp   */
#define ibpSignal   (-2)	/* signal received (other than SIGTRAP)  */
#define ibpInternal (-3)	/* internal (count == 0) bp encountered  */
#define ibpContinue (-4)	/* bp encountered but not yet recognized */

#define bsActiveOut	0	/* breakpoint active, not set in code    */
#define bsActive	1	/* breakpoint active, is set in code     */
#define bsSuspended	2	/* breakpoint suspended, not set in code */


/***********************************************************************
 * DISPLAY FORMATS:
 */

#define dfNil		 0		/* anonymous	*/
#define dfDecimal	 1		/* integer	*/
#define dfUnsigned	 2		/* uns integer	*/
#define dfOctal		 3		/* octal int	*/
#define dfHex		 4		/* hex int	*/
#define dfFFloat	 5		/* real, type F */
#define dfEFloat	 6		/* real, type E */
#define dfGFloat	 7		/* real, type G */
#define dfChar		 8		/* character	*/
#define dfByte		 9		/* decimal byte */
#define dfPStr		10		/* ptr to str	*/
#define dfStr		11		/* simple str	*/
#define dfProc		12		/* procedure	*/
#define dfAddr		13		/* address	*/
#define dfStruct	14		/* structure	*/
#define dfType		15		/* type info	*/
#define dfEnum		16		/* enumeration	*/

#define dfString200	17		/* Series 200 string	*/
#define dfString500	18		/* Series 500 string	*/
#define dfText		19		/* Pascal FILE of TEXT	*/
#define dfSet		20		/* Pascal set		*/
#define dfFLabel	21		/* FORTRAN format label */
#define dfPAC		22		/* Pascal array of char */
#define dfStringFtnSpec 23		/* Spectrum FORTRAN string	*/
#define dfStringModSpec 24		/* Spectrum Modcal string	*/
#define dfPackedDecimal 25		/* Spectrum Packed Decimal	*/
#define dfBool		26		/* Pascal Bool/Fortran Logical	*/
#define dfInst          27		/* Assembly Instruction */
#define dfLongString200	28		/* Series 200 string	*/
#define dfCobstruct     29		/* Cobol Structure      */

typedef int	DFE, *pDFE;

#if (FOCUS || SPECTRUM)
#define	sbFmtDef	"%#10.8lx"	/* don't make bigger than vsbFmt[] */
#else
#define	sbFmtDef	"%#lx"
#endif


/***********************************************************************
 * DISPLAY MODE:
 *
 * This is used for passing around info.
 */

typedef struct MODES {
	    DFE		df;	/* display format			*/
	    int		len;	/* length of item, if meaningful	*/
	    int		cnt;	/* number of times to apply the format	*/
	    int		imap;	/* selects map set to use (normally 0)	*/
	} MODER, *pMODER;

#define cbMODER (sizeof (MODER))
#define modeNil ((pMODER) 0)


/***********************************************************************
 * FORMAT CONTROL for PrintPos():
 */

#define fmtNil	 0	 /* just do side effects, no printing	 */
#define fmtSave	 0x1	 /* do NOT reset vifd, vipd, viln, vslop */
#define fmtFile	 0x2	 /* print the file name			 */
#define fmtProc	 0x4	 /* print the procedure name		 */
#define fmtAsm   0x8     /* print the machine instruction        */


/***********************************************************************
 * COREFILE MAP:
 */

typedef struct MAPS {
	    long	b1;	/* primary bases */
	    long	e1;
	    long	f1;
	    long	b2;	/* alternate bases */
	    long	e2;
	    long	f2;
	    int		fn;	/* file number that goes with this map */
	} MAPR, *pMAPR;

#define fnNil (-1)


/***********************************************************************
 * COMMAND TYPES:
 *
 * These are used for repeating previous commands.
 */

#define cmdNil		0
#define cmdPrint	1	/* print next source line		*/
#define cmdPrintBack	2	/* print previous source line		*/
#define cmdWindow	3	/* print next window of source lines	*/
#define cmdWindowBack	4	/* print previous window of source	*/
#define cmdDisplay	5	/* display the contents of a variable	*/
#define cmdUpArrow	6	/* display contents of previous loc	*/
#define cmdLineSingle	7	/* single step, follow proc calls	*/
#define cmdProcSingle	8	/* single step, skip proc calls		*/
#define cmdScrollDown	9	/* print next window of source lines	*/
#define cmdScrollUp    10	/* print previous window of source	*/
#define cmdJump        11	/* last cmd was jump to a line number   */
#define cmdSplit       12	/* last cmd was toggle split windows    */
#define cmdInstSingle  13       /* instruction step, follow proc calls  */
#define cmdPInstSingle 14       /* instruction step, skip proc calls    */

typedef int CMDE, *pCMDE;


/***********************************************************************
 * SIGNAL ACTION RECORD:
 *
 * Fields are defined so that all false is a good default.
 */

typedef struct {
	    bits	fNoStop	  : 1;	/* do not stop on signal?	  */
	    bits	fNoReport : 1;	/* do not report non-stop signal? */
	    bits	fIgnore	  : 1;	/* ignore non-stop signal?	  */
	} SAR, *pSAR;

#define cbSAR	(sizeof (SAR))
#define saNil	((pSAR) 0)


/***********************************************************************
 * ADDRESS SPACES:
 */

#define spaceText 1		/* for looking at child process */
#define spaceData 2

#define ptChild		0	/* for ptrace() */
#define ptReadI		1
#define ptReadD		2
#define ptReadUser	3
#define ptWriteI	4
#define ptWriteD	5
#define ptWriteUser	6
#define ptResume	7
#define ptTerm		8
#define ptSingle	9

#ifdef SPECTRUM

#define ptReadKernalStack  10		/* push data on child's stack	   */
#define ptWriteKernalStack 11		/* pop data from child's stack	   */

/* the following are NEW ptrace requests add for multiprocess work */
#define ptPpid		12	/* give us pid of parent of this process */
#define ptCmdName	13	/* return name of executable */
#define ptReleaseChild	13	/* go `way ya bother me, Kid.... */
#define ptMultiChild	12	/* we own you and your firstborn */

#endif

#ifdef FOCUS

#define ptPushStack	10		/* push data on child's stack	   */
#define ptPopStack	11		/* pop data from child's stack	   */
#define ptCallProc	12		/* call procedure on child process */

					/* used for calls to AdrFAdr()	    */
#define ptrCall		0		/* EPP used for procedure call only */
#define ptrCode		1		/* EDSP to code only, never heap    */
#define ptrData		2		/* EDSP, DB-rel, or SB-rel pointer  */

typedef int	PTRE;

#endif /* FOCUS */

/***********************************************************************
 * SCREEN MANAGEMENT
 */

#define	SOURCE		1
#define	ASSEMBLY	2
#define	SPLIT		3

#define	SetGeneralRegs	0
#ifdef SPECTRUM
#define	SetFloatingRegs	1
#define	SetSpecialRegs	2
#else
#define	Set98635Regs	1
#define	Set68881Regs	2
#define	Set98248ARegs	3
#endif

/***********************************************************************
 * OTHER STUFF:
 */

#define CMDBUFSIZ     10000    /* real big due to long file names */
#ifdef HPE
typedef int	PLABEL;		/* an xsrt function pointer (32 bits)    */
#endif /* HPE */

#define cbVarMax cbTokMax
#define chNull '\0'
#define pidNil 0

#include "ext.h"			/* now all "exported" declarations */

/*
 * These were from "#include <fcntl.h>" on a System III:
 */
#define O_RDONLY	0
#define O_WRONLY	1
#define O_RDWR		2
