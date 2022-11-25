static char *HPUX_ID = "@(#) $Revision: 66.1 $";

/*
$Compile: cc -v -g %f /usr/contrib/lib/libptyreq.a -o %F
*/


/**************************************************************************/
/*                                                                        */
/*  SHL - Shell Layers                                                    */
/*                                                                        */
/*  User interface: AT&T                                                  */
/*  Program: Rob Gardner, FSD                                             */
/*  Working: November 22, 1985                                            */
/*  Last Changed: June 7, 1988 (pscott)                                   */
/*                                                                        */
/**************************************************************************/
/*
    define:

	DUMP        to enable the 'dump' command (stack dump)
	LOGINCMD    to enable the 'login' command
	BANG        to enable the ! command
	INITOPTION  to enable automatic startup of a login shell
	TTYONLY     to restrict usage to tty input
	INFO        for informative messages

*/
#ifndef hp9000s800
#define LOGINCMD        /* for 'login' command */
#endif
#define INITOPTION      /* understands command line options */
#define INFO            /* want info messages */
#define BANG            /* for subshell command */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>

#ifdef SIGTSTP		/* for handling BSD job cntrl */
#    include <bsdtty.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#    define STOPPED(x) ((x & 0x00FF) == WSTOPPED) /* check if stopped */
#endif

#include <fcntl.h>
#include <ctype.h>
#include <sys/ptyio.h>
#include <sys/errno.h>
#include "ptyrequest.h"

#ifdef hp9000s500               /* so 500 doesn't count layers as users */
#include <sys/types.h>
#include <sys/stat.h>
#endif

#define MASK(s) (1L<<(s-1))     /* for select */

#ifndef VSWTCH              /* until termio defines it better */
#define LOBLK 0020000
#define VSWTCH 7
#define CSWTCH '\032'
#endif

#define MAX_LAYER 16        /* maximum number of layers*/
#define LAYBUFSIZE 4096     /* layer buffer size */
#define SPACE ' '
#define NONE (-3)   /* can't be -1 since it conflicts with wait3 return val */
#define AMBIG (-2)
#define FROMLAYERS (-1)	   /* passed to bang() if called from layers() */
#define proc
#define SAME 0
#define CONTROL_Z '\032'
#define DEFAULT_ESC CONTROL_Z
#define BLOCK_SIGINT (1 << (SIGINT - 1))
#define NFDS 32              /* Number of file descriptors to use in select */
			     /* Some code will need to be changed if this   */
			     /* limit is raised. Also NFDS must be >=       */
			     /* MAX_LAYER + 3.                              */


struct jtabstruct {         /* jump table for command syntax and execution */
    char cmd_name[12];
    char cmd_syntax[28];
    int (*func)();
};

int create(),
    block(),
    delete(),
    help(),
    layers(),
    resume(),
    toggle(),
    unblock(),
    name(),
#ifdef DUMP
    int dump(),
#endif
#ifdef LOGINCMD
    login(),            /* login command, ie, get login: prompt in layer */
#endif LOGINCMD
#ifdef BANG
    bang(),
#endif BANG
    quit();

int catch_break(), catch_alarm(), sig_die(), orphanage();

struct jtabstruct jumptable[] =
	{   "create",   "[-[name] | name [command]]",  create,
	    "block",    "name [name ...]",             block,
	    "delete",   "name [name ...]",             delete,
	    "help",     "",                            help,
	    "?",        "",                            help,
	    "layers",   "[l] [name ...]",              layers,
	    "resume",   "[name] | [resume] name",      resume,
	    "toggle",   "",                            toggle,
	    "unblock",  "name [name ...]",             unblock,
	    "name",     "[oldname] newname",           name,
#ifdef DUMP
	    "dump",     "",                            dump,
#endif
#ifdef LOGINCMD
	    "login",    "[name]",                      login,
#endif LOGINCMD
#ifdef BANG
	    "!",        "[command]",                   bang,
#endif BANG
	    "quit",     "",                            quit,
	    "",         0
	};

jmp_buf Sjbuf;                              /* for error recovery */

struct layer_struct {
    char path[80];                          /* pathname of (slave) pty */
    char name[80];                          /* name of layer */
    int fd;                                 /* file descriptor of layer */
    int blocked;                            /* is layer blocked? flag */
    int pid;                                /* pid of process in layer */
    int status;                             /* defunct ? */
    char *buffer;                           /* buffers for layer output */
    int numread;                            /* number of chars read */
    char escape;                            /* escape char for each layer */
};

struct layer_struct layer[MAX_LAYER];

int current_layer, recent_layer;            /* whooz who */

struct termio savedmodes;
#ifdef TIOCGLTC		/* if we have process group control */
			/* then save and propogate the susp */
			/* and dsusp settings.		    */
struct ltchars savesusp;
#endif

#ifdef SV_BSDSIG	/* we have BSD signal semantics */
struct sigvec vec;
#endif

extern int errno;

#ifdef LOGINCMD
int loginflag;
#endif LOGINCMD

int croakflag = 0;

#ifdef INITOPTION
char initcmd[80];
#endif INITOPTION

int debug = 0;
char *realtty;


