# $Source: /misc/source_product/9.10/commands.rcs/bin/ps/makefile,v $
# $Revision: 66.13.1.11 $            $Author: root $
# $State: Exp $               $Locker:  $
# $Date: 94/07/28 10:04:59 $
#
#
# Makefile for the 'ps' command, and other system-info related commands,
# such as top, monitor, w, uptime, and readmem.
#
# The command w, a link to uptime, is now installed by the Common Build
# process.  The command readmem is installed (by the Common Build) on
# the s300 only.
#
# The following defines list is for HP-UX 7.0.  Kernel developers who are
# compiling this file locally from un-unifdef'd include files may need to
# define these flags.  Also, it helps to put this source directory in
# ./TOOLS/ps/ underneath your main kernel directory, use the -I../.. option
# on compiles, define -DBUILDFROMH, and link the include files from ./h
# into ./sys.
#
# COMMDEFS = -DACLS -DAUDIT -DBSDDOMAIN -DBSDJOBCTL -DCDROM -DDOTDOT \
# 	-DDUX -DFSD_KI -DINET -DLAN -DLANRFA -DLOCKF -DLONGFILENAMES \
# 	-DNFS -DNFS3_2 -DNSCONFIG -DNSDIAG -DPOSIX -DPSTAT -DRFAIPCFIX -DSWFS \
# 	-DSYSCALLTRACE -D_HPUX_SOURCE -Dhpux -DW
# 
# DEFS800 = -Dhp9000s800 -D__hp9000s800 \
# 	-DAC_ON -DFPC_BUG -DFS_TUNE1 -DGENERIC -DICA_ON -DIDEMPOTENT_REQ \
# 	-DIODCIO -DKPREEMPT -DLAN_CIO -DLWSYSCALL -DMBTHRESHOLD -DMESG \
# 	-DNIO -DNOCOMPAT -DNOPAGING -DNS_KPREEMPT -DRDB -DSEMA -DSHMEM \
# 	-DSYMLINK -Dspectrum
# 
# DEFS300 = -Dhp9000s200 -Dhp9000s300 -D__hp9000s200 -D__hp9000s300 \
# 	-DBSDIPC -DCLEANUP_DEBUG -DCONFIG_HP200 -DDCTRACE -DHFS \
# 	-DKDB_ON_BREAK -DKERNEL_DEBUG_ONLY -DKERNEL_TUNE_DLB -DKERNEL_TUNE_FG \
# 	-DKERNEL_TUNE_SYSCALL -DMC68030 -DNENHANCE -DNLS -DNLS16 -DNS -DNSIPC \
# 	-DNS_NAMING_GLOBALS -DPROCESSLOCK -DPXP -DRTPRIO -DSOCKET -DTRACE \
# 	-DWOPR -DWRITE_GUARD_7_0 -UGPROF -ULOCAL_DISC -UNS_QA -UUMM -Uvax \
# 	-Wc,-Nd3500,-Ns3500
# 
# OBSOLETE_FLAGS = -DTEMPORARY -DBSDPORT -DITE
# 
# UNUSED_FLAGS = -DKERNEL

# "w" now installed by Common Build
MAKE_TARGET = top monitor uptime

SHELL=/bin/sh

SHARED_LIB = shared_lib.a

PREFIX = /lib/pcc_prefix.s
ASSEM = /bin/as
ASMFLAGS = 
CPP= /lib/cpp
# /usr/src/lib/libc/stub is used for include path for SYS.h for S800 dlstats.o
CPPFLAGS= -P -DAS -DLOCORE -D__hp9000s800 -I/usr/src/lib/libc/stub
SED= /bin/sed
SEDFLAGS= -e '/^[ 	]*$$/d' -e 's/^\./	\./' -e 's/; _/;/' -e 'y/; /\n	/' -e 's/ /	/'

GET_PRINFO = ./get_prinfo.o
FIND_TTYS = ./find_ttys.o

MONOFILES = monitor.o mode_x.o mode_c.o dlstats.o mode_help.o
OFILES = ps.o top.o uptime.o $(MONOFILES) readmem.o
CFILES = ps.c  top.c  uptime.c monitor.c mode_x.c mode_c.c mode_help.c

# Needed only by uptime.
CFLAGS_UPTIME = -DW

default: $(MAKE_TARGET) ps readmem

$(MAKE_TARGET):	$(SHARED_LIB) hp9000.h

