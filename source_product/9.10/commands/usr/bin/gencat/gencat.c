static char *HPUX_ID = "@(#) $Revision: 70.1 $";

/*
 * 1984
 *	9/5	1.3	Fix the null pointer bug of addmsg()
 *	9/7	1.4	Add '\\' function to check_buf()
 *	9/10	1.5	Add message number sequence check to addmsg()
 *	12/10	1.6	Modify the directory size
 *	12/18	1.7	Pre-localize the messages
 *	12/18	27.1	Ship to the shared source
 * 1985
 *	2/1	27.2	Change delimiter between msgnum and message 
 *	2/26	27.3	Delete the MPE special characters ('%' and '&')
 *	3/16	27.4	Change for the NLS stuff changes
 * 1986
 *	12/3	42.1	Add "$quote c" directive (-jh)
 */

/*
 * This program makes the formated message catalog file from the
 * source message catalog file.
 *
 * source catalog
 *
 *	Set numbers begin at the 1st column, $set ## [comment].
 *	Set number must be ascending order in one file.
 *	1 <= set number <= 255.
 *	Message numbers also begin at the 1st column, #### [message].
 *	Message numbers must be ascending order in one set.
 *	1 <= message number <= 32767.
 *	Comment lines must begin with `$' at the 1st column,
 *	$ [comment].
 *	If the message will be longer than one line, `&' must be just
 *	before the `\n' character.
 *	Catalog entries may have up to five parameters, their position
 *	being denoted by an exclamation mark (`!').
 *
 * formated catalog
 *
 *	This file is a binary one with over head, message directories
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
 * errors
 *
 *	Gencat performs several checks on the source file during the
 *	formatting of a new-style catalog.  The file is verified to
 *	ensure that the recore format is correct.  If any
 *	verification check fails, formating process is aborted,
 *	leaving the source file intact.
 *	Specially, the source file is checked that;
 *
 *		All directive are legal and used correctly.
 *		Set number are in ascending order.
 *		Set number are greater than 0 and less than or equal
 *		to 255.
 *		Message number are greater than 0 and less than or
 *		equal to 32767.
 *		Continuation and concatenation characters are correct.
 *		Parameter substitution characters are used correctly,
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#ifdef NLS16
#include <nl_ctype.h>
#endif NLS16

#ifdef vax
#include <sys/file.h>
#else
#include <fcntl.h>
#endif

#ifdef NLS || defined NLS16
#define NL_SETN 1
#include <msgcat.h>
#include <locale.h>
#include <nl_types.h>
nl_catd nl_fn;
#else 
#define catgets(i,sn,mn,s) (s)
#endif NLS || NLS16

#define isoctal(c)	('0' <= c && c <= '7')

struct ar {
	DIR		directory;
	struct ar	*next;
	char		*message;
};
typedef struct ar	AR;

int	errno;			/* External error variable */
int	catfd;			/* Catalog file descriptor */
long	nummsgs = 0,		/* Number of message in archive */
	lseek();
FILE	*fd;			/* Message file descriptor */
AR	*root = NULL,		/* Root pointer to archive */
	*ptr,
	**prevptr;
char	*catfname,		/* Save catalog file name */
	*msgfname,		/* Save message file name */
	*strcpy(),
	*malloc(),
	*remove_number();
int	quotec = 0;
int	prev_setnum = 1;	/* Default set number */
long	prev_msgnum = 0;
long	Max_MsgLen = NL_TEXTMAX;	/* maximum message length */
int	tflag = 0,		/* truncation flag1 */
	localtflag,		/* truncation flag2 */
	hflag = 0;		/* header flag, for future use*/

/* tflag is used as an indicator that there was at least one (possibly
   more) line truncated from the message file. localtflag is used to
   indicate that the line being read has been truncated and that an error
   message has already been emitted */

main(argc, argv)
	int argc;
	char **argv;
{
	int	c;
	extern	char *optarg;
	extern	int optind;
	int	openflag,
		OptErr;

#ifdef NLS16
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale(),stderr);
		nl_fn=(nl_catd)-1;
	}
	else
		nl_fn=catopen("gencat",0);
