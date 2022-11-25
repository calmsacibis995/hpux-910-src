# @(#) $Revision: 70.8 $
#
# $Source: /misc/source_product/9.10/commands.rcs/bin/as/makefile,v $
#
# Makefile to build as (used to be as20, but as10 is
# no longer supported as of 8.0 so as20 has become as)
#

WFLGS = -Wc,-Yu 

# defs to set the processor

DEBUG =
DIRS = -I/usr/contrib/include
DEFS = -DZDNTT -DSDOPT -DLISTER2 -DPIC -DOFORTY -DCPLUSPLUS -DTEMPLATES
PROCFLAGS = -DM68020 -DM68881 -DM68851 -DDRAGON
CCOMFLG = -Wc,-Nd1500 -Wc,-Nw400
FLAGS = $(DEFS) $(CCOMFLG) $(PROCFLAGS) $(DIRS)
COMMAND = as
RCOMMAND = ras

YFLAGS = -dl -Xp./yaccpar.as

OBJECTS = asgram.o ea.o icode.o instr.o pass0.o pass1.o scan.o \
	  symbols.o symtabs.o pass2.o list.o \
	  misc.o error.o header.o debug.o sdopt.o dragon.o \
	  iops.o match.o vtables.o

#
# GENFILES: files generated as part of the make (in addition to .o)
#
GENFILES = ivalues.h genmatch genmatch.o \
	   match.c asgram.c asgram.h iops.c
VERSION  = hpux_rel.o
INCLUDES = asgram.h adrmode.h align.h opfmts.h pass0.h symbols.h \
	   ivalues.h verify.h icode.h  bitmask.h sdopt.h header.h


default: all

all:  $(COMMAND) $(RCOMMAND)

$(COMMAND):  $(OBJECTS) $(VERSION)
	$(CC) $(DEBUG) $(LDFLAGS) -o $@ $(VERSION) $(OBJECTS) $(LIBS)
	echo "Make of $@ complete"

$(RCOMMAND):  $(OBJECTS) $(VERSION)
	$(CC) $(DEBUG) $(LDFLAGS) -o $@ $(VERSION) $(OBJECTS) $(LIBS) \
		-Wl,-a,archive
	echo "Make of $@ complete"

$(OBJECTS):  $(INCLUDES)

icode.o pass2.o ea.o:	fixup.h

asgram.h:	asgram.y
	yacc $(YFLAGS) asgram.y
	mv y.tab.h asgram.h
	mv y.tab.c asgram.c

asgram.c:	asgram.y
	yacc $(YFLAGS) asgram.y
	mv y.tab.h asgram.h
	mv y.tab.c asgram.c

asgram.o:	asgram.h asgram.c
	$(CC) $(CFLAGS) $(FLAGS) $(WFLGS) +O1 -c asgram.c

ivalues.h:	pairs.dat gendef.awk
	/lib/cpp -P $(DEFS) pairs.dat | awk -f gendef.awk >ivalues.h

iops.c:		pairs.dat genops.awk
	/lib/cpp -P $(DEFS) pairs.dat | awk -f genops.awk >iops.c

match.c:	pairs.dat ./genmatch
	./genmatch >match.c

./genmatch:	genmatch.o vtables.o
	$(CC) -Wl,-a,archive genmatch.o vtables.o -o genmatch

genmatch.o:	ivalues.h verify.h

clean:
	/bin/rm -f $(OBJECTS) $(VERSION)
	/bin/rm -f $(GENFILES)

clobber: clean
	/bin/rm -f $(COMMAND)

#
# Re-define the .c.o rule so implicit compiles will use FLAGS as well as
# CFLAGS
#

.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

# copy /bin/as to the . relative fileset BUILDENV
# change mode, owner, and group on /bin/as
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	as
	mkdir -p ../../../Filesets.sl/BUILDENV/bin
	! cp as ../../../Filesets.sl/BUILDENV/bin
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/bin/as
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/bin
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
# copy /bin/as to the . relative fileset BUILDENV
# change mode, owner, and group on /bin/as
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/bin
	! cp as ../../../Filesets.sl/BUILDENV/bin/as
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/bin/as
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/bin
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/as to the . relative fileset C-MIN
# change mode, owner, and group on /bin/as
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/bin
	! cp as ../../../Filesets.sl/C-MIN/bin/as
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/bin/as
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/as to the . relative fileset BUILDENV
# change mode, owner, and group on /bin/as
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/bin
	! ln ../../../Filesets.sl/BUILDENV/bin/as ../../../Filesets.sl/BUILDENV/bin/as20
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/bin
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/as to the . relative fileset C-MIN
# change mode, owner, and group on /bin/as
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/bin
	! ln ../../../Filesets.sl/C-MIN/bin/as ../../../Filesets.sl/C-MIN/bin/as20
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
