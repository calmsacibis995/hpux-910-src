/* @(#) $Revision: 66.1 $ */      
#

#include "rcv.h"

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 17	/* set number */
#endif NLS

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Give names to all the temporary files that we will need.
 */


char	tempMail[256];
char	tempQuit[256];
char	tempEdit[256];
char	tempSet[256];
char	tempResid[256];
char	tempMesg[256];
char	tempZedit[256];

tinit()
{
	register char *cp, *cp2;
	char uname[PATHSIZE];
	char *Temp = '\0';
	register int err = 0;
	pid_t pid;
	char tbuf[PATHSIZE];

	pid = getpid();
	Temp = value("TMPDIR");
	if (Temp == NOSTR || Temp == '\0')
		Temp = TMPDIR;
	sprintf(tempMail, "%s/Rs%-d", Temp, pid);
	sprintf(tempResid, "%s/Rq%-d", Temp, pid);
	sprintf(tempQuit, "%s/Rm%-d", Temp, pid);
	sprintf(tempEdit, "%s/Re%-d", Temp, pid);
	sprintf(tempSet, "%s/Rx%-d", Temp, pid);
	sprintf(tempMesg, "%s/Rx%-d", Temp, pid);
	sprintf(tempZedit, "%s/Rz%-d", Temp, pid);

	if (strlen(myname) != 0) {
		uid = getuserid(myname);
		if (uid == -1) {
			printf((catgets(nl_fn,NL_SETN,1, "\"%s\" is not a user of this system\n")),
			    myname);
			exit(1);
		}
	}
	else {
		uid = getuid() & UIDMASK;
		if (username(uid, uname) < 0) {
			copy((catgets(nl_fn,NL_SETN,2, "ubluit")), myname);
			err++;
			if (rcvmode) {
				printf((catgets(nl_fn,NL_SETN,3, "Who are you!?\n")));
				exit(1);
			}
		}
		else
			copy(uname, myname);
	}
	strcpy(homedir, Getf("HOME"));
	findmail();
	assign("MBOX", Getf("MBOX"));
	assign("MAILRC", Getf("MAILRC"));
	assign("DEAD", Getf("DEAD"));
	assign("save", "");
	assign("asksub", "");
	assign("header", "");
}
