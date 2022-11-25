/* @(#) $Revision: 70.4 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_tty.h"

static char *freespace;

/*
 * Terminal type initialization routines,
 * and calculation of flags at entry or after
 * a shell escape which may change them.
 */
static short GT;

#ifndef NONLS8 /* User messages */
# define	NL_SETN	12	/* set number */
# include	<msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

gettmode()
{

	GT = 1;
#ifndef USG
	if (gtty(2, &tty) < 0)
		return;
	if (ospeed != tty.sg_ospeed)
		value(SLOWOPEN) = tty.sg_ospeed < B1200;
	ospeed = tty.sg_ospeed;
	normf = tty.sg_flags;
	UPPERCASE = (tty.sg_flags & LCASE) != 0;
	if ((tty.sg_flags & XTABS) == XTABS || teleray_glitch)
		GT = 0;
	NONL = (tty.sg_flags & CRMOD) == 0;
#else
	if (ioctl(2, TCGETA, &tty) < 0)
		return;
	if (ismodem(2)) {
		/* baud rate info is only valid if device under modem control */
		if (ospeed != (tty.c_cflag & CBAUD))	/* mjm */
			value(SLOWOPEN) = (tty.c_cflag & CBAUD) < B1200;
		ospeed = tty.c_cflag & CBAUD;
	} else {
		value(SLOWOPEN) = 0;
		ospeed = B38400;	/* no modem control pretend fastest baud rate */
	}
	normf = tty;
	UPPERCASE = (tty.c_iflag & IUCLC) != 0;
	if ((tty.c_oflag & TABDLY) == TAB3 || teleray_glitch)
		GT = 0;
	NONL = (tty.c_oflag & ONLCR) == 0;
#endif
}

static bool hp_style = 0;	/* HP-style keyboard escape sequences? */