proc main(argc, argv)
int argc;
char *argv[];
{
    int i;
    char cmd_str[80];
    char command[80];
    struct jtabstruct *index;

    realtty = (char *) ttyname(0);
#ifdef TTYONLY
    if ( realtty == (char *)0) {
	fprintf(stderr,"Input must be from a terminal device.\r\n");
	exit(1);
    }
#endif TTYONLY

    /* initialize */
    for (i = 0; i < MAX_LAYER; i++) {
	strcpy(layer[i].name, "");
	strcpy(layer[i].path, "");
	layer[i].escape = CONTROL_Z;
	layer[i].fd = NONE;
	layer[i].blocked = 1;
	layer[i].pid = NONE;
	layer[i].status = NONE;
	layer[i].buffer = (char *) 0;
	layer[i].numread = 0;
    }

    if (realtty != (char *) 0)
	ioctl(0, TCGETA, &savedmodes);
    else
	fakemodes();
#ifdef TIOCGLTC
    ioctl(0, TIOCGLTC, &savesusp);
#endif

    for (i=0; i < 18; i++) /* don't mess with job cntl sigs 24-28 */
	signal(i, sig_die); /* also: we are using sigvector for */
			    /* SIGCLD. This is a documented no-no but */
			    /* will work ok as long as we don't intermix */
			    /* signal for SIGCLD 			*/

    for (i=19; i < 24; i++)
	signal(i, sig_die);

    for (i=29; i < NSIG; i++)
	signal(i, sig_die);

    signal(SIGQUIT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGINT, catch_break);

#ifdef SV_BSDSIG		/* i.e. we have BSD signal semantics */
    vec.sv_handler = orphanage;
    vec.sv_flags |= SV_BSDSIG;
    sigvector(SIGCLD,&vec, (struct sigvec *)0);
#else
    signal(SIGCLD, orphanage);
#endif


    sigsetmask(MASK(SIGINT) | MASK(SIGCLD));

    current_layer = recent_layer = NONE;

#ifdef INITOPTION
    if (argc > 1) {
	if (argc > 2) {
	    strcpy(initcmd, argv[2]);
	    strcat(initcmd, "\r");
	}
	else
	    strcpy(initcmd, "");
	strcpy(command, "- ");
	strcat(command, argv[1]);
	create(command);
    }
#endif INITOPTION

    while (1) {
	reset_tty();
	signal(SIGINT, SIG_IGN);
#ifdef LOGINCMD
	loginflag = 0;
#endif LOGINCMD
	printf(">>> ");
	fflush(stdout);
	gets(cmd_str);
        for(i=0; cmd_str[i] == ' ' || cmd_str[i] == '\t'; i++)  ;
        strcpy(command,&cmd_str[i]);
	if ( *command == '\0')
	    continue;
	index = jumptable;
	while ( (*index).func != 0 ) {
	    int i, len, gotmatch = 0;
	    len = strlen((*index).cmd_name);
	    for (i=0; i <= len; i++) {
		if ( command[i] == '\0' ) {
		    ((*index).func)("");
		    gotmatch++;
		    break;
		}
		if ( (i>0) &&  (command[i] == SPACE) ) {
		    ((*index).func)(&command[i+1]);
		    gotmatch++;
		    break;
		}
		if ( (*index).cmd_name[i] != command[i] )
		    break;  /* doesn't match */
		if ( (*index).cmd_name[i] == '\0' ) {
		    /* got a perfect match */
		    ((*index).func)("");
		    gotmatch++;
		    break;
		}
	    }
	    if (gotmatch)
		break;
	    index++;
	}
	if ( (*index).func == 0 ) {
	    /* check layer-names: command might be a layer name */
	    int layno;
	    layno = findno(command);
	    if (layno == NONE)
		printf("Illegal command '%s'; Use '?' for help\r\n", command);
	    else if (layno == AMBIG)
		printf("Ambiguous command '%s'; Use '?' for help\r\n", command);
	    else
		run_layer(command);
	}
    }
}

