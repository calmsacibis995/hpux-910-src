static char *HPUX_ID = "@(#) $Revision: 66.3 $";

/*
**  TSET -- set terminal modes
**
**	This program does sophisticated terminal initialization.
**	I recommend that you include it in your .start_up or .login
**	file to initialize whatever terminal you are on.
**
**	There are several features:
**
**	A special file or sequence (as controlled by the ttycap file)
**	is sent to the terminal.
**
**	Mode bits are set on a per-terminal_type basis (much better
**	than UNIX itself).  This allows special delays, automatic
**	tabs, etc.
**
**	Erase and Kill characters can be set to whatever you want.
**	Default is to change erase to control-H on a terminal which
**	can overstrike, and leave it alone on anything else.  Kill
**	is always left alone unless specifically requested.  These
**	characters can be represented as "^X" meaning control-X;
**	X is any character.
**
**	Terminals which are dialups or plugboard types can be aliased
**	to whatever type you may have in your home or office.  Thus,
**	if you know that when you dial up you will always be on a
**	TI 733, you can specify that fact to tset.  You can represent
**	a type as "?type".  This will ask you what type you want it
**	to be -- if you reply with just a newline, it will default
**	to the type given.
**
**	The current terminal type can be queried.
**
**	Usage:
**		tset [-] [-EC] [-eC] [-kC] [-s] [-h] [-r]
**			[-m [ident] [test baudrate] :type]
**			[-Q] [-I] [-S] [type]
**
**		In systems with environments, use:
**			eval `tset -s ...`
**		Actually, this doesn't work in old csh's.
**		Instead, use:
**			tset -s ... > tset.tmp
**			source tset.tmp
**			rm tset.tmp
**		or:
**			set noglob
**			set term=(`tset -S ....`)
**			setenv TERM $term[1]
**			setenv TERMCAP "$term[2]"
**			unset term
**			unset noglob
**
**	Positional Parameters:
**		type -- the terminal type to force.  If this is
**			specified, initialization is for this
**			terminal type.
**
**	Flags:
**		- -- report terminal type.  Whatever type is
**			decided on is reported.  If no other flags
**			are stated, the only affect is to write
**			the terminal type on the standard output.
**		-r -- report to user in addition to other flags.
**		-EC -- set the erase character to C on all terminals
**			except those which cannot backspace (e.g.,
**			a TTY 33).  C defaults to control-H.
**		-eC -- set the erase character to C on all terminals.
**			C defaults to control-H.  If neither -E or -e
**			are specified, the erase character is set to
**			control-H if the terminal can both backspace
**			and not overstrike (e.g., a CRT).  If the erase
**			character is NULL (zero byte), it will be reset
**			to '#' if nothing else is specified.
**		-kC -- set the kill character to C on all terminals.
**			Default for C is control-X.  If not specified,
**			the kill character is untouched; however, if
**			not specified and the kill character is NULL
**			(zero byte), the kill character is set to '@'.
**		-iC -- reserved for setable interrupt character.
**		-qC -- reserved for setable quit character.
**		-m -- map the system identified type to some user
**			specified type. The mapping can be baud rate
**			dependent. This replaces the old -d, -p flags.
**			(-d type  ->  -m dialup:type)
**			(-p type  ->  -m plug:type)
**			Syntax:	-m identifier [test baudrate] :type
**			where: ``identifier'' is whatever is found in
**			/etc/ttytype for this port, (abscence of an identifier
**			matches any identifier); ``test'' may be any combination
**			of  >  =  <  !  @; ``baudrate'' is as with stty(1);
**			``type'' is the actual terminal type to use if the
**			mapping condition is met. Multiple maps are scanned
**			in order and the first match prevails.
**		-n -- If the new tty driver from UCB is available, this flag
**			will activate the new options for erase and kill
**			processing. This will be different for printers
**			and crt's. For crts, if the baud rate is < 1200 then
**			erase and kill don't remove characters from the screen.
**		-h -- don't use getenv() to determine terminal type.
**			In V6 meant don't read htmp file or use env.
**		-s -- output setenv commands for TERM.  This can be
**			used with
**				`tset -s ...`
**			and is analagous to:
**				setenv TERM `tset - ...`
**		-S -- Similar to -s but outputs string suitable for
**			use in csh .login files as follows:
**				set noglob
**				set term=(`tset -S .....`)
**				setenv TERM $term[1]
**				unset term
**				unset noglob
**		-Q -- be quiet.  don't output 'Erase set to' etc.
**		-I -- don't do terminal initialization (is & if
**			strings).
**		-v -- On virtual terminal systems, don't set up a
**			virtual terminal.  Otherwise tset will tell
**			the operating system what kind of terminal you
**			are on (if it is a known terminal) and fix up
**			the output of -s to use virtual terminal sequences.
**
**	Files:
**		/etc/ttytype
**			contains a terminal id -> terminal type
**			mapping; used when any user mapping is specified,
**			or the environment doesn't have TERM set.
**		/usr/lib/terminfo/?/*
**			a terminal_type -> terminal_capabilities
**			mapping.
**
**	Return Codes:
**		-1 -- couldn't access terminfo file
**		1 -- bad terminal type, or standard output not tty.
**		0 -- ok.
**
**	Defined Constants:
**		DIALUP -- the type code for a dialup port.
**		PLUGBOARD -- the type code for a plugboard port.
**		ARPANET -- the type code for an arpanet port.
**		BACKSPACE -- control-H, the default for -e.
**		CTRL('X') -- control-X, the default for -k.
**		OLDERASE -- the system default erase character.
**		OLDKILL -- the system default kill character.
**		FILEDES -- the file descriptor to do the operation
**			on, nominally 1 or 2.
**		STDOUT -- the standard output file descriptor.
**		UIDMASK -- the bit pattern to mask with the getuid()
**			call to get just the user id.
**		GTTYN -- defines file containing generalized ttynames
**			and compiles code to look there.
**
**	Requires:
**		Routines to handle ttytype, and terminfo.
**
**	Compilation Flags:
**		OLDFLAGS -- must be defined to compile code for any of
**			the -d, -p, or -a flags.
**		OLDDIALUP -- accept the -d flag.
**		OLDPLUGBOARD -- accept the -p flag.
**		OLDARPANET -- accept the -a flag.
**		FULLLOGIN -- if defined, login sets the ttytype from
**			/etc/ttytype file.
**		GTTYN -- if set, compiles code to look at /etc/ttytype.
**		UCB_NTTY -- set to handle new tty driver modes.
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		Bad flag
**			An incorrect option was specified.
**		Too few args
**			more command line arguments are required.
**		Unexpected arg
**			wrong type of argument was encountered.
**		Cannot open ...
**			The specified file could not be opened.
**		Type ... unknown
**			An unknown terminal type was specified.
**		Erase set to ...
**			Telling that the erase character has been
**			set to the specified character.
**		Kill set to ...
**			Ditto for kill
**		Erase is ...    Kill is ...
**			Tells that the erase/kill characters were
**			wierd before, but they are being left as-is.
**		Not a terminal
**			Set if FILEDES is not a terminal.
**
**	Compilation Instructions:
**		cc -n -O -o tset tset.c -lcurses
**		chown bin tset
**		chmod 4755 tset
**
**	Author:
**		Eric Allman
**		Electronics Research Labs
**		U.C. Berkeley
**
**	History:
**		7/86 -- Bug fix to reset; instead of resetting to 5 bits,
**			it'll reset to 8 bits for spectrum, 7 bits for
**			other systems.
**		12/84-- Converted to work with AT&T 5.2 terminfo (compiled)
**			database.  TERMINFO environment variable now
**			meaningless; tgetstr() area pointer now ignored;
**			aliases useless.  Sigh.
**		3/82 -- Converted to Bell 3.0; dropped V6 flag totally,
**			including -u (don't update /etc/htmp) option.
**			Ask code modified to open tty unless explcit 
**			assignment to ffile 3  already done;  III does
**			not allow reads from stderr (2) in default case
**			DST
**		1/81 -- Added alias checking for mapping identifiers.
**		9/80 -- Added UCB_NTTY mods to setup the new tty driver.
**			Added the 'reset ...' invocation.
**		7/80 -- '-S' added. '-m' mapping added. TERMCAP string
**			cleaned up.
**		3/80 -- Changed to use tputs.  Prc & flush added.
**		10/79 -- '-s' option extended to handle TERMCAP
**			variable, set noglob, quote the entry,
**			and know about the Bourne shell.  Terminal
**			initialization moved to before any information
**			output so screen clears would not screw you.
**			'-Q' option added.
**		8/79 -- '-' option alone changed to only output
**			type.  '-s' option added.  'VERSION7'
**			changed to 'V6' for compatibility.
**		12/78 -- modified for eventual migration to VAX/UNIX,
**			so the '-' option is changed to output only
**			the terminal type to STDOUT instead of
**			FILEDES.  FULLLOGIN flag added.
**		9/78 -- '-' and '-p' options added (now fully
**			compatible with ttytype!), and spaces are
**			permitted between the -d and the type.
**		8/78 -- The sense of -h and -u were reversed, and the
**			-f flag is dropped -- same effect is available
**			by just stating the terminal type.
**		10/77 -- Written.
*/
# if defined hp9000s800 || defined hp9000s200 || defined hp9000s500
#  define USG
# endif hp9000s800 || hp9000s200 || hp9000s500
# ifdef USG
#  define index strchr
#  define rindex strrchr
#  define curerase mode.c_cc[VERASE]
#  define curkill mode.c_cc[VKILL]
#  define olderase oldmode.c_cc[VERASE]
#  define oldkill oldmode.c_cc[VKILL]
# else
#  define curerase mode.sg_erase
#  define curkill mode.sg_kill
#  define olderase oldmode.sg_erase
#  define oldkill oldmode.sg_kill
# endif