setterm(type)
	char *type;
{
	char *tparm(), *tparm1;
	register int unknown, i;
	register int l;
	register int x;
	int errret;
	extern char termtype[];

	unknown = 0;
	if (cur_term && exit_ca_mode)
		putpad(exit_ca_mode);
	cur_term = 0;
	strcpy(termtype, type);
	setupterm(type, 2, &errret);
	if (errret != 1) {
		unknown++;
		cur_term = 0;
		setupterm("unknown", 1, &errret);
	}
	resetterm();
	setsize(1);
#ifdef TRACE
	if (trace) fprintf(trace, "after setupterm, lines %d, columns %d, clear_screen '%s', cursor_address '%s'\n", lines, columns, clear_screen, cursor_address);
#endif
	/*
	 * Initialize keypad arrow keys.
	 */
	freespace = allocspace;

	kpadd(arrows, key_ic, "i", "inschar");
	kpadd(immacs, key_ic, "\033", "inschar");
	kpadd(arrows, key_eic, "i", "inschar");
	kpadd(immacs, key_eic, "\033", "inschar");

	kpboth(arrows, immacs, key_up, "k", "up");
	kpboth(arrows, immacs, key_down, "j", "down");
	kpboth(arrows, immacs, key_left, "h", "left");
	kpboth(arrows, immacs, key_right, "l", "right");
	kpboth(arrows, immacs, key_home, "H", "home");
	kpboth(arrows, immacs, key_il, "o\033", "insline");
	kpboth(arrows, immacs, key_dl, "dd", "delline");
	kpboth(arrows, immacs, key_clear, "\014", "clear");
	kpboth(arrows, immacs, key_eol, "d$", "clreol");
	kpboth(arrows, immacs, key_sf, "\005", "scrollf");
	kpboth(arrows, immacs, key_dc, "x", "delchar");
	kpboth(arrows, immacs, key_npage, "\006", "npage");
	kpboth(arrows, immacs, key_ppage, "\002", "ppage");
	kpboth(arrows, immacs, key_sr, "\031", "sr");
	kpboth(arrows, immacs, key_eos, "dG", "clreos");

	for (x=0; x<MAXNOMACS; x++)		/* find start of non-keyboard mappings */
		if (arrows[x].cap == 0)
			break;
	arrows_start = x;
	for (x=0; x<MAXNOMACS; x++)		/* find start of non-keyboard mappings */
		if (immacs[x].cap == 0)
			break;
	immacs_start = x;

	/*
	 * Handle funny termcap capabilities
	 */
	if (change_scroll_region && save_cursor && restore_cursor) insert_line=delete_line="";
	if (parm_insert_line && insert_line==NULL) insert_line="";
	if (parm_delete_line && delete_line==NULL) delete_line="";
	if (insert_character && enter_insert_mode==NULL) enter_insert_mode="";
	if (insert_character && exit_insert_mode==NULL) exit_insert_mode="";
	if (GT == 0)
		tab = back_tab = _NOSTR;

#ifdef SIGTSTP
	/*
	 * Now map users susp char to ^Z, being careful that the susp
	 * overrides any arrow key, but only for hackers (=new tty driver).
	 */
	{
		static char sc[2];
		int i, fnd;

# ifdef	TIOCGETD	
		ioctl(2, TIOCGETD, &ldisc);
# endif
		if (!value(NOVICE) && (olttyc.t_suspc != -1) && (olttyc.t_suspc != 0)) {
			sc[0] = olttyc.t_suspc;
			sc[1] = 0;
			for (i=0; i<=4; i++)
				if (arrows[i].cap[0] == olttyc.t_suspc)
					addmac(sc, NULL, NULL, arrows);
		}
	}
#endif

	if ((tparm1 = tparm(cursor_address, 2, 2)) != NULL)	/* OOPS */
		if (tparm1[0] == 'O')
			cursor_address = 0;
	else
		costCM = cost(tparm(cursor_address, 10, 8));
	costSR = cost(scroll_reverse);
	costAL = cost(insert_line);
	costDP = cost(tparm(parm_down_cursor, 10));
	costLP = cost(tparm(parm_left_cursor, 10));
	costRP = cost(tparm(parm_right_cursor, 10));
	costCE = cost(clr_eol);
	costCD = cost(clr_eos);
	/* proper strings to change tty type */
	termreset();
	gettmode();
	value(REDRAW) = insert_line && delete_line;
	value(OPTIMIZE) = !cursor_address && !tab;
	if (ospeed == B1200 && !value(REDRAW))
		value(SLOWOPEN) = 1;	/* see also gettmode above */
	if (unknown)
		serror((nl_msg(1, "%s: Unknown terminal type")), type);
#ifdef ED1000
/* 	Force the editor to not use cursor motion , and backtabs */

	costCM = 1000;
/*	costSR = 1000;*/
/*	costAL = 1000;*/
	costDP = 1000;
	costLP = 1000;
	costRP = 1000;
	costCE = 1000;
	costCD = 1000;
	tab = back_tab = _NOSTR;
#endif ED1000

}

/************
 * Set up the window size.
 ************/

#ifdef SIGWINCH
struct winsize save_win;
#endif SIGWINCH

setsize(init_setup)
int init_setup;
{
	register int l, i;

#ifdef SIGWINCH
	struct winsize curr_win;
#endif SIGWINCH

/* On initial setup, LINES and COLUMNS should take precedence over the    */
/* real window size, according to POSIX.2 standards.			  */

#ifdef SIGWINCH
	if (init_setup)
	{
		/* use LINES and COLUMNS, instead of 0, as init values */
		save_win.ws_row = lines;
		save_win.ws_col = columns;
	}
	else	/* not initial setup -- called by sigwinch handler */
	{
		if (ioctl(0, TIOCGWINSZ, &curr_win) >= 0)
		{
		    lines = save_win.ws_row = curr_win.ws_row;
                    columns = save_win.ws_col = curr_win.ws_col;
                    sprintf(lines_env_string, "LINES=%d", curr_win.ws_row);
                    putenv(lines_env_string);
                    sprintf(cols_env_string, "COLUMNS=%d", curr_win.ws_col);
		    putenv(cols_env_string);
		}
	}
#endif SIGWINCH

        i = lines;
        if (lines <= 1)
                lines = 24;
        if (lines > TUBELINES)
                lines = TUBELINES;
        l = lines;
        if (ospeed < B1200)
                l = 9;  /* including the message line at the bottom */
        else if (ospeed < B2400)
                l = 17;
        if (l > lines)
                l = lines;
        if (columns <= 4)
                columns = 1000;
        options[WINDOW].ovalue = options[WINDOW].odefault = l - 1;
        if (defwind)
                options[WINDOW].ovalue = defwind;
        options[SCROLL].ovalue = options[SCROLL].odefault =
                hard_copy ? 11 : ((l-1) / 2);
        if (i <= 0)
                lines = 2;

}



