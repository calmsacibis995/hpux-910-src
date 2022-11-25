static char *HPUX_ID = "@(#) $Revision: 70.1 $";

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>

#ifdef NLS16
#include <nl_ctype.h>
#else
#define CHARADV(p)	(*p++)
#define PCHARADV(c, p)	(*p++ = c)
#define ADVANCE(p)	(p++)
#endif

#ifndef NLS
#define catgets(i,sn, mn, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <locale.h>
#include <nl_types.h>
nl_catd nl_fn;
#endif NLS

#define LINESIZ (2 * (NL_TEXTMAX + 2))

/* define macro which advances through white space */
#define WSADVANCE(p) \
	while (*p == ' ' || *p == '\t') p++;

/* define macro which converts number string to integer value */
#define GETDIGIT(p,s) \
	while (isdigit(*p)) s = s * 10 + *p++ - '0';

/* define macro which reads through white space to a comma */
#define READTOCOMMA(p) \
	WSADVANCE(p);  \
	if (*p != ',') return(0);  \
	p++; \
	WSADVANCE(p); 


#define DEF	"define"
#define DEFLEN	6		/* strlen(DEF) */
#define SETN	"NL_SETN"
#define SETNLEN 7		/* strlen(SETN) */
#define NL_MSG1	"nl_msg"
#define MSGLEN1	6		/* strlen(NL_MSG1) */
#define NL_MSG2 "catgets"
#define MSGLEN2 7		/* strlen(NL_MSG2) */

#define TMPFILE	"/usr/tmp/findmsgXXXXXX"

FILE	*fptr;
int	exitcode = 0;

main(argc, argv)
int argc;
char **argv;
{
	int	aflag = 0;	/* sort all messages in all files */
	int	header = 0;	/* print file name header for each file */
	int	onintr();

#ifdef NLS || defined NLS16
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale(),stderr);
		nl_fn = (nl_catd)-1;
	}
	else
		nl_fn = catopen("findmsg",0);
#endif NLS || NLS16

	if (signal(SIGHUP, onintr) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGINT, onintr) == SIG_IGN)
		signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, onintr);
	signal(SIGTERM, onintr);

	argc--, argv++;

	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'a':
			aflag++;	/* sort all messages in all files */
			break;
		case 't':
			dumpx();	/* use tmp file */
			break;
		default:
			fprintf(stderr, (catgets(nl_fn,NL_SETN,1, "usage: findmsg [-a] files ..\n")));
			exit(1);
		}
		argc--, argv++;
	}

	if (argc == 0) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,1, "usage: findmsg [-a] files ..\n")));
		exit(1);
	} else
		if (argc > 1 && !aflag)
			header++;
		while (argc-- > 0) {
			if ((fptr = fopen(*argv, "r")) == NULL) {
				perror(*argv);
			} else {
				if (header) {
					printf("$ ==============\n");
					printf("$ %s\n", *argv);
					printf("$ ==============\n");
				}
				process();
				if (!aflag) {
					dump();
				}
				fclose(fptr);
			}
			argv++;
		}
	if (aflag)
		dump();
	onintr();		/* delete tmp file if any */
}

char	*bufptr;
char	buf[LINESIZ];

/*
 *	process each input file
 */

process()
{
	char	msgbuf[LINESIZ];
	int	setn = 0, n;

	while (fgets(buf, LINESIZ, fptr) != NULL) {
		if (!chkcmt(buf,fptr))
		   if ((n = chkset(buf)) > 0) {
			setn = n;
		   } else {
			bufptr = buf;
			while ((n = chkmsg(msgbuf)) > 0)
				add(setn, n, msgbuf);
		}
	}
}

/*
 *	check if the line is a comment
 *
 */
chkcmt(buf,fptr)
char *buf;
FILE *fptr;
{
	char *p = buf;
	char buf2[LINESIZ];

	WSADVANCE(p);
	if (*p == '/' && *(p+1) == '*')  {
	   p += 2;
	   while (1) {
	      while (*p) {
		 if (*p == '*' && *(p+1) == '/') {
		    p += 2;
		    WSADVANCE(p);
		    if (*p == '\n')
		       return(1);
 		    else
	               return(0);
		 }
		 else	
	            p++;
	     }
	     if (fgets(buf2,LINESIZ,fptr) == NULL) 
		   return(0);
	        else
		   p = buf2;
           }
	}
	return(0);
}


