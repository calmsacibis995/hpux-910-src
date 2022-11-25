/*	@(#) $Revision: 64.1 $	*/
/*
 *	Inform the DATAKIT VCS Common Control of a new Server
 *	and receive requests for its service.
 */
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include <dk.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#ifdef	SVR3
#	define	SIGRTN	void
#else
#	define	SIGRTN	int
#endif
#include <setjmp.h>

#define NCHAN	255		/* number of channels supported */
#define BLX_NODE	0
#define G1_NODE	1

static	unsigned short	dkmgrstamp[NCHAN];
static	char		dkmgribuf[512];
int			dkmgropen = 0;
int			dksrvfd;
static	struct 	mgrmsg	dkmgrmsg;
static jmp_buf		noluck;
static	int		nodetype = BLX_NODE;
static	short		hchan;

SIGRTN			timeup();

extern int		errno;
extern char		*sys_errlist[];
extern FILE		*logf;
extern int		loglvl;

struct mgrmsg *
dkmgr(server)
char	*server;
{
	struct mgrmsg *dknmgr();
	return(dknmgr(server, atoi(getenv("DKINTF"))));
}

struct mgrmsg *
dknmgr(server, intf)
char	*server;
{
	register	n;
	struct diocreq	iocb;
	SIGRTN		(*sigwas)();
	unsigned int	timewas;
	int		m_type = 0;
	char		dial_dev[32];

	dkmgrmsg.m_errmsg = 0;
	errno = 0;
	sprintf(dial_dev, "/dev/dk/dial%d", intf);
	if ((dksrvfd = dkmgropen) == 0) {
		dksrvfd = open(dial_dev, O_RDWR) ;
		if (dksrvfd < 0) {
#if hpux
			char *s = "";
#endif
			dkmgrmsg.m_chan = -errno;
#if hpux
			switch (errno - 700) {
				case -1:
					s = "Control channel has been reset";
					break;
				case -2:
					s ="Control channel is in LINGR state";
					break;
				case -3:
					s = "Can't open control channel";
					break;
				case -4:
					s = "Can't post a read on control channel";
					break;
				case -5:
					s = "Can't send Host Reset to VCS";
					break;
			}
			if (s != "")
				sprintf(dkmgribuf, "dkmgr: %s", s);
			else
#endif
			sprintf(dkmgribuf, "dkmgr: Can't open %s", dial_dev);
			dkmgrmsg.m_errmsg = dkmgribuf;
			sleep(5);
			return(&dkmgrmsg);
		}
		/*
		 * Botch: req_traffic and req_error share the same
		 * field and sometimes req_error isn't set on
		 * error returns.
		 */
		iocb.req_traffic = 066; /* just give a large number */
		iocb.req_error = 066;	/* in case this field gets moved */

#if hpux
		if (write(dksrvfd, server, strlen(server)) < 0) {
			dkmgrmsg.m_chan = -errno;
			sprintf(dkmgribuf, "dkmgr: Can't write server name %s %s"
				, server, sys_errlist[errno]);
			dkmgrmsg.m_errmsg = dkmgribuf;
			goto closesrv;
		}		 
#else
		write(dksrvfd, server, strlen(server)) ;
#endif

		if (ioctl(dksrvfd, DKIOCNEW, &iocb) < 0) {
			/* This error code is always returned on the 
			   first request to the interface; ignore it */
			if (errno == 99)
				dkmgrmsg.m_chan = 0 ;
			else {
				dkmgrmsg.m_chan = -errno;
				if(iocb.req_error == 066)
					sprintf(dkmgribuf, "dkmgr: Unable to contact Datakit for server %s -- %s",
					    server, sys_errlist[errno]);
				else
#if hpux
					sprintf(dkmgribuf, "dkmgr: Unable to create server %s -- %s [%s]",
					    server, dkerr(iocb.req_error), sys_errlist[errno]);
#else
					sprintf(dkmgribuf, "dkmgr: Unable to create server %s -- %s",
					    server, dkerr(iocb.req_error));
#endif
				dkmgrmsg.m_errmsg = dkmgribuf;
			}

#if hpux
	closesrv:
#endif
			/* Don't get stuck on the close */

			sigwas = signal(SIGALRM, timeup);
			timewas = alarm(5);

			if(!setjmp(noluck))
				close(dksrvfd);

			alarm(timewas);
			signal(SIGALRM, sigwas);

			return(&dkmgrmsg);
		}
		ioctl(dksrvfd, DIOCEXCL, 0) ;
		iocb.req_driver = DKR_BLOCK ;
		ioctl(dksrvfd, DIOCRMODE, &iocb) ;
		/* Now get the # chans per iface returned in iocb struct*/
		if (ioctl(dksrvfd, DIOCINFO, &iocb) < 0)
		{
			hchan = 64;
			fprintf(logf, "dkmgr: ioctl failed; defaulting chans per intfc to %d\n", hchan);
		}
		else
		{	hchan = iocb.req_chans;
			if (loglvl >= 8) 
				fprintf(logf, "dkmgr: chans per intfc=%d\n",
					hchan);
		}
		dkmgropen = dksrvfd ;
		fprintf(logf, "dkmgr: SERVER %s is ACTIVE and SERVING\n", server);
		fflush(logf);
	}
again:
	errno = 0 ;
	if ((n = read(dksrvfd, dkmgribuf, sizeof(dkmgribuf))) <= 0) {
		if (n == 0) {
			close(dksrvfd) ;		/* a host restart */
			dkmgropen = 0 ;
#if hpux
#endif
			sprintf(dkmgribuf, "dkmgr: Server %s end of file, restarting", server);
			dkmgrmsg.m_errmsg = dkmgribuf;
			return(&dkmgrmsg);
		}else if(n < 0){
			if(errno == EINTR)
				dkmgrmsg.m_chan = 0;
			else{
				dkmgrmsg.m_chan = -errno;
				sprintf(dkmgribuf, "dkmgr: Server %s read failed -- %s",
				    server, sys_errlist[errno]);
				dkmgrmsg.m_errmsg = dkmgribuf;
			}
			return(&dkmgrmsg);
		}
	}
	dkmgribuf[n] = '\0' ;
	if (loglvl >= 9) {
		fwrite(dkmgribuf, n + 1, 1, logf);
		putc('\n', logf);
	}
	memset(&dkmgrmsg, '\0', sizeof(struct mgrmsg));
	if ((m_type = dkmformat(dkmgribuf)) == 0) {
		sprintf(dkmgribuf,"dkmgr: cannot type message\n");
		dkmgrmsg.m_errmsg = dkmgribuf;
	}
	else if (dkmexec(m_type, dkmgribuf) < 0)
		goto again;	/* duplicate message from controller */
	dkmgrmsg.m_chan += (intf * hchan); /* convert to logical channel */
	if (loglvl >= 8) {
		fprintf(logf, "dkmgr: M=%d,C=%d,T=%u,D=%s,U=%s,R=%s,L=%s\n",
		m_type, dkmgrmsg.m_chan, dkmgrmsg.m_tstamp,
		dkmgrmsg.m_dial,
		dkmgrmsg.m_uid?dkmgrmsg.m_uid:"", dkmgrmsg.m_source,
		dkmgrmsg.m_lname?dkmgrmsg.m_lname:"");
		fflush(logf);
	}
	return(&dkmgrmsg);
}

