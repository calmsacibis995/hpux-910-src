static char *RCS_ID="@(#)$Revision: 70.1 $ $Date: 91/11/07 10:47:57 $";
/*	@(#) $Revision: 70.1 $	*/
/* HDB uucp's version of getx25 */
/*
   /usr/lib/uucp/X25/getx25 <line> <baud> <pad type>


	/etc/getty replacement for X.25

	Radek Linhart   BCD R&D   27 MAR 84     V1.0

	Modified by Rob Gardner FSD R&D MARCH 85

*/

char LOGFILE[] = "/usr/spool/uucp/.Log/LOGX25";
#define DEBUG

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <pwd.h>
#include <utmp.h>

#define	IN	0
#define	OUT	1
#define	ERR	2

#define TRUE	1
#define FALSE	0

#define MAXIOBUFF  129

#define INIT_RS232 2
#define INIT_2334A 2
#define LOGINTOUT  240
#define RECOVER	   120

#define SIGENABLE SIGTRAP
#define SIGDISABLE SIGILL

char lockfile[100];
char iobuff[MAXIOBUFF];
char pad[21];
int x28_brk_flag=FALSE;
jmp_buf Sjbuf;
int opx25pid, clrpid;
int debug;

int disabled = FALSE;

struct baudtab {char *baud; int speed; int ispeed;}  btab[] =  {

  "4",	B300,	300,
  "U",	B1200,	1200,
  "2",	B2400,	2400,
  "7",	B4800,	4800,
  "5",	B9600,	9600,
};

#define NITAB sizeof btab/sizeof btab[0]
#define streq(a,b) (!strcmp(a,b))

/****************************************************************************
****************************************************************************/


main (argc,argv)
int argc;
char *argv[];

