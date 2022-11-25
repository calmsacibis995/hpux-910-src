/* @(#) $Revision: 70.8 $ */   
/* lerror.c
 *	This file was added to support lint message buffering.  Message
 *	buffering works in the following fashion:
 *
 *	A message is printed out via lwerror (warning error) or luerror
 *	(exceptions: lint errors, output via lerror, and compiler
 *	errors, output via cerror).  lwerror and luerror check the
 *	type of the message, and determine the appropriate routine to call.
 *	There are three message "types" :  the message is to be buffered,
 *	the message is not to be buffered, or the message is from a header file.
 *
 *	All (non-header file) buffered messages go into the ctmpfile.  This file
 *	is "dumped" before lint1 completes.  Non-header unbuffered messages
 *	are printed immediately.  Header file messages are always buffered
 *	in the htmpfile.  The rationale is to complain about header files
 *	only once.  This means that the htmpfile is saved between calls
 *	to lint1.  The second pass, lint2, is responsible for dumping these
 *	messages.
 *
 *	Functions:
 *	==========
 *		bufhdr - buffer a header file message
 *		bufsource - buffer a source file message
 *		catchsig - set signals so they are caught
 *		hdrclose - close header file
 *		iscfile - checks to see if file is source or header
 *		lerror - lint error message routine
 *		luerror - lint uerror message
 *		lwerror - lint werror message (warning)
 *		onintr - handle interrupts
 *		tmpopen - open temp files and set up signal processing
 *		unbuffer - dump the ctmpfile buffered messages
 */

# include	<stdio.h>
# include	<signal.h>
# include 	<string.h>
# include	<malloc.h>
# include 	<sys/types.h>
# include	"messages.h"
# include	"lerror.h"
# include 	"manifest"

extern void	exit();

extern int	lineno;
extern char	ftitle[ ];
extern short nerrors;

/* iscfile
 *  compares name with sourcename (file name from command line)
 *  if it is the same then
 *    if fileflag is false then print the source file name as a title
 *    return true
 *  otherwise return false
 */

int fileflag = FALSE;

enum boolean
iscfile( name ) char *name;
{
	extern char	sourcename[ ];

#ifdef LINTHACK
	return (TRUE);
#else
	if ( !strcmp( name, sourcename ) ) {
		if ( fileflag == FALSE ) {
			fileflag = TRUE;
			(void)fprntf( stderr, "\n%s\n==============\n", name );
		}
		return( TRUE );
	}
	return( FALSE );
#endif /* LINTHACK */
}
/* onintr - cleans up after an interrupt  */
void
onintr( )
{
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGPIPE, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);

	(void)putc( '\n', stderr );
	lerror( "", HCLOSE | FATAL );
	/* note that no error message is printed */
}


/* catchsig - set up traps to field interrupts
 *	the onintr routine is the trap handler
 */
