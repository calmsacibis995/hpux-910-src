/* @(#) $Revision: 70.1 $ */    

#include <stdio.h>
#include "lifdef.h"
#include "global.h"

char *getenv();
long lseek();

extern int errno;

/* returns number of characters read or -1 in case of errors */ 
			
int maxsize = -1;

unitread(fd, buffer, nbyte, where, routine)
register int fd;
register char *buffer;
register int nbyte;
register int where;
register char *routine;
{
	register int i, n, total;
	char kbuf[1024];

	bugout("unitread: called from %s nbyte %d where %d", 
			routine, nbyte, where);
	/*
	 * If the user variable LIFMAXREAD is set,
	 * clip all reads to that size.
	 */
	if (maxsize==-1) {			/* looked in env yet? */
		char *s;
		s=getenv("LIFMAXREAD");		/* look for limit */
		if (s==NULL)			/* if limit not found */
			maxsize=K256;		/* limit us to 256K reads */
		else				/* if limit given: */
			maxsize=atoi(s);	/* set limit */
	}

	if (nbyte > maxsize)			/* if trying to get overmuch */
		nbyte = maxsize;		/* clip to maximum read */

	total = 0;				/* how much read so far */

	/* read in first non-k segment */
	if (where & 1023) {		/* not starting on 1k boundary */
		int old=where;
		where &= ~1023;
		n = read_blocks(fd, kbuf, 1024, where, routine);
		if (n == -1) return -1;
		where += 1024;			/* further up */
		for (i=old & 1023; i<1024 && nbyte>0; i++) {
			total++;
			nbyte--;
			*buffer++ = kbuf[i];
		}
		if (n!=1024)			/* if read truncated */
			return total;		/* exit now */
	}

	/* read main bunch */
	if (nbyte>=1024) {
		int count;

		count = nbyte & ~1023;
		n = read_blocks(fd, buffer, count, where, routine);
		if (n==-1) return -1;
		total += n;
		nbyte -= n;
		where += n;			/* further up */
		buffer += n;
		if (n!=count)			/* if read truncated */
			return total;		/* exit now */
	}

	/* read trailing stuff */
	if (nbyte<0 || nbyte>=1024) {
		fprintf(stderr, "unitread error: nbyte=%d ?\n", nbyte);
		exit(1);
	}

	if (nbyte>0) {
		n = read_blocks(fd, kbuf, 1024, where, routine);
		if (n==-1) return -1;
		if (n>=nbyte)			/* if we read lots */
			n = nbyte;		/* say we read just enough */

		total+=n;
		for (i=0; i<n; i++)
			*buffer++ = kbuf[i];
	}

	return total;
}


read_blocks(fd, buffer, nbyte, where, routine)
register int fd;
register char *buffer;
register int nbyte;
register int where;
register char *routine;
{
	register int n;

	bugout("read_blocks: %d bytes from %d", nbyte, where);
		
	if ((nbyte & 1023) || (where & 1023)) {
		fprintf(stderr, "read_blocks: nbyte=%d where=%d not aligned\n",
			nbyte, where);
		exit(4);
	}

	n = lseek(fd, where, 0);
	if (n != where) {
		perror("lseek");
		fprintf(stderr, "read_blocks(%s): sought to %d not %d\n", 
			routine, n, where);
		return -1;
	}

	n = read(fd, buffer, nbyte);
	if (n==-1) {
		perror("read");
		fprintf(stderr, "read_blocks(%s): error on %d byte write\n",
			routine, nbyte);
		return -1;
	}
	return n;		/* read may have truncated, fine */
}


/*
 * Close a file, eliminate cacheing (if any).
 */
closeit(fd)
register int fd;
{
	register int n;

	n = close(fd);
	if (n==-1)
		perror("close");
	return n;
}
