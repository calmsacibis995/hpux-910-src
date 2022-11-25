/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 66.1 $";
#endif
/*
 *	Suck up system messages
 *	dmesg
 *		print current buffer
 *	dmesg -
 *		print and update incremental history
 */

#include <stdio.h>
#include <sys/types.h>
#include <nlist.h>
#include <sys/msgbuf.h>
#include <fcntl.h>

struct	msgbuf msgbuf;
char	*msgbufp;
int	sflg;
int	of	= -1;

struct	msgbuf omesg;
struct	nlist nl[2] = {
#ifdef hp9000s200
	{ "_Msgbuf" },
#else  /* hp9000s200 */
#  ifndef UNDERSCORE
	{ "msgbuf" },
#  else  /* UNDERSCORE */
	{ "_msgbuf" },
#  endif /* UNDERSCORE */
#endif /*  hp9000s200 */
	{ "" }
};

main(argc, argv)
char **argv;
{
	int mem;
	register char *mp, *omp, *mstart;
	int samef, sawnl, ignore;

	if (argc>1 && argv[1][0] == '-') {
		sflg++;
		argc--;
		argv++;
	}
	if (sflg) {
		of = open("/usr/adm/msgbuf", O_RDWR | O_CREAT, 0644);
		if (of < 0)
			done("Can't open /usr/adm/msgbuf\n");
		read(of, (char *)&omesg, sizeof(omesg));
		lseek(of, 0L, 0);
	}
	sflg = 0;
#ifdef	hpux
	nlist(argc>2? argv[2]:"/hp-ux", nl);
#else  /* !hpux */
	nlist(argc>2? argv[2]:"/vmunix", nl);
#endif /* !hpux */
	if (nl[0].n_type==0)
		done("Can't get kernel namelist\n");
	if ((mem = open((argc>1? argv[1]: "/dev/kmem"), 0)) < 0)
		done("Can't read kernel memory\n");
	lseek(mem, (long)nl[0].n_value, 0);
	read(mem, &msgbuf, sizeof (msgbuf));
	if (msgbuf.msg_magic != MSG_MAGIC)
		done("Magic number wrong (namelist mismatch?)\n");
	if (msgbuf.msg_bufx >= MSG_BSIZE)
		msgbuf.msg_bufx = 0;
	if (omesg.msg_bufx >= MSG_BSIZE)
		omesg.msg_bufx = 0;
	mstart = &msgbuf.msg_bufc[omesg.msg_bufx];
	omp = &omesg.msg_bufc[msgbuf.msg_bufx];
	mp = msgbufp = &msgbuf.msg_bufc[msgbuf.msg_bufx];
	samef = 1;
	do {
		if (*mp++ != *omp++) {
			mstart = msgbufp;
			samef = 0;
			pdate();
			fputs("...\n", stdout);
			break;
		}
		if (mp >= &msgbuf.msg_bufc[MSG_BSIZE])
			mp = msgbuf.msg_bufc;
		if (omp >= &omesg.msg_bufc[MSG_BSIZE])
			omp = omesg.msg_bufc;
	} while (mp != mstart);
	if (samef && omesg.msg_bufx == msgbuf.msg_bufx)
		exit(0);
	mp = mstart;
	pdate();
	sawnl = 1;
	do {
#ifdef	hpux
		if (*mp && (*mp & 0200) == 0)
			putchar(*mp);
#else  /* !hpux	 (4.3BSD code for logging) */
		if (sawnl && *mp == '<')
			ignore = 1;
		if (*mp && (*mp & 0200) == 0 && !ignore)
			putchar(*mp);
		if (ignore && *mp == '>')
			ignore = 0;
		sawnl = (*mp == '\n');
#endif /* hpux */
		mp++;
		if (mp >= &msgbuf.msg_bufc[MSG_BSIZE])
			mp = msgbuf.msg_bufc;
	} while (mp != msgbufp);
	done((char *)NULL);
}

done(s)
char *s;
{
	register char *p, *q;

	if (s) {
		pdate();
		fputs(s, stdout);
	} else if (of != -1)
		write(of, (char *)&msgbuf, sizeof(msgbuf));
	exit(s!=NULL);
}

pdate()
{
	extern char *ctime();
	static firstime;
	time_t tbuf;

	if (firstime==0) {
		char *s;

		firstime++;
		time(&tbuf);
		s = ctime(&tbuf) + 3;
		s[0]  = '\n';
		s[13] = '\n';
		s[14] = '\0';
		fputs(s, stdout);
	}
}