dkmgrack(chan)
{
	dkmgrnak(chan, 0);
}

dkmgrnak(chan, error)
{
	extern int nodetype;
	extern unsigned short dkmgrstamp[];
	chan %= hchan;
	if (nodetype == G1_NODE) {
		char omsg[15];

		sprintf(omsg, "%u.%u.%u", chan, dkmgrstamp[chan], error);
		write(dkmgropen, omsg, strlen(omsg));
	}
	else {
		char omsg[5] ;
		short odata[2] ;

		odata[0] = chan ;
		odata[1] = dkmgrstamp[chan] ;
		dktcanon("ss", odata, omsg) ;
		omsg[4] = error ;
		write(dkmgropen, omsg, sizeof omsg) ;
	}
}

static SIGRTN
timeup()
{
	longjmp(noluck, 1);
}


/*
*	Classify the incoming server request message into 
*	one of five types:
*	1 - From BLX to BLX Node
*		(chan)(token)dialstring\n
*		origin.uid\n
*		trunk.traffic
*	2 - From BLX to BLX Node  (with G1.4 Toll Node)
*		(chan)(token)dialstring\n
*		trunk.traffic
*	3 - From G1.4 to BLX Node
*		(chan)(token)dialstring\n
*		uid\n
*		origin.node.mod.ochan.cflag\n
*		trunk.traffic
*	4 - From BLX to G1.4 Node
*		chan.token.lflag\n
*		dialstring\n	\n and
*		origin.uid  	origin may be omitted
*	5 - From G1.4 to G1.4 Node
*		chan.token.lflag\n
*		dialstring\n
*		uid\n
*		origin.node.mod.ochan.cflag
*	Return message format type or zero (error)
*/
static
dkmformat(msg)
register char *msg;
{
	extern int nodetype;
	char *strchr();
	register char *cp;
	int nl_count = 0;

	/* message types 1, 2 and 3 begin with channel number in (low)(high)
	format i.e. high byte is always zero for channels < 256 */

	if (msg[1] == '\0') {
		nodetype = BLX_NODE;
		/* count up the \n characters in message */
		cp = msg + 4; /* bypass the channel & time stamp */
		while (cp = strchr(cp, '\n')) {
			nl_count++;
			cp++;
		}
		if (nl_count == 1)
			return(3);
		else if (nl_count == 2)
			return(1);
		else if (nl_count == 3)
			return(2);
		else return(0);
	}
	else {
		/* just count up the \n characters */
		nodetype = G1_NODE;
		cp = msg;
		while (cp = strchr(cp, '\n')) {
			nl_count++;
			cp++;
		}
		if (nl_count < 3 ) return(4);
		else if (nl_count == 3) return(5);
		else return(0);
	}
}

