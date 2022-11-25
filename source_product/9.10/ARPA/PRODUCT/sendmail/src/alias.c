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
static char rcsid[] = "$Header: alias.c,v 1.26.109.13 95/02/21 16:07:00 mike Exp $";
# 	ifndef hpux
# 		ifdef NDBM
static char sccsid[] = "@(#)alias.c	5.21 (Berkeley) 6/1/90 (with NDBM)";
# 		else	/* ! NDBM */
static char sccsid[] = "@(#)alias.c	5.21 (Berkeley) 6/1/90 (without NDBM)";
# 		endif	/* NDBM */
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: alias.o $Revision: 1.26.109.13 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include <sys/types.h>
# include <sys/stat.h>
# include <sys/unistd.h>
# include <signal.h>
# include <errno.h>
# include "sendmail.h"
# include <sys/file.h>
# include <pwd.h>
# ifdef HP_NFS
# 	include <rpcsvc/ypclnt.h>
# endif	/* HP_NFS */

/*
**  ALIAS -- Compute aliases.
**
**	Scans the alias file for an alias for the given address.
**	If found, it arranges to deliver to the alias list instead.
**	Uses libndbm database if -DNDBM.
**
**      If local alias lookup fails, consult NIS map if NIS is 
**      running on the system.
**
**	Parameters:
**		a -- address to alias.
**		sendq -- a pointer to the head of the send queue
**			to put the aliases in.
**		e -- the current envelope.
**
**	Returns:
**		none
**
**	Side Effects:
**		Aliases found are expanded.
**
**	Notes:
**		If NoAlias (the "-n" flag) is set, no aliasing is
**			done.
**
**	Deficiencies:
**		It should complain about names that are aliased to
**			nothing.
*/


char *mydomain = NULL; /* NIS domain name*/

void alias(a, sendq, e)
	register ADDRESS *a;
	ADDRESS **sendq;
	register ENVELOPE *e;
{
	register char *p;
	extern char *aliaslookup(), 
		    *NISlookup(); /* NIS aliases map lookup*/

# ifdef DEBUG
	if (tTd(27, 1))
		printf("alias(%s)\n", a->q_paddr);
# endif	/* DEBUG */

	/* don't realias already aliased names */
	if (bitset(QDONTSEND|QBADADDR|QVERIFIED, a->q_flags))
		return;

	e->e_to = a->q_paddr;

	/*
	**  Look up this name
	*/

	if (NoAlias || (AliasDB == NULL))
		p = NULL;
	else {
		p = aliaslookup(AliasDB, a->q_user);
# ifdef HP_NFS
		 /* Consult NIS map if it is available on the system */
		if (p == NULL)               
			p = NISlookup(a);   
#  endif /* HP_NFS */
	}
	if (p == NULL)
		return;

	/*
	**  Match on Alias.
	**	Deliver to the target list.
	*/

# ifdef DEBUG
	if (tTd(27, 1))
		printf("%s (%s, %s) aliased to %s\n",
		    a->q_paddr, a->q_host, a->q_user, p);
# endif	/* DEBUG */
	if (bitset(EF_VRFYONLY, e->e_flags))
	{
		a->q_flags |= QVERIFIED;
		e->e_nrcpts++;
		return;
	}
	message("aliased to %s", p);
	AliasLevel++;
	sendtolist(p, a, sendq, e);
	AliasLevel--;
}

/*
**  ALIASLOOKUP -- look up a name in the alias file.
**
**	Parameters:
**		name -- the name to look up.
**
**	Returns:
**		the value of name.
**		NULL if unknown.
**
**	Side Effects:
**		none.
**
**	Warnings:
**		The return value will be trashed across calls.
*/

char *
aliaslookup(alias_db, name)
	DBM *alias_db;
	char *name;
{
	datum rhs, lhs;

	/* create a key for fetch */
	lhs.dptr = name;
	lhs.dsize = strlen(name) + 1;
	rhs = dbm_fetch(alias_db, lhs);

# ifdef DEBUG
	if (tTd(27,3))
		printf("aliaslookup(%s) about to return '%s'\n", name, rhs.dptr);
# endif /* DEBUG */

	return (rhs.dptr);
}

