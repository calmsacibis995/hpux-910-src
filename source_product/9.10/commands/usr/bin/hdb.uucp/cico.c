static char *RCS_ID="@(#)$Revision: 72.1 $ $Date: 92/10/29 16:31:46 $";
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1
#include <nl_types.h>
#endif NLS

/*	@(#) $Revision: 72.1 $	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	

 * uucp file transfer program:
 * to place a call to a remote machine, login, and
 * copy files between the two machines.

*/
#include "uucp.h"
#include "uust.h"
#include "uusub.h"

#ifndef	V7
#include <sys/sysmacros.h>
#endif

#ifdef NOTIMER
setjmp(x)
int x;
{return 0;};

alarm(x)
int x;
{return 0;};
#endif

#define NOT_VALID_NAME	"NOT_VALID_HOSTNAME"

jmp_buf Sjbuf;
extern int Errorrate;
char	uuxqtarg[MAXBASENAME] = {'\0'};

#define USAGE	strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,85, "[-xNUM] [-r[0|1]] -sSYSTEM")))

extern void closedem();

main(argc, argv, envp)
char *argv[];
char **envp;
{
	extern onintr(), timeout();
	extern intrEXIT();
	extern char *pskip();
	int ret, seq, exitcode;
	char file[NAMESIZE];
	char msg[BUFSIZ], *p, *q;
	char xflag[6];	/* -xN N is single digit */
	char *ttyn;
	char	cb[128];
        int uucpuid;
	char dpath[BUFSIZ];
        int err;
	time_t	ts, tconv;
	char *Not_Valid_Char = NULL;

#ifdef NLS
        nlmsg_fd = catopen("uucp",0);
#endif NLS
	Uid = getuid();
	Euid = geteuid();	/* this should be UUCPUID */
	if (Uid == 0){
            err = gninfo("uucp",&uucpuid,dpath);  
            if (err  == FAIL) uucpuid = UUCPUID;
	    setuid(uucpuid);	/* fails in ATTSV, but so what? */
        }
	Env = envp;
	Role = SLAVE;
	strcpy(Logfile, LOGCICO);
	*Rmtname = NULLCHAR;

	closedem();
	time(&Nstat.t_qtime);
	tconv = Nstat.t_start = Nstat.t_qtime;
	strcpy(Progname, "uucico");
	Pchar = 'C';
	(void) signal(SIGILL, intrEXIT);
	(void) signal(SIGTRAP, intrEXIT);
	(void) signal(SIGIOT, intrEXIT);
	(void) signal(SIGEMT, intrEXIT);
	(void) signal(SIGFPE, intrEXIT);
	(void) signal(SIGBUS, intrEXIT);
	(void) signal(SIGSEGV, intrEXIT);
	(void) signal(SIGSYS, intrEXIT);
	if (signal(SIGPIPE, SIG_IGN) != SIG_IGN)	/* This for sockets */
		(void) signal(SIGPIPE, intrEXIT);
	(void) signal(SIGINT, onintr);
	(void) signal(SIGHUP, onintr);
	(void) signal(SIGQUIT, onintr);
	(void) signal(SIGTERM, onintr);
#ifdef ATTSV
	(void) signal(SIGUSR1, SIG_IGN);
	(void) signal(SIGUSR2, SIG_IGN);
#endif
	ret = guinfo(Euid, User);
	ASSERT(ret == 0, "BAD UID ", "", ret);
	strncpy(Uucp, User, NAMESIZE);

	setuucp(User);
	ret = guinfo(Uid, Loginuser);
	ASSERT(ret == 0, "BAD LOGIN_UID ", "", ret);

	*xflag = NULLCHAR;
	Ifn = Ofn = -1;
	while ((ret = getopt(argc, argv, "d:r:s:x:")) != EOF) {
		switch (ret) {
		case 'd':
			Spool = optarg;
			break;
		case 'r':
			Role = atoi(optarg);
			break;
		case 's':
			if (versys(optarg)) {
			    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,88, "%s not in Systems file\n")), optarg);
			    cleanup(101);
			}
			strncpy(Rmtname, optarg, MAXBASENAME);
			Rmtname[MAXBASENAME] = '\0';
			/* set args for possible xuuxqt call */
			strcpy(uuxqtarg, Rmtname);
			break;
		case 'x':
			Debug = atoi(optarg);
			if (Debug <= 0)
				Debug = 1;
			if (Debug > 9)
				Debug = 9;
			/* Only Uid==0 can do 5 and above due to passwd display */
			if ((Debug > 4) && (Uid != 0))
				Debug = 4;
			(void) sprintf(xflag, "-x%d", Debug);
			break;
		default:
			(void) fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,89, "\tusage: %s %s\n"))),
			    Progname, USAGE);
			exit(1);
		}
	}

	if (Role == MASTER) {
	    if (*Rmtname == NULLCHAR) {
		DEBUG(5, (catgets(nlmsg_fd,NL_SETN,90, "No -s specified\n")) , "");
		cleanup(101);
	    }
	    /* get Myname - it depends on who I'm calling--Rmtname */
	    (void) mchFind(Rmtname);
	    myName(Myname);
	    if (EQUALSN(Rmtname, Myname, SYSNSIZE)) {
		DEBUG(5, (catgets(nlmsg_fd,NL_SETN,91, "This system specified: -sMyname: %s, ")), Myname);
		cleanup(101);
	    }
	}

	ASSERT(chdir(Spool) == 0, Ct_CHDIR, Spool, errno);
	strcpy(Wrkdir, Spool);

	if (Role == SLAVE) {

#ifdef RAYDEBUG
     for (;;);
#endif
		/*
		 * initial handshake
		 */
		ret = savline();
		Ifn = 0;
		Ofn = 1;
		fixline(Ifn, 0, D_ACU);
		freopen(RMTDEBUG, "a", stderr);
		/* get MyName - use logFind to check PERMISSIONS file */
		(void) logFind(Loginuser, "");
		myName(Myname);

		DEBUG(4,(catgets(nlmsg_fd,NL_SETN,92, "cico.c: Myname - %s\n")),Myname);
		DEBUG(4,(catgets(nlmsg_fd,NL_SETN,93, "cico.c: Loginuser - %s\n")),Loginuser);
		Nstat.t_scall = times(&Nstat.t_tga);
		(void) sprintf(msg, "here=%s", Myname);
		omsg('S', msg, Ofn);
		(void) signal(SIGALRM, timeout);
		(void) alarm(2 * MAXMSGTIME);	/* give slow machines a second chance */
		if (setjmp(Sjbuf)) {

			/*
			 * timed out
			 */
			ret = restline();
			rmlock(CNULL);
			exit(0);
		}
		for (;;) {
			ret = imsg(msg, Ifn);
			if (ret != 0) {
				(void) alarm(0);
				ret = restline();
				rmlock(CNULL);
				exit(0);
			}
			if (msg[0] == 'S')
				break;
		}
		Nstat.t_ecall = times(&Nstat.t_tga);
		(void) alarm(0);
		q = &msg[1];
		p = pskip(q);
		strncpy(Rmtname, q, MAXBASENAME);
		Rmtname[MAXBASENAME] = '\0';

		seq = 0;
		while (*p == '-') {
			q = pskip(p);
			switch(*(++p)) {
			case 'x':
				Debug = atoi(++p);
				if (Debug <= 0)
					Debug = 1;
				break;
			case 'Q':
				seq = atoi(++p);
				break;
			default:
				break;
			}
			p = q;
		}
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,95, "sys-%s\n")), Rmtname);

