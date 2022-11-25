# @(#) $Revision: 72.16 $
#
# S300 makefile for compilers directory
#

# Outer level makefile to build ccom, cpass1, cpass2, ccom.ansi,
# cpass1.ansi and c1. This file will build those pieces by invoking the
# sub-makefile ccom.make with appropriate defines and the PROCFLAGS.

PFLG20     = -DFASTMATCH -DDBG_FMT -DOLDSWITCH -DSA -DSA_MACRO -ULCD \
	     -DR2REGS -DC1 -DC1SUPPORT -DONEPASS -UC1_C -UNOCDB -UANSI
PFLG20_ANSI= -DFASTMATCH -DDBG_FMT -DOLDSWITCH -DSA -DSA_MACRO -ULCD \
	     -DR2REGS -DC1 -DC1SUPPORT -DONEPASS -UC1_C -UNOCDB -DANSI \
             -DQUADC

SPLITC     = -DNEWC0SUPPORT -DFASTMATCH -DOLDSWITCH -ULCD -DR2REGS \
	     -DC1 -DC1SUPPORT -UONEPASS -DC1_C -DNOCDB -UANSI
SPLITC_ANSI= -DNEWC0SUPPORT -DFASTMATCH -DOLDSWITCH -ULCD -DR2REGS \
	     -DC1 -DC1SUPPORT -UONEPASS -DC1_C -DNOCDB -DANSI -DQUADC
XXCFLAGS   = -O -UDEBUGGING -Ucookies -Dhp9000s200 -Dxcomp300_800 \
	     -Dprntf=printf -Dfprntf=fprintf -Dsprntf=sprintf \
	     -DSLOW_WALKF

default:	c.c0 c.c1 ccom cpass1 cpass2 ccom.ansi cpass1.ansi

c.c0:
	cd c0; $(MAKE) default

c.c1:
	cd c1;	$(MAKE) default

ccom:
	[ ! -d D.$@ ] && mkdir D.$@ || exit 0
	$(MAKE) -f ccom.make COMMAND=$@ D=D.$@ \
	    PROCFLAGS="$(PFLG20)" ANSIFLAGS=-UANSI $@

ccom.ansi:
	[ ! -d D.$@ ] && mkdir D.$@ || exit 0
	$(MAKE) -f ccom.make COMMAND=$@ D=D.$@ \
	    PROCFLAGS="$(PFLG20_ANSI)" ANSIFLAGS=-DANSI $@

cpass1:
	[ ! -d D.$@ ] && mkdir D.$@ || exit 0
	$(MAKE) -f ccom.make COMMAND=$@ D=D.$@ \
	    PROCFLAGS="$(SPLITC)" ANSIFLAGS=-UANSI $@

cpass1.ansi:
	[ ! -d D.$@ ] && mkdir D.$@ || exit 0
	$(MAKE) -f ccom.make COMMAND=$@ D=D.$@ \
	    PROCFLAGS="$(SPLITC_ANSI)" ANSIFLAGS=-DANSI $@

cpass2:
	[ ! -d D.$@ ] && mkdir D.$@ || exit 0
	$(MAKE) -f ccom.make COMMAND=$@ D=D.$@ \
	    PROCFLAGS="$(SPLITC)" ANSIFLAGS=-UANSI $@

c1opt:
	cd c1; $(MAKE)

clean:
	for targ in ccom ccom.ansi cpass1 cpass1.ansi cpass2; do    \
	    $(MAKE) -f ccom.make D=D.$$targ COMMAND=$$targ clean;   \
	done
	cd c1; $(MAKE) clean

clobber:
	for targ in ccom ccom.ansi cpass1 cpass1.ansi cpass2; do    \
	    $(MAKE) -f ccom.make D=D.$$targ COMMAND=$$targ clobber; \
	done
	cd c0; $(MAKE) clobber
	cd c1; $(MAKE) clobber
	rm -f kerncom

xcomp: xccom

xccom:
	$(MAKE) -f ccom.make  COMMAND=ccom PROCFLAGS="$(PFLG20)" OBJ3="" XCFLAGS="$(XXCFLAGS)"  LINC="." clobber ccom

# copy /lib/compilers.300 to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp ccom ../../../Filesets.sl/BUILDENV/lib/ccom
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/ccom
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset C-MIN
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/lib
	! cp ccom ../../../Filesets.sl/C-MIN/lib/ccom
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/lib/ccom
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/lib
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp ccom.ansi ../../../Filesets.sl/BUILDENV/lib/ccom.ansi
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/ccom.ansi
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset C
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C/lib
	! cp ccom.ansi ../../../Filesets.sl/C/lib/ccom.ansi
	! chmog 0555 bin bin ../../../Filesets.sl/C/lib/ccom.ansi
	! chmog 555 bin bin ../../../Filesets.sl/C/lib
	! chmog 555 bin bin ../../../Filesets.sl/C/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp cpass1 ../../../Filesets.sl/BUILDENV/lib/cpass1
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/cpass1
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset C
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C/lib
	! cp cpass1 ../../../Filesets.sl/C/lib/cpass1
	! chmog 0555 bin bin ../../../Filesets.sl/C/lib/cpass1
	! chmog 555 bin bin ../../../Filesets.sl/C/lib
	! chmog 555 bin bin ../../../Filesets.sl/C/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp cpass1.ansi ../../../Filesets.sl/BUILDENV/lib/cpass1.ansi
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/cpass1.ansi
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset C
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C/lib
	! cp cpass1.ansi ../../../Filesets.sl/C/lib/cpass1.ansi
	! chmog 0555 bin bin ../../../Filesets.sl/C/lib/cpass1.ansi
	! chmog 555 bin bin ../../../Filesets.sl/C/lib
	! chmog 555 bin bin ../../../Filesets.sl/C/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp cpass2 ../../../Filesets.sl/BUILDENV/lib/cpass2
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/cpass2
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/compilers.300 to the . relative fileset C
# change mode, owner, and group on /lib/compilers.300
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C/lib
	! cp cpass2 ../../../Filesets.sl/C/lib/cpass2
	! chmog 0555 bin bin ../../../Filesets.sl/C/lib/cpass2
	! chmog 555 bin bin ../../../Filesets.sl/C/lib
	! chmog 555 bin bin ../../../Filesets.sl/C/
	! chmog 555 bin bin ../../../Filesets.sl/
# lib/c.c0 and lib/c.c1
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! cp c0/c0 ../../../Filesets.sl/BUILDENV/lib/c.c0
	! cp c1/c1 ../../../Filesets.sl/BUILDENV/lib/c.c1
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/c.c0
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/c.c1
	mkdir -p ../../../Filesets.sl/C/lib
	! chmog 555 bin bin ../../../Filesets.sl/C/lib
	! cp c0/c0 ../../../Filesets.sl/C/lib/c.c0
	! cp c1/c1 ../../../Filesets.sl/C/lib/c.c1
	! chmog 0555 bin bin ../../../Filesets.sl/C/lib/c.c0
	! chmog 0555 bin bin ../../../Filesets.sl/C/lib/c.c1
	mkdir -p ../../../Filesets.sl/FORTRAN-PRG/lib
	! chmog 555 bin bin ../../../Filesets.sl/FORTRAN-PRG/lib
	! cp c1/c1 ../../../Filesets.sl/FORTRAN-PRG/lib/fc1
	! chmog 555 bin bin ../../../Filesets.sl/FORTRAN-PRG/lib/fc1
	! chmog 555 bin bin ../../../Filesets.sl/FORTRAN-PRG
	! chmog 555 bin bin ../../../Filesets.sl
