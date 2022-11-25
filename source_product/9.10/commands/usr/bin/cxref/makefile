# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/cxref/makefile,v $
# $Revision: 72.1.1.7 $

# Common makefile for cxref -- built using the s300 front-end C compiler
# files

# the MIP varirable tells where to find the s300 front-end compiler files
MIP = ../../../lib/compilers.300

XPASS = xpass
CLEANFILES = $(CLN)

# Common flags
FLAGS = -DCXREF -DBUG4 -DNOCDB -DSLOW_WALKF -DC1 -D_HPUX_SOURCE
FLAGS_COMPAT = -UANSI
FLAGS_ANSI  = -DANSI -DLDBL

# S800 specific flags, needed for the cross-compile 
FLAGS800 = $(FLAGS) -Dxcomp300_800 -Dprntf=printf -Dfprntf=fprintf -Dsprntf=sprintf 
FLAGS300 = $(FLAGS)

# flags for unifdef'ing cgram.y
UNIFFLAGS_800 = -UIRIF -DLINT -UAPEX -Dxcomp300_800
UNIFFLAGS_300 = -UIRIF -DLINT -UAPEX -Uxcomp300_800

YACC = yacc

EXTERNFILES=	$(MIP)/cgram.y 	\
		$(MIP)/comm1.c	\
		$(MIP)/optim.c	\
		$(MIP)/pftn.c	\
		$(MIP)/scan.c	\
		$(MIP)/trees.c	\
		$(MIP)/xdefs.c	\
		$(MIP)/messages.c	\
		$(MIP)/messages.h	\
		$(MIP)/manifest	\
		$(MIP)/macdefs	\
		$(MIP)/mfile1	\
		$(MIP)/common	\
		$(MIP)/commonb	\
		$(MIP)/opcodes.h	\
		$(MIP)/mac2defs	\
		$(MIP)/yaccpar

EXTERNFILES300= $(EXTERNFILES) $(MIP)/print.s

EXTERNFILES800= $(EXTERNFILES) $(MIP)/quad.c

# sources linked from other directories that need to be cleaned up
# rm -f won't complain if the file is not there, so don't distinguish
# 300/800 exceptions, just list everything
CLN =		cgram.y		\
		comm1.c		\
		optim.c		\
		pftn.c		\
		scan.c		\
		trees.c		\
		xdefs.c		\
		messages.c	\
		messages.h	\
		manifest	\
		mfile1		\
		common		\
		commonb		\
		opcodes.h	\
		macdefs		\
		mac2defs	\
		print.s		\
		xccom_rel.c	\
		xcpp_rel.c	\
		quad.c		\
		yaccpar


XPASSOBJS    =  cgram.o		\
		comm1.o		\
		optim.o	\
		pftn.o		\
		scan.o		\
		trees.o		\
		xdefs.o		\
		messages.o	\
		cxlocal.o	\
		xpass_rel.o	\
		xccom_rel.o

XPASSOBJS300 = $(XPASSOBJS)  print.o 

XPASSOBJS800 = $(XPASSOBJS) quad.o

XPASSINC = manifest macdefs mfile1 cxmacdefs messages.h

LIBS="-lm" 

default:
	clnfiles="$(CLN)";	\
	xpassinc="$(XPASSINC)";		\
	if hp9000s300; then				\
		efiles="$(EXTERNFILES300)";		\
		xpassobjs="$(XPASSOBJS300)";		\
		flags="$(FLAGS300)";			\
		unifflags="$(UNIFFLAGS_300)";		\
	elif hp9000s800; then				\
		efiles="$(EXTERNFILES800)";             \
		xpassobjs="$(XPASSOBJS800)";            \
		flags="$(FLAGS800)";                    \
		unifflags="$(UNIFFLAGS_800)";		\
	fi;						\
	xpassname="xpass";			\
	target="clean_o all";                   \
	$(MAKE) EFILES="$$efiles" XPASSOBJS="$$xpassobjs"       \
		XPASS="$$xpassname"             \
		CLEANFILES="$$clnfiles" XPASSINC="$$xpassinc"   \
		FLAGS="$$flags $(FLAGS_COMPAT)"	\
		UNIFFLAGS="$$unifflags -UANSI -ULDBL"	\
		CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" MIP="$(MIP)"	\
		$$target ;	\
	xpassname="xpass.ansi";         \
	target="clean_o xpass.ansi";            \
	$(MAKE) EFILES="$$efiles" XPASSOBJS="$$xpassobjs"       \
		XPASS="$$xpassname"             \
		CLEANFILES="$$clnfiles" XPASSINC="$$xpassinc"   \
		FLAGS="$$flags $(FLAGS_ANSI)"    \
		UNIFFLAGS="$$unifflags -DANSI -DLDBL"       \
		CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" MIP="$(MIP)"	\
		$$target


all: link_files cxref $(XPASS)

# --- CXREF ---
cxref: cxr.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(FLAGS) cxr.o -o cxref 


# --- XPASS ---
$(XPASS): $(XPASSOBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(FLAGS) $(XPASSOBJS) -o $(XPASS) $(LIBS)

$(XPASSOBJS): $(XPASSINC)


cgram.c: cgram.y
	-unifdef $(UNIFFLAGS) cgram.y >gram.y
	$(YACC) -l -Xp./yaccpar gram.y
	mv y.tab.c cgram.c
	rm -f gram.y

cgram.o: cgram.c
	$(CC) $(CFLAGS) $(FLAGS) +O1 -c cgram.c

comm1.o: common

#
# Cxref, xpass and xpass.ansi all rely on sources in other places.  This
# target creates links to those sources so we can access the remote
# file as if it were in the current directory.
# The link is symbolic so that a dependency make (patch make) will see
# the date stamps on the actual source files, not the link.
#
link_files:
	ln -s $(EFILES) .
	ln -s $(MIP)/hpux_rel.c ./xccom_rel.c

clean:
	-rm -f *.o cgram.c $(CLEANFILES)

# clean_o target:  clean up .o files and cgram.c ;  needed between the
#       recursive calls to build xpass, and xpass.ansi.
clean_o:
	-rm -f *.o  cgram.c


clobber: clean
	rm -f cxref xpass xpass.ansi

#
# Re-define .c.o rule to use FLAGS
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

# copy /usr/bin/cxref to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cxref
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cxref
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp cxref ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/cxref
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
# copy /usr/bin/cxref to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cxref
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp cxref ../../../../Filesets.sl/C-TOOLS/usr/bin/cxref
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/cxref
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/cxref to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cxref
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp xpass ../../../../Filesets.sl/C-TOOLS/usr/lib/xpass
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/xpass
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/cxref to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cxref
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp xpass.ansi ../../../../Filesets.sl/C-TOOLS/usr/lib/xpass.ansi
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/xpass.ansi
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