/*
* Decision Table Processor to Process The 8 Possible Line Formats
* which are found in incoming service messages:
*	1 (Chan)(Token)Dialstring
*	2 Origin.UID
*	3 Trunk.Traffic
*	4 UID.
*	5 Origin.Node.Module.Ochan.Cflag
*	6 Chan.Token.Lflag
*	7 Dialstring
*	8 Insert Default Trunk Name for Remote BLX Terminals
* Each "rule" parses a line until '\n' or '\0' (unless indicated)
* Each rule returns the address of where the next rule
* is to begin, NULL (end of parse) or -1 (duplicate message to be discarded)
*/

static
dkmexec(m_type, msg)
register char *msg;
{
	char *dkmru01(), *dkmru02(), *dkmru03(), *dkmru04(),
	*dkmru05(), *dkmru06(), *dkmru07(), *dkmru08();

	/* Each Row in the rule array represents the order
	* in which lines are positioned in a message
	*/

	static char * (*rule_array [] [5]) () = {
		{ dkmru01, dkmru02, dkmru03, 0 },
		{ dkmru01, dkmru04, dkmru05, dkmru03, 0 },
		{ dkmru01, dkmru03, 0 },
		{ dkmru08, dkmru06, dkmru07, dkmru02, 0 },
		{ dkmru06, dkmru07, dkmru04, dkmru05, 0 }
	};
	register char * (**path) ();
	register char *cp = msg;
	path = rule_array[m_type - 1];
#if hpux
	/*
	 * this is a workaroubd for a compiler bug.
	 */
	while (1) {
		if (**path) {
			if (cp) {
				cp = (**path) (cp);
				path++;
				if ((int) cp == -1) 
					return(-1);
			} else	break;
		} else break;
	}
#else
	while ( **path && cp) {
		cp = (**path) (cp);
		path++;
		if ((int) cp == -1) return(-1);
	}
#endif
	return(0);
}

/* Parse line of the form (Chan)(Token)Dialstring\n */

static char *
dkmru01(cp)
register char *cp;
{
	extern struct mgrmsg dkmgrmsg;
	extern unsigned short dkmgrstamp[NCHAN];
	extern char dkmgribuf[];

	dkfcanon("ss", cp, &dkmgrmsg);
	if((dkmgrmsg.m_chan < 2) || (dkmgrmsg.m_chan >= NCHAN)){
		sprintf(dkmgribuf, "dkmgr: Bad channel number (%d) on request", dkmgrmsg.m_chan);
		dkmgrmsg.m_chan = 0;
		dkmgrmsg.m_errmsg = dkmgribuf;
		return(NULL);
	}
	if (dkmgrstamp[dkmgrmsg.m_chan] == dkmgrmsg.m_tstamp)
		return((char *) -1);
	else
		dkmgrstamp[dkmgrmsg.m_chan] = dkmgrmsg.m_tstamp ;
	cp += 4;	/* point past the chan and time-stamp */

	/* remaining fields are dial.service.protocol.parm */

	dkmgrmsg.m_dial = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_service = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}

	/* protocol may have /origtype in it */

	dkmgrmsg.m_protocol = cp;
	cp = strpbrk(cp, "./\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	if (*cp == '/') {
		*cp = 0;
		dkmgrmsg.m_origtype = ++cp;
		cp = strpbrk(cp, ".\n");
		if (cp == 0) return(NULL);
		if (*cp == '\n') {
			*cp = 0;
			return(cp +1);
		}
		else {
			*cp = 0;
			cp++;
		}
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_parm = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	cp = strchr(cp, '\n');
	if (cp) return(cp+1);
	else return(NULL);
}

/* Parse line of the form Origin.UID\n */

static char *
dkmru02(cp)
register char *cp;
{
	extern struct mgrmsg dkmgrmsg;
	char *dkmuidfix();

	dkmgrmsg.m_source = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_uid = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) {
		dkmgrmsg.m_uid = dkmuidfix(dkmgrmsg.m_uid);
		return(NULL);
	}
	if (*cp == '\n') {
		*cp = 0;
		dkmgrmsg.m_uid = dkmuidfix(dkmgrmsg.m_uid);
		return(cp + 1);
	}
	else {
		*cp = 0;
		dkmgrmsg.m_uid = dkmuidfix(dkmgrmsg.m_uid);
		cp++;
	}
	cp = strchr(cp, '\n');
	if (cp) return(cp+1);
	else return(NULL);
}