# ifdef HP_NFS
/*
**  NISLOOKUP -- look up a name in the NIS.
**
**	Parameters:
**		a -- the address to look up.
**
**	Returns:
**		the value of name.
**		NULL if unknown.
**
**	Side Effects:
**		sets 
**
**	Warnings:
**		The return value will be trashed across calls.
*/

char *
NISlookup(a)
	register ADDRESS *a;
  {
	char *result;
	int insize, outsize, err;
	extern char *AliasMap;

		/*
		 * if we did not find a local alias, then
		 * try a remote alias through NIS.
		 */
	if (AliasMap==NULL || *AliasMap=='\0') return(NULL);

	if (mydomain==NULL)
		{
		  yp_get_default_domain(&mydomain);
		  if (mydomain == NULL) return(NULL);
# 	ifdef DEBUG
		  if (tTd(27, 1))
	    		printf("NIS domain is %s\n",mydomain);
# 	 endif /* DEBUG */
		}
	if (bitset(QWASLOCAL,a->q_flags))
		return(NULL);

	/* Try with the exact size first; some systems require this.
	 * If that fails, try with the size + 1 (for the terminating
	 * NULL byte); some system require this.
	 */
	insize = strlen(a->q_user);
	err = yp_match(mydomain, AliasMap, a->q_user, insize, &result, &outsize);
	if (err == YPERR_KEY)
		err = yp_match(mydomain, AliasMap, a->q_user, insize+1, &result, &outsize);
	if (err)
		{
		  errno = 0;
		  return(NULL);
		}


# 	ifdef DEBUG
        if (tTd(27, 1))
		printf("%s maps to %s\n",a->q_user, result);
# 	 endif /* DEBUG */
	a->q_flags |= QDOMAIN;
	return(result);
}
# endif	/* HP_NFS */

/*
**  INITALIASES -- initialize for aliasing
**
**	Parameters:
**		aliasfile -- location of aliases.
**		init -- if set, initialize the NDBM files.
**
**	Returns:
**		Ptr to the alias db..
**
**	Side Effects:
**		initializes aliases:
**		opens the database.
*/

# define DBMMODE	0644  

DBM *
initaliases(aliasfile, init, e)
	char *aliasfile;
	bool init;
	register ENVELOPE *e;
{
	int atcnt;
	time_t modtime; 
	bool automatic = FALSE;
	char buf[MAXNAME];
	DBM *alias_db;
	struct stat stb;

	if (aliasfile == NULL || stat(aliasfile, &stb) < 0)
	{
		if (aliasfile != NULL && init) {
			syserr("Cannot open %s", aliasfile);
			ExitStat = EX_OSFILE;
		}
		errno = 0;
		return NULL;
	}

	/*
	**  Check to see that the alias file is complete.
	**	If not, we will assume that someone died, and it is up
	**	to us to rebuild it.
	*/

	if (!init){
		alias_db = dbm_open(aliasfile, O_RDONLY, DBMMODE);
		if (alias_db == NULL){
			usrerr("Cannot dbm_open %s", aliasfile);
			return(NULL);
		}
	}
	atcnt = SafeAlias * 2;
	if (atcnt > 0)
	{
		while (!init && atcnt-- >= 0 && aliaslookup(alias_db, "@") == NULL)
		{
			/*
			**  Reinitialize alias file in case the new
			**  one is mv'ed in instead of cp'ed in.
			**
			**	Only works with new DBM -- old one will
			**	just consume file descriptors forever.
			**	If you have a dbmclose() it can be
			**	added before the sleep(30).
			*/
			dbm_close(alias_db);
			sleep(30);
			alias_db = dbm_open(aliasfile, O_RDONLY, DBMMODE);
		}
	}
	else
		atcnt = 1;

	/*
	**  See if the DBM version of the file is out of date with
	**  the text version.  If so, go into 'init' mode automatically.
	**	This only happens if our effective userid owns the DBM.
	**	Note the unpalatable hack to see if the stat succeeded.
	*/

	modtime = stb.st_mtime;
	(void) strcpy(buf, aliasfile);
	(void) strcat(buf, ".pag");
	stb.st_ino = 0;
	if (!init && (stat(buf, &stb) < 0 || stb.st_mtime < modtime || atcnt < 0))
	{
		errno = 0;
		if (AutoRebuild && stb.st_ino != 0 && stb.st_uid == geteuid())
		{
			init = TRUE;
			automatic = TRUE;
			message("rebuilding alias database");
# ifdef LOG
			if (LogLevel > 0)
				syslog(LOG_INFO, "rebuilding alias database");
#  endif /* LOG */
		}
		else
		{
# ifdef LOG
			if (LogLevel > 0)
				syslog(LOG_INFO, "alias database out of date");
#  endif /* LOG */
			message("Warning: alias database out of date");
		}
	}


	/*
	**  If necessary, load the DBM file.
	*/

	if (init)
	{
		DBM* writealiases();
# ifdef LOG
		if (LogLevel >= 6)
		{
			extern char *username();

			syslog(LOG_NOTICE, "alias database %srebuilt by %s",
				automatic ? "auto" : "", username());
		}
#  endif /* LOG */
		return(writealiases(aliasfile, modtime, e));
	} else
		return(alias_db);
}

