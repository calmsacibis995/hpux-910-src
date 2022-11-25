static char *HPUX_ID = "@(#) $Revision: 70.4 $";

/*----------------------------------------------------------------------------
   MODIFICATION HISTORY

   Date         User            Changes
   11/13/91     rajs            Modified for hardware flow control.
				-g option is modified assuming that SIGTSTP
				is defined for versions later than 7.0.
----------------------------------------------------------------------------*/


#include <stdio.h>
#include <sys/types.h>
#define _TERMIOS_INCLUDED
#define _INCLUDE_TERMIO
#include <sys/termio.h>
#include <sys/termiox.h>

#include <sys/modem.h>
#include <signal.h>	/* just to find if SIGTSTP is defined -- yuck! */

#ifndef VSWTCH          /* until this is defined in termio.h */
#define LOBLK 0020000
#define VSWTCH 7
#define CSWTCH '\032'
#endif


#if defined (SIGTSTP) || defined (SIGWINCH)
#include <sys/bsdtty.h>
#endif 

#ifndef NONLS
#include <locale.h>
#include <nl_ctype.h>
#endif /* NONLS */

#define CBS	010
#define CCAN	030

struct {
	char	*string;
	int	speed;
} speeds[] = {
	"0",	B0,
	"50",	B50,
	"75",	B75,
	"110",	B110,
	"134",	B134,
	"134.5",B134,
	"150",	B150,
	"200",	B200,
	"300",	B300,
	"600",	B600,
	"900",	B900,
	"1200",	B1200,
	"1800",	B1800,
	"2400",	B2400,
	"3600",	B3600,
	"4800",	B4800,
	"7200",	B7200,
	"9600",	B9600,
	"19200", B19200,
	"38400", B38400,
	"57600", B57600,
	"115200", B115200,
	"230400", B230400,
	"460800", B460800,
	"exta",	EXTA,
	"extb",	EXTB,
	0,
};
struct mds {
	char	*string;
	int	set;
	int	reset;
};

struct mds cmodes[] = {
	"-parity", CS8, PARENB|CSIZE,
	"-evenp", CS8, PARENB|CSIZE,
	"-oddp", CS8, PARENB|PARODD|CSIZE,
	"parity", PARENB|CS7, PARODD|CSIZE,
	"evenp", PARENB|CS7, PARODD|CSIZE,
	"oddp", PARENB|PARODD|CS7, CSIZE,
	"parenb", PARENB, 0,
	"-parenb", 0, PARENB,
	"parodd", PARODD, 0,
	"-parodd", 0, PARODD,
	"cs8", CS8, CSIZE,
	"cs7", CS7, CSIZE,
	"cs6", CS6, CSIZE,
	"cs5", CS5, CSIZE,
	"cstopb", CSTOPB, 0,
	"-cstopb", 0, CSTOPB,
	"hupcl", HUPCL, 0,
	"hup", HUPCL, 0,
	"-hupcl", 0, HUPCL,
	"-hup", 0, HUPCL,
	"clocal", CLOCAL, 0,
	"-clocal", 0, CLOCAL,
	"loblk", LOBLK, 0,
	"-loblk", 0, LOBLK,
	"cread", CREAD, 0,
	"-cread", 0, CREAD,
	"raw", CS8, (CSIZE|PARENB),
	"-raw", (CS7|PARENB), CSIZE,
	"cooked", (CS7|PARENB), CSIZE,
	"hp", (CS8|CREAD), (CSIZE|PARENB|PARODD|CSTOPB),
	0
};

struct mds imodes[] = {
	"ignbrk", IGNBRK, 0,
	"-ignbrk", 0, IGNBRK,
	"brkint", BRKINT, 0,
	"-brkint", 0, BRKINT,
	"ignpar", IGNPAR, 0,
	"-ignpar", 0, IGNPAR,
	"parmrk", PARMRK, 0,
	"-parmrk", 0, PARMRK,
	"inpck", INPCK, 0,
	"-inpck", 0,INPCK,
	"istrip", ISTRIP, 0,
	"-istrip", 0, ISTRIP,
	"inlcr", INLCR, 0,
	"-inlcr", 0, INLCR,
	"igncr", IGNCR, 0,
	"-igncr", 0, IGNCR,
	"icrnl", ICRNL, 0,
	"-icrnl", 0, ICRNL,
	"-nl", ICRNL, (INLCR|IGNCR),
	"nl", 0, ICRNL,
	"iuclc", IUCLC, 0,
	"-iuclc", 0, IUCLC,
	"lcase", IUCLC, 0,
	"-lcase", 0, IUCLC,
	"LCASE", IUCLC, 0,
	"-LCASE", 0, IUCLC,
	"ixon", IXON, 0,
	"-ixon", 0, IXON,
	"ixany", IXANY, 0,
	"-ixany", 0, IXANY,
	"ixoff", IXOFF, 0,
	"-ixoff", 0, IXOFF,
#ifdef IENQAK
	"ienqak", IENQAK, 0,
	"-ienqak", 0, IENQAK,
#endif /* IENQAK */
	"raw", 0, -1,
	"-raw", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON), 0,
	"cooked", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON), 0,
	"sane", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON),
		(IGNBRK|PARMRK|INPCK|INLCR|IGNCR|IUCLC|IXOFF),
	"hp", (BRKINT|IGNPAR|ICRNL|IXON|IXANY),
		(IGNBRK|PARMRK|INPCK|INLCR|ISTRIP|IGNCR|IUCLC|IXOFF),
	0
};

