/* @(#) $Revision: 72.2 $ */      
# include	"curses.ext"
# include	<signal.h>
# include	<nl_types.h>
# include	<langinfo.h>

/*----------------------------------------------------------------------*/
/*									*/
/* The following ifdef is a method of determining whether or not we are */
/* on a 9.0 system or 8.xx system.					*/
/* The specific problem is that if we are using a 8.07 os then SIGWINCH */
/* is defined in signal.h however, curses does not yet support this     */
/* feature; thus the header /usr/include/curses.h does not check and    */
/* create/not create the SIGWINCH entries into the window structure.    */
/* The method that we will use to determine if this is a 9.0 os, which  */
/* is a valid system for sigwinch is to check the SIGWINDOW flag.  If   */
/* This flag is set, then it's 9.0 otherwise, it is a os prior to 9.0   */
/*  									*/
/*  Assumptions:  							*/
/*     		OS		SIGWINCH	SIGWINDOW		*/
/*		-----------------------------------------		*/
/*		9.0 +		True		True			*/
/*		8.07		True		Not Set			*/
/*		Pre-8.07	Not Set		Not Set			*/
/*									*/
/* This should probably be removed after support for 8.xx is dropped	*/
/*									*/
/*	Note: SIGWINCH should be Defined for 9.0 and forward.  If it 	*/
/*	      is defined for Pre-9.0, then it should be undefined.	*/
/*									*/

#ifndef _SIGWINDOW
#  ifdef SIGWINCH
#    undef SIGWINCH
#  endif
#endif
/*									*/
/*		Done with special SIGWINCH definitions.			*/
/*----------------------------------------------------------------------*/

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();
extern	char	*tparm();

