# @(#) $Revision: 72.53 $
#
# Common makefile for uucp (HoneyDanBer)
#
FINDMSG = /usr/bin/findmsg
GENCAT = /usr/bin/gencat

FLAGS = -DUUSUB	 -I./dkit/include		# for normal case
CFLAGS =
UUGETTY=uugetty
UUGETTY_M=uugetty.M
LIBDKFLAGS=-L ./dkit/lib -ldk
INCLUDEDIR=./dkit/include

X25=$(ROOT)/usr/lib/uucp/X25
DIALDIR=$(ROOT)/usr/lib/uucp/newconfig
PROTOCOLS=dio.o
LIBDK=libdk.a

LOCKS=$(ROOT)/usr/spool/locks
PUBDIR=$(ROOT)/usr/spool/uucppublic
SPOOL=$(ROOT)/usr/spool/uucp
#		things are linked from SPOOL into XQTDIR and CORRUPT
XQTDIR=	$(SPOOL)/.Xqtdir
#		this is optional
CORRUPT=$(SPOOL)/.Corrupt
#		for logfiles
LOGDIR=$(SPOOL)/.Log
#		for sequence number files
SEQDIR=$(SPOOL)/.Sequence
#		for STST files
STATDIR=$(SPOOL)/.Status
LOGUUCP=$(LOGDIR)/uucp
LOGUUX=$(LOGDIR)/uux
LOGUUXQT=$(LOGDIR)/uuxqt
LOGCICO=$(LOGDIR)/uucico
WORKSPACE=$(SPOOL)/.Workspace
ADMIN=$(SPOOL)/.Admin
#		for saving old log files
OLDLOG=$(SPOOL)/.Old
#		maximum number of uuxqts at one file in this file
MAXUUXQTS=Maxuuxqts

#		Maximum number of uuscheds at one time
MAXUUSCHEDS=Maxuuscheds


COMMANDS=uucp uux uuxqt uucico uusched uuls uuclean uusub \
	 uusnap getx25 opx25 clrsvc dialit
COMMANDS_M=uucp.M uux.M uuxqt.M uucico.M uusched.M uuls.M uuclean.M uusub.M \
	 uusnap.M getx25.M opx25.M clrsvc.M dialit.M
TOOLS=uuname uucheck uustat uucleanup $(UUGETTY)
TOOLS_M=uuname.M uucheck.M uustat.M uucleanup.M $(UUGETTY_M)
OFILES=utility.o cpmv.o expfile.o gename.o getpwinfo.o \
	ulockf.o xqt.o logent.o versys.o gnamef.o systat.o \
	$(GETOPT)
LFILES=utility.c cpmv.c expfile.c gename.c getpwinfo.c \
	ulockf.c xqt.c logent.c versys.c gnamef.c systat.c \
	$(GETOPTSRC)
OUUCP=uucpdefs.o uucp.o gwd.o permission.o getargs.o uucpname.o \
	versys.o gtcfile.o $(STRPBRK) chremdir.o
LUUCP=uucpdefs.c uucp.c gwd.c permission.c getargs.c uucpname.c \
	versys.c gtcfile.c $(STRPBRKSRC) chremdir.c
OUUX=uucpdefs.o uux.o gwd.o permission.o getargs.o getprm.o \
	uucpname.o versys.o gtcfile.o chremdir.o $(STRPBRK)
LUUX=uucpdefs.c uux.c gwd.c permission.c getargs.c getprm.c \
	uucpname.c versys.c gtcfile.c chremdir.c $(STRPBRKSRC)
OUUXQT=uucpdefs.o uuxqt.o mailst.o getprm.o gnamef.o uucpname.o \
	permission.o getargs.o gtcfile.o $(STRPBRK) \
	shio.o chremdir.o
LUUXQT=uucpdefs.c uuxqt.c mailst.c getprm.c gnamef.c uucpname.c \
	permission.c getargs.c gtcfile.c $(STRPBRKSRC) \
	shio.c chremdir.c
OUUCICO=uucpdefs.o cico.o conn.o callers.o cntrl.o pk0.o pk1.o \
	fio.o gio.o xio.o anlwrk.o permission.o getargs.o gnamef.o \
	gnxseq.o pkdefs.o imsg.o gtcfile.o ub_sst.o \
	mailst.o uucpname.o line.o chremdir.o $(STRPBRK) $(PROTOCOLS)
