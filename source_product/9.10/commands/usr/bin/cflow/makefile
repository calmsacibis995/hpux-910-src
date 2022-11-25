# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/cflow/makefile,v $
# $Revision: 70.3.1.10 $

# Common makefile for cflow

#LIBS = -lmalloc
LIBS = /usr/lib/libmalloc.a
FLAGS = -DFLEXNAMES -DAPEX

# This assumes the directory that contains lint and its associated files is
# in shared source (at the same level of the source tree).
LINT = ../lint

UNIFDEFFLAGS = -UOSF
#
# The cflow makefile depends on compiler tools.  The s800 used to have a
# directory (/usr/src/cmd/mip) that was a snapshot of AT&T pcc compiler
# source.  It now uses the /src/lib/compilers.300 directory that has
# the same AT&T heritage, but has been modified for ANSI-C support.
#
MIP=../../../lib/compilers.300

default: all

all:	cflow dag lpfx nmf flip

cflow:	cflow.sh
	-unifdef $(UNIFDEFFLAGS) cflow.sh > cflow

dag:	dag.c
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) dag.c -o dag

lpfx:	lpfx.c $(LINT)/lerror.h $(LINT)/lmanifest $(LINT)/lpass2.h \
	$(MIP)/manifest $(MIP)/apex.h
	$(CC) $(CFLAGS) $(FLAGS) -I$(LINT) -I$(MIP) $(LDFLAGS) -o lpfx lpfx.c $(LIBS)

nmf:	nmf.c
	$(CC) $(CFLAGS) $(FLAGS) nmf.c $(LDFLAGS) -o nmf

flip:	flip.c
	$(CC) $(CFLAGS) $(FLAGS) flip.c $(LDFLAGS) -o flip

clean:
	/bin/rm -f *.o a.out make.out core

clobber:	clean
	/bin/rm -f cflow dag lpfx nmf flip

# copy /usr/bin/cflow to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cflow
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cflow
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp cflow ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/cflow
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
# copy /usr/bin/cflow to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cflow
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp cflow ../../../../Filesets.sl/C-TOOLS/usr/bin/cflow
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/cflow
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/cflow to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cflow
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp dag ../../../../Filesets.sl/C-TOOLS/usr/lib/dag
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/dag
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/cflow to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cflow
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp flip ../../../../Filesets.sl/C-TOOLS/usr/lib/flip
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/flip
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/cflow to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cflow
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp lpfx ../../../../Filesets.sl/C-TOOLS/usr/lib/lpfx
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/lpfx
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/cflow to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/cflow
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/lib
	! cp nmf ../../../../Filesets.sl/C-TOOLS/usr/lib/nmf
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib/nmf
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
