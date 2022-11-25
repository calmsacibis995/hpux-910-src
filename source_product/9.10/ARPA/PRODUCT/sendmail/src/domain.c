/*
 * Copyright (c) 1986 Eric P. Allman
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

# include "sendmail.h"

# ifndef lint
static char rcsid[] = "$Header: domain.c,v 1.14.109.6 95/02/21 16:07:44 mike Exp $";
# 	ifndef hpux
# 		ifdef NAMED_BIND
static char sccsid[] = "@(#)domain.c	5.22 (Berkeley) 6/1/90 (with name server)";
# 		else	/* ! NAMED_BIND */
static char sccsid[] = "@(#)domain.c	5.22 (Berkeley) 6/1/90 (without name server)";
# 		endif	/* NAMED_BIND */
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: domain.o $Revision: 1.14.109.6 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# ifdef NAMED_BIND

# 	include <sys/param.h>
# 	include <errno.h>
# 	include <arpa/nameser.h>
# 	include <resolv.h>
# 	include <netdb.h>

typedef union {
	HEADER qb1;
	char qb2[PACKETSZ];
} querybuf;

static char hostbuf[MAXMXHOSTS*PACKETSZ];

getmxrr(host, mxhosts, rcode, e)
	char *host, **mxhosts;
	int *rcode;
	ENVELOPE *e;
{
	extern int h_errno;
	register u_char *eom, *cp;
	register int i, j, n;
	int nmx = 0;
	register char *bp;
	char localhost[MAXNAME];
	HEADER *hp;
	querybuf answer;
	int ancount, qdcount, buflen;
	bool seenlocal = FALSE;
	u_short pref, localpref, type, prefer[MAXMXHOSTS];
	int weight[MAXMXHOSTS];

	/*
	** Disable the usual resolver search scheme.
	** Disable appending of default domain names unless
	** the host has no domain.
	*/
	_res.options &= ~RES_DNSRCH;
	if (strchr(host, '.') != NULL)
		_res.options &= ~RES_DEFNAMES;

	h_errno = 0;
	errno = 0;
	n = res_search(host, C_IN, T_MX, (char *)&answer, sizeof(answer));

	/*
	** Restore the resolver options.
	*/
	_res.options |= (RES_DEFNAMES | RES_DNSRCH);

	if (n < 0)
	{
# 	ifdef DEBUG
		if (tTd(8, 1))
			printf("getmxrr: res_search failed (errno=%d, h_errno=%d)\n",
			    errno, h_errno);
# 	endif	/* DEBUG */
		switch (h_errno)
		{
		  case NO_DATA:
		  case NO_RECOVERY:
			/* no MX data on this host */
			goto punt;

		  case HOST_NOT_FOUND:
			/* the host just doesn't exist */
			*rcode = EX_NOHOST;
			break;

		  case TRY_AGAIN:
			/* couldn't connect to the name server */
			if (!UseNameServer && errno == ECONNREFUSED)
				goto punt;

			/* it might come up later; better queue it up */
			*rcode = EX_TEMPFAIL;
			break;
		}

		/* irreconcilable differences */
		return (-1);
	}

	/* find first satisfactory answer */
	hp = (HEADER *)&answer;
	cp = (u_char *)&answer + sizeof(HEADER);
	eom = (u_char *)&answer + n;
	for (qdcount = ntohs(hp->qdcount); qdcount--; cp += n + QFIXEDSZ)
		if ((n = dn_skipname(cp, eom)) < 0)
			goto punt;
	nmx = 0;
	buflen = sizeof(hostbuf);
	bp = hostbuf;
	ancount = ntohs(hp->ancount);
	while (--ancount >= 0 && cp < eom && nmx < MAXMXHOSTS) {
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		GETSHORT(type, cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		GETSHORT(n, cp);
		if (type != T_MX)
		{
# 	ifdef DEBUG
			if (tTd(8, 8) || _res.options & RES_DEBUG)
				printf("unexpected answer type %d, size %d\n",
				    type, n);
# 	endif	/* DEBUG */
			cp += n;
			continue;
		}
		GETSHORT(pref, cp);
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
# 	ifdef LOG
		if (LogLevel > 11)
			syslog(LOG_DEBUG, "%s: MX host=%s, pref=%d", 
			       e->e_id, bp, pref);
# 	 endif /* LOG */
		expand("\001w", localhost, &localhost[sizeof(localhost) - 1], e);
		if (!strcasecmp(bp, localhost)) {
			if (!seenlocal || pref < localpref)
				localpref = pref;
			seenlocal = TRUE;
			continue;
		}
		weight[nmx] = mxrand(bp);
		prefer[nmx] = pref;
		mxhosts[nmx++] = bp;
		n = strlen(bp);
		bp += n;
		if (bp[-1] != '.')
		{
			*bp++ = '.';
			n++;
		}
		*bp++ = '\0';
		buflen -= n + 1;
	}
	if (nmx == 0) {
punt:		
# 	ifdef LOG
		if (LogLevel > 11)
			syslog(LOG_DEBUG, "%s: no MX records for %s",
			       e->e_id, host);
# 	 endif /* LOG */
		mxhosts[0] = strcpy(hostbuf, host);
		return(0);
	}

	/* sort the records */
	for (i = 0; i < nmx; i++)
	{
		for (j = i + 1; j < nmx; j++)
		{
			if (prefer[i] > prefer[j] ||
			    (prefer[i] == prefer[j] && weight[i] > weight[j]))
			{
				register int temp;
				register char *temp1;

				temp = prefer[i];
				prefer[i] = prefer[j];
				prefer[j] = temp;
				temp1 = mxhosts[i];
				mxhosts[i] = mxhosts[j];
				mxhosts[j] = temp1;
				temp = weight[i];
				weight[i] = weight[j];
				weight[j] = temp;
			}
		}
		if (seenlocal && prefer[i] >= localpref)
		{
			/*
			 * truncate higher pref part of list; if we're
			 * the best choice left, we should have realized
			 * awhile ago that this was a local delivery.
			 * Berkeley called this a configuration error.
			 * We will go ahead and send to the original host.
			 */
# 	ifdef LOG
			if (LogLevel > 11)
				syslog(LOG_DEBUG,
				       "%s: truncating MX host list at pref=%d",
				       e->e_id, localpref);
# 	 endif /* LOG */
			nmx = i;
			if (i == 0)
			{
				goto punt;
			}
			break;
		}
	}

	return (nmx);
}

/*
**  MXRAND -- create a randomizer for equal MX preferences
**
**	If two MX hosts have equal preferences we want to randomize
**	the selection.  But in order for signatures to be the same,
**	we need to randomize the same way each time.  This function
**	computes a pseudo-random hash function from the host name.
**
**	Parameters:
**		host -- the name of the host.
**
**	Returns:
**		A random but repeatable value based on the host name.
**
**	Side Effects:
**		none.
*/

mxrand(host)
	register char *host;
{
	int hfunc;
	static unsigned int seed;

	if (seed == 0)
	{
		seed = (int) curtime() & 0xffff;
		if (seed == 0)
			seed++;
	}

	if (tTd(17, 9))
		printf("mxrand(%s)", host);

	hfunc = seed;
	while (*host != '\0')
	{
		int c = *host++;

		if (isascii(c) && isupper(c))
			c = tolower(c);
		hfunc = ((hfunc << 1) + c) % 2003;
	}

	hfunc &= 0xff;

	if (tTd(17, 9))
		printf(" = %d\n", hfunc);
	return hfunc;
}

getcanonname(host, hbsize)
	char *host;
	int hbsize;
{
	extern int h_errno;
	register u_char *eom, *cp;
	register int n; 
	HEADER *hp;
	querybuf answer;
	u_short type;
	int first, ancount, qdcount, loopcnt;
	char nbuf[PACKETSZ];
	int saveoptions;
# 	ifdef HOSTINFO
	extern STAB *stab();
	STAB *st, *stlist[10];
	int curst = -1;
	char ahost[MAXDNAME];
# 	 endif /* HOSTINFO */

	/*
	**  Ensure that the name server search scheme is enabled.
	*/
	saveoptions = _res.options;
	_res.options |= (RES_DEFNAMES | RES_DNSRCH);
	loopcnt = 0;
	ahost[0] = '\0';
loop:

# 	ifdef HOSTINFO
	/*
	**  Check if we've already canonicalized this host name.
	*/
	st = stab(host, ST_HOST, ST_FIND);
	if (st != NULL) {
		if (loopcnt == 0) {
			strncpy(host, st->s_hostinfo.ho_name, hbsize);
			host[hbsize - 1] = '\0';
		}
		goto finish;
	}
	else {
		/*
		**  Add an entry for this host name.
		*/
		if (curst >= 0)
			stlist[++curst] = stab(host, ST_HOST, ST_ENTER);
		else
			strncpy(ahost, host, MAXDNAME);
	}
# 	 endif /* HOSTINFO			 */

	if (!UseNameServer)
	{
		register struct hostent *hp;
		struct hostent *gethostbyname();
			
		hp = gethostbyname(host);
		if (hp != (struct hostent *)NULL)
		{
			strncpy(host, hp->h_name, hbsize);
			host[hbsize - 1] = '\0';
# 	ifdef HOSTINFO
			stlist[++curst] = stab(host, ST_HOST, ST_ENTER);
# 	 endif /* HOSTINFO */
		}
		goto finish;
	}
             
	/*
	 * Use query type of ANY if possible (NO_WILDCARD_MX), which will
	 * find types CNAME, A, and MX, and will cause all existing records
	 * to be cached by our local server.  If there is (might be) a
	 * wildcard MX record in the local domain or its parents that are
	 * searched, we can't use ANY; it would cause fully-qualified names
	 * to match as names in a local domain.
	 */
# 	ifdef NO_WILDCARD_MX
	n = res_search(host, C_IN, T_ANY, (char *)&answer, sizeof(answer));
# 	else	/* ! NO_WILDCARD_MX */
	n = res_search(host, C_IN, T_CNAME, (char *)&answer, sizeof(answer));
# 	endif	/* NO_WILDCARD_MX */
	if (n < 0) {
# 	ifdef DEBUG
		if (tTd(8, 1))
			printf("getcanonname:  res_search failed (errno=%d, h_errno=%d)\n",
			    errno, h_errno);
# 	 endif /* DEBUG */
		goto finish;
	}

	/* find first satisfactory answer */
	hp = (HEADER *)&answer;
	ancount = ntohs(hp->ancount);

	/* we don't care about errors here, only if we got an answer */
	if (ancount == 0) {
# 	ifdef DEBUG
		if (tTd(8, 1))
			printf("rcode = %d, ancount=%d\n", hp->rcode, ancount);
# 	endif	/* DEBUG */
		goto finish;
	}
	cp = (u_char *)&answer + sizeof(HEADER);
	eom = (u_char *)&answer + n;
	for (qdcount = ntohs(hp->qdcount); qdcount--; cp += n + QFIXEDSZ)
		if ((n = dn_skipname(cp, eom)) < 0)
			goto finish;

	/*
	 * just in case someone puts a CNAME record after another record,
	 * check all records for CNAME; otherwise, just take the first
	 * name found.
	 */
	for (first = 1; --ancount >= 0 && cp < eom; cp += n) {
		if ((n = dn_expand((char *)&answer, eom, cp, nbuf,
		    sizeof(nbuf))) < 0)
			break;
		if (first) {			/* XXX */
			(void)strncpy(host, nbuf, hbsize);
			host[hbsize - 1] = '\0';
# 	ifdef HOSTINFO
			stlist[++curst] = stab(host, ST_HOST, ST_ENTER);
# 	 endif /* HOSTINFO */
			first = 0;
		}
		cp += n;
		GETSHORT(type, cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		GETSHORT(n, cp);
		if (type == T_CNAME)  {
			/*
			 * assume that only one cname will be found.  More
			 * than one is undefined.  Copy so that if dn_expand
			 * fails, `host' is still okay.
			 */
 			if ((n = dn_expand((char *)&answer, eom, cp, nbuf,
			    sizeof(nbuf))) < 0)
				break;
			(void)strncpy(host, nbuf, hbsize); /* XXX */
			host[hbsize - 1] = '\0';
			if (++loopcnt > 8)	/* never be more than 1 */
				goto finish;
			goto loop;
		}
	}

finish:
# 	ifdef HOSTINFO

	/*
	** Restore the resolver options.
	*/
	_res.options = saveoptions;

	/*
	** Return if we found it immediately.
	*/
	if (curst < 0)
		return;

	/*
	**  Enter initial key
	*/
	stlist[++curst] = stab(ahost, ST_HOST, ST_ENTER);

	/*
	**  Fill out the host information.
	*/
	if (stlist[0] != NULL) {
		stlist[0]->s_hostinfo.ho_name = newstr(host);
		stlist[0]->s_hostinfo.ho_errno = 0;
		stlist[0]->s_hostinfo.ho_exitstat = EX_OK;
		while (curst) {
			if (stlist[curst] != NULL)
				bcopy(&(stlist[0]->s_hostinfo),
				      &(stlist[curst]->s_hostinfo), sizeof(st->s_hostinfo));
			curst--;
		}
	}
# 	 endif /* HOSTINFO */
}

# else /* not NAMED_BIND */

# 	include <netdb.h>

getcanonname(host, hbsize)
	char *host;
	int hbsize;
{
	struct hostent *hp;

	hp = gethostbyname(host);
	if (hp == NULL)
		return;

	if (strlen(hp->h_name) >= hbsize)
		return;

	(void) strcpy(host, hp->h_name);
}

# endif /* not NAMED_BIND */