/*
 * Map both map1 and map2 as below.  map2 surrounded by esc and i.
 */
kpboth(map1, map2, key, mapto, desc)
struct maps *map1, *map2;
char *key, *mapto, *desc;
{
	char surmapto[30];
	char *p;

	if (key == 0)
		return;
	
	/* if HP-style keyboard escape sequences (and we havn't previously done this) */
	if (!hp_style && *key=='\033' && (key[1] >= IS_MACRO_LOW_BOUND) 
	    && isalpha(key[1] & TRIM) && key[2]=='\0') {
		value(KEYBOARDEDIT_I) = 0;		/* then change keyboardedit! value */
		options[KEYBOARDEDIT_I].odefault = 0;	/* and its default               */
		hp_style++;				/* and don't check anymore       */
	}

	kpadd(map1, key, mapto, desc);
	if (any(*key, "\b\n "))
		return;
	strcpy(surmapto, "\33");
	strcat(surmapto, mapto);
	strcat(surmapto, "a");
	p = freespace;
	strcpy(p, surmapto);
	freespace += strlen(surmapto) + 1;
	kpadd(map2, key, p, desc);
}

/*
 * Define a macro.  mapstr is the structure (mode) in which it applies.
 * key is the input sequence, mapto what it turns into, and desc is a
 * human-readable description of what's going on.
 */
kpadd(mapstr, key, mapto, desc)
struct maps *mapstr;
char *key, *mapto, *desc;
{
	int i;

	for (i=0; i<MAXNOMACS; i++)
		if (mapstr[i].cap == 0)
			break;
	if (key == 0 || i >= MAXNOMACS)
		return;
	mapstr[i].cap = key;
	mapstr[i].mapto = mapto;
	mapstr[i].descr = desc;
}

char *
fkey(i)
	int i;
{
	if (i < 0 || i > 9)
		return _NOSTR;
	switch (i) {
	case 0: return key_f0;
	case 1: return key_f1;
	case 2: return key_f2;
	case 3: return key_f3;
	case 4: return key_f4;
	case 5: return key_f5;
	case 6: return key_f6;
	case 7: return key_f7;
	case 8: return key_f8;
	case 9: return key_f9;
	case 10: return key_f0;
	}
}

/*
 * cost figures out how much (in characters) it costs to send the string
 * str to the terminal.  It takes into account padding information, as
 * much as it can, for a typical case.  (Right now the typical case assumes
 * the number of lines affected is the size of the screen, since this is
 * mainly used to decide if insert_line or scroll_reverse is better, and this always happens
 * at the top of the screen.  We assume cursor motion (cursor_address) has little
 * padding, if any, required, so that case, which is really more important
 * than insert_line vs scroll_reverse, won't be really affected.)
 */
static int costnum;
cost(str)
char *str;
{
	int countnum();

	if (str == NULL || *str=='O')	/* OOPS */
		return 10000;	/* infinity */
	costnum = 0;
	tputs(str, lines, countnum);
	return costnum;
}

/* ARGSUSED */
countnum(ch)
char ch;
{
	costnum++;
}


#include <sys/sysmacros.h>      /* for major/minor macro's for pty check */

/* macros for isapty() */
#ifdef hp9000s500
#define SPTYMAJOR 29            /* major number for slave  pty's */
#define PTYSC     0xfe          /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)
#endif
#if defined(hp9000s200) || defined(hp9000s800)
#define SPTYMAJOR 17            /* major number for slave  pty's */
#define PTYSC     0x00          /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)
#endif

/*
 * ismodem() returns true if the file is a tty and is under modem
 * control (and thus the baud rate info for the device is useful).
 */
ismodem(fd)
int fd;
{
	struct stat sbuf;

	if (tty.c_cflag & CLOCAL)
		return (0);					/* no modem: local line */

	return !( (fstat(fd, &sbuf) == 0) &&			/* no modem if this is a pty */
		  ((sbuf.st_mode & S_IFMT) == S_IFCHR) &&
		  (major(sbuf.st_rdev) == SPTYMAJOR) &&
		  (select_code(sbuf.st_rdev) == PTYSC)    );
}
