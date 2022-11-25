# $Source: /misc/source_product/9.10/commands.rcs/lib/libcurses/screen/makefile,v $
# @(#) $Revision: 72.2 $

# Common makefile for libcurses/screen subdirectory

#
#	MINICURSES is a flag used to compile a small version of
#       libcurses.  It should not be used for the compiling of
#       libcurses.a.  It is strictly for the application programmers
#       convenience.
#
#	If MINICURSES is desired with ti4 then type:
#	    "make ti4 MINICURSES=-DMINICURSES"
#

NPARTS		= 8
MINICURSES	=
FLAGS		= -DVIDEO -DKEYPAD

OBJS =			\
	__cflush.o	\
	__sscans.o	\
	_blanks.o	\
	_c_clean.o	\
	_clearhl.o	\
	_clearline.o	\
	_comphash.o	\
	_delay.o	\
	_delchars.o	\
	_dellines.o	\
	_dumpwin.o	\
	_ec_quit.o	\
	_fixdelay.o	\
	_fixScreen.o	\
	_fixWindow.o	\
	_forcehl.o	\
	_hlmode.o	\
	_id_char.o	\
	_init_cost.o	\
	_inschars.o	\
	_insmode.o	\
	_kpmode.o	\
	_line_free.o	\
	_ll_move.o	\
	_outch.o	\
	_outchar.o	\
	_pos.o		\
	_reset.o	\
	_scrdown.o	\
	_scrollf.o	\
	_setctrl.o	\
	_sethl.o	\
	_setmode.o	\
	_setwind.o	\
	_shove.o	\
	_showctrl.o	\
	_sprintw.o	\
	_sputc.o	\
	_syncmodes.o	\
	_tscroll.o	\
	_winchTrap.o	\
	_window.o	\
	_winlist.o	\
	addch.o		\
	addstr.o	\
	baudrate.o	\
	beep.o		\
	box.o		\
	capnames.o	\
	cbreak.o	\
	chktypeahd.o	\
	clear.o		\
	clearok.o	\
	clreolinln.o	\
	clrtobot.o	\
	clrtoeol.o	\
	cntcostfn.o	\
	crmode.o	\
	curses.o	\
	def_prog.o	\
	def_shell.o	\
	delayoutpt.o	\
	delch.o		\
	delcurterm.o	\
	deleteln.o	\
	delwin.o	\
	doprnt.o	\
	doscan.o	\
	doupdate.o	\
	draino.o	\
	echo.o		\
	endwin.o	\
	erase.o		\
	erasechar.o	\
	fixterm.o	\
	flash.o		\
	flushinp.o	\
	getch.o		\
	getstr.o	\
	gettmode.o	\
	has_ic.o	\
	has_il.o	\
	idln.getst.o	\
	idlok.o		\
	initkeypad.o	\
	initscr.o	\
	insch.o		\
	insertln.o	\
	intrflush.o	\
	keypad.o	\
	killchar.o	\
	leaveok.o	\
	line_alloc.o	\
	ll_refresh.o	\
	longname.o	\
	m_addch.o	\
	m_addstr.o	\
	m_clear.o	\
	m_erase.o	\
	m_getch.o	\
	m_move.o	\
	m_refresh.o	\
	m_setctrl.o	\
	m_showctrl.o	\
	m_tstp.o	\
	makenew.o	\
	meta.o		\
	miniinit.o	\
	move.o		\
	mvcur.o		\
	mvprintw.o	\
	mvscanw.o	\
	mvwin.o		\
	mvwprintw.o	\
	mvwscanw.o	\
	naps.o		\
	newpad.o	\
	newterm.o	\
	newwin.o	\
	nl.o		\
	nocbreak.o	\
	nocrmode.o	\
	nodelay.o	\
	noecho.o	\
	nonl.o		\
	noraw.o		\
	nttychktrm.o	\
	overlay.o	\
	overwrite.o	\
	pnoutrfrsh.o	\
	prefresh.o	\
	printw.o	\
	putp.o		\
	raw.o		\
	reset_prog.o	\
	resetshell.o	\
	resetterm.o	\
	resetty.o	\
	restarttrm.o	\
	saveterm.o	\
	savetty.o	\
	scanw.o		\
	scroll.o	\
	scrollok.o	\
	select.o	\
	set_term.o	\
	setbuffred.o	\
	setcurterm.o	\
	setterm.o	\
	setupterm.o	\
	showstring.o	\
	subwin.o	\
	tgetent.o	\
	tgetflag.o	\
	tgetnum.o	\
	tgetstr.o	\
	tgoto.o		\
	touchwin.o	\
	tparm.o		\
	tputs.o		\
	traceonoff.o	\
	tstp.o		\
	two.twostr.o	\
	typeahead.o	\
	unctrl.o	\
	vidattr.o	\
	vidputs.o	\
	vsprintf.o	\
	vsscanf.o	\
	wattroff.o	\
	wattron.o	\
	wattrset.o	\
	wnoutrfrsh.o	\
	wprintw.o	\
	wrefresh.o	\
	writechars.o	\
	wscanw.o	\
	wstandend.o	\
	wstandout.o

OBJS_SL = $(OBJS:.o=_sl.o)

default: all

all: libcurses.a libcurses.sl tic

libcurses.a: $(OBJS) hpux_rel.o
	rm -f libcurses.a
	ar qc libcurses.a hpux_rel.o `lorder $(OBJS) | tsort`

