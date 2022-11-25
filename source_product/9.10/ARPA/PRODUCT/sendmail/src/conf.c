/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

# ifndef lint
static char rcsid[] = "$Header: conf.c,v 1.17.109.8 95/02/21 16:07:28 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)conf.c	5.26 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: conf.o $Revision: 1.17.109.8 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include <sys/ioctl.h>
# ifndef hpux
# 	include <sys/param.h>
# endif	/* not hpux */
# include <pwd.h>
# include "sendmail.h"
# include "pathnames.h"

/*
**  CONF.C -- Sendmail Configuration Tables.
**
**	Defines the configuration of this installation.
**
**	Compilation Flags:
**		VMUNIX -- running on a Berkeley UNIX system.
**
**	Configuration Variables:
**		HdrInfo -- a table describing well-known header fields.
**			Each entry has the field name and some flags,
**			which are described in sendmail.h.
**
**	Notes:
**		I have tried to put almost all the reasonable
**		configuration information into the configuration
**		file read at runtime.  My intent is that anything
**		here is a function of the version of UNIX you
**		are running, or is really static -- for example
**		the headers are a superset of widely used
**		protocols.  If you find yourself playing with
**		this file too much, you may be making a mistake!
*/




/*
**  Header info table
**	Final (null) entry contains the flags used for any other field.
**
**	Not all of these are actually handled specially by sendmail
**	at this time.  They are included as placeholders, to let
**	you know that "someday" I intend to have sendmail do
**	something with them.
*/

struct hdrinfo	HdrInfo[] =
{
		/* originator fields, most to least significant  */
	"resent-sender",	H_FROM|H_RESENT,
	"resent-from",		H_FROM|H_RESENT,
	"resent-reply-to",	H_FROM|H_RESENT,
	"sender",		H_FROM,
	"from",			H_FROM|H_ACHECK,
	"reply-to",		H_FROM,
	"full-name",		0,
	"return-receipt-to",	H_FROM,
	"errors-to",		H_FROM,
	"return-path",		H_FROM|H_ACHECK,
		/* destination fields */
	"to",			H_RCPT,
	"resent-to",		H_RCPT|H_RESENT,
	"cc",			H_RCPT,
	"resent-cc",		H_RCPT|H_RESENT,
	"bcc",			H_RCPT|H_ACHECK,
	"resent-bcc",		H_RCPT|H_ACHECK|H_RESENT,
	"apparently-to",	H_RCPT,
		/* message identification and control */
	"message-id",		0,
	"resent-message-id",	H_RESENT,
	"message",		H_EOH,
	"text",			H_EOH,
		/* date fields */
	"date",			0,
	"resent-date",		H_RESENT,
		/* trace fields */
	"received",		H_TRACE|H_FORCE,
	"via",			H_TRACE|H_FORCE,
	"mail-from",		H_TRACE|H_FORCE,

	NULL,			0,
};


/*
**  Location of system files/databases/etc.
*/

char	*ConfFile =	_PATH_SENDMAILCF;	/* runtime configuration */
char	*FreezeFile =	_PATH_SENDMAILFC;	/* frozen version of above */


/*
**  Privacy values
*/

struct prival PrivacyValues[] =
{
	"public",		PRIV_PUBLIC,
	"needmailhelo",		PRIV_NEEDMAILHELO,
	"needexpnhelo",		PRIV_NEEDEXPNHELO,
	"needvrfyhelo",		PRIV_NEEDVRFYHELO,
	"noexpn",		PRIV_NOEXPN,
	"novrfy",		PRIV_NOVRFY,
	"restrictmailq",	PRIV_RESTRICTMAILQ,
	"restrictqrun",		PRIV_RESTRICTQRUN,
	"authwarnings",		PRIV_AUTHWARNINGS,
	"goaway",		PRIV_GOAWAY,
	NULL,			0,
};



/*
**  Miscellaneous stuff.
*/

int	DtableSize =	50;		/* max open files; reset in 4.2bsd */
char    *AliasMap;                      /* NIS aliases map */

/*
**  SETDEFAULTS -- set default values
**
**	Because of the way freezing is done, these must be initialized
**	using direct code.
**
**	Parameters:
**		e -- the default envelope.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Initializes a bunch of global variables to their
**		default values.
*/

void setdefaults(e)
	register ENVELOPE *e;
{
	QueueLA = 8;			/* option x */
	QueueFactor = 10000;		/* option q */
	RefuseLA = 12;			/* option X */
	SpaceSub = ' ';			/* option B */
	WkRecipFact = 1000;		/* option y */
	WkClassFact = 1800;		/* option z */
	WkTimeFact = 9000;		/* option Z */
	FileMode = 0600;		/* option F */
	DefUid = 1;			/* option u */
	DefGid = 1;			/* option g */
	CheckpointInterval = 10;	/* option C */
	e->e_sendmode = SM_FORK;	/* option d */
	e->e_errormode = EM_PRINT;	/* option e */
	Bit8Mode = B8_ENCODE;		/* option E */
	AliasMap = ALIAS_MAP;		/* option N */
	MaxMessageSize = 0;		/* option K */
}