/*
**  WRITEALIASES -- read and process the alias file.
**
**	This routine writes the new alias files.
**
**	Parameters:
**		aliasfile -- the pathname of the alias file master.
**		modtime -- last modification time for ypxfr.
**		e -- the current envelope.
**
**	Returns:
**		the (DBM *) pointing to the open file.
**
**	Side Effects:
**		Reads aliasfile into the symbol table.
**		Optionally, builds the .dir & .pag files.
*/

DBM *
writealiases(aliasfile, modtime,e)
	char *aliasfile;
	time_t modtime;
	register ENVELOPE *e;
{
	register char *p;
	char *rhs;
	bool skipping;
	int naliases = -1, bytes, longest;
	FILE *af;
	void (*oldsigint)();
	void (*oldsighup)();
	void (*oldsigterm)();
	void (*oldsigquit)();
	ADDRESS al, bl;
	char line[BUFSIZ];
	char *tempfile;
	DBM *alias_db;

	if ((af = fopen(aliasfile, "r+")) == NULL)
	{
# ifdef DEBUG
		if (tTd(27, 1))
			printf("Can't open %s\n", aliasfile);
# endif	/* DEBUG */
		errno = 0;
		return NULL;
	}

	/* see if someone else is rebuilding the alias file already */
	if (lockf(fileno(af), F_TLOCK, 0) < 0 && 
	    (errno == EACCES || errno == EAGAIN))
	{
		/* yes, they are -- wait until done and then return */
		message("Alias file is already being rebuilt");
		if (OpMode != MD_INITALIAS)
		{
			/* wait for other rebuild to complete */
			(void) lockf(fileno(af), F_LOCK, 0);
		}
		(void) fclose(af);
		errno = 0;
		return(dbm_open(aliasfile, O_RDONLY, DBMMODE));
	}

	/*
	**  Create the new DBM files.
	*/

	{
		register char *p;
		char *mktemp();
		int fd;

		/*
		 * Create temporary alias database for rebuild.
		 */
		tempfile = mktemp("afAAXXXXX");
		if (tempfile == NULL)
			tempfile = newstr(aliasfile);
		else 
			tempfile = newstr(tempfile);
	
		oldsigint = signal(SIGINT, SIG_IGN);
		oldsighup = signal(SIGHUP, SIG_IGN);
		oldsigquit = signal(SIGQUIT, SIG_IGN);
		oldsigterm = signal(SIGTERM, SIG_IGN);
		(void) strcpy(line, tempfile);
		(void) strcat(line, ".dir");
		if ((fd = creat(line, DBMMODE)) < 0)
		{
			syserr("Cannot make %s", line);
			close(fd);
			goto bad;
		}
		close(fd);
		(void) strcpy(line, tempfile);
		(void) strcat(line, ".pag");
		if ((fd = creat(line, DBMMODE)) < 0)
		{
			syserr("Cannot make %s", line);
			close(fd);
			goto bad;
		}
		close(fd);
		alias_db = dbm_open(tempfile, O_RDWR|O_CREAT|O_TRUNC, DBMMODE);
		if (alias_db == NULL){
			syserr("Cannot dbm_open(%s)", tempfile);
			goto bad;
		}
	}

	/*
	**  Read and interpret lines
	*/

	FileName = aliasfile;
	LineNumber = 0;
	naliases = bytes = longest = 0;
	skipping = FALSE;
	while (fgets(line, sizeof (line), af) != NULL)
	{
		int lhssize, rhssize;

		errno = 0;
		LineNumber++;
		p = strchr(line, '\n');
		if (p != NULL)
			*p = '\0';
		switch (line[0])
		{
		  case '#':
		  case '\0':
			skipping = FALSE;
			continue;

		  case ' ':
		  case '\t':
			if (!skipping)
				syserr("554 Non-continuation line starts with space");
			skipping = TRUE;
			continue;
		}
		skipping = FALSE;

		/*
		**  Process the LHS
		**	Find the colon separator, and parse the address.
		**	It should resolve to a local name -- this will
		**	be checked later (we want to optionally do
		**	parsing of the RHS first to maximize error
		**	detection).
		*/

		for (p = line; *p != '\0' && *p != ':' && *p != '\n'; p++)
			continue;
		if (*p++ != ':')
		{
			syserr("554 missing colon");
			continue;
		}

		if (parseaddr(line, &al, 1, ':', e) == NULL)
		{
			syserr("554 %.40s... illegal alias name", line);
			continue;
		}

		loweraddr(&al);

		/*
		**  Process the RHS.
		**	'al' is the internal form of the LHS address.
		**	'p' points to the text of the RHS.
		*/

		rhs = p;
		for (;;)
		{
			register char c;

			if (CheckAliases)
			{
				/* do parsing & compression of addresses */
				while (*p != '\0')
				{
					extern char *DelimChar;

					while ((isascii(*p) && isspace(*p)) ||
								*p == ',')
						p++;
					if (*p == '\0')
						break;
					parseaddr(p, &bl, -1, ',', e);
					p = DelimChar;
				}
			}
			else
			{
				p = &p[strlen(p)];
				if (p[-1] == '\n')
					*--p = '\0';
			}

			/* rhs overflow */
			if (p >= line + sizeof line - 1) {
				syserr("Alias too long (%d chars max)",
				       sizeof(line)-1);
				skipping = TRUE;
				break;
			}

			/* see if there should be a continuation line */
			c = fgetc(af);
			if (!feof(af))
				(void) ungetc(c, af);
			if (c != ' ' && c != '\t')
				break;

			/* read continuation line */
			if (fgets(p, sizeof line - (p - line), af) == NULL)
				break;
			LineNumber++;
		}

		/* continue on rhs overflow */
		if (skipping)
		    continue;

		if (al.q_mailer != LocalMailer)
		{
			syserr("554 %s... cannot alias non-local names",
			        al.q_paddr);
			continue;
		}

		/*
		**  Insert alias into symbol table or DBM file
		*/

		lhssize = strlen(al.q_user) + 1;
		rhssize = strlen(rhs) + 1;

		{
			datum key, content;

			key.dsize = lhssize;
			key.dptr = al.q_user;
			content.dsize = rhssize;
			content.dptr = rhs;
			dbm_store(alias_db, key, content, DBM_REPLACE);
		}

		/* statistics */
		naliases++;
		bytes += lhssize + rhssize;
		if (rhssize > longest)
			longest = rhssize;
	}
	FileName = NULL;

	{
		/* add the distinquished alias "@" */
		datum key, value;
		char last_modified[16];

		key.dptr = "YP_LAST_MODIFIED";
		key.dsize = strlen(key.dptr);
		sprintf(last_modified, "%10.10d", modtime);
		value.dptr = last_modified;
		value.dsize = strlen(value.dptr);
		dbm_store(alias_db, key, value, DBM_REPLACE);

		key.dptr = "YP_MASTER_NAME";
		key.dsize = strlen(key.dptr);
		value.dptr = macvalue('w', e);
		value.dsize = strlen(value.dptr);
		dbm_store(alias_db, key, value, DBM_REPLACE);

		key.dsize = 2;
		key.dptr = "@";
		dbm_store(alias_db, key, key, DBM_REPLACE);

		dbm_close(alias_db);

		if (strcmp(aliasfile, tempfile)) {
			char tname[MAXNAME], aname[MAXNAME];
			int fd;

			/* Copy the new .dir file into place */
			(void) strcpy(aname, aliasfile);
			(void) strcat(aname, ".dir");
			(void) strcpy(tname, tempfile);
			(void) strcat(tname, ".dir");
			if ((fd = creat(aname, DBMMODE)) < 0 ||
			     mv(tname, aname))
			{
				syserr("Cannot make %s", aname);
				close(fd);
				unlink(tname);
				naliases = -1;
				goto bad;
			}

			close(fd);
			unlink(tname);

			/* Copy the new .pag file into place */
			(void) strcpy(aname, aliasfile);
			(void) strcat(aname, ".pag");
			(void) strcpy(tname, tempfile);
			(void) strcat(tname, ".pag");
			if ((fd = creat(aname, DBMMODE)) < 0  ||
			     mv(tname, aname))
			{
				syserr("Cannot make %s", aname);
				close(fd);
				unlink(tname);
				naliases = -1;
				goto bad;
			}
			close(fd);
			unlink(tname);
		}
bad:
		/* restore the old signal handlers */
		(void) signal(SIGINT, oldsigint);
		(void) signal(SIGHUP, oldsighup);
		(void) signal(SIGQUIT, oldsigquit);
		(void) signal(SIGTERM, oldsigterm);
	}

	/* closing the alias file drops the lock */
	(void) fclose(af);
	e->e_to = NULL;
	if (naliases < 0)
		return NULL;
	message("%s: %d aliases, longest %d bytes, %d bytes total",
		aliasfile, naliases, longest, bytes);
# ifdef LOG
	if (LogLevel > 0)
		syslog(LOG_INFO, "%s: %d aliases, longest %d bytes, %d bytes total",
		       aliasfile, naliases, longest, bytes);
#  endif /* LOG */
	return alias_db;
}
/*
**  FORWARD -- Try to forward mail
**
**	This is similar but not identical to aliasing.
**
**	Parameters:
**		user -- the name of the user who's mail we would like
**			to forward to.  It must have been verified --
**			i.e., the q_home field must have been filled
**			in.
**		sendq -- a pointer to the head of the send queue to
**			put this user's aliases in.
**
**	Returns:
**		none.
**
**	Side Effects:
**		New names are added to send queues.
*/

