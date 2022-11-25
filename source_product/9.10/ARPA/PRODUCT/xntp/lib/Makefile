/*
 *
 *
 *  $Header: Makefile,v 1.2.109.4 94/10/28 16:42:42 mike Exp $
 *  New make file for libntp.a
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

NTPLIBSRCS=	atoint.c atolfp.c atouint.c auth12crypt.c authdecrypt.c \
	authdes.c \
	authencrypt.c authkeys.c authparity.c authreadkeys.c authusekey.c \
	buftvtots.c caljulian.c calleapwhen.c caltontp.c calyearstart.c \
	clocktime.c dofptoa.c dolfptoa.c emalloc.c fptoa.c fptoms.c getopt.c \
	gettstamp.c hextoint.c hextolfp.c humandate.c inttoa.c \
	lib_strbuf.c mfptoa.c mfptoms.c modetoa.c mstolfp.c \
	msutotsf.c numtoa.c refnumtoa.c numtohost.c octtoint.c \
	prettydate.c ranny.c tsftomsu.c tstotv.c tvtoa.c tvtots.c \
	uglydate.c uinttoa.c utvtoa.c hpux.c clocktypes.c \
	parse.c parse_conf.c parse_meinberg.c parse_schmid.c parse_rawdcf.c \
	parse_dcf7000.c md5.c authmd5encrypt.c authmd5decrypt.c \
	authmd512crypt.c decodenetnum.c systime.c msyslog.c
NTPLIBOBJS=	atoint.o atolfp.o atouint.o auth12crypt.o authdecrypt.o \
	authdes.o \
	authencrypt.o authkeys.o authparity.o authreadkeys.o authusekey.o \
	buftvtots.o caljulian.o calleapwhen.o caltontp.o calyearstart.o \
	clocktime.o dofptoa.o dolfptoa.o emalloc.o fptoa.o fptoms.o getopt.o \
	gettstamp.o hextoint.o hextolfp.o humandate.o inttoa.o \
	lib_strbuf.o mfptoa.o mfptoms.o modetoa.o mstolfp.o \
	msutotsf.o numtoa.o refnumtoa.o numtohost.o octtoint.o \
	prettydate.o ranny.o tsftomsu.o tstotv.o tvtoa.o tvtots.o \
	uglydate.o uinttoa.o utvtoa.o hpux.o clocktypes.o \
	parse.o parse_conf.o parse_meinberg.o parse_schmid.o parse_rawdcf.o \
	parse_dcf7000.o	md5.o authmd5encrypt.o authmd5decrypt.o \
	authmd512crypt.o decodenetnum.o systime.o msyslog.o

LDLIBRARIES=	../lib/libntp.a

AUTHDEFS=-DDES -DMD5
LIBDEFS= -DXNTP_BIG_ENDIAN
DEFS= -DREFCLOCK -DNO_SIGNED_CHAR_DECL -DDEBUG -DHPUXDEBUG \
      -DHPUX=9 -DNOKMEM -DHAVE_UNISTD_H 

NTPLIBOPT= $(AUTHDEFS) $(LIBDEFS) $(DEFS)

CCFLAGS=	-O $(NTPLIBOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NTPLIBOPT) $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

/*
# $(LIBNAME).a:	$(OBJS)
# 	ar rv $@ $?
# 	rm -f $?
#	$(RANLIB) $@
# 
# depend:
# 	mkdep $(CFLAGS) $(SOURCE)
# 
# clean:
# 	rm -f $(LIBNAME).a *.o *.out *.ln make.log Makefile.bak \
# 		lintlib.errs lint.errs
# 
# distclean:	clean
# 	rm -f Makefile
# 
*/

#ifdef BFA
BFADB=		/bfa/ARPA/databases/bootp.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(NTPLIBOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

libntp.a :: $(NTPLIBOBJS)

all : build

debug : build

bfa : build

build : $(TARGET:/^$/libntp.a/)


alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	# Nothing to install since we are using archive libs.
	# $(INSTALL) $(TARGET:/^$/libntp.a/)

rinstall: install
	# Nothing to install since we are using archive libs.
	# $(RINSTALL) $(TARGET:/^$/libntp.a/)

clean :
	$(RM) $(RMFLAGS) $(NTPLIBOBJS)


clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/libntp.a/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(NTPLIBOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]
