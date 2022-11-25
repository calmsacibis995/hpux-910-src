/* @(#)  $Revision: 66.5 $ */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef  hpux
#  include <varargs.h>
#endif  hpux
#include "restore.h"


/*
 * Insure that all the components of a pathname exist.
 */
pathcheck(name)
	char *name;
{
	register char *cp;
	struct entry *ep;
	char *start;

	start = index(name, '/');
	if (start == 0)
		return;
	for (cp = start; *cp != '\0'; cp++) {
		if (*cp != '/')
			continue;
		*cp = '\0';
		ep = lookupname(name);
		if (ep == NIL) {
			ep = addentry(name, psearch(name), NODE);
			newnode(ep);
		}
		ep->e_flags |= NEW|KEEP;
		*cp = '/';
	}
}

/*
 * Change a name to a unique temporary name.
 */
mktempname(ep)
	register struct entry *ep;
{
	char oldname[MAXPATHLEN];

	if (ep->e_flags & TMPNAME)
		badentry(ep, "mktempname: called with TMPNAME");
	ep->e_flags |= TMPNAME;
	(void) strcpy(oldname, myname(ep));
	freename(ep->e_name);
	ep->e_name = savename(gentempname(ep));
	ep->e_namlen = strlen(ep->e_name);
	renameit(oldname, myname(ep));
}

/*
 * Generate a temporary name for an entry.
 */
char *
gentempname(ep)
	struct entry *ep;
{
	static char name[MAXPATHLEN];
	struct entry *np;
	long i = 0;

	for (np = lookupino(ep->e_ino); np != NIL && np != ep; np = np->e_links)
		i++;
	if (np == NIL)
		badentry(ep, "not on ino list");
	(void) sprintf(name, "%s%d%d", TMPHDR, i, ep->e_ino);
	return (name);
}

/*
 * Rename a file or directory.
 */
renameit(from, to)
	char *from, *to;
{
	if (rename(from, to) < 0) {
		fprintf(stderr, "Warning: cannot rename %s to %s", from, to);
		(void) fflush(stderr);
		perror("");
		return;
	}
	vprintf(stdout, "rename %s to %s\n", from, to);
}

/*
 * Create a new node (directory).
 */
newnode(np)
	struct entry *np;
{
	char *cp;

#if defined(TRUX) && defined(B1)
        register struct inotab *itp;
	char curname[MAXPATHLEN],savecp[MAXPATHLEN];
	char   	 *cwd;
	int  exists=0;
#endif /* TRUX && B1 */

	if (np->e_type != NODE)
		badentry(np, "newnode: not a node");
	cp = myname(np);

#if defined(TRUX) && defined(B1)
/* if the cild dir of an MLD touch a file so the system creates the mld itself
 * then remove that file. If an MLD itself, do a mkmultdir
 * i.e. /tmp is SEC_I_MLD & /tmp/mac0000...2 is SEC_I_MLDCHILD 
 */

        if (it_is_b1fs) {

		msg_b1("newnode: file:%s mld: %d mac:%d\n",cp,
		        np->secinfo.di_type_flags, np->secinfo.di_tag[1]);
		itp = inotablookup(np->e_ino);
		if (itp->secinfo.di_type_flags == SEC_I_MLDCHILD)
			return;
        }
#endif /* TRUX && B1 */