{
   extern char iobuff[];
   extern char pad[];
   extern jmp_buf Sjbuf;
   extern int alarmtout(), got_sig();

   char device[21],line[21],cmd[129],baud[11],name[21];
   char initpar [129],disxon[21];

   int par04,speed;
   int com=FALSE;

   struct termio ttydescr;
   struct baudtab *btabp;


   int status;
   int logcount;

   char firstc;
   char scriptflag[100], charflag[100];

#ifdef DEBUG
	logentry("getx25 starting");
#endif
	if (setjmp (Sjbuf))
		bailout(30);

	{ int i; for (i=1; i<NSIG; i++) signal(i, got_sig); }

	signal(SIGHUP, got_sig);
	signal(SIGCLD, SIG_IGN);
	signal (SIGALRM,alarmtout);



	if ( (argc != 4) && (argc != 5))
		err (RECOVER);

	strcpy (line,argv[1]);
	sprintf (device,"/dev/%s",line);
	sprintf(lockfile, "/usr/spool/uucp/LCK..%s", line);
	strcpy (baud,argv[2]);
	strcpy (pad,argv[3]);
	if ( (argc==5) && (strcmp(argv[4], "debug") == 0))
		debug=1;
	else
		debug=0;

	for (btabp = btab; btabp < &btab[NITAB]; btabp++)
   		if (strcmp (btabp->baud,baud) == 0)  
			break;

	if (btabp >= &btab[NITAB])    
		btabp = btab;

	speed = btabp->speed;
	par04 = 30;                        /* 10000/(btabp->ispeed); */

	sprintf (initpar,"1:0,2:0,3:0,4:%d,5:1,6:5,7:8,8:0,9:0,",par04);
	strcat  (initpar,"12:1,13:0,14:0,15:0,16:0,17:0");
	strcpy  (disxon, "4:2,5:0,12:0");

#ifdef DEBUG
	if (debug)
	logentry("making [uw]tmp entry");
#endif

#ifdef HPFCLA
	if (fork())
		wait(0);
	else	
		execl("/etc/getty", "/etc/getty", line, "!", "0", 0);
#else
	account(line);
#endif
#ifdef DEBUG
	if (debug)
	logentry("[uw]tmp entry made, closing [012]");
#endif

	close (IN);
	close (OUT);
	close (ERR);

#ifdef DEBUG
	if (debug)
	logentry("[012] closed, re-opening");
#endif

	if (open (device,O_RDWR) != IN)
		err (RECOVER);

	if (dup (IN) != OUT) 
		err (RECOVER);

	if (dup (IN) != ERR) 
		err (RECOVER);

	if ((chown (device,0) != 0) || (chmod (device,0666) != 0)) 
		err (RECOVER);

#ifdef DEBUG
	if (debug)
	logentry("[012] re-opened and chowned, doing ioctl now");
#endif

	ttydescr.c_iflag = BRKINT | IGNPAR | ISTRIP | ICRNL | IXON | IXOFF;
	ttydescr.c_oflag = OPOST  | ONLCR;
        ttydescr.c_cflag = speed | CS8 | CREAD | CLOCAL | HUPCL;        
	/*ttydescr.c_cflag = speed | CS8 | CREAD | HUPCL | CRTS;  mung clocal */
	ttydescr.c_lflag = ISIG | ICANON;
	ttydescr.c_line = 0;
	ttydescr.c_cc[0] = 127;
	ttydescr.c_cc[1] = 28;
	ttydescr.c_cc[2] = '#';
	ttydescr.c_cc[3] = '@';
	ttydescr.c_cc[4] = 4;
	ttydescr.c_cc[5] = 0;
	ttydescr.c_cc[6] = 0;

	signal(SIGHUP, SIG_IGN);
	if (ioctl (IN,TCSETA,&ttydescr) < 0) 
		err (RECOVER);
	signal(SIGHUP, got_sig);
#ifdef DEBUG
	if (debug)
	logentry("ioctl done");
#endif

/*      sleep (INIT_RS232);

	write(OUT, "\r", 1);
	sleep(1);
	ioctl(IN, TCFLSH, 2);
*/
#ifdef DEBUG
	if (debug)
	logentry("calling clrsvc");
#endif

	signal(SIGHUP, SIG_IGN);        /* may get SIGHUP here */
	if (clrpid = fork()) {
		if (setjmp (Sjbuf)) {
/*                      if (clrpid)
*/                          kill(clrpid, SIGKILL);
			logentry("CLRSVC hung");
			err (30);
		}
		alarm(30);
		wait(&status);
		alarm(0);
	}
	else {
#ifdef HPFCLA
		execl("/usr/lib/uucpx25/clrsvc",
			"/usr/lib/uucpx25/clrsvc", device, pad, 0);
#else
		execl("/usr/lib/uucp/X25/clrsvc",
			"/usr/lib/uucp/X25/clrsvc", device, pad, 0);
#endif
		logentry("exec of clrsvc failed");
		exit(9);
	}

	if (status & 0xffff) {
		logentry("clrsvc failed");
		err(RECOVER);
	}
	open(device, O_RDWR);   /* assert DTR once again! */
	signal(SIGHUP, got_sig);

#ifdef DEBUG
	if (debug)
	logentry("clrsvc OK");
#endif
/*
	write(OUT, "\r", 1);
	sleep(1);
	ioctl(IN, TCFLSH, 0);
*/
#ifdef DEBUG
	if (debug)
	logentry("waiting for incoming");
#endif

	logentry ( "NEW" );
	write(OUT, "\021", 1);  /* send DC1 */
	read(IN, &firstc, 1);
#ifdef DEBUG
	if (debug)
	logentry("got 1st char, calling opx25");
#endif
#ifdef HPFCLA
	sprintf(scriptflag, "-f/usr/lib/uucpx25/%s.in", pad);
#else
	sprintf(scriptflag, "-f/usr/lib/uucp/X25/%s.in", pad);
#endif
	sprintf(charflag, "-c%c", firstc);

	if (opx25pid = fork()) {
		if (setjmp (Sjbuf)) {
/*                      if (opx25pid)
*/                          kill(opx25pid, SIGKILL);
			logentry("OPX25 hung");
			err (30);
		}
		alarm(120);
		wait(&status);
		alarm(0);
	}
	else {
#ifdef HPFCLA
		execl("/usr/lib/uucpx25/opx25", "opx25",
				charflag, scriptflag, "-i0", "-o1", 0);
#else
		execl("/usr/lib/uucp/X25/opx25", "opx25",
				charflag, scriptflag, "-i0", "-o1", 0);
#endif
		logentry("exec of opx25 failed");
		exit(9);
	}

	if (status & 0xffff) {
		logentry("opx25 failed");
		err(20);
	}
#ifdef DEBUG
	if (debug)
	logentry("opx25 OK");
#endif

	set_x28 (initpar);

	if (setjmp (Sjbuf))
		bailout(30);

	alarm (LOGINTOUT);

	logcount=0;

	ioctl(IN, TCFLSH, 2);

#ifdef DEBUG
	if (debug)
	logentry("asking login:");
#endif

	do {
		signal(SIGHUP, got_sig);
#ifdef DEBUG
		if (debug)
		logentry("reset SIGHUP");
#endif
		outstring ("login: ");
		instring (name,'*');
		if (logcount++ > 10) {
			close(IN);
			close(OUT);
			close(ERR);
			err(30);
		}
	} while ( name[0] == '\0' );

	alarm (0);

	logentry(name);

/*      if (uucp)
	   	set_x28 (disxon);
	else
*/
	{       ttydescr.c_lflag = ISIG | ICANON;
	   	if (ioctl (IN,TCSETA,&ttydescr) < 0) err (RECOVER);
	   	outstring ("\033)C\n");
	}
#ifdef DEBUG
	if (debug)
	{ char s[100];
	    sprintf(s,"execing login %s", name);
	    logentry(s);
	}
#endif

	chmod(device, 0622);
	{ int i; for (i=1; i<NSIG; i++) signal(i, SIG_DFL); }
#ifdef HPFCLA
	execl ("/etc/login","/etc/login",name,"60",0);
#else
	execl ("/bin/login","/bin/login",name,"60",0);
#endif

	}