/*
**  SETDEFUSER -- set/reset DefUser using DefUid (for initgroups())
*/

setdefuser()
{
	struct passwd *defpwent;

	if (DefUser != NULL)
		free(DefUser);
	if ((defpwent = getpwuid(DefUid)) != NULL)
		DefUser = newstr(defpwent->pw_name);
	else
		DefUser = newstr("nobody");
}


/*
**  GETRUID -- get real user id (V7)
*/

uid_t
getruid()
{
	if (OpMode == MD_DAEMON)
		return (RealUid);
	else
		return (getuid());
}


/*
**  GETRGID -- get real group id (V7).
*/

gid_t
getrgid()
{
	if (OpMode == MD_DAEMON)
		return (RealGid);
	else
		return (getgid());
}

/*
**  USERNAME -- return the user id of the logged in user.
**
**	Parameters:
**		none.
**
**	Returns:
**		The login name of the logged in user.
**
**	Side Effects:
**		none.
**
**	Notes:
**		The return value is statically allocated.
*/

char *
username()
{
	static char *myname = NULL;
	extern char *getlogin();
	register struct passwd *pw;
	extern struct passwd *getpwuid();

	/* cache the result */
	if (myname == NULL)
	{
		myname = getlogin();
		if (myname == NULL || myname[0] == '\0')
		{

			pw = getpwuid(getruid());
			if (pw != NULL)
				myname = newstr(pw->pw_name);
		}
		else
		{

			myname = newstr(myname);
			if ((pw = getpwnam(myname)) == NULL ||
			      getuid() != pw->pw_uid)
			{
				pw = getpwuid(getuid());
				if (pw != NULL)
					myname = newstr(pw->pw_name);
			}
		}
		if (myname == NULL || myname[0] == '\0')
		{
			syserr("554 Who are you?");
			myname = "postmaster";
		}
	}

	return (myname);
}
/*
**  TTYPATH -- Get the path of the user's tty
**
**	Returns the pathname of the user's tty.  Returns NULL if
**	the user is not logged in or if s/he has write permission
**	denied.
**
**	Parameters:
**		none
**
**	Returns:
**		pathname of the user's tty.
**		NULL if not logged in or write permission denied.
**
**	Side Effects:
**		none.
**
**	WARNING:
**		Return value is in a local buffer.
**
**	Called By:
**		savemail
*/

# include <sys/stat.h>

char *
ttypath()
{
	struct stat stbuf;
	register char *pathn;
	extern char *ttyname();
	extern char *getlogin();

	/* compute the pathname of the controlling tty */
	if ((pathn = ttyname(2)) == NULL && (pathn = ttyname(1)) == NULL &&
	    (pathn = ttyname(0)) == NULL)
	{
		errno = 0;
		return (NULL);
	}

	/* see if we have write permission */
	if (stat(pathn, &stbuf) < 0 || !bitset(02, stbuf.st_mode))
	{
		errno = 0;
		return (NULL);
	}

	/* see if the user is logged in */
	if (getlogin() == NULL)
		return (NULL);

	/* looks good */
	return (pathn);
}
/*
**  CHECKCOMPAT -- check for From and To person compatible.
**
**	This routine can be supplied on a per-installation basis
**	to determine whether a person is allowed to send a message.
**	This allows restriction of certain types of internet
**	forwarding or registration of users.
**
**	If the hosts are found to be incompatible, an error
**	message should be given using "usrerr" and non-0 should
**	be returned.
**
**	'NoReturn' can be set to suppress the return-to-sender
**	function; this should be done on huge messages.
**
**	Parameters:
**		to -- the person being sent to.
**
**	Returns:
**		an exit status
**
**	Side Effects:
**		none (unless you include the usrerr stuff)
*/

checkcompat(to, e)
	register ADDRESS *to;
	register ENVELOPE *e;
{
# ifdef lint
	if (to == NULL)
		to++;
# endif /* lint */
# ifdef EXAMPLE_CODE
	/* this code is intended as an example only */
	register STAB *s;

	s = stab("arpa", ST_MAILER, ST_FIND);
	if (s != NULL && e->e_from.q_mailer != LocalMailer &&
	    to->q_mailer == s->s_mailer)
	{
		usrerr("553 No ARPA mail through this machine: see your system administration");
		/* NoReturn = TRUE; to supress return copy */
		return (EX_UNAVAILABLE);
	}
# endif /* EXAMPLE_CODE */
	return (EX_OK);
}

# if 0
/*
**  HOLDSIGS -- arrange to hold all signals
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Arranges that signals are held.
*/

holdsigs()
{
}
/*
**  RLSESIGS -- arrange to release all signals
**
**	This undoes the effect of holdsigs.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Arranges that signals are released.
*/

rlsesigs()
{
}
# endif	/* 0 */
/*
**  GETLA -- get the current load average
**
**	This code stolen from la.c.
**
**	Parameters:
**		none.
**
**	Returns:
**		The current load average as an integer.
**
**	Side Effects:
**		none.
*/

# ifdef PSTAT
# 	include <sys/pstat.h>
# endif /* PSTAT */

