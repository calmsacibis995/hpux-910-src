/* @(#) $Revision: 66.5 $ */   
/*
	These routines are used to create a compacted macro file
	(read: pre-processed macro file) when the INCORE compile
	time flag is set.  Basically, a core dump of the data
	segment is written to and restored from a file.

	Please realize that this is an ugly solution to an ugly
	problem in ugly code (nroff).  As such, it is extremely
	non-portable.

	This code cannot be linked with shared libraries!!!

	Edgar Circenis
	UDL - 10/23/90
*/

#include "tdef.hd"
#if !defined(NOCOMPACT) && defined(INCORE)
#include "stdio.h"

extern int frozen;
extern int version,mversion;

#if defined(__hp9000s800) && defined(BSDPORT)
extern char __data_start;
#endif
#if defined(__hp9000s800) && defined(comet)
#include <sys/param.h>
#endif
#if defined(__hp9000s300)	 /* note that this is 300 only */
#include <machine/param.h>
#define PMASK (NBPG-1)                  /* s300 pagesize - 1 */
extern int etext;                       /* end of text segment */
#endif
freeze(s) char *s;
{	int fd;
	char *base, *top;
	unsigned int len;

#if defined(__hp9000s800) || defined(__hp9000s300)
# ifdef BSDPORT
	base = &__data_start;
# else
#ifdef __hp9000s300
	base = (char *) (((int) &etext + PMASK) & ~PMASK);
		/*
		    for the 300, the data segment begins on the
		    page boundary following the end of the text
		    segment, so it is necessary to round up
		*/
#else
	base = (char *)USRDATA;
# endif
# endif
	top = (char *)sbrk(0);
	len = top - base;
	if((fd = creat(s, 0666)) < 0) {
		perror(s);
		return(1);
	}
	/* mimic old compressed macro behavior by using version stamps */
	write(fd, &version, sizeof(version));
	write(fd, &mversion, sizeof(mversion));
	write(fd, &len, sizeof(len));
	write(fd, base, len);
	close(fd);
	return(0);
#else
	/* not implemented on other machines */
	return(1);
#endif
}

thaw(s) char *s;
{	int fd;
	char *base;
	unsigned int len;
	int tversion;
	char *oldbrk;

#if defined(__hp9000s800) || defined(__hp9000s300)
# ifdef BSDPORT
	base = &__data_start;
# else
#ifdef __hp9000s300
	base = (char *) (((int) &etext + PMASK) & ~PMASK);
		    /* round up to next page */
#else
	base = (char *)USRDATA;
# endif
# endif
	if(*s == 0) {
		return(1);
	}
	if((fd = open(s, 0)) < 0) {
		perror(s);
		return(1);
	}
	if ((read(fd, &tversion, sizeof(tversion)) != sizeof(tversion)) ||
	    (tversion != version)) {
		close(fd);
		return(1);
	}
	if ((read(fd, &tversion, sizeof(tversion)) != sizeof(tversion)) ||
	    (tversion != mversion)) {
		close(fd);
		return(1);
	}
	if (read(fd, &len, sizeof(len)) != sizeof(len)) {
		close(fd);
		return(1);
	}
	oldbrk = (char *) sbrk(0);
	if (brk( (char *) (len + base)) < 0) {
		close(fd);
		return(1);
	}
			/*
			    Note: changed this from brk(len) to
			    brk(base + len) because not all machines'
			    data segments start at address 0. In
			    particular, the 300. For machines with
			    a base of 0, this change will have no effect.
			*/
	if (read(fd, base, len) != len) {
		brk(oldbrk);
		close(fd);
		return(1);
	}
	close(fd);
	return(0);
#else
	/* not implemented on other machines */
	return(1);
#endif
}

#endif /* !defined(NOCOMPACT) && defined(INCORE) */