#ifdef BBA_COMPILE
#pragma BBA_IGNORE
#endif
catchsig( )
{
	if ((signal(SIGINT, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGINT, onintr);
	if ((signal(SIGHUP, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGHUP, onintr);
	if ((signal(SIGQUIT, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGQUIT, onintr);
	if ((signal(SIGPIPE, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGPIPE, onintr);
	if ((signal(SIGTERM, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGTERM, onintr);
}
/* tmpopen - open temp files etc.
 *  open source message buffer file for writing
 *  open header message file for updating
 *    if open fails, open it for writing
 *  otherwise
 *    initialize header file name and count list from header message file
 *
 *  if opens succeed return success
 *  otherwise return failure
 */

char		*htmpname = NULL;
static FILE	*htmpfile = NULL;

CRECORD * msghead[NUMBUF];	/* array of ptrs to msg bufs */
int	msgmax[NUMBUF];		/* available size of each msg buf */
int	msgtotals[ NUMBUF ];	/* current size of each buf */

tmpopen( )
{
int i;

	catchsig( );

	/* Open what used to be ctmpfile - now an array of arrays in memory */
	for (i=0; i<NUMBUF; i++)
		{
		msgtotals[i]=0;
		msgmax[i] = INIT_MSGS;
		if ((msghead[i] = (CRECORD *)malloc(INIT_MSGS*CRECSZ)) == NULL)
#ifdef BBA_COMPILE
#			pragma BBA_IGNORE
#endif
			lerror( "out of memory (message buff)", FATAL|ERRMSG );
		}

	if ( (htmpfile = fopen( htmpname, "a" )) == NULL ) {
		lerror( "cannot open header message buffer file",
			  FATAL | ERRMSG );
	}
}
/* hdrclose - write header file name/count to header message buffer file
 *	then close the file
 */
hdrclose( )
{
	(void)fclose( htmpfile );
}
/* lerror - lint error message routine
 *  if code is [CH]CLOSE error close and unlink appropriate files
 *  if code is FATAL exit
 */
#ifdef BBA_COMPILE
#pragma BBA_IGNORE
#endif
lerror( message, code ) char *message; int code;
{
	if ( code & ERRMSG ) 
		(void)fprntf( stderr, "lint error: %s\n", message );
	if ( code & HCLOSE ) 
		if ( htmpfile != NULL ) {
			(void)fclose( htmpfile );
			(void)unlink( htmpname );
		}
	if ( code & FATAL ) 
		exit( FATAL );
}
/* lwerror - lint warning error message
 *	if the message is to be buffered, call the appropriate routine:
 *    bufhdr( ) for a header file
 *    bufsource( ) for a source file
 *
 *  if not, call werror( )
 */

/* VARARGS1 */
lwerror( msgndx, arg1, arg2, arg3 ) int	msgndx;
{
	extern char		*strip( );
	extern enum boolean	iscfile( );
	extern char		*msgtext[ ];
	extern short		msgbuf[ ];
	char		*filename;

	if ( !(msginfo[msgndx].msgclass & warnmask) ) return;

	filename = strip( ftitle );

#ifdef LINTHACK
	return(0);
#else
	if ( iscfile( filename ) == TRUE ) 
		if ( msginfo[ msgndx ].msgbuf == 0 ) 
			werror( msgtext[ msgndx ], arg1, arg2, arg3 );
		else 
			bufsource( WERRTY, msgndx, arg1, arg2, arg3 );
	else
		bufhdr( WERRTY, filename, msgndx, arg1, arg2, arg3 );
#endif /* LINTHACK */
}
/* luerror - lint error message
 *	if the message is to be buffered, call the appropriate routine:
 *    bufhdr( ) for a header file
 *    bufsource( ) for a source file
 *
 *  if not, call uerror( )
 */
/* VARARGS1 */
luerror( msgndx, arg1, arg2, arg3 )	short msgndx;
{
	extern char	*strip( );
	extern char	*msgtext[ ];
	extern short	msgbuf[ ];
	char		*filename;
#ifdef LINTHACK
	extern char	sourcename[ ];
#endif /* LINTHACK */

	if ( !(msginfo[msgndx].msgclass & warnmask) ) return;

	filename = strip( ftitle );

	nerrors++;   /* suppress subsequent cerrors even for buffered errs */

#ifdef LINTHACK
        if (nerrors == 1)
                fprntf (stderr, "Invalid Domain/C extensions found:\n\n");
#endif /* LINTHACK */

	if ( iscfile( filename ) == true ) 
#ifdef LINTHACK
		if ( msginfo[msgndx].msgbuf == 0 ) {
			if (filename && *filename)
				fprintf (stderr, "%s: ", filename);
			else
				fprintf (stderr, "%s: ", sourcename);
			uerror( msgtext[ msgndx ], arg1, arg2, arg3 );
		}
#else /* LINTHACK */
		if ( msginfo[msgndx].msgbuf == 0 ) 
			uerror( msgtext[ msgndx ], arg1, arg2, arg3 );
		else 
			bufsource( UERRTY, msgndx, arg1, arg2, arg3 );
#endif /* LINTHACK */
	else 
		bufhdr( UERRTY, filename, msgndx, arg1, arg2, arg3 );

	if( nerrors > MAXUSEFULERRS ) 
		lerror( "too many errors", ERRMSG|FATAL|HCLOSE);
}
/* bufsource - buffer a message for the source file */
/*
* With FLEXNAMES, keep the actual pointer to the name strings in
* ctmpfile, then when the file's contents are examined later, the
* strings will still be in core, so the pointers will still be ok.
*/


/* VARARGS2 */
bufsource( code, msgndx, arg1, arg2, arg3 ) int code, msgndx;
{
	int		bufndx;
	extern short msgbuf[ ], msgtype[ ];
	CRECORD	record;
	int mtype = msginfo[msgndx].msgargs;

	bufndx = msginfo[msgndx].msgbuf;
	if (  bufndx == 0  ||  bufndx >= NUMBUF )
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		lerror( "message buffering scheme flakey",
		  HCLOSE | FATAL | ERRMSG );
	else 
		{
			if ( msgtotals[ bufndx ] >= msgmax[bufndx] ) {
			    msgmax[bufndx] += ADDL_MSGS;
			    msghead[bufndx] = 
				(CRECORD *)realloc((void *)msghead[bufndx], 
					(unsigned)CRECSZ*msgmax[bufndx]);
			}
			record.code = code | mtype;
			record.lineno = lineno;

			switch( mtype & ARG1TY ) {

			case STRINGTY:
				record.arg1.name1 = (char *) arg1;
				break;

			case CHARTY:
				record.arg1.char1 = (char) arg1;
				break;

			case NUMTY:
				record.arg1.number = (int) arg1;
				break;

			default:
				break;
			}

			/* Handle second arg, if any */
			switch ( mtype & ARG2TY ) {

			case STR2TY:
				record.arg2.name2 = (char *) arg2;
				break;

			case NUM2TY:
#ifdef BBA_COMPILE
#				pragma BBA_IGNORE
#endif
				record.arg2.num2 = (int) arg2;
				break;

			case CHAR2TY:
				record.arg2.char2 = (char) arg2;
				break;

			default: break;
			}

			/* Handle TRIPLESTR */
			if (mtype & STR3TY)
				record.arg3.name3 = (char *) arg3;


			msghead[ bufndx ][ msgtotals[bufndx]++ ] = record;

		}
}

/* write error to htmpfile - print a message */
void
hdrwerror()
{
#ifndef OSF
	lerror( "cannot write to header message buffer file",
	  HCLOSE | FATAL | ERRMSG );
#endif
}

/* bufhdr - buffer a message for a header file */
/*
* With FLEXNAMES, since htmpfile lives until lint2 walks over it, the
* name strings are dumped like they are to the normal output - as a null
* terminated string after the record which would normally contain the
* name(s).
*/

static char curhdr[BUFSIZ];

/* VARARGS3 */
bufhdr( code, filename, msgndx, arg1, arg2, arg3 )
  int code; char *filename; int	msgndx;
{
	extern short            msgtype[ ];
	extern char			sourcename[ ];

	HRECORD	record;
	int mtype = msginfo[msgndx].msgargs;

	if ( strcmp( curhdr, filename ) != 0 ) 
	    {
		/* that is, if this is a new (or first) header file
		 * write a new filename marker.  Duplicates will be
		 * eliminated in lint2's unbuffer
		 */

		(void)strcpy(curhdr, filename);
		record.msgndx = HDRFILE;
		record.code = 0;
		record.lineno = 0;
		if (fwrite( (void *)&record, (size_t)HRECSZ, 1, htmpfile) != 1)
#ifdef BBA_COMPILE
#			pragma BBA_IGNORE
#endif
			hdrwerror();
		if (fwrite( (void *)curhdr, strlen(curhdr)+1, 1, htmpfile) != 1)
#ifdef BBA_COMPILE
#			pragma BBA_IGNORE
#endif
			hdrwerror();
		if (fwrite( (void *)sourcename, strlen(sourcename)+1, 1, 
								htmpfile) != 1)
#ifdef BBA_COMPILE
#			pragma BBA_IGNORE
#endif
			hdrwerror();

	    }
	

	/* curhdr contains current header file name */
	record.msgndx = msgndx;
	record.code = code | mtype;
	record.lineno = lineno;

	switch( mtype & ARG1TY ) {

	case CHARTY:
		record.arg1.char1 = (char) arg1;
		break;

	case NUMTY:
		record.arg1.number = (int) arg1;
		break;

	default:
		break;

	}

	/* Handle second arg, if any */
	switch ( mtype & ARG2TY ) {

	case STR2TY:
		break;

	case CHAR2TY:
		record.arg2.char2 = (char) arg2;
		break;

	case NUM2TY:
		record.arg2.num2 = (int) arg2;
		break;

	default: break;
	}
	

	if ( fwrite( (void *) &record, (size_t)HRECSZ, 1, htmpfile ) != 1 ) 
#ifdef BBA_COMPILE
#		pragma BBA_IGNORE
#endif
		hdrwerror();
	if (mtype & STRINGTY)
	{
		if ( fwrite( (void *)arg1, strlen((char *)arg1)+1, 1, htmpfile)
			!= 1 )
		{
#ifdef BBA_COMPILE
#			pragma BBA_IGNORE
#endif
			hdrwerror();
		}
	}

	if (mtype & STR2TY)
	{
		if ( fwrite( (void *)arg2, strlen((char *)arg2)+1, 1, htmpfile)
			!= 1 )
		{
#ifdef BBA_COMPILE
#			pragma BBA_IGNORE
#endif
			hdrwerror();
		}
	}

	if (mtype & STR3TY)
	{
		if ( fwrite( (void *)arg3, strlen((char *)arg3)+1, 1, htmpfile)
			!= 1 )
		{
#ifdef BBA_COMPILE
#			pragma BBA_IGNORE
#endif
			hdrwerror();
		}
	}
}
/* unbuffer - write out information saved in ctmpfile */
unbuffer( )
{
	extern char *outmsg[ ], *outformat[ ];
	int	i, j, stop;
	int	perline, toggle;
	enum boolean	codeflag;
	CRECORD	record;

	/* loop for each message type - outer loop */
	for ( i = 1; i < NUMBUF; ++i ) 
		if ( msgtotals[ i ] != 0 ) {
			codeflag = false;

			stop = msgtotals[ i ];

			/* loop for each occurrence of message - inner loop */
			for ( j = 0; j < stop; ++j ) {

				record = msghead[i][j] ;
				if ( codeflag == false ) {
					if ( record.code & WERRTY ) (void)fprntf( stderr, "warning: " );
					perline = 1;
					toggle = 0;
					if ( record.code & SIMPL ) perline = 2;
					else
						if ( !( record.code & ~WERRTY ) ) 
							/* PLAINTY */
							perline = 3;
					(void)fprntf( stderr, "%s\n", outmsg[ i ] );
					codeflag = true;
				}
				(void)fprntf( stderr, "    (%d)  ", record.lineno );
				switch( record.code & ~( WERRTY | SIMPL ) ) {

				case TRIPLESTR:
					(void)fprntf( stderr, outformat[ i ], record.arg1.name1,
					    record.arg2.name2, record.arg3.name3 );
					break;

				case DBLSTRTY:
					(void)fprntf( stderr, outformat[ i ], record.arg1.name1,
					    record.arg2.name2 );
					break;

				case STRINGTY:
					(void)fprntf( stderr, outformat[ i ], record.arg1.name1 );
					break;

				case CHARTY:
					(void)fprntf( stderr, outformat[ i ], record.arg1.char1 );
					break;

				case NUMTY:
					(void)fprntf( stderr, outformat[ i ], record.arg1.number );
					break;

				case NUMTY|STR2TY:
				case CHARTY|STR2TY:
					(void)fprntf( stderr, outformat[i], record.arg1.number, record.arg2.name2 );
					break;

				case NUMTY|HEX2TY:
				case CHARTY|HEX2TY:
				case CHARTY|CHAR2TY:
				case NUMTY|CHAR2TY:
					(void)fprntf( stderr, outformat[i], record.arg1.number, record.arg2.num2 );
					break;

				default:
					(void)fprntf( stderr, outformat[ i ] );
					break;

				}
				if ( ++toggle == perline ) {
					(void)fprntf( stderr, "\n" );
					toggle = 0;
				}
				else (void)fprntf( stderr, "\t" );
			} /* end, inner for loop */

			if ( toggle != 0 ) (void)fprntf( stderr, "\n" );
	} /* end, outer for loop */

}

#ifdef APEX
extern int all_ext_flag;
extern int ignore_extensions;
/* issue a warning about Domain reference parameters */
/* The default is to warn about reference parameters only in the main file,
 * (not included files) because of the numerous occurences in Domain system
 * headers.  It is also possible to request all or no warnings.
 */
warn_ref_parm()
{

      if (ignore_extensions)
              return;
      if (all_ext_flag)
              WERROR( MESSAGE( 243 ) );       /* warn unconditionally */
      else if (iscfile(strip(ftitle)))
              WERROR( MESSAGE( 243 ) );       /* warn only if in sourcefile */

}
#endif
