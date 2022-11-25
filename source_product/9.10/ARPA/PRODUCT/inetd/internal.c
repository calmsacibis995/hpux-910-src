/*
** Copyright 1990 (C) Hewlett Packard Corporation
**
** Portions are:
**
** Copyright (c) 1983 Regents of the University of California.
** All rights reserved.  The Berkeley software License Agreement
** specifies the terms and conditions for redistribution.
**
** from 1.38 88/02/07 SMI; from UCB 5.14 (Berkeley) 1/23/89
*/

#ifndef lint
static char rcsid[] = "$Header: internal.c,v 1.2.109.1 91/11/21 12:00:56 kcs Exp $";
#endif /* ~lint */


#include "inetd.h"
#include <sys/pstat.h>

#ifdef BFA
static void
die()
{
	exit(0);
}

catchpipe()
{
	struct sigvec vec, ovec;

	vec.sv_mask = SIGBLOCK;
	vec.sv_handler = die;
	vec.sv_onstack = 0;
	sigvector(SIGPIPE, &vec, NULL);
}
#endif BFA

/*
**
** SETPROCTITLE
**
**    Internal services set their process title so that they
**    don't look like multiple instances of inetd.
**
*/

setproctitle(a, s)
	char *a;
	int s;
{
	static char buf[80], pbuf[80];

	strcpy(buf, " -");
	strcat(buf, a);
	strcat(buf, " [");
	strcat(buf, remoteaddr);
	strcat(buf, "] ");
#ifdef SETPROCTITLE
#ifdef PSTAT_SETCMD
	strcpy(pbuf, Argv[0]);
	strcat(pbuf, buf);
	pstat(PSTAT_SETCMD, pbuf, strlen(pbuf), 0, 0);
#else
	/* pre-8.0 pstat */
	Argv[1] = buf;
	pstat_set_command(Argv);
#endif /* PSTAT_SETCMD */
#endif /* SETPROCTITLE */
}



/*
 * Internet services provided internally by inetd:
 */

/* ARGSUSED */
echo_stream(s, sep)		/* Echo service -- echo data back */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZ];
	int i;

#ifdef BFA
	catchpipe();
#endif
	setproctitle(sep->se_service, s);
	while ((i = read(s, buffer, sizeof(buffer))) > 0 &&
	    write(s, buffer, i) > 0)
		;
	exit(0);
}

/* ARGSUSED */
echo_dg(s, sep)			/* Echo service -- echo data back */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZ];
	int i, size;
	struct sockaddr sa;

	size = sizeof(sa);
	if ((i = recvfrom(s, buffer, sizeof(buffer), 0, &sa, &size)) < 0)
		return;
	(void) sendto(s, buffer, i, 0, &sa, sizeof(sa));
}



/* ARGSUSED */
discard_stream(s, sep)		/* Discard service -- ignore data */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZ];

#ifdef BFA
	catchpipe();
#endif
	setproctitle(sep->se_service, s);
	while (1) {
		while (read(s, buffer, sizeof(buffer)) > 0)
			;
		if (errno != EINTR)
			break;
	}
	exit(0);
}

/* ARGSUSED */
discard_dg(s, sep)		/* Discard service -- ignore data */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZ];

	(void) read(s, buffer, sizeof(buffer));
}



#include <ctype.h>
#define LINESIZ 72
char ring[128];
char *endring;

initring()
{
	register int i;

	endring = ring;

	for (i = 0; i <= 128; ++i)
		if (isprint(i))
			*endring++ = i;
}

/* ARGSUSED */
chargen_stream(s, sep)		/* Character generator */
	int s;
	struct servtab *sep;
{
	register char *rs;
	int len;
	char text[LINESIZ+2];

#ifdef BFA
	catchpipe();
#endif
	setproctitle(sep->se_service, s);

	if (!endring) {
		initring();
		rs = ring;
	}

	text[LINESIZ] = '\r';
	text[LINESIZ + 1] = '\n';
	for (rs = ring;;) {
		if ((len = endring - rs) >= LINESIZ)
			bcopy(rs, text, LINESIZ);
		else {
			bcopy(rs, text, len);
			bcopy(ring, text + len, LINESIZ - len);
		}
		if (++rs == endring)
			rs = ring;
		if (write(s, text, sizeof(text)) != sizeof(text))
			break;
	}
	exit(0);
}

/* ARGSUSED */
chargen_dg(s, sep)		/* Character generator */
	int s;
	struct servtab *sep;
{
	struct sockaddr sa;
	static char *rs;
	int len, size;
	char text[LINESIZ+2];

	if (endring == 0) {
		initring();
		rs = ring;
	}

	size = sizeof(sa);
	if (recvfrom(s, text, sizeof(text), 0, &sa, &size) < 0)
		return;

	if ((len = endring - rs) >= LINESIZ)
		bcopy(rs, text, LINESIZ);
	else {
		bcopy(rs, text, len);
		bcopy(ring, text + len, LINESIZ - len);
	}
	if (++rs == endring)
		rs = ring;
	text[LINESIZ] = '\r';
	text[LINESIZ + 1] = '\n';
	(void) sendto(s, text, sizeof(text), 0, &sa, sizeof(sa));
}



/*
 * Return a machine readable date and time, in the form of the
 * number of seconds since midnight, Jan 1, 1900.  Since gettimeofday
 * returns the number of seconds since midnight, Jan 1, 1970,
 * we must add 2208988800 seconds to this figure to make up for
 * some seventy years Bell Labs was asleep.
 */

long
machtime()
{
	struct timeval tv;

	if (gettimeofday(&tv, (struct timezone *)0) < 0) {
		fprintf(stderr, "Cannot get time of day\n");
		return (0L);
	}
	return (htonl((long)tv.tv_sec + 2208988800));
}

/* ARGSUSED */
machtime_stream(s, sep)
	int s;
	struct servtab *sep;
{
	long result;

	result = machtime();
	(void) write(s, (char *) &result, sizeof(result));
}

/* ARGSUSED */
machtime_dg(s, sep)
	int s;
	struct servtab *sep;
{
	long result;
	struct sockaddr sa;
	int size;

	size = sizeof(sa);
	if (recvfrom(s, (char *)&result, sizeof(result), 0, &sa, &size) < 0)
		return;
	result = machtime();
	(void) sendto(s, (char *) &result, sizeof(result), 0, &sa, sizeof(sa));
}


/* ARGSUSED */
daytime_stream(s, sep)		/* Return human-readable time of day */
	int s;
	struct servtab *sep;
{
	char buffer[256];
	time_t time(), clock;
	char *ctime();

	clock = time((time_t *) 0);

	(void) sprintf(buffer, "%.24s\r\n", ctime(&clock));
	(void) write(s, buffer, strlen(buffer));
}

/* ARGSUSED */
daytime_dg(s, sep)		/* Return human-readable time of day */
	int s;
	struct servtab *sep;
{
	char buffer[256];
	time_t time(), clock;
	struct sockaddr sa;
	int size;
	char *ctime();

	clock = time((time_t *) 0);

	size = sizeof(sa);
	if (recvfrom(s, buffer, sizeof(buffer), 0, &sa, &size) < 0)
		return;
	(void) sprintf(buffer, "%.24s\r\n", ctime(&clock));
	(void) sendto(s, buffer, strlen(buffer), 0, &sa, sizeof(sa));
}