LUUCICO=uucpdefs.c cico.c conn.c callers.c cntrl.c pk0.c pk1.c \
	fio.c gio.c xio.c anlwrk.c permission.c getargs.c gnamef.c \
	gnxseq.c pkdefs.c imsg.c gtcfile.c ub_sst.c \
	mailst.c uucpname.c line.c chremdir.c $(STRPBRKSRC) eio.c dio.c
OUUNAME=uuname.o uucpname.o uucpdefs.o getpwinfo.o
LUUNAME=uuname.c uucpname.c uucpdefs.c getpwinfo.c
OUUSTAT=uustat.o gnamef.o expfile.o uucpdefs.o getpwinfo.o ulockf.o \
	getargs.o utility.o uucpname.o
LUUSTAT=uustat.c gnamef.c expfile.c uucpdefs.c getpwinfo.c ulockf.c \
	getargs.c utility.c uucpname.c
OUUSCHED=uusched.o gnamef.o expfile.o uucpdefs.o getpwinfo.o ulockf.o \
	systat.o getargs.o utility.o
LUUSCHED=uusched.c gnamef.c expfile.c uucpdefs.c getpwinfo.c ulockf.c \
	systat.c getargs.c utility.c
OUUCLEANUP=uucleanup.o gnamef.o expfile.o uucpdefs.o getpwinfo.o \
	uucpname.o ulockf.o getargs.o cpmv.o utility.o
LUUCLEANUP=uucleanup.c gnamef.c expfile.c uucpdefs.c getpwinfo.c \
	uucpname.c ulockf.c getargs.c cpmv.c utility.c
OUUGETTY=uugetty.o ulockf.o uucpdefs.o
LUUGETTY=uugetty.c ulockf.c uucpdefs.c
OUULS=uuls.o
LUULS=uuls.c
OUUSNAP=uusnap.o uucpdefs.o
LUUSNAP=uusnap.c uucpdefs.c
LUUCLEAN=uuclean.c uucpdefs.c gnamef.c mailst.c getpwinfo.c \
	 getargs.c utility.c systat.c cpmv.c logent.c rkill.c \
	 ulockf.c
OUUCLEAN=uuclean.o uucpdefs.o gnamef.o mailst.o getpwinfo.o \
	 getargs.o utility.o systat.o cpmv.o logent.o rkill.o \
	 ulockf.o
LUUSUB=uusub.c ub_sst.c uucpdefs.c xqt.c getpwinfo.c ulockf.c \
	utility.c logent.c systat.c getargs.c
OUUSUB= uucpdefs.o uusub.o ub_sst.o uucpdefs.o getpwinfo.o ulockf.o \
	xqt.o utility.o logent.o systat.o getargs.o
OGETX25=getx25.o
LGETX25=getx25.c
OOPX25=opx25.o
LOPX25=opx25.c
OCLRSVC=clrsvc.o
LCLRSVC=clrsvc.c
INIT=init
LIBDKFILES = dkit/lib/dkbreak.c dkit/lib/dkcanon.c dkit/lib/dkdial.c \
	dkit/lib/dkeof.c dkit/lib/dkerr.c dkit/lib/dkexec.c \
	dkit/lib/dkmgr.c dkit/lib/dkminor.c dkit/lib/dknamer.c \
	dkit/lib/dksplice.c dkit/lib/dksplwait.c dkit/lib/dkxenviron.c \
	dkit/lib/dkxstdio.c dkit/lib/dkxwrite.c dkit/lib/dtnamer.c \
	dkit/lib/dxnamer.c dkit/lib/maphost.c dkit/lib/pull.c \
	dkit/lib/pupucommon.c dkit/lib/push.c
OLIBDKFILES = dkit/lib/dkbreak.o dkit/lib/dkcanon.o dkit/lib/dkdial.o \
	dkit/lib/dkeof.o dkit/lib/dkerr.o dkit/lib/dkexec.o \
	dkit/lib/dkmgr.o dkit/lib/dkminor.o dkit/lib/dknamer.o \
	dkit/lib/dksplice.o dkit/lib/dksplwait.o dkit/lib/dkxenviron.o \
	dkit/lib/dkxstdio.o dkit/lib/dkxwrite.o dkit/lib/dtnamer.o \
	dkit/lib/dxnamer.o dkit/lib/maphost.o dkit/lib/pull.o \
	dkit/lib/pupucommon.o dkit/lib/push.o

default: all

metrics: $(COMMANDS_M) $(TOOLS_M)

all: $(INIT) dkit/lib/$(LIBDK) $(COMMANDS) $(TOOLS) uucp.cat

