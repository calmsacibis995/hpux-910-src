/* @(#) $Revision: 72.2 $ */    
#include "curses.ext"
#include "../local/uparm.h"

#ifdef hpe
#include <ctype.h>
#endif hpe

extern	struct	term _first_term;
extern	struct	term *cur_term;

static char firststrtab[2048];
static int called_before = 0;	/* To check for first time. */
char *getenv();
char ttytype[512];	/* just the first 511 bytes of the name field */
#ifndef termpath
#define termpath(file) "/usr/lib/terminfo/file"
#endif
#define MAGNUM 0432

#define getshi()	getsh(ip) ; ip += 2

/*
 * "function" to get a short from a pointer.  The short is in a standard
 * format: two bytes, the first is the low order byte, the second is
 * the high order byte (base 256).  The only negative number allowed is
 * -1, which is represented as 255, 255.  This format happens to be the
 * same as the hardware on the pdp-11 and vax, making it fast and
 * convenient and small to do this on a pdp-11.
 */

#ifdef vax
#define getsh(ip)	(* (short *) ip)
#endif
#ifdef pdp11
#define getsh(ip)	(* (short *) ip)
#endif

#ifndef getsh
/*
 * Here is a more portable version, which does not assume byte ordering
 * in shorts, sign extension, etc.
 */
getsh(p)
register char *p;
{
	register int rv, sv;
	rv = *((unsigned char *) p++);
	sv = *((unsigned char *) p);
	if (rv == 0377)
		if (sv == 0377)
			return -1;
	return rv + (sv * 256);
}
#endif

/*
 * setupterm: low level routine to dig up terminfo from database
 * and read it in.  Parms are terminal type (0 means use getenv("TERM"),
 * file descriptor all output will go to (for ioctls), and a pointer
 * to an int into which the error return code goes (0 means to bomb
 * out with an error message if there's an error).  Thus, setupterm(0, 1, 0)
 * is a reasonable way for a simple program to set up.
 */