$(SHARED_LIB):  $(FIND_TTYS)
	ar -r $@  $(FIND_TTYS)

$(GET_PRINFO): get_prinfo.c getpr.h hp9000.h

ps: ps.o
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) $@.o -o $@ $$libs

uptime: uptime.o
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) $@.o -o $@ $$libs

monitor: $(MONOFILES)
	if hp9000s300; then					  \
		$(CC) $(CFLAGS) $(MONOFILES) -o $@ -lcurses ;	  \
	elif hp9000s800; then					  \
		$(CC) $(CFLAGS) $(MONOFILES) -o $@ -lcurses -lIO ; \
 	fi;

top: top.o
	$(CC) $@.o -o $@ $(FIND_TTYS) -lm -lcurses

readmem: readmem.o
	$(CC) $@.o -o $@

uptime.o: uptime.c
	$(CC) $(CFLAGS_UPTIME) $< -c -o $@

mode_c.o: kdefs.h
monitor.o: kernelvars.h

kernelvars.h: kernelvars
	/bin/sh <$? >$@		# produce .h file from script

kdefs.h: kernelvars.h		# no nl[], just #defines
	grep '#define' <$? >$@

dlstats.o:  dlstats_300.s dlstats_800.s
	if hp9000s300; then				\
		$(CC) -c dlstats_300.s -o dlstats.o;	\
	elif hp9000s800; then				\
		$(CC) -c dlstats_800.s -o dlstats.o;	\
 	fi;

.c.o:
	$(CC) $(CFLAGS) -Wp,-H200000 -DBUILT_ON='"@(#)Built on: '`uname -r`'"' -c $< -o $@

clean:
	rm -f $(OFILES) $(SHARED_LIB) $(GET_PRINFO) $(FIND_TTYS) \
		kdefs.h kernelvars.h

clobber:  clean
	rm -f $(MAKE_TARGET) ps readmem

# copy /bin/ps to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/ps
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.

install:	ps
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp ps ../../../Filesets.sl/UX-CORE/bin
	! chmog 02555 bin sys ../../../Filesets.sl/UX-CORE/bin/ps
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/ps to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/ps
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp ps ../../../Filesets.sl/UX-CORE/bin/ps
	! chmog 02555 bin sys ../../../Filesets.sl/UX-CORE/bin/ps
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ps to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/ps
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp top ../../../Filesets.sl/CMDS-AUX/usr/bin/top
	! chmog 02555 bin sys ../../../Filesets.sl/CMDS-AUX/usr/bin/top
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ps to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/ps
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp uptime ../../../Filesets.sl/CMDS-AUX/usr/bin/uptime
	! chmog 02555 bin sys ../../../Filesets.sl/CMDS-AUX/usr/bin/uptime
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ps to the . relative fileset USRCONTRB
# change mode, owner, and group on /bin/ps
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! cp monitor ../../../Filesets.sl/USRCONTRB/usr/contrib/bin/monitor
	! chmog 04555 root bin ../../../Filesets.sl/USRCONTRB/usr/contrib/bin/monitor
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ps to the . relative fileset USRCONTRB
# change mode, owner, and group on /bin/ps
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! cp readmem ../../../Filesets.sl/USRCONTRB/usr/contrib/bin/readmem
	! chmog 02555 bin sys ../../../Filesets.sl/USRCONTRB/usr/contrib/bin/readmem
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/bin
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ps to the . relative fileset USRCONTRB
# change mode, owner, and group on /bin/ps
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/USRCONTRB/usr/contrib/lib
	! cp monitor.help ../../../Filesets.sl/USRCONTRB/usr/contrib/lib/monitor.help
	! chmog 0444 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/lib/monitor.help
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/lib
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ps to the . relative fileset USRCONTRB
# change mode, owner, and group on /bin/ps
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/USRCONTRB/usr/contrib/man/man1m
	! cp monitor.1m ../../../Filesets.sl/USRCONTRB/usr/contrib/man/man1m/monitor.1m
	! chmog 0444 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/man/man1m/monitor.1m
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/man/man1m
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/man/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/contrib/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/usr/
	! chmog 555 bin bin ../../../Filesets.sl/USRCONTRB/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ps to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/ps
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/usr/bin
	! ln ../../../Filesets.sl/CMDS-AUX/usr/bin/uptime ../../../Filesets.sl/CMDS-AUX/usr/bin/w
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