void forward(user, sendq, e)
	ADDRESS *user;
	ADDRESS **sendq;
	register ENVELOPE *e;
{
	char buf[60];
	extern bool safefile();

# ifdef DEBUG
	if (tTd(27, 1))
		printf("forward(%s)\n", user->q_paddr);
#  endif /* DEBUG */

	if (user->q_mailer != LocalMailer || bitset(QBADADDR, user->q_flags))
		return;
# ifdef DEBUG
	if (user->q_home == NULL)
		syserr("554 forward: no home");
#  endif /* DEBUG */

	/* good address -- look for .forward file in home */
	define('z', user->q_home, e);
	expand("\001z/.forward", buf, &buf[sizeof buf - 1], e);
        /* make sure the user (or root) owns the file */
	if ((!safefile(buf, user->q_uid, S_IREAD)) &&
	    (!safefile(buf, 0,           S_IREAD)))
		return;

	/* we do have an address to forward to -- do it */
	include(buf, "forwarding", user, sendq, e);
}
/*
**  MV - move a file
**
**	This is simply a call to /bin/mv which takes care of the
**	fork, exec, and wait.
**
**	Parameters:
**		source -- the source file to move
**
**		dest   -- the target of the move
**
**	Returns:
**		The exit value of the exec to /bin/mv.
**
**	Side Effects:
**		None.
*/

mv(source, dest)
	char *source, *dest;
{
        int pid, status;

        if (source == NULL || dest == NULL)
                return(1);

        if((pid = dofork()) == 0) {
		close(1);
		close(2);
# ifdef V4FS
		  (void) execl("/usr/bin/mv", "mv", "-f", source, dest, 0);
		  syserr("execl /usr/bin/mv");
# else	/* ! V4FS */
                (void) execl("/bin/mv", "mv", "-f", source, dest, 0);
                syserr("execl /bin/mv");
# endif /* V4FS */
		_exit(127);
        }

        if (pid == -1 ) {
                return(-1);
	}

        if (waitpid(pid, &status, 0) == -1)
		status = -1;

	return(status);
}
