# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/lp/makefile,v $
# $Revision: 66.2.1.103 $

############################
# Common makefile for lp commands and utility:
#
# << command >>
# accept
# cancel
# disable
# enable
# lp
# lpadmin
# lpalt
# lpana
# lpfence
# lpmove
# lpsched
# lpshut
# lpstat
# reject
# rcancel
# rlp
# rlpdaemon
# rlpstat
#
# << utility >>
# converter	--    pstatus and qstatus file format converter to 7.0
############################

ADMIN	= lp
LIB	= lib.a
AR	= ar
DFLAGS	= -DADMIN='"$(ADMIN)"' -DREMOTE
FLAGS	= -lm

FINDMSG = /usr/bin/findmsg
GENCAT	= /usr/bin/gencat

CFILES	= accept cancel disable enable lp lpadmin lpalt lpana lpfence lpmove \
	lpsched lpshut lpstat reject rcancel rlp rlpdaemon rlpstat

MFILES  = accept.M cancel.M disable.M enable.M lp.M lpadmin.M lpalt.M lpana.M \
	  lpfence.M lpmove.M lpsched.M lpshut.M lpstat.M reject.M rcancel.M \
	  rlp.M rlpdaemon.M rlpstat.M

OBJS =	dest.o destlist.o fullpath.o gwd.o enter.o fifo.o \
	getname.o isclass.o isprinter.o isrequest.o outlist.o \
	prilist.o outputq.o pstatus.o date.o isdest.o eaccess.o \
	qstatus.o fatal.o lock.o altit.o request.o sendmail.o \
	trim.o wrtmsg.o findtty.o \
	common.o rmjob.o sendit.o recvjob.o alter.o displayq.o \
	mpstatus.o

REL   = hpux_rel.o

MSGS  = accept.msg cancel.msg date.msg disable.msg enable.msg lp.msg \
	lpadmin.msg lpalt.msg lpana.msg lpfence.msg lpmove.msg \
	lpsched.msg lpshut.msg lpstat.msg \
	outputq.msg pstatus.msg qstatus.msg reject.msg \
	wrtmsg.msg common.msg rmjob.msg sendit.msg recvjob.msg displayq.msg \
	mpstatus.msg rcancel.msg rlp.msg rlpdaemon.msg rlpstat.msg


default: all

all: $(LIB) $(CFILES) lp.cat converter
	cd filter; $(MAKE)
	cd fonts; $(MAKE)

metrics: $(MFILES)

$(LIB):	$(OBJS)
	$(AR) r $(LIB) $(OBJS)
	-chmod 664 $@

do_link:
	@libs="$(LIB)";						\
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd $$libs -lsec -lcurses";		\
	fi;							\
	echo "\t$(CC)" $(LDFLAGS) -o $(TARG) $(REL) $(TARG).o $$libs; \
	$(CC) $(LDFLAGS) -o $(TARG) $(REL) $(TARG).o $$libs

do_metrics:
	@libs="$(LIB)";                                         \
	$(METRICS) $(LDFLAGS) -o $(TARG) $(REL) $(TARG).c $$libs

accept: $(LIB) $(REL) accept.o
	@$(MAKE) TARG=$@ do_link

cancel: $(LIB) $(REL) cancel.o
	@$(MAKE) TARG=$@ do_link

disable: $(LIB) $(REL) disable.o
	@$(MAKE) TARG=$@ do_link

enable: $(LIB) $(REL) enable.o
	@$(MAKE) TARG=$@ do_link

lp: $(LIB) $(REL) lp.o
	@$(MAKE) TARG=$@ do_link

lpadmin: $(LIB) $(REL) lpadmin.o
	@$(MAKE) TARG=$@ do_link

lpalt: $(LIB) $(REL) lpalt.o
	@$(MAKE) TARG=$@ do_link

lpana: $(LIB) $(REL) lpana.o
	@$(MAKE) TARG=$@ LIB="$(LIB) $(FLAGS)" do_link

lpfence: $(LIB) $(REL) lpfence.o
	@$(MAKE) TARG=$@ do_link

lpmove: $(LIB) $(REL) lpmove.o
	@$(MAKE) TARG=$@ do_link

lpsched: $(LIB) $(REL) lpsched.o
	@$(MAKE) TARG=$@ do_link

lpshut: $(LIB) $(REL) lpshut.o
	@$(MAKE) TARG=$@ do_link

lpstat: $(LIB) $(REL) lpstat.o
	@$(MAKE) TARG=$@ do_link

reject: $(LIB) $(REL) reject.o
	@$(MAKE) TARG=$@ do_link

rcancel: $(LIB) $(REL) rcancel.o
	@$(MAKE) TARG=$@ do_link

rlp: $(LIB) $(REL) rlp.o
	@$(MAKE) TARG=$@ do_link

rlpdaemon: $(LIB) $(REL) rlpdaemon.o
	@$(MAKE) TARG=$@ do_link

rlpstat: $(LIB) $(REL) rlpstat.o
	@$(MAKE) TARG=$@ do_link