setupterm(term, filenum, errret)
char *term;
int filenum;	/* This is a UNIX file descriptor, not a stdio ptr. */
int *errret;
{
	char tiebuf[4096];
	char fname[512];
	register char *ip;
	register char *cp;
	int n, tfd;
	char *lcp, *ccp;
	int snames, nbools, nints, nstrs, sstrtab;
	char *strtab;
	int no_unknown = 0;
	char *UNKNOWN="unknown";

	if (term == NULL)
		term = getenv("TERM");
	if (term == NULL || *term == '\0')
		term = "unknown";
	tfd = -1;
	if (cp=getenv("TERMINFO")) {
		strcpy(fname, cp);
		cp = fname + strlen(fname);
		*cp++ = '/';
		*cp++ = *term;
		*cp++ = '/';
		strcpy(cp, term);
		tfd = open(fname, 0);
	}
	if (tfd < 0) {
#ifdef hpe
		if (isdigit(term[0]))
			strcpy(fname,"hp");
		strcat(fname,term);
#else
		strcpy(fname, termpath(a/));
		cp = fname + strlen(fname);
		cp[-2] = *term;
		strcpy(cp, term);
#endif hpe
		tfd = open(fname, 0);
	}

#ifdef hpe
	if (tfd < 0) {
		char *p;
		if (!strcmp(fname,"unknown")) {
			p = "I don't know what kind of terminal you are on.\n";
			write(2,p,strlen(p));
			p="Please enter command \":SETVAR TERM <term type>\"\n";
			write(2,p,strlen(p));
		}
		else {
			p = "No term info file for terminal type: ";
			write(2,p,strlen(p));
			write(2,term,strlen(fname));
			write(2,"\n",1);
		}
		exit(1);
	}
#else
	if( tfd < 0 )
	{
	    if (!strcmp(term,UNKNOWN)) {
		    no_unknown++;
	    } else {
		if( access( termpath(.), 0 ) )
		{
			if( errret == 0 )
				perror( termpath(.) );
			else
				*errret = -1;
		}
		else
		{
			if( errret == 0 )
			{
				write(2, "No such terminal: ", 18);
				write(2, term, strlen(term));
				write(2, "\r\n", 2);
			}
			else
			{
				*errret = 0;
			}
		}
		if( errret == 0 )
			exit( -2 );
		else
			return -1;
	    }
	}
#endif hpe

	if( called_before && cur_term )		/* 2nd or more times through */
	{
		cur_term = (struct term *) calloc(1,sizeof (struct term));
		if (cur_term == (struct term *)0) {
		    if( errret == 0 ) {
			perror("calloc");
			exit(2);
		    } else {
			*errret = 0;
			return -1;
		    }
		}
		strtab = NULL;
	}
	else					/* First time through */
	{
		cur_term = &_first_term;
		called_before = TRUE;
		strtab = firststrtab;
	}

	if( filenum == 1 && !isatty(filenum) )	/* Allow output redirect */
	{
		filenum = 2;
	}
	cur_term -> Filedes = filenum;
	def_shell_mode();

	if (errret)
		*errret = 1;
	if (no_unknown) {
		bell="\007";
		carriage_return="\015";
		cursor_down="\012";
		scroll_forward="\012";
		columns=80;
		auto_right_margin=1;
		generic_type=1;
		strcpy(ttytype,UNKNOWN);
		goto out;
	}
	n = read(tfd, tiebuf, sizeof tiebuf);
	close(tfd);
	if (n <= 0) {
corrupt:
		write(2, "corrupted term entry\r\n", 22);
		if (errret == 0)
			exit(-3);
		else
			return -1;
	}
	if (n == sizeof tiebuf) {
		write(2, "term entry too long\r\n", 21);
		if (errret == 0)
			exit(-4);
		else
			return -1;
	}
	ip = tiebuf;

	/* Pick up header */
	snames = getshi();
	if (snames != MAGNUM) {
		goto corrupt;
	}
	snames = getshi();
	nbools = getshi();
	nints = getshi();
	nstrs = getshi();
	sstrtab = getshi();
	if (strtab == NULL) {
		strtab = (char *) calloc(1,sstrtab);
		if (strtab == (char *)0) {
		    if( errret == 0 ) {
			perror("calloc");
			exit(2);
		    } else {
			*errret = 0;
			return -1;
		    }
		}
	}

	/*
	 * Copy the terminal names into the ttytype[] buffer.  If there
	 * are too many names to fit into ttytype[], copy as much as
	 * will fit, and then delete the last partial name.
	 */
	if (snames < sizeof ttytype - 1) {
		/*
		 * The whole thing fits.
		 */
		memcpy(ttytype, ip, snames);
	}
	else {
		/*
		 * Copy as much as will fit into our limited space.
		 */
		memcpy(ttytype, ip, sizeof ttytype);

		/*
		 * Now delete partial name at the end.
		 */
		cp = ttytype + sizeof ttytype - 1;
		while (cp > ttytype && *cp != '|')
		    continue;
		*cp = '\0'; /* stomp on the '|' */
	}
	ip += snames;

	/*
	 * Inner blocks to share this register among two variables.
	 */
	{
		register char *sp;
		char *fp = &cur_term->Xon_xoff;
		register char s;
		for (cp= &cur_term->Auto_left_margin; nbools--; ) {
			s = *ip++;
			if (cp <= fp)
				*cp++ = s;
		}
	}

	/* Force proper alignment */
	if (((unsigned int) ip) & 1)
		ip++;

	{
		register short *sp1, *sp2;
		short *fp1 = &cur_term->Width_status_line;
		short *fp2 = &cur_term->Label_width;
		register int s;

		for(sp1=&cur_term->Columns,sp2=&cur_term->Num_labels;nints--;) {
			s = getshi();
			if (sp1 <= fp1)
				*sp1++ = s;
			else if (sp2 <= fp2)
				*sp2++ = s;
		}
	}

#ifdef TIOCGWINSZ
	/*
	 *  Query the system for the terminal dimensions.
	 *
	 *  This block of code will call ioctl with the TIOCGWINSZ 
	 *  parameter.  This will cause the tty driver to return
	 *  the dimensions of the current display.  
	 * 
	 *  A couple of notes:  filenum will normally be stdout (value of 1)
	 *  however, if stdout has been redirected to a file, then filenum
	 *  will have been set to stderr (value of 2) prior entering this
	 *  section of code.  (Ths determination for fiilenum is in the 
	 *  early parts of the setupterm() function.)
	 *
	 *  If stderr has also been redirected to a file, then the ioctl
	 *  will return a value of -1 indicating an error.  When this 
	 *  happens, the value for lines and columns will be either the
	 *  value predetermined in the terminfo file, or set according to
	 *  environment variable (see following block of code).
	 *
	 *  
	 */
	{
		struct winsize wsz;

		if ( ioctl( filenum , TIOCGWINSZ , &wsz ) >= 0 ) {

		/* It has been demonstrated that older versions of */
		/* hp-ux ioctl calls may returned successfully from */
		/* ioctl (...,TIOCGWINSZ,...), but still hve the    */
		/* values of 0 for row and columns.  If this is     */
		/* the case, then use the default as defined in the */
		/* terminfo file.				    */

		/* If returned value is Not zero then set it accord-*/
		/* ingly.					    */
		   if ( wsz.ws_row != 0 ) lines = wsz.ws_row;
		   if ( wsz.ws_col != 0 ) columns = wsz.ws_col;
		};
        }
#endif

	lcp = getenv("LINES");
	ccp = getenv("COLUMNS");
	if (lcp)
		lines = atoi(lcp);
	if (ccp)
		columns = atoi(ccp);

	{
		register char **pp1;
		register char **pp2;
		char **fp1 = (char **)&cur_term->strs2.Memory_unlock;
		char **fp2 = (char **)&cur_term->strs3.Key_f63;

		for(pp1 = &cur_term->strs.Back_tab,
		    pp2 = &cur_term->strs3.Plab_norm; nstrs--; ) {
			n = getshi();
			if (pp1 <= fp1) {
				if (n == -1)
					*pp1++ = NULL;
				else
					*pp1++ = strtab+n;
			} else if (pp2 <= fp2) {
				if (n == -1)
					*pp2++ = NULL;
				else
					*pp2++ = strtab+n;
			}
		}
	}

	for (cp=strtab; sstrtab--; ) {
		*cp++ = *ip++;
	}

out:
	/*
	 * If tabs are being expanded in software, turn this off
	 * so output won't get messed up.  Also, don't use tab
	 * or backtab, even if the terminal has them, since the
	 * user might not have hardware tabs set right.
	 */
#ifdef USG
	if ((cur_term -> Nttyb.c_oflag & TABDLY) == TAB3) {
		cur_term->Nttyb.c_oflag &= ~TABDLY;
		/*
		tab = NULL;
		back_tab = NULL;
		*/
		reset_prog_mode();
		return 0;
	}
#else
	if ((cur_term -> Nttyb.sg_flags & XTABS) == XTABS) {
		cur_term->Nttyb.sg_flags &= ~XTABS;
		/*
		tab = NULL;
		back_tab = NULL;
		*/
		reset_prog_mode();
		return 0;
	}
#endif
#ifdef DIOCSETT
	reset_prog_mode();
#endif 
#ifdef LTILDE
	ioctl(cur_term -> Filedes, TIOCLGET, &n);
	if (n & LTILDE);
		reset_prog_mode();
#endif
	return 0;
}
