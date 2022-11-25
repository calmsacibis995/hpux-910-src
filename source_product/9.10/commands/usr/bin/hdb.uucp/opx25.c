static char *RCS_ID="@(#)$Revision: 70.1 $ $Date: 91/11/07 10:48:02 $";
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS
/*	@(#) $Revision: 70.1 $	*/
/* HDB version of opx25 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <termio.h>
#include <fcntl.h>

char LOGFILE[] = "/usr/spool/uucp/.Log/LOGX25";


/*
 * what can the script input be?
 */
#define LABEL   1		/* <name> <LABCHAR> */
#define EXPECT  2		/* expect <delay> <string> */
#define ERROR 	3		/* error <label> */
#define ECHOS	4		/* echo <string> (to stderr) */
#define GOTO	5		/* goto <label> */
#define RUN	6		/* run <progname> <args...> */
#define EXIT	7		/* exit <optional exitarg> */
#define SEND	9		/* send <string> */
#define EXEC	10		/* like run, but no fork */
#define END	11		/* end of script file */
#define IDENT	12		/* any identifier */
#define EOL	13		/* end of line */
#define COMMENT 14		/* line beginning with COMCHAR */
#define STRING	15		/* in "", with C-style escapes */
#define NUMBER	16		/* a number */
#define SET	17		/* set debug */
#define BREAK   18		/* send a break character */
#define TIMEOUT 19		/* set total timeout */
#define EXPECTG 20		/* like EXPECT, but garbage in string
				   allowed */




/*
 * routines for processing lines that begin with keywords in kwd 
 */
int doexpect(), doerror(), doecho(), dogoto(), dorun(), 
	doexit(), dosend(), doexec(), doend(), donothing(),
	doset(), dobreak(), dotimeout(), doexpectg();
	
/*
 * all lines begin with a keyword in this table, unless they contain a 
 * label or a comment.  Each keyword has a function that interprets it. 
 */
typedef int (*funcp)();
struct kwd {
	char *k_string;
	int k_token;
	funcp k_fn;
} kwd[] = {
	0,		LABEL,		donothing,
	"expect",	EXPECT,		doexpect,
	"error",	ERROR,		doerror,
	"echo",		ECHOS,		doecho,
	"goto",		GOTO,		dogoto,
	"run",		RUN,		dorun,
	"exit",		EXIT,		doexit,
	"send",		SEND,		dosend,
	"exec",		EXEC,		doexec,
	"set",		SET,		doset,
	"break",	BREAK,		dobreak,
	"timeout",	TIMEOUT,	dotimeout,
	"expectg",	EXPECTG,	doexpectg,
	0,		COMMENT,	donothing,
	0,		EOL,		donothing,
	0,		END,		doend,
	0,		0,		0
};



funcp tlook();
char *copy();

FILE *scriptfile;
FILE *logfile;
FILE *dbgfile;
char *scriptname;

char *sname();
int linenum;			/* in scriptfile, for error messages */
int sawerror;			/* did last expect or run produce an error? */
int havetime;			/* alarm hasn't rung yet */
int debug;
int log;			/* make a log? */
int numlog;			/* log numbers only? */
int verbose;
int timeout=0;                  /* total time available for alarms */
int indev = 0;			/* input and output for the devices */
int outdev = 1;
char firstc;			/* first character, already read by caller */

#define COMCHAR '/'		/* comment lines begin with this */
#define QUOCHAR '"'		/* strings surrounded by this */
#define LABCHAR ':'		/* label character, following names */

#define TRUE    1
#define FALSE	0

#define streq(a,b) ( strcmp((a), (b)) == 0 )

#define LINE 300		/* max size of script or device input */

char phonenumber[LINE];		/* number to send */


/*
 * set up args, then process one line at a time
 */
main(ac, av)
	char **av; {

       
#ifdef NLS
        nl_catd nlmsg_fd;
        nlmsg_fd = catopen("uucp",0);
#endif
	arginit(ac, av);

	for (linenum = 1; ; linenum++)
		lprocess();
#ifdef NLS
        catclose(nlmsg_fd);
#endif

}