struct mds lmodes[] = {
	"isig", ISIG, 0,
	"-isig", 0, ISIG,
	"icanon", ICANON, 0,
	"-icanon", 0, ICANON,
	"xcase", XCASE, 0,
	"-xcase", 0, XCASE,
	"lcase", XCASE, 0,
	"-lcase", 0, XCASE,
	"-iexten", 0, IEXTEN,
	"iexten", IEXTEN, 0,
	"LCASE", XCASE, 0,
	"-LCASE", 0, XCASE,
	"echo", ECHO, 0,
	"-echo", 0, ECHO,
	"echoe", ECHOE, 0,
	"-echoe", 0, ECHOE,
	"echok", ECHOK, 0,
	"-echok", 0, ECHOK,
	"lfkc", ECHOK, 0,
	"-lfkc", 0, ECHOK,
	"echonl", ECHONL, 0,
	"-echonl", 0, ECHONL,
	"noflsh", NOFLSH, 0,
	"-noflsh", 0, NOFLSH,
	"raw", 0, (ISIG|ICANON|XCASE),
	"-raw", (ISIG|ICANON), 0,
	"cooked", (ISIG|ICANON), 0,
	"sane", (ISIG|ICANON|ECHO|ECHOK),
		(XCASE|ECHOE|ECHONL|NOFLSH),
	"hp", (ISIG|ICANON|ECHO|ECHOK|ECHOE),
		(XCASE|ECHONL|NOFLSH),
	0,
};