#ifdef NOSTRANGERS
/* here's the place to look the remote system up in the Systems file.
 * If the command NOSTRANGERS (/usr/lib/uucp/remote.unknown) is executable and 
/* If they're not in my file then hang up */
		if ( (access(NOSTRANGERS, 1) == 0) && versys(Rmtname)) {
		    char unkcmd[64];

		    omsg('R',  "You are unknown to me", Ofn);

/* Fix for defect DSDe408240
/* Problem: If the nuucp password is given to a user, and
/*          /usr/lib/uucp/remote.unknown is executable on remote host, then
/*	    the user could do the following:
/*	        rlogin nuucp
/*		Password: xxxx (enter the known/given passwd)
/*	    then the user would end up with 
/*		Shere=SYSTEM_NAME
/*	    at this point the user could do the following:
/*		<CTRL-p>Shere=;sh<CTRL-j>
/*	    which would enable a shell for the user, and 
/*	    here after any command can be run followed be <CTRL-j>
/*	    (i.e. vi /usr/lib/uucp/Systems<CTRL-j>).
/*    NOTE: By doing <CTRL-p>Shere=;sh<CTRL-j> the content of &msg[1] 
/*          (and hence Rmtname) would be here=;sh
/*          hint: system(<any thing>;sh) will execute <any thing>
/*                followed by sh
/*
/* Solution: Check Rmtname for following char.:
/*			& ; | < > ' " 
/*           and close the security hole!
*/
		    Not_Valid_Char = strchr(Rmtname,';');
		    if (Not_Valid_Char != NULL) {
			(void) sprintf(unkcmd, "%s %s", NOSTRANGERS, NOT_VALID_NAME);
			system(unkcmd);
			cleanup(101);
		    }
		    Not_Valid_Char = strchr(Rmtname,'&');
		    if (Not_Valid_Char != NULL) {
			(void) sprintf(unkcmd, "%s %s", NOSTRANGERS, NOT_VALID_NAME);
			system(unkcmd);
			cleanup(101);
		    }
		    Not_Valid_Char = strchr(Rmtname,'|');
		    if (Not_Valid_Char != NULL) {
			(void) sprintf(unkcmd, "%s %s", NOSTRANGERS, NOT_VALID_NAME);
			system(unkcmd);
			cleanup(101);
		    }
		    Not_Valid_Char = strchr(Rmtname,'<');
		    if (Not_Valid_Char != NULL) {
			(void) sprintf(unkcmd, "%s %s", NOSTRANGERS, NOT_VALID_NAME);
			system(unkcmd);
			cleanup(101);
		    }
		    Not_Valid_Char = strchr(Rmtname,'>');
		    if (Not_Valid_Char != NULL) {
			(void) sprintf(unkcmd, "%s %s", NOSTRANGERS, NOT_VALID_NAME);
			system(unkcmd);
			cleanup(101);
		    }
		    Not_Valid_Char = strchr(Rmtname,'\'');
		    if (Not_Valid_Char != NULL) {
			(void) sprintf(unkcmd, "%s %s", NOSTRANGERS, NOT_VALID_NAME);
			system(unkcmd);
			cleanup(101);
		    }
		    Not_Valid_Char = strchr(Rmtname,'"');
		    if (Not_Valid_Char != NULL) {
			(void) sprintf(unkcmd, "%s %s", NOSTRANGERS, NOT_VALID_NAME);
			system(unkcmd);
			cleanup(101);
		    }
		    (void) sprintf(unkcmd, "%s %s", NOSTRANGERS, Rmtname);

		    system(unkcmd);
		    cleanup(101);
		}