#endif NLS16

	OptErr = FALSE;
	while( (c=getopt(argc,argv,"l")) != EOF ) {
		switch(c) {
		case 'l':
			/* lopt = optarg */
			Max_MsgLen = MAX_BUFLEN - 1;
			continue;
		case 'h':
			/*********** for future use
			hflag = 1;
			************/
			OptErr = TRUE;
			continue;
		case '?':
			OptErr = TRUE;
			break;
		}
	}
	/* move argv & argc past options */
	argv = &argv[optind];		/* arguments begin here */
	argc = argc - optind;		/* argument count */
	if (argc < 2 || OptErr) {	/* Check valid usage */
		fprintf(stderr,
			(catgets(nl_fn,NL_SETN,1,"usage: gencat [ -l ] catfile files ...\n")));
		exit(1);
	}

	catfname = *argv;
	/* Open the catalog file for read. */
	if ((catfd = open(*argv, O_RDONLY)) == ERROR) {
		if (errno != ENOENT) {
			Perror(*argv);
			exit(2);
		}
	}
	else {
		/* Read the contents and add it to archive */
		read_catfile();

		/* Close the catalog file */
		if (close(catfd) == ERROR) {
			Perror(catfname);
			exit(3);
		}
	}

	argv++;
	argc--;
	
	while (argc--) {
		/* Open the message file */
		if ((fd = fopen(*argv, "r")) == NULL) {
			Perror(*argv);
			exit(4);
		}
		msgfname = *argv;
		/* Read the contents and add it to archive. */
		read_msgfile();

		if (fclose(fd) == EOF) {
			Perror(*argv);
			exit(5);
		}
		argv++;
	}
	if (tflag) {		/* truncation occured */
		exit(8);
	}

	/* Open the catalog file for write. */
	if (catfd == ERROR) {		/* Open new file */
		openflag = O_WRONLY | O_CREAT;
	}
	else {				/* Rewrite the existent file. */
		openflag = O_WRONLY | O_TRUNC;
	}
	if ((catfd = open(catfname, openflag, MODE)) == ERROR) {
		Perror(catfname);
		exit(6);
	}

	/* Write out the messages to the catalog file */
	write_catfile();

	if (close(catfd) == ERROR) {
		Perror(catfname);
		exit(7);
	}
	return(0);
}

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
	else if (len == 0) { /* End of file, catalog file must be new.  */
		return;
	}

	/*
	 * Check the catalog file ID.
	 */
	if (strncmp(buf, CATID, IDLEN)) {
		/* Bad catalog file ID. */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,2,"gencat: %s: Not a catalog file\n")), catfname);
		exit(11);
	}

	/*
	 * Get the number of messages in the catalog.
	 */
	nummsgs = *(long *) &buf[NUM_MSG_POS];
	/*
	 * Read directories into archive
	 */
	prevptr = &root;
	for (i = 0; i < nummsgs; i++) {
		*prevptr = (AR *) malloc((unsigned)sizeof(AR));	
		/* Read one directory */
		if (read(catfd, (char *)*prevptr, DIRSIZE) != DIRSIZE) {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,3,"gencat: %s: Bad catalog file\n")), catfname);
			exit(12);
		}
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
		ptr->message = malloc((unsigned)(len + 1));
		if (read(catfd, ptr->message, (unsigned) len) != len) {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,4,"gencat: %s: Bad catalog file\n")), catfname);
			exit(13);
		}
		ptr->message[len] = NULL;
		ptr = ptr->next;
	}
}