clean:
	/bin/rm -f *.o *.msg dkit/*/*.o

clobber:	clean
	/bin/rm -f $(COMMANDS) $(TOOLS) *.cat dkit/lib/libdk.a

#
# The "burn" and "cmp" targets are leftover from previous versions of
# this makefile.
#
burn:
	/bin/rm -f $(BIN)/OLDuucp
	/bin/rm -f $(BIN)/OLDuux
	/bin/rm -f $(INSDIR)/OLDuuxqt
	/bin/rm -f $(INSDIR)/OLDuucico
	/bin/rm -f $(BIN)/OLDuuname
	/bin/rm -f $(INSDIR)/OLDuucheck
	/bin/rm -f $(INSDIR)/OLDuucleanup
	/bin/rm -f $(BIN)/OLDuustat
	/bin/rm -f $(INSDIR)/OLDuusched
	/bin/rm -f $(INSDIR)/OLDuugetty

cmp:	all
	cmp uucp $(BIN)/uucp
	rm uucp
	cmp uux $(BIN)/uux
	rm uux
	cmp uuxqt $(INSDIR)/uuxqt
	rm uuxqt
	cmp uucico $(INSDIR)/uucico
	rm uucico
	cmp uuname $(BIN)/uuname
	rm uuname
	cmp uucheck $(INSDIR)/uucheck
	rm uucheck
	cmp uucleanup $(INSDIR)/uucleanup
	rm uucleanup
	cmp uustat $(BIN)/uustat
	rm uustat
	cmp uusched $(INSDIR)/uusched
	rm uusched
	-cmp uugetty $(INSDIR)/uugetty
	/bin/rm uugetty
	/bin/rm *.o

init:	anlwrk.o permission.o cpmv.o expfile.o gename.o \
	getargs.o getprm.o getpwinfo.o gnamef.o \
	gnxseq.o gwd.o imsg.o logent.o \
	mailst.o shio.o \
	systat.o ulockf.o uucpname.o versys.o xqt.o


uucp:	$(OUUCP) $(OFILES)
	$(CC) $(CFLAGS)  $(FLAGS) $(LDFLAGS) $(OUUCP) $(OFILES) \
		$(LIBS) -o uucp

uucp.M:	$(LUUCP) $(LFILES)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUCP) $(LFILES) $(LIBS) -o uucp.M

#
# Due to the large number of source files that need to be cataloged,
# this seems a more practical way of making the message catalog than
# explicitly listing all the .cat or .msg files in a macro.
# If source for a command does not exist, we can assume that the
# makefile will complain when it tries to compile it, so we don't
# worry about trying to find all the source files at this point.
#
uucp.cat: *.c
	for i in *.c;				\
	do					\
		$(FINDMSG) $$i > uucp.msg;	\
		$(GENCAT) $@ uucp.msg;		\
	done

uux:	$(OUUX) $(OFILES)
	$(CC) $(CFLAGS)  $(FLAGS) $(LDFLAGS) $(OUUX) $(OFILES) \
		$(LIBS) -o uux

uuxqt:	$(OUUXQT) $(OFILES)
	$(CC) $(CFLAGS)  $(FLAGS) $(LDFLAGS) $(OUUXQT) $(OFILES) \
		$(LIBS) -o uuxqt

uucico:	$(OUUCICO) $(OFILES) dkit/lib/$(LIBDK)
	$(CC) $(CFLAGS)  $(FLAGS) $(OUUCICO) $(OFILES) $(LIBS) \
		$(LDFLAGS) $(LIBDKFLAGS) -o uucico

uuname:	$(OUUNAME)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUUNAME) \
		$(LIBS) -o uuname

uustat:	$(OUUSTAT) $(GETOPT)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUUSTAT) $(GETOPT) \
		$(LIBS) -o uustat

uucleanup:	$(OUUCLEANUP) $(GETOPT)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUUCLEANUP) $(GETOPT) \
		$(LIBS) -o uucleanup

uusched:	$(OUUSCHED) $(GETOPT)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUUSCHED) $(GETOPT) \
		$(LIBS) -o uusched

uucheck:	uucheck.o uucpname.o $(GETOPT)
	$(CC) $(CFLAGS) $(FLAGS)  $(LDFLAGS) uucheck.o uucpname.o \
		$(GETOPT) $(LIBS) -o uucheck

uugetty:	$(OUUGETTY)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUUGETTY) -o uugetty

uux.M:	$(LUUX) $(LFILES)
	$(METRICS) $(CFLAGS)  $(FLAGS) $(LUUX) $(LFILES) \
		$(LIBS) -o uux.M

uuxqt.M:	$(LUUXQT) $(LFILES)
	$(METRICS) $(CFLAGS)  $(FLAGS) $(LUUXQT) $(LFILES) \
		$(LIBS) -o uuxqt.M

uucico.M:	$(LUUCICO) $(LFILES)
	$(METRICS) $(CFLAGS)  $(FLAGS) $(LUUCICO) $(LFILES) $(LIBS) \
		$(LIBDKFLAGS) -o uucico.M

uuname.M:	$(LUUNAME)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUNAME) \
		$(LIBS) -o uuname.M

uustat.M:	$(LUUSTAT) $(GETOPTSRC)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUSTAT) $(GETOPTSRC) \
		$(LIBS) -o uustat.M

uucleanup.M:	$(LUUCLEANUP) $(GETOPTSRC)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUCLEANUP) $(GETOPTSRC) \
		$(LIBS) -o uucleanup.M

uusched.M:	$(LUUSCHED) $(GETOPTSRC)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUSCHED) $(GETOPTSRC) \
		$(LIBS) -o uusched.M

uucheck.M:	uucheck.c uucpname.c $(GETOPTSRC)
	$(METRICS) $(CFLAGS) $(FLAGS)  uucheck.c uucpname.c \
		$(GETOPTSRC) $(LIBS) -o uucheck.M

uugetty.M:	$(LUUGETTY)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUGETTY) -o uugetty.M

uucheck.o:	permission.c

utility.o permission.o cico.o conn.o callers.o cpmv.o \
	anlwrk.o cntrl.o expfile.o gename.o \
	getpwinfo.o fio.o gio.o xio.o gnamef.o gnxseq.o gwd.o imsg.o \
	ioctl.o logent.o mailst.o sdmail.o line.o shio.o \
	systat.o ulockf.o uucpdefs.o uucpname.o uuname.o \
	uucleanup.o uucheck.o uusched.o  uucp.o uustat.o dio.o \
	uux.o uuxqt.o versys.o xqt.o:	uucp.h parms.h

uusub:	$(OUUSUB) $(GETOPT)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUUSUB) $(GETOPT) \
		$(LIBS) -o uusub

uusnap:	$(OUUSNAP) $(GETOPT)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUUSNAP) $(GETOPT) \
		$(LIBS) -o uusnap

uuls:	$(OUULS) $(GETOPT)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUULS) $(GETOPT) \
		$(LIBS) -o uuls

uuclean:	$(OUUCLEAN) $(GETOPT)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OUUCLEAN) $(GETOPT) \
		$(LIBS) -o uuclean

uusub.M:	$(LUUSUB) $(GETOPTSRC)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUSUB) $(GETOPTSRC) \
		$(LIBS) -o uusub.M

uusnap.M:	$(LUUSNAP) $(GETOPTSRC)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUSNAP) $(GETOPTSRC) \
		$(LIBS) -o uusnap.M

uuls.M:	$(LUULS) $(GETOPTSRC)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUULS) $(GETOPTSRC) \
		$(LIBS) -o uuls.M

uuclean.M:	$(LUUCLEAN) $(GETOPTSRC)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LUUCLEAN) $(GETOPTSRC) \
		$(LIBS) -o uuclean.M

dialit:	dialit.o
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) dialit.o -o dialit

opx25: $(OOPX25)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OOPX25) -o opx25

getx25: $(OGETX25)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OGETX25) -o getx25

clrsvc: $(OCLRSVC)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(OCLRSVC) -o clrsvc

dialit.M:	dialit.c
	$(METRICS) $(CFLAGS) $(FLAGS) dialit.c -o dialit.M

opx25.M: $(LOPX25)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LOPX25) -o opx25.M

getx25.M: $(LGETX25)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LGETX25) -o getx25.M

clrsvc.M: $(LCLRSVC)
	$(METRICS) $(CFLAGS) $(FLAGS) $(LCLRSVC) -o clrsvc.M

hpux_rel.o: hpux_rel.c

dkit/lib/$(LIBDK): $(LIBDKFILES) $(OLIBDKFILES)
	cd dkit/lib;				\
	$(MAKE) LIBDK=$(LIBDK);			\
	if test -s $(LIBDK); then		\
	    touch $(LIBDK);			\
	fi

#
# We re-define the .c.o rule so that implicit compiles (cc -c) will
# use the FLAGS macro
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

dio.o: $(INCLUDEDIR)
callers.o: $(INCLUDEDIR)
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/UUCP/etc
	! cp getx25 ../../../../Filesets.sl/UUCP/etc/getx25
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/etc/getx25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/etc
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uucp ../../../../Filesets.sl/UUCP/usr/bin/uucp
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/bin/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uulog ../../../../Filesets.sl/UUCP/usr/bin/uulog
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/bin/uulog
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uuls ../../../../Filesets.sl/UUCP/usr/bin/uuls
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/bin/uuls
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uuname ../../../../Filesets.sl/UUCP/usr/bin/uuname
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/bin/uuname
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uupick ../../../../Filesets.sl/UUCP/usr/bin/uupick
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/bin/uupick
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uusnap ../../../../Filesets.sl/UUCP/usr/bin/uusnap
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/bin/uusnap
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uusnaps ../../../../Filesets.sl/UUCP/usr/bin/uusnaps
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/bin/uusnaps
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uustat ../../../../Filesets.sl/UUCP/usr/bin/uustat
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/bin/uustat
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uuto ../../../../Filesets.sl/UUCP/usr/bin/uuto
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/bin/uuto
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp uux ../../../../Filesets.sl/UUCP/usr/bin/uux
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/bin/uux
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/nls/C
	! cp uucp.cat ../../../../Filesets.sl/UUCP/usr/lib/nls/C/uucp.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/nls/C/uucp.cat
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp SetUp ../../../../Filesets.sl/UUCP/usr/lib/uucp/SetUp
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/SetUp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp Uutry ../../../../Filesets.sl/UUCP/usr/lib/uucp/Uutry
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/Uutry
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp HP2334A.clr ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.clr
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.clr
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp HP2334A.in ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.in
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.in
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp HP2334A.out ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.out
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.out
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp HP2334A.outf ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.outf
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.outf
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp HP2334A.outg ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.outg
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.outg
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp HP2334A.outi ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.outi
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/HP2334A.outi
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp clrsvc ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/clrsvc
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/clrsvc
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp opx25 ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/opx25
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/opx25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! cp ventel.out ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/ventel.out
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25/ventel.out
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/X25
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Cvt ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Cvt
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Cvt
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Devices ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Devices
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Devices
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Dialcodes ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Dialcodes
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Dialcodes
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Dialers ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Dialers
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Dialers
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Make.cvt ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Make.cvt
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Make.cvt
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Maxuuscheds ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Maxuuscheds
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Maxuuscheds
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Maxuuxqts ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Maxuuxqts
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Maxuuxqts
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Permissions ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Permissions
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Permissions
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Poll ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Poll
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Poll
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp README.CVT ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/README.CVT
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/README.CVT
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp Systems ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Systems
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/Systems
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp awkdevices ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/awkdevices
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/awkdevices
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp awkperm ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/awkperm
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/awkperm
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp awksystems ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/awksystems
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/awksystems
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp dialit ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/dialit
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/dialit
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp dialit.c ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/dialit.c
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/dialit.c
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp uucheck ../../../../Filesets.sl/UUCP/usr/lib/uucp/uucheck
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/uucheck
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp uucico ../../../../Filesets.sl/UUCP/usr/lib/uucp/uucico
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/uucico
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp uuclean ../../../../Filesets.sl/UUCP/usr/lib/uucp/uuclean
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/uuclean
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp uucleanup ../../../../Filesets.sl/UUCP/usr/lib/uucp/uucleanup
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/uucleanup
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp uugetty ../../../../Filesets.sl/UUCP/usr/lib/uucp/uugetty
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/uugetty
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp uusched ../../../../Filesets.sl/UUCP/usr/lib/uucp/uusched
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/uusched
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp uusub ../../../../Filesets.sl/UUCP/usr/lib/uucp/uusub
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/uusub
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 04555, uucp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! cp uuxqt ../../../../Filesets.sl/UUCP/usr/lib/uucp/uuxqt
	! chmog 04555 uucp bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/uuxqt
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp remote.unkno ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/remote.unknown
	! chmog 0444 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/remote.unknown
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp uudemon.admi ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/uudemon.admin
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/uudemon.admin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp uudemon.clea ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/uudemon.cleanu
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/uudemon.cleanu
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp uudemon.hour ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/uudemon.hour
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/uudemon.hour
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.uucp to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/hdb.uucp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! cp uudemon.poll ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/uudemon.poll
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig/uudemon.poll
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/uucp/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