#accept.M: accept.c
#       @$(MAKE) TARG=$@ do_metrics
#
#cancel.M: cancel.c
#       @$(MAKE) TARG=$@ do_metrics
#
#disable.M: disable.c
#       @$(MAKE) TARG=$@ do_metrics
#
#enable.M: enable.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lp.M: lp.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lpadmin.M: lpadmin.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lpalt.M: lpalt.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lpana.M: lpana.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lpfence.M: lpfence.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lpmove.M: lpmove.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lpsched.M: lpsched.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lpshut.M: lpshut.c
#       @$(MAKE) TARG=$@ do_metrics
#
#lpstat.M: lpstat.c
#       @$(MAKE) TARG=$@ do_metrics
#
#reject.M: reject.c
#       @$(MAKE) TARG=$@ do_metrics
#
#rcancel.M: rcancel.c
#       @$(MAKE) TARG=$@ do_metrics
#
#rlp.M: rlp.c
#       @$(MAKE) TARG=$@ do_metrics
#
#rlpdaemon.M: rlpdaemon.c
#       @$(MAKE) TARG=$@ do_metrics
#
#rlpstat.M: rlpstat.c
#       @$(MAKE) TARG=$@ do_metrics

#######
# Re-define .c.o rule so implicit compiles will use DFLAGS
#######

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -c $<

#######
# Metrics file creation rule
#######
.c.M:
	$(METRICS) $(CFLAGS) $(DFLAGS) -o $*.M $*.c

#######
# Local Header dependencies
#######

$(OBJS): lp.h
dest.o destlist.o findtty.o outlist.o wrtmsg.o: lpsched.h

#######
# Create message catalog file
#######

lp.cat: $(MSGS)
	 $(GENCAT) $@ $(MSGS)

######
# File converter for 7.0
######

converter: $(REL) converter.o
	$(CC) $(LDFLAGS) -o $@ $(REL) converter.o

#######
# Define .c to .msg rule so make will automatically know how to create a
# .msg from a .c file if it needs to
#######

.c.msg:
	$(FINDMSG) $*.c > $*.msg

.SUFFIXES: .cat .msg .M


#######
# Clean & clobber targets.  <command>clean or <command>clobber allow you
# to clean or clobber only files for one of the commands; topclean and
# topclobber allow you to clean or clobber only the lp directory (leaving
# the subdirectories untouched); just plain clean or clobber will clear out
# the lp directories and the subdirectories as well.
#######

clean:	topclean
	cd filter; $(MAKE) clean
	cd fonts; $(MAKE) clean
topclean:	;  /bin/rm -f $(LIB) *.o *.msg
acceptclean:	;  /bin/rm -f accept.o accept.msg
cancelclean:	;  /bin/rm -f cancel.o cancel.msg
disableclean:	;  /bin/rm -f disable.o disable.msg
enableclean:	;  /bin/rm -f enable.o enable.msg
lpclean:	;  /bin/rm -f lp.o lp.msg
lpadminclean:	;  /bin/rm -f lpadmin.o lpadmin.msg
lpmoveclean:	;  /bin/rm -f lpmove.o lpmove.msg
lpschedclean:	;  /bin/rm -f lpsched.o lpsched.msg
lpshutclean:	;  /bin/rm -f lpshut.o lpshut.msg
lpstatclean:	;  /bin/rm -f lpstat.o lpstat.msg
rejectclean:	;  /bin/rm -f reject.o reject.msg
libclean:
	/bin/rm -f $(OBJS)

clobber:	topclobber
	cd filter; $(MAKE) clobber
	cd fonts; $(MAKE) clobber
topclobber: topclean
	/bin/rm -f $(CFILES) lp.cat $(LIB)
acceptclobber:	acceptclean
	/bin/rm -f accept accept.cat
cancelclobber:	cancelclean
	/bin/rm -f cancel cancel.cat
disableclobber:	disableclean
	/bin/rm -f disable disable.cat
enableclobber:	enableclean
	/bin/rm -f enable enable.cat
lpclobber:	lpclean
	/bin/rm -f lp lp.cat
lpadminclobber:	lpadminclean
	/bin/rm -f lpadmin lpadmin.cat
lpmoveclobber:	lpmoveclean
	/bin/rm -f lpmove lpmove.cat
lpschedclobber:	lpschedclean
	/bin/rm -f lpsched lpsched.cat
lpshutclobber:	lpshutclean
	/bin/rm -f lpshut lpshut.cat
lpstatclobber:	lpstatclean
	/bin/rm -f lpstat lpstat.cat
rejectclobber:	rejectclean
	/bin/rm -f reject reject.cat