read_msgfile()
{
	int	set = 0,	/* Current set number */
		num;		/* Temporary variable */
	long	lnum = 0,	/* Current line number of input file */
		getline();
	char	buf[MAX_BUFLEN+9]; /* include msg number, space and quotes */

	/* Reset as needed */
	ptr = root;
	prevptr = &root;
	prev_setnum = 0;
	prev_msgnum = 0;

	while ((lnum = getline (buf, lnum)) != EOF) {
		if (strncmp (buf, SET, SETLEN) == 0) {
			/* Found "$set" */
			/* Get the set number */
			num = get_num (&buf[SETLEN]);
			if (num < 1 || num > MAX_SETNUM || num == set) {
				/* Illegal set number */
				fprintf(stderr, (catgets(nl_fn,NL_SETN,5,"\"%s\", line %ld: Bad set number\n")), msgfname, lnum);
				exit(20);
			}
			if (num < set) {
				/* Illegal set number sequence */
				fprintf(stderr, (catgets(nl_fn,NL_SETN,6,"\"%s\", line %ld: Bad set number sequence\n")), msgfname, lnum);
				exit(21);
			}
			set = num;
		}
		else if (strncmp (buf, DELSET, DELSETLEN) == 0) {
			/* Found "$delset" */
			/* Get the set number and delete it */
			num = get_num(&buf[DELSETLEN]);
			if (num <= set || delset(num) == ERROR) {
				/* Illegal set number */
				fprintf(stderr, (catgets(nl_fn,NL_SETN,7,"\"%s\", line %ld: Bad delset number\n")), msgfname, lnum);
				exit(22);
			}
		}
		else if (strncmp (buf, QUOTE, QUOTELEN) == 0) {
			/* Found "$quote" */
			/* Get the quote character */
			get_quote(&buf[QUOTELEN], lnum);
		}
		else if (buf[0] == '$') {
			/* Comment line */
			/* Ignore */
		}
		else if (isdigit(*buf)) {
			/* This line must be a message line */
			if (set == 0) {
				set = NL_SETD;
				/* No `$set' found
				fprintf(stderr, (catgets(nl_fn,NL_SETN,8,"\"%s\", line %ld: No `$set'\n")), msgfname, lnum);
				exit(23);
				****************/
			}
			addmsg(set, buf, lnum);
		}
		else if (IsEmpty(buf)) {	/* "blank" line */
			}
		else {
			/* Illegal line - no message number */
			fprintf(stderr, (catgets(nl_fn,NL_SETN,9,"\"%s\", line %ld: No message number\n")), msgfname, lnum);
			exit(24);
		}
	}
}

long
getline(buf, lnum)
char *buf;
long lnum;
{
	int	c,
		temp;
	char	*bufptr = buf;
	long	savelnum = lnum + 1;

	/* lnum is the current message number - 1 */
	/* localtflag is used to indicate that there has or has not already
	   been a error message for a line that was too long (also used in
           check_buf()*/

	localtflag = 0;
	for (;;) {
		c = getc(fd);
		if (bufptr >= &buf[Max_MsgLen+9]) {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,10, "\"%s\", line %ld: Message too long\n")), msgfname, savelnum);
			tflag = 1;
			localtflag = 1;
		}
		if (c == EOF) {
			if (bufptr == buf) {	/* good EOF */
				return (EOF);
			}
			else {			/* bad EOF without '\n' */
				(void) ungetc('\n', fd);
			}
		}
		else if (c == '\\') {
			if ((temp = getc(fd)) == '\n') {
				/* Concatenation */
				lnum++;
			}
#ifdef NLS16
			else if (FIRSTof2(temp)) {
				*bufptr++ = c;
				(void) ungetc(temp, fd);
			}
#endif NLS16
			else {
				*bufptr++ = c;
				*bufptr++ = temp;
			}
		}
		else if (c == '\n') {
			lnum++;
			*bufptr = NULL;
			return (lnum);
		}
	/* current line has been truncated. Continue reading in characters
  	   until EOF or a linefeed has been reached. */
		else if (localtflag) {
			while ((c != EOF) && ( c != '\n')) {
				c = getc(fd);
			}
			if (c == EOF)
			    return(EOF);
			else {
				*bufptr=NULL;
				lnum++;
				return (lnum);
			}
		}
		else {
			*bufptr++ = c;
#ifdef NLS16
			if (FIRSTof2(c))
				if ((temp = getc(fd)) == '\n' || temp == EOF)
					(void) ungetc(temp, fd);
				else
					*bufptr++ = temp;
#endif NLS16
		}
	}
}