	vprintf(stdout, "Make node %s\n", cp);
#if defined(DUX) || defined(DISKLESS)
	if (np->e_CDF != C_CDF) {
#endif
#if defined(TRUX) && defined(B1)
		if (it_is_b1fs) 	chproclvl(itp->secinfo, curfile, cp, 1);
#endif /* TRUX && B1 */
		if (mkdir(cp, 0777) < 0) {
#if defined(TRUX) && defined(B1)
		   /* Fix the name it may have mldchild in it's path */
		    strcpy(savecp,cp);
		    rm_mldchildname(cp); 
		    if (mkdir(cp, 0777) < 0) {
#endif /* TRUX && B1 */
			np->e_flags |= EXISTED;
			fprintf(stderr, "Warning: ");
			(void) fflush(stderr);
#if !defined(TRUX) && !defined(B1)
			perror(cp);
#else /* TRUX && B1 */
			exists = 1;
			perror(savecp);
		    }
#endif /* TRUX && B1 */
		}
#if defined(TRUX) && defined(B1)
	if (it_is_b1fs && itp->secinfo.di_type_flags == SEC_I_MLD) {

		forcepriv(SEC_MULTILEVELDIR);
		if (mkmultdir(cp) !=0){
			disablepriv(SEC_MULTILEVELDIR);
			cwd = getcwd(curname, MAXPATHLEN+1);
			chdir(cp); /* for the nested mld bug in kernel */
			forcepriv(SEC_MULTILEVELDIR);
			if (mkmultdir(".") !=0)
			   if (!exists){/*if existed,it probabely already mld */
				fprintf(stderr, "Error: cannot make %s multileveldir\n",savecp);
				(void) fflush(stderr);
			   }
			disablepriv(SEC_MULTILEVELDIR);
			chdir(cwd);
		}
		disablepriv(SEC_MULTILEVELDIR);

	} 
	sec_chmod(cp,itp->secinfo, exists); 
#endif /* TRUX && B1 */
#if defined(DUX) || defined(DISKLESS)
	} else {
		np->e_flags |= EXISTED;
	}
#endif
}

/*
 * Remove an old node (directory).
 */
removenode(ep)
	register struct entry *ep;
{
	char *cp;

	if (ep->e_type != NODE)
		badentry(ep, "removenode: not a node");
	if (ep->e_entries != NIL)
		badentry(ep, "removenode: non-empty directory");
	ep->e_flags |= REMOVED;
	ep->e_flags &= ~TMPNAME;
	cp = myname(ep);
	if (rmdir(cp) < 0) {
		fprintf(stderr, "Warning: ");
		(void) fflush(stderr);
		perror(cp);
		return;
	}
	vprintf(stdout, "Remove node %s\n", cp);
}

/*
 * Remove a leaf.
 */
removeleaf(ep)
	register struct entry *ep;
{
	char *cp;

	if (ep->e_type != LEAF)
		badentry(ep, "removeleaf: not a leaf");
	ep->e_flags |= REMOVED;
	ep->e_flags &= ~TMPNAME;
	cp = myname(ep);
	if (unlink(cp) < 0) {
		fprintf(stderr, "Warning: ");
		(void) fflush(stderr);
		perror(cp);
		return;
	}
	vprintf(stdout, "Remove leaf %s\n", cp);
}

/*
 * Create a link.
 */
linkit(existing, new, type)
	char *existing, *new;
	int type;
{

	if (type == SYMLINK) {
		if (symlink(existing, new) < 0) {
			fprintf(stderr,
				"Warning: cannot create symbolic link %s->%s: ",
				new, existing);
			(void) fflush(stderr);
			perror("");
			return (FAIL);
		}
	} else if (type == HARDLINK) {
		if (link(existing, new) < 0) {
			fprintf(stderr,
				"Warning: cannot create hard link %s->%s: ",
				new, existing);
			(void) fflush(stderr);
			perror("");
			return (FAIL);
		}
	} else {
		panic("linkit: unknown type %d\n", type);
		return (FAIL);
	}
	vprintf(stdout, "Create %s link %s->%s\n",
		type == SYMLINK ? "symbolic" : "hard", new, existing);
	return (GOOD);
}

/*
 * find lowest number file (above "start") that needs to be extracted
 */
ino_t
lowerbnd(start)
	ino_t start;
{
	register struct entry *ep;

	for ( ; start < maxino; start++) {
		ep = lookupino(start);
		if (ep == NIL || ep->e_type == NODE)
			continue;
		if (ep->e_flags & (NEW|EXTRACT))
			return (start);
	}
	return (start);
}