struct mds omodes[] = {
	"opost", OPOST, 0,
	"-opost", 0, OPOST,
	"olcuc", OLCUC, 0,
	"-olcuc", 0, OLCUC,
	"lcase", OLCUC, 0,
	"-lcase", 0, OLCUC,
	"LCASE", OLCUC, 0,
	"-LCASE", 0, OLCUC,
	"onlcr", ONLCR, 0,
	"-onlcr", 0, ONLCR,
	"-nl", ONLCR, (OCRNL|ONLRET),
	"nl", 0, ONLCR,
	"ocrnl", OCRNL, 0,
	"-ocrnl",0, OCRNL,
	"onocr", ONOCR, 0,
	"-onocr", 0, ONOCR,
	"onlret", ONLRET, 0,
	"-onlret", 0, ONLRET,
	"fill", OFILL, OFDEL,
	"-fill", 0, OFILL|OFDEL,
	"nul-fill", OFILL, OFDEL,
	"del-fill", OFILL|OFDEL, 0,
	"ofill", OFILL, 0,
	"-ofill", 0, OFILL,
	"ofdel", OFDEL, 0,
	"-ofdel", 0, OFDEL,
	"cr0", CR0, CRDLY,
	"cr1", CR1, CRDLY,
	"cr2", CR2, CRDLY,
	"cr3", CR3, CRDLY,
	"tab0", TAB0, TABDLY,
	"tabs", TAB0, TABDLY,
	"tab1", TAB1, TABDLY,
	"tab2", TAB2, TABDLY,
	"tab3", TAB3, TABDLY,
	"-tabs", TAB3, TABDLY,
	"nl0", NL0, NLDLY,
	"nl1", NL1, NLDLY,
	"ff0", FF0, FFDLY,
	"ff1", FF1, FFDLY,
	"vt0", VT0, VTDLY,
	"vt1", VT1, VTDLY,
	"bs0", BS0, BSDLY,
	"bs1", BS1, BSDLY,
	"raw", 0, OPOST,
	"-raw", OPOST, 0,
	"cooked", OPOST, 0,
	"tty33", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tn300", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"ti700", CR2, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"vt05", NL1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tek", FF1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tty37", (FF1|VT1|CR2|TAB1|NL1), (NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	"sane", (OPOST|ONLCR), (OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
			NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	"hp", (OPOST|ONLCR), (OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
			NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	0,
};

/* modem control modes */
struct mds mmodes[] = {
	"crts", MRTS, 0,
	"-crts", 0, MRTS,
	0,
};

#ifdef SIGTSTP
struct ltchars ltb;
int lmb;

/* job control modes */
struct mds ltmodes[] = {
	"tostop", LTOSTOP, 0,
	"-tostop", 0, LTOSTOP,
	0,
};
#endif /* SIGTSTP */

#ifdef SIGWINCH
	struct winsize win;
#endif /* SIGWINCH */

char	*arg;
int	match;
char	*STTY="stty: ";
char	*USAGE="usage: stty [ -a | -g | modes ]\n";
int	pitt = 0;
int	hwfctl=0;
#if defined(hp9000s200) || defined(hp9000s500)
unsigned long	mb;
#else
mflag	mb;
#endif
struct termios pb;
struct termio cb;
struct termiox xb;

main(argc, argv)
char	*argv[];
{
	register i;
	int nomodem = 0;
	int sargc=argc;


#ifndef NONLS
	if (!setlocale(LC_ALL,""))
		fputs(_errlocale(),stderr);
#endif /* NONLS */
	if((ioctl(0, TCGETATTR, &pb) == -1) || (ioctl(0, TCGETA, &cb) == -1))
	{
		perror(STTY);
		exit(2);
	}

	if(ioctl(0, TCGETX, &xb) != -1)
			hwfctl = 1;

	if(ioctl(0, MCGETA, &mb) == -1) {
		nomodem++;
	}

#ifdef SIGWINCH
	if (ioctl(0, TIOCGWINSZ, &win) == -1)
	{
		/*
		 *  Upon failure, set to obviously incorrect values.
		 */
		win.ws_row = 0;
		win.ws_col = 0;
	}
#endif /* SIGWINCH */

#ifdef SIGTSTP
	if(ioctl(0, TIOCGLTC, &ltb) == -1) {
		perror(STTY);
		exit(2);
	}

	if(ioctl(0, TIOCLGET, &lmb) == -1) {
		perror(STTY);
		exit(2);
	}
#endif /* SIGTSTP */

	if (argc == 1 || (argc==2 && strcmp(argv[1],"--")==0)) {
		prmodes();
		exit(0);
	}
	if ((argc == 2) && (argv[1][0] == '-') && (argv[1][2] == '\0')
		|| (argc == 3 && strcmp(argv[2],"--")==0))
	switch(argv[1][1]) {
		case 'a':
			pramodes();
			exit(0);
		case 'g':
			prencode();
			exit(0);
		default:
			fprintf(stderr, "%s", USAGE);
			exit(2);
	}
	while(--argc > 0) {

		arg = *++argv;
		match = 0;
		/* ignore -- as first option (POSIX weirdness) */
		if (sargc-argc==1 && eq("--"));
		else if (eq("erase") && --argc)
			cb.c_cc[VERASE] = gct(*++argv);
		else if (eq("intr") && --argc)
			cb.c_cc[VINTR] = gct(*++argv);
		else if (eq("quit") && --argc)
			cb.c_cc[VQUIT] = gct(*++argv);
/***
 * At present VMIN maps to VEOF and _V2_VMIN
 * VTIME maps to VEOL and _V2_VTIME
 * when tty driver has been modified
 * to allow for seperating the POSIX variables.
 *
 ***/
		else if (eq("min") && --argc)
		{
			pb.c_cc[VMIN] = ntoi(*++argv);
			cb.c_cc[_V2_VMIN] = pb.c_cc[VMIN];
		}
		else if (eq("time") && --argc)
		{
			pb.c_cc[VTIME] = ntoi(*++argv);
			cb.c_cc[_V2_VTIME] = pb.c_cc[VTIME];
		}
		else if (eq("eof") && --argc)
		{
			cb.c_cc[VEOF] = gct(*++argv);
			pb.c_cc[VMIN] = cb.c_cc[VEOF];
		}
		else if (eq("eol") && --argc)
		{
			cb.c_cc[VEOL] = gct(*++argv);
			pb.c_cc[VTIME] = cb.c_cc[VEOL];
		}
		else if (eq("kill") && --argc)
			cb.c_cc[VKILL] = gct(*++argv);
		else if (eq("swtch") && --argc)
			cb.c_cc[VSWTCH] = gct(*++argv);
#ifdef	SIGTSTP
		else if (eq("susp") && --argc)
			ltb.t_suspc = gct(*++argv);
		else if (eq("dsusp") && --argc)
			ltb.t_dsuspc = gct(*++argv);
		else if (eq("start") && --argc)
			pb.c_cc[VSTART] = gct(*++argv);
		else if (eq("stop") && --argc)
			pb.c_cc[VSTOP] = gct(*++argv);
# ifdef	notdef
		else if (eq("rprnt") && --argc)
			ltb.t_rprntc = gct(*++argv);
		else if (eq("flush") && --argc)
			ltb.t_flushc = gct(*++argv);
		else if (eq("werase") && --argc)
			ltb.t_werasc = gct(*++argv);
		else if (eq("lnext") && --argc)
			ltb.t_lnextc = gct(*++argv);
# endif	/* notdef */
#endif	/* SIGTSTP */

#ifdef SIGWINCH
		else if (eq("size"))
		{
			ioctl(open("/dev/tty", 0), TIOCGWINSZ, &win);
                        printf("%d %d\n", win.ws_row, win.ws_col);
                }	
                else if ((eq("rows") || eq("cols") || eq("columns")) && --argc) 
		{
			if (eq("rows"))
                        	win.ws_row = ntoi(*++argv);
			else
                        	win.ws_col = ntoi(*++argv);

			if (ioctl(0, TIOCSWINSZ, &win) == -1)
			{
				perror(STTY);
				exit(2);
			}
		}
#endif /* SIGWINCH */

		else if (eq("ispeed") && --argc)
		{
			arg = *++argv;
			for(i=0; speeds[i].string; i++)
			{
				if(eq(speeds[i].string)) 
				{
					cfsetispeed(&pb, 
				(unsigned)speeds[i].speed);	
					arg = *(argv + 1);
					break;
				}
			}
		}
		else if (eq("ospeed") && --argc)
		{
			arg = *++argv;
			for(i=0; speeds[i].string; i++)
			{
				if(eq(speeds[i].string)) 
				{
					cfsetospeed(&pb, speeds[i].speed);	
					arg = *(argv + 1);
					break;
				}
			}
		}
		else if (eq("iexten")) {
			pb.c_lflag |= IEXTEN;
		}
		else if (eq("-iexten")) {
			pb.c_lflag &= ~IEXTEN;
		}
		else if (eq("ek")) {
			cb.c_cc[VERASE] = CERASE;
			cb.c_cc[VKILL] = CKILL;
		}
		else if (eq("rtsxoff")) {
			xb.x_hflag |= RTSXOFF;
		}
		else if (eq("-rtsxoff")) {
			xb.x_hflag &= ~RTSXOFF;
		}
		else if (eq("ctsxon")) {
		xb.x_hflag |= CTSXON;
		}
		else if (eq("-ctsxon")) {
		xb.x_hflag &= ~CTSXON;
		}
		else if (eq("line") && --argc)
			cb.c_line = atoi(*++argv);
		else if (eq("raw")) {
			cb.c_cc[_V2_VMIN] = 1;
			pb.c_cc[VMIN] = 1;
			cb.c_cc[_V2_VTIME] = 1;
			pb.c_cc[VTIME] = 1;
		}
		else if (eq("-raw") | eq("cooked")) {
			cb.c_cc[VEOF] = CEOF;
			pb.c_cc[VMIN] = CEOF;
			cb.c_cc[VEOL] = CNUL;
			pb.c_cc[VTIME] = CNUL;
		}
		else if(eq("sane")) {
			cb.c_cc[VERASE] = CERASE;
			cb.c_cc[VKILL] = CKILL;
			cb.c_cc[VQUIT] = CQUIT;
			cb.c_cc[VINTR] = CINTR;
			cb.c_cc[VEOF] = CEOF;
			pb.c_cc[VMIN] = CEOF;
			cb.c_cc[VEOL] = CNUL;
			pb.c_cc[VTIME] = CNUL;
			cb.c_cc[VSWTCH] = CNUL;
			xb.x_hflag &= ~RTSXOFF;
			xb.x_hflag &= ~CTSXON;
		}
		else if(eq("hp")) {
			cb.c_cc[VERASE] = CBS;
			cb.c_cc[VKILL] = CCAN;
			cb.c_cc[VQUIT] = CQUIT;
			cb.c_cc[VINTR] = CINTR;
			cb.c_cc[VEOF] = CEOF;
			pb.c_cc[VMIN] = CEOF;
			cb.c_cc[VEOL] = CNUL;
			pb.c_cc[VTIME] = CNUL;
			xb.x_hflag &= ~RTSXOFF;
			xb.x_hflag &= ~CTSXON;
		}

		for(i=0; speeds[i].string; i++)
			if(eq(speeds[i].string)) {
				cb.c_cflag &= ~CBAUD;
				cb.c_cflag |= speeds[i].speed&CBAUD;
				pb.c_cflag &= ~COUTBAUD;
				pb.c_cflag |= speeds[i].speed&COUTBAUD;
			}
		for(i=0; imodes[i].string; i++)
			if(eq(imodes[i].string)) {
				cb.c_iflag &= ~imodes[i].reset;
				cb.c_iflag |= imodes[i].set;
				pb.c_iflag &= ~imodes[i].reset;
				pb.c_iflag |= imodes[i].set;
			}
		for(i=0; omodes[i].string; i++)
			if(eq(omodes[i].string)) {
				cb.c_oflag &= ~omodes[i].reset;
				cb.c_oflag |= omodes[i].set;
				pb.c_oflag &= ~omodes[i].reset;
				pb.c_oflag |= omodes[i].set;
			}
		for(i=0; cmodes[i].string; i++)
			if(eq(cmodes[i].string)) {
				cb.c_cflag &= ~cmodes[i].reset;
				cb.c_cflag |= cmodes[i].set;
				pb.c_cflag &= ~cmodes[i].reset;
				pb.c_cflag |= cmodes[i].set;
			}
		for(i=0; lmodes[i].string; i++)
			if(eq(lmodes[i].string)) {
				cb.c_lflag &= ~lmodes[i].reset;
				cb.c_lflag |= lmodes[i].set;
				pb.c_lflag &= ~lmodes[i].reset;
				pb.c_lflag |= lmodes[i].set;
			}
		for(i=0; mmodes[i].string; i++)
			if(eq(mmodes[i].string)) {
				mb &= ~mmodes[i].reset;
				mb |= mmodes[i].set;
			}
#ifdef	SIGTSTP
		for(i=0; ltmodes[i].string; i++)
			if(eq(ltmodes[i].string)) {
				lmb &= ~ltmodes[i].reset;
				lmb |= ltmodes[i].set;
			}
#endif	/* SIGTSTP */
		if(!match)
			if(!encode(arg)) {
				fprintf(stderr, "unknown mode: %s\n", arg);
				exit(2);
			}
	}
	if(ioctl(0, TCSETAW, &cb) == -1)
	{
		perror(STTY);
		exit(2);
	}
	pb.c_iflag |= ((unsigned int) cb.c_iflag);
	pb.c_oflag |= ((unsigned int) cb.c_oflag);
	pb.c_cflag |= ((unsigned int) cb.c_cflag);
	pb.c_lflag |= ((unsigned int) cb.c_lflag);

	for (i = 0; i < 8; i++)
		pb.c_cc[i] = cb.c_cc[i];

	if(ioctl(0, TCSETATTR, &pb) == -1) 
	{
		perror(STTY);
		exit(2);
	}	
	if(hwfctl && ((ioctl(0, TCSETX, &xb) == -1))) {
	/*-----------------------------------------------
	| currently do nothing, since, knowledge of
	| hardware flow control support is inconclusive
	-----------------------------------------------*/
	}
	if(!nomodem) {
		if(ioctl(0, MCSETAW, &mb) == -1) {
			perror(STTY);
			exit(2);
		}
	}
#ifdef SIGTSTP
	if(ioctl(0, TIOCSLTC, &ltb) == -1) {
		perror(STTY);
		exit(2);
	}
	if(ioctl(0, TIOCLSET, &lmb) == -1) {
		perror(STTY);
		exit(2);
	}
#endif /* SIGTSTP */

	exit(0);
}

eq(string)
char *string;
{
	register i;

	if(!arg)
		return(0);
	i = 0;
loop:
	if(arg[i] != string[i])
		return(0);
	if(arg[i++] != '\0')
		goto loop;
	match++;
	return(1);
}

prmodes()
{
	register m;
	register p;
	register x;

	m = cb.c_cflag;
	x = xb.x_hflag;

	prspeed("speed ", m&CBAUD);
	if (m&PARENB)
		if (m&PARODD)
			printf("oddp ");
		else
			printf("evenp ");
	else
		printf("-parity ");
	if(((m&PARENB) && !(m&CS7)) || (!(m&PARENB) && !(m&CS8)))
		printf("cs%c ",'5'+(m&CSIZE)/CS6);
	if (m&CSTOPB)
		printf("cstopb ");
	if (m&HUPCL)
		printf("hupcl ");
	if (!(m&CREAD))
		printf("cread ");
	if (m&CLOCAL)
		printf("clocal ");
	if (m&LOBLK)
		printf("loblk ");
#ifdef SIGTSTP
	if(ltb.t_suspc != 0377)
	    pit(ltb.t_suspc, "susp", "; ");
	if(ltb.t_dsuspc != 0377)
	    pit(ltb.t_dsuspc, "dsusp", "");
# ifdef notdef
	printf("\n");
	if(ltb.t_rprntc != 0377)
	    pit(ltb.t_rprntc, "rprnt", "; ");
	if(ltb.t_flushc != 0377)
	    pit(ltb.t_flushc, "flush", "; ");
	if(ltb.t_werasc != 0377)
	    pit(ltb.t_werasc, "werase", "; ");
	if(ltb.t_lnextc != 0377)
	    pit(ltb.t_lnextc, "lnext", "");
# endif /* notdef */
#endif /* SIGTSTP */
	printf("\n");
	if(cb.c_line != 0)
		printf("line = %d; ", cb.c_line);
	if(cb.c_cc[VINTR] != CINTR)
		pit(cb.c_cc[VINTR], "intr", "; ");
	if(cb.c_cc[VQUIT] != CQUIT)
		pit(cb.c_cc[VQUIT], "quit", "; ");
	if(cb.c_cc[VERASE] != CERASE)
		pit(cb.c_cc[VERASE], "erase", "; ");
	if(cb.c_cc[VKILL] != CKILL)
		pit(cb.c_cc[VKILL], "kill", "; ");
	if(cb.c_cc[VEOF] != CEOF)
		pit(cb.c_cc[VEOF], "eof", "; ");
	if(cb.c_cc[VEOL] != CNUL)
		pit(cb.c_cc[VEOL], "eol", "\n");
	if(pb.c_cc[VMIN] != CEOF)
		printf("min = %d; ", pb.c_cc[VMIN]);
	if(pb.c_cc[VTIME] != CNUL)
		printf("time = %d; ", pb.c_cc[VTIME]);
	if(pb.c_cc[VSTART] != CNUL)
		pit(pb.c_cc[VSTART], "start", "; ");
	if(pb.c_cc[VSTOP] != CNUL)
		pit(pb.c_cc[VSTOP], "stop", "; ");
	if(cb.c_cc[VSWTCH] != CSWTCH)
		pit(cb.c_cc[VSWTCH], "swtch", "; ");
	if(pitt) printf("\n");
	m = cb.c_iflag;
	if (m&IGNBRK)
		printf("ignbrk ");
	else if (m&BRKINT)
		printf("brkint ");
	if (!(m&INPCK))
		printf("-inpck ");
	else if (m&IGNPAR)
		printf("ignpar ");
	if (m&PARMRK)
		printf("parmrk ");
	if (!(m&ISTRIP))
		printf("-istrip ");
	if (m&INLCR)
		printf("inlcr ");
	if (m&IGNCR)
		printf("igncr ");
	if (m&ICRNL)
		printf("icrnl ");
	if (m&IUCLC)
		printf("iuclc ");
	if (!(m&IXON))
		printf("-ixon ");
	else if (!(m&IXANY))
		printf("-ixany ");
	if (m&IXOFF)
		printf("ixoff ");
	if(x&RTSXOFF) printf("rtsxoff ");
	if(x&CTSXON) printf("ctsxon ");
#ifdef IENQAK
	if (m&IENQAK)
		printf("ienqak ");
#endif /* IENQAK */
	m = cb.c_oflag;
	if (!(m&OPOST))
		printf("-opost ");
	else {
	if (m&OLCUC)
		printf("olcuc ");
	if (m&ONLCR)
		printf("onlcr ");
	if (m&OCRNL)
		printf("ocrnl ");
	if (m&ONOCR)
		printf("onocr ");
	if (m&ONLRET)
		printf("onlret ");
	if (m&OFILL)
		if (m&OFDEL)
			printf("del-fill ");
		else
			printf("nul-fill ");
#ifdef SIGTSTP
	if(lmb&LTOSTOP)
		printf("tostop ");
#endif /* SIGTSTP */
	delay((m&CRDLY)/CR1, "cr");
	delay((m&NLDLY)/NL1, "nl");
	delay((m&TABDLY)/TAB1, "tab");
	delay((m&BSDLY)/BS1, "bs");
	delay((m&VTDLY)/VT1, "vt");
	delay((m&FFDLY)/FF1, "ff");
	}
	printf("\n");
	m = cb.c_lflag;
	p = pb.c_lflag;
	if (!(m&ISIG))
		printf("-isig ");
	if (!(m&ICANON))
		printf("-icanon ");
	printf("-iexten "+((p&IEXTEN)!=0));
	if (m&XCASE)
		printf("xcase ");
	printf("-echo "+((m&ECHO)!=0));
	printf("-echoe "+((m&ECHOE)!=0));
	printf("-echok "+((m&ECHOK)!=0));
	if (m&ECHONL)
		printf("echonl ");
	if (m&NOFLSH)
		printf("noflsh ");
	printf("\n");
}

pramodes()
{
	register m;
	register p;
	register x;

	m = cb.c_cflag;
	x = xb.x_hflag;

	prspeed("speed ", m&CBAUD);
	printf("line = %d; ", cb.c_line);
#ifdef SIGTSTP
	pit(ltb.t_suspc, "susp", "; ");
	pit(ltb.t_dsuspc, "dsusp", "");
# ifdef notdef
	printf("\n");
	pit(ltb.t_rprntc, "rprnt", "; ");
	pit(ltb.t_flushc, "flush", "; ");
	pit(ltb.t_werasc, "werase", "; ");
	pit(ltb.t_lnextc, "lnext", "");
# endif /* notdef */
#endif /* SIGTSTP */

	printf("\n");

#ifdef SIGWINCH
	printf("rows = %d; ", win.ws_row);
	printf("columns = %d", win.ws_col);
	printf("\n");	
#endif /* SIGWINCH */

	pit(cb.c_cc[VINTR], "intr", "; ");
	pit(cb.c_cc[VQUIT], "quit", "; ");
	pit(cb.c_cc[VERASE], "erase", "; ");
	pit(cb.c_cc[VKILL], "kill", "; ");
	pit(cb.c_cc[VSWTCH], "swtch", "\n");
	pit(cb.c_cc[VEOF], "eof", "; ");
	pit(cb.c_cc[VEOL], "eol", "; ");
	printf("min = %d; ", pb.c_cc[VMIN]);
	printf("time = %d; ", pb.c_cc[VTIME]);
	pit(pb.c_cc[VSTOP], "stop", "; ");
	pit(pb.c_cc[VSTART], "start", "\n");
	printf("-parenb "+((m&PARENB)!=0));
	printf("-parodd "+((m&PARODD)!=0));
	printf("cs%c ",'5'+(m&CSIZE)/CS6);
	printf("-cstopb "+((m&CSTOPB)!=0));
	printf("-hupcl "+((m&HUPCL)!=0));
	printf("-cread "+((m&CREAD)!=0));
	printf("-clocal "+((m&CLOCAL)!=0));
	printf("-loblk "+((m&LOBLK)!=0));
	printf("-crts "+((mb&MRTS)!=0));
	printf("\n");
	m = cb.c_iflag;
	printf("-ignbrk "+((m&IGNBRK)!=0));
	printf("-brkint "+((m&BRKINT)!=0));
	printf("-ignpar "+((m&IGNPAR)!=0));
	printf("-parmrk "+((m&PARMRK)!=0));
	printf("-inpck "+((m&INPCK)!=0));
	printf("-istrip "+((m&ISTRIP)!=0));
	printf("-inlcr "+((m&INLCR)!=0));
	printf("-igncr "+((m&IGNCR)!=0));
	printf("-icrnl "+((m&ICRNL)!=0));
	printf("-iuclc "+((m&IUCLC)!=0));
	printf("\n");
	printf("-ixon "+((m&IXON)!=0));
	printf("-ixany "+((m&IXANY)!=0));
	printf("-ixoff "+((m&IXOFF)!=0));
	printf("-rtsxoff "+((x&RTSXOFF)!=0));
	printf("-ctsxon "+((x&CTSXON)!=0));
#ifdef IENQAK
	printf("-ienqak "+((m&IENQAK)!=0));
#endif /* IENQAK */
	printf("\n");
	m = cb.c_lflag;
	p = pb.c_lflag;
	printf("-isig "+((m&ISIG)!=0));
	printf("-icanon "+((m&ICANON)!=0));
	printf("-iexten "+((p&IEXTEN)!=0));
	printf("-xcase "+((m&XCASE)!=0));
	printf("-echo "+((m&ECHO)!=0));
	printf("-echoe "+((m&ECHOE)!=0));
	printf("-echok "+((m&ECHOK)!=0));
	printf("-echonl "+((m&ECHONL)!=0));
	printf("-noflsh "+((m&NOFLSH)!=0));
	printf("\n");
	m = cb.c_oflag;
	printf("-opost "+((m&OPOST)!=0));
	printf("-olcuc "+((m&OLCUC)!=0));
	printf("-onlcr "+((m&ONLCR)!=0));
	printf("-ocrnl "+((m&OCRNL)!=0));
	printf("-onocr "+((m&ONOCR)!=0));
	printf("-onlret "+((m&ONLRET)!=0));
	printf("-ofill "+((m&OFILL)!=0));
	printf("-ofdel "+((m&OFDEL)!=0));
#ifdef SIGTSTP
	printf("-tostop "+((lmb&LTOSTOP)!=0));
#endif /* SIGTSTP */
	delay((m&CRDLY)/CR1, "cr");
	delay((m&NLDLY)/NL1, "nl");
	delay((m&TABDLY)/TAB1, "tab");
	delay((m&BSDLY)/BS1, "bs");
	delay((m&VTDLY)/VT1, "vt");
	delay((m&FFDLY)/FF1, "ff");
	printf("\n");
}
ntoi(cp)
char *cp;
{
	int	c;
	c = *cp;
	if(isdigit(c))
		return(atoi(cp));
	else if (c=='^') {
		c = *++cp;
		if (c == '?')
			c = 0177;
		else if (c == '-')
			c = 0377;
		else
			c &= 037;
	}
	return(c);
}

gct(cp)
register char *cp;
{
	register c;

	c = *cp++;
	if (c == '^') {
		c = *cp;
		if (c == '?')
			c = 0177;
		else if (c == '-')
			c = 0377;
		else
			c &= 037;
	}
	return(c);
}

pit(what, itsname, sep)
	unsigned char what;
	char *itsname, *sep;
{

	pitt++;
	printf("%s", itsname);
	if (what == 0377) {
		printf(" <undef>%s", sep);
		return;
	}
	printf(" = ");

#ifndef NONLS
	if (isprint(what)) {
		printf("%c%s", what, sep);
		return;
	}
#endif	/* NONLS */

	if (what & 0200) {
		printf("-");
		what &= ~ 0200;
	}
	if (what == 0177) {
		printf("DEL%s", sep);
		return;
	} else if (what < ' ') {
		printf("^");
		what += '@';
	}
	printf("%c%s", what, sep);
}

delay(m, s)
char *s;
{
	if(m)
		printf("%s%d ", s, m);
}

int	speed[] = {
	0,50,75,110,134,150,200,300,600,900,1200,1800,2400,3600,4800,
	7200,9600,19200,38400,57600,115200,230400,460800,0,0,0,0,0,0,0,0,0
};

prspeed(c, s)
char *c;
{
	int osp, isp;
	if( (osp = cfgetospeed(&pb)) != (isp = cfgetispeed(&pb)) && (isp != 0))
		printf("ispeed %d baud; ospeed %d baud; ", speed[isp], 
			speed[osp]);
	else
		printf("%s%d baud; ", c, speed[s]);
}

prencode()
{
#ifndef	SIGTSTP
	/*------------------------------------------------------------
	| rajs :  This case doesn't occur for versions later than 7.0
	|         Changes for hardware flow control have been made
	|         with this assumption
	------------------------------------------------------------*/
	printf("%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
#else	/* SIGTSTP */
	printf("%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
#endif	/* SIGTSTP */
/*
 * AT&T BELL characteristics from termio
 */
	cb.c_iflag,cb.c_oflag,cb.c_cflag,cb.c_lflag,cb.c_line,
	cb.c_cc[0], cb.c_cc[1],cb.c_cc[2],cb.c_cc[3],cb.c_cc[4],
	cb.c_cc[5], cb.c_cc[6],cb.c_cc[7],
/*
 * POSIX 1003.1 characteristics from termios
 */
	pb.c_iflag,pb.c_oflag,pb.c_cflag,pb.c_lflag,pb.c_cc[0],
	pb.c_cc[1],pb.c_cc[2],pb.c_cc[3],pb.c_cc[4],pb.c_cc[5],
	pb.c_cc[6],pb.c_cc[7], pb.c_cc[8],pb.c_cc[9], pb.c_cc[10],
	pb.c_cc[11], pb.c_cc[12],pb.c_cc[13], pb.c_cc[14],pb.c_cc[15],
	mb
#ifdef	SIGTSTP		/* true for versions later than 7.0 */
	,lmb,ltb.t_suspc,ltb.t_dsuspc,xb.x_hflag
#endif	/* SIGTSTP */
					);
#ifdef  SIGWINCH
       	  printf(":%x:%x", win.ws_row, win.ws_col);
#endif  /* SIGWINCH */
}

encode(arg)
char *arg;
{

/*	For job control, three more entries are needed.  The entries */
/*	are for tostop, susp and dsusp				     */

	int	bell[13];
	int	posx[20];
	int	modem, i, index;
	int     bellx[1];
#ifdef	SIGTSTP
	int sig[3];
#endif	/* SIGTSTP */

#ifdef  SIGWINCH
	int sigwin[2];
#endif  /* SIGWINCH */

#if defined(SIGTSTP) && defined(SIGWINCH)
	i = sscanf(arg, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
#else
#ifdef SIGTSTP
	i = sscanf(arg, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
#else 			/* never occurs in versions beyond 7.0 */	
#ifdef SIGWINCH
	i = sscanf(arg, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
#else			/* never occurs in versions beyond 7.0 */
	i = sscanf(arg, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
#endif /* SIGWINCH */
#endif /* SIGTSTP */
#endif	

/*
 * AT&T BELL characteristics from termio
 */
	&bell[0],&bell[1],&bell[2],&bell[3],&bell[4],
	&bell[5],&bell[6],&bell[7],&bell[8],&bell[9],&bell[10],&bell[11],&bell[12],

/*
 * POSIX 1003.1 characteristics from termios
 */
	&posx[0],&posx[1],&posx[2],&posx[3],&posx[4],
	&posx[5],&posx[6],&posx[7],&posx[8],&posx[9],
	&posx[10],&posx[11],&posx[12],&posx[13],&posx[14],
	&posx[15],&posx[16],&posx[17],&posx[18],&posx[19],
	&modem

#if defined(SIGTSTP) && defined(SIGWINCH)
	,&sig[0],&sig[1],&sig[2],&bellx[0],&sigwin[0],&sigwin[1]
#else	
#ifdef SIGTSTP
	,&sig[0],&sig[1],&sig[2],&bellx[0]
#else	
#ifdef SIGWINCH		/* never occurs in versions beyond 7.0 */
	,&sigwin[0],&sigwin[1]
#endif /* SIGWINCH */
#endif /* SIGTSTP */
#endif	
				);

/*
 * Versions later than 7.0 have SIGTSTP but may not have hardware flow control
 */
	if(i < 34 + 3) return(0);
	if(i < 38) bellx[0] = 0;

/*
 * AT&T BELL characteristics from termio
 */
	cb.c_iflag = (ushort) bell[0];
	cb.c_oflag = (ushort) bell[1];
	cb.c_cflag = (ushort) bell[2];
	cb.c_lflag = (ushort) bell[3];
	cb.c_line  = (char) bell[4];
	
	for(index=0; index<8; index++)
		cb.c_cc[index] = (char) bell[index+5];

/*
 * POSIX 1003.1 characteristics from termios
 */
	pb.c_iflag = (ushort) posx[0];
	pb.c_oflag = (ushort) posx[1];
	pb.c_cflag = (ushort) posx[2];
	pb.c_lflag = (ushort) posx[3];

	for(index=0; index<NCCS; index++)
		pb.c_cc[index] = (char) posx[index+4];
	mb = (unsigned int) modem;

#ifdef	SIGTSTP
		lmb = sig[0];
		ltb.t_suspc = (char) sig[1];
		ltb.t_dsuspc = (char) sig[2];
#endif	/* SIGTSTP */

	xb.x_hflag = (ushort) bellx[0];

#ifdef  SIGWINCH
		win.ws_row = (ushort) sigwin[0];
		win.ws_col = (ushort) sigwin[1];
#endif  /* SIGWINCH */

	return(1);
}


