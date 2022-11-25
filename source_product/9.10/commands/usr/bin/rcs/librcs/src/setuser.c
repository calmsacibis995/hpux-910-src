/* $Header: setuser.c,v 66.1 90/11/14 14:47:19 ssa Exp $ */

#include "rcsbase.h"

static int ruid;             /* Real uid */
static int rgid;             /* Real gid */
static int euid;             /* Effective uid */
static int egid;             /* Effective gid */
static int initflag = false; /* TRUE if uid's & gid's have been set */

static
inituids()
{
	ruid = getuid();
	rgid = getgid();
	euid = geteuid();
	egid = getegid();
	initflag = true;
}

setruser()
{
	if (initflag == false)
		inituids();

	setuid(ruid);
	setgid(rgid);
}

seteuser()
{
	if (initflag == false)
		inituids();

	setuid(euid);
	setgid(egid);
}
