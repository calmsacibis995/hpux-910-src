/*	@(#) $Revision: 66.1 $	*/
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include	"pupu.h"
#include	<varargs.h>
#include	<errno.h>
#include	<string.h>

	Xint	cfd;

	Xchar	mhead[H_DATA]	= "\206\006";
	Xchar	buf[BLOCKSIZE];
	XcharP	mnext;
	Xchar	sseq, rseq;

	Xjmp	giveup;
	XfnP	alarmwas;

	Xchar	wdir[NAMELEN];
	Xchar	basedir[NAMELEN];
	Xchar	wholename[NAMELEN];

	Xlink	linkhead;

	XfnP	errorfunction;
	Xint	lasterror;
	XcharP	remoteorlocal;
	XcharP	currentfile, timeoutmsg;

	unsigned char	openfiles[((MAXFD+4)+7)/8];	/* bit map of MAXFD+4 bits */

	Efn	gotinit, gotinitreply;

	Xexp	inittable[] = {
		'I',	gotinit,
		'i',	gotinitreply,
		0,	0
	};

	Efn	remotedone, remoteerror, alldone;

	Xexp	justquit[] = {
		'x',	remotedone,
		'e',	remoteerror,
		'X',	alldone,
		0,	0
	};

	SexpP	toexpect;

	Xchar	myversion[6]	= PPLEVEL2;
	Xchar	theirversion[6];

	Xchar	options[32];

	Efn	timeout;

begin(fd, errfcn, rol, initwdir)
	PfnP	errfcn;
	PcharP	rol, initwdir;
{
	cfd = fd;
	errorfunction = errfcn;
	remoteorlocal = rol;
	currentfile = NIL;
	lasterror = 0;

	strcpy(wdir, initwdir);

	signal(SIGALRM, timeout);
}

sendinit(opt)
	PcharP	opt;
{
	msginit('I');
	msgfmt("%s", myversion);
	msgfmt("%s", opt);

	msgconf(inittable);
}

gotinit()
{
	msgtake("%s", theirversion);
	msgtake("%s", options);

	msginit('i');
	msgfmt("%s", myversion);
	msgsend();

	return(1);
}

gotinitreply()
{
	msgtake("%s", theirversion);

	return(1);
}
	
err(msg, parm, en)
	PcharP	msg, parm;
{
	/*
	** Process error message
	*/

	warn(msg, parm, en);

	/*
	** If we are remote, send termination message to local.  Send
	** exit status message if the local has the code to handle it.
	*/

	if(*remoteorlocal == 'r')
		if (strcmp(theirversion, PPLEVEL2) >= 0) {
			msginit('x');
			msgfmt("%d", lasterror);
			msgsend();
		} else
			reply('X');

	stopnow(en);
}

warn(msg, parm, en)
	PcharP	msg, parm;
{
	RcharP	ep;
	Achar	ebuf[BUFSIZ];

	ep = ebuf;

	ep += sprintf(ep, "(%s): %s", remoteorlocal, msg);

	if(parm)
		ep += sprintf(ep, " [%s]", parm);

	if(en > 0 && errno >= 0 && errno <= sys_nerr)
		sprintf(ep, " -- %s", sys_errlist[errno]);

db(stderr, "warn * %s\n", ebuf);
	if(*remoteorlocal == 'l')
		(*errorfunction)(ebuf);
	else{
		msginit('e');
		msgfmt("%s", ebuf);
		msgsend();
	}

	lasterror = en < 0 ? -en : en;
}

	/* VARARGS2 */
openn(name, flags, mode)
	PcharP	name;
{
	Rint	fd;

	fd = open(name, flags, mode);

	if(fd >= 0)
		openfiles[fd/8] |= 1<<(fd%8);	/* set */

	return(fd);
}

closen(fd)
{
	close(fd);

	openfiles[fd/8] &= ~(1<<(fd%8));	/* clear */
}

remotedone()
{
	Aint	en;

	msgtake("%d", &en);

	if(en)
		lasterror = en;

	return(1);
}

remotefail()
{
	Aint	en;

	msgtake("%d", &en);
	stopnow(en);
}

alldone()
{
	stopnow(0);
}

stopnow(en)
{
	Rint	i;
	RlinkP	lp, nlp;

	alarm(0);
	signal(SIGALRM, alarmwas);

	for(i = 0; i < MAXFD+4; i++)
		if(openfiles[i/8]&(1<<(i%8)))	/* test bit i */
			closen(i);

	for(lp = linkhead.next; lp; lp = nlp){
		nlp = lp->next;
		free(lp);
	}
	linkhead.next = 0;

	if(en > 0)
		lasterror = en;
	else if(en < 0)
		lasterror = -en;

	longjmp(giveup, 1);
}

