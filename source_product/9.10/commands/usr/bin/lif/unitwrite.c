/* @(#) $Revision: 56.1 $ */   

#include <stdio.h>
#include "lifdef.h"
#include "global.h"

long lseek();

extern int errno;
extern char *Fatal[];

/* returns number of characters written or -1 in case of errors */ 
			
unitwrite(fd, buffer, nbyte, where, routine)
register int fd;
register char *buffer;
register int nbyte;
register int where;
register char *routine;
{
	register int i, n, total;
	char kbuf[1024];

	bugout("unitwrite: called from %s nbyte %d where %d", 
			routine, nbyte, where);

	total = 0;				/* how much written so far */

	/* write out first non-k segment */
	if (where & 1023) {		/* not starting on 1k boundary */
		int old=where;
		where &= ~1023;
		n = read_blocks(fd, kbuf, 1024, where, routine);
		if (n==-1) return -1;
		for (i=n; i<1024; i++)
			kbuf[i]=0;
		for (i=old & 1023; i<1024 && nbyte>0; i++) {
			total++;
			nbyte--;
			kbuf[i] = *buffer++;
		}
		n = write_blocks(fd, kbuf, 1024, where, routine);
		if (n != 1024) return -1;
		where += 1024;			/* further up */
	}

	/* write main bunch */
	if (nbyte>=1024) {
		int count;

		count = nbyte & ~1023;
		n = write_blocks(fd, buffer, count, where, routine);
		if (n!=count) return -1;
		total += n;
		nbyte -= n;
		where += n;			/* further up */
		buffer += n;
	}

	/* write trailing stuff */
	if (nbyte<0 || nbyte>=1024) {
		fprintf(stderr, "unitwrite error: nbyte=%d ?\n", nbyte);
		exit(1);
	}

	if (nbyte>0) {
		n = read_blocks(fd, kbuf, 1024, where, routine);
		if (n==-1) return -1;
		for (i=n; i<1024; i++)
			kbuf[i]=0;
		for (i=0; i<nbyte; i++)
			kbuf[i] = *buffer++;
		n = write_blocks(fd, kbuf, 1024, where, routine);
		if (n!=1024) return -1;
		total+=nbyte;
	}

	return total;
}


write_blocks(fd, buffer, nbyte, where, routine)
register int fd;
register char *buffer;
register int nbyte;
register int where;
register char *routine;
{
	register int n;

	bugout("write_blocks: %d bytes to %d", nbyte, where);
		
	if ((nbyte & 1023) || (where & 1023)) {
		fprintf(stderr, "write_blocks: nbyte=%d where=%d not aligned\n",
			nbyte, where);
		exit(4);
	}

	n = lseek(fd, where, 0);
	if (n != where) {
		perror("lseek");
		fprintf(stderr, "write_blocks(%s): sought to %d not %d\n", 
			routine, n, where);
		return -1;
	}

	n = write(fd, buffer, nbyte);
	if (n == -1) {
		perror("write");
		fprintf(stderr, "write_blocks(%s): error on %d byte write\n",
			routine, nbyte);
	} else if (n!=nbyte) {
		fprintf(stderr, "%s: file truncated, %s\n", routine, Fatal[NOSPACE]);
	}
	return n;
}
