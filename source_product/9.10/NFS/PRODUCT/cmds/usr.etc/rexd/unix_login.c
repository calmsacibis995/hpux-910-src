/* 	@(#)unix_login.c	$Revision: 1.24.109.3 $	$Date: 93/09/02 09:11:30 $  */

/* unix_login.c 1.2 87/03/16 NFSSRC */

/* NOTE: rexd.c, mount_nfs.c and unix_login.c share a single message	*/
/* catalog (rexd.cat).  For that reason we have allocated messages 	*/
/* 1 through 40 for rexd.c, 41 through 80 for mount_nfs.c and from 81   */
/* on for unix_login.c.  If we need more messages in this file we will	*/
/* need to take into account the message numbers that are already used 	*/
/* by the other files.							*/

#ifdef PATCH_STRING
static char *patch_3066="@(#) PATCH_9.0: unix_login.o $Revision: 1.24.109.3 $ 93/09/01 PHNE_3066";
#endif
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

# include <sys/types.h>
# include <rpc/types.h>
# include <sys/ioctl.h>
# include <sys/signal.h>
# include <sys/file.h>
# include <pwd.h>
# include <errno.h>
# include <stdio.h>
# include <utmp.h>
# include <signal.h>
# include <fcntl.h>
# include <netdb.h>
# include <sys/time.h>
# include <sys/stat.h>
# include <sys/sysmacros.h>
# include <netinet/in.h>

#ifdef hpux
#include <sys/termio.h>
/*
 * Several constants are defined in both termio.h and sgtty.h with 
 * conflicting values. In each case the value we want is in sgtty.h
 * so we use bsdundef.h to undefine the definitions from termio.h
 */
#include "bsdundef.h"

#include <sgtty.h>
#include <bsdterm.h>
#endif hpux
# include <rpcsvc/rex.h>
/*
**	include the TRACE macros
*/
#ifdef TRACEON 
#undef TRACEON
#define LIBTRACE
#endif

extern int errno;
#include <arpa/trace.h>

#ifdef NLS
extern nl_catd nlmsg_fd;
#endif NLS

#ifdef hpux
/*
**	maximum number of characters in a pty path ...
*/
# define	MAX_PTY_LEN	32
#endif 

/* needed by calls to sigvector, initialized in rexd main */
extern struct sigvec vec_ign;

/*
 * unix_login - hairy junk to simulate logins for Unix
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#ifndef hpux
char Ttys[] = "/etc/ttys";	/* file to get index of utmp */
char Utmp[] = "/etc/utmp";	/* the tty slots */
char Wtmp[] = "/usr/adm/wtmp";	/* the log information */
static int TtySlot;		/* slot number in Utmp */
#endif

extern int Master, Slave;       /* sides of the pty */
int InputSocket, OutputSocket;	/* Network sockets */
fd_set HelperMask;		/* exported to rexd */
int Helper1, Helper2;		/* pids of the helpers */
char UserName[256];		/* saves the user name for loging */
char HostName[256];		/* saves the host name for loging */
char PtyName[16] = "/dev/ttypn";/* name of the tty we allocated */

/* variables used to pass the pty size from the parent to the child */
extern int ptysize_set;         /* have we set the pty size yet */
extern int pipe1[2];             /* pipe for parent to tell child it has 
				 * received and set the modes for the pty
				 */


/*
 * Check for user being able to run on this machine.
 * returns 0 if OK, TRUE if problem, error message in "error"
 * copies name of shell if user is valid.
 */
