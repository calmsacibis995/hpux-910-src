# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/lint/makefile,v $
# $Revision: 72.2.1.45 $

# Common makefile for lint utility

MIP = ../../../lib/compilers.300

# Common flags
#
#  -DBUG4 is necessary to turn off pcc debugging tools
#  (these tools cannot be used with the lint shell script
#  since their output conflicts)
STATS=
FLAGS=-DLINT -DLINT_TRY -DNOCDB -DBUG4 -DANSI \
	-DSILENT_ANSI_CHANGES -DAPEX -DSIZED_ENUMS $(STATS)
LLIB_FLAGS = -Aa -E -Wp,-C -Dlint -D__lint -U_APEX_SOURCE

# S800 specific flags, needed for the cross-compile
FLAGS_800 = $(FLAGS) -Dxcomp300_800 -Dprntf=printf -Dfprntf=fprintf \
		-Dsprntf=sprintf -DSLOW_WALKF 
FLAGS_300 = $(FLAGS) -DNEWC0SUPPORT

# flags for unifdef'ing cgram.y
UNIFFLAGS_800 = -UIRIF -DLINT -Dxcomp300_800 -DAPEX
UNIFFLAGS_300 = -UIRIF -DLINT -Uxcomp300_800 -DAPEX

# flags for unifdef'ing llib-l* sources
# (the AES unifdef flag can be removed when the s800 picks up "AES")
UNIFFLAGS_LLIB_WS = -DAES -U_APEX_SOURCE
UNIFFLAGS_LLIB_MU = -UAES -U_APEX_SOURCE

YACC = yacc -Xp./yaccpar

MIP_FILES = $(MIP)/trees.c $(MIP)/pftn.c $(MIP)/scan.c \
	     $(MIP)/comm1.c $(MIP)/messages.c $(MIP)/optim.c \
	     $(MIP)/xdefs.c $(MIP)/cgram.y \
	     $(MIP)/mfile1 $(MIP)/manifest $(MIP)/common \
	     $(MIP)/commonb $(MIP)/yaccpar $(MIP)/opcodes.h \
	     $(MIP)/mac2defs $(MIP)/messages.h $(MIP)/apex.h \
	     $(MIP)/apex_config.c

MIP_300 = $(MIP_FILES) $(MIP)/print.s $(MIP)/walkf.s

MIP_800 = $(MIP_FILES) $(MIP)/quad.c

# sources linked from other directories that need to be cleaned up
# rm -f won't complain if the file is not there, so don't distinguish
# 300/800 exceptions, just list everything
CCOM_CLEAN = \
    trees.c  pftn.c scan.c comm1.c messages.c optim.c xdefs.c \
    cgram.y mfile1  manifest common commonb yaccpar opcodes.h \
    mac2defs messages.h print.s walkf.s quad.c gram.y apex.h apex_config.c

MFILES = macdefs manifest mfile1

# Source and Object files for first and second passes
CFILES1 = trees.c pftn.c scan.c comm1.c messages.c optim.c xdefs.c \
        lerror.c msgbuf.c lint.c apex_config.c
OFILES1 = trees.o pftn.o scan.o comm1.o messages.o optim.o xdefs.o \
	cgram.o lerror.o msgbuf.o lint.o hpux_rel.o apex_config.o
CFILES2 = lpass2.c lerror2.c msgbuf2.c messages.c
OFILES2 = lpass2.o lerror2.o msgbuf2.o hpux_rel.o messages.o apex_config.o

OBJ1_300 = $(OFILES1) print.o walkf.o
OBJ1_800 = $(OFILES1) quad.o
OBJ2_300 = $(OFILES2) print.o
OBJ2_800 = $(OFILES2)

# Lint libraries
LIBS = llib-lc.ln llib-port.ln llib-lm.ln llib-lcurses.ln llib-ldbm.ln \
	llib-lM.ln
lib =

# Intermediate libraries
TLIBS = llib-lM

#llib-lBSD.ln llib-lPW.ln llib-ldvio.ln llib-lportnls.ln <-apex only