int
addmsg(setnum, buf, lnum)
	int setnum;
	char *buf;
	long lnum;
{
	long	msgnum;
	AR	*tempptr;
	int	delim = 0;

	/* Check the message number */
	if ((msgnum = get_num(buf)) < 1 || msgnum > MAX_MSGNUM ) {
		/* Illegal message number */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,12,"\"%s\", line %ld: Bad message number\n")), msgfname, lnum);
		exit(41);
	}

	/* Check the set number sequence */
	if (setnum < prev_setnum) {
		/* Set number sequence error! */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,6,"\"%s\", line %ld: Bad set number sequence\n")), msgfname, lnum);
		exit(21);
	}
	else if (setnum > prev_setnum) {
		/* new set. reset the prev_msgnum */
		prev_setnum = setnum;
		prev_msgnum = 0;
	}

	/* Check the message number sequence */
	if (msgnum <= prev_msgnum) {
		/* Message number sequence error! */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,13,"\"%s\", line %ld: Bad message number sequence\n")), msgfname, lnum);
		exit(42);
	}
	prev_msgnum = msgnum;

	/* Remove the message number and blanks */
	buf = remove_number(buf);
	if (*buf == ' ' || *buf == '\t') {
		buf++;
		delim++;
	}
	else if (*buf != NULL) {
		/* Bad delimiter */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,14,"\"%s\", line %ld: Bad delimiter\n")), msgfname, lnum);
		exit(45);
	}

	/* Check the validity of message */
	if (check_buf(buf, lnum) == ERROR) {
		/* Illegal message */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,15,"\"%s\", line %ld: Bad message\n")), msgfname, lnum);
		exit(43);
	}

	/* Move the archive pointer to the set */
	(void) search_set(setnum);

	/* Move the archive pointer inside the set */
	if (ptr && ptr->directory.setnum == setnum)
		while (ptr &&
		       ptr->directory.setnum == setnum &&
		       ptr->directory.msgnum < msgnum	 ) {
			prevptr = &(ptr->next);
			ptr = ptr->next;
		}

	if (ptr &&
	    ptr->directory.setnum == setnum &&
	    ptr->directory.msgnum == msgnum   ) {
		/* There already exists the message */
		if (*buf || delim) {
			/* Change the message */
			ptr->directory.length = strlen(buf);
			free(ptr->message);
			ptr->message
				= malloc((unsigned)(ptr->directory.length + 1));
			strcpy(ptr->message, buf);
		}
		else {
			/* Delete the directory */
			nummsgs--;
			*prevptr = ptr->next;
			free(ptr->message);
			free((char *)ptr);
			ptr = *prevptr;
		}
	}
	else {
		/* There is no directory with the setnum and msgnum */
		if (*buf || delim) {
			/* There is a message. Add it. */
			nummsgs++;
			tempptr = (AR *) malloc(sizeof(AR));	
			tempptr->directory.setnum = setnum;
			tempptr->directory.msgnum = msgnum;
			tempptr->directory.length = strlen(buf);
			tempptr->directory.reserved = 0;
			tempptr->message
			  = malloc((unsigned)(tempptr->directory.length + 1));
			strcpy(tempptr->message, buf);
			tempptr->next = ptr;
			*prevptr = tempptr;
			prevptr = &(tempptr->next);
		}
		else {
			/* There is no message.  Cannot delete. */
			fprintf(stderr, (catgets(nl_fn,NL_SETN,16,"\"%s\", line %ld: Bad delete message number\n")), msgfname, lnum);
			exit(44);
		}
	}
}

int
get_num(buf)
	char *buf;
{
	int	num = 0;

	while (*buf == ' ' || *buf == '\t')
		buf++;
	while (isdigit (*buf))
		num = num * 10 + *buf++ - '0';
	return (num);
}

get_quote(buf, lnum)
	char *buf;
	long lnum;
{
	while (*buf == ' ' || *buf == '\t')
		buf++;
	quotec = CHARAT(buf);
}

char *
remove_number(buf)
	char *buf;
{
	while (isdigit(*buf))
		buf++;
	return(buf);
}

