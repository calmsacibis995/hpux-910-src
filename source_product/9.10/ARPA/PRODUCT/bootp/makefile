#
# bootpd   - BOOTP server daemon
#
# $Header: makefile,v 1.4.109.1 91/11/21 11:48:36 kcs Exp $
#

# The next few lines may be uncommented and changed to alter the default
# filenames bootpd uses for its configuration and dump files.
#CONFFILE=-DCONFIG_FILE=\"/usr/etc/bootptab\"
#DUMPFILE=-DDUMP_FILE=\"/usr/etc/bootpd.dump\"
#FILESPECS=${CONFFILE} ${DUMPFILE}

# Users of SunOS 4.0 may have to add -DSUNOS40 to BOOTPDOPT below.
# Remove the -DVEND_CMU if you don't wish to support the "CMU vendor format"
# in addition to the RFC1048 format.

BOOTPDOPT=-DDEBUG -DVEND_CMU -DSYSLOG -DPRIVATE=static ${FILESPECS}

DESTDIR=
ETCDIR=/etc
OBJS=bootpd.o readfile.o hash.o
CFLAGS=${BOOTPDOPT} -O -I../include
LDFLAGS= -s ../util/libutil.a

all: bootpd bootpquery

bootpd:	${OBJS}
	sh newvers.sh
	${CC} ${CFLAGS} -o bootpd version.c ${OBJS} ${LDFLAGS}

bootpquery: bootpquery.o
	${CC} ${CFLAGS} -o bootpquery bootpquery.o ${LDFLAGS}

bootpquery.o: bootpquery.c bootp.h
bootpd.o: bootpd.c bootpd.h bootp.h
readfile.o: readfile.c bootpd.h bootp.h
hash.o: hash.c hash.h

clean:
	@rm -f core bootpd *.BAK *.CKP *~ .emacs* *.o version.c

clobber: clean