proc create(layer_name)
char *layer_name;
{
    extern char *malloc();
    char dev[50], *arps[50], slave[50];
    char name[10];    /* Used only if a layer name is not specified   */
    char ps1buf[50];  /* Holds definition of PS1 environment variable */
    char *addenvp[2]; /* Holds environment array.                     */
    int nargs;
    int ptyerror, fileflags;
    int layno, fd, dummy;
    int quick = 1, nindex = 0;

    nargs = getargs(layer_name, arps);

    /* if first arg is '-' then it's a login shell */

    if (strcmp(layer_name, "-") == 0)
	return(loginshell(nargs > 1 ? arps[1] : ""));


    /* Don't Allow user to specify a numeric layer name. A numeric */
    /* layer name would conflict with the default conventions for  */
    /* layer names. A numeric layer name is of the form # or (#),  */
    /* where # is a decimal number.                                */
    
    if (!valid_name(layer_name)) 
    {
	fprintf(stderr, "Illegal layer name specified\r\n");
	return(1);
    }
    
    /* find a free slot */

    layno = name_exists(layer_name);
    if ( (layer_name && (*layer_name != '\0')) && (layno >= 0) )
	printf("Layer '%s' already exists\r\n", layer_name);
    else {
	for (layno = 0; layno < MAX_LAYER; layno++)
	    if (layer[layno].name[0] == '\0')
		break;
	if (layno >= MAX_LAYER)
	    printf("No room for any more layers\r\n");
	else {

	    /* Set up default layer name if one wasn't specified */

	    if (layer_name == NULL || *layer_name == '\0') {
		sprintf(name,"(%d)",layno + 1);
		layer_name = name;
	    }

	    /* Set up additional environment variables */
	    /* (only PS1 at this time)                 */

	    strcpy(ps1buf,"PS1=");
	    strcat(ps1buf,layer_name);
	    strcat(ps1buf," ");
	    addenvp[0] = ps1buf;
	    addenvp[1] = (char *)0;
	    {
		char *foo;
		foo = (char *) malloc(strlen(ps1buf)+1);
		if (foo != NULL) {
		    strcpy(foo, ps1buf);
		    putenv(foo);
		}
	    }

	    /* Allocate buffer space for this layer */

	    if (layer[layno].buffer == (char *) 0)
		if ( (layer[layno].buffer = malloc(LAYBUFSIZE)) == (char *) 0) {
		    fprintf(stderr, "Malloc failed\r\n");
		    die();
		}

	    /* create new window */

	    if (setjmp(Sjbuf)) {
		fprintf(stderr, "ptydaemon did not respond\r\n");
		return(1);
	    }
	    /* if no args, make it a shell */
	    if (nargs < 2) {
		arps[1] = (char *) getenv("SHELL");
		arps[2] = NULL;
	    }
	    else
		arps[nargs] = NULL;
	    signal(SIGALRM, catch_alarm);
	    alarm(15);
	    fd = getpty((char *) NULL, (char *) NULL, (char **) NULL,
	      (char *) NULL, (char *) NULL, 0, slave, dev, &ptyerror);
	    alarm(0);
	    signal(SIGALRM, sig_die);
	    if (fd < 0) {
		fprintf(stderr, "ptydaemon request error: ");
		fprintf(stderr, "%s\r\n", pty_errlist[ptyerror]);
		return(1);
	    }
#ifdef hp9000s500
	    {                           /* don't count as a user */
		struct stat sbuf;
		if (fstat(0, &sbuf) == 0)
		    ioctl(fd, 0x80002601, sbuf.st_rdev);
		/* note: this isn't documented, and it isn't legal,
		    so keep your mouth shut */
	    }
#endif
	    /* now invoke stuff */
	    dummy = vfork();
	    if (dummy == 0) {          /* child */
		int i;
		close(0);
		close(1);
		setpgrp();
		if (open(slave, O_RDWR) < 0) {
		    perror("slave failed to open\r");
		    dummy = -1;
		    _exit(1);
		}
		ioctl(0, TCSETAW, &savedmodes);
#ifdef TIOCGLTC /* propogate susp and dsusp settings to layer */
		ioctl(0, TIOCSLTC, &savesusp);
#endif
		dup(0);
		close(2);
		dup(0);
		for (i=getnumfds()-1; i>=3; i--) close(i);
		for (i=1; i < 18; i++) signal(i, SIG_DFL);
		for (i=19; i < NSIG; i++) signal(i, SIG_DFL);
#ifdef SV_BSDSIG
		vec.sv_handler = SIG_DFL;
		sigvector(SIGCLD,&vec, (struct sigvec *)0);
#else
		signal(SIGCLD,SIG_DFL);
#endif

#ifdef SIGTSTP	/* We want layers to ignore BSD job cntrl */
		/* signals. If the subsequent exec'ed processes */
		/* are job control cognizant then they will */
		/* reset these signals and do the "right thing" */
		signal(SIGTSTP, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
#endif

		sigsetmask(0);
		execvp(arps[1], &arps[1]);
		perror("exec failed\r");
		dummy = -1;         /* tell parent there was a problem */
		_exit(1);
	    }
	    if (dummy == -1)        /* note that this may occur because */
		return(1);          /* the fork failed, *or* because the */
				    /* exec failed */
	    layer[layno].pid = dummy;

	    /* Set O_NDELAY on pty */

	    if ((fileflags = fcntl(fd, F_GETFL, 0)) == -1) {
		perror("Could not get file flags for pty\r");
		return(1);
	    }

	    if (fcntl(fd, F_SETFL, (fileflags | O_NDELAY)) == -1) {
		perror("Could not set O_NDELAY mode on pty\r");
		return(1);
	    }

	    layer[layno].fd = fd;

	    dummy = 1;
	    if (ioctl(fd ,TIOCTRAP, &dummy) != 0) {
		perror("TIOCTRAP ioctl failed\r");
		die();
	    }

	    if (ioctl(fd ,TIOCMONITOR, &dummy) != 0) {
		perror("TIOCMONITOR ioctl failed\r");
		die();
	    }

	    if (ioctl(fd, TIOCSIGMODE, TIOCSIGBLOCK) != 0) {
		perror("TIOCSIGMODE ioctl failed\r");
		die();
	    }

	    strcpy(layer[layno].name, layer_name);
	    layer[layno].blocked = 1;
	    layer[layno].escape = savedmodes.c_cc[VSWTCH];
	    strcpy(layer[layno].path, slave);
#ifdef INFO
	    fprintf(stderr, "[layer %s created]\r\n", layer[layno].name);
#endif INFO
	    push(layno);
	    run_layer(layer[layno].name);
	}
    }
    return(0);
}


proc loginshell(layer_name)
char *layer_name;
{
    extern char *malloc();
    char dev[50], *arps[50];
    char name[10];    /* Used only if a layer name is not specified   */
    char ps1buf[50];  /* Holds definition of PS1 environment variable */
    char *addenvp[2]; /* Holds environment array.                     */
    int nargs;
    int ptyerror, fileflags;
    int layno, fd, dummy;

    nargs = getargs(layer_name, arps);
    if (nargs > 1) {
	fprintf(stderr, "Only one argument allowed\r\n");
	return(1);
    }

    /* Don't Allow user to specify a numeric layer name. A numeric */
    /* layer name would conflict with the default conventions for  */
    /* layer names. A numeric layer name is of the form # or (#),  */
    /* where # is a decimal number.                                */

    if (!valid_name(layer_name)) 
    {
	fprintf(stderr, "Illegal layer name specified\r\n");
	return(1);
    }

    /* first find a free slot */
    layno = name_exists(layer_name);
    if ( (*layer_name != '\0') && (layno >= 0) )
	fprintf(stderr, "Layer '%s' already exists\r\n", layer_name);
    else {
	for (layno = 0; layno < MAX_LAYER; layno++)
	    if (layer[layno].name[0] == '\0')
		break;
	if (layno >= MAX_LAYER)
	    fprintf(stderr, "No room for any more layers\r\n");
	else {

	    /* Set up default layer name if one wasn't specified */

	    if (*layer_name == '\0') {
		sprintf(name,"(%d)",layno + 1);
		layer_name = name;
	    }

	    /* Set up additional environment variables */
	    /* (only PS1 at this time)                 */

	    strcpy(ps1buf,"PS1=");
	    strcat(ps1buf,layer_name);
	    strcat(ps1buf," ");
	    addenvp[0] = ps1buf;
	    addenvp[1] = (char *)0;

	    /* Allocate buffer space for this layer */

	    if (layer[layno].buffer == (char *) 0)
		if ( (layer[layno].buffer = malloc(LAYBUFSIZE)) == (char *) 0) {
		    fprintf(stderr, "Malloc failed\r\n");
		    die();
		}

	    /* create new window */

	    if (setjmp(Sjbuf)) {
		fprintf(stderr, "ptydaemon did not respond\r\n");
		return(1);
	    }
	    signal(SIGALRM, catch_alarm);
	    alarm(15);
#ifdef LOGINCMD
	    if (loginflag)
		fd = getloginpty(dev, 0, &ptyerror);
	    else
#endif LOGINCMD
		fd = getshellpty(dev, 0, addenvp, &ptyerror);
	    alarm(0);
	    signal(SIGALRM, sig_die);
	    if (fd < 0) {
		fprintf(stderr, "ptydaemon request error: ");
		fprintf(stderr, "%s\r\n", pty_errlist[ptyerror]);
		return(1);
	    }

#ifdef hp9000s500
	    {
		struct stat sbuf;
		if (fstat(0, &sbuf) == 0)
		    ioctl(fd, 0x80002601, sbuf.st_rdev);
	    }
	    /* note: this isn't documented, and it isn't legal,
		so keep your mouth shut */
#endif

	    /* Set O_NDELAY on pty */

	    if ((fileflags = fcntl(fd, F_GETFL, 0)) == -1) {
		perror("Could not get file flags for pty\r");
		return(1);
	    }

	    if (fcntl(fd, F_SETFL, (fileflags | O_NDELAY)) == -1) {
		perror("Could not set O_NDELAY mode on pty\r");
		return(1);
	    }

	    layer[layno].fd = fd;

	    dummy = 1;
	    if (ioctl(fd ,TIOCTRAP, &dummy) != 0) {
		perror("TIOCTRAP ioctl failed\r");
		die();
	    }

	    if (ioctl(fd ,TIOCMONITOR, &dummy) != 0) {
		perror("TIOCMONITOR ioctl failed\r");
		die();
	    }

	    if (ioctl(fd, TIOCSIGMODE, TIOCSIGBLOCK) != 0) {
		perror("TIOCSIGMODE ioctl failed\r");
		die();
	    }

	    strcpy(layer[layno].name, layer_name);
	    layer[layno].blocked = 1;
	    layer[layno].escape = savedmodes.c_cc[VSWTCH];
	    ioctl(fd, TCSETA, &savedmodes);
	    strcpy(layer[layno].path, dev);
#ifdef INFO
	    fprintf(stderr, "[layer %s created]\r\n", layer[layno].name);
#endif INFO
	    push(layno);
	    run_layer(layer[layno].name);
	}
    }
    return(0);
}

proc block(arg)
char *arg;
{
    int layno, nargs, i;
    char *arps[50];
    struct termio foo;

    nargs = getargs(arg, arps);
    if (nargs == 0) {
	fprintf(stderr, "Must specify a layer name\r\n");
	return;
    }
    for (i=0; i < nargs; i++) {
	arg = arps[i];
	layno = findno(arg);
	switch (layno) {
	    case AMBIG:
		fprintf(stderr, "'%s' is ambiguous\r\n", arg);
		break;
	    case NONE:
		fprintf(stderr, "No such layer '%s'\r\n", arg);
		break;
	    default:
		layer[layno].blocked = 1;
		ioctl(layer[layno].fd, TCGETA, &foo);
		foo.c_cflag |= LOBLK;
		ioctl(layer[layno].fd, TCSETA, &foo);
#ifdef INFO
		fprintf(stderr, "[layer %s blocked]\r\n", layer[layno].name);
#endif
		break;
	}
    }
}

proc delete(arg)
char *arg;
{
    int nargs, i;
    char *arps[50];

    nargs = getargs(arg, arps);
    if (nargs == 0) {
	fprintf(stderr, "Must specify a layer name\r\n");
	return;
    }
    for (i=0; i < nargs; i++) {
	int layno;
	arg = arps[i];
	layno = findno(arg);
	switch (layno) {
	    case AMBIG:
		fprintf(stderr, "'%s' is ambiguous\r\n", arg);
		break;
	    case NONE:
		fprintf(stderr, "No such layer '%s'\r\n", arg);
		break;
	    default:
		close(layer[layno].fd);
		layer[layno].fd = NONE;
		layer[layno].pid = NONE;
		layer[layno].status = NONE;
		layer[layno].numread = 0;
		layer[layno].escape = CONTROL_Z;
#ifdef INFO
		fprintf(stderr, "[layer %s deleted]\r\n", layer[layno].name);
#endif
		strcpy(layer[layno].name, "");
		strcpy(layer[layno].path, "");
		push(layno);        /* bring layer to top of stack and... */
		pop();              /* blow it away */
		break;
	}
    }
}

proc help(arg)          /* this is a cheap help routine, could be better */
char *arg;
{
    struct jtabstruct *index;

    if ( (arg != NULL) && (*arg != '\0'))
	printf("arg is %s\r\n", arg);
    fprintf(stderr, "Commands are:\r\n");
    index = jumptable;
    while ( (*index).func != 0 ) {
	fprintf(stderr, "\t%s %s\r\n", (*index).cmd_name, (*index).cmd_syntax);
	index++;
    }
    return;
}

proc layers(arg)
char *arg;
{
    int layno, i, lawng = 0;
    int nargs;
    char *arps[50], grplist[100], cmd[220];

    strcpy(grplist, "");
    nargs = getargs(arg, arps);
    if (nargs > 0 && strcmp(arps[0], "-l") == 0)
	    lawng++;
    if (nargs == 0 || (nargs == 1 && lawng) ) {
	fprintf(stderr, "pty pathname      blocked/chars     escape      name\r\n");
	fprintf(stderr, "------------      -------------     ------      ----\r\n");
	for (i=0; i < MAX_LAYER; i++)
	    if (layer[i].name[0] != '\0') {
		fprintf(stderr, "%s\t    %c", layer[i].path, layer[i].blocked?'y':'n');
		if ( layer[i].blocked && layer[i].numread)
		    fprintf(stderr, "%6d      ", layer[i].numread);
		else
		    fprintf(stderr, "            ");
		if (layer[i].escape < 040)
		    fprintf(stderr, "     ^%c", layer[i].escape | 0100);
		else
		    fprintf(stderr, "     %c ", layer[i].escape);
		fprintf(stderr, "         %s\n", layer[i].name);
		if (layer[i].pid != NONE) {
		    sprintf(cmd, "%d ", layer[i].pid);
		    strcat(grplist, cmd);
		}
	    }
    }
    else {
	for (i=lawng; i < nargs; i++) {
	    arg = arps[i];
	    layno = findno(arg);
	    switch (layno) {
		case AMBIG:
		    fprintf(stderr, "'%s' is ambiguous\r\n", arg);
		    break;
		case NONE:
		    fprintf(stderr, "No such layer '%s'\r\n", arg);
		    break;
		default:
#ifdef INFO
		    fprintf(stderr, "[layer %s %sblocked]\r\n", layer[layno].name,
					layer[layno].blocked ? "" : "un");
#endif
		    if (layer[layno].pid != NONE) {
			sprintf(cmd, "%d ", layer[layno].pid);
			strcat(grplist, cmd);
		    }
		    break;
	    }
	}
    }
    if (current_layer == NONE)
	fprintf(stderr, "No current layer\r\n");
    else
	fprintf(stderr, "[current layer: %s]\r\n", layer[current_layer].name);

    if (!lawng || grplist[0] == '\0')
	return;
    arps[0] = "/bin/ps";
    arps[1] = "-f";
    arps[2] = "-g";
    arps[3] = grplist;
    arps[4] = NULL;
    bang(FROMLAYERS, arps);

    return;
}

proc resume(arg)
char *arg;
{
    int layno, nargs;
    char *arps[50];

    nargs = getargs(arg, arps);
    if (nargs > 1) {
	fprintf(stderr, "Only one argument allowed\r\n");
	return;
    }

    if (*arg == '\0') {
	if (current_layer == NONE)
	    fprintf(stderr, "No current layer\r\n");
	else
	    run_layer(layer[current_layer].name);
    }
    else {
	layno = findno(arg);
	switch (layno) {
	    case AMBIG:
		fprintf(stderr, "'%s' is ambiguous\r\n", arg);
		break;
	    case NONE:
		fprintf(stderr, "No such layer '%s'\r\n", arg);
		break;
	    default:
		push(layno);
		run_layer(arg);
		break;
	}
    }
}

proc toggle(arg)
char *arg;
{
    int nargs, oldlayer, newlayer;
    char *arps[50];

    nargs = getargs(arg, arps);
    if (nargs > 0) {
	fprintf(stderr, "No arguments allowed\r\n");
	return;
    }
    oldlayer = pop();
    if (oldlayer == NONE) {
	fprintf(stderr, "No existing layers\r\n");
	return;
    }
    newlayer = pop();
    if (newlayer == NONE) {
	fprintf(stderr, "Only one layer exists\r\n");
	push(oldlayer);
	run_layer(layer[oldlayer].name);
	return;
    }
    push(oldlayer);
    push(newlayer);
    run_layer(layer[newlayer].name);
    return;
}

proc unblock(arg)
char *arg;
{
    int layno;
    int nargs, i;
    char *arps[50];
    struct termio foo;

    nargs = getargs(arg, arps);
    if (nargs == 0) {
	fprintf(stderr, "Must specify a layer name\r\n");
	return;
    }
    for (i=0; i < nargs; i++) {
	arg = arps[i];
	layno = findno(arg);
	switch (layno) {
	    case AMBIG:
		fprintf(stderr, "'%s' is ambiguous\r\n", arg);
		break;
	    case NONE:
		fprintf(stderr, "No such layer '%s'\r\n", arg);
		break;
	    default:
		layer[layno].blocked = 0;
		ioctl(layer[layno].fd, TCGETA, &foo);
		foo.c_cflag &= ~LOBLK;
		ioctl(layer[layno].fd, TCSETA, &foo);
#ifdef INFO
		fprintf(stderr, "[layer %s unblocked]\r\n", layer[layno].name);
#endif INFO
		break;
	}
    }
}

proc quit(arg)
char *arg;
{
    int layno;

    for (layno = 0; layno < MAX_LAYER; layno++)
	if (layer[layno].name[0] != '\0') {
	    close(layer[layno].fd);
	    fprintf(stderr, "[layer %s deleted]\r\n", layer[layno].name);
	}
    exit(0);
}


proc run_layer(which)        /* 'which' is the name of the layer to run */
char *which;
{
    int check_esc_flag, nread, inmask, ptymask, readfds, exceptfds, i;
    int layno, fd;
    int layer_croaked;          /* Did layer die somehow? */
    int layer_stopped;
    char interrupt;
    struct termio foo;
    long mask;

    layno = findno(which);
    switch (layno) {
	case AMBIG:
	    fprintf(stderr, "'%s' is ambiguous\r\n", which);
	    return;
	case NONE:
	    fprintf(stderr, "No layer named '%s'\r\n", which);
	    return(1);
	default:
	    which = layer[layno].name;
	    break;
    }
    /* run layer #layno */
    fd = layer[layno].fd;
#ifdef SIGTSTP
    if (STOPPED(layer[layno].status))  { /* if the layer was stopped */
	kill(layer[layno].pid,SIGCONT);  /* start it up again */
	layer[layno].status = NONE; /* reset status */
    }
#endif

    push(layno);

#ifdef INFO
    fprintf(stderr, "[running layer %s]\r\n", which);
#endif INFO

    ioctl(fd, TCGETA, &foo);
    interrupt = foo.c_cc[VINTR];
    if ( !(foo.c_lflag & ISIG) )
	interrupt = '\0';


/*  setxonstate(xonstate[current]); */

    /* write out anything that has been buffered */

    if (layer[layno].numread != 0) {
	if (write(1, layer[layno].buffer, layer[layno].numread) != layer[layno].numread) {
	    perror("write to stdout failed\r");
	    die();
	}
	layer[layno].numread = 0;
    }

#ifdef SIGTSTP
    if (layer[layno].status != NONE && !STOPPED(layer[layno].status)) {
#else
    if (layer[layno].status != NONE) {
#endif
	cleanup(layno);                     /* this layer has died */
	return(1);
    }
    inmask   = 1; /* standard in (file descriptor 0) */
    ptymask  = 0;
    for (i = 0; i < MAX_LAYER; i++) {
	/* only read from layers that have space left in their buffers */
	if (layer[i].fd != NONE && layer[i].numread != LAYBUFSIZE)
	    ptymask |= (1 << layer[i].fd);
    }

/*  check_esc_flag = 1; */

    raw_tty();
    signal(SIGINT, catch_break);


    for (;;) {
	char c;

	sigsetmask(0L);
	readfds   = (inmask | ptymask);
	exceptfds = ptymask;
	if (!croakflag) {           /* croakflag set when SIGCLD rec'd */
	    if (select(NFDS, &readfds, (int *) 0,
		    &exceptfds, (struct timeval *) 0) < 0) {
		if (errno != EINTR) {
		    perror("Select failed");
		    die();
		}
		readfds   = 0;
		exceptfds = 0;
	    }
	} else
	    readfds = exceptfds = 0;
	if (croakflag)  {
	    layer_stopped = handle_death(&readfds);
	    if (layer_stopped) { /* if layer stopped return to shl prompt */
	 	current_layer = NONE;
		croakflag = 0;
		return(0);
	    }
	}

	sigsetmask(MASK(SIGINT) | MASK(SIGCLD));


	if (exceptfds & ptymask) {

	    /* Handshake ioctls */

	    for (i = 0; i < MAX_LAYER; i++) {
		if ( (layer[i].fd != NONE) && (layer[i].status == NONE) &&
			(exceptfds & (1 << layer[i].fd)) != 0) {

		    struct request_info reqbuf;
		    struct termio       tbuf;
		    int request;

		    if (ioctl(layer[i].fd, TIOCREQGET, &reqbuf) != 0) {
			perror("TIOCREQGET ioctl failed\r");
			die();
		    }

		/* get pid if not already there */
		    if (layer[i].pid == NONE)
			layer[i].pid = reqbuf.pgrp;

		    request = reqbuf.request;

		    layer_croaked = 0;
		    if (request == TCSETA || request == TCSETAW
						|| request == TCSETAF) {
			if (ioctl(layer[i].fd, reqbuf.argget, &tbuf) != 0) {
			    perror("reqbuf.argget ioctl failed\r");
			    die();
			}
		    /* update loblk and swtch settings */
			if (tbuf.c_cflag & LOBLK)
			    layer[i].blocked = 1;
			else
			    layer[i].blocked = 0;
			layer[i].escape = tbuf.c_cc[VSWTCH];
			if (layer[i].escape == '\0')
			    layer[i].escape = CONTROL_Z;

		    /* stty 0 or hangup done in layer */
			if ((tbuf.c_cflag & CBAUD) == 0)
			    layer_croaked = 1;


		    /*
			if ((tbuf.c_iflag & IXON) == 0)
			    xonstate[i] = XON_OFF;
			else
			    xonstate[i] = XON_ON;

			if (i == current_layer)
			    setxonstate(xonstate[i]);
		    */
		    }

		    /* Finish handshake */

		    if (ioctl(layer[i].fd, TIOCREQSET, &reqbuf) != 0) {
			perror("TIOCREQSET ioctl failed\r");
			die();
		    }

		    if (layer_croaked) {
			if (i == current_layer) {
			    if (layer[i].numread != 0) {
				if (write(1, layer[i].buffer,layer[i].numread)
						    != layer[i].numread) {
				    perror("write to stdout failed\r");
				    die();
				}
				layer[layno].numread = 0;
			    }
			    layer[i].status = 0;
			    cleanup(i);
			    return(0);
			}
			layer[i].status = 0;
			readfds |= (1<<layer[i].fd);
			ptymask &= ~(1<<layer[i].fd);
		    }
		}
	    }
	}

#ifdef INITOPTION
	if (*initcmd) {
	    write(layer[layno].fd, initcmd, strlen(initcmd));
	    strcpy(initcmd, "");
	}
#endif INITOPTION

	if (readfds & inmask) {
	    /* this should really read more than 1 char per read */
	    switch (read(0, &c, 1)) {
		case -1:
		    perror("read from stdin failed\r");
		    die();
		    break;
		case 0:
		    break;
		case 1:
		    if (c == layer[current_layer].escape) {
			if (c < 040)
			    fprintf(stderr, "^%c\r\n", c | 0100);
			else
			    fprintf(stderr, "%c\r\n", c);
			return(0);
		    }

		    write(layer[layno].fd, &c, 1);
	    /* this interrupt stuff is an experiment that didn't
	       turn out too well; needs a bit more work.
	       the idea was to flush the output buffer when the
	       interrupt character was received. */
		    if ( interrupt && (c == interrupt) ) {
			layer[layno].numread = 0;
			layer[layno].buffer[0] = '\0';
		    }
		    break;
	    }
	}

	if (readfds & ptymask) {
	    for (i = 0; i < MAX_LAYER; i++) {
		if (layer[i].fd != NONE &&
				(readfds & (1 << layer[i].fd)) != 0) {

		    if (i == layno)  {
			if ((nread = read(layer[i].fd, layer[i].buffer,
							LAYBUFSIZE)) < 0) {
			    perror("read from pty failed\r");
			    die();
			}

			if (nread != 0) {
			    if (write(1, layer[i].buffer, nread) != nread) {
				perror("write to pty failed\r");
				die();
			    }
			}
		    }
		    else {
			if ((nread = read(layer[i].fd,
				&layer[i].buffer[layer[i].numread],
				    LAYBUFSIZE - layer[i].numread)) < 0) {
			    perror("read from pty failed\r");
			    die();
			}

			if ( !layer[i].blocked ) {
			    write(1, layer[i].buffer, nread);
			    layer[i].numread = 0;
			}
			else {
			    layer[i].numread += nread;
			    if (layer[i].numread == LAYBUFSIZE)
				ptymask &= ~(1 << layer[i].fd);
			}
		    }
		}
		/* here check to see if the layer is defunct, and if so */
		/* release pty (close master) and set table entry to NONE */
#ifdef SIGTSTP
		if (layer[i].status != NONE && !STOPPED(layer[i].status)) {
#else
		if (layer[i].status != NONE) {
#endif
		    if (i == current_layer) {
			cleanup(i);
			return(1);
		    }
		    close(layer[i].fd);
		    ptymask &= ~(1 << layer[i].fd);
		    layer[i].fd = NONE;
		}
	    }   /* end for */
	}
    }
}


proc findno(name)            /* find layer number of layer 'name' */
char *name;
{
    int layno, prefixes = 0, match;
    char tmpnam[10];
    int i;

    if ( (name == NULL) || (*name == '\0') )
	return(NONE);

    i = getnum(name);
    if (i != -1) {
	sprintf(tmpnam,"(%d)",i);
	name = tmpnam;
    }

    for (layno = 0; layno < MAX_LAYER; layno++ ) {
	if (strcmp(name, layer[layno].name) == SAME)
	    return(layno);
	if (strncmp(name, layer[layno].name, strlen(name)) == SAME) {
	    prefixes++;
	    match = layno;
	}
    }
    if (prefixes == 1)
	return(match);
    else if (prefixes > 1)
	return(AMBIG);
    else
	return(NONE);
}


proc orphanage()            /* catch SIGCLD */
{
    croakflag = 1;
}


proc cleanup(layno)
int layno;
{
    int exitstat, signo, core;
    signo = layer[layno].status & 0177;
    core = layer[layno].status & 0200;
    exitstat = layer[layno].status >> 8;
    fprintf(stderr, "[layer %s terminated ", layer[layno].name);
    if (exitstat)
	printf("with exit status %d]\r\n", exitstat);
    else if (signo)
	printf("due to signal %d%s]\r\n", signo,
					(core ? " (core dumped)" : ""));
    else
	printf("normally]\r\n");
    delete(layer[layno].name);
    return;
}


proc handle_death(fdmask)
int *fdmask;
{
    int ret,  pid;
#ifdef SIGTSTP
    union wait stat;
#else
    int stat;
#endif

/*
    This routine needs a bit of explanation.
    The croakflag variable is set when a SIGCLD signal is recieved,
    and this routine is called by run_layer when it sees that croakflag
    is suddenly set. This routine does a wait(2) on the dead process,
    being careful to use setjmp and alarm to avoid possible hanging.
    It then saves the exit information in the status field of the layer
    structure after determining which layer died (markdead). Then
    croakflag is reset to 0, and the SIGCLD signal is reset. At this
    point, if there are other dead children, another SIGCLD will be
    received, and croakflag will be set again. That's the tricky part.
    If run_layer() was not reading from a layer that died, the select bit
    mask is modified so run_layer() notices this "event."
*/
#ifdef SIGTSTP
    sigsetmask(MASK(SIGINT) | MASK(SIGCLD));	/* unblocked in run_layer() */
#endif
    while (croakflag)  {
#ifdef SIGTSTP
	    pid = wait3(&stat.w_status, WUNTRACED|WNOHANG, (int *)0);
	    vec.sv_handler = orphanage; /* re-install after wait */
	    sigvector(SIGCLD,&vec, (struct sigvec *)0);
#else
     
	if (!setjmp(Sjbuf)) {
	    signal(SIGALRM, catch_alarm);
	    alarm(3);           /* make sure the wait doesn't hang */
	    pid = wait(&stat);
	    alarm(0);
	    signal(SIGALRM, sig_die);
	}
#endif
#ifdef SIGTSTP
	ret = markdead(pid, stat.w_status);
	if (WIFSTOPPED(stat.w_status)) 
		return(1);
#else
	ret = markdead(pid, stat);
#endif
	if (ret != -1)
	    *fdmask |= (1 << ret);
#ifdef SIGTSTP
	croakflag = (pid > 0) ? 1 : 0; /* loop until wait3() finds no kids */
#else
	croakflag = 0;
#endif
#ifndef SV_BSDSIG	/* re-install handler if using signal */
	signal(SIGCLD, orphanage);
#endif
    }
return(0);
}

proc markdead(pid, stat)        /* just find which layer has process pid */
int pid, stat;                  /* running in it */
{
    int i;
    for (i=0; i < MAX_LAYER; i++)
	if (layer[i].pid == pid) {
	    layer[i].status = stat;
	    return(layer[i].fd);
	}
    return(-1);
}



proc catch_alarm()
{
    longjmp(Sjbuf, 1);
}

proc catch_break()
{
    if (current_layer != NONE)
	ioctl(layer[current_layer].fd, TIOCBREAK, 0);
    signal(SIGINT, catch_break);
}

proc die()
{
    reset_tty();
    fprintf(stderr, "Fatal error. Disconnected\r\n");
    exit(1);
}


proc sig_die(signo)
int signo;
{
    fprintf(stderr, "SHL got signal #%d\r\n", signo);
    die();
}


proc reset_tty()
{
    ioctl(0, TCSETAW, &savedmodes);
    fcntl(0, F_SETFL,  ~O_NDELAY & fcntl(0, F_GETFL, 0));
}


proc raw_tty()
{
    struct termio raw;

    ioctl(0, TCGETA, &raw);

    raw.c_iflag &= ~(INLCR | ICRNL | IGNCR | IUCLC | IXANY);
    raw.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONOCR | ONLRET);
    raw.c_oflag |= OPOST;
    raw.c_lflag &= ~(ICANON | ISIG | ECHO);
    raw.c_cc[VEOF] = '\01';
    raw.c_cc[VEOL] = '\0';

    (void)ioctl(0, TCSETAW, &raw);

    fcntl(0, F_SETFL, O_NDELAY | fcntl(0, F_GETFL, 0));
}

proc fakemodes()
{
    savedmodes.c_iflag = BRKINT | IGNPAR | IXON | IXOFF | ICRNL;
    savedmodes.c_oflag = OPOST | ONLCR | TAB3;
    savedmodes.c_lflag = ISIG | ICANON; /* echo intentionally omitted */
    savedmodes.c_cflag = B9600 | CS8 | CREAD | CLOCAL | LOBLK;
    savedmodes.c_cc[VINTR] = CINTR;
    savedmodes.c_cc[VQUIT] = CQUIT;
    savedmodes.c_cc[VERASE] = CERASE;
    savedmodes.c_cc[VKILL] = CKILL;
    savedmodes.c_cc[VEOF] = CEOF;
    savedmodes.c_cc[VSWTCH] = CSWTCH;

}


/** NOTE: the following routine is AT&T source code **/

/*
 * generate a vector of pointers (arps) to the
 * substrings in string "s".
 * Each substring is separated by blanks and/or tabs.
 *	s	-> string to analyze
 *	arps	-> array of pointers
 * returns:
 *	i	-> # of subfields
 */
proc getargs(s, arps)
register char *s, *arps[];
{
	register int i;

	i = 0;
	while (1) {
		arps[i] = NULL;
		while (*s == ' ' || *s == '\t')
			*s++ = '\0';
		if (*s == '\n')
			*s = '\0';
		if (*s == '\0')
			break;
		arps[i++] = s++;
		while (*s != '\0' && *s != ' '
			&& *s != '\t' && *s != '\n')
				s++;
	}
	return(i);
}

/********* END AT&T source code **************/

proc name(arg)
char *arg;
{
    int layno, nargs;
    char *from, *to;
    char *arps[50];

    nargs = getargs(arg, arps);
    switch (nargs) {
	case 0:
	    fprintf(stderr, "Need at least one argument\r\n");
	    return;
	    break;
	case 1:
	    if (current_layer == NONE) {
		fprintf(stderr, "No current layer to name\r\n");
		return;
		break;
	    }
	    from = layer[current_layer].name;
	    to = arps[0];
	    break;
	case 2:
	    from = arps[0];
	    to = arps[1];
	    break;
	default:
	    fprintf(stderr, "Only two arguments allowed\r\n");
	    return;
	    break;
    }

    /* Don't Allow user to specify a numeric layer name. A numeric */
    /* layer name would conflict with the default conventions for  */
    /* layer names. A numeric layer name is of the form # or (#),  */
    /* where # is a decimal number.                                */

    if (!valid_name(to)) 
    {
	fprintf(stderr, "Illegal layer name specified\r\n");
	return;
    }

    layno = findno(from);
    switch (layno) {
	case AMBIG:
	    fprintf(stderr, "'%s' is ambiguous\r\n", from);
	    return;
	    break;
	case NONE:
	    fprintf(stderr, "No such layer '%s'\r\n", from);
	    return;
	    break;
	default:
	    break;
    }

    if (name_exists(to) >= 0) { 
       fprintf(stderr, "Layer '%s' already exists\r\n", to);
       return;
    }

#ifdef INFO
    fprintf(stderr, "[layer %s named %s]\r\n", layer[layno].name, to);
#endif INFO
    strcpy(layer[layno].name, to);
    return;
}

/* Getnum() makes sure that all characters are numeric before calling */
/* atoi(). -1 is returned if they are not.                            */

proc getnum(arg)
    char *arg;
{
    char *s;

    if (arg == (char *)0 || *arg == '\0')
	return(-1);

    s = arg;
    while (*s != '\0') {
	if (*s < '0' || *s >'9')
	    return (-1);
	s++;
    }
    return(atoi(arg));
}

#ifdef LOGINCMD
proc login(arg)
{
    char s[100];

    loginflag++;
    strcpy(s, "- ");
    strcat(s, arg);
    create(s);
}
#endif LOGINCMD

#ifdef BANG
proc bang(arg, argv)        /* fork a sub-shell and run something in it */
char *arg;                  /* the non-null argument is used for the exec */
char **argv;
{
    int pid, i, stat;

    pid = vfork();
    switch (pid) {
	case -1:
	    perror("fork failed\r");
	    return;
	    break;
	case 0:                     /* child */
	    for (i=getnumfds()-1;i>=3;i--)
		close(i);
	    for (i=0; i < 18; i++)
		signal(i, SIG_DFL);
	    for (i=19; i < NSIG; i++)
		signal(i, SIG_DFL);
#ifdef SV_BSDSIG
	    vec.sv_handler = SIG_DFL;
	    sigvector(SIGCLD,&vec, (struct sigvec *)0);
#else
	    signal(SIGCLD,SIG_DFL);
#endif

	    reset_tty();

	    sigsetmask(0);
	    if (arg == FROMLAYERS ) {
		arg = argv[0];
		execv(arg, argv);
	    }
	    else if (arg == NULL || *arg == '\0') {
		arg = (char *) getenv("SHELL");
		if (arg == NULL || *arg == '\0')
		    arg = "/bin/sh";
		execlp(arg, arg, 0);
	    }
	    else
		execl("/bin/sh", "sh", "-c", arg, 0);
	    fprintf(stderr, "exec failed: %s\r\n", arg);
	    _exit(0);
	    break;
	default:
#ifdef SV_BSDSIG
	    vec.sv_handler = SIG_DFL;
	    sigvector(SIGCLD,&vec, (struct sigvec *)0);
#else
	    signal(SIGCLD, SIG_DFL);
#endif
	    signal(SIGINT, SIG_IGN);
	/* a layer might die while we're waiting for the subshell to
	    terminate, so be sure to keep track of deaths */
	    while ( (i = wait(&stat)) != pid)
		markdead(i, stat);
#ifdef SV_BSDSIG
	    vec.sv_handler = orphanage;
	    sigvector(SIGCLD,&vec, (struct sigvec *)0);
#else
	    signal(SIGCLD, orphanage);
#endif
	    signal(SIGINT, catch_break);
	    break;
    }
    printf("\r\n");
    raw_tty();
    return;
}
#endif BANG

/******* stack routines *********/

#define STACK struct stack_type

STACK {
    int value;
    STACK *next;
};

STACK *tos = NULL;          /* tos == top of stack */

STACK *new()
{
    STACK *foo;
    foo = (STACK *) malloc(sizeof(STACK));
    if (foo == NULL) {
	fprintf(stderr, "malloc failed\r\n");
	die();
    }
    return(foo);
}


proc push(val)
int val;
{
    STACK *p, *q;
    if (tos == NULL) {          /* if tos is null, create new entry */
	tos = new();
	tos->value = val;
	tos->next = NULL;
	current_layer = val;
	return;
    }
    if (tos->value == val) {    /* if val is already at tos, do nothing */
	current_layer = val;
	return;
    }

    p = tos;
    while (p->next != NULL) {
	if (p->next->value == val) {    /* if val is found, bring */
	    q = p->next;                /* it to top of stack */
	    p->next = q->next;
	    q->next = tos;
	    tos = q;
	    current_layer = val;
	    return;
	}
	p = p->next;
    }
    /* val not found, make new entry at top of list */
    p = new();
    p->value = val;
    p->next = tos;
    tos = p;
    current_layer = val;
    return;
}

proc int pop()
{
    int i;
    STACK *p;

    if (tos == NULL)
	return(NONE);

    p = tos;
    i = p->value;
    tos = p->next;
    free(p);
    current_layer = top();
    return(i);
}

proc int top()
{
    if (tos == NULL)
	return(NONE);
    else
	return(tos->value);
}

#ifdef DUMP
proc dump()
{
    STACK *p;
    int i;

    p = tos;
    printf("stack dump:\r\n");
    while (p != NULL) {
	i = p->value;
	printf("  %d  '%s'\r\n", i, layer[i].name);
	p = p->next;
    }
}
#endif

/* valid_name determines if name is of the form # or (#), and */
/* returns -1 if it is.                                       */

proc valid_name(name)
char *name;
{
   int len;
   char name_in_parens[80];

   if (getnum(name) != -1)  /*  is a number  */
      return(0);  /* not a valid layer name */
   else if ( *name == '('  &&  name[(len=strlen(name))-1] == ')' ) {        
           strncpy(name_in_parens, &name[1], len-2);
           name_in_parens[len-2] = '\0';
	   
           if (getnum(name_in_parens) != -1)  /* was a number in parens */
              return(0);  /* not a valid layer name */
           else
              return(1);  /* valid layer name */
        }
        else
           return(1);     /* valid layer name */
}        


/* name_exists returns layer number if name exists else it returns -1 */
proc name_exists(name)  
char *name;
{
   int layno;

   for (layno=0; layno<MAX_LAYER; layno++) {
       if (strcmp(name,layer[layno].name) == SAME)
           return(layno);
   }
   return(-1);
}   