#endif NOSTRANGERS

		if (mlock(Rmtname)) {
			omsg('R', "LCK", Ofn);
			cleanup(101);
		}
		
		/* validate login using PERMISSIONS file */
		if (logFind(Loginuser, Rmtname) == FAIL) {
			Uerror = SS_BAD_LOG_MCH;
			logent(UERRORTEXT, "FAILED");
			systat(Rmtname, SS_BAD_LOG_MCH, UERRORTEXT,
			    Retrytime);
			omsg('R', "LOGIN", Ofn);
			cleanup(101);
		}

		ret = callBack();
		DEBUG(4,(catgets(nlmsg_fd,NL_SETN,98, "return from callcheck: %s")),ret ? "TRUE" : "FALSE");
		if (ret==TRUE) {
			(void) signal(SIGINT, SIG_IGN);
			(void) signal(SIGHUP, SIG_IGN);
			omsg('R', "CB", Ofn);
			logent("CALLBACK", "REQUIRED");

			/*
			 * set up for call back
			 */
			systat(Rmtname, SS_CALLBACK, "CALL BACK", Retrytime);
			gename(CMDPRE, Rmtname, 'C', file);
			(void) close(creat(file, CFILEMODE));
                        wfcommit(file,file,Rmtname); /* Fix to AT&T bug */
			xuucico(Rmtname);
			cleanup(101);
		}

		if (callok(Rmtname) == SS_SEQBAD) {
			Uerror = SS_SEQBAD;
			logent(UERRORTEXT, "PREVIOUS");
			omsg('R',  "BADSEQ", Ofn);
			cleanup(101);
		}

		if ((ret = gnxseq(Rmtname)) == seq) {
			omsg('R', "OK", Ofn);
			cmtseq();
		} else {
			Uerror = SS_SEQBAD;
			systat(Rmtname, SS_SEQBAD, UERRORTEXT, Retrytime);
			logent(UERRORTEXT, "HANDSHAKE FAILED");
			ulkseq();
			omsg('R', "BADSEQ", Ofn);
			cleanup(101);
		}
		ttyn = ttyname(Ifn);
		if (ttyn != NULL) {
			strcpy(Dc, BASENAME(ttyn, '/'));
			chmod(ttyn, 0666);	/* can fail, but who cares? */
		} else
			strcpy(Dc, "notty");
		/* set args for possible xuuxqt call */
		strcpy(uuxqtarg, Rmtname);
	}

	strcpy(User, Uucp);
	if (Role == MASTER && callok(Rmtname) != 0) {
		logent("SYSTEM STATUS", "CAN NOT CALL");
		cleanup(101);
	}

	chremdir(Rmtname);

	(void) strcpy(Wrkdir, RemSpool);
	if (Role == MASTER) {

		/*
		 * master part
		 */
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
		if (Ifn != -1 && Role == MASTER) {
			(void) write(Ofn, EOTMSG, strlen(EOTMSG));
			(void) close(Ofn);
			(void) close(Ifn);
			Ifn = Ofn = -1;
			rmlock(CNULL);
			sleep(3);
		}
		(void) sprintf(msg, "call to %s ", Rmtname);
		if (mlock(Rmtname) != 0) {
			logent(msg, "LOCKED");
			CDEBUG(1, (catgets(nlmsg_fd,NL_SETN,109, "Currently Talking With %s\n")),
			    Rmtname);
 			cleanup(100);
		}
		Nstat.t_scall = times(&Nstat.t_tga);
		Ofn = Ifn = conn(Rmtname);
		Nstat.t_ecall = times(&Nstat.t_tga);
		if (Ofn < 0) {
			delock(Rmtname);
			logent(UERRORTEXT, "CONN FAILED");
                        UB_SST(-Ofn); /* for uusub */
			systat(Rmtname, Uerror, UERRORTEXT, Retrytime);
			cleanup(101);
		} else {
			logent(msg, "SUCCEEDED");
                        UB_SST(ub_ok); /* for uusub */
		}
	
		if (setjmp(Sjbuf)) {
			delock(Rmtname);
			Uerror = SS_LOGIN_FAILED;
			logent(Rmtname, UERRORTEXT);
                        UB_SST(US_S_LOGIN);
			systat(Rmtname, SS_LOGIN_FAILED,
			    UERRORTEXT, Retrytime);
			DEBUG(4, (catgets(nlmsg_fd,NL_SETN,112, "%s - failed\n")), UERRORTEXT);
			cleanup(101);
		}
		(void) signal(SIGALRM, timeout);
		/* give slow guys lots of time to thrash */
		(void) alarm(3 * MAXMSGTIME);
		for (;;) {
			ret = imsg(msg, Ifn);
			if (ret != 0) {
				continue; /* try again */
			}
			if (msg[0] == 'S')
				break;
		}
		(void) alarm(0);
		if(EQUALSN("here=", &msg[1], 5)){
			/*
			/* this is a problem.  We'd like to compare with an
			 * untruncated Rmtname but we fear incompatability.
			 * So we'll look at most 6 chars (at most).
			 */
			if(!EQUALSN(&msg[6], Rmtname, (strlen(Rmtname)< 7 ?
						strlen(Rmtname) : 6))){
				delock(Rmtname);
				Uerror = SS_WRONG_MCH;
				logent(&msg[6], UERRORTEXT);
                                UB_SST(US_S_LOGIN);
				systat(Rmtname, SS_WRONG_MCH, UERRORTEXT,
				     Retrytime);
				DEBUG(4, (catgets(nlmsg_fd,NL_SETN,113, "%s - failed\n")), UERRORTEXT);
				cleanup(101);
			}
		}
		CDEBUG(1,(catgets(nlmsg_fd,NL_SETN,114, "Login Successful: System=%s\n")),&msg[6]);
		seq = gnxseq(Rmtname);
		(void) sprintf(msg, "%s -Q%d %s", Myname, seq, xflag);
		omsg('S', msg, Ofn);
		(void) alarm(2 * MAXMSGTIME);	/* give slow guys some thrash time */
		for (;;) {
			ret = imsg(msg, Ifn);
			DEBUG(4, (catgets(nlmsg_fd,NL_SETN,115, "msg-%s\n")), msg);
			if (ret != 0) {
				(void) alarm(0);
				delock(Rmtname);
				ulkseq();
				cleanup(101);
			}
			if (msg[0] == 'R')
				break;
		}
		(void) alarm(0);

		/*  check for rejects from remote */
		Uerror = 0;
		if (EQUALS(&msg[1], "LCK")) 
			Uerror = SS_RLOCKED;
		else if (EQUALS(&msg[1], "LOGIN"))
			Uerror = SS_RLOGIN;
		else if (EQUALS(&msg[1], "CB"))
			Uerror = SS_CALLBACK;
		else if (EQUALS(&msg[1], "You are unknown to me"))
			Uerror = SS_RUNKNOWN;
		else if (EQUALS(&msg[1], "BADSEQ"))
			Uerror = SS_SEQBAD;
		else if (!EQUALS(&msg[1], "OK"))
			Uerror = SS_UNKNOWN_RESPONSE;
		if (Uerror)  {
			delock(Rmtname);
			systat(Rmtname, Uerror, UERRORTEXT, Retrytime);
			logent(UERRORTEXT, "HANDSHAKE FAILED");
			CDEBUG(1, (catgets(nlmsg_fd,NL_SETN,117, "HANDSHAKE FAILED: %s\n")), UERRORTEXT);
			ulkseq();
			cleanup(101);
		}
		cmtseq();
	}
	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,118, " Rmtname %s, ")), Rmtname);

	strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,119, "Role %s,  ")));
        strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,120, "MASTER")));
        strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,121, "SLAVE")));
	DEBUG(4, msg1, Role ? msg2 : msg3);
	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,122, "Ifn - %d, ")), Ifn);
	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,123, "Loginuser - %s\n")), Loginuser);

	/* alarm/setjmp added here due to experience with uucico
	 * hanging for hours in imsg().
	 */
	if (setjmp(Sjbuf)) {
		delock(Rmtname);
		logent("startup", "TIMEOUT");

		strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,126, "%s - timeout\n")));
                strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,127, "startup")));

		DEBUG(4, msg1,msg2);
		cleanup(101);
	}
	(void) alarm(MAXSTART);
	ret = startup(Role);
	(void) alarm(0);

	if (ret != SUCCESS) {
		delock(Rmtname);
		logent("startup", "FAILED");
		Uerror = SS_STARTUP;
		CDEBUG(1, (catgets(nlmsg_fd,NL_SETN,130, "%s\n")), UERRORTEXT);
		systat(Rmtname, Uerror, UERRORTEXT, Retrytime);
		exitcode = 101;
	} else {
		logent("startup", "OK");
		systat(Rmtname, SS_INPROGRESS, UTEXT(SS_INPROGRESS),Retrytime);
		Nstat.t_sftp = times(&Nstat.t_tga);

		exitcode = cntrl(Role);
		Nstat.t_eftp = times(&Nstat.t_tga);
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,133, "cntrl - %d\n")), exitcode);
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGALRM, timeout);

		if (exitcode == 0) {
			(void) time(&ts);
			(void) sprintf(cb, "conversation complete %s %ld",
				Dc, ts - tconv);
			logent(cb, "OK");
			systat(Rmtname, SS_OK, UTEXT(SS_OK), Retrytime);

		} else {
			logent("conversation complete", "FAILED");
			systat(Rmtname, SS_CONVERSATION,
			    UTEXT(SS_CONVERSATION), Retrytime);
		}
		(void) alarm(2 * MAXMSGTIME);	/* give slow guys some thrash time */
		omsg('O', "OOOOO", Ofn);
		CDEBUG(4, "send OO %d,", ret);
		if (!setjmp(Sjbuf)) {
			for (;;) {
				omsg('O', "OOOOO", Ofn);
				ret = imsg(msg, Ifn);
				if (ret != 0)
					break;
				if (msg[0] == 'O')
					break;
			}
		}
		(void) alarm(0);
	}
	cleanup(exitcode);
        catclose(nlmsg_fd);
}