/*
 *	check if the line contains the valid NL_SETN definition
 *
 *		returns set number
 */
chkset(buf)
char *buf;
{
	char	*p = buf;
	int	set = 0;

	if (*p++ != '#')
		return(0);
	WSADVANCE(p);
	if (strncmp(p, DEF, DEFLEN))
		return(0);
	p += DEFLEN;
	WSADVANCE(p);
	if (strncmp(p, SETN, SETNLEN))
		return(0);
	p += SETNLEN;
	WSADVANCE(p);
	GETDIGIT(p,set);
	if (set < 1 || set > NL_SETMAX)		/* check that set is within */
		return(0);			/* limits */
	return(set);
}

/*
 *	check if the line contains nl_msg macro or nl_msg comment
 *
 *	o catgets(nl_fn,NL_SETN,msgnum, "message");
 *	o "message"	/* nl_msg msgnum */
/*
 *		returns message number
 *		message string is copied to msgbuf
 */
chkmsg(msgbuf)
char  *msgbuf;
{
	register char	*q;
	register int	c;
	int	msgnum = 0;
	int	macro = 0;
	int	string = 0;
	int	length = 0;

	*msgbuf = 0;

	while (*bufptr) {
		if (strncmp(bufptr, NL_MSG1, MSGLEN1) == 0) {
			bufptr += MSGLEN1;
			if (*bufptr == '(') {
				msgnum = 0;
				macro++;
				bufptr++;
			}
			WSADVANCE(bufptr);
			GETDIGIT(bufptr,msgnum);
			if (msgnum < 1 || msgnum > NL_MSGMAX) 
			    return(0);
			if (!macro)
			   while (*bufptr != '*' && *(bufptr+1) != '/' && (isprint(*bufptr) || isspace(*bufptr))) 
				bufptr++;
		        else 
			   WSADVANCE(bufptr);
			if (msgnum == 0 || macro && *bufptr != ',' || !macro && *bufptr != '*')
				return(0);
			if (!macro && string)
				return(msgnum);
		}
		else if (strncmp(bufptr, NL_MSG2, MSGLEN2) == 0) {
			bufptr += MSGLEN2;
			WSADVANCE(bufptr);
			if (*bufptr == '(') {
			   msgnum = 0;
			   macro++;
			   bufptr++;
			   while (isalnum(*bufptr) || (*bufptr == '_'))
			      bufptr++;
			   READTOCOMMA(bufptr);
			   if (strncmp(bufptr,"NL_SETN",7) == 0)  {
			      bufptr += 7;
			      READTOCOMMA(bufptr);
			   }
			  else
			     return(0);
			}
			WSADVANCE(bufptr);
			GETDIGIT(bufptr,msgnum);
			if (msgnum < 1 || msgnum > NL_MSGMAX) 
			    return(0);
			if (!macro)
			   while (*bufptr != '*' && *(bufptr+1) != '/' && (isprint(*bufptr) || isspace(*bufptr))) 
				bufptr++;
		        else 
			   WSADVANCE(bufptr);
			if (msgnum == 0 || macro && *bufptr != ',' || !macro && *bufptr != '*')
				return(0);
			if (!macro && string)
				return(msgnum);

		} else if (CHARADV(bufptr) == '"') {
			q = msgbuf;
			while ((c = CHARADV(bufptr)) && c != '"' && c != '\n') {
				if (++length > NL_TEXTMAX) {
					fprintf(stderr, (catgets(nl_fn,NL_SETN,4, "findmsg: maximum message size NL_TEXTMAX exceeded. \n")));
					return(0);
				}
				PCHARADV(c, q);
				if (c == '\\') {
					c = CHARADV(bufptr);
					if (++length > NL_TEXTMAX) {
						fprintf(stderr, (catgets(nl_fn,NL_SETN,4, "findmsg: maximum message size NL_TEXTMAX exceeded. \n")));
						return(0);
					}
					PCHARADV(c, q);
					if (c == '\n') {
						if (fgets(buf, LINESIZ, fptr) == NULL) break;
						bufptr = buf;
						}
					if (c == 0)
						return(0);
				}
			}
			*q = 0;
			if (c == '\n')
				return(0);
			if (msgnum)
				return(msgnum);
			string++;
		}
	}
	return(0);
}