ValidUser(host, uid, error, shell)
    char *host;
    int uid;
    char *error;
    char *shell;
{
    struct passwd *pw, *getpwuid();
    extern int  check_rhosts;       /* defined in rexd.c and set to true 
				       if rexd is started with -r */

    TRACE("ValidUser: SOP");
    if (uid == 0) {
        TRACE("ValidUser: root execution not allowed");
    	errprintf(error,(catgets(nlmsg_fd,NL_SETN,81, "rexd: root execution not allowed\n")),uid);
	return(1);
    }
    pw = getpwuid(uid);
    if (pw == NULL || pw->pw_name == NULL ) {
        TRACE2("ValidUser: no password entry for uid %d", uid);
    	errprintf(error,(catgets(nlmsg_fd,NL_SETN,82, "rexd: User id %d not valid\n")),uid);
	return(1);
    }
    else 
     if (check_rhosts) 
       {
	 /* ruserok calls the getpw routines which will overwrite
	 ** the area pointed to by pw, so we must copy the data.
	 */
         strcpy(UserName, pw->pw_name);
	 TRACE3("ValidUser: Checking equiv for user %s and host %s",
		     UserName, host);
         if ( ruserok(host,FALSE ,UserName,UserName) !=0 )
	   {
	     TRACE2("ValidUser: user %d denied access",uid);
             errprintf( error, (catgets(nlmsg_fd,NL_SETN,83, "rexd: User id %d, denied access\n")),uid);
	     return(1);
           } 
       }
    strncpy(UserName, pw->pw_name, sizeof(UserName)-1 );
    strncpy(HostName, host, sizeof(HostName)-1 );
    strcpy(shell,pw->pw_shell);
    setproctitle(pw->pw_name, host);
    TRACE("ValidUser: user is valid");
    return(0);
}

/*
 *  eliminate any controlling terminal that we have already
 */
NoControl()
{
    int devtty;

    TRACE("NoControl: SOP");
#ifdef hpux
    setpgrp();
#else not hpux
    devtty = open("/dev/tty",O_RDWR);
    if (devtty > 0) {
    	    ioctl(devtty, TIOCNOTTY, NULL);
	    close(devtty);
    }
#endif hpux
    TRACE("NoControl: EOP");
}

/*
 * Allocate a pseudo-terminal
 * sets the global variables Master and Slave.
 * returns 1 on error, 0 if OK
 */
AllocatePty(socket0, socket1, sname)
    int socket0, socket1;
    char *sname;             /* name of the slave pty */
{

    int on = 1;
    struct passwd  *pw;   /* used to get the uid associated with UserName */
    int uid;              /* uid associated with UserName */
    struct sockaddr_in from;
    int	fromlen;

    sigvector(SIGHUP, &vec_ign, NULL);
#ifdef SIGTTOU
    sigvector(SIGTTOU, &vec_ign, NULL);
    sigvector(SIGTTIN, &vec_ign, NULL);
#endif SIGTTOU
    TRACE("AllocatePty: SOP");

    if (getpty(&Master, &Slave, NULL, sname) != 0)
	{
	  TRACE("AllocatePty: can't get a pty");
	  return(1);
	}
    NoControl();

    fromlen = sizeof (from);
    if (getpeername(socket0, &from, &fromlen) < 0) {
	TRACE("AllocatePty: Couldn't get peer name of remote host");
	log_perror((catgets(nlmsg_fd,NL_SETN,86, "Can't get peer name of host")));
    }

    pw = getpwnam(UserName);
    uid = pw->pw_uid;
    TRACE3("AllocatePty: login user %s with uid %d",UserName,uid);
    account(USER_PROCESS,uid,UserName,sname,HostName,&from);
    
    InputSocket = socket0;
    OutputSocket = socket1;
    ioctl(Master, FIONBIO, &on);
    FD_ZERO(&HelperMask);
    FD_SET(InputSocket,&HelperMask);
    FD_SET(Master,&HelperMask);
    TRACE2("AllocatePty: Allocated pty, %s", sname);
    return(0);
}

#ifdef hpux
/*
 * Fix the slave side of the pty to have the proper initialized
 * modes so that things will work right most of the time.
 */