/*
 * interpret arguments:
 *	-f scriptfile
 *	-n phonenumber
 *	-d debug
 * 	-v verbose
 *	-i input fd
 *	-o output fd
 */
arginit(ac, av)
	char **av; {
	extern char *optarg;
	int c, fd;

	while ((c = getopt(ac, av, "c:i:o:f:n:dv")) != EOF)
		switch (c) {

		  case 'f':	
			scriptfile = fopen(optarg, "r");
			if (scriptfile == NULL) {
				perror(optarg);
				exit(1);
			}
			scriptname = optarg;
			break;

		  case 'c':
			firstc = optarg[0];
			break;

		  case 'd':
			debug = TRUE;
			break;

		  case 'v':
			verbose = TRUE;
			break;

		  case 'n':
			strcpy(phonenumber, optarg);
			break;

		  case 'i':
			indev = atoi(optarg);
			break;

		  case 'o':
			outdev = atoi(optarg);
			break;

		  default:
			usage(av[0]);
			exit(1);
		}

	if (scriptfile == NULL)
		scriptfile = stdin;

}





usage(name) {
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,536, "usage: %s -f script [-d] [-v]\n")), name);
}



		
/*
 * process a line from the script 
 */
lprocess() {
	char line[LINE], tokebuf[LINE];
	char *curlp;
	int token;
	funcp f;

	/* read in a line */
	fgets(line, LINE, scriptfile);
	dbgout("%s", line);

	/*
	 * find what token is at the beginning 
	 * 	tokebuf is the actual string 
	 * 	token is the token type (EXPECT, ...) 
	 */
	if (feof(scriptfile)) {
		*tokebuf = '\0';
		token = END;
	} else {
		curlp = line;
		token = gettoke(&curlp, tokebuf);
	}

	/*
	 * see if this is allowed at the beginning of a line 
	 * if so, call the appropriate function, giving it the rest 
	 * of the line as an argument 
	 */
	f = tlook(token, tokebuf);
	if (f == NULL) {
		if (strlen(tokebuf))
			error((catgets(nlmsg_fd,NL_SETN,537, "%s unknown")), tokebuf); 
		else	synerr();
		return;
	}

	/* call the function for that token */
	(*f)(curlp);
}




error(q,w,e,r,t) {

	fprintf(stderr, "%d: ", linenum);
	fprintf(stderr, q,w,e,r,t);
	putc('\n', stderr);
}




synerr() {
	error((catgets(nlmsg_fd,NL_SETN,538, "syntax error")));
}




/*
 * copy first token in plinep into tokep.  adjust plinep accordingly.
 * return what type of token it is.  This is the top-level lexing routine.
 */
gettoke(plinep, tokep)
	char **plinep, *tokep; {
	char *linep;
	int type;

	linep = *plinep;
	
	skipwhite(&linep);

	switch (*linep) {

	  case COMCHAR:
		type = COMMENT;
		break;

	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
		getnumber(&linep, tokep);
		type = NUMBER;
		break;
 
	  case QUOCHAR:
		++linep;
		getstring(&linep, tokep);
		type = STRING;
		break;

	  case 0:
	  case '\n':
		*tokep = '\0';
		type = EOL;
		break;

	  default:
		getident(&linep, tokep);
		skipwhite(&linep);
		if (*linep == LABCHAR) {
			linep++;
			type =  LABEL;
		} else	type = IDENT;
		break;
	}

	*plinep = linep;
	return type;
}




/* skip white space, adjusting line pointer */
skipwhite(plinep)
	char **plinep; {
	register char *linep;
	register c;

	linep = *plinep;
	while ((c = *linep) == ' ' || c == '\t')
		++linep;
	*plinep = linep;

}




/*
 * read in a string, interpreting escapes 
 * update plinep; put string into token 
 * the opening quote has already been read 
 */