# This sort of structure was needed so that different paths, flags
# and extra files could be specified when run on different machines.
default:
	if hp9000s300 || hp9000s700; then			\
		unifflags_llib="$(UNIFFLAGS_LLIB_WS)";		\
	else							\
		unifflags_llib="$(UNIFFLAGS_LLIB_MU)";		\
	fi;							\
	if hp9000s300; then					\
		mipfiles="$(MIP_300)";				\
		files1="$(OBJ1_300)";				\
		files2="$(OBJ2_300)";				\
		unifflags="$(UNIFFLAGS_300)";           	\
		flags="$(FLAGS_300)";				\
	elif hp9000s800; then					\
		mipfiles="$(MIP_800)";				\
		files1="$(OBJ1_800)";				\
		files2="$(OBJ2_800)";				\
		unifflags="$(UNIFFLAGS_800)";           	\
		flags="$(FLAGS_800)";				\
	fi;							\
	$(MAKE)	MIP_FILES="$$mipfiles"				\
		OFILES1="$$files1" 				\
		OFILES2="$$files2" 				\
		LIBS="$(LIBS) $$lib"				\
		UNIFFLAGS="$$unifflags"				\
		UNIFFLAGS_LLIB="$$unifflags_llib"		\
		PFLAGS="$$flags"				\
		all

all:	link_files lint1 lint2 lint hdck $(LIBS)

# This is to allow all the necessary files to be accessed from the MIP
# directory as if they were in the local directory.
# We use symbolic links so when you stat the file, you get info about
# the actual file and not about the link.
link_files:
	ln -s $(MIP_FILES) .

#makes for the first pass of lint
# also create version of lint1 for use within the build process.
# This lint1 is linked with libc.a and not delivered to integration.
lint1:	$(OFILES1)
	$(CC) $(LDFLAGS) -o lint1 $(OFILES1) -lm
	$(CC) $(LDFLAGS) -Wl,-a,archive -o lint1_arc $(OFILES1) -lm

# Make a lint1 that will run on the host processor when compiling
# lint in the cross-compilation environment.  This is needed so that
# the lint libraries may be built in the cross-compilation environment.

# Dependencies
trees.o pftn.o scan.o:	messages.h $(MFILES)
comm1.o:		common $(MFILES)
messages.o:		messages.c messages.h
optim.o xdefs.o:	$(MFILES)
gram.y:		cgram.y
		-unifdef -DANSI $(UNIFFLAGS) cgram.y >gram.y
cgram.c:	gram.y yaccpar
		$(YACC) gram.y ; mv y.tab.c cgram.c
cgram.o:	cgram.c $(MFILES)
		$(CC) -c +O1 $(PFLAGS) cgram.c
lerror.o msgbuf.o:		lerror.h messages.h
lint.o:		lerror.h lmanifest messages.h $(MFILES)

$(OFILES2):     $(CFILES2) hpux_rel.c manifest lerror.h lmanifest lpass2.h
		$(CC) -DLINT2 $(CFLAGS) $(PFLAGS) -c $<

# makes for the lint libraries
# these will be made only with native compilation
#
# The following kludge to llib-lc because AES not supported on s800 for
# 9.0.  (There are other ways to do it, but this was what hit me
# first.)
#
llib-lc.ln:  llib-lc lint1 mktmpdir
	-unifdef $(UNIFFLAGS_LLIB) llib-lc > tmp/llib-lc;		\
	$(CC) $(LLIB_FLAGS)						\
	    -D_HPUX_SOURCE -D_LANLINK -D_LANBLD -DNFS tmp/llib-lc |	\
	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-lc.ln;			\
	rm -f /tmp/htmp$$$$

llib-port.ln:  llib-port lint1 mktmpdir
	-unifdef $(UNIFFLAGS_LLIB) llib-port  > tmp/llib-port;		\
	$(CC) $(LLIB_FLAGS) tmp/llib-port |				\
	./lint1_arc -vxp -H /tmp/htmp$$$$ >llib-port.ln;		\
	rm -f /tmp/htmp$$$$

llib-lm.ln:  llib-lm lint1 mktmpdir
	-unifdef $(UNIFFLAGS_LLIB) llib-lm  > tmp/llib-lm;	\
	$(CC) $(LLIB_FLAGS) -D_HPUX_SOURCE tmp/llib-lm |	\
	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-lm.ln;		\
	rm -f /tmp/htmp$$$$

llib-lM:	llib-lm
	-unifdef -D_libM llib-lm > llib-lM