/**********************************************************************/

got_sig(signo) int signo;
{
  char msg[100];

	sprintf(msg,"Caught signal #%d", signo);
	logentry(msg);
	if (signo == SIGHUP)
	    err(5);
	else
	    err(30);
}

/**********************************************************************/


/**********************************************************************/

alarmtout ()
{
   extern jmp_buf Sjbuf;

	longjmp (Sjbuf,1);
}

/**********************************************************************/

instring (string,ctrl)
char string[],ctrl;
{
   extern char iobuff[];
   int c,i,j;

	if ((i=read (IN,iobuff,MAXIOBUFF)) < 0) 
		err (RECOVER);
	iobuff[i == 0? 0: i-1]='\0';

	i=j=0;

	switch (ctrl) {
	   case 'a':
		{
		while ((c=iobuff[i++]) != '\0')
		  	if (isalpha(c)) string[j++] = c;
		string[j] = '\0';
		break;
		}
	   case 'd':
		{
		while ((c=iobuff[i++]) != '\0')
		  	if (isdigit(c)) string[j++] = c;
		string[j] = '\0';
		break;
		}
	   case 'p':
		{
		while ((c=iobuff[i++]) != '\0')
		  	if (isprint(c)) string[j++] = c;
		string[j] = '\0';
		break;
		}
	   default:
		strcpy (string,iobuff);
	   }

	return strlen (string);
}

/***********************************************************************/

outstring (string)
char string[];
{
	if (write (OUT,string,strlen(string)) <0) 
		err (RECOVER);
}

/**********************************************************************/

strisdigit (string)
char string[];

{
   int c,p=0;

	while ((c=string[p++]) != '\0')
	   	if (!isdigit(c)) return (FALSE);
	return (TRUE);
}


/************************************************************************/

set_x28 (par)
char par[];
{
   extern char iobuff[];
   extern int x28_brk_flag;
   int i;

	if (!x28_brk_flag) {
	   outstring ("\020");
	   x28_brk_flag = TRUE;
	}
	else {
	   ioctl (OUT,TCSBRK,0);
	   sleep (1);
	}

	for (i=1;i++<=2;)
	   	instring (iobuff,'*');

	sprintf (iobuff,"set %s\r",par);
	outstring (iobuff);
	for (i=1;i++<=4;)
	   	instring (iobuff,'*');

	sleep (INIT_2334A);
}


/**********************************************************************/

err (recover)
int recover;
{
    char foo[100];
	ioctl(IN, TCFLSH, 2);
	sprintf(foo,"err called with (%d)", recover);
	logentry(foo);
	sleep (recover);
	exit (9);
}

logentry(s)
char *s; 
{
   int logfd;
   FILE *logfile;
   long clock;

	logfd = open(LOGFILE, O_WRONLY|O_CREAT|O_APPEND, 0600);
	if (logfd == -1)
		return;

	logfile = fdopen(logfd, "a");
	if (logfile == NULL)
		return;
	clock = time(0);
	fprintf(logfile, "\t%s\t%s", s, ctime(&clock));
	fclose(logfile);
}

bailout(arg)
{
    logentry("bailing out");
    err(arg);
}

#ifndef HPFCLA
account(line)
char *line;
{
	register int ownpid;
	register struct utmp *u;
	extern struct utmp *getutent();
	register FILE *fp;

/* look in "utmp" file for our own entry and change it to LOGIN */
	ownpid = getpid();

	while ((u = getutent()) != NULL) {

/* is this our own entry? */
		if (u->ut_type == INIT_PROCESS && u->ut_pid == ownpid) {
			strncpy(u->ut_line, line, sizeof(u->ut_line));
			strncpy(u->ut_user, "LOGIN", sizeof(u->ut_user));
			u->ut_type = LOGIN_PROCESS;

/* write out the updated entry */
			pututline(u);
			break;
		}
	}

/* if we were successful in finding an entry for ourselves in
   the utmp file, then attempt to append to the end of the wtmp file */
	if (u != NULL && (fp = fopen(WTMP_FILE,"r+")) != NULL) {
		fseek(fp, 0L,2);        /* seek to end of file */
		fwrite(u, sizeof(*u),1,fp);
		fclose(fp);
	}

/* close the utmp file */
	endutent();
}

#endif