getstring(plinep, token)
	char **plinep, *token; {
	register char *tokep, c;
	char *linep;
	int morestring;
	
	linep = *plinep;
	tokep = token;
	morestring = TRUE;

	while (morestring && (c = *linep++) != QUOCHAR) {
		switch (c) {
		  case '\0':
		  case '\n':
			/* unexpected end of string (no closing quote) */
			morestring = FALSE;
			continue;

		  case '\\':
			/* C-style escape */
			switch (c = *linep++) {
			  case '#':
				strcpy(tokep, phonenumber);
				tokep += strlen(tokep);	
				continue;
			  case 'b': 
				c = '\b'; break;
			  case 'n':
				c = '\n'; break;
			  case 'r':
				c = '\r'; break;
			  case 't': 
				c = '\t'; break;
			  case '0':
			  case '1':
			  case '2':
			  case '3':
			  case '4':
			  case '5':
			  case '6':
			  case '7':
				--linep;
				c = getoctal(&linep);
				break;
			  case '\0':
			  case '\n':
				morestring = FALSE;
				continue;
			}
			break;
		}
		*tokep++ = c;
	}
	*tokep = '\0';
	*plinep = linep;

}




/*
 * interpret escapes of the form \ddd, where ddd is octal ascii
 * return the number, updating plinep
 */
getoctal(plinep)
	char **plinep; {
	register char *linep;
	register num, i;

	num = 0;
	linep = *plinep;
	for (i = 0; i < 3; i++) 
		if ('0' <= *linep && *linep <= '7') 
			num = (num << 3) + *linep++ - '0';
		else	break;
	*plinep = linep;
	return num;

}



/*
 * read in an identifier -- any alphanumeric sequence 
 * write it into tokep.  update plinep 
 */
getident(plinep, tokep)
	char **plinep, *tokep; {
	register char *linep;
	register c;

	linep = *plinep;
	while (isalnum(c = *linep) || c == '_') {
		++linep;
		*tokep++ = c;
	}
	*tokep = '\0';
	*plinep = linep;
}





/*
 * read in a number, writing it into tokep (as a string), and updating 
 * plp 
 */
getnumber(plp, tokep)
	char **plp, *tokep; {
	register char *lp;

	lp = *plp;

	while (isdigit(*lp))
		*tokep++ = *lp++;
	*tokep = '\0';

	*plp = lp;
}


 

doexpect (lp) {awaitstring (lp,FALSE);}

doexpectg (lp) {awaitstring (lp,TRUE);}


/*
 * syntax: expect | expectg <delay> <string>
 *	reads the device until string is found
 *	(some garbage can come first -- that's ignored)
 *	if the string arrives before the delay, that's success
 *	else failure.  sawerror tells whether the expectatons were fulfilled
 *
 *	expectg allows garbage characters within of expected string
 */
awaitstring(lp,garbageok) 
	char *lp; int garbageok; {
	char inbuf[LINE];
	int delay;
	int firstmatch=FALSE;
	register char *pattern;
	char c;

	if (gettoke(&lp, inbuf) != NUMBER) {
		error((catgets(nlmsg_fd,NL_SETN,539, "syntax: expect number string")));
		return;
	}
	delay = atoi(inbuf);

	if (gettoke(&lp, inbuf) != STRING) {
		error((catgets(nlmsg_fd,NL_SETN,540, "syntax: expect number string")));
		return;
	}
	pattern = inbuf;
	havetime = TRUE;
	setalarm(delay);

	/* if the alarm goes off, havetime becomes FALSE */
	while (havetime && *pattern) {
		if (firstc) {
			c = firstc;
			firstc = 0;
		} else  read(indev, &c, 1);
		if (havetime) {
			logchar(c);
			dbgout("read char %c\n", c);
		}
		if (c != *pattern++)
		  if (!(garbageok && firstmatch))
			pattern = inbuf;
		else  firstmatch=TRUE;
		if (verbose)
			putc(c, stderr);
	}

	setalarm(0);

	sawerror = *pattern;
	if (sawerror)
		dbgout("expect failed\n");
	else	dbgout("expect succeeded\n");
}



	
/*
 * set an alarm for delay seconds.  timeout, if non-zero, has the
 * time available for alarms; exit if time has run out.
 */