/*
 * clean and exit with "code" status
 */
cleanup(code)
register int code;
{
	int ret;
	char *ttyn;

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	rmlock(CNULL);
	closedem();
	if (Role == SLAVE) {
		ret = restline();
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,138, "ret restline - %d\n")), ret);
		sethup(0);
		ttyn = ttyname(Ifn);
		if (ttyn != NULL)
			chmod(ttyn, 0666);	/* can fail, but who cares? */
	}
	if (Ofn != -1) {
		if (Role == MASTER)
			(void) write(Ofn, EOTMSG, strlen(EOTMSG));
		(void) close(Ifn);
		(void) close(Ofn);
	}
	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,139, "exit code %d\n")), code);

	strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,140, "Conversation Complete: Status %s\n\n"))) ;
	strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,141, "FAILED"))); 
        strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,142, "SUCCEEDED")));


	CDEBUG(1, msg1, code ? msg2: msg3);

	cleanTM();
	if (code == 0)
		xuuxqt(uuxqtarg);
	exit(code);
}

short TM_cnt = 0;
char TM_name[MAXNAMESIZE];

cleanTM()
{
	register int i;
	char tm_name[MAXNAMESIZE];

	DEBUG(7,(catgets(nlmsg_fd,NL_SETN,143, "TM_cnt: %d\n")),TM_cnt);
	for(i=0; i < TM_cnt; i++) {
                (void) sprintf(tm_name ,TM_name, i);
		DEBUG(7, (catgets(nlmsg_fd,NL_SETN,145, "tm_name: %s\n")), tm_name);
		unlink(tm_name);
	}
}

