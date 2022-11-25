static char *HPUX_ID = "@(#) $Revision: 64.3 $";
/*	dumpmsg.c	84/12/14			*/

/*
 * This program takes the formatted message catalog file and produces
 * a string message catalog file for translation/modification.
 *
 * formatted catalog < file.cat >
 *
 *	This file is a binary one with overhead, message directories
 *	and messages.  The structure is;
 *
 *		Over head
 *			Catalog I.D.		8 bytes
 *			Number of messages	4 bytes
 *		Directories
 *			set number		2 bytes
 *			message number		2 bytes
 *			message byte address	4 bytes
 *			message byte length	4 bytes
 *		Messages
 *			message without NULL terminator
 *						variable length
 *
 * string catalog < file.msg >
 *
 *	Set numbers will begin at the 1st column, $set ##
 *	Set number will be in ascending order.
 *		1 <= set number <= 255.
 *	Message numbers also begin at the 1st column, #### [message].
 *	Message numbers will be in ascending order in one set.
 *		1 <= message number <= 32766.
 *	Comment lines must begin with `$' at the 1st column,
 *		$ [comment].
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <msgcat.h>

#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <locale.h>
#include <nl_types.h>
nl_catd nl_fn;
#endif NLS

#ifndef NONLS
#include <nl_ctype.h>
#endif NONLS

struct ar {
	DIR		directory;
	struct ar	*next;
	unsigned char	*message;
};
typedef struct ar	AR;

int	errno;			/* External error variable */
int	catfd;			/* Catalog file descriptor */

#ifndef NONLS
int langid;
#endif NONLS

long	nummsgs = 0;		/* Number of message in archive */
long	lseek();
FILE	*fd;			/* Message file descriptor */
AR	*root = NULL,		/* Root pointer to archive */
	*ptr,
	**prevptr;
char	*catfname,		/* Save catalog file name */
	*msgfname,		/* Save message file name */
	*strcpy(),
	*malloc();
nl_catd	catd;

main(argc, argv)
int argc;
char *argv[];
{

#ifndef NONLS
	langid = currlangid();
#endif NONLS
#ifdef NLS16
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale(),stderr);
		nl_fn=(nl_catd)-1;
	}
	else
		nl_fn=catopen("dumpmsg",0);
#endif NLS16

	/* Check the number of arguments */
	if (argc == 1) {
		/* Message catalog file has not been specified. */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,1, "usage : dumpmsg catfile\n")));
		exit(1);
	}

	for(++argv; --argc; ++argv) {
		catfname = *argv;
		/* Open the catalog file for read. */
		if ((catfd = open(*argv, O_RDONLY)) == ERROR) {
			/*
			 * Cannot open the catalog file.
			 */
			Perror(*argv);
			exit(2);
		}
		else {
	
#ifdef TRACE
			fprintf(stderr, (catgets(nl_fn,NL_SETN,2, "Open catalog file : %s\n")), catfname);
#endif TRACE
			/* Read the contents and add it to archive */
			read_catfile();
	
			/* Close the catalog file */
			if (close(catfd) == ERROR) {
				/*
				 * Error occured in closing the catalog file.
				 */
				Perror(catfname);
				exit(3);
			}
	
#ifdef TRACE
			fprintf(stderr, (catgets(nl_fn,NL_SETN,3, "Close catalog file : %s\n")), catfname);
#endif TRACE
	
		}
	
		/* Write out the messages to std out */
		write_msg();
	
	}
	/* Successful completion */
	exit (0);
}
	
int
read_catfile()
{
	int	len;
	char	buf[OVERHEAD];
	long	i;
		
	/*
	 * Read the header of catalog file
	 */
	if ((len = read(catfd, buf, OVERHEAD)) == ERROR) {
		Perror(catfname);
		exit(10);
	}
	else if (len == 0) {
		/*
		 * End of file.
		 * This catalog file must be new.
		 */

#ifdef TRACE
		fprintf(stderr, (catgets(nl_fn,NL_SETN,4, "No messages in catalog file\n")));
#endif TRACE

		return;
	}

	/*
	 * Check the catalog file ID.
	 */
	if (strncmp(buf, CATID, IDLEN)) {
		/* Bad catalog file ID. */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,5, "dumpmsg: %s: Not a catalog file\n")), catfname);
		exit(11);
	}

	/*
	 * Get the number of messages in the file.
	 */
	nummsgs = *(long *) &buf[NUM_MSG_POS];

