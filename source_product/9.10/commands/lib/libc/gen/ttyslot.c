/* @(#) $Revision: 64.5 $ */      
/*LINTLIBRARY*/
/*
 * Return the number of the slot in the utmp file
 * corresponding to the current user: try for file 0, 1, 2.
 * Returns -1 if slot not found.
 */
#ifdef _NAMESPACE_CLEAN
#define ttyname _ttyname
#define strrchr _strrchr 
#define strncmp _strncmp 
#define open    _open 
#define read    _read 
#define close   _close
#define ttyslot _ttyslot
#endif
 
#include <sys/types.h>
#include <utmp.h>
#define	NULL	0

extern char *ttyname(), *strrchr();
extern int strncmp(), open(), read(), close();

#ifdef _NAMESPACE_CLEAN
#undef ttyslot
#pragma _HP_SECONDARY_DEF _ttyslot ttyslot
#define ttyslot _ttyslot
#endif

int
ttyslot()
{
	struct utmp ubuf;
	register char *tp, *p;
	register int s, fd;
	int saved_entry = -1;

/* the saved_entry was added to support returning a non-dead entry first.
   if there were no non-dead entries, it would return the first available
   dead entry as it was doing before.
*/ 

	if((tp=ttyname(0)) == NULL && (tp=ttyname(1)) == NULL &&
					(tp=ttyname(2)) == NULL)
		return(-1);

	if (strncmp(tp,"/dev/",5) == 0)
		p = tp + 5;
	else
		p = tp;

	if((fd=open(UTMP_FILE, 0)) < 0)
		return(-1);
	s = 0;
	while(read(fd, (char*)&ubuf, sizeof(ubuf)) == sizeof(ubuf)) {
		if ( strncmp(p, ubuf.ut_line, sizeof(ubuf.ut_line)) == 0) {
			if (ubuf.ut_type == INIT_PROCESS ||
                            ubuf.ut_type == LOGIN_PROCESS ||
                            ubuf.ut_type == USER_PROCESS ) {
				 (void) close(fd);
				        return(s);
                        } else  {
                                 if (    ubuf.ut_type == DEAD_PROCESS  &&
                                         saved_entry < 0) 
						saved_entry = s;
				}
		}
		s++;
	}
	(void) close(fd);
	return(saved_entry);
}