TMname(file, pnum)
char *file;
{

	(void) sprintf(file, "%s/TM.%.5d.%.3d", RemSpool, pnum, TM_cnt);
	if (TM_cnt == 0)
	    (void) sprintf(TM_name, "%s/TM.%.5d", RemSpool, pnum);
	DEBUG(7, (catgets(nlmsg_fd,NL_SETN,146, "TMname(%s)\n")), file);
	TM_cnt++;
}

/*
 * intrrupt - remove locks and exit
 */
onintr(inter)
register int inter;
{
	char str[30];
	/* I'm putting a test for zero here because I saw it happen
	 * and don't know how or why, but it seemed to then loop
	 * here for ever?
	 */
	if (inter == 0)
	    exit(99);
	(void) signal(inter, SIG_IGN);
	(void) sprintf(str, "SIGNAL %d", inter);
	logent(str, "CAUGHT");
	cleanup(inter);
}

/*ARGSUSED*/
intrEXIT(inter)
{
	char	cb[10];
	extern int errno;

	(void) sprintf(cb, "%d", errno);
	logent("INTREXIT", cb);
	(void) signal(SIGIOT, SIG_DFL);
	(void) signal(SIGILL, SIG_DFL);
	rmlock(CNULL);
	closedem();
	(void) setuid(Uid);
	abort();
}

/*
 * catch SIGALRM routine
 */
timeout()
{
	longjmp(Sjbuf, 1);
}

static char *
pskip(p)
register char *p;
{
	while( *p && *p != ' ' )
		++p;
	if( *p ) *p++ = 0;
	return(p);
}

void
closedem()
{
	register i;

	for(i=getnumfds()-1;i>=3;i--)
#ifdef NLS
		if (i!=nlmsg_fd)	/* skip NLS message catalog */
#endif
		(void) close(i);
}
