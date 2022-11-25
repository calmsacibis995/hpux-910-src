# $Source: /misc/source_product/9.10/commands.rcs/lib/c2/makefile,v $
# $Revision: 66.5 $

#
# Makefile for c2 for HP9000 S200/S300
#
# This is the makefile for /lib/c2, the assembler-source optimizer.
#
# Interesting things to do are:
#
# make 			Makes the 68020 optimizer (c2)


COMMAND = c2
FLAGS = -DM68020 -DDRAGON -DVOLATILE -DPIC
LOADFLGS = -q

OFILES= alloc.o bits.o cache.o canon.o cc.o collapse.o \
	exit.o flow.o input.o main.o misc.o movem.o \
	options.o output.o print.o rmove.o span.o tidy.o ultimate.o zero.o \
	sched.o sched2.o time30.o time40.o hpux_rel.o

SOURCES= $(OFILES:.o=.c)

default: $(COMMAND)

$(COMMAND):	$(OFILES) print.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(FLAGS) $(LOADFLGS) \
			-o $@ $(OFILES) -lmalloc

$(OFILES): o68.h opcodes.h sched.h

print.o: print.s
	as20 -o print.o print.s

clean:
	rm -f $(OFILES)

clobber: clean
	rm -f $(COMMAND)

#
# Re-define the .c.o rule so implicit compiles will use FLAGS
#

.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<
# copy /lib/c2 to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/c2
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp c2 ../../../Filesets.sl/BUILDENV/lib/c.c2
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/c.c2
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/c2 to the . relative fileset C
# change mode, owner, and group on /lib/c2
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C/lib
	! cp c2 ../../../Filesets.sl/C/lib/c.c2
	! chmog 0555 bin bin ../../../Filesets.sl/C/lib/c.c2
	! chmog 555 bin bin ../../../Filesets.sl/C/lib
	! chmog 555 bin bin ../../../Filesets.sl/C/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/c2 to the . relative fileset FORTRAN-PRG
# change mode, owner, and group on /lib/c2
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/FORTRAN-PRG/lib
	! cp c2 ../../../Filesets.sl/FORTRAN-PRG/lib/fc2
	! chmog 0555 bin bin ../../../Filesets.sl/FORTRAN-PRG/lib/fc2
	! chmog 555 bin bin ../../../Filesets.sl/FORTRAN-PRG/lib
	! chmog 555 bin bin ../../../Filesets.sl/FORTRAN-PRG/
	! chmog 555 bin bin ../../../Filesets.sl/
