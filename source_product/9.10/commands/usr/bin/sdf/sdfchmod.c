/* @(#) $Revision: 38.1 $ */     
static char *HPUX_ID = "@(#) $Revision: 38.1 $";
#include "s500defs.h"
#include <stdio.h>
#include <sys/stat.h>

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	01777	/* all (note absence of setuid, etc) */

#define	P_READ	00444	/* read permit */
#define	P_WRITE	00222	/* write permit */
#define	P_EXEC	00111	/* exec permit */
#define	P_SETID	06000	/* set[ug]id */
#define	P_STICKY	01000	/* sticky bit */

#ifdef	DEBUG
int debug = 0;
#endif	DEBUG

char	*ms, *pname;
struct	stat st;

main(argc,argv)
char **argv;
{
	register i;
	register char *p;
	int status = 0;

	pname = argv[0];

	if (argc < 3)
		usage();
	ms = argv[1];
	newmode(0);
	for (i = 2; i < argc; i++) {
		p = argv[i];
		if (sdfstat(p, &st) < 0) {
			fprintf(stderr, "%s: can't access %s\n",
				pname, p);
			++status;
			continue;
		}
		ms = argv[1];
		if (sdfchmod(p, newmode(st.st_mode)) < 0) {
			perror(p);
			/*fprintf(stderr, "%s: can't change %s\n",
				pname, p);*/
			++status;
			continue;
		}
	}
	exit(status);
}

usage()
{
	fprintf(stderr, "Usage: %s [ugoa][+-=][rwxstugo] device:file [...]\n",
		pname);
	exit(255);
}

newmode(nm)
unsigned nm;
{
	register o, m, b;

	m = abs();
	if (!*ms)
		return(m);
	do {
		m = who();
		while (o = what()) {
			b = where(nm);
			switch (o) {
			case '+':
				nm |= b & m;
				break;
			case '-':
				nm &= ~(b & m);
				break;
			case '=':
				nm &= ~m;
				nm |= b & m;
				break;
			}
		}
	} while (*ms++ == ',');
	if (*--ms) {
		fprintf(stderr, "%s: invalid mode\n", pname);
		exit(255);
	}
	return(nm);
}

abs()
{
	register c, i;

	i = 0;
	while ((c = *ms++) >= '0' && c <= '7')
		i = (i << 3) + (c - '0');
	ms--;
	return(i);
}

who()
{
	register m;

	m = 0;
	for (;;) switch (*ms++) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		ms--;
		if (m == 0)
			m = ALL;
		return m;
	}
}

what()
{
	switch (*ms) {
	case '+':
	case '-':
	case '=':
		return *ms++;
	}
	return(0);
}

where(om)
register om;
{
	register m;

	m = 0;
	switch (*ms) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		m &= (P_READ|P_WRITE|P_EXEC);
		m |= (m << 3) | (m << 6);
		++ms;
		return m;
	}
	for (;;) switch (*ms++) {
	case 'r':
		m |= P_READ;
		continue;
	case 'w':
		m |= P_WRITE;
		continue;
	case 'x':
		m |= P_EXEC;
		continue;
	case 's':
		m |= P_SETID;
		continue;
	case 't':
		m |= P_STICKY;
		continue;
	default:
		ms--;
		return m;
	}
}
