/*	@(#)dbm.h	$Revision: 1.15.109.1 $	$Date: 91/11/19 14:42:20 $  */

/*      Copyright 1987 Hewlett-Packard Company  */
/*      Copyright 1985 Sun Microsystems, Inc.  */

#define	PBLKSIZ	1024
#define	DBLKSIZ	4096
#define	BYTESIZ	8
#define	NULL	((char *) 0)

long	bitno;
long	maxbno;
long	blkno;
long	hmask;

char	pagbuf[PBLKSIZ];
char	dirbuf[DBLKSIZ];

int	dirf;
int	pagf;
int	dbrdonly;

typedef	struct
{
	char	*dptr;
	int	dsize;
} datum;

datum	fetch();
datum	makdatum();
datum	firstkey();
datum	nextkey();
datum	firsthash();
long	calchash();
long	hashinc();