/*
# define	FULLLOGIN	1
/*/
# define	GTTYN		"/etc/ttytype"

# ifndef USG
#  include	<sgtty.h>
# else
#  include	<curses.h>
#  include	<termio.h>
#  include	<term.h>
# endif

# include	<fcntl.h>
# include	<stdio.h>
# include	<signal.h>

# define	YES		1
# define	NO		0
#undef CTRL
# define	CTRL(x)		(x & ~0140)
# define	BACKSPACE	(CTRL('H'))
# define	CHK(val, dft)	(val<=0 ? dft : val)
# define	isdigit(c)	(c >= '0' && c <= '9')
# define	isalnum(c)	(c > ' ' && !(index("<@=>!:|\177", c)) )
# define	OLDERASE	'#'
# define	OLDKILL		'@'

# define	FILEDES		2	/* do gtty/stty on this descriptor */
# define	STDOUT		1	/* output of -s/-S to this descriptor */

# define	UIDMASK		-1

# ifdef UCB_NTTY
# define	USAGE	"usage: tset [-] [-nrsIQS] [-eC] [-kC] [-m [ident][test speed]:type] [type]\n"
# else
# define	USAGE	"usage: tset [-] [-rsIQS] [-eC] [-kC] [-m [ident][test speed]:type] [type]\n"
# endif

# define	OLDFLAGS
# define	DIALUP		"dialup"
# define	OLDDIALUP	"sd"
# define	PLUGBOARD	"plugboard"
# define	OLDPLUGBOARD	"sp"
/***
# define	ARPANET		"arpanet"
# define	OLDARPANET	"sa"
/***/

# define	DEFTYPE		"unknown"