/*
 * Stick a '0' in front of old-style all-numeric
 * uid's to signify they are really octal so
 * the "*n" format will convert properly.
 */

#define UID_SIZE 64
static char *
dkmuidfix(uid)
register char *uid;
{
	static char new_uid[UID_SIZE];
	if (strcspn(uid, "01234567") > 0)
		return(uid);		/* can't be octal */
	if (*uid == '0')
		return(uid);		/* already is octal */
	if (strcspn(uid, "01234567") > 0)
	if (strlen(uid) > UID_SIZE - 2)
		return(uid);		/* too big for me */
	strcpy(new_uid, "0");		/* start with '0' */
	strcat(new_uid, uid);
	return(new_uid);
}

/* Parse line of the form Trunk.Traffic\n */

static char *
dkmru03(cp)
register char *cp;
{
	extern struct mgrmsg dkmgrmsg;

	dkmgrmsg.m_lname = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	cp = strchr(cp, '\n');
	if (cp) return(cp+1);
	else return(NULL);
}

/* Parse line of the form UID.\n */

static char *
dkmru04(cp)
register char *cp;
{
	extern struct mgrmsg dkmgrmsg;

	if (*cp == '\n' || *cp == '\0')
		dkmgrmsg.m_uid = NULL;
	else
		dkmgrmsg.m_uid = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	cp = strchr(cp, '\n');
	if (cp) return(cp+1);
	else return(NULL);
}

/* Parse line of the form Origin.Node.Module.Ochan.Cflag */ 

static char *
dkmru05(cp)
register char *cp;
{
	extern struct mgrmsg dkmgrmsg;

	dkmgrmsg.m_source = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_srcnode = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_srcmod = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_srcchan = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_cflag = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	cp = strchr(cp, '\n');
	if (cp) return(cp+1);
	else return(NULL);
}

/* Parse line of the form Chan.Token.Lflag */

static char *
dkmru06(cp)
register char *cp;
{
	extern struct mgrmsg dkmgrmsg;
	extern unsigned short dkmgrstamp[];
	extern char dkmgribuf[];

	dkmgrmsg.m_chan = (short) atoi(cp);
	if((dkmgrmsg.m_chan < 2) || (dkmgrmsg.m_chan >= NCHAN)){
		sprintf(dkmgribuf, "dkmgr: Bad channel number (%d) on request", dkmgrmsg.m_chan);
		dkmgrmsg.m_chan = 0;
		dkmgrmsg.m_errmsg = dkmgribuf;
		return(NULL);
	}
	cp = strpbrk(cp, ".");
	if (cp == 0) return(NULL);
	cp++;
	dkmgrmsg.m_tstamp = atoi(cp);
	if (dkmgrstamp[dkmgrmsg.m_chan] == dkmgrmsg.m_tstamp)
		return((char *) -1);
	else
		dkmgrstamp[dkmgrmsg.m_chan] = dkmgrmsg.m_tstamp ;
	dkmgrmsg.m_lflag = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	cp = strchr(cp, '\n');
	if (cp) return(cp+1);
	else return(NULL);
}

/* Parse line of the form Dialstring\n */

static char *
dkmru07(cp)
register char *cp;
{
	extern struct mgrmsg dkmgrmsg;

	/* fields are dial.service.protocol.parm */

	dkmgrmsg.m_dial = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_service = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}

	/* protocol may have /origtype in it */
	dkmgrmsg.m_protocol = cp;
	cp = strpbrk(cp, "./\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	if (*cp == '/') {
		*cp = 0;
		dkmgrmsg.m_origtype = ++cp;
		cp = strpbrk(cp, ".\n");
		if (cp == 0) return(NULL);
		if (*cp == '\n') {
			*cp = 0;
			return(cp +1);
		}
		else {
			*cp = 0;
			cp++;
		}
	}
	else {
		*cp = 0;
		cp++;
	}
	dkmgrmsg.m_parm = cp;
	cp = strpbrk(cp, ".\n");
	if (cp == 0) return(NULL);
	if (*cp == '\n') {
		*cp = 0;
		return(cp + 1);
	}
	else {
		*cp = 0;
		cp++;
	}
	cp = strchr(cp, '\n');
	if (cp) return(cp+1);
	else return(NULL);
}

/* Insert Default Name for Remote BLX Terminals 
*  This is a kludge routine which sets m_source so the server
*  will be happy; no fields are parsed
*/

static char *
dkmru08(cp)
register char *cp;
{
	extern struct mgrmsg dkmgrmsg;

	dkmgrmsg.m_source = "BLX-Node";
	return(cp);
}
