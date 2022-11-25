/*
 *
 *  $Header: Makefile,v 1.2.109.16 94/12/19 16:03:15 mike Exp $
 *  New make file for xntpd
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

NTPDOBJS=	ntp_config.o ntp_control.o ntp_io.o ntp_leap.o \
		ntp_loopfilter.o ntp_monitor.o ntp_peer.o ntp_proto.o \
		ntp_refclock.o ntp_request.o ntp_restrict.o ntp_timer.o \
		ntp_unixclock.o ntp_util.o ntpd.o refclock_chu.o \
		refclock_conf.o refclock_local.o refclock_pst.o \
		refclock_wwvb.o refclock_goes.o refclock_mx4200.o \
		refclock_parse.o refclock_as2201.o refclock_omega.o \
		refclock_tpro.o refclock_leitch.o refclock_irig.o \
		ntp_intres.o ntp_filegen.o  ver.o

NTPDSRCS=	ntp_config.c ntp_control.c ntp_io.c ntp_leap.c \
		ntp_loopfilter.c ntp_monitor.c ntp_peer.c ntp_proto.c \
		ntp_refclock.c ntp_request.c ntp_restrict.c ntp_timer.c \
		ntp_unixclock.c ntp_util.c ntpd.c refclock_chu.c \
		refclock_conf.c refclock_local.c refclock_pst.c \
		refclock_wwvb.c refclock_goes.c refclock_mx4200.c \
		refclock_parse.c refclock_as2201.c refclock_omega.c \
		refclock_tpro.c refclock_leitch.c refclock_irig.c \
		ntp_intres.c ntp_filegen.c  ver.c newvers.sh
SRCS=		$(NTPDSRCS)
HDRS=


LDLIBRARIES=    ../lib/libntp.a ../adjtime/libadjtime.a -lBSD

NTPDOPT=	-DREFCLOCK -DNO_SIGNED_CHAR_DECL -DDEBUG -DHPUXDEBUG \
      		-DHPUX=9 -DNOKMEM -DHAVE_UNISTD_H
CLOCKDEFS= 	-DLOCAL_CLOCK -DWWVB
XNINS=		/usr/contrib/bin
CONFINS=	/usr/contrib/etc

INSOPTSC=       -u root -g bin
INSOPTSN=	-u root -g bin

CCFLAGS=	-O $(NTPDOPT) $(CLOCKDEFS) $(NEWCCFLAGS)
LDFLAGS=	-s -lBSD $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DHPUXDEBUG $(NTPDOPT) $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/xntpd.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(NTPDOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

ver.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c ver.c

xntpd :: $(NTPDOBJS) ver.c

all : build

debug : build

bfa : build

build : $(TARGET:/^$/xntpd ntp.conf.sample/)

ntp.conf.sample : ntp.conf
	cp $(SRCDIR)/ntp.conf ntp.conf.sample


alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	$(INSTALL) $(INSOPTSN) -f $(XNINS) $(TARGET:/^$/xntpd/)
	$(INSTALL) $(INSOPTSC) -f $(CONFINS) $(TARGET:/^$/ntp.conf.sample/)

rinstall: install
	$(RINSTALL) $(TARGET:/xntpd/sbin\/xntpd/)
	$(RINSTALL) $(TARGET:/^$/ntp.conf.sample/)

clean :
	$(RM) $(RMFLAGS) $(NTPDOBJS)
	
clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/xntpd/)
	$(RM) $(RMFLAGS) $(TARGET:/^$/ntp.conf.sample/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(NTPDOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following xntpd sources:\n" >version.log
	for i in $(SRCS) $(HDRS) ; do
		if [ "$i" = "ver.c" ] ; then
			continue;
		fi
		ident $i | 
		grep Header |
		awk 'BEGIN { spaces="      ";\
			     tablen=5-length(FILE)+1;\
			     tab=substr(spaces,1,tablen);\
			    }\
		     {print FILE tab $3}' FILE=$i -
	done | sort | pr -2 -t >>version.log
