##
#	@(#)$Revision: 1.15.109.1 $	$Date: 91/11/19 14:16:25 $  
# (#)Makefile	1.2 87/08/07 3.2/4.3NFSSRC 
# (#)Makefile	1.2 86/12/30 NFSSRC
#
# (#)Makefile 1.1 86/09/24 Copyright Sun Micro 1986
#
# Makefile for rpc.lockd - the network lock manager

SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DTRACEON ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DTRACEON ${CFLAGS}
DLFLAGS	= -g 
BCFLAGS	= -DBFA ${CFLAGS}
BLFLAGS	= ${LDFLAGS}

CSOURCE = prot_main.c prot_lock.c prot_pklm.c prot_pnlm.c prot_msg.c\
	  prot_proc.c prot_alloc.c prot_priv.c prot_free.c prot_share.c pmap.c\
	  tcp.c udp.c xdr_nlm.c xdr_klm.c xdr_sm.c sm_monitor.c sem.c\
  	  svc_udp.c hash.c prot_libr.c nfs_fcntl.s ../../usr.etc/logging.c

OBJECTS = prot_main.o prot_lock.o prot_pklm.o prot_pnlm.o prot_msg.o\
	  prot_proc.o prot_alloc.o prot_priv.o prot_free.o prot_share.o pmap.o\
	  tcp.o udp.o xdr_nlm.o xdr_klm.o xdr_sm.o sm_monitor.o sem.o\
  	  svc_udp.o hash.o prot_libr.o nfs_fcntl.o logging.o

BSOURCE = prot_main.c prot_lock.c prot_pklm.c prot_pnlm.c prot_msg.c\
	  prot_proc.c prot_alloc.c prot_priv.c prot_free.c prot_share.c pmap.c\
	  udp.c xdr_nlm.c xdr_klm.c xdr_sm.c sm_monitor.c sem.c\
  	  svc_udp.c hash.c prot_libr.c nfs_fcntl.s 

BFA_SRC = _prot_main.c _prot_lock.c _prot_pklm.c _prot_pnlm.c _prot_msg.c\
	  _prot_proc.c _prot_alloc.c _prot_priv.c _prot_free.c _prot_share.c\
	  _pmap.c _tcp.c _udp.c _xdr_nlm.c _xdr_klm.c _xdr_sm.c _sm_monitor.c\
	  _svc_udp.c _hash.c _prot_libr.c _logging.c _sem.c

BFA_OBJ = _prot_main.o _prot_lock.o _prot_pklm.o _prot_pnlm.o _prot_msg.o\
	  _prot_proc.o _prot_alloc.o _prot_priv.o _prot_free.o _prot_share.o\
	  _pmap.o _udp.o _xdr_nlm.o _xdr_klm.o _xdr_sm.o _sm_monitor.o _sem.o\
  	  _svc_udp.o _hash.o _prot_libr.o nfs_fcntl.o 


TARGETS = rpc.lockd
BFATARG	= rpc.lockd.b
HDRS=   prot_lock.h prot_time.h sm_res.h priv_prot.h

.SUFFIXES:.b
.c.o:
	@-rm -f $*
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $< 
.c:
	@-rm -f $*
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $* $< ${RPCSVC_LIB} ${LIBS} ${LIBC}

.c.b:
	@-rm -f /BFA/$*.B $@ _$< _$*.o
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? -B"-d/BFA/$*.B" ${BLIBRPC} ${BLIBC} 

all: 		${TARGETS}

bfa:		${BFATARG}

rpc.lockd:	${OBJECTS}
	@-rm -f $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS}  -o $@ ${OBJECTS} ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.lockd.b:	${BSOURCE} _logging.o _tcp.o
	@-rm -f /BFA/rpc.lockd.B ${BFA_SRC} ${BFA_OBJ}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${BSOURCE} _logging.o _tcp.o -B"-d/BFA/rpc.lockd.B" ${BLIBRPC} ${BLIBC} 

nfs_fcntl.o: nfs_fcntl.s
	$(AS) -o nfs_fcntl.o nfs_fcntl.s

_logging.o:     ${Nfs}/cmds/usr.etc/logging.c
	@-rm -f $@ _$*.c /nfs/BFA/logging.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/logging.c -B"-d/nfs/BFA/logging.B"

_tcp.o:     ${Nfs}/cmds/usr.etc/rpc.lockd/tcp.c
	@-rm -f $@ _$*.c /nfs/BFA/tcp.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/rpc.lockd/tcp.c -B"-d/nfs/BFA/tcp.B"

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
		${INS} rpc.lockd usr/etc root other 0500 

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