llib-lM.ln:  llib-lM lint1 mktmpdir
	-unifdef $(UNIFFLAGS_LLIB) -D_libM llib-lm > tmp/llib-lM;	\
	$(CC) $(LLIB_FLAGS) -D_HPUX_SOURCE tmp/llib-lM |		\
	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-lM.ln;			\
	rm -f /tmp/htmp$$$$

llib-lcurses.ln:  llib-lcurses lint1 mktmpdir
	-unifdef $(UNIFFLAGS_LLIB) llib-lcurses  > tmp/llib-lcurses;	\
	$(CC) $(LLIB_FLAGS) tmp/llib-lcurses |				\
	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-lcurses.ln;		\
	rm -f /tmp/htmp$$$$

llib-ldbm.ln:  llib-ldbm lint1 mktmpdir
	-unifdef $(UNIFFLAGS_LLIB) llib-ldbm  > tmp/llib-ldbm;	\
	$(CC) $(LLIB_FLAGS) -D_HPUX_SOURCE tmp/llib-ldbm |	\
	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-ldbm.ln;		\
	rm -f /tmp/htmp$$$$

# The following four libraries are initially for apex use only.
#llib-lBSD.ln:  llib-lBSD lint1 mktmpdir
#	-unifdef $(UNIFFLAGS_LLIB) llib-lBSD  > tmp/llib-lBSD;		\
#	$(CC) $(LLIB_FLAGS)						\
#	    -D_HPUX_SOURCE -D_LANLINK -D_LANBLD -DNFS tmp/llib-lBSD |	\
#	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-lBSD.ln;			\
#	rm -f /tmp/htmp$$$$

#llib-lPW.ln:  llib-lPW lint1 mktmpdir
#	-unifdef $(UNIFFLAGS_LLIB) llib-lPW  > tmp/llib-lPW;		\
#	$(CC) $(LLIB_FLAGS)						\
#	    -D_HPUX_SOURCE -D_LANLINK -D_LANBLD -DNFS tmp/llib-lPW |	\
#	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-lBSD.ln;			\
#	rm -f /tmp/htmp$$$$

#llib-ldvio.ln:  llib-ldvio lint1 mktmpdir
#	-unifdef $(UNIFFLAGS_LLIB) llib-ldvio  > tmp/llib-ldvio;	\
#	$(CC) $(LLIB_FLAGS)						\
#	    -D_HPUX_SOURCE -D_LANLINK -D_LANBLD -DNFS tmp/llib-ldvio |	\
#	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-ldvio.ln;		\
#	rm -f /tmp/htmp$$$$

#llib-lportnls.ln:  llib-lportnls lint1 mktmpdir
#	-unifdef $(UNIFFLAGS_LLIB) llib-lportnls  > tmp/llib-lportnls;	\
#	$(CC) $(LLIB_FLAGS) -D_HPUX_SOURCE 				\
#	    -D_LANLINK -D_LANBLD -DNFS tmp/llib-lportnls |		\
#	./lint1_arc -vx -H /tmp/htmp$$$$ >llib-lportnls.ln;		\
#	rm -f /tmp/htmp$$$$

lint2:	$(OFILES2)
	$(CC) $(LDFLAGS) -o lint2 $(OFILES2)
	$(CC) $(LDFLAGS) -Wl,-a,archive -o lint2_arc $(OFILES2)

hdck:	hdck.c missing_h.c apex.h apex_config.o
	$(CC) $(CFLAGS) $(FLAGS) -o hdck hdck.c apex_config.o
	$(CC) $(CFLAGS) $(FLAGS) -o missing_h missing_h.c

msgbuf2.o:	lerror.h
lpass2.o:	lerror.h lmanifest lpass2.h manifest
lerror2.o:	lerror.h lmanifest lpass2.h manifest messages.h

lintlint:
	if hp9000s300; then                                     \
		unifflags="$(UNIFFLAGS_300)";                   \
		flags="$(FLAGS_300)";                           \
	elif hp9000s800; then                                   \
		unifflags="$(UNIFFLAGS_800)";                   \
		flags="$(FLAGS_800)";                           \
	fi;                                                     \
	$(MAKE) UNIFFLAGS="$$unifflags"                         \
		PFLAGS="$$flags"                                \
		lintlint1 lintlint2

