##
#	@(#)$Revision: 1.6.109.1 $	$Date: 91/11/19 14:18:59 $  
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

CSOURCE	= ../sm_svc.c ../sm_proc.c ../sm_statd.c ../../rpc.lockd/tcp.c \
	  ../../rpc.lockd/udp.c ../../rpc.lockd/pmap.c ../../usr.etc/logging.c

800OBJS = sm_svc.o sm_proc.o sm_statd.o tcp.o udp.o pmap.o logging.o
BFA_OBJ = sm_svc.b sm_proc.b sm_statd.b tcp.b

800TARG	= rpc.statd
BFATARG	= rpc.statd.b
HDRS=  sm_statd.h  

all: 		${800TARG}

bfa:		${BFATARG}

rpc.statd: ${800OBJS}
	@-rm -f $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS}  -o $@ ${800OBJS} ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.statd.b:	${CSOURCE} logging.o
	@-rm -f /users/chm/BFA/rpc.statd.B ${BFA_SRC} ${BFA_OBJ}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${CSOURCE} -B"-d/users/chm/BFA/rpc.statd.B" logging.o ${BLIBRPC} ${BLIBC} 

sm_svc.o:	../sm_svc.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

sm_proc.o:	../sm_proc.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

sm_statd.o:	../sm_statd.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

tcp.o: ../../rpc.lockd/tcp.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../../rpc.lockd/tcp.c

udp.o: ../../rpc.lockd/udp.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../../rpc.lockd/udp.c

pmap.o: ../../rpc.lockd/pmap.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../../rpc.lockd/pmap.c

logging.o:	../../logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${800TARG} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
		${INS} rpc.statd etc root other 0500 

install:	all
	test "${INS}" && ${INS} ${800TARG}

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
