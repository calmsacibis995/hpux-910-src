##
#	@(#)$Revision: 1.11.109.1 $	$Date: 91/11/19 14:19:04 $  
# (#)Makefile	1.2 87/08/07 3.2/4.3NFSSRC
# (#)Makefile	1.2 86/12/30 NFSSRC
#
# (#)Makefile 1.1 86/09/24 Copyright Sun Micro 1986
#
# Makefile for rpc.statd - the network status monitor
SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DTRACEON ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DTRACEON ${CFLAGS}
DLFLAGS	= -g 
BCFLAGS	= -DBFA ${CFLAGS}
BLFLAGS	= ${LDFLAGS}

CSOURCE	= sm_svc.c sm_proc.c sm_statd.c ../rpc.lockd/tcp.c ../rpc.lockd/udp.c \
	../rpc.lockd/pmap.c ../../usr.etc/logging.c
OBJECTS = sm_svc.o sm_proc.o sm_statd.o ../rpc.lockd/tcp.o ../rpc.lockd/udp.o ../rpc.lockd/pmap.o logging.o
BSOURCE	= sm_svc.c sm_proc.c sm_statd.c 
BFA_SRC = _sm_svc.c _sm_proc.c _sm_statd.c _tcp.c _udp.o _pmap.o _logging.c
BFA_OBJ = _sm_svc.o _sm_proc.o _sm_statd.o 

TARGETS	= rpc.statd
BFATARG	= rpc.statd.b
HDRS=  sm_statd.h  

.SUFFIXES:.b
.c.o:
	@-rm -f $*
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $< 
.c:
	@-rm -f $*
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $* $< ${RPCSVC_LIB} ${LIBS} ${LIBC}

.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? -B"-d/nfs/BFA/$*.B" ${BLIBRPC} ${BLIBC} 

all: 		${TARGETS}

bfa:		${BFATARG}

rpc.statd: ${OBJECTS}
	@-rm -f $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS}  -o $@ ${OBJECTS} ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.statd.b:	${BSOURCE} _logging.o _tcp.o _udp.o _pmap.o
	@-rm -f /nfs/BFA/rpc.statd.B ${BFA_SRC} ${BFA_OBJ}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${BSOURCE} _logging.o _tcp.o _udp.o _pmap.o -B"-d/nfs/BFA/rpc.statd.B" ${BLIBRPC} ${BLIBC} 

_logging.o:     ${Nfs}/cmds/usr.etc/logging.c
	@-rm -f $@ _$*.c /nfs/BFA/logging.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/logging.c -B"-d/nfs/BFA/logging.B"

_tcp.o:     ${Nfs}/cmds/usr.etc/rpc.lockd/tcp.c
	@-rm -f $@ _$*.c /nfs/BFA/tcp.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/rpc.lockd/tcp.c -B"-d/nfs/BFA/tcp.B"


../rpc.lockd/tcp.o: ../rpc.lockd/tcp.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../rpc.lockd/tcp.c -o ../rpc.lockd/tcp.o

_udp.o:     ${Nfs}/cmds/usr.etc/rpc.lockd/udp.c
	@-rm -f $@ _$*.c /nfs/BFA/udp.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/rpc.lockd/udp.c -B"-d/nfs/BFA/udp.B"


../rpc.lockd/udp.o: ../rpc.lockd/udp.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../rpc.lockd/udp.c -o ../rpc.lockd/udp.o

_pmap.o:     ${Nfs}/cmds/usr.etc/rpc.lockd/pmap.c
	@-rm -f $@ _$*.c /nfs/BFA/pmap.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/rpc.lockd/pmap.c -B"-d/nfs/BFA/pmap.B"


../rpc.lockd/pmap.o: ../rpc.lockd/pmap.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../rpc.lockd/pmap.c -o ../rpc.lockd/pmap.o

logging.o:	../../usr.etc/logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../../usr.etc/logging.c $(RPCSVC_LIB}

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
		${INS} rpc.statd usr/etc root other 0500 

depend:;
	maketd ${CFLAGS} ${IDIRS} ${CSOURCE} > ,makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' > ,eddep
	@echo '$$r ,makedep' >> ,eddep
	@echo 'w' >> ,eddep
	cp Makefile Makefile.bak
	ed - Makefile < ,eddep
	rm -f ,eddep ,makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- maketd depends upon it


