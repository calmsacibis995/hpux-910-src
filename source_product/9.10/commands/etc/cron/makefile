# $Source: /misc/source_product/9.10/commands.rcs/etc/cron/makefile,v $
# $Revision: 66.1.1.15 $

# Common makefile for cron, crontab, at, and batch

FINDMSG = /usr/bin/findmsg
GENCAT  = /usr/bin/gencat 

CMDS= cron at crontab batch
CATS= cron.cat at.cat crontab.cat 
CRONMSGS= cron.msg funcs.msg
CRONTABMSGS= crontab.msg permit.msg funcs.msg
ATMSGS= at.msg funcs.msg pdt.msg ptime.msg pdate.msg permit.msg
REL= hpux_rel.o


default: all

all:	$(CMDS) $(CATS)

libelm.a: elm.o
	ar cr libelm.a elm.o

cron:	cron.o funcs.o libelm.a $(REL)
	libs="libelm.a";					\
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) $(LDFLAGS) $(REL) cron.o funcs.o -o $@ $$libs

cron.cat: $(CRONMSGS)
	  $(GENCAT) $@ $(CRONMSGS)

crontab:	crontab.o permit.o funcs.o $(REL)
	libs="";						\
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) $(LDFLAGS) $(REL) crontab.o permit.o funcs.o -o $@ $$libs

crontab.cat: $(CRONTABMSGS) 
	$(GENCAT) $@ $(CRONTABMSGS)


at:	at.o pdt.o ptime.o pdate.o funcs.o permit.o $(REL)
	libs="";						\
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) $(LDFLAGS) $(REL) \
	    at.o pdt.o ptime.o pdate.o funcs.o permit.o -o $@ $$libs

at.cat: $(ATMSGS)
	$(GENCAT) $@ $(ATMSGS)

batch:	batch.sh
	cp batch.sh $@

cron.o:	cron.c cron.h
crontab.o:	crontab.c cron.h
at.o:	at.c cron.h

clean:
	/bin/rm -f *.o *.msg libelm.a

clobber:	clean
	/bin/rm -f $(CMDS) $(CATS)

# 
# We define some rules for creating .msg and .cat files so make will do
# our work for us.  With these rules, all we have to do is indicate the
# .cat file depends on the .msg file; make will then know it needs to create
# the .msg from the .c if it doesn't already exist.
#
.c.msg:
	$(FINDMSG) $*.c > $*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cron
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc
	! cp cron ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/cron
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc
	! cp cron ../../../Filesets.sl/CMDS-MIN/etc/cron
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/cron
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! cp at.allow ../../../Filesets.sl/CMDS-MIN/etc/newconfig/at.allow
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig/at.allow
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! cp cron.allow ../../../Filesets.sl/CMDS-MIN/etc/newconfig/cron.allow
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig/cron.allow
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! cp crontab.root ../../../Filesets.sl/CMDS-MIN/etc/newconfig/crontab.root
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig/crontab.root
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! cp queuedefs ../../../Filesets.sl/CMDS-MIN/etc/newconfig/queuedefs
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig/queuedefs
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp at ../../../Filesets.sl/CMDS-MIN/usr/bin/at
	! chmog 04555 root bin ../../../Filesets.sl/CMDS-MIN/usr/bin/at
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp batch ../../../Filesets.sl/CMDS-MIN/usr/bin/batch
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/bin/batch
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp crontab ../../../Filesets.sl/CMDS-MIN/usr/bin/crontab
	! chmog 04555 root bin ../../../Filesets.sl/CMDS-MIN/usr/bin/crontab
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp at.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/at.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/at.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp cron.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/cron.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/cron.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp crontab.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/crontab.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/crontab.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/cron to the . relative fileset CMDS-MIN
# change mode, owner, and group on /etc/cron
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! cp proto ../../../Filesets.sl/CMDS-MIN/etc/newconfig/.proto
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig/.proto
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/etc/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