struct msg {
	int	setnum;
	int	msgnum;
	char	*message;
	struct msg	*next;
};

struct msg	root;
struct msg	*lastp = &root;

FILE	*tmpfptr;
char	*tmpfname;

long	currpos;

/*
 *	add one message to link
 */
add(setn, msgn, message)
int setn;
int msgn;
char *message;
{
	struct msg	*p, *alloc();
	register struct msg	*q;

	p = alloc(setn, msgn, message);

	if (lastp->setnum > setn ||
	    lastp->setnum == setn && lastp->msgnum > msgn)
		lastp = &root;		/* rewind the link pointer */

	for (q = lastp/*->next*/; q; q = q->next) {
		if (q->setnum > setn ||
		    q->setnum == setn && q->msgnum > msgn) {
			/* add new link */
			lastp->next = p;
			p->next = q;
			lastp = p;
			return;
		} else if (q->setnum == setn && q->msgnum == msgn) {
			/* replace the message */
			if (!tmpfptr)
				free(q->message);
			q->message = p->message;
			free(p);
			lastp = q;
			return;
		}
		lastp = q;
	}
	lastp->next = p;
	lastp = p;
}

/*
 *	allocate a space for one message structure
 *	if malloc() fails, dumps current messages into tmp file
 *	and tries to continue.
 */
struct msg *
alloc(setn, msgn, message)
int setn;
int msgn;
char *message;
{
	struct msg	*p;
	char		*malloc();

	if ((p = (struct msg *)malloc(sizeof(struct msg))) == NULL ||
	    tmpfptr == NULL && (p->message = malloc(strlen(message)+1)) == NULL) {
		if (tmpfptr == NULL && root.next) {
#ifdef DEBUG
			fprintf(stderr, "dump!!\n");
#endif
			dumpx();
			exitcode = 2;
			return(alloc(setn, msgn, message));
		} else {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,2, "out of memory\n")));
			exit(1);
		}
	}
	if (tmpfptr == NULL)
		strcpy(p->message, message);
	else {
		p->message = (char *)currpos;
		currpos += outstr(message);
	}
	p->setnum = setn;
	p->msgnum = msgn;
	p->next = 0;
	return(p);
}

/*
 *	dumps current message list to stdout
 */
dump()
{
	struct msg	*p = root.next;
	struct msg	*tmp;
	int	set = 0;
	int	c;

	while (p) {
		if (p->setnum != set) {
			set = p->setnum;
			printf("$set %d\n", set);
		}
		if (tmpfptr) {
			printf("%d ", p->msgnum);
			fseek(tmpfptr, (long)p->message, 0);
			while ((c = getc(tmpfptr)) && c != EOF)
				putchar(c);
			putchar('\n');
		} else {
			printf("%d %s\n", p->msgnum, p->message);
			free(p->message);
		}
		tmp = p;
		p = p->next;
		free(tmp);
	}
	root.next = 0;
	lastp = &root;
	if (tmpfptr) {
		currpos = 0;
		rewind(tmpfptr);
	}
}

/*
 *	dumps current message list to a temporary file
 */
dumpx()
{
	struct msg	*p = root.next;
	int	n;
	char	*mktemp();

	tmpfname = mktemp(TMPFILE);
	if ((tmpfptr = fopen(tmpfname, "w+")) == NULL) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,3, "findmsg: cannot create temp file %s\n")), tmpfname);
		exit(1);
	}
	currpos = 0;

	while (p) {
		n = outstr(p->message);
		free(p->message);
		p->message = (char *)currpos;
		currpos += n;
		p = p->next;
	}
}

outstr(str)
char *str;
{
	register	i = 1;

	while (*str) {
		putc(*str++, tmpfptr);
		i++;
	}
	putc('\0', tmpfptr);
	return(i);
}

onintr()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	if (tmpfptr) {
		fclose(tmpfptr);
		unlink(tmpfname);
	}
	exit(exitcode);
}
