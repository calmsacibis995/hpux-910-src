/* @(#) $Revision: 66.3 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"
#ifdef ED1000
#include "ex_sm.h"
#endif ED1000

extern	char	*hpux_rev;
bool	pflag, nflag;
int	poffset;

#ifdef ED1000
bool	intr;
bool	diff;
char	temp[128];
char	sm_cmdline[151];
char	sm_org_cmdline[151];
char	*org_input;
char	repeat_line[SM_MAXLLEN+1];
int	repeat_count = 0;
int	repeat_active = 0;
char	*sm_cmd_ptr;
#endif ED1000

#define	nochng()	lchng = chng

#ifndef NONLS8	/* User messages */
# define	NL_SETN	3	/* set number */
# include	<msgbuf.h>
# undef	getchar
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Main loop for command mode command decoding.
 * A few commands are executed here, but main function
 * is to strip command addresses, do a little address oriented
 * processing and call command routines to do the real work.
 */
commands(noprompt, exitoneof)
	bool noprompt, exitoneof;
{
	register line *addr;
	register int c;
	register int lchng;
	int given;
	int seensemi;
	int cnt;
	bool hadpr;
	char *vgetpass();

#ifdef ED1000
        int	i,n;
        char	dummy;
        char	ch[10];
        char	*ptr;
        int	godoit = -1;
	int	omagic;
	int	onoprompt;
	int	ohush;
	int	odot;
	int	ointty;
	int	once;
	int	oinput;
	int	index;

	once = 0;
#endif ED1000

	resetflav();
	nochng();

#ifdef ED1000
	godoit = -1;
#endif ED1000

	for (;;) {
		/*
		 * If dot at last command
		 * ended up at zero, advance to one if there is a such.
		 */
		if (dot <= zero) {
			dot = zero;
			if (dol > zero)
				dot = one;
		}
		shudclob = 0;

#ifdef ED1000
		if (godoit == -1)
			godoit = 1;
#endif ED1000
		/*
		 * If autoprint or trailing print flags,
		 * print the line at the specified offset
		 * before the next command.
		 */
		if (pflag ||
		    lchng != chng && value(AUTOPRINT) && !inglobal && !inopen && endline) {
			pflag = 0;
			nochng();
			if (dol != zero) {
				addr1 = addr2 = dot + poffset;
				if (addr1 < one || addr1 > dol)
error((nl_msg(1, "Offset out-of-bounds|Offset after command too large")));
				setdot1();
				goto print;
			}
		}
		nochng();

		/*
		 * Print prompt if appropriate.
		 * If not in global flush output first to prevent
		 * going into pfast mode unreasonably.
		 */
		if (inglobal == 0) {
			flush();
#ifdef ED1000
			if (!hush && value(PROMPT) && !globp &&
			    !noprompt && !endline && repeat_active) {
				printf("/");
				while (!sm_eol(*org_input)){
					printf("%c",*org_input);
					org_input++;
				}
				printf("\n");
				org_input++;
				flush();
			}
			if (!hush && value(PROMPT) && !globp &&
			    !noprompt && endline) {
				putchar('/');
				putchar('\021'|QUOTE);
				flush();
				hadpr = 1;
				if (repeat_count){
					printf("_%6d\n",repeat_count);
					repeat_count--;
					strcpy(&sm_cmdline[1],repeat_line);
					sm_cmdline[0] = ' ';
					org_input = sm_org_cmdline;
					repeat_active = 1;
				}else{
					sm_cmdline[1] = getach();
					strcpy(&sm_cmdline[2],input);
					if (sm_cmdline[1] == '/'){
						getaline();
					}
					putaline();
					sm_cmdline[0] = '%';  /* mark for sm */
					strcpy(sm_org_cmdline,&sm_cmdline[1]);
					repeat_active = 0;
				}
				sm_trans(sm_cmdline); 
				flush();
			}else{
				if (intr && endline){
					if (once)
						input = oinput;
					else
						input = 0;
					ointty = intty;
					intty = 1;
					sm_cmd_ptr = sm_cmdline;
					index = 0;
					while (index < 152){
						*sm_cmd_ptr++ = '\0';
						index++;
					}
					sm_cmd_ptr = sm_cmdline;
					sm_cmd_ptr++;
					while ((c = getach()) != '\n'){
						*sm_cmd_ptr++ = c;
						if (c == EOF)	break;
						if (c == '\04')	break;
					}
					*sm_cmd_ptr++ = c;
					if (sm_cmdline[1] == '\04'){
						sm_cmdline[0] = ' ';
						sm_cmdline[2] = '\0';
					}else{
						sm_cmdline[0] = '%';
					}
					oinput = input;
					sm_trans(sm_cmdline); 
					flush();
					intty = ointty;
					once = 1;
				}
#else ED1000
			if (!hush && value(PROMPT) && !globp && !noprompt && endline) {
				putchar(':');
#ifdef TEPE
				/* hp3000 tepe testing prompt */
				outchar('\021');
#endif
				hadpr = 1;
#endif ED1000
			}
			TSYNC();
		}

		/*
		 * Gobble up the address.
		 * Degenerate addresses yield ".".
		 */
#ifdef ED1000
		diff = 0;
#endif ED1000
		addr2 = 0;
		given = seensemi = 0;
		do {
			addr1 = addr2;
			addr = address(0);
			c = getcd();
			if (addr == 0)
				if (c == ',')
					addr = dot;
				else if (addr1 != 0) {
					addr2 = dot;
					break;
				} else
					break;
			addr2 = addr;
			given++;
			if (c == ';') {
				c = ',';
				dot = addr;
				seensemi = 1;
			}
		} while (c == ',');
		if (c == '%') {
			/* %: same as 1,$ */
			addr1 = one;
			addr2 = dol;
			given = 2;
			c = getchar();
		}
		if (addr1 == 0)
			addr1 = addr2;
#ifdef ED1000
		else
			diff = 1;
#endif ED1000
		if (c == ':')
			c = getchar();

		/*
		 * Set command name for special character commands.
		 */
		tailspec(c);

		/*
		 * If called via : escape from open or visual, limit
		 * the set of available commands here to save work below.
		 */
		if (inopen) {
			if (c=='\n' || c=='\r' || c==CTRL(d) || c==EOF) {
				if (addr2)
					dot = addr2;
				if (c == EOF)
					return;
				continue;
			}
			if (any(c, "o"))
notinvis:
				tailprim(Command, 1, 1);
		}
choice:
		laste = 0;
		switch (c) {

		case 'a':

			switch(peekchar()) {
			case 'b':
/* abbreviate */
				tail("abbreviate");
				setnoaddr();
				mapcmd(0, 1);
				anyabbrs = 1;
				continue;
			case 'r':
/* args */
				tail("args");
				setnoaddr();
				eol();
				pargs();
				continue;
#ifdef ED1000
/* ask */
			case 's':
				tail("ask");
				eol();
				if ( value(ASK) ){
					ptr = "OK? \007";
					write(2,ptr,strlen(ptr));
					lseek(2,0L,2);
					read(2,ch,10);

					if (ch[0] != 'y' && ch[0] != 'Y'){
						godoit = 0;
					} else	godoit = 1; 
				}else godoit = 1;
					continue;
#endif ED1000
			}

/* append */
			if (inopen)
				goto notinvis;
			tail("append");
			setdot();
			aiflag = exclam();
			donewline();
			vmacchng(0);
			deletenone();
			setin(addr2);
			inappend = 1;
			ignore(append(gettty, addr2));
			inappend = 0;
			nochng();
			continue;

		case 'c':
			switch (peekchar()) {

/* copy */
			case 'o':
				tail("copy");
				vmacchng(0);
				move();
				continue;

/* ========================================================================= */
/*
** CRYPT block 1
*/
#ifdef CRYPT
/* crypt */
			case 'r':
				tail("crypt");
ent_crypt:
				setnoaddr();
				xflag = 1;
				xeflag = 0;
				key = vgetpass("Entering encrypting mode.  Key:");
				kflag = crinit(key, perm);
				continue;
#endif CRYPT
/* ========================================================================= */

/* cd */
			case 'd':
				tail("cd");
				goto changdir;

/* chdir */
			case 'h':
				ignchar();
				if (peekchar() == 'd') {
					register char *p;
					tail2of("chdir");
changdir:
					if (savedfile[0] == '/' || !value(WARN))
						ignore(exclam());
					else
						ignore(quickly());
					if (skipend()) {
						p = getenv("HOME");
						if (p == NULL)
							error((nl_msg(2, "Home directory unknown")));
					} else
						getone(), p = file;
					eol();
					if (chdir(p) < 0)
						filioerr(p);
					if (savedfile[0] != '/')
						edited = 0;
					continue;
				}
				if (inopen)
					tailprim("change", 2, 1);
				tail2of("change");
				break;

			default:
				if (inopen)
					goto notinvis;
				tail("change");
				break;
			}
/* change */
			aiflag = exclam();
			setCNL();
			vmacchng(0);
			setin(addr1);
			delete(0);
			inappend = 1;
			ignore(append(gettty, addr1 - 1));
			inappend = 0;
			nochng();
			continue;

/* delete */
		case 'd':
			/*
			 * Caution: dp and dl have special meaning already.
			 */
			tail("delete");
#ifdef ED1000
			if ( godoit > 0){
				c = cmdreg();
				setCNL();
				vmacchng(0);
				if (c)
					YANKreg(c);
				delete(0);
				appendnone();
			}
			godoit = -1;
#else ED1000
			c = cmdreg();
			setCNL();
			vmacchng(0);
			if (c)
				YANKreg(c);
			delete(0);
			/*
			 * Next line added so that :1,6d will display
			 * "6 lines deleted" instead of "6 lines".
			 */
			notenam = "delete";
			/*
			 * Next line added so that the sequence `dw:d^Mp'
			 * will restore the line instead of the word.
			 */
			DEL[0] = 0;
			appendnone();
#endif ED1000
			continue;

/* edit */
/* ex */
		case 'e':
			tail(peekchar() == 'x' ? "ex" : "edit");
#ifdef ED1000
			if (godoit > 0) {
				godoit = -1;
#endif ED1000
editcmd:
			if (!exclam() && chng)
				c = 'E';
			filename(c);
			if (c == 'E') {
				ungetchar(lastchar());
				ignore(quickly());
			}
			setnoaddr();
doecmd:
			init();
			addr2 = zero;
			laste++;
			sync();
			rop(c);
			nochng();
#ifdef ED1000
			}else{
				/* setlastchar('\n');
				 * while (lastchar() != '\n' && lastchar() != EOF)
				 *	ignchar();
				 */
				peekc = 0;
				globp = input = 0;
				godoit = -1;
				endline = 1;
			}
#endif ED1000
			continue;

/* file */
		case 'f':
			tail("file");
			setnoaddr();
			filename(c);
			noonl();
#ifdef ED1000
			synctmp();
#endif ED1000
			continue;

/* global */
		case 'g':
			tail("global");
			global(!exclam());
			nochng();
			continue;

#ifdef ED1000
		case '?':	/*ddl.rel1 Add in ? command*/
		case 'h':
			sm_help();
			continue;
#endif ED1000

/* insert */
		case 'i':
			if (inopen)
				goto notinvis;
			tail("insert");
			setdot();
			nonzero();
			aiflag = exclam();
			donewline();
			vmacchng(0);
			deletenone();
			setin(addr2);
			inappend = 1;
			ignore(append(gettty, addr2 - 1));
			inappend = 0;
			if (dot == zero && dol > zero)
				dot = one;
			nochng();
			continue;

/* join */
		case 'j':
			tail("join");
			c = exclam();
			setcount();
			nonzero();
			donewline();
			vmacchng(0);
			if (given < 2 && addr2 != dol)
				addr2++;
			join(c);
			continue;

/* k */
		case 'k':
casek:
			pastwh();
			c = getchar();
			if (endcmd(c))
				serror((nl_msg(3, "Mark what?|%s requires following letter")), Command);

#ifdef	NLS16
			if (IS_FIRST(c))
				/* ignore the 2nd byte of 16-bit character to
				** get the proper error message (below).
				*/
				ignchar();
#endif

			donewline();

#ifndef NONLS8	/* 8bit integrity */
			/*
			 * ONLY the ASCII letters a-z may be used, not all
			 * the 8 or 16-bit characters that might be defined
			 * as lowercase.
			 */
			if ((c < IS_MACRO_LOW_BOUND) || !islower(c & TRIM) || !isascii(c & TRIM))
#else NONLS8
			if ((c < IS_MACRO_LOW_BOUND) || !islower(c))
#endif NONLS8

				error((nl_msg(4, "Bad mark|Mark must specify a letter")));
			setdot();
			nonzero();
			names[c - 'a'] = *addr2 &~ 01;
			anymarks = 1;
			continue;

/* list */
		case 'l':
			tail("list");
			setCNL();
			ignorf(setlist(1));
			pflag = 0;
			goto print;

		case 'm':
			if (peekchar() == 'a') {
				ignchar();
				if (peekchar() == 'p') {
/* map */
					tail2of("map");
					setnoaddr();
					mapcmd(0, 0);
					continue;
				}
/* mark */
				tail2of("mark");
				goto casek;
			}
/* move */
			tail("move");
#ifdef ED1000
			if (godoit>0){
				vmacchng(0);
				move();
			}
			godoit = -1;
#else ED1000
			vmacchng(0);
			move();
#endif ED1000
			continue;

		case 'n':
			if (peekchar() == 'u') {
				tail("number");
				goto numberit;
			}
/* next */
			tail("next");
			setnoaddr();
			ckaw();
			ignore(quickly());
			if (getargs())
				makargs();
			next();
			c = 'e';
			filename(c);
			goto doecmd;

/* open */
		case 'o':
			tail("open");
			oop();
			pflag = 0;
			nochng();
			continue;

		case 'p':
		case 'P':
			switch (peekchar()) {

/* put */
			case 'u':
				tail("put");
				setdot();
				c = cmdreg();
				eol();
				vmacchng(0);
				if (c)
					putreg(c);
				else
					put();
				continue;

			case 'r':
				ignchar();
				if (peekchar() == 'e') {
/* preserve */
					tail2of("preserve");
					eol();
					if (preserve() == 0)
						error((nl_msg(5, "Preserve failed!")));
					else
						error((nl_msg(6, "File preserved.")));
				}
				tail2of("print");
				break;

			default:
				tail("print");
				break;
			}
/* print */
			setCNL();
			pflag = 0;
print:
			nonzero();
#ifndef ED1000
			if (clear_screen && span() > lines) {
				flush1();
				vclear();
			}
#endif ED1000
			plines(addr1, addr2, 1);
			continue;

/* quit */
		case 'q':
			tail("quit");
			setnoaddr();
			c = quickly();
			eol();
#ifdef ED1000
			if (godoit > 0) {
				godoit = -1;
#endif ED1000

#if defined NLS || defined NLS16
			if (right_to_left)
				reset_rlterm();
			if (!c) {
quit:
				if (right_to_left)
					reset_rlterm();
				nomore();
			}
#else
			if (!c)
quit:
				nomore();
#endif
			if (inopen) {
				vgoto(WECHO, 0);
				if (!ateopr())
					vnfl();
				else {
					tostop();
				}
				flush();
				setty(normf);
			}
			cleanup(1);
			exit(0);
#ifdef ED1000
			} else {
				godoit = -1;
				continue;
			}
#endif ED1000

		case 'r':
			if (peekchar() == 'e') {
				ignchar();
				switch (peekchar()) {

/* rewind */
				case 'w':
					tail2of("rewind");
					setnoaddr();
					if (!exclam()) {
						ckaw();
						if (chng && dol > zero)

#ifndef NONLS8	/* User messages */
							error((nl_msg(7, "No write|No write since last change (:rewind! overrides)")));
#else NONLS8
							error("No write@since last change (:rewind! overrides)");
#endif NONLS8

					}
					eol();
					erewind();
					next();
					c = 'e';
					ungetchar(lastchar());
					filename(c);
					goto doecmd;

/* recover */
				case 'c':
					tail2of("recover");
					setnoaddr();
					c = 'e';
					if (!exclam() && chng)
						c = 'E';
					filename(c);
					if (c == 'E') {
						ungetchar(lastchar());
						ignore(quickly());
					}
					init();
					addr2 = zero;
					laste++;
					sync();
					recover();
					rop2();
					revocer();
					if (status == 0)
						rop3(c);
					else if (laste) {
#ifdef VMUNIX
						tlaste();
#endif
						laste = 0;
						sync();
					}
					if (dol != zero)
						change();
					nochng();
					continue;
				}
				tail2of("read");
			} else
				tail("read");
/* read */

			if (real_empty) {
				/*
				 * ensure that the lines read in are placed
				 * prior to the dummy line visual stuck in an
				 * otherwise empty file and remember that we
				 * still have to deal with the dummy line
				 * below
				 */
				addr1 = addr2 = zero;
				oreal_empty = real_empty;
			} else
				oreal_empty = 0;

			if (savedfile[0] == 0 && dol == zero)
				c = 'e';
			pastwh();
			vmacchng(0);
			if (peekchar() == '!') {
				setdot();
				ignchar();
				unix0(0);
				filter(0);

				/*
				 * If we had a dummy line in an empty buffer and
				 * we were able to read some real line(s) into
				 * the buffer, then shift dol so that the dummy
				 * empty line is moved from the current set of
				 * lines to the undo save area.  If the user
				 * undo's this ":read" then the buffer will once
				 * again contain the blank line.  Unfortunately,
				 * after the undo the blank line will no longer
				 * be flagged as not real (via real_empty).  Few
				 * users should either notice or care about this
				 * shortcoming and it would be difficult if not
				 * impossible to fix in the current algorithm.
				 */
				if (oreal_empty && (dol > one))
					dol -= 1;
				oreal_empty = 0;

				continue;
			}
			filename(c);
			/*
			 * The same dummy line processing as above is performed
			 * in rop2() which is called by rop() below.  Because of
			 * modelines processing, we can't wait until rop()
			 * returns to patch up the dummy blank line.
			 */
			rop(c);
			nochng();
			if (inopen && endline && addr1 > zero && addr1 < dol)
				dot = addr1 + 1;
#ifdef ED1000
			if (savedfile[0] != 0)
				CP(file, savedfile);
#endif ED1000
			continue;

		case 's':
			switch (peekchar()) {
			/*
			 * Caution: 2nd char cannot be c, g, or r
			 * because these have meaning to substitute.
			 */
#ifdef ED1000
			case 'r':
				tail("sr");
				eol();
				value(MAGIC) = omagic;
				noprompt = onoprompt;
				hush = ohush;
				dot = odot;
				setdot1();
				continue;
			case 's':
				tail("ss");
				if (dol == zero){
					while (((c = getchar()) != '\n') && (c != EOF)){
					}
					continue;
				}
				eol();
				omagic = value(MAGIC);
				ohush = hush;
				hush = 1;
				odot = dot;
				value(MAGIC) = 1;
				onoprompt = noprompt;
				noprompt  = 1;
				continue;
#endif ED1000

/* set */
			case 'e':
				tail("set");
				setnoaddr();
				set();
				continue;

/* shell */
			case 'h':
#ifndef ED1000
				tail("shell");
				setNAEOL();
				vnfl();
				putpad(exit_ca_mode);
				flush();
				resetterm();
#if defined NLS || defined NLS16
				if (right_to_left)
					reset_rlterm();
#endif
				unixwt(1, unixex("-i", (char *) 0, 0, 0));
#if defined NLS || defined NLS16
				if (right_to_left)
					set_rlterm();
#endif
				vcontin(0);
				continue;
#else ED1000
				flush();
				rte_show();
				/* shell escape is probably very useful.
				 * should add this in later; right now
				 * 'sh' is the show command.
				 */
				continue;

/* screen mode */       case 'm':
				tail("sm");
				sm_main("m");
				continue;

/* screen q mode */     case 'q':
				tail("sq");
				sm_main("q");
				continue;
#endif ED1000

/* source */
			case 'o':
#ifdef notdef
				if (inopen)
					goto notinvis;
#endif
				tail("source");
				setnoaddr();
				getone();
				eol();
#ifdef ED1000
				intr = 1;
				source(file, 0);
				intr = 0;
				godoit = -1;
#else ED1000
				source(file, 0);
#endif ED1000
				continue;

#ifdef SIGTSTP
/* stop, suspend */
			case 't':
				tail("stop");
				goto suspend;
			case 'u':
				tail("suspend");
suspend:
				if (!dosusp)
					error(nl_msg(12,"No job control|Not using a shell with job control"));
# ifdef NTTYDISC
				if (ldisc != NTTYDISC)
					error("Old tty driver|Not using new tty driver/shell");
# endif
				c = exclam();
				eol();
				if (!c)
					ckaw();
				onsusp();
				continue;
#endif

			}
			/* fall into ... */

/* & */
/* ~ */
/* substitute */
		case '&':
		case '~':
			Command = "substitute";
			if (c == 's')
				tail(Command);
#ifdef ED1000
			if (godoit>0){
				godoit = -1;
				vmacchng(0);
				if (!substitute(c))
					pflag = 0;
			}else {
				godoit = -1;
				input = globp = peekc = 0;
				endline = 1;
			}
#else ED1000
			vmacchng(0);
			if (!substitute(c))
				pflag = 0;
#endif ED1000
			continue;

/* t */
		case 't':
			if (peekchar() == 'a') {
				tail("tag");
				tagfind(exclam());
				if (!inopen)
					lchng = chng - 1;
				else
					nochng();
				continue;
			}
			tail("t");
			vmacchng(0);
			move();
			continue;

		case 'u':
			if (peekchar() == 'n') {
				ignchar();
				switch(peekchar()) {
/* unmap */
				case 'm':
					tail2of("unmap");
					setnoaddr();
					mapcmd(1, 0);
					continue;
/* unabbreviate */
				case 'a':
					tail2of("unabbreviate");
					setnoaddr();
					mapcmd(1, 1);
					anyabbrs = 1;
					continue;
				}
/* undo */
				tail2of("undo");
			} else
				tail("undo");
			setnoaddr();
			markDOT();

			/*
			 * Don't allow the user to force an undo when there
			 * is nothing to undo--this ability can lead to core
			 * dumps and other problems.

			c = exclam();

			 */

			donewline();
			undo(0);
			continue;

		case 'v':
			switch (peekchar()) {

			case 'e':
/* version */
				tail("version");
				setNAEOL();
#ifdef ED1000
				printf((nl_msg(101, " Ed1000 : ")));
				printf((nl_msg(102, " Use ? for help\n")));
				outcol  = 1;
				outline = 23;	/* This prevents the */
				flush();	/* screen from moving */
						/* up at the start */
#else ED1000
				printf((nl_msg(8, " HP Version %s")), hpux_rev+16);
				noonl();
#endif ED1000
				continue;

/* visual */
			case 'i':
				tail("visual");
				if (inopen) {
					c = 'e';
					goto editcmd;
				}
				vop();
				pflag = 0;
				nochng();
				continue;
			}
/* v */
			tail("v");
			global(0);
			nochng();
			continue;

/* write */
		case 'w':
			c = peekchar();
			tail(c == 'q' ? "wq" : "write");
wq:
			if (skipwh() && peekchar() == '!') {
				pofix();
				ignchar();

				if (real_empty) {
					/*
					   don't write out the dummy line visual
					   stuck in an otherwise empty file
					*/
					setall();
					delete(0);
					addr2 = 0;
				}

				setall();
				unix0(0);
				filter(1);
			} else {

				if (real_empty) {
					/*
					   don't write out the dummy line visual
					   stuck in an otherwise empty file
					*/
					setall();
					delete(0);
					addr2 = 0;
				}

				setall();
				wop(1);
				nochng();
			}
			if (c == 'q')
				goto quit;
			continue;
/* ========================================================================= */
/*
** CRYPT block 2
*/
#ifdef CRYPT
/* X: crypt */
		case 'X':
			goto ent_crypt;
#endif CRYPT
/* ========================================================================= */

/* xit */
		case 'x':
			tail("xit");
			if (!chng)
				goto quit;
			c = 'q';
			goto wq;

/* yank */
		case 'y':
			tail("yank");
			c = cmdreg();
			setcount();
			eol();
			vmacchng(0);
			if (c)
				YANKreg(c);
			else
				yank();
			/*
			 * line added so that the sequence `dw:ya^Mp'
			 * will copy the line instead of restoring the word.
			 */
			DEL[0] = 0;
			continue;

/* z */
		case 'z':
			zop(0);
			pflag = 0;
			continue;

/* * */
/* @ */
		case '*':
#ifdef ED1000
			while (!endcmd(getcd())){
			}
			continue;
#endif ED1000
		case '@':
			c = getchar();
			if (c=='\n' || c=='\r')
				ungetchar(c);
			if (any(c, "@*\n\r"))
				c = lastmac;

#ifndef NONLS8	/* 8bit integrity */
		        if (c >= IS_MACRO_LOW_BOUND) {
			    if (isupper(c & TRIM))
				c = tolower(c & TRIM);
			    if (!islower(c & TRIM))
#else NONLS8
		        if (c >= IS_MACRO_LOW_BOUND) {
			    if (isupper(c))
				c = tolower(c);
			    if (!islower(c))
#endif NONLS8

				error((nl_msg(9, "Bad register")));
			} else {
			    error((nl_msg(9, "Bad register")));
			}
			donewline();
			setdot();
			cmdmac(c);
			continue;

/* | */
		case '|':
			endline = 0;
			goto caseline;

/* \n */
		case '\n':
			endline = 1;
caseline:
			notempty();
			if (addr2 == 0) {
				if (cursor_up != _NOSTR && c == '\n' && !inglobal)
					c = CTRL(k);
				if (inglobal)
					addr1 = addr2 = dot;
				else {
					if (dot == dol)
						error((nl_msg(10, "At EOF|At end-of-file")));
					addr1 = addr2 = dot + 1;
				}
			}
			setdot();
			nonzero();
			if (seensemi)
				addr1 = addr2;
			getline(*addr1);
			if (c == CTRL(k)) {
				flush1();
				destline--;
				if (hadpr)
					shudclob = 1;
			}
#ifdef ED1000
			if (endline)
				plines(addr1, addr2, 1);
			else
				dot = addr2;
#else
			plines(addr1, addr2, 1);
#endif ED1000
			continue;

/* " */
		case '"':
			comment();
			continue;

/* # */
		case '#':
numberit:
			setCNL();
			ignorf(setnumb(1));
			pflag = 0;
			goto print;

/* = */
		case '=':
			donewline();
			setall();
			if (inglobal == 2)
				pofix();
			printf("%d", lineno(addr2));
			noonl();
			continue;

/* ! */
		case '!':
			if (addr2 != 0) {
				vmacchng(0);
				unix0(0);
				setdot();
				filter(2);
			} else {
				unix0(1);
				pofix();
				putpad(exit_ca_mode);
				flush();
				resetterm();
#if defined NLS || defined NLS16
				if (right_to_left)
					reset_rlterm();
#endif
				unixwt(1, unixex("-c", uxb, 0, 0));
#if defined NLS || defined NLS16
				if (right_to_left)
					set_rlterm();
#endif
				vclrech(1);	/* vcontin(0); */
				nochng();
			}
			continue;
/* < */
/* > */
		case '<':
		case '>':
			for (cnt = 1; peekchar() == c; cnt++)
				ignchar();
			setCNL();
			vmacchng(0);
			shift(c, cnt);
			continue;

/* ^D */
/* EOF */
		case CTRL(d):
		case EOF:
			if (exitoneof) {
				if (addr2 != 0)
					dot = addr2;
				return;
			}
			if (!isatty(0)) {
				if (intty)
					/*
					 * Chtty sys call at UCB may cause a
					 * input which was a tty to suddenly be
					 * turned into /dev/null.
					 */
					onhup();
				return;
			}
			if (addr2 != 0) {
				setlastchar('\n');
				putnl();
			}
			if (dol == zero) {
				if (addr2 == 0)
					putnl();
				notempty();
			}
			ungetchar(EOF);
			zop(hadpr);
			continue;

		default:
#ifndef	NLS16
#ifndef NONLS8	/* Character set features */
			if ((c < IS_MACRO_LOW_BOUND) || !isalpha(c & TRIM))
#else NONLS8
			if ((c < IS_MACRO_LOW_BOUND) || !isalpha(c))
#endif NONLS8
#else
			/* regard 16-bit character as a possible command character */
			if (!IS_KANJI(c) && ((c < IS_MACRO_LOW_BOUND) || !isalpha(c & TRIM)))
#endif

				break;
			ungetchar(c);
			tailprim("", 0, 0);
		}
		error((nl_msg(11, "What?|Unknown command character '%c'")), c);
	}
}
