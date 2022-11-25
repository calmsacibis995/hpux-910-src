/* $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/note.c,v $
 * $Revision: 1.7 $		$Author: dah $
 * $State: Exp $		$Locker:  $
 * $Date: 86/05/12 16:01:13 $
 *
 * $Log:	note.c,v $
 * Revision 1.7  86/05/12  16:01:13  16:01:13  dah
 * use usecdiff (completely homegrown) instead of ms_subsec
 * 
 * Revision 1.6  86/01/27  16:55:15  dah (Dave Holt)
 * fix typo
 * 
 * Revision 1.4  85/12/11  11:23:33  dah (Dave Holt)
 * some progress in linting
 * 
 * Revision 1.3  85/12/06  16:55:55  dah (Dave Holt)
 * try handling note.h
 * 
 * Revision 1.2  85/12/03  16:41:40  dah (Dave Holt)
 * added standard firstci header
 * 
 * $Endlog$
 */

/*
 *  Dave Holt
 *  designed to take timing measurements between points of iscan
 */

/*LINTLIBRARY*/

#include <sys/types.h>
#include <stdio.h>
#include <sys/time.h>

#define MAXINT	((1 << (sizeof(int)*8 -1)) -1)


#define EVENTS	10

struct info {
    int min;
    int max;
    int avg;
    int count;
};
    
struct info ia[EVENTS][EVENTS];

int noting = 0;			/* by default, no noting */
int last_event = -1;
struct timeval last_time;

note_time(event)
int event;
{
    struct timeval tv;
    struct timezone tz;
    int diff, i, j;
    
    if ((event < 0) || (event >= EVENTS)) {
	fprintf(stderr, "note_time: bad event # = %d\n", event);
	exit(1);
    }
    
    if (gettimeofday(&tv,&tz) != 0)
        perror("gettimeofday");
    
    if (last_event == -1) {
 	for (i = 0; i < EVENTS; i++) {
	    for (j = 0; j < EVENTS; j++) {
		ia[i][j].min = MAXINT;
		ia[i][j].max = 0;
		ia[i][j].avg = 0;
		ia[i][j].count = 0;
 	    }
	}
    } else {
	diff = usecdiff(last_time, tv);
	note_delta(&(ia[last_event][event]), diff);	
    }

    last_event = event;
    last_time = tv;
}

note_delta(ip, diff)
struct info *ip;
int diff;
{
    if (diff < ip->min)
 	ip->min = diff;
    
    if (diff > ip->max)
        ip->max = diff;
    
    (ip->count)++;
    
    ip->avg += ((diff - ip->avg) / ip->count);
}

note_posts()
{
    int i, j;

    (void) "$Header: note.c,v 1.7 86/05/12 16:01:13 dah Exp $";		/* ident'ify our .o file */
                                /* here is as good a place as any */

    for (i = 0; i < EVENTS; i++) {
	for (j = 0; j < EVENTS; j++) {
	    if (ia[i][j].count > 0) {
	      fprintf(stderr, 
		"(%2d to %2d): count = %5d,  min=%10d,  max=%10d,  avg=%10d\n",
	    	i, j, ia[i][j].count, 
		ia[i][j].min, ia[i][j].max, ia[i][j].avg);
	    }
        }
    }
}

/*
 * usecdiff returns the difference between old and new in microseconds
 * old and new are timevals passed by value.
 * if old > new, or the difference is > ~20 minutes (won't fit in 32 bits),
 * -1 is retured
 */
#define MS_MAXINT	((1 << (sizeof(int)*8 -1)) -1)
#define MS_MAXSEC	((MS_MAXINT -999999) / 1000000)
int 
usecdiff(old, new)
    struct timeval old, new;
{
    struct timeval diff, tv_sub();
    int us;

    if (!earlier(old, new)) {
	fprintf(stderr,
		"usecdiff: old not earlier than new: %d.%06d, %d.%06d\n",
		old.tv_sec, old.tv_usec, new.tv_sec, new.tv_usec);
	return(-1);
    }

    diff = tv_sub(new, old);

    if (diff.tv_sec > MS_MAXSEC) {	/* can't be represented as an int */
	fprintf(stderr, "usecdiff: delta t is too big!");
	return(-1);
    }

    us = diff.tv_usec + (diff.tv_sec * 1000000);
    return(us);
}

/* 
 * Is timeval 'a' earlier than timeval 'b'?  
 * a and b must be normalized (i.e. usecs must be positive).
 */
int 
earlier(a, b)
	struct timeval a, b;
{
    if ( (b.tv_sec > a.tv_sec) ||
	 ((b.tv_sec == a.tv_sec) && (b.tv_usec > a.tv_usec))
       ) {
	return(1);
    } else {
	return(0);
    }
}

/*
 * return (a - b) as a timeval
 * a and b must be normalized (i.e. usecs must be positive).
 */
struct timeval 
tv_sub(a, b)
	struct timeval a, b;
{
    struct timeval diff;

    diff.tv_sec = a.tv_sec - b.tv_sec;
    diff.tv_usec = a.tv_usec - b.tv_usec;
    if (diff.tv_usec < 0) {	/* normalize */
	diff.tv_usec += 1000000;
	diff.tv_sec--;
    }
    return(diff);
}
