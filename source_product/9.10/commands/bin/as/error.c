/* @(#) $Revision: 70.1 $ */      

# include <stdio.h>
# include "pass0.h"

# define MAXERR	30
extern long line;
extern char * filenames[];
extern short wflag;
extern short rflag;

short nerrors = 0;
short nerrors_this_stmt = 0;


/*******************************************************************************
 * aerror
 *	Handle fatal assembler errors.  Print message and terminate process.
 */
/* VARARGS1 */
aerror(s,a,b,c)
 char *s;
{ fprintf(stderr, "as: Fatal error: \"%s\" line %3d: ", filenames[0], line);
  fprintf(stderr, s, a, b, c);
  fprintf(stderr, "\n");
  unlink_tfiles();
  unlink_ofile();
  if (rflag) unlink(filenames[0]);
  EXIT(1);
}

/*******************************************************************************
 * uerror
 *	Handle normal errors.  Print message
 */
/* VARARGS1 */
uerror(s,a,b,c)
 char *s;
{ fprintf(stderr, "as error: \"%s\" line %3d: ", filenames[0], line);
  fprintf(stderr, s, a, b, c);
  fprintf(stderr, "\n");
  nerrors++;
  nerrors_this_stmt++;
  if (nerrors > MAXERR)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("too many errors");
}

/*******************************************************************************
 * werror
 *	Handle assembler warnings.  Print message.
 *	-w option causes warnings to be suppressed.
 */
/* VARARGS1 */
werror(s,a,b,c)
 char *s;
{ if (wflag) return;
  fprintf(stderr, "as: warning: \"%s\" line %3d: ", filenames[0], line);
  fprintf(stderr, s, a, b, c);
  fprintf(stderr, "\n");
}

#if BFA
extern char bfastring[];

bfa_exit(n)
{  bfareport(bfastring);
   exit(n);
}
# endif