# ifdef GTTYN
# define	NOTTY		0
# else
# define	NOTTY		'x'
# endif

/*
 * Baud Rate Conditionals
 */
# define	ANY		0
# define	GT		1
# define	EQ		2
# define	LT		4
# define	GE		(GT|EQ)
# define	LE		(LT|EQ)
# define	NE		(GT|LT)
# define	ALL		(GT|EQ|LT)



# define	NMAP		10

struct	map {
	char *Ident;
	char Test;
	char Speed;
	char *Type;
} map[NMAP];

struct map *Map = map;

/* This should be available in an include file */
struct
{
	char	*string;
	int	speed;
	int	baudrate;
} speeds[] = {
	"0",	B0,	0,
	"50",	B50,	50,
	"75",	B75,	75,
	"110",	B110,	110,
	"134",	B134,	134,
	"134.5",B134,	134,
	"150",	B150,	150,
	"200",	B200,	200,
	"300",	B300,	300,
	"600",	B600,	600,
	"900",	B900,	900,
	"1200",	B1200,	1200,
	"1800",	B1800,	1800,
	"2400",	B2400,	2400,
	"3600",	B3600,	3600,
	"4800",	B4800,	4800,
	"9600",	B9600,	9600,
	"19200",B19200,	19200,
	"38400",B38400,	38400,
	"exta",	EXTA,	19200,
	"extb",	EXTB,	38400,
	0,
};

#ifdef CBVIRTTERM
struct vterm {
	char cap[2];
	char *value;
} vtab [] = {
	"al",	"\033\120",
	"cd",	"\033\114",
	"ce",	"\033\113",
	"cm",	"\033\107%r%.%.",
	"cl",	"\033\112",
	"dc",	"\033\115",
	"dl",	"\033\116",
	"ic",	"\033\117",
	"kl",	"\033\104",
	"kr",	"\033\103",
	"ku",	"\033\101",
	"kd",	"\033\102",
	"kh",	"\033\105",
	"nd",	"\033\103",
	"se",	"\033\142\004",
	"so",	"\033\141\004",
	"ue",	"\033\142\001",
	"up",	"\033\101",
	"us",	"\033\141\001",
	"\0\0", NULL,
};

int VirTermNo = -2;
# endif CBVIRTTERM

char	Erase_char;		/* new erase character */
char	Kill_char;		/* new kill character */
char	Specialerase;		/* set => Erase_char only on terminals with backspace */

# ifdef	GTTYN
char	*Ttyid = NOTTY;		/* terminal identifier */
# else
char	Ttyid = NOTTY;		/* terminal identifier */
# endif
char	*TtyType;		/* type of terminal */
char	*DefType;		/* default type if none other computed */
char	*NewType;		/* mapping identifier based on old flags */
int	Mapped;			/* mapping has been specified */
int	Dash_h;			/* don't read htmp */
int	DoSetenv;		/* output setenv commands */
int	BeQuiet;		/* be quiet */
int	NoInit;			/* don't output initialization string */
int	IsReset;		/* invoked as reset */
int	Report;			/* report current type */
int	Ureport;		/* report to user */
int	RepOnly;		/* report only */
int	CmndLine;		/* output full command lines (-s option) */
int	Ask;			/* ask user for termtype */
int	DoVirtTerm = YES;	/* Set up a virtual terminal */
int	New = NO;		/* use new tty discipline */
int	HasAM;			/* True if terminal has automatic margins */
int	PadBaud;		/* Min rate of padding needed */

struct delay
{
	int	d_delay;
	int	d_bits;
};

# include	"tset.delays.h"

# ifndef USG
struct sgttyb	mode;
struct sgttyb	oldmode;
extern short	ospeed;
# else
struct termio	mode;
struct termio	oldmode;
short		ospeed;
# endif
# ifdef CBVIRTTERM
struct termcb block = {0, 2, 0, 0, 0, 20};
# endif CBVIRTTERM


