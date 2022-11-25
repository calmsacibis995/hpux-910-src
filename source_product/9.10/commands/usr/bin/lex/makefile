# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/lex/makefile,v $
# $Revision: 70.2.1.9 $

# Common makefile for command lex.

OBJECTS = main.o sub1.o sub2.o subnls.o subnls2.o header.o parser.o msgs.o
RELEASE = hpux_rel.o

# Commented out the following line because, for official Core Commands
# and Libraries builds, "-DNLS -DNLS16 and -O" are all among the
# defaults.  Build admin types want to be able to control such "generic"
# options from the build configuration file.  Did not mess with the
# "osf" target as that's not my bailiwick.
#
# CCFLAGS = -DNLS -DNLS16 -O

# "make" or "make default" builds HP-UX version

default: 	lex lex.cat

# "make osf" builds OSF version (no NLS16 support)

osf:
	make CCFLAGS="-DNLS -DOSF -O"

lex:	$(OBJECTS) $(RELEASE)
	cc -o lex $(CCFLAGS)  $(OBJECTS) $(RELEASE)

$(OBJECTS): ldefs.c
main.o : main.c once.c ldefs.c 
	cc -c main.c $(CCFLAGS)

sub1.o : sub1.c ldefs.c 
	cc -c sub1.c $(CCFLAGS)

sub2.o : sub2.c ldefs.c
	cc -c sub2.c $(CCFLAGS)

subnls.o : subnls.c ldefs.c
	cc -c subnls.c $(CCFLAGS)

subnls2.o : subnls2.c ldefs.c
	cc -c subnls2.c $(CCFLAGS)

header.o : header.c ldefs.c
	cc -c header.c $(CCFLAGS)

hpux_rel.o: hpux_rel.c
	cc -c hpux_rel.c $(CCFLAGS)

parser.o: parser.c ldefs.c
	cc -c parser.c $(CCFLAGS)

parser.c: parser.y 
	yacc parser.y
	mv y.tab.c parser.c

msgs.o : msgs.c msgs.h
	cc -c msgs.c $(CCFLAGS)


lex.cat : lex.cat.in
	gencat lex.cat lex.cat.in 
	


clean:
	/bin/rm -f *.o parser.c lex.cat

clobber:  clean
	/bin/rm -f lex

# copy /usr/bin/lex to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lex
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	lex
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/bin
	! cp lex ../../../../Filesets.sl/BUILDENV/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin/lex
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
# copy /usr/bin/lex to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lex
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/bin
	! cp lex ../../../../Filesets.sl/BUILDENV/usr/bin/lex
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin/lex
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lex to the . relative fileset C-AUX
# change mode, owner, and group on /usr/bin/lex
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/bin
	! cp lex ../../../../Filesets.sl/C-AUX/usr/bin/lex
	! chmog 0555 bin bin ../../../../Filesets.sl/C-AUX/usr/bin/lex
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lex to the . relative fileset C-AUX
# change mode, owner, and group on /usr/bin/lex
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/lib/nls/C
	! cp lex.cat ../../../../Filesets.sl/C-AUX/usr/lib/nls/C/lex.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/C/lex.cat
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lex to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/bin/lex
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/bin
	! cp lex ../../../../Filesets.sl/BUILDENV/usr/bin/lex
	! chmog 0555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin/lex
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lex to the . relative fileset C-AUX
# change mode, owner, and group on /usr/bin/lex
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/bin
	! cp lex ../../../../Filesets.sl/C-AUX/usr/bin/lex
	! chmog 0555 bin bin ../../../../Filesets.sl/C-AUX/usr/bin/lex
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lex to the . relative fileset C-AUX
# change mode, owner, and group on /usr/bin/lex
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/lib/nls/C
	! cp lex.cat ../../../../Filesets.sl/C-AUX/usr/lib/nls/C/lex.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/C/lex.cat
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
