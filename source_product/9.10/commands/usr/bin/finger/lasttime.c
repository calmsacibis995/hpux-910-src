/* HPUX_ID: @(#)lasttime.c	37.1     86/06/17 */
/*
 * lasttime
 */
#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <utmp.h>

#define NMAX	sizeof(buf[0].ut_name)
#define	nameq(a,b)	(!strncmp(a,b,NMAX))

struct	utmp buf[128];
char	*argv;

struct utmp *
lasttime(user, count)
	int count;
	char *user;
{
	int bl, wtmp;   
	int curcount;
	register struct utmp *bp;
	struct stat stb;
 
	argv = user;
	curcount=0;
	time(&buf[0].ut_time);
	wtmp = open("/etc/wtmp", 0);
	if (wtmp < 0) {
		perror("/etc/wtmp");
		exit(1);
	}
	fstat(wtmp, &stb);
	bl = (stb.st_size + sizeof (buf)-1) / sizeof (buf);
	for (bl--; bl >= 0; bl--) {
		lseek(wtmp, bl * sizeof (buf), 0);
		bp = &buf[read(wtmp, buf, sizeof (buf)) / sizeof(buf[0]) - 1];
		for ( ; bp >= buf; bp--) {
			if (want(bp)) {
				if (curcount == count) {
				    close(wtmp);
				    return(bp);
				}
				curcount++;
			}
		}
	}
	close(wtmp);
	return((struct utmp*)NULL);
}

want(bp)
	struct utmp *bp;
{
	register char *av;
	register int ac;

	if (bp->ut_name[0] == 0)
		return (0);
	av = argv;
	if (nameq(av, bp->ut_name))
	    return (1);
	return (0);
}
