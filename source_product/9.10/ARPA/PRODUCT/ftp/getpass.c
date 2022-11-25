/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getpass.c	based on 5.3 (Berkeley) 9/22/88";
static char rcsid[] = "$Header: getpass.c,v 1.4.109.1 91/11/21 11:49:48 kcs Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/ioctl.h>
#include <sys/signal.h>
#include <stdio.h>
#ifdef hpux
#include <sgtty.h>
#endif

char *
getlongpass(prompt)
	char *prompt;
{
	struct sgttyb ttyb;
	register int ch;
	register char *p;
	FILE *fp, *outfp;
	long omask;
	int svflagval;
#define	PASSWD_LEN	128
	static char buf[PASSWD_LEN + 1];

	/*
	 * read and write to /dev/tty if possible; else read from
	 * stdin and write to stderr.
	 */
#ifdef hpux
	outfp = stderr;
	if ((fp = fopen("/dev/tty", "r")) == NULL) {
		fp = stdin;
	}
	else
		setbuf(fp, (char*)NULL);
#else
	if ((outfp = fp = fopen("/dev/tty", "w+")) == NULL) {
		outfp = stderr;
		fp = stdin;
	}
#endif

	(void)ioctl(fileno(fp), TIOCGETP, &ttyb);
	svflagval = ttyb.sg_flags;
	ttyb.sg_flags &= ~ECHO;
	omask = sigblock(sigmask(SIGINT));
	(void)ioctl(fileno(fp), TIOCSETP, &ttyb);

	fputs(prompt, outfp);
	rewind(outfp);			/* implied flush */
	for (p = buf; (ch = getc(fp)) != EOF && ch != '\n';)
		if (p < buf + PASSWD_LEN)
			*p++ = ch;
	*p = '\0';
	(void)write(fileno(outfp), "\n", 1);

	ttyb.sg_flags = svflagval;
	(void)ioctl(fileno(fp), TIOCSETP, &ttyb);
	(void)sigsetmask(omask);
	if (fp != stdin)
		(void)fclose(fp);
	return(buf);
}