FixPty(slave)
int slave;
{
	struct termio term;

	TRACE("FixPty: SOP");
	/*
	**	get the modes on the slave side of the pty ...
	*/
	(void)ioctl(slave, TCGETA, &term);
	/*
	**	now condition the pty
	**	NOTE:	this should be the same setup that is
	**	done by getty/login on a new terminal line,
	**	except initialize so that the client is doing
	**	local echo.
	*/
	term.c_iflag &= ~(ICRNL|IUCLC|INPCK|ISTRIP);
	term.c_iflag |= IXON;
	term.c_oflag &= ~ONLRET;
	term.c_oflag |= OPOST|ONLCR|TAB3;
	term.c_cflag &= ~(CSIZE|CBAUD);    /*** clear char size and baud rate */
	term.c_cflag |= CS8|PARENB|B9600;  /*** reset char size and baud rate */
	term.c_lflag |= ISIG|ICANON|ECHO;	/* by default, remote echo */

	(void)ioctl(slave, TCSETA, &term);

	TRACE("FixPty: EOP");
	return;
}
#endif hpux


  /*
   * Special processing for interactive operation.
   * Given pointers to three standard file descriptors,
   * which get set to point to the pty.
   */
DoHelper(pfd0, pfd1, pfd2)
    int *pfd0, *pfd1, *pfd2;
{
    int pgrp;

    TRACE("DoHelper: SOP");
    pgrp = getpid();
    setpgrp(pgrp, pgrp);
#ifdef TIOCSPGRP
    ioctl(Slave, TIOCSPGRP, &pgrp);
#endif TIOCSPGRP

    sigvector( SIGINT, &vec_ign, NULL);
    close(Master);

    *pfd0 = Slave;
    *pfd1 = Slave;
    *pfd2 = Slave;
    TRACE("DoHelper: EOP");
}


/*
 * destroy the helpers when the executing process dies
 */
KillHelper(grp, sname)
    int grp;
    char *sname;         /* name of the slave pty */
{
    TRACE("KillHelper: SOP")
    TRACE5("master = %d, Slave = %d, InputSocket = %d OutputSocket = %d",
	   Master, Slave, InputSocket, OutputSocket);
    close(Master);
    FD_ZERO(&HelperMask);
    close(InputSocket);
    close(OutputSocket);
#ifdef hpux 
    TRACE3("KillHelper: logout user %s with uid %d",UserName,grp);
    account(DEAD_PROCESS,grp,"",sname,NULL,NULL);
#else
    LogoutUser();
#endif

    if (grp) 
      {
	TRACE2("KillHelper: calling killpg with %d", grp);
	killpg(grp,SIGTERM);
      }
    TRACE("KillHelper: EOP");
}


#ifndef hpux  /* not used by hpux. we use account instead */
/*
 * edit the Unix traditional data files that tell who is logged
 * into "the system"
 */