main(argc, argv)
int	argc;
char	*argv[];
{
	char		termbuf[32];
	auto char	*bufp;
	register char	*p;
	char		*command;
	register int	i;
	int		j;
	int		Break;
	int		Not;
	char		*nextarg();
	char		*mapped();
	extern char	*rindex();
	extern char	*getenv();
# ifdef GTTYN
	char		*stypeof();
	extern char	*ttyname();
	extern char	*tgetstr();
# endif
	char		bs_char;
	int		csh;
	int		settle = NO;
	int		setmode();
	extern		prc();
# ifdef UCB_NTTY
	int		lmode;
	int		ldisc;

	ioctl(FILEDES, TIOCLGET, &lmode);
	ioctl(FILEDES, TIOCGETD, &ldisc);
# endif

# ifndef USG
	if (gtty(FILEDES, &mode) < 0)
# else
	if (ioctl(FILEDES, TCGETA, &mode) < 0)
# endif
	{
		prs("Not a terminal\n");
		flush();
		exit(1);
	}
	bmove(&mode, &oldmode, sizeof mode);
# ifndef USG
	ospeed = mode.sg_ospeed & 017;
# else
	ospeed = mode.c_cflag & CBAUD;
# endif
	signal(SIGINT, setmode);
	signal(SIGQUIT, setmode);
	signal(SIGTERM, setmode);

	if (command = rindex(argv[0], '/'))
		command++;
	else
		command = argv[0];
	if (sequal(command, "reset") )
	{
	/*
	 * reset the teletype mode bits to a sensible state.
	 * Copied from the program by Kurt Shoens & Mark Horton.
	 * Very useful after crapping out in raw.
	 */
#  ifdef TIOCGETC
		struct tchars tbuf;
#  endif TIOCGETC
#  ifdef UCB_NTTY
		struct ltchars ltc;

		if (ldisc == NTTYDISC)
		{
			ioctl(FILEDES, TIOCGLTC, &ltc);
			ltc.t_suspc = CHK(ltc.t_suspc, CTRL('Z'));
			ltc.t_dsuspc = CHK(ltc.t_dsuspc, CTRL('Y'));
			ltc.t_rprntc = CHK(ltc.t_rprntc, CTRL('R'));
			ltc.t_flushc = CHK(ltc.t_flushc, CTRL('O'));
			ltc.t_werasc = CHK(ltc.t_werasc, CTRL('W'));
			ltc.t_lnextc = CHK(ltc.t_lnextc, CTRL('V'));
			ioctl(FILEDES, TIOCSLTC, &ltc);
		}
#  endif UCB_NTTY
#  ifndef USG
#   ifdef TIOCGETC
		ioctl(FILEDES, TIOCGETC, &tbuf);
		tbuf.t_intrc = CHK(tbuf.t_intrc, CTRL('?'));
		tbuf.t_quitc = CHK(tbuf.t_quitc, CTRL('\\'));
		tbuf.t_startc = CHK(tbuf.t_startc, CTRL('Q'));
		tbuf.t_stopc = CHK(tbuf.t_stopc, CTRL('S'));
		tbuf.t_eofc = CHK(tbuf.t_eofc, CTRL('D'));
		/* brkc is left alone */
		ioctl(FILEDES, TIOCSETC, &tbuf);
#   endif TIOCGETC
		mode.sg_flags &= ~(RAW
#   ifdef CBREAK
					|CBREAK
#   endif CBREAK
						|VTDELAY|ALLDELAY);
		mode.sg_flags |= XTABS|ECHO|CRMOD|ANYP;
		curerase = CHK(curerase, OLDERASE);
		curkill = CHK(curkill, OLDKILL);
#  else USG
		ioctl(FILEDES, TCGETA, &mode);
		curerase = CHK(curerase, OLDERASE);
		curkill = CHK(curkill, OLDKILL);
		mode.c_cc[VINTR] = CHK(mode.c_cc[VINTR], CTRL('?'));
		mode.c_cc[VQUIT] = CHK(mode.c_cc[VQUIT], CTRL('\\'));
		mode.c_cc[VEOF] = CHK(mode.c_cc[VEOF], CTRL('D'));

		mode.c_iflag &= ~(IGNBRK|PARMRK|INPCK|INLCR|IGNCR|IUCLC|IXOFF);
		mode.c_iflag |= (BRKINT|ISTRIP|ICRNL|IXON);
		mode.c_oflag &= ~(OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
				NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
		mode.c_oflag |= (OPOST|ONLCR);
		mode.c_cflag &= ~(CSIZE|PARODD|CLOCAL);
#   ifndef hp9000s800
		mode.c_cflag |= (CS8|CREAD);
#   else hp9000s800
		mode.c_cflag |= (CS8|CSTOPB|CREAD);
#   endif hp9000s800
		mode.c_lflag &= ~(XCASE|ECHONL|NOFLSH);
		mode.c_lflag |= (ISIG|ICANON|ECHO|ECHOK);
		ioctl(FILEDES, TCSETAW, &mode);
#  endif USG
		BeQuiet = YES;
		IsReset = YES;
	}
	else if (argc == 2 && sequal(argv[1], "-"))
	{
		RepOnly = YES;
	}
	argc--;

	/* scan argument list and collect flags */
	while (--argc >= 0)
	{
		p = *++argv;
		if (*p == '-')
		{
			if (*++p == NULL)
				Report = YES; /* report current terminal type */
			else while (*p) switch (*p++)
			{

# ifdef UCB_NTTY
			  case 'n':
				ldisc = NTTYDISC;
				if (ioctl(FILEDES, TIOCSETD, &ldisc)<0)
					fatal("ioctl ", "new");
				continue;
# endif

			  case 'r':	/* report to user */
				Ureport = YES;
				continue;

			  case 'E':	/* special erase: operate on all but TTY33 */
				Specialerase = YES;
				/* explicit fall-through to -e case */

			  case 'e':	/* erase character */
				if (*p == NULL)
					Erase_char = -1;
				else
				{
					if (*p == '^' && p[1] != NULL)
						Erase_char = CTRL(*++p);
					else
						Erase_char = *p;
					p++;
				}
				continue;

			  case 'k':	/* kill character */
				if (*p == NULL)
					Kill_char = CTRL('X');
				else
				{
					if (*p == '^' && p[1] != NULL)
						Kill_char = CTRL(*++p);
					else
						Kill_char = *p;
					p++;
				}
				continue;

# ifdef OLDFLAGS
# ifdef	OLDDIALUP
			  case 'd':	/* dialup type */
				NewType = DIALUP;
				goto mapold;
# endif

# ifdef OLDPLUGBOARD
			  case 'p':	/* plugboard type */
				NewType = PLUGBOARD;
				goto mapold;
# endif

# ifdef OLDARPANET
			  case 'a':	/* arpanet type */
				Newtype = ARPANET;
				goto mapold;
# endif

mapold:				Map->Ident = NewType;
				Map->Test = ALL;
				if (*p == NULL)
				{
					p = nextarg(argc--, argv++);
				}
				Map->Type = p;
				Map++;
				Mapped = YES;
				p = "";
				continue;
# endif

			  case 'm':	/* map identifier to type */
				/* This code is very loose. Almost no
				** syntax checking is done!! However,
				** illegal syntax will only produce
				** weird results.
				*/
				if (*p == NULL)
				{
					p = nextarg(argc--, argv++);
				}
				if (isalnum(*p))
				{
					Map->Ident = p;	/* identifier */
					while (isalnum(*p)) p++;
				}
				else
					Map->Ident = "";
				Break = NO;
				Not = NO;
				while (!Break) switch (*p)
				{
					case NULL:
						p = nextarg(argc--, argv++);
						continue;

					case ':':	/* mapped type */
						*p++ = NULL;
						Break = YES;
						continue;

					case '>':	/* conditional */
						Map->Test |= GT;
						*p++ = NULL;
						continue;

					case '<':	/* conditional */
						Map->Test |= LT;
						*p++ = NULL;
						continue;

					case '=':	/* conditional */
					case '@':
						Map->Test |= EQ;
						*p++ = NULL;
						continue;
					
					case '!':	/* invert conditions */
						Not = ~Not;
						*p++ = NULL;
						continue;

					case 'B':	/* Baud rate */
						p++;
						/* intentional fallthru */
					default:
						if (isdigit(*p) || *p == 'e')
						{
							Map->Speed = baudrate(p);
							while (isalnum(*p) || *p == '.')
								p++;
						}
						else
							Break = YES;
						continue;
				}
				if (Not)	/* invert sense of test */
				{
					Map->Test = (~(Map->Test))&ALL;
				}
				if (*p == NULL)
				{
					p = nextarg(argc--, argv++);
				}
				Map->Type = p;
				p = "";
				Map++;
				Mapped = YES;
				continue;

			  case 'h':	/* don't get type from env */
				Dash_h = YES;
				continue;

			  case 'u':    /* ignore this one */
				continue;

			  case 's':	/* output setenv commands */
				DoSetenv = YES;
				CmndLine = YES;
				continue;

			  case 'S':	/* output setenv strings */
				DoSetenv = YES;
				CmndLine = NO;
				continue;

			  case 'Q':	/* be quiet */
				BeQuiet = YES;
				continue;

			  case 'I':	/* no initialization */
				NoInit = YES;
				continue;

			  case 'A':	/* Ask user */
				Ask = YES;
				continue;
			
#ifdef CBVIRTTERM
			  case 'v':	/* no virtual terminal */
				DoVirtTerm = NO;
				continue;
#endif CBVIRTTERM

			  default:
				*p-- = NULL;
				fatal("Bad flag -", p);
			}
		}
		else
		{
			/* terminal type */
			DefType = p;
		}
	}

	if (DefType)
	{
		if (Mapped)
		{
			Map->Ident = "";	/* means "map any type" */
			Map->Test = ALL;	/* at all baud rates */
			Map->Type = DefType;	/* to the default type */
		}
		else
			TtyType = DefType;
	}

	/* get current idea of terminal type from environment */
	if (!Dash_h && !Mapped && TtyType == 0)
		TtyType = getenv("TERM");

	/* determine terminal id if needed */
	if (!RepOnly && Ttyid == NOTTY && (TtyType == 0 || !Dash_h))
		Ttyid = ttyname(FILEDES);

# ifdef GTTYN
	/* If still undefined, look at /etc/ttytype */
	if (TtyType == 0)
	{
		TtyType = stypeof(Ttyid);
	}
# endif

	/* If still undefined, use DEFTYPE */
	if (TtyType == 0)
	{
		TtyType = DEFTYPE;
	}

	/* check for dialup or other mapping */
	if (Mapped)
		TtyType = mapped(TtyType);

	/* TtyType now contains a pointer to the type of the terminal */
	/* If the first character is '?', ask the user */
	if (TtyType[0] == '?')
	{
		Ask = YES;
		TtyType++;
		if (TtyType[0] == '\0')
			TtyType = DEFTYPE;
	}
	if (Ask)
	{
		int asktty;

		/* open the terminal and write to value of TtyType */
		asktty = open("/dev/tty",O_RDWR);
		if (asktty == -1) {
		    prs("Cannot open /dev/tty read/write to ask.  Quitting.\n");
		    flush();
		    exit(1);
		}

		write(asktty,"TERM = (",8);
		write(asktty,TtyType,strlen(TtyType));
		write(asktty,") ",2);

		/* read the terminal.  If not empty, set type */
		i = read(asktty, termbuf, sizeof termbuf - 1);
		if (i >= 0)
		{
			if (termbuf[i - 1] == '\n')
				i--;
			termbuf[i] = '\0';
			if (termbuf[0] != '\0')
				TtyType = termbuf;
		}
		close(asktty);
	}

	/* get terminal capabilities */
	{
		int errret;

		setupterm(TtyType, 1, &errret);
		switch (errret)
		{
	  	case -1:
			prs("Cannot find terminfo directory\n");
			flush();
			exit(-1);
	
	  	case 0:
			prs("Type ");
			prs(TtyType);
			prs(" unknown\n");
			flush();
			if (DoSetenv)
			{
				TtyType = DEFTYPE;
				setupterm(TtyType, 1, &errret);
			}
			else
				exit(1);
		}
	}

	if (!RepOnly)
	{
		/* determine erase and kill characters */
		if (Specialerase && !tgetflag("bs"))
			Erase_char = 0;
		p = tgetstr("kb", (char *)0);
		if (p == NULL || p[1] != '\0')
			p = tgetstr("bc", (char *)0);
		if (p != NULL && p[1] == '\0')
			bs_char = p[0];
		else if (tgetflag("bs"))
			bs_char = BACKSPACE;
		else
			bs_char = 0;
		if (Erase_char == 0 && !tgetflag("os") && curerase == OLDERASE)
		{
			if (tgetflag("bs") || bs_char != 0)
				Erase_char = -1;
		}
		if (Erase_char < 0)
			Erase_char = (bs_char != 0) ? bs_char : BACKSPACE;

		if (curerase == 0)
			curerase = OLDERASE;
		if (Erase_char != 0)
			curerase = Erase_char;

		if (curkill == 0)
			curkill = OLDKILL;
		if (Kill_char != 0)
			curkill = Kill_char;

		/* set modes */
		PadBaud = padding_baud_rate;
		for (i=0; speeds[i].string; i++)
			if (speeds[i].baudrate == padding_baud_rate) {
				PadBaud = speeds[i].speed;
				break;
			}
# ifndef USG
		setdelay("dC", CRdelay, CRbits, &mode.sg_flags);
		setdelay("dN", NLdelay, NLbits, &mode.sg_flags);
		setdelay("dB", BSdelay, BSbits, &mode.sg_flags);
		setdelay("dF", FFdelay, FFbits, &mode.sg_flags);
		setdelay("dT", TBdelay, TBbits, &mode.sg_flags);
		if (tgetflag("UC") || (command[0] & 0140) == 0100)
			mode.sg_flags |= LCASE;
		else if (tgetflag("LC"))
			mode.sg_flags &= ~LCASE;
		mode.sg_flags &= ~(EVENP | ODDP | RAW);
# ifdef CBREAK
		mode.sg_flags &= ~CBREAK;
# endif
		if (tgetflag("EP"))
			mode.sg_flags |= EVENP;
		if (tgetflag("OP"))
			mode.sg_flags |= ODDP;
		if ((mode.sg_flags & (EVENP | ODDP)) == 0)
			mode.sg_flags |= EVENP | ODDP;
		mode.sg_flags |= CRMOD | ECHO | XTABS;
		if (tgetflag("NL"))	/* new line, not line feed */
			mode.sg_flags &= ~CRMOD;
		if (tgetflag("HD"))	/* half duplex */
			mode.sg_flags &= ~ECHO;
		if (tgetflag("pt"))	/* print tabs */
			mode.sg_flags &= ~XTABS;
# else
		setdelay("dC", CRdelay, CRbits, &mode.c_oflag);
		setdelay("dN", NLdelay, NLbits, &mode.c_oflag);
		setdelay("dB", BSdelay, BSbits, &mode.c_oflag);
		setdelay("dF", FFdelay, FFbits, &mode.c_oflag);
		setdelay("dT", TBdelay, TBbits, &mode.c_oflag);
		setdelay("dV", VTdelay, VTbits, &mode.c_oflag);

		if (tgetflag("UC") || (command[0] & 0140) == 0100) {
			mode.c_iflag |= IUCLC;
			mode.c_oflag |= OLCUC;
		}
		else if (tgetflag("LC")) {
			mode.c_iflag &= ~IUCLC;
			mode.c_oflag &= ~OLCUC;
		}
		mode.c_iflag &= ~(PARMRK|INPCK);
		mode.c_lflag |= ICANON;
		if (tgetflag("EP")) {
			mode.c_cflag |= PARENB;
			mode.c_cflag &= ~PARODD;
		}
		if (tgetflag("OP")) {
			mode.c_cflag |= PARENB;
			mode.c_cflag |= PARODD;
		}

		mode.c_oflag |= ONLCR;
		mode.c_iflag |= ICRNL;
		mode.c_lflag |= ECHO;
		mode.c_oflag |= TAB3;
		if (tgetflag("NL")) {	/* new line, not line feed */
			mode.c_oflag &= ~ONLCR;
			mode.c_iflag &= ~ICRNL;
		}
		if (tgetflag("HD"))	/* half duplex */
			mode.c_lflag &= ~ECHO;
		if (tgetflag("pt"))	/* print tabs */
			mode.c_oflag &= ~TAB3;
		
		mode.c_lflag |= (ECHOE|ECHOK);
# endif
# ifdef CBVIRTTERM
		HasAM = tgetflag("am");
# endif CBVIRTTERM
# ifdef UCB_NTTY
		if (ldisc == NTTYDISC)
		{
			lmode |= LCTLECH;	/* display ctrl chars */
			if (tgetflag("hc"))
			{	/** set printer modes **/
				lmode &= ~(LCRTBS|LCRTERA|LCRTKIL);
				lmode |= LPRTERA;
			}
			else
			{	/** set crt modes **/
				if (!tgetflag("os"))
				{
					lmode &= ~LPRTERA;
					lmode |= LCRTBS;
					if (mode.sg_ospeed >= B1200)
						lmode |= LCRTERA|LCRTKIL;
				}
			}
		}
		ioctl(FILEDES, TIOCLSET, &lmode);
# endif

		/* output startup string */
		if (!NoInit)
		{
# ifndef USG
			if (oldmode.sg_flags&(XTABS|CRMOD))
			{
				oldmode.sg_flags &= ~(XTABS|CRMOD);
				setmode(-1);
			}
# else
			if (oldmode.c_oflag&(TAB3|ONLCR|OCRNL|ONLRET))
			{
				oldmode.c_oflag &= (TAB3|ONLCR|OCRNL|ONLRET);
				setmode(-1);
			}
# endif
# ifdef CBVIRTTERM
			block.st_termt = 0;
			ioctl(FILEDES, LDSETT, &block);
# endif CBVIRTTERM
			if (IsReset) { /* reset */
			    if (reset_1string) {
				tputs(reset_1string, 0, prc);
				flush();
				settle = YES;
			    }
			    if (reset_2string) {
				tputs(reset_2string, 0, prc);
				flush();
				settle = YES;
			    }
			    if (settabs()) {
				flush();
				settle = YES;
			    }
			    if (reset_file)
				cat(reset_file);
			    if (reset_3string) {
				tputs(reset_3string, 0, prc);
				flush();
				settle = YES;
			    }
			} else { /* tset */
			    if (init_1string) {
				tputs(init_1string, 0, prc);
				flush();
				settle = YES;
			    }
			    if (init_2string) {
				tputs(init_2string, 0, prc);
				flush();
				settle = YES;
			    }
			    if (settabs()) {
				flush();
				settle = YES;
			    }
			    if (init_file)
				cat(init_file);
			    if (init_prog)
				system(init_prog);
			    if (init_3string) {
				tputs(init_3string, 0, prc);
				flush();
				settle = YES;
			    }
			}
			if (settle)
			{
				prc('\r');
				flush();
				sleep(1);	/* let terminal settle down */
			}
		}

# ifdef CBVIRTTERM
		if (DoVirtTerm) {
			j = tgetnum("vt");
			VirTermNo = -1;
			for (i=0; vt_map[i].stdnum; i++)
				if (vt_map[i].stdnum == j)
					VirTermNo = vt_map[i].localnum;
		} else
			VirTermNo = -1;
# endif CBVIRTTERM

		setmode(0);	/* set new modes, if they've changed */

		/* set up environment for the shell we are using */
		/* (this code is rather heuristic, checking for $SHELL */
		/* ending in the 3 characters "csh") */
		csh = NO;
		if (DoSetenv)
		{
			char *sh;

			if ((sh = getenv("SHELL")) && (i = strlen(sh)) >= 3)
			{
				if ((csh = sequal(&sh[i-3], "csh")) && CmndLine)
					write(STDOUT, "set noglob;\n", 12);
			}
			if (!csh)
				/* running Bourne shell */
				write(STDOUT, "export TERM;\n", 13);
		}
	}

	/* report type if appropriate */
	if (DoSetenv || Report || Ureport)
	{
		if (DoSetenv)
		{
			if (csh)
			{
				if (CmndLine)
					write(STDOUT, "setenv TERM ", 12);
				write(STDOUT, TtyType, strlen(TtyType));
				write(STDOUT, " ", 1);
				if (CmndLine) {
					write(STDOUT, ";\n", 2);
					write(STDOUT, "unset noglob;\n", 14);
				}
			}
			else
			{
				write(STDOUT, "TERM=", 5);
				write(STDOUT, TtyType, strlen(TtyType));
				write(STDOUT, ";\n", 2);
			}
		}
		else if (Report)
		{
			write(STDOUT, TtyType, strlen(TtyType));
			write(STDOUT, "\n", 1);
		}
		if (Ureport)
		{
			prs("Terminal type is ");
			prs(TtyType);
			prs("\n");
			flush();
		}
	}

	if (RepOnly)
		exit(0);

	/* tell about changing erase and kill characters */
	reportek("Erase", curerase, olderase, OLDERASE);
	reportek("Kill", curkill, oldkill, OLDKILL);

	exit(0);
}

/*
 * Set the hardware tabs on the terminal, using the clear_all_tabs,
 * set_tab and column_address terminfo capabilities.
 * This is done before if and is, so they can patch in case we blow this.
 */
settabs()
{
	char caps[100];
	char *capsp = caps;
	char *tg_out, *tgoto();
	int c;

	if (clear_all_tabs && set_tab) {
		prc('\r');	/* force to be at left margin */
		tputs(clear_all_tabs, 0, prc);
	}
	if (set_tab) {
		for (c=8; c<columns; c += 8) {
			/* get to that column. */
			tg_out = "OOPS";	/* also returned by tgoto */
			if (column_address)
				tg_out = tgoto(column_address, 0, c);
			if (*tg_out == 'O' && cursor_address)
				tg_out = tgoto(cursor_address, c, lines-1);
			if (*tg_out != 'O')
				tputs(tg_out, 1, prc);
			else {
				prc(' '); prc(' '); prc(' '); prc(' ');
				prc(' '); prc(' '); prc(' '); prc(' ');
			}
			/* set the tab */
			tputs(set_tab, 0, prc);
		}
		prc('\r');
		return 1;
	}
	return 0;
}

setmode(flag)
int	flag;
/* flag serves several purposes:
 *	if called as the result of a signal, flag will be > 0.
 *	if called from terminal init, flag == -1 means reset "oldmode".
 *	called with flag == 0 at end of normal mode processing.
 */
{
# ifndef USG
	struct sgttyb *ttymode;
# else
	struct termio *ttymode;
# endif

	if (flag < 0)	/* unconditionally reset oldmode (called from init) */
		ttymode = &oldmode;
	else if (!bequal(&mode, &oldmode, sizeof mode))
		ttymode = &mode;
	else		/* don't need it */
# ifndef USG
	ttymode = (struct sgttyb *)0;
# else
	ttymode = (struct termio *)0;
# endif
	
	if (ttymode)
	{
# ifdef USG
		ioctl(FILEDES, TCSETAW, ttymode);
# else
		ioctl(FILEDES, TIOCSETN, ttymode);     /* don't flush */
# endif
	}
# ifdef CBVIRTTERM
	if (VirTermNo != -2) {
		int r1, r2;
		extern int errno;

		r1 = ioctl(FILEDES, LDGETT, &block);
		block.st_flgs |= TM_SET;
		block.st_termt = VirTermNo;
		if (block.st_termt < 0)
			block.st_termt = 0;
		if (!HasAM)
			block.st_flgs |= TM_ANL;
		else
			block.st_flgs &= ~TM_ANL;
		r2 = ioctl(FILEDES, LDSETT, &block);
	}
# endif

	if (flag > 0)	/* trapped signal */
		exit(1);
}

reportek(name, new, old, def)
char	*name;
char	old;
char	new;
char	def;
{
	register char	o;
	register char	n;
	register char	*p;
	char		*bufp;

	if (BeQuiet)
		return;
	o = old;
	n = new;

	if (o == n && n == def)
		return;
	prs(name);
	if (o == n)
		prs(" is ");
	else
		prs(" set to ");
	if ((bufp = tgetstr("kb", (char *)0)) != (char *)0 && n == *bufp && bufp[1] == NULL)
		prs("Backspace\n");
	else if (n == 0177)
		prs("Delete\n");
	else
	{
		if (n < 040)
		{
			prs("Ctrl-");
			n ^= 0100;
		}
		p = "x\n";
		p[0] = n;
		prs(p);
	}
	flush();
}




setdelay(cap, dtab, bits, flags)
char		*cap;
struct delay	dtab[];
int		bits;
#ifndef USG
short		*flags;
#else
unsigned short	*flags;
#endif USG
{
	register int	i;
	register struct delay	*p;
	extern short	ospeed;

	/* see if this capability exists at all */
	i = tgetnum(cap);
	if (i < 0)
		i = 0;
	/* No padding at speeds below PadBaud */
	if (PadBaud > ospeed)
		i = 0;

	/* clear out the bits, replace with new ones */
	*flags &= ~bits;

	/* scan dtab for first entry with adequate delay */
	for (p = dtab; p->d_delay >= 0; p++)
	{
		if (p->d_delay >= i)
		{
			p++;
			break;
		}
	}

	/* use last entry if none will do */
	*flags |= (--p)->d_bits;
}


prs(s)
char	*s;
{
	while (*s != '\0')
		prc(*s++);
}


char	OutBuf[256];
int	OutPtr;

prc(c)
char	c;
{
	OutBuf[OutPtr++] = c;
	if (OutPtr >= sizeof OutBuf)
		flush();
}

flush()
{
	if (OutPtr > 0)
		write(2, OutBuf, OutPtr);
	OutPtr = 0;
}


cat(file)
char	*file;
{
	register int	fd;
	register int	i;
	char		buf[BUFSIZ];

	fd = open(file, 0);
	if (fd < 0)
	{
		prs("Cannot open ");
		prs(file);
		prs("\n");
		flush();
		return;
	}

	while ((i = read(fd, buf, BUFSIZ)) > 0)
		write(FILEDES, buf, i);

	close(fd);
}



bmove(from, to, length)
char	*from;
char	*to;
int	length;
{
	register char	*p, *q;
	register int	i;

	i = length;
	p = from;
	q = to;

	while (i-- > 0)
		*q++ = *p++;
}



bequal(a, b, len)	/* must be same thru len chars */
char	*a;
char	*b;
int	len;
{
	register char	*p, *q;
	register int	i;

	i = len;
	p = a;
	q = b;

	while ((*p == *q) && --i > 0)
	{
		p++; q++;
	}
	return ((*p == *q) && i >= 0);
}

sequal(a, b)	/* must be same thru NULL */
char	*a;
char	*b;
{
	register char *p = a, *q = b;

	while (*p && *q && (*p == *q))
	{
		p++; q++;
	}
	return (*p == *q);
}

# ifdef GTTYN
char *
stypeof(ttyid)
char	*ttyid;
{
	static char	typebuf[BUFSIZ];
	register char	*PortType;
	register char	*PortName;
	register char	*TtyId;
	register char	*p;
	register FILE	*f;

	if (ttyid == NOTTY)
		return (DEFTYPE);
	f = fopen(GTTYN, "r");
	if (f == NULL)
		return (DEFTYPE);

	/* split off end of name */
	TtyId = ttyid;
	while (*ttyid)
		if (*ttyid++ == '/')
			TtyId = ttyid;

	/* scan the file */
	while (fgets(typebuf, sizeof typebuf, f) != NULL)
	{
		p = PortType = typebuf;
		while (*p && isalnum(*p))
			p++;
		*p++ = NULL;

		/* skip separator */
		while (*p && !isalnum(*p))
			p++;

		PortName = p;
		/* put NULL at end of name */
		while (*p && isalnum(*p))
			p++;
		*p = NULL;

		/* check match on port name */
		if (sequal(PortName, TtyId))	/* found it */
		{
			fclose (f);
			return(PortType);
		}
	}
	fclose (f);
	return (DEFTYPE);
}
# endif

baudrate(p)
char	*p;
{
	char buf[8];
	int i = 0;

	while (i < 7 && (isalnum(*p) || *p == '.'))
		buf[i++] = *p++;
	buf[i] = NULL;
	for (i=0; speeds[i].string; i++)
		if (sequal(speeds[i].string, buf))
			return (speeds[i].speed);
	return (-1);
}

char *
mapped(type)
char	*type;
{
	extern short	ospeed;
	int	match;

# ifdef DEB
	printf ("spd:%d\n", ospeed);
	prmap();
# endif
	Map = map;
	while (Map->Ident)
	{
		if (*(Map->Ident) == NULL || sequal(Map->Ident, type))
		{
			match = NO;
			switch (Map->Test)
			{
				case ANY:	/* no test specified */
				case ALL:
					match = YES;
					break;
				
				case GT:
					match = (ospeed > Map->Speed);
					break;

				case GE:
					match = (ospeed >= Map->Speed);
					break;

				case EQ:
					match = (ospeed == Map->Speed);
					break;

				case LE:
					match = (ospeed <= Map->Speed);
					break;

				case LT:
					match = (ospeed < Map->Speed);
					break;

				case NE:
					match = (ospeed != Map->Speed);
					break;
			}
			if (match)
				return (Map->Type);
		}
		Map++;
	}
	/* no match found; return given type */
	return (type);
}

# ifdef DEB
prmap()
{
	Map = map;
	while (Map->Ident)
	{
	printf ("%s t:%d s:%d %s\n",
		Map->Ident, Map->Test, Map->Speed, Map->Type);
	Map++;
	}
}
# endif

char *
nextarg(argc, argv)
int	argc;
char	*argv[];
{
	if (argc <= 0)
		fatal ("Too few args: ", *argv);
	if (*(*++argv) == '-')
		fatal ("Unexpected arg: ", *argv);
	return (*argv);
}

fatal (mesg, obj)
char	*mesg;
char	*obj;
{
	prs (mesg);
	prs (obj);
	prc ('\n');
	prs (USAGE);
	flush();
	exit(1);
}
