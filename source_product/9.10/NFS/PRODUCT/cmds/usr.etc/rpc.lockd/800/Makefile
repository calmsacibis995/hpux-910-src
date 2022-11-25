##
#	@(#)$Revision: 1.9.109.2 $	$Date: 92/03/06 16:25:55 $  
# (#)Makefile	1.2 87/08/07 3.2/4.3NFSSRC 
# (#)Makefile	1.2 86/12/30 NFSSRC
#
# (#)Makefile 1.1 86/09/24 Copyright Sun Micro 1986
#
# Makefile for rpc.lockd - the network lock manager

SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS} -DBSD_ONLY
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DTRACEON ${CFLAGS} -DBSD_ONLY
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DTRACEON ${CFLAGS} -DBSD_ONLY
DLFLAGS	= -g 
BCFLAGS	= -DBFA ${CFLAGS} -DBSD_ONLY
BLFLAGS	= ${LDFLAGS}

CSOURCE = ../prot_lock.c ../prot_pklm.c ../prot_pnlm.c \
	  ../prot_msg.c ../prot_proc.c ../prot_alloc.c ../prot_priv.c\
	  ../prot_free.c ../prot_share.c ../pmap.c \
	  ../tcp.c ../udp.c ../xdr_nlm.c ../xdr_klm.c ../xdr_sm.c ../sem.c\
	  ../sm_monitor.c ../svc_udp.c ../hash.c ../prot_libr.c ../prot_main.c 

800OBJS = prot_lock.o prot_pklm.o prot_pnlm.o prot_msg.o\
	  prot_proc.o prot_alloc.o prot_priv.o\
	  prot_free.o prot_share.o pmap.o \
	  tcp.o udp.o xdr_nlm.o xdr_klm.o xdr_sm.o sem.o sm_monitor.o \
	  svc_udp.o hash.o prot_libr.o prot_main.o nfs_fcntl.o logging.o

# NOTE: nfs_fnctl.o not included in BFAOBJ because it is not really C code
BFAOBJ = prot_main.o prot_lock.o prot_pklm.o prot_pnlm.o prot_msg.o\
	 prot_proc.o prot_alloc.o prot_priv.o\
	 tcp.o udp.o xdr_nlm.o xdr_klm.o xdr_sm.o sem.o sm_monitor.o \
	 svc_udp.o hash.o prot_libr.o 


800TARG = rpc.lockd
TARGETS = ${800TARG}
BFATARG	= rpc.lockd.b
HDRS=   prot_lock.h prot_time.h sm_res.h priv_prot.h

all: 		${800TARG}

bfa:		${BFATARG}

rpc.lockd: ${800OBJS}
	@-rm -f $@
	$(CC) ${CFLAGS} -DBSD_ONLY ${LDFLAGS} ${IDIRS}  -o $@ ${800OBJS} ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.lockd.b:	${CSOURCE} 
	@-rm -f /users/chm/BFA/rpc.lockd.B ${BFA_SRC} ${BFA_OBJ}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${CSOURCE} -B"-d/users/chm/BFA/rpc.lockd.B" logging.o nfs_fcntl.o ${BLIBRPC} ${BLIBC} 

prot_main.o:	../prot_main.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_lock.o:	../prot_lock.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_pklm.o:	../prot_pklm.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_pnlm.o:	../prot_pnlm.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_msg.o:	../prot_msg.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_proc.o:	../prot_proc.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_alloc.o:	../prot_alloc.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_priv.o:	../prot_priv.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_free.o:	../prot_free.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_share.o:	../prot_share.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

pmap.o:	../pmap.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

tcp.o:	../tcp.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

udp.o:	../udp.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

xdr_nlm.o:	../xdr_nlm.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

xdr_klm.o:	../xdr_klm.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

xdr_sm.o:	../xdr_sm.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

sm_monitor.o:	../sm_monitor.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

svc_udp.o:	../svc_udp.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

sem.o:	../sem.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

hash.o:	../hash.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

prot_libr.o:	../prot_libr.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

logging.o:	../../logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

lint:;		lint -bnx ${CFLAGS} -DBSD_ONLY ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
		${INS} rpc.lockd etc root other 0500 

depend:;
	maketd ${CFLAGS} -DBSD_ONLY ${IDIRS} ${CSOURCE} > ,makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' > ,eddep
	@echo '$$r ,makedep' >> ,eddep
	@echo 'w' >> ,eddep
	cp Makefile Makefile.bak
	ed - Makefile < ,eddep
	rm -f ,eddep ,makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile

# NOTE: The following defines and target are used to produce the system
# call stub for calling nfs_fcntl on the 800.  This may just go away on the
# 800 if nfs_fcntl.o goes into libc.  At any rate, the SYS.h in this
# directory should really be the real one they are currently using.  However,
# we don't have the ability to pull that at the moment, so just do this
# for now.  This code was copied from the 800 stub makefile.  DS

CPPFLAGS= -P -DAS -DLOCORE ${CFLAGS} ${IDIRS}
PREFIX=${TOOL_DIR}/lib/pcc_prefix.s
SED=	/bin/sed
SEDFLAGS= -e '/^[ 	]*$$/d' -e 's/^\./	\./' -e 's/; _/;/' -e 'y/; /\n	/' -e 's/ /	/'

nfs_fcntl.o: SYS.h nfs_fcntl.c
	${CPP} ${CPPFLAGS} nfs_fcntl.c | ${SED} ${SEDFLAGS} \
		| cat ${PREFIX} - | ${ASSEM} ${ASMFLAGS} -o nfs_fcntl.o

# DO NOT DELETE THIS LINE -- maketd depends upon it