check_buf(buf, lnum)
	char *buf;
	long lnum;
{
	/* Check the '\' characters. */
	char	*p1,
		*p2;
	int	n,
		c,
		quote = 0;

	p1 = p2 = buf;
	if ((c = CHARAT(p1)) && c == quotec) {	/* note quote char */
		quote++;
		CHARADV(p1);
	}
	/* check to see if the line exceeded the alloted line length, after
	   the length of the quote characters have been subtracted. But 
	   first check to see if the current line has already had an line
	   too long message output'd */
	if ((strlen(p1) - quote > Max_MsgLen) && (!localtflag)){
		fprintf(stderr, (catgets(nl_fn,NL_SETN,10,"\"%s\", line %ld: Message too long\n")), msgfname, lnum);
		tflag = 1;
	}
	while (c = CHARADV(p1)) {

		if (c == quotec) {
			quote++;
			break;
		}
		else if (c == '\\') {
			c = CHARADV(p1);
			if      (c == 'n') *p2++ = '\n';
			else if (c == 't') *p2++ = '\t';
			else if (c == 'b') *p2++ = '\b';
			else if (c == 'r') *p2++ = '\r';
			else if (c == 'f') *p2++ = '\f';
			else if (c == 'v') *p2++ = '\v';
			else if (isoctal(c)) {	/* '\ddd' */
				sscanf(&p1[-1], "%3o", &n);
				*p2++ = (char)n;
				if (isoctal(*p1)) {
					p1++;
					if (isoctal(*p1))
						p1++;
				}
			}
			else {
				WCHARADV(c,p2);
			}
		}
		else {
			WCHARADV(c,p2);
		}
	}
	if (quote != 0 && quote != 2) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,18,"\"%s\", line %ld: Missing quote character\n")), msgfname, lnum);
		exit(62);
	}
	*p2 = NULL;
	return(0);
}

int
delset(setnum)
	int setnum;
{
	if (setnum < 1 || setnum > MAX_SETNUM ||
	    (search_set(setnum) == ERROR)) {
		/* Illegal set number */
		return(ERROR);
	}
	while (ptr && ptr->directory.setnum == setnum) {
		nummsgs--;
		*prevptr = ptr->next;
		free(ptr->message);
		free((char *)ptr);
		ptr = *prevptr;
	}
	return(setnum);
}

int
search_set(setnum)
	int setnum;
{
	while (ptr) {
		if (ptr->directory.setnum == setnum)
			/* Found the set */
			return(setnum);
		if (ptr->directory.setnum > setnum)
			/* Set not found */
			return(ERROR);
		prevptr = &(ptr->next);
		ptr = ptr->next;
	}
	/* Set not found */
	return(ERROR);
}

write_catfile()
{
	long	address;
	int	i;

	/*
	 * Move the file pointer to the top of the file.
	 */
	if (lseek(catfd, 0l, L_SET) == ERROR) {
		Perror(catfname);
		exit(50);
	}
	/*
	 * Write the catalog file ID.
	 */
	if (write(catfd, CATID, IDLEN) != IDLEN) {
		Perror(catfname);
		exit(51);
	}
	/*
	 * Write the number of messages.
	 */
	if (write(catfd, (char *)&nummsgs, sizeof(nummsgs))
					!= sizeof(nummsgs)) {
		Perror(catfname);
		exit(52);
	}
	/*
	 * Write the directories.
	 */
	ptr = root;
	address = OVERHEAD + nummsgs * DIRSIZE;
	for (i = 0; i < nummsgs; i++) {
		ptr->directory.addr = address;
		if (write(catfd, (char *)&(ptr->directory), DIRSIZE)
							 != DIRSIZE) {
			Perror(catfname);
			exit(53);
		}
		address += ptr->directory.length;
		ptr = ptr->next;
	}
	/*
	 * Write the messages.
	 */
	ptr = root;
	for (i = 0; i < nummsgs; i++) {
		if (write(catfd, ptr->message, (unsigned) ptr->directory.length)
					    != ptr->directory.length) {
			Perror(catfname);
			exit(54);
		}
		ptr = ptr->next;
	}
}

Perror(s)
	char *s;
{
	fprintf(stderr, "gencat: ");
	perror(s);
}

IsEmpty(buf)
char *buf;
{
	while ( *buf == ' ' || *buf == '\t' ) buf++;
	return ( (*buf == '\0') ? TRUE : FALSE );
}
