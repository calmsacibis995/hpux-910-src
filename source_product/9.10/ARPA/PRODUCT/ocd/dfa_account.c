#ifndef lint
static char rcsid[] = "$Header: dfa_account.c,v 1.1.109.2 92/04/28 17:45:33 charlie Exp $";
#endif /* ~line */

/*
 * routine used by both telnetd and rlogind for utmp accounting.
 * modified for use by ocd. NOT FOR USE BY telnetd NOR rlogind
 *
 * 28-Apr-92 - Declare variable 'slave' as extern variable.  null_slave
 *             is just a place holder since nothing is passed in.  This
 *             is a fix for an SR.                                  cc
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <utmp.h>

/*** The account routine below replaces the rmut routine that was **/
/*** used here.  This routine is slightly more general in that it **/
/*** can be used to either create a utmp entry, or to mark it dead **/
/*** both which we need to do.  We create the utmp entry in the daemon **/
/*** becuase the HP-UX login will not succeed if there is not one there. **/

/*** the basis of the account routine was stolen from the ***/
/*** pty daemon written by John Marvin.  The extra flags arguement **/
/*** was deleted since we want to always to the same things ***/
/*** also added a call to endutent() which was not there. dds. ***/

#define FALSE 0
#define TRUE  1
#define DEVOFFSET  5            /* number of characters in "/dev/"     */

dfa_account(type,pid,user,null_slave,hostname,hostaddr)
    short type,pid;
    char  *user,*null_slave;
    char *hostname;
    struct sockaddr_in *hostaddr;
{
    struct utmp utmp, *oldu;
    extern struct utmp *_pututline(), *getutid();
    extern char slave[];
    FILE *fp;
    int writeit;
    char *s;
    char *strrchr();

    /** open utmp file ***/

    setutent();

    /* initialize utmp structure */

    memset(utmp.ut_user,'\0',sizeof(utmp.ut_user));
    memset(utmp.ut_line,'\0',sizeof(utmp.ut_line));
    /*
     * in ut_line, ocd needs the pseudonym passed in user parameter
     */
    strncpy(utmp.ut_user,"ocd",sizeof(utmp.ut_user));
    strncpy(utmp.ut_line,user ,sizeof(utmp.ut_line));
    utmp.ut_pid  = pid;
    utmp.ut_type = type;

    s = strrchr(slave,'/') + 1; /* Must succeed since slave_pty */
				/* begins with /dev/.           */
    if (strncmp(s,"tty",3) == 0)
	s += 3;
    memset(utmp.ut_id, '\0', sizeof(utmp.ut_id));
    strncpy(utmp.ut_id, s, sizeof(utmp.ut_id));

    /* host information */
    if (hostname != NULL) {
	    strncpy(utmp.ut_host, hostname, sizeof(utmp.ut_host));
	    utmp.ut_addr = hostaddr->sin_addr.s_addr;
    }
    else {
	    memset(utmp.ut_host, '\0', sizeof (utmp.ut_host));
	    utmp.ut_addr = 0;
    }

    utmp.ut_exit.e_termination = 0;
    utmp.ut_exit.e_exit = 0;
    (void) time(&utmp.ut_time);

    setutent();     /* Start at beginning of utmp file. */
    writeit = TRUE;
    if (type == DEAD_PROCESS) {

	if ((oldu = getutid(&utmp)) != (struct utmp *)0) {

	       /* Copy in the old "user" and "line" fields */
	       /* to our new structure.                    */
		memcpy(&utmp.ut_user[0],&oldu->ut_user[0],sizeof(utmp.ut_user));
		memcpy(&utmp.ut_line[0],&oldu->ut_line[0],sizeof(utmp.ut_line));
	}
	else
	    writeit = FALSE;
    }
    if (writeit == FALSE || _pututline(&utmp) == (struct utmp *)0){
	    endutent();
	    return(0);
    }


    /*
    **	close utmp file and return ...
    */
    endutent();
    return(0);
}
