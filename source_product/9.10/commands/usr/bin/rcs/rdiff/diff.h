/*
 * $Header: diff.h,v 66.1 90/02/08 13:55:30 smp Exp $
 */

/*
 * diff - common declarations
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "system.h"
#include "bin.h"

/*
 * Output format options
 */
int	opt;

#define	D_NORMAL	0	/* Normal output */
#define	D_EDIT		-1	/* Editor script out */
#define	D_REVERSE	1	/* Reverse editor script */
#define	D_CONTEXT	2	/* Diff with context */
#define	D_IFDEF		3	/* Diff with merged #ifdef's */
#define D_NREVERSE      4       /* Reverse editor script, no trailing '.'*/

/*
 * Algorithm related options
 */
int	hflag;			/* -h, use halfhearted DIFFH */
int	bflag;			/* ignore blanks in comparisions */

/*
 * Options on hierarchical diffs.
 */
int	lflag;			/* long output format with header */
int	rflag;			/* recursively trace directories */
int	sflag;			/* announce files which are same */
char	*start;			/* do file only if name >= this */

/*
 * Variables for -I D_IFDEF option.
 */
int	wantelses;		/* -E */
char	*ifdef1;		/* String for -1 */
char	*ifdef2;		/* String for -2 */
char	*endifname;		/* What we will print on next #endif */
int	inifdef;

/*
 * Variables for -c context option.
 */
int	context;		/* lines of context to be printed */

/*
 * State for exit status.
 */
int	status;
int	anychange;
char	*tempfile;		/* used when comparing against std input */

/*
 * Variables for diffreg.
 */
char	**diffargv;		/* option list to pass to recursive diffs */

/*
 * Input file names.
 */
char	*file1, *file2;
struct	stat stb1, stb2;

char	*malloc(), *talloc(), *ralloc();
char	*savestr(), *splice(), *splicen();
char	*mktemp(), *copytemp(), *rindex();
int	done();

extern	char diffh[], diff[], pr[];