setalarm(delay) {
	int catch();

	if (delay && timeout) {
		timeout -= delay;
		if (timeout < 0) {
			dbgout("ran out of time\n");
			exit(1);
		}
		else if ( timeout == 0 ) timeout = -1;
	}
	signal(SIGALRM, catch);
	alarm(delay);
}




catch() {
	dbgout("alarm rang\n");
	havetime = FALSE;
}




doerror(lp) { findlabel(lp, FALSE); }





dogoto(lp) { findlabel(lp, TRUE); }




/*
 * syntax: {error, goto} label
 *	we go to the label always if the keyword was goto,
 *	but only when there is an error (sawerror TRUE) if it was "error" 
 */
findlabel(lp, always)
	char *lp; {
	char findbuf[LINE], labbuf[LINE], inbuf[LINE];

	if (gettoke(&lp, findbuf) != IDENT) {
		error((catgets(nlmsg_fd,NL_SETN,546, "missing label")));
		return;
	}
	if (always || sawerror) {
		if (fseek(scriptfile, 0L, 0) == -1) {
			error((catgets(nlmsg_fd,NL_SETN,547, "error or goto impossible")));
			return;
		}
		linenum = 0;
		while (1) {
			++linenum;
			fgets(inbuf, LINE, scriptfile);
			if (feof(scriptfile)) {
				error((catgets(nlmsg_fd,NL_SETN,548, "label %s not found")), findbuf);
				exit(1);
			}
			lp = inbuf;
			if (gettoke(&lp, labbuf) == LABEL
			    && streq(labbuf, findbuf)) {
				dbgout(inbuf);
				return;
			}
		}
	}

}
		





dorun(lp) { runprog(lp, TRUE); }




doexec(lp) { runprog(lp, FALSE); }





/*
 * run or exec a program.  The 2 keywords are the same, except that
 *	exec produces no child process.  "sawerror" tells whether 
 * 	the run was sucessful (exit code of child).
 */
runprog(lp, forkwait)
	char *lp; { 
	char *eargs[100], strbuf[LINE];
	int eanum;
	int status;

	eanum = 0;
	while (gettoke(&lp, strbuf) != EOL)
		eargs[eanum++] = copy(strbuf);
	eargs[eanum] = 0;
	if (forkwait && fork()) {
		wait(&status);
		sawerror = (status & 0xFFFF);
		dbgout("run result %d\n", sawerror);
		return;
	} 

	execvp(eargs[0], eargs);
	error((catgets(nlmsg_fd,NL_SETN,550, "%s not found")), eargs[0]);
	exit(1);

}
	




/*
 * syntax: exit <optional number>
 *	exit with this number, or default 1 
 */
doexit(lp) 
	char *lp; {
	char numbuf[LINE];
	int exnum;

	if (gettoke(&lp, numbuf) == NUMBER)
		exnum = atoi(numbuf);
	else	exnum = 1;

	exit(exnum);
}





/*
 * syntax: timeout number
 * 	don't wait any longer than this amount of time
 */
dotimeout(lp) 
	char *lp; {
	char numbuf[LINE];
	int exnum;

	if (gettoke(&lp, numbuf) != NUMBER) 
		error((catgets(nlmsg_fd,NL_SETN,551, "expected number")));

	timeout = atoi(numbuf);
}





dosend(lp) { sendstr(lp, stdout); }




doecho(lp) { sendstr(lp, stderr); }




/*
 * send or echo a string.  The difference is the output file 
 */
sendstr(lp, f)
	FILE *f;
	char *lp; { 
	char strbuf[LINE];

	if (gettoke(&lp, strbuf) != STRING) {
		error((catgets(nlmsg_fd,NL_SETN,552, "expected string")));
		return;
	}
	if (f == stdout) {
		dbgout("sending %s\n", strbuf); 
		write(outdev, strbuf, strlen(strbuf));
	}
	else    fputs(strbuf, f);
	if (verbose && f != stderr)
		fputs(strbuf, stderr);
	if (gettoke(&lp, strbuf) != EOL)
		error((catgets(nlmsg_fd,NL_SETN,554, "too many args")));
}




