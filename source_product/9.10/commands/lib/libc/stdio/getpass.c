/* @(#) $Revision: 66.6 $ */
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#  ifdef __lint
#  define fileno _fileno
#  define getc _getc
#  define putc _putc
#  endif /* __lint */
#define fputs _fputs
#define fopen _fopen
#define fclose _fclose
#define kill _kill
#define ioctl _ioctl
#define getpid _getpid
#define sigaction _sigaction
#define sigemptyset _sigemptyset
#define tcflush _tcflush
#define tcgetattr _tcgetattr
#define tcsetattr _tcsetattr
#define getpass _getpass
#define setbuf _setbuf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"
#include <signal.h>
#include <termios.h>

extern void setbuf();
extern FILE *fopen();
extern int fclose();
extern int kill(), ioctl();
extern pid_t getpid();
static int intrupt;

static void
catch()
{
    ++intrupt;
}

#ifdef _NAMESPACE_CLEAN
#undef getpass
#pragma _HP_SECONDARY_DEF _getpass getpass
#define getpass _getpass
#endif /* _NAMESPACE_CLEAN */

char *
getpass(prompt)
char *prompt;
{
    void catch();
    register char *p;
    register int c;
    FILE *fi;
    register int fno;
    static char pbuf[9];
    struct sigaction newsig, oldsig;
    struct termios save_ttyb, ttyb;

    if ((fi = fopen("/dev/tty", "r")) == NULL)
	return (char *)NULL;

    /*
     * We don't want any buffering for our i/o.
     */
    setbuf(fi, (char *)NULL);

    /*
     * Install signal handler for SIGINT so that we can restore
     * the tty settings after we change them.  The handler merely
     * increments the variable "intrupt" to tell us that an
     * interrupt signal was received.
     */
    newsig.sa_handler = catch;
    sigemptyset(&newsig.sa_mask);
    newsig.sa_flags = 0;
    sigaction(SIGINT, &newsig, &oldsig);
    intrupt = 0;

    /*
     * Get the terminal characters (save for later restoration) and
     * reset them so that echo is off
     */
    fno = fileno(fi);
    tcgetattr(fno, &ttyb);

    save_ttyb = ttyb;
    ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

    tcsetattr(fno, TCSAFLUSH, &ttyb);

    /*
     * Write the prompt and read in the user's response.
     */
    fputs(prompt, stderr);
    for (p = pbuf; !intrupt && (c = getc(fi)) != '\n' && c != EOF; )
    {
	if (p < &pbuf[8])
	    *p++ = c;
    }
    *p = '\0';
    putc('\n', stderr);

    /*
     * Restore the terminal to its previous characteristics.
     * Restore the old signal handler for SIGINT.
     */
    tcsetattr(fno, TCSANOW, &save_ttyb);
    sigaction(SIGINT, &oldsig, (struct sigaction *)0);

    if (fi != stdin)
	fclose(fi);

    /*
     * If we got a SIGINT while we were doing things, send the SIGINT
     * to ourselves so that the calling program receives it (since we
     * were intercepting it for a period of time.)
     */
    if (intrupt)
	kill(getpid(), SIGINT);

    return pbuf;
}