struct screen *
newterm(type, outfd, infd)
char *type;
FILE *outfd, *infd;
{
#ifdef	SIGTSTP
	int		_tstp();
# ifdef	hpux
	struct sigvec vec;
# endif	hpux
#endif	SIGTSTP

#ifdef SIGWINCH
	int		_winchTrap();
# ifdef hpux
	struct sigvec wvec;
# endif 
#endif 

	struct screen *scp;
	struct screen *_new_tty();
	extern int _endwin;

	char *tmp_attrib_str;

#ifdef DEBUG
	if(outf) fprintf(outf, "NEWTERM() isatty(2) %d, getenv %s\n",
		isatty(2), getenv("TERM"));
#endif
	SP = (struct screen *) calloc(1, sizeof (struct screen));
	if (SP == (struct screen *)0)
		return (struct screen *)0;
	SP->term_file = outfd;
	SP->input_file = infd;
	/*
	 * The default is echo, for upward compatibility, but we do
	 * all echoing in curses to avoid problems with the tty driver
	 * echoing things during critical sections.
	 */
	SP->fl_echoit = 1;
	savetty();
	scp = _new_tty(type, outfd);
	if (scp == NULL)
		return (struct screen *)NULL;
#ifdef USG
	(cur_term->Nttyb).c_lflag &= ~ECHO;
	(cur_term->Nttyb).c_oflag &= ~OCRNL;
#else
	(cur_term->Nttyb).sg_flags &= ~ECHO;
	(cur_term->Nttyb).sg_oflag &= ~OCRNL;
#endif
	reset_prog_mode();
#ifdef SIGTSTP
# ifndef hpux
	(void) signal(SIGTSTP, _tstp);
# else	hpux
	vec.sv_handler = _tstp;
	vec.sv_mask = vec.sv_onstack = 0;
	sigvector(SIGTSTP, &vec, &vec);
	if (vec.sv_handler == SIG_IGN)
	(void) sigvector(SIGTSTP, &vec, (struct sigvec *)0);
# endif	hpux
#endif

#ifdef SIGWINCH
# ifndef hpux
	(void) signal(SIGWINCH, _winchTrap);
# else 
	(void) sigvector(SIGWINCH, (struct sigvec *)0, &wvec);
	if (wvec.sv_handler == SIG_DFL) {
		wvec.sv_handler = _winchTrap;
		wvec.sv_mask = sigmask(SIGWINCH);
		wvec.sv_onstack = 0;
		(void) sigvector(SIGWINCH, &wvec, (struct sigvec *)0);
	}
# endif 
#endif
	
	if (curscr != NULL) {
#ifdef DEBUG
		if(outf) fprintf(outf, "INITSCR: non null curscr = 0%o\n", curscr);
#endif
	}
#ifdef DEBUG
	if(outf) fprintf(outf, "LINES = %d, COLS = %d\n", LINES, COLS);
#endif
	LINES =	lines;
	COLS =	columns;
	curscr = makenew(LINES, COLS, 0, 0);
	stdscr = newwin(LINES, COLS, 0, 0);
#ifdef SIGWINCH
	_sumscr = newwin(LINES, COLS, 0, 0);
#endif
#ifdef DEBUG
	if(outf) fprintf(outf, "SP %x, stdscr %x, curscr %x\n", SP, stdscr, curscr);
#endif
	SP->std_scr = stdscr;
	SP->cur_scr = curscr;

	/*
	 * Optimization: save set-attribute strings (only for setting of 
	 * single attributes) so don't have to continually recompute them.
	 */
        if (set_attributes) {
		if (make_attrstr()) {		
                        tmp_attrib_str = tparm(set_attributes, 1, 0, 0, 0, 0, 0, 0, 0, 0);
	                strcpy_attrib(&(attrstr[hashattr(A_STANDOUT)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	                strcpy_attrib(&(attrstr[hashattr(A_UNDERLINE)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 1, 0, 0, 0, 0, 0, 0);
       		        strcpy_attrib(&(attrstr[hashattr(A_REVERSE)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 1, 0, 0, 0, 0, 0);
                	strcpy_attrib(&(attrstr[hashattr(A_BLINK)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 1, 0, 0, 0, 0);
                	strcpy_attrib(&(attrstr[hashattr(A_DIM)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 0, 1, 0, 0, 0);
	                strcpy_attrib(&(attrstr[hashattr(A_BOLD)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 0, 0, 1, 0, 0);
       		        strcpy_attrib(&(attrstr[hashattr(A_INVIS)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 0, 0, 0, 1, 0);
               		strcpy_attrib(&(attrstr[hashattr(A_PROTECT)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 0, 0, 0, 0, 1);
	                strcpy_attrib(&(attrstr[hashattr(A_ALTCHARSET)]), tmp_attrib_str);
		}
        }

	/* Maybe should use makewin and glue _y's to DesiredScreen. */
	_endwin = FALSE;
	return scp;
}


make_attrstr()
{
	int i,j;

	if (attrstr) return 1;

	if ((attrstr = (char **) calloc(NHASHKEYS, sizeof(char *))) == NULL)
		return 0;

	for (i = 0; i < NHASHKEYS; i++) {
		if ((attrstr[i] = (char *) calloc(MAX_ATTR_STRLEN, sizeof(char))) == NULL) {
			for (j = 0; j < i; j++) cfree(attrstr[j]);
			cfree((char *) attrstr);
			attrstr = NULL;
			break;
		}
	}

	return attrstr;
}
/*****************************************************************************/
/*      Function:  strcpy_attrib                                             */
/*                                                                           */
/*  Description:  Copy sttribute string from surce to target.  If neccesary  */
/*		  reallocate memory for the target space. 		     */
/*				      					     */
/*  Call:	strcpy(target,source)					     */
/*									     */
/*    Where:       							     */
/*                  char **target	Address of the pointer that points   */
/*					to the target string location.	     */
/*									     */
/*		    char *source	Address of the source string.	     */
/*									     */
/*  Method:       This function will copy an attribute string from the       */
/*		  source location to a target location.  The reason we	     */
/*		  don't use a standard strcpy is that some addition          */
/*		  checks are performed to verify that the space that         */
/*		  has been preallocated for the attribute string is          */
/*		  large enough for the final attribute string.  If the       */
/*		  actual length of the set attribute string is longer        */
/*		  than the default length of 'MAX_ATTR_STRLEN' then 	     */
/*		  we increase the space allocated by 'realloc'ing the	     */
/*		  space.  Should an error be returned during the 	     */
/*		  realloc, then, there will not be any pregenerated	     */
/*		  attribute string for the particular 'target'.  The	     */
/*		  target is thereby set to become a 'null pointer' (0).	     */
/*                                                                           */
/*		  The function that reads the attribute string will check    */
/*		  for the 'null pointer' and take the action of generating   */
/*		  the attribute string rather than us a pregenerated string. */
/*                                                                           */
/*****************************************************************************/
strcpy_attrib(target,source)
    char **target, *source;
{

   char *work, *current;

	if ( strlen(source) >= MAX_ATTR_STRLEN )
	{
		/* reallocate the space.  Add 1 to ensure that there is */
		/* space for the Null terminator.		        */
		current = *target;
		work = realloc(*target,strlen(source)+1);
		if (! work )
		{  *target=0;
		   free(current);
                } else 		*target = work;
	}
	/* If target is not a null pointer, then copy the string */
	if ( *target ) 
		strcpy(*target,source);
}