donothing(){}




doend() { exit(0); }



/*
 * given a token type and the token string, find the type in the
 * kwd table and return a pointer to the entry.  If the token type is
 * IDENT, it might match any string 
 */
funcp
tlook(token, tokebuf) {
	struct kwd *kwdp;

	for (kwdp = kwd; kwdp->k_fn; kwdp++) {
		if (token == kwdp->k_token) 
			return kwdp->k_fn;
		if (token == IDENT 
		    && kwdp->k_string 
		    && streq(tokebuf, kwdp->k_string))
			return kwdp->k_fn;
	}

	return NULL;

}




/* copy a string to a safe place */
char *
copy(s)
	char *s; {
	char *new;

	new = (char *) malloc(strlen(s)+1);
	strcpy(new, s);
	return new;

}


/* 
 * set some internal flags:
 */
struct flagtab {
	char *f_string;
	int *f_variable;
} flagtab[] = {
	"debug",	&debug,
	"verbose",	&verbose,
	"log",		&log,
	"numlog",	&numlog,
	0,		0
};

doset(lp)
	char *lp; {
	char *flag;
	char flagbuf[LINE];
	int value;
	struct flagtab *fp;


	if (gettoke(&lp, flagbuf) != IDENT) {
		error((catgets(nlmsg_fd,NL_SETN,555, "expected flag")));
		return;
	}

	/*
	 * "nodebug" means turn off debugging
	 * similarly for other flags
	 */
	flag = flagbuf;
	if (flag[0] == 'n' && flag[1] == 'o') {
		value = FALSE;
		flag += 2;
	} else	value = TRUE;

	for (fp = flagtab; fp->f_string; fp++) 
		if (streq(fp->f_string, flag)) {
			*fp->f_variable = value;
			if (fp->f_variable == &log || fp->f_variable == &numlog)
				loginit();
			return;
		}

	error((catgets(nlmsg_fd,NL_SETN,556, "flag \"%s\" unrecognized")), flag);

}



/* debugging output */
dbgout(q,w,e,r,t) {

	if (!debug)
		return;
	if (dbgfile == NULL) {
		int logfd;

		logfd = open("/tmp/opx25.log", O_WRONLY|O_CREAT|O_APPEND, 0666);
		dbgfile = fdopen(logfd, "a");
		if (dbgfile == NULL || logfd < 0) {
			perror("/tmp/opx25.log");
			exit(1);
		}
	}
	fprintf(dbgfile, "+ ");
	fprintf(dbgfile,q,w,e,r,t);
	fflush(dbgfile);
}



/* send a break */
dobreak(lp) {

	ioctl(outdev, TCSBRK, 0);
	dbgout("sent a break\n");

}
 
/* return file component of full pathname */
char *
sname(s)
	char *s; {
	char *p;
	char *strrchr();

	if (!s) return s;
	p = strrchr(s, '/');
	if (p == NULL)
		p = s;
	else	++p;
	return p;

}

loginit() {
	long clock;
	int logfd;

	logfd = open(LOGFILE, 
				O_WRONLY|O_CREAT|O_APPEND,
				0600);
	if (logfd != -1)
		logfile = fdopen(logfd, "a");
	else	return;
	if (logfile == NULL || numlog)
		return;
	clock = time(0);
	logstring( "opx25 ");
	if (scriptname)
		logstring(sname(scriptname));
	else	logstring("(no script)");
	logstring(" ");
	logstring(ctime(&clock));
}

logstring(s)
	char *s; {

	if (logfile != NULL)
		fputs(s, logfile);
}

logchar(c)
	char c; {

	if (numlog && !isdigit(c))
		return;
	if (logfile != NULL)
		putc(c, logfile);
}

