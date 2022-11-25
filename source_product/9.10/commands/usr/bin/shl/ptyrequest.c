/* @(#) $Revision: 29.1 $ */   
#include <sys/types.h>
#include <ndir.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "ptyrequest.h"

static void get_response();

#define NENVP 3 /* Number of additional positions in envp[] to allocate */


#define DEFAULT_SHELL "/bin/sh"
getshellpty(slave,master,addenvp, error)
    char *slave, *master, **addenvp;
    int  *error;
{
    extern char *strrchr(),*getenv();
    char *argv[2];
    char **envp;
    char argzero[DIRSIZ + 2];
    char shell[MAXARGSTRLEN];
    char *s;
    int slen;
    int i;
    extern char *malloc();

    if ((s = getenv("SHELL")) == (char *)0)
	s = DEFAULT_SHELL;

    strncpy(shell,s,MAXARGSTRLEN);

    if ((s = strrchr(shell,'/')) == (char *)0)
	s = shell;
    else
	s++;

    strcpy(argzero,"-");
    strncat(argzero,s,DIRSIZ);
    argzero[DIRSIZ + 1] = '\0';
    argv[0] = argzero;
    argv[1] = (char *)0;

    /* Count number of additional environment variables */

    i = 0;
    envp = addenvp;
    if (envp != (char **)0) {
	while (*envp != (char *)0) {
	    envp++;
	    i++;
	}
    }

    /* Allocate environment pointer array */

    envp = (char **) malloc( (i + NENVP) * sizeof(char *) );
    if (envp == (char **)0) {
	if (error != (int *)0)
	    *error = E_ALLOCERR;
	return(-1);
    }

    i = 0;
    if ((s = getenv("TERM")) == (char *)0)
	s = "unknown";

    if ( (envp[i] = malloc(6 + strlen(s))) != (char *)0 ) {
	strcpy(envp[i],"TERM=");
	strcat(envp[i++],s);
    }

    if ((s = getenv("TTYNAME")) != (char *)0) {
	if ( (envp[i] = malloc(9 + strlen(s))) != (char *)0 ) {
	    strcpy(envp[i],"TTYNAME=");
	    strcat(envp[i++],s);
	}
    }

    /* Add additional environment variables */

    if (addenvp != (char **)0) {
	while (*addenvp != (char *)0) {
	    envp[i] = *addenvp++;
	    i++;
	}
    }
    envp[i] = (char *)0;

    return(getpty(shell,argv,envp,(char *)0,(char *)0,CREATE_UTMP,
		  slave,master,error));
}

getloginpty(slave,master,error)
    char *slave, *master;
    int  *error;
{
    return(getpty(LOGIN_PROGRAM,(char **)0,(char **)0,(char *)0,(char *)0,
		  (PRINTISSUE|RESPAWN|CREATE_UTMP|CREATE_WTMP),
		  slave,master,error));
}

getpty(path,argv,envp,user,password,flags,slave,master,error)
    char *path;
    char **argv;
    char **envp;
    char *user;
    char *password;
    int  flags;
    char *slave, *master;
    int  *error;
{
    struct ptyrequest  reqpacket;
    struct ptyresponse respacket;
    int retval;
    int ptyfd;
    char *pool;
    int poolsize;
    int length;
    int i;

    reqpacket.myuid = geteuid();
    reqpacket.mygid = getegid();
    reqpacket.flags = flags;

    pool = reqpacket.pool;
    poolsize = 0;

    reqpacket.pathoffset = poolsize;
    if (path == (char *)0)
	reqpacket.pathlength = 0;
    else {
	if ((i = strlen(path)) >= (POOLSIZE - poolsize)) {
	    if (error != (int *)0)
		*error = E_POOLERR;
	    return(-1);
	}
	i++; /* Allow for space taken by terminating null */
	strcpy(pool,path);
	reqpacket.pathlength = i;
	poolsize += i;
	pool += i;
    }

    reqpacket.argvoffset = poolsize;
    length = 0;
    if (argv != (char **)0) {

	while (*argv != (char *)0) {
	    if ((i = strlen(*argv)) >= (POOLSIZE - poolsize)) {
		if (error != (int *)0)
		    *error = E_POOLERR;
		return(-1);
	    }
	    i++; /* Allow for space taken by terminating null */
	    strcpy(pool,*argv);
	    length += i;
	    pool += i;
	    argv++;
	}
    }

    reqpacket.argvlength = length;
    poolsize += length;

    reqpacket.environoffset = poolsize;
    length = 0;
    if (envp != (char **)0) {

	while (*envp != (char *)0) {
	    if ((i = strlen(*envp)) >= (POOLSIZE - poolsize)) {
		if (error != (int *)0)
		    *error = E_POOLERR;
		return(-1);
	    }
	    i++; /* Allow for space taken by terminating null */
	    strcpy(pool,*envp);
	    length += i;
	    pool += i;
	    envp++;
	}
    }

    reqpacket.environlength = length;
    poolsize += length;

    if (user == (char *)0)
	user = "";
    strncpy(reqpacket.user,user,USERLEN);
    reqpacket.user[USERLEN - 1] = '\0';

    if (password == (char *)0)
	password = "";
    strncpy(reqpacket.password,password,PASSWORDLEN);
    reqpacket.password[PASSWORDLEN - 1] = '\0';

    if ((retval = send_request(&reqpacket)) != 0) {
	if (error != (int *)0)
	    *error = retval;
	return(-1);
    }
    get_response(&respacket);
    if (respacket.error != 0) {
	    *error = respacket.error;
	return(-1);
    }
    if ((ptyfd = open(respacket.master,O_RDWR)) < 0) {
	if (error != (int *)0)
	    *error = E_OPENERR;
	return(-1);
    }
    if (slave != (char *)0)
	strcpy(slave,respacket.slave);
    if (master != (char *)0)
	strcpy(master,respacket.master);
    return(ptyfd);
}

static short requestno = 0;
static long  curtype;

static int
send_request(packetptr)
    struct ptyrequest *packetptr;
{
    struct ptymsgreqbuf mbuf;
    key_t  qid, ftok();
    int try,queueno;

    if ((qid = ftok(PTYDAEMONPROG,REQID)) == (key_t) -1)
	return(E_FTOKERR);

    if ((queueno = msgget(qid,0)) < 0)
	return(E_DEMONERR);

    curtype = ((++requestno << 16) | getpid());
    mbuf.mtype = curtype;
    memcpy(mbuf.mtext,(char *)packetptr,REQUESTSIZE);

    try = 0;
    while (try < 3) {
	if (msgsnd(queueno,(struct msgbuf *)&mbuf,REQUESTSIZE,IPC_NOWAIT) != 0) {
	    try++;
	    sleep(3);
	}
	else
	    break;
    }
    if (try >= 3)
	return(E_DEMONERR);
    else
	return(0);
}

static void
get_response(packetptr)
    struct ptyresponse *packetptr;
{
    struct ptymsgresbuf mbuf;
    key_t  qid, ftok();
    int try,queueno;
    int ret;

    if ((qid = ftok(PTYDAEMONPROG,RESID)) == (key_t) -1) {
	packetptr->error = E_FTOKERR;
	return;
    }

    if ((queueno = msgget(qid,0)) < 0) {
	packetptr->error = E_DEMONERR;
	return;
    }

    ret = msgrcv(queueno,(struct msgbuf *)&mbuf,RESPONSESIZE,curtype,0);
    if (ret != RESPONSESIZE) {
	packetptr->error = E_DEMONERR;
	return;
    }

    memcpy((char *)packetptr,mbuf.mtext,RESPONSESIZE);
    return;
}
