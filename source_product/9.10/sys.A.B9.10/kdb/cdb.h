/* @(#) $Revision: 66.2 $ */     
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * This file declares types, macros, and structures used widely throughout
 * the debugger.
 */

#include "basic.h"

/*
 * LIBRARY ROUTINE TYPES:
 *
 * Note that malloc() and free() are NOT here; all calls to them
 * must go through Malloc() and Free().
 */

extern long	lseek();
extern char	*strchr();
extern char	*strrchr();
extern char	*strcpy();
extern char	*strncpy();
extern char	*strcat();
extern char	*strncat();
extern int	strlen();
extern int	strspn();
extern int	vpc;		/* defined in Kasm68020.s */

#include "sym.h"
#include "tk.h"
#include "ty.h"

#define cbFDR	(sizeof (FDR))
#define ilnNil	0
#define ifdNil (-1)

#define cbPDR	(sizeof (PDR))
#define ipdNil	(-1)

#define cbMDR	(sizeof (MDR))
#define imdNil	(-1)

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

typedef struct BPS {
	    ADRT	adr;		/* address of breakpoint	 */
	    uint	inst;		/* instruction that lives there	 */
	    int		count;		/* times to hit before recognize */
	    char	*sbBp;		/* command list for bp, if any	 */
	    FLAGT	fTell;		/* don't report, regardless	 */
	} BPR, *pBPR;

#define cbBPR	(sizeof (BPR))

#define	ibpTemp	      0		/* where to save temporary bps for exec	 */
#define ibpNil	    (-1)	/* no breakpoint hit, or no current bp   */
#define ibpFail     (-2)	/* ptrace failed */
#define ibpInternal (-3)	/* internal (count == 0) bp encountered  */
#define ibpContinue (-4)	/* bp encountered but not yet recognized */


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
/*#define dfString500	18		   Series 500 string	*/
#define dfText		19		/* Pascal FILE of TEXT	*/
#define dfSet		20		/* Pascal set		*/
#define dfFLabel	21		/* FORTRAN format label */

#define dfInst	22			/* s200 machine instruction */
#define dfCChar	23			/* compact chars */

typedef int	DFE, *pDFE;

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

#define cmdMachInst	9	/* single step one machine instruction	    */
#define cmdProcInst	10	/* single step jump over procedure calls    */

typedef int CMDE, *pCMDE;


/***********************************************************************
 * ADDRESS SPACES:
 */

#define spaceText 1		/* for looking at child process */
#define spaceData 2

#define accRead		0	/* for Access() */
#define accWrite	1

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



/***********************************************************************
 * OTHER STUFF:
 */

#define chNull '\0'
#define pidNil 0

#include "ext.h"			/* now all "exported" declarations */