msgtake(fmt, ptr)
	PcharP	fmt, ptr;
{
	if(strcmp(fmt, "%s") == 0)
		strcpy(ptr, mnext);	/* Foo!  sscanf stops at white space */
	else
		sscanf(mnext, fmt, ptr);

	mnext += strlen(mnext)+1;
}

reply(type)
	Pchar	type;
{
	if(++sseq > SEQMAX)
		sseq = SEQMIN;
	mhead[H_SEQN] = sseq;

	mhead[H_TYPE] = type;

	timeoutmsg = "Protocol exchange timed out";

	alarm(WTIMEOUT);

	write(cfd, mhead, sizeof(mhead));

	alarm(0);
}

confirmed()
{
	return(1);
}

remoteerror()
{
	Achar	ebuf[BUFSIZ];

	msgtake("%s", ebuf);

	(*errorfunction)(ebuf);

	return(0);
}

msginit(type)
	Pchar	type;
{
	if(toexpect){
		msgwait(toexpect);
		toexpect = 0;
	}

	if(++sseq > SEQMAX)
		sseq = SEQMIN;
	mhead[H_SEQN] = sseq;

	mhead[H_TYPE] = type;

	strcpy(buf, mhead);

	mnext = &buf[H_DATA];
}

	/* VARARGS0 */
msgfmt(va_alist)
	va_dcl
{
	RcharP	fmt;
	va_list	args;

	va_start(args);

	fmt = va_arg(args, TcharP);

	mnext += vsprintf(mnext, fmt, args) + 1;
}

msgconf(expect)
	PexpP	expect;
{
	msgsend();

	toexpect = expect;
}

msgsend()
{
	Rint	len;
db(stderr, "msgsend %c\n", buf[H_TYPE]);

	len = mnext - buf;

	timeoutmsg = "Protocol exchange timed out";

	alarm(WTIMEOUT);

	if(write(cfd, buf, len) != len)
		if(*remoteorlocal == 'r')
			stopnow(EX_IOERR);
		else
			err("Message write failed", NIL, EX_IOERR);

	alarm(0);
}

msgwait(expect)
	PexpP	expect;
{
	Rint	len;
	RexpP	ep;

db(stderr, "msgwait %X\n", expect);
	timeoutmsg = "Protocol exchange timed out";

	for(;;){
		alarm(RTIMEOUT);

		len = read(cfd, buf, sizeof(buf));

		alarm(0);
db(stderr, "read len %d type %c\n", len, buf[H_TYPE]);

#if hpux
		if(len <= 0) {
			char	errmsg[BUFSIZ];
			if(len == 0)
				sprintf( errmsg, "Message %c read eof", 
						expect->type );
			else
				sprintf( errmsg, "Message %c read error", 
						expect->type );
			err(errmsg, NIL, EX_IOERR);
		}
#else
		if(len <= 0)
			err("Message read error", NIL, EX_IOERR);
#endif

		if(++rseq > SEQMAX)
			rseq = SEQMIN;
		if(buf[H_MAG1] != mhead[H_MAG1] || buf[H_MAG2] != mhead[H_MAG2])
#if hpux
		{
			char wbuf[60];
			sprintf("Bad magic number on message (len=%d) <%.2x><%.2x><%.2x><%.2x><%.2x>", len, buf[0]&0xff, buf[1]&0xff, buf[2]&0xff, buf[3]&0xff, buf[4]&0xff);
			err(wbuf, NIL, -EX_PROTOCOL);
		}
#else
			err("Bad magic number on message", NIL, -EX_PROTOCOL);
#endif
		if(buf[H_SEQN] != rseq)
			err("Bad sequence number on message", NIL, -EX_PROTOCOL);

		mnext = &buf[H_DATA];

		for(ep = expect; ep->type; ep++)
			if(ep->type == buf[H_TYPE])
				if((*ep->fn)())
					return;
				else
					break;

		if(!ep->type)
			err("Unexpected message type", &buf[H_TYPE], -EX_PROTOCOL);
	}
}


sendfile(name)
	RcharP	name;
{
	AcharP	d;

	if(d = strrchr(name, '/')){
		if(!wdir[0])
#if hpux
			/*
			 * get the name of the current directory.
			 */
			getcwd(wdir, NAMELEN);
#else
			curdir(wdir);
#endif

		if(d == name)
			name = "/";
		else
			*d = '\0';

		if(chdir(name) < 0){
			warn("Can't change directory to", name, EX_NOINPUT);
			return;
		}

		name = d+1;
	}

	putfile(name, name);

	if(d)
		chdir(wdir);
}

timeout()
{
	err(timeoutmsg, currentfile, -EX_IOERR);
}