libclobber:	libclean
	/bin/rm -f $(LIB)
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp cancel ../../../../Filesets.sl/LP-SPOOL/usr/bin/cancel
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/cancel
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp disable ../../../../Filesets.sl/LP-SPOOL/usr/bin/disable
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/disable
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp enable ../../../../Filesets.sl/LP-SPOOL/usr/bin/enable
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/enable
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp lp ../../../../Filesets.sl/LP-SPOOL/usr/bin/lp
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/lp
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp lpalt ../../../../Filesets.sl/LP-SPOOL/usr/bin/lpalt
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/lpalt
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! cp lpstat ../../../../Filesets.sl/LP-SPOOL/usr/bin/lpstat
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/bin/lpstat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp accept ../../../../Filesets.sl/LP-SPOOL/usr/lib/accept
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/accept
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp filter/divpage ../../../../Filesets.sl/LP-SPOOL/usr/lib/divpage
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/divpage
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp filter/fontdl ../../../../Filesets.sl/LP-SPOOL/usr/lib/fontdl
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/fontdl
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp lpadmin ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpadmin
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpadmin
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp lpana ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpana
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpana
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp lpfence ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpfence
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpfence
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp lpmove ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpmove
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpmove
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp filter/lprpp ../../../../Filesets.sl/LP-SPOOL/usr/lib/lprpp
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/lprpp
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp lpsched ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpsched
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpsched
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp lpshut ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpshut
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/lpshut
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! cp lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp filter/plotdvr ../../../../Filesets.sl/LP-SPOOL/usr/lib/plotdvr
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/plotdvr
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp filter/printstat ../../../../Filesets.sl/LP-SPOOL/usr/lib/printstat
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/printstat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp rcancel ../../../../Filesets.sl/LP-SPOOL/usr/lib/rcancel
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/rcancel
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, lp, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp reject ../../../../Filesets.sl/LP-SPOOL/usr/lib/reject
	! chmog 04555 lp bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/reject
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp filter/reverse ../../../../Filesets.sl/LP-SPOOL/usr/lib/reverse
	! chmog 0555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/reverse
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp rlp ../../../../Filesets.sl/LP-SPOOL/usr/lib/rlp
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/rlp
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp rlpdaemon ../../../../Filesets.sl/LP-SPOOL/usr/lib/rlpdaemon
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/rlpdaemon
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp rlpstat ../../../../Filesets.sl/LP-SPOOL/usr/lib/rlpstat
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/rlpstat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! cp filter/rwrite ../../../../Filesets.sl/LP-SPOOL/usr/lib/rwrite
	! chmog 04555 root bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/rwrite
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/cmodel
	! cp cmodel/rcmodel ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/cmodel/rcmodel
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/cmodel/rcmodel
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/cmodel
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts
	! cp fonts/lp2l ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts/lp2l
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts/lp2l
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts
	! cp fonts/lp4l ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts/lp4l
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts/lp4l
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts
	! cp fonts/lp4p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts/lp4p
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts/lp4p
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/fonts
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/HPGL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL1
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL1
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/HPGL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/HPGL2.cent ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2.cent
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2.cent
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/PCL4 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL4
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL4
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/PCL5 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL5
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL5
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/clustermodel ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/clustermodel
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/clustermodel
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/dumb ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/dumb
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/dumb
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/dumbplot ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/dumbplot
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/dumbplot
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/hp256x.cent ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp256x.cent
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp256x.cent
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/hp33440a ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp33440a
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp33440a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/hpC1208a ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hpC1208a
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hpC1208a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/paintjetXL300 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/paintjetXL300
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/paintjetXL300
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/postscript ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/postscript
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/postscript
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! cp model/rmodel ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/rmodel
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/rmodel
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/smodel
	! cp smodel/rsmodel ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/smodel/rsmodel
	! chmog 0444 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/smodel/rsmodel
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/smodel
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/accept.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/cancel.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/disable.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/enable.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lpadmin.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lpmove.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lpsched.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lpshut.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lpstat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/lp.cat ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C/reject.cat
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/colorpro
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/deskjet
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/paintjetXL300 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/deskjet1200C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/deskjet500
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/deskjet500C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/deskjet550C
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/draftpro
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2225a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2225d
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2227a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2228a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2235a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2276a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2300-1100L
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2300-840L
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2560
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2563a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2564b
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2565a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2566b
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2567b
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2631g
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp33440a ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2684a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp33440a ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2686a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2932a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp2934a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL4 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp33447a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp3630a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp7440a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp7475a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp7550a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp7570a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp7595a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/HPGL2 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/hp7596a
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL4 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/laserjet
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL5 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/laserjet4
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL5 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/laserjet4Si
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL5 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/laserjetIIISi
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL5 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/colorlaserjet
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/paintjet
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/quietjet
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL3 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/ruggedwriter
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lp to the . relative fileset LP-SPOOL
# change mode, owner, and group on /usr/bin/lp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! ln ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/PCL1 ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model/thinkjet
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/model
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/lp/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/spool/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/
	! chmog 555 bin bin ../../../../Filesets.sl/
	mkdir -p ../../../../Filesets.sl/LP-SPOOL/etc/newconfig
	! cp lpschedCDF ../../../../Filesets.sl/LP-SPOOL/etc/newconfig/lpschedCDF
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/etc/newconfig/lpschedCDF
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL/etc
	! chmog 555 bin bin ../../../../Filesets.sl/LP-SPOOL
	! chmog 555 bin bin ../../../../Filesets.sl
