/*	@(#) $Revision: 70.1 $	*/
static char *RCS_ID="@(#)$Revision: 70.1 $ $Date: 91/11/07 10:47:52 $";
/* HDB version of clrsvc */
/* Same as revision 27.5 except for logfile name */

/*
   /usr/lib/uucp/X25/clrsvc <device file> <pad type>


	clear switched virtual circuit on X.25

	Radek Linhart   BCD R&D   27 MAR 84

*/


#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/times.h>

#define TRUE	1
#define FALSE	0


char pad[21];
int devfd;

/****************************************************************************
****************************************************************************/
jmp_buf Sjbuf;
int alarmtout();
int hangup();

main (argc,argv)
int argc;
char *argv[];


{
char device[21];
char opx25[100];
int status;
int pid;
char scriptflag[50], inflag[50], outflag[50];

struct termio ttydescr,ottydescr;

	signal(SIGHUP, hangup);
if (argc != 3) exit (1);
strcpy (device,argv[1]);
strcpy (pad,argv[2]);

if ((devfd = open (device,O_RDWR)) < 0) exit (1);

if (ioctl (devfd,TCGETA,&ttydescr) < 0) exit (1);
if (ioctl (devfd,TCGETA,&ottydescr) < 0) exit (1);
ttydescr.c_lflag = 0;
ttydescr.c_cc[4] = 1;
ttydescr.c_cc[5] = 0;
if (ioctl (devfd,TCSETA,&ttydescr) < 0) exit (1);

sprintf(scriptflag, "-f/usr/lib/uucp/X25/%s.clr", pad);
sprintf(inflag, "-i%d", devfd);
sprintf(outflag, "-o%d", devfd);

switch (setjmp(Sjbuf)) {
case 1:
    if (pid)
	kill(pid, SIGKILL);
    exit(1);
    break;
case 2:
    if (pid)
	kill(pid, SIGKILL);
    exit(0);
    break;
default:
    break;
}
signal(SIGALRM, alarmtout);
alarm(40);
if ( (pid=fork()))
	wait(&status);
else    execl("/usr/lib/uucp/X25/opx25", "opx25",
		scriptflag,
		inflag,
		outflag,
		0);

alarm(0);
if (status & 0xffff)
	exit(1);

if (ioctl (devfd,TCSETA,&ottydescr) < 0) exit (1);
exit (0);

}


alarmtout()
{
	longjmp(Sjbuf, 1);
}

hangup()
{
    longjmp(Sjbuf, 2);
}