# if defined(PSTAT) && defined(PSTAT_SETCMD)

getla()
{
	struct pst_dynamic p;

	if (pstat(PSTAT_DYNAMIC, &p, sizeof(p), 0, 0) < 0)
	    return(0);
	return((int)(p.psd_avg_1_min + 0.5));
}

# else /* defined(PSTAT) && defined(PSTAT_SETCMD) */

# 	ifndef hpux

getla()
{
	double avenrun[3];

	if (getloadavg(avenrun, sizeof(avenrun) / sizeof(avenrun[0])) < 0)
		return (0);
	return ((int) (avenrun[0] + 0.5));
}

# 	else /* hpux */

# 		include <nlist.h>

struct	nlist Nl[] =
{
# 		ifdef hp9000s200
	{ "_avenrun" },
# 		else /* ! hp9000s200 */
	{ "avenrun" },
# 		endif /* hp9000s200 */
# 		define	X_AVENRUN	0
	{ 0 },
};

# 		include <sys/fcntl.h>

getla()
{
	static int kmem = -1;
	double avenrun[3];
	extern off_t lseek();

	if (kmem < 0)
	{
		kmem = open("/dev/kmem", 0, 0);
		if (kmem < 0)
			return (-1);
		(void) fcntl(kmem, F_SETFD, ~0);
		nlist("/hp-ux", Nl);
		if (Nl[0].n_type == 0)
			return (-1);
	}
	if (lseek(kmem, (off_t) Nl[X_AVENRUN].n_value, 0) == -1 ||
	    read(kmem, (char *) avenrun, sizeof(avenrun)) < sizeof(avenrun))
	{
		/* thank you Ian */
		return (-1);
	}
	return ((int) (avenrun[0] + 0.5));
}

# 	endif /* hpux */

# endif  /* defined(PSTAT) && defined(PSTAT_SETCMD) */
/*
**  SHOULDQUEUE -- should this message be queued or sent?
**
**	Compares the message cost to the load average to decide.
**
**	Parameters:
**		pri -- the priority of the message in question.
**
**	Returns:
**		TRUE -- if this message should be queued up for the
**			time being.
**		FALSE -- if the load is low enough to send this message.
**
**	Side Effects:
**		none.
*/

bool
shouldqueue(pri)
	long pri;
{
	if (CurrentLA < QueueLA)
		return (FALSE);
	if (CurrentLA >= RefuseLA)
		return (TRUE);
	return (pri > (QueueFactor / (CurrentLA - QueueLA + 1)));
}
/*
**  SETPROCTITLE -- set process title for ps
**
**	Parameters:
**		fmt -- a printf style format string.
**		a, b, c -- possible parameters to fmt.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Clobbers argv of our main procedure so ps(1) will
**		display the title.
*/

# ifdef PSTAT

/*VARARGS1*/
void setproctitle(fmt, a, b, c)
	char *fmt;
{
# 	ifdef SETPROCTITLE
	register char *bp;
	register int i;
	extern char *Args;
	static char buf[BUFSIZ];

	strcpy(buf, Args);
	strcat(buf, " -");
	(void) sprintf(buf+strlen(Args)+2, fmt, a, b, c);
	/* clean out CR's and LF's */
	bp = buf;
	while (*bp) {
		if (*bp == '\n' | *bp == '\r')
			*bp = ' ';
		bp++;
	}

# 		ifndef PSTAT_SETCMD
	{
		char *Argv[2];
		Argv[0] = buf;
		Argv[1] = NULL;
		pstat_set_command(Argv);
	}
# 		else	/* ! not PSTAT_SETCMD */
        pstat(PSTAT_SETCMD, buf, strlen(buf), 0, 0);
# 		endif	/* not PSTAT_SETCMD */
# 	endif /* SETPROCTITLE */
}

# else /* ! PSTAT */

/*VARARGS1*/
void setproctitle(fmt, a, b, c)
	char *fmt;
{
# 	ifdef SETPROCTITLE
	register char *p;
	register int i;
	extern char **Argv;
	extern char *LastArgv;
	char buf[MAXLINE];

	(void) sprintf(buf, fmt, a, b, c);

	/* make ps print "(sendmail)" */
	p = Argv[0];
	*p++ = '-';

	i = strlen(buf);
	if (i > LastArgv - p - 2)
	{
		i = LastArgv - p - 2;
		buf[i] = '\0';
	}
	(void) strcpy(p, buf);
	p += i;
	while (p < LastArgv)
		*p++ = ' ';
# 	endif /* SETPROCTITLE */
}

# endif /* PSTAT */


# if 0	/* No longer used */
/*
**  REAPCHILD -- pick up the body of my child, lest it become a zombie
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Picks up extant zombies.
*/

# include <sys/wait.h>

void reapchild()
{
# ifdef WNOHANG
	union wait status;

	while (wait3(&status, WNOHANG, (struct rusage *) NULL) > 0)
		continue;
# else /* ! WNOHANG */
	auto int status;

	while (wait(&status) > 0)
		continue;
# endif /* WNOHANG */
}
# endif /* 0 */
