#
# @(#) $Revision: 70.7.1.6 $
# $Source: /misc/source_product/9.10/commands.rcs/lib/cpp.ansi/makefile,v $
#
# Common makefile for ANSI cpp
#
# This makefile makes three targets, cpp.ansi, libcpp.a, and libcppxl.a
# The object files are built differently, so we rename the objects
# that go into the libcpp.a file as object_a.o instead of object.o
#
# This allows the two makes to be done simultaneously without
# confusing the .o files
#

SHELL=/bin/sh
LIBFLAGS = -DNLS -DOPT_INCLUDE
CMDFLAGS = -DNLS -DCXREF -DOPT_INCLUDE
BADUMPO=/usr/local/lib/badump.o
BACPP=/usr/local/lib/bacpp
LIBP2=

SOURCES = main.c startup.c define.c substitute.c support.c if.c \
	error.c readline.c file.c iflex.c ifgram.y memmove.c support.h file.h \
	if.h define.h hpux_rel.c

BASOURCES = BA/main.c BA/startup.c BA/define.c BA/substitute.c \
	BA/support.c BA/if.c BA/error.c BA/readline.c BA/file.c \
	BA/iflex.c BA/ifgram.y BA/memmove.c BA/support.h BA/file.h \
	BA/if.h BA/define.h BA/hpux_rel.c

OBJECTS = startup.o define.o substitute.o support.o if.o error.o \
	readline.o file.o iflex.o ifgram.o memmove.o hpux_rel.o

OBJECTS_XL = startup.o define.o substitute.o support.o if.o error.o \
	readline.o file.o iflex.o ifgram.o memmove.o main.o

LIBOBJECTS = $(OBJECTS:.o=_a.o)

LIBOBJECTS_XL = $(OBJECTS_XL:.o=_a.o)

default: cpp.ansi libcpp.a

cpp.ansi: $(OBJECTS) main.o
	$(CC) $(CFLAGS) $(OBJECTS) main.o $(LIBP2) -o cpp.ansi

libcpp.a: $(LIBOBJECTS)
	@rm -f libcpp.a
	ar qc libcpp.a $(LIBOBJECTS)

libcppxl.a: $(LIBOBJECTS_XL)
	@rm -f libcppxl.a
	ar qc libcppxl.a $(LIBOBJECTS_XL)

libcppcmd.a: $(OBJECTS)
	@rm -f libcppcmd.a
	ar qc libcppcmd.a $(OBJECTS)

ifgram.o: ifgram.c y.tab.h
	if [ -x /bin/hp9000s300 ] && /bin/hp9000s300; then  \
	$(CC) $(CFLAGS) $(CMDFLAGS) +O1 -c ifgram.c;        \
	else                                                \
	$(CC) $(CFLAGS) $(CMDFLAGS) -c ifgram.c;            \
	fi;

ifgram_a.o: ifgram.c y.tab.h
	if [ -x /bin/hp9000s300 ] && /bin/hp9000s300; then  \
	$(CC) $(CFLAGS) $(LIBFLAGS) +O1 -c ifgram.c -o $@;  \
	else                                                \
	$(CC) $(CFLAGS) $(LIBFLAGS) -c ifgram.c -o $@;      \
	fi;

ifgram.c y.tab.h: ifgram.y
	yacc -d ifgram.y
	sed "s/yy/zz/g" y.tab.c > ifgram.c
	rm -f y.tab.c

clean:
	@echo "\tRemoving all object files"
	@/bin/rm -f $(OBJECTS) main.o ifgram.c y.tab.h
	@/bin/rm -f $(LIBOBJECTS)

clobber: clean
	@echo "\tRemoving cpp.ansi, libcpp.a, and libcppcmd.a"
	@/bin/rm -f cpp.ansi libcpp.a libcppcmd.a

#####################################################################
# Re-define .c.o rule so implicit compiles will use the correct FLAGS
#####################################################################
.SUFFIXES:
.SUFFIXES: .o .s .c _a.o

.c.o:
	$(CC) $(CFLAGS) $(CMDFLAGS) -c $<


.c_a.o:
	$(CC) $(CFLAGS) $(LIBFLAGS) -c $< -o $@

#########################
# Branch analysis version
#########################

ba: BA/bacpp

BA/bacpp : baobjects
	cd BA;\
	rm -f ba.o;\
	makeba *.o;\
	cc -c ba.c -o ba.o;\
	cc $(OBJECTS) main.o ba.o $(BADUMPO) $(LIBP2) -o bacpp

baobjects: $(BASOURCES)
	cd BA;\
	make -f ../makefile "CFLAGS=-t p,$(BACPP) -DBA_OPTA" objects;

$(BASOURCES) : $$(@F)
	cp $? $@
	chmod +w $@

##########################
# Source file dependencies
##########################

main.o: main.c support.h file.h

startup.o startup_a.o: startup.c support.h file.h

define.o define_a.o: define.c support.h define.h

substitute.o substitute_a.o: substitute.c support.h define.h file.h

support.o support_a.o: support.c support.h

if.o if_a.o: if.c support.h

error.o error_a.o: error.c support.h file.h

readline.o readline_a.o: readline.c support.h if.h file.h

file.o file_a.o: file.c support.h file.h

iflex.o iflex_a.o: iflex.c support.h y.tab.h

memmove.o memmove_a.o: memmove.c

hpux_rel.o hpux_rel_a.o: hpux_rel.c

# copy /lib/cpp.ansi to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/cpp.ansi
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cpp.ansi
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp cpp.ansi ../../../Filesets.sl/BUILDENV/lib
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/cpp.ansi
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
# copy /lib/cpp.ansi to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/cpp.ansi
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp cpp.ansi ../../../Filesets.sl/BUILDENV/lib/cpp.ansi
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/cpp.ansi
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/cpp.ansi to the . relative fileset C-AUX
# change mode, owner, and group on /lib/cpp.ansi
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-AUX/lib
	! cp cpp.ansi ../../../Filesets.sl/C-AUX/lib/cpp.ansi
	! chmog 0555 bin bin ../../../Filesets.sl/C-AUX/lib/cpp.ansi
	! chmog 555 bin bin ../../../Filesets.sl/C-AUX/lib
	! chmog 555 bin bin ../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