LINT = lint
lintlint1: $(CFILES1) lerror.h lmanifest
	-unifdef -DANSI $(UNIFFLAGS) cgram.y >lgram.y
	$(YACC) lgram.y ; mv y.tab.c lgram.c
	$(LINT) -bhc $(PFLAGS) lgram.c
	$(LINT) $(PFLAGS) $(CFILES1) lgram.ln -lm
	rm -f lgram.ln lgram.y lgram.c

lintlint2: $(CFILES2) lerror.h lmanifest
	$(LINT) -DLINT2 $(PFLAGS) $(CFILES2)

# Made this a separate target so making individual lint libs will work.
mktmpdir:
	if [ ! -d tmp ]; then	\
		rm -f tmp;	\
		mkdir tmp;	\
	fi;

lint: lint.sh apex.sh
	-unifdef -t -UOSF -UPAXDEV lint.sh > lint
	-unifdef -t -UOSF -UDOMAIN apex.sh > apex

clean:
	rm -f $(CCOM_CLEAN)
	rm -f *.o cgram.c
	rm -f lint1_arc
	rm -f lint2_arc

clobber: clean
	rm -rf tmp
	rm -f apex lint lint1 lint2 hdck missing_h
	rm -f $(LIBS)
	rm -f $(TLIBS)

# Needed to redefine the rule for compilation from a c source file to
# an object level file as to include some extra flags.
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<

# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	lint
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/bin
	! cp lint ../../../../Filesets.sl/BUILDENV/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin/lint
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/bin
	! cp lint ../../../../Filesets.sl/BUILDENV/usr/bin/lint
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin/lint
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp lint ../../../../Filesets.sl/C-TOOLS/usr/bin/lint
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/lint
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp lint1 ../../../../Filesets.sl/BUILDENV/usr/lib/lint1
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/lint1
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp lint1 ../../../../Filesets.sl/C-TOOLS/usr/lib/lint1
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/lint1
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp lint2 ../../../../Filesets.sl/BUILDENV/usr/lib/lint2
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/lint2
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp lint2 ../../../../Filesets.sl/C-TOOLS/usr/lib/lint2
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/lint2
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-lM ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lM
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lM
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-lM.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lM.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lM.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-lc ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lc
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lc
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp llib-lc.ln ../../../../Filesets.sl/BUILDENV/usr/lib/llib-lc.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/llib-lc.ln
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-lc.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lc.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lc.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-lcurses ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lcurses
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lcurses
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-ldbm ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-ldbm
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-ldbm
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-ldbm.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-ldbm.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-ldbm.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-lm ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lm
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lm
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-lm.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lm.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lm.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-port ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-port
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-port
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-port.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-port.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-port.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp llib-lcurses.ln ../../../../Filesets.sl/BUILDENV/usr/lib/llib-lcurses.l
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/llib-lcurses.l
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-lcurses.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lcurses.l
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lcurses.l
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/bin
	! cp lint ../../../../Filesets.sl/BUILDENV/usr/bin/lint
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin/lint
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp lint ../../../../Filesets.sl/C-TOOLS/usr/bin/lint
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/lint
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp lint1 ../../../../Filesets.sl/BUILDENV/usr/lib/lint1
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/lint1
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp lint1 ../../../../Filesets.sl/C-TOOLS/usr/lib/lint1
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/lint1
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp lint2 ../../../../Filesets.sl/BUILDENV/usr/lib/lint2
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/lint2
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp lint2 ../../../../Filesets.sl/C-TOOLS/usr/lib/lint2
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/lint2
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-lM ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lM
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lM
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-lM.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lM.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lM.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-lc ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lc
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lc
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp llib-lc.ln ../../../../Filesets.sl/BUILDENV/usr/lib/llib-lc.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/llib-lc.ln
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-lc.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lc.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lc.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-lcurses ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lcurses
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lcurses
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-ldbm ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-ldbm
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-ldbm
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-ldbm.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-ldbm.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-ldbm.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-lm ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lm
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lm
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-lm.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lm.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lm.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp tmp/llib-port ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-port
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-port
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-port.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-port.ln
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-port.ln
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp llib-lcurses.ln ../../../../Filesets.sl/BUILDENV/usr/lib/llib-lcurses.l
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/llib-lcurses.l
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lint to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/lint
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp llib-lcurses.ln ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lcurses.l
	! chmog 0444 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/llib-lcurses.l
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
