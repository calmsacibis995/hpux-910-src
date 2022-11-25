# $Source: /misc/source_product/9.10/commands.rcs/lib/libPW/makefile,v $
# $Revision: 64.2.1.2 $

# Common makefile for libPW

COMOBJS =	hpux_rel.o alloca.o any.o anystr.o bal.o cat.o clean.o \
		curdir.o dname.o fatal.o fdfopen.o giveup.o imatch.o   \
		index.o lockit.o logname.o patoi.o patol.o regcmp.o \
		regex.o rename.o repeat.o repl.o satoi.o setsig.o sname.o \
		strend.o substr.o trnslat.o userdir.o userexit.o username.o \
		verify.o xalloc.o xcreat.o xlink.o xopen.o xpipe.o xunlink.o \
		xwrite.o xmsg.o zero.o zeropad.o move.o abspath.o
OBJS300 = 
OBJS800 = 
LIB	= libPW.a

default:
	if hp9000s300; then					\
		objs="$(COMOBJS) $(OBJS300)";			\
	elif hp9000s800; then					\
		objs="$(COMOBJS) $(OBJS800)";			\
	fi;							\
	$(MAKE) OBJECTS="$$objs" $(LIB)

$(LIB): $(OBJECTS)
	/bin/rm -f $(LIB)
	ar q $(LIB) `lorder $(OBJECTS) | tsort`

clean:
	/bin/rm -f $(COMOBJS) $(OBJS300) $(OBJS800)

clobber:	clean
	/bin/rm -f $(LIB)
# copy /lib/libPW to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libPW
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp libPW.a ../../../Filesets.sl/BUILDENV/lib/libPW.a
	! chmog 0444 bin bin ../../../Filesets.sl/BUILDENV/lib/libPW.a
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libPW to the . relative fileset PROG-AUX
# change mode, owner, and group on /lib/libPW
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/PROG-AUX/lib
	! cp libPW.a ../../../Filesets.sl/PROG-AUX/lib/libPW.a
	! chmog 0444 bin bin ../../../Filesets.sl/PROG-AUX/lib/libPW.a
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/lib
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