libcurses.sl: $(OBJS_SL) hpux_rel.o
	rm -f libcurses.sl
	ld -b -o libcurses.sl hpux_rel.o `lorder $(OBJS_SL) | tsort`

wrapup:
	@$(MAKE) TYPE="" OBJECTS="$(OBJS)" wrap

wrapup.sl:
	@$(MAKE) TYPE="_sl" OBJECTS="$(OBJS_SL)" wrap

objects:
	@$(MAKE) TYPE="" OBJECTS="$(OBJS)" objs

objects.sl:
	@$(MAKE) TYPE="_sl" OBJECTS="$(OBJS_SL)" objs

#
# wrap --
#    Checks to see if all sub-portions of libcurses have completed.
#    If so, creates libcurses.a from the objects
#    Also creates the tic command.
#
wrap: hpux_rel.o
	@if [ ! -f OK$(TYPE).objs ]; then			\
	    echo "\tChecking sub-portions...";			\
	    i=1;						\
	    bad=false;						\
	    while [ $$i -le $(NPARTS) ]; do			\
		if [ ! -f OK$(TYPE).objs$$i ]; then		\
		    echo "Sub-part $$i didn't work!" >&2;	\
		    bad=true;					\
		fi;						\
		i=`expr $$i + 1`;				\
	    done;						\
	    if $$bad; then					\
		exit 1;						\
	    else						\
		touch OK$(TYPE).objs;				\
	    fi;							\
	fi
	@if [ ! -f OK$(TYPE).objs ]; then			\
	    echo "Not all objects were created!" >&2;		\
	    exit 1;						\
	fi
	@if [ "$(TYPE)" = "_sl" ]; then				\
	    rm -f libcurses.sl;				\
	    echo "\tCreating libcurses.sl";			\
	    ld -b -o libcurses.sl hpux_rel.o			\
			`lorder $(OBJECTS) | tsort`;		\
	else							\
	    rm -f libcurses.a;				\
	    echo "\tCreating libcurses.a";			\
	    ar qc libcurses.a hpux_rel.o 			\
			`lorder $(OBJECTS) | tsort`;		\
	fi
	
tic: tic.o capnames.o
	$(CC) $(CFLAGS) $(LDFLAGS) tic.o capnames.o -o tic

#
# objs --
#        Based on the value of $(PART), either just call do_gen (PART
#        is "all") or split the object list into $(NPART) pieces and
#        call d_gen with the PART piece of the entire object list.
#
#        The object list is split into NPART pieces by placing the
#        file names into a file [one file per line] and then using
#        split to split the file into the correct number of pieces.
#        The appropriate file is then chosen and the object file names
#        are read from it to be passed to "do_objs".
#
#        NPARTS can be from 1 to 9.
#        
objs:
	@if [ "$(PART)" = "all" ]; then				\
	    objs="$(OBJECTS)";					\
	else							\
	    tmp=olst$$$$;					\
	    rm -f $$tmp;					\
	    for i in $(OBJECTS); do				\
		echo $$i;					\
	    done >$$tmp;					\
	    lines=`wc -l <$$tmp`;				\
	    n=`expr \( $$lines + $(NPARTS) - 1 \) / $(NPARTS)`;	\
	    split -$$n $$tmp $$tmp;				\
	    suffix=a`echo $(PART) | tr 123456789 abcdefghi`;	\
	    objs=`cat $$tmp$$suffix`;				\
	    objs=`echo $$objs`;					\
	    rm -f $$tmp*;					\
	fi;							\
	$(MAKE) TYPE=$(TYPE) OBJECTS="$$objs" PART=$(PART) \
		DIR=$(DIR) do_objs;

#
# do_objs -- do the real work.  If PART is not all or 1 we also
#            touch a flag file to indicate that our part was
#            sucessful.  If part is all we just touch the master
#            flag file.
#
do_objs: $(OBJECTS)
	if [ $(PART) != "all" ]; then		\
	    touch OK$(TYPE).objs$(PART);	\
	else					\
	    touch OK$(TYPE).objs;		\
	fi

ti4: libcurses.a ti4.o
	$(CC) $(CFLAGS) $(MINICURSES) ti4.o libcurses.a -o ti4

ti4.o:		
	$(CC) mkti4.c -o mkti4
	./mkti4 >ti4.c
	$(CC) $(CFLAGS) $(FLAGS) $(MINICURSES) -c ti4.c 

clean:	
	@echo "\tRemoving object files..."
	@rm -f $(OBJS) tic.o ti4.o
	@rm -f $(OBJS_SL)
	@rm -f OK*

clobber: clean
	rm -f libcurses.a libcurses.sl tic ti4

#
# Re-define .c.o rule so compiles will use FLAGS
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) $(MINICURSES) -c $<

.c_sl.o:
	$(CC) $(CFLAGS) $(FLAGS) $(MINICURSES) +z -c $< -o $@

#
# LOCAL DEPENDENCIES
#

$(OBJS) $(OBJS_SL):	curses.ext

ns_curses.h 	\
curses.ext: 	curshdr.h

_delay.o 	\
tparm.o 	\
tputs.o: 	ns_curses.h

fixterm.o 	\
reset_prog.o 	\
resetshell.o	\
resetrterm.o	\
restarttrm.o	\
saveterm.o	\
setupterm.o	\
tic:		../local/uparm.h

.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
