/* @(#) $Revision: 37.2 $ */      
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif

/* EMACS_MODES: c tabstop=4 !fill */

/*
 *	cmrcheck -- Check list in p file to see if this cmr is valid.
 *
 *
 */

#include "../../hdr/filehand.h"
#include <stdio.h>


/* Debugging options. */
#define MAXLENCMR 12
#ifdef TRACE
#define TR(W,X,Y,Z) fprintf (stdout, W, X, Y, Z)
#else
#define TR(W,X,Y,Z) /* W X Y Z */
#endif

#define CMRLIMIT 128					/* Length of cmr string. */

cmrcheck (cmr, appl)
char	*cmr,
		*appl;
{
	char		lcmr[CMRLIMIT],			/* Local copy of CMR list. */
				*p[2],					/* Field to match in .FRED file. */
				*format = "%s is not a valid CMR.";
	extern char *strrchr (), *gf ();	/* Quiet lint. */

	TR("Cmrcheck: cmr=(%s) appl=(%s)\n", cmr, appl, NULL);
	p[1] = EMPTY;
	strcpy (lcmr, cmr);
	while ((p[0] = strrchr (lcmr, ',')) != EMPTY) {
		p[0]++;							/* Skip the ','. */
		if (strlen (p[0]) != MAXLENCMR || sweep (SEQVERIFY, gf (appl), EMPTY,
		  '\n', WHITE, 40, p, EMPTY, (char**) NULL, (int (*)()) NULL,
		  (int (*)()) NULL) != FOUND) {

			fprintf (stdout, (nl_msg(1,format)), p[0]);
			fprintf (stdout, "\n");
			TR("Cmrcheck: return=1\n", NULL, NULL, NULL);
			return (1);
			}
		p[0]--;							/* Go back to comma. */
		*p[0] = NULL;					/* Clobber comma to end string. */
		}
	TR("Cmrcheck: last entry\n", NULL, NULL, NULL);
	p[0] = lcmr;						/* Last entry on the list. */
	if (strlen (p[0]) != MAXLENCMR || sweep (SEQVERIFY, gf (appl), EMPTY, '\n',
	  WHITE, 40, p, EMPTY, (char**) NULL, (int (*)()) NULL, (int (*)()) NULL)
	  != FOUND) {
		fprintf (stdout, (nl_msg(1,format)), p[0]);
		fprintf (stdout, "\n");
		TR("Cmrcheck: return=1\n", NULL, NULL, NULL);
		return (1);
		}
	TR("Cmrcheck: return=0\n", NULL, NULL, NULL);
	return (0);
}