LoginUser()
{
  FILE *ttysFile;
  register char *last = PtyName + sizeof("/dev");
  char line[256];
  int count;
  int utf;
  struct utmp utmp;
  
  TRACE("LoginUser: SOP");
  ttysFile = fopen(Ttys,"r");
  TtySlot = 0;
  count = 0;
  if (ttysFile != NULL) {
      while (fgets(line, sizeof(line), ttysFile) != NULL) {
        register char *lp;
	lp = line + strlen(line) - 1;
	if (*lp == '\n') *lp = '\0';
	count++;
	if (strcmp(last,line+2)==0) {
	  TtySlot = count;
	  break;
	}
      }
      fclose(ttysFile);
  }
  if (TtySlot > 0 && (utf = open(Utmp,O_WRONLY)) >= 0) {
      lseek(utf, TtySlot*sizeof(utmp), L_SET);
      strncpy(utmp.ut_line,last,sizeof(utmp.ut_line));
      strncpy(utmp.ut_name,UserName,sizeof(utmp.ut_name));
      strncpy(utmp.ut_host,HostName,sizeof(utmp.ut_host)); 
      time(&utmp.ut_time);
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
  if (TtySlot > 0 && (utf = open(Wtmp,O_WRONLY)) >= 0) {
      lseek(utf, (long)0, L_XTND);
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
  TRACE("LoginUser: EOP");
}

#endif
#ifndef hpux  /* not used by hpux. we use account instead */

/*
 * edit the Unix traditional data files that tell who is logged
 * into "the system".
 */
LogoutUser()
{
  int utf;
  register char *last = PtyName + sizeof("/dev");
  struct utmp utmp;

  TRACE("LogoutUser: ");
  if (TtySlot > 0 && (utf = open(Utmp,O_RDWR)) >= 0) {
      lseek(utf, TtySlot*sizeof(utmp), L_SET);
      read(utf, (char *)&utmp, sizeof(utmp));
      if (strncmp(last,utmp.ut_line,sizeof(utmp.ut_line))==0) {
	lseek(utf, TtySlot*sizeof(utmp), L_SET);
	strcpy(utmp.ut_name,"");
	strcpy(utmp.ut_host,"");
	time(&utmp.ut_time);
	write(utf, (char *)&utmp, sizeof(utmp));
      }
      close(utf);
  }
  if (TtySlot > 0 && (utf = open(Wtmp,O_WRONLY)) >= 0) {
      lseek(utf, (long)0, L_XTND);
      strncpy(utmp.ut_line,last,sizeof(utmp.ut_line));
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
  TtySlot = 0;
  TRACE("LogoutUser: EOP");
}
#endif 

/*
 * set the pty modes to the given values
 */
SetPtyMode(mode)
    struct rex_ttymode *mode;
{
    TRACE("SetPtyMode: SOP");
#ifdef hpux
    bsd_tty_ioctl(Master, TIOCSETN, &mode->basic);
    bsd_tty_ioctl(Master, TIOCSETC, &mode->more);
    bsd_tty_ioctl(Master, TIOCSLTC, &mode->yetmore);
    bsd_tty_ioctl(Master, TIOCLSET, &mode->andmore);
#else not hpux
    int ldisc = NTTYDISC;
    
    ioctl(Slave, TIOCSETD, &ldisc);
    ioctl(Slave, TIOCSETN, &mode->basic);
    ioctl(Slave, TIOCSETC, &mode->more);
    ioctl(Slave, TIOCSLTC, &mode->yetmore);
    ioctl(Slave, TIOCLSET, &mode->andmore);
#endif hpux
    TRACE("SetPtyMode: EOP");
}

/*
 * set the pty window size to the given value
 */
SetPtySize(size)
    struct ttysize *size;
{
    int pgrp;

    TRACE("SetPtySize: SOP");
#ifdef TIOCSSIZE
    (void) ioctl(Master, TIOCSSIZE, size);
    TRACE("SetPtySize: calling SendSignal to set window size");
    SendSignal(SIGWINCH);
	write(pipe1[1], "",1);
#else
    if ( ptysize_set == FALSE )
      {
	/* We set the pty size once before the command is execed. After 
	 * that we can not do anything to change the size of the 
	 * pty.
	 */
	TRACE2("SetPtySize: writing lines, lines = %d", (*size).ts_lines);
	write(pipe1[1], &((*size).ts_lines), sizeof( (*size).ts_lines));
	TRACE2("SetPtySize: writing cols, cols =  %d", (*size).ts_cols);
	write(pipe1[1], &((*size).ts_cols), sizeof( (*size).ts_cols));
	ptysize_set = TRUE;
      }
    else
      TRACE("SetPtySize: can't change window size");
    /* can't do much for now. XXX */
#endif
    TRACE("SetPtySize: EOP");
}


/*
 * send the given signal to the group controlling the terminal
 */
#ifdef hpux
#include <sys/ptyio.h>
#endif hpux

SendSignal(sig)
    int sig;
{
    int pgrp;

    TRACE("SendSignal: SOP");
#ifdef hpux
    (void) ioctl(Master, TIOCSIGSEND, sig);
#else not hpux
    if (ioctl(Slave, TIOCGPGRP, &pgrp) >= 0)
    	(void) killpg( pgrp, sig);
#endif hpux
    TRACE("SendSignal: EOP");
}


/*
 * called when the main select loop detects that we might want to
 * read something.
 *
 * When reading the master Pty and writing to the Output socket SIGCLD
 * must be blocked or we can lose data. This will happen is the signal
 * is received after the read but before the write.  *mjk* 
 */
HelperRead(fds)
    fd_set *fds;
{
    char buf[128];
    int cc;
    long   oldmask;   /* the mask of blocked signals before the call to 
			 sigblock and sigsetmask */

    TRACE("HelperRead: SOP");
    if (FD_ISSET(Master,fds)) {
        /* block SIGCLD to avoid losing data if a sigchild would be 
	   received between the read and the write calls */
        oldmask = sigblock(sigmask(SIGCLD));
        TRACE("HelperRead: reading from the master pty");
    	cc = read(Master, buf, sizeof buf);
	TRACE2("HelperRead: read %d characters from the master pty",cc);
	if (cc > 0)
	  {
	        TRACE("HelperRead: writing to OutputSocket");
		(void) write(OutputSocket, buf, cc);
	  }
	else {
		if (cc < 0 && errno != EINTR && errno != EWOULDBLOCK)
		  {
		        TRACE("HelperRead: pty read error");
			log_perror((catgets(nlmsg_fd,NL_SETN,84, "pty read")));
		  }
		TRACE("HelperRead: shutting down OutputSocket removing Master from HelperMask");
		shutdown(OutputSocket, 1);
		FD_CLR(Master,&HelperMask);
	}
	/* reset the signal block mask to it's previous value */
	oldmask = sigsetmask(oldmask);
    }
    if (FD_ISSET(InputSocket,fds)) {
        TRACE("HelperRead: reading from InputSocket");
    	cc = read(InputSocket, buf, sizeof buf);
	TRACE2("HelperRead: read %d characters from InputSocket",cc);
	if (cc > 0)
	  {  
	     TRACE("HelperRead: writing to Master pty");
	     (void) write(Master, buf, cc);
	  }
	else {
		if (cc < 0 && errno != EINTR && errno != EWOULDBLOCK)
		  {
		     TRACE("HelperRead: socket read error");
		     log_perror((catgets(nlmsg_fd,NL_SETN,85, "socket read")));
		  }
		TRACE("HelperRead: remove InputSocket from HelperMask");
		FD_CLR(InputSocket,&HelperMask);
	}
    }
    TRACE("HelperRead: EOP");
}

void
FlushPty()
/*
** This routine is called to flush data that the child has written to the 
** Pty slave that the parent may not have read from the Master and written
** to the OutputSocket. This can happen if the child dies before the parent
** hits the main loop. FlushPty is called from the SIGCLD handler, CatchChild.
*/
{
  int more = TRUE;        /* possibly more characters to read */
  int cc;                 /* number of characters read from Master Pty */
  char buf[128];          /* buffer for reading characters */
  struct timeval poll;    /* timeout value to poll using select */
  int    Masterfd = 0;    /* file descriptor for the master pty */

        /* initialize timeout to poll the Master Pty */
        poll.tv_sec   = 0;
        poll.tv_usec  = 0;
        /* set the file descriptor for the Master pty */
        Masterfd = (1<<Master);	/* assuming that Master is < 32 */

       while (more) 
	 {
	   TRACE("FlushPty: calling select ... ");
	   /* set Masterfd to the file descriptor of the Master Pty */
	   if (select(32, &Masterfd, (int *)0, (int *)0, &poll) > 0 )
	     {
	       TRACE("FlushPty: there is data to flush from pty");
	       cc = read(Master, buf, sizeof buf);
	       TRACE2("FlushPty: read %d characters from the master pty",cc);
	       if (cc > 0)
		 {
		   TRACE("FlushPty: writing to OutputSocket");
		   (void) write(OutputSocket, buf, cc);
		 }
	       if (cc < ( sizeof buf) )
		 {
		   TRACE("FlushPty: data is flushed");
		   more = FALSE;
		 }
	     }
	   else  /* Master was not ready for reading */
	     {
	       TRACE("FlushPty: data is flushed");
	       more = FALSE;
	     }
	 } /* while */
    TRACE("FlushPty: EOP");
} /* FlushPty */

/*** The account routine below replaces the rmut routine that was **/
/*** used here.  This routine is slightly more general in that it **/
/*** can be used to either create a utmp entry, or to mark it dead **/
/*** both which we need to do.  We create the utmp entry in the daemon **/
/*** becuase the HP-UX login will not succeed if there is not one there. **/

/*** the basis of the account routine was stolen from the ***/
/*** pty daemon written by John Marvin.  The extra flags arguement **/
/*** was deleted since we want to always to the same things ***/
/*** also added a call to endutent() which was not there. dds. ***/

#define DEVOFFSET  5            /* number of characters in "/dev/"     */

account(type,pid,user,slave,hostname,hostaddr)
    short type,pid;
    char  *user,*slave;
    char *hostname;
    struct sockaddr_in *hostaddr;
{
    struct utmp utmp, *oldu;
    extern struct utmp *_pututline(), *getutid();
    FILE *fp;
    int writeit;
    char *s;
    char *strrchr();

    /** open utmp file ***/

    setutent();

    /* initialize utmp structure */

    memset(utmp.ut_user,'\0',sizeof(utmp.ut_user));
    memset(utmp.ut_line,'\0',sizeof(utmp.ut_line));
    strncpy(utmp.ut_user,user,sizeof(utmp.ut_user));
    strncpy(utmp.ut_line,slave + DEVOFFSET,sizeof(utmp.ut_line));
    utmp.ut_pid  = pid;
    utmp.ut_type = type;

    s = strrchr(slave,'/') + 1; /* Must succeed since slave_pty */
				/* begins with /dev/.           */
    if (strncmp(s,"tty",3) == 0)
	s += 3;
    /*
    **	NOTE:	this only allows for two character pty's -- ie "p0"
    */
    utmp.ut_id[0] = *s++;
    utmp.ut_id[1] = *s;
    utmp.ut_id[2] = utmp.ut_id[3] = '\0';

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

    /* Add entry to WTMP file. */
    wtmplog(&utmp);

    /*
    **	close utmp file and return ...
    */
    endutent();
    return(0);
}


/*
**	getpty.c	--	get a master and slave pty pair
****
**	Note:	this does not setpgrp any longer, the pty bug is
**		now fixed!  Or seems to be ...
*/

struct	stat	stb;
struct	stat	m_stbuf;	/* stat buffer for master pty */
struct	stat	s_stbuf;	/* stat buffer for slave pty */

/*
**	ptymdirs	--	directories to search for master ptys
**	ptysdirs	--	directories to search for slave ptys
**	ptymloc		--	full path to pty master
**	ptysloc		--	full path to pty slave
*/
char	*ptymdirs[] = { "/dev/ptym/", "/dev/", (char *) 0 } ;
char	*ptysdirs[] = { "/dev/pty/", "/dev/", (char *) 0 } ;
char	ptymloc[MAX_PTY_LEN];
char	ptysloc[MAX_PTY_LEN];

/*
**	ltrs	--	legal first char of pty name (letter)
**	nums	--	legal second char of pty name (number)
*/
char	ltrs[] = "pqrstuvwxyz";
char	nums[] = "0123456789abcdef";

/*
**	getpty()	--	get a pty pair
**
**	Input	--	none
**	Output	--	zero if pty pair gotten; non-zero if not
**	[value parameters mfd, sfd]
**		  mfd	Master FD for the pty pair
**		  sfd	Slave  FD for the pty pair
**	[optional value parameters -- only set if != NULL]
**		mname	Master pty file name
**		sname	Slave pty file name
*/

/*
 * Modified by Glen A. Foster, September 23, 1986 to get around a problem
 * with 4.2BSD job control in the HP-UX (a.k.a. system V) kernel.  Before
 * this fix, getpty() used to find a master pty to use, then open the cor-
 * responding slave side, just to see if it was there (kind of a sanity
 * check), then close the slave side, fork(), and let the child re-open 
 * the slave side in order to get the proper controlling terminal.  This
 * was an excellent solution EXCEPT for the case when another process was
 * already associated with the same slave side before we (rlogind) were
 * exec()ed.  In that case, the controlling tty stuff gets all messed up,
 * and the solution is to NOT open the slave side in the parent (before the
 * fork()), but to let the child be the first to open it after its setpgrp()
 * call.  This works in all cases.  This stuff is black magic, really!
 *
 * This is necessary due to HP's implementation of 4.2BSD job control.
 */

/*
**	NOTE:	this routine should be put into a library somewhere, since
**	both rlogin and telnet need it!  also, other programs might want to
**	call it some day to get a pty pair ...
*/
getpty(mfd, sfd, mname, sname)
int	*mfd, *sfd;
char	*mname, *sname;
{
    int loc, ltr, num;
    register int mlen, slen;

    for (loc=0; ptymdirs[loc] != (char *) 0; loc++) {
	if (stat(ptymdirs[loc], &stb))			/* no directory ... */
	    continue;					/*  so try next one */

	/*	generate the master pty path	*/
	(void) strcpy(ptymloc, ptymdirs[loc]);
	(void) strcat(ptymloc, "ptyLN");
	mlen = strlen(ptymloc);

	/*	generate the slave pty path	*/
	(void) strcpy(ptysloc, ptysdirs[loc]);
	(void) strcat(ptysloc, "ttyLN");
	slen = strlen(ptysloc);

	for (ltr=0; ltrs[ltr] != '\0'; ltr++) {
	    ptymloc[mlen - 2] = ltrs[ltr];
	    ptymloc[mlen - 1] = '0';
	    if (stat(ptymloc, &stb))			/* no ptyL0 ... */
		continue;				/* try next ltr */

	    for (num=0; nums[num] != '\0'; num++) {
		ptymloc[mlen - 1] = nums[num];
		if ((*mfd=open(ptymloc,O_RDWR)) < 0)	/* no master	*/
		    continue;				/* try next num	*/
		
		ptysloc[slen - 2] = ltrs[ltr];
		ptysloc[slen - 1] = nums[num];

		/*
		**	NOTE:	changed to only stat the slave device; see
		**	comments all over the place about job control ...
		*/
		if (fstat(*mfd, &m_stbuf) < 0 || stat(ptysloc, &s_stbuf) < 0) {
		    close(*mfd);
		    continue;
		}
		/*
		**	sanity check: are the minor numbers the same??
		*/
		if (minor(m_stbuf.st_rdev) != minor(s_stbuf.st_rdev)) {
		    close(*mfd);  
		    continue;				/* try next num	*/
		}

		/*	else we got both a master and a slave pty	*/
		/*
		**	set the slave pty default mode to 0622
		**
 		**  (void) chmod (ptysloc, 0622);
		**
		**	if the pty names are requested, then fill them in.
		**	this is somewhat dangerous: if someone calls getpty()
		**	without these arguments, and they don't "just happen"
		**	to be NULL (ie. they are random numbers on the stack)
		**	then getpty will try to write there, and may dump core
		*/
		if (mname != (char *) 0)
		    (void) strcpy(mname, ptymloc);
		if (sname != (char *) 0)
		    (void) strcpy(sname, ptysloc);
		return 0;				/* return OK	*/
	    }
	}
    }

    /*	we were not able to get the master/slave pty pair	*/
    return -1;		
}