#ifdef TRACE
	fprintf(stderr, (catgets(nl_fn,NL_SETN,6, "%ld messages in catalog file\n")), nummsgs);
#endif TRACE

	/*
	 * Read directories into archive
	 */
	prevptr = &root;
	for (i = 0; i < nummsgs; i++) {
		*prevptr = (AR *) malloc((unsigned)sizeof(AR));	
		/* Read one directory */
		if (read(catfd, (char *)*prevptr, DIRSIZE) != DIRSIZE) {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,7, "dumpmsg: %s: Bad catalog file\n")),
				catfname);
			exit(12);
		}

#ifdef TRACE
		fprintf(stderr, (catgets(nl_fn,NL_SETN,8, "setnum : %d\n")), (*prevptr)->directory.setnum);
		fprintf(stderr, (catgets(nl_fn,NL_SETN,9, "msgnum : %d\n")), (*prevptr)->directory.msgnum);
		fprintf(stderr, (catgets(nl_fn,NL_SETN,10, "length : %d\n")), (*prevptr)->directory.length);
		fprintf(stderr, (catgets(nl_fn,NL_SETN,11, "addr   : %d\n\n")), (*prevptr)->directory.addr);
#endif TRACE

		prevptr = &((*prevptr)->next);
	}
	*prevptr = NULL;

	/*
	 * Read message into archive
	 */
	ptr = root;
	for (i = 0; i < nummsgs; i++) {
		/* Read one message */
		len = ptr->directory.length;
		ptr->message = (unsigned char *)malloc((unsigned)(len + 1));
		if (read(catfd, ptr->message, len) != len) {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,12, "dumpmsg: %s: Bad catalog file\n")),
				catfname);
			exit(13);
		}
		ptr->message[len] = NULL;

#ifdef TRACE
		fprintf(stderr, (catgets(nl_fn,NL_SETN,13, "setnum  : %d\n")), ptr->directory.setnum);
		fprintf(stderr, (catgets(nl_fn,NL_SETN,14, "msgnum  : %d\n")), ptr->directory.msgnum);
		fprintf(stderr, (catgets(nl_fn,NL_SETN,15, "length  : %d\n")), ptr->directory.length);
		fprintf(stderr, (catgets(nl_fn,NL_SETN,16, "addr    : %d\n")), ptr->directory.addr);
		fprintf(stderr, (catgets(nl_fn,NL_SETN,17, "message : %s\n\n")), ptr->message);
#endif TRACE

		ptr = ptr->next;
	}
}

int
write_msg()
{
	unsigned char	*p;
	int	i;
	short	setnum = 0;

	ptr = root;
	for (i = 0; i < nummsgs; i++) {

		/*
	 	* Write the set number.
	 	*/

		if (ptr->directory.setnum != setnum) {
			setnum = ptr->directory.setnum;
 			printf("$set %d\n", setnum);
		}

		/*
	 	* Write the message number.
	 	*/

		printf("%d ", ptr->directory.msgnum);

		/*
	 	* Write the message.
	 	*/

		for (p = ptr->message; 
		     p < ptr->message + ptr->directory.length; p++) {
			switch(*p) {
			case '\\':
				putchar('\\');
				putchar('\\');
				break;
			case '\b':
				putchar('\\');
				putchar('b');
				break;
			case '\f':
				putchar('\\');
				putchar('f');
				break;
			case '\n':
				putchar('\\');
				putchar('n');
				break;
			case '\r':
				putchar('\\');
				putchar('r');
				break;
			case '\t':
				putchar('\\');
				putchar('t');
				break;
			default:
#ifdef NLS16
				if (FIRSTof2(*p&0377) &&
				    !isspace(*(p+1)&0377) &&
				    !iscntrl(*(p+1)&0377)) {
					putchar(*p++);
					putchar(*p);
				}
				else
#endif NLS16
#ifdef NONLS
				if(isprint(*p))
#else NONLS
				if(nl_isprint(*p, langid))
#endif NONLS
					putchar(*p);
				else
					printf("\\%.3o", *p);
				break;
			}
		}

		putchar('\n');
		ptr = ptr->next;
	}
}

int
Perror(s)
char *s;
{
	fprintf(stderr, "dumpmsg: ");
	perror(s);
}