/*
 * find highest number file (below "start") that needs to be extracted
 */
ino_t
upperbnd(start)
	ino_t start;
{
	register struct entry *ep;

	for ( ; start > ROOTINO; start--) {
		ep = lookupino(start);
		if (ep == NIL || ep->e_type == NODE)
			continue;
		if (ep->e_flags & (NEW|EXTRACT))
			return (start);
	}
	return (start);
}

/*
 * report on a badly formed entry
 */
badentry(ep, msg)
	register struct entry *ep;
	char *msg;
{

	fprintf(stderr, "bad entry: %s\n", msg);
	fprintf(stderr, "name: %s\n", myname(ep));
	fprintf(stderr, "parent name %s\n", myname(ep->e_parent));
	if (ep->e_sibling != NIL)
		fprintf(stderr, "sibling name: %s\n", myname(ep->e_sibling));
	if (ep->e_entries != NIL)
		fprintf(stderr, "next entry name: %s\n", myname(ep->e_entries));
	if (ep->e_links != NIL)
		fprintf(stderr, "next link name: %s\n", myname(ep->e_links));
	if (ep->e_next != NIL)
		fprintf(stderr, "next hashchain name: %s\n", myname(ep->e_next));
	fprintf(stderr, "entry type: %s\n",
		ep->e_type == NODE ? "NODE" : "LEAF");
	fprintf(stderr, "inode number: %ld\n", ep->e_ino);
	panic("flags: %s\n", flagvalues(ep));
}

/*
 * Construct a string indicating the active flag bits of an entry.
 */
char *
flagvalues(ep)
	register struct entry *ep;
{
	static char flagbuf[BUFSIZ];

	(void) strcpy(flagbuf, "|NIL");
	flagbuf[0] = '\0';
	if (ep->e_flags & REMOVED)
		(void) strcat(flagbuf, "|REMOVED");
	if (ep->e_flags & TMPNAME)
		(void) strcat(flagbuf, "|TMPNAME");
	if (ep->e_flags & EXTRACT)
		(void) strcat(flagbuf, "|EXTRACT");
	if (ep->e_flags & NEW)
		(void) strcat(flagbuf, "|NEW");
	if (ep->e_flags & KEEP)
		(void) strcat(flagbuf, "|KEEP");
	if (ep->e_flags & EXISTED)
		(void) strcat(flagbuf, "|EXISTED");
	return (&flagbuf[1]);
}

/*
 * Check to see if a name is on a dump tape.
 */
ino_t
dirlookup(name)
	char *name;
{
	ino_t ino;

	ino = psearch(name);
	if (ino == 0 || BIT(ino, dumpmap) == 0)
		fprintf(stderr, "%s is not on tape\n", name);
	return (ino);
}

/*
 * Elicit a reply.
 */
reply(question)
	char *question;
{
	char c;

	do	{
		fprintf(stderr, "%s? [yn] ", question);
		(void) fflush(stderr);
		c = getc(terminal);
		while (c != '\n' && getc(terminal) != '\n')
			if (feof(terminal))
				return (FAIL);
	} while (c != 'y' && c != 'n');
	if (c == 'y')
		return (GOOD);
	return (FAIL);
}

/*
 * handle unexpected inconsistencies
 */
#ifdef	hpux
panic(va_alist)
	va_dcl
{
	va_list ap;
	char *fmt;

	va_start(ap);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (reply("abort") == GOOD) {
		if (reply("dump core") == GOOD)
			abort();
		done(1);
	}
}

#else	hpux
/* VARARGS1 */
panic(msg, d1, d2)
	char *msg;
	long d1, d2;
{

	fprintf(stderr, msg, d1, d2);
	if (reply("abort") == GOOD) {
		if (reply("dump core") == GOOD)
			abort();
		done(1);
	}
}
#endif	hpux
