/* $Revision: 66.2 $ */
/* wrtmsg(user, msg) -- write message to user's tty if logged in.
	return codes: TRUE ==> success,
		      FALSE ==> failure
*/

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 16					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#include	"lpsched.h"
#ifdef TRUX
#include <sys/security.h>
#endif

#ifdef REMOTE
wrtmsg(user, from, msg)
char *user;
char *from;
char *msg;
#else REMOTE
wrtmsg(user,msg)
char *user;
char *msg;
#endif REMOTE
{
	char *tty, *findtty();
	FILE *f;
	int sigalrm();
	unsigned alarm(), sleep();

#ifdef SecureWare
        if ((ISSECURE) ? (!lp_can_send(user, &f)) :
	    ((tty = findtty(user)) == NULL || (f = fopen(tty, "w")) == NULL))
#else
	if((tty = findtty(user)) == NULL || (f = fopen(tty, "w")) == NULL)
#endif
		return(FALSE);
	signal(SIGALRM, sigalrm);
	alarm(10);
	fputc(BEL, f);
	fflush(f);
	sleep(2);
	fputc(BEL, f);
#ifdef REMOTE
	fprintf(f, (nl_msg(1, "\nlp@%s: %s\n")), from, msg);
#else REMOTE
	fprintf(f, (nl_msg(1, "\nlp: %s\n")), msg);
#endif REMOTE
	alarm(0);
#ifdef SecureWare
	if(ISSECURE)
        	return lp_end_msg(f);
	else{
	    fclose(f);
	    return(TRUE);
	}
#else
	fclose(f);
	return(TRUE);
#endif
}

/* sigalrm() -- catch SIGALRM */

static int
sigalrm()
{
}

/*
 *	wrtremote(user, host, mesg) -- write message to user's tty 
 *		both local and remote user
 */

wrtremote(user, host, mesg)
char	*user, *host, *mesg;
{
#ifdef REMOTE
	int	status, pid;

	for(;(pid = fork()) == -1;)
	    ;

	if(pid){
		for(;wait(&status) != pid;)
		    ;
		if(!status)
			return(TRUE);
		else
			return(FALSE);
	}else{
		execlp("/usr/lib/rwrite", "rwrite", user, host, mesg, NULL);
		exit(1);
	}
#endif REMOTE
}
