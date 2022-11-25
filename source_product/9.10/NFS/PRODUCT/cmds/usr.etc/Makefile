##
#	@(#)$Revision: 1.65.109.3 $	$Date: 92/07/31 11:32:40 $  
# (#)Makefile 1.1 86/02/05 SMI; from UCB 4.27 83/07/06
# (#)Makefile	2.4 86/04/17 NFSSRC
#
SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DTRACEON ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DTRACEON ${CFLAGS}
DLFLAGS	= -g 
BCFLAGS	= -DBFA ${CFLAGS}
BCFLAGS2 = -DBFA ${CFLAGS} ${CFLAGS2}
BLFLAGS	= ${LDFLAGS}

# Programs that live in subdirectories, and have makefiles of their own.

SUBDIR	= nis ypserv

CSOURCE	= rpcinfo.c nfsstat.c rpc.mountd.c rpc.rstatd.c rpc.rwalld.c \
	rpc.ruserd.c rpc.sprayd.c rpc.ypassd.c rwall.c showmount.c spray.c \
	rpc.rquota.c exportfs.c
TARGETS	= rpcinfo nfsstat rpc.mountd rpc.rstatd rpc.rwalld rpc.rusersd \
	rpc.sprayd rpc.yppasswdd rwall showmount spray rpc.rquotad  \
	exportfs

BFATARG	= rpcinfo.b nfsstat.b rpc.rstatd.b rpc.rwalld.b rpc.rusersd.b \
	  rpc.sprayd.b rpc.ypassd.b rwall.b showmount.b spray.b rpc.mountd.b \
	  rpc.rquotad.b exportfs.b

# MISC	are targets which are needed for rpc.*.c; check_exit needs to be
#	a .c file since each may have different TRACEON settings.

MISC	= logging.o check_exit.c
BFAMISC	= _logging.o _check_exit.o

.SUFFIXES:.b
.c:
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $* $< ${RPCSVC_LIB} ${LIBS} ${LIBC}
.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/nfs/BFA/$*.B" 

all:		${TARGETS}

bfa:		${BFATARG}

${SUBDIR}:;
	cd $@; $(MAKE) ${MFLAGS}

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f *.b _*.c 

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall: 	all
	${INS} rpcinfo usr/etc root other 0555
	${INS} nfsstat usr/etc root other 4555
	${INS} rpc.mountd usr/etc root other 0544
	${INS} rpc.rstatd usr/etc root other 0544
	${INS} rpc.rwalld usr/etc root other 0544
	${INS} rpc.rusersd usr/etc root other 0544
	${INS} rpc.sprayd usr/etc root other 0544
	cp rpc.ypassd rpc.yppasswdd
	${INS} rpc.yppasswdd usr/etc root other 0500
	${INS} rwall usr/etc root other 0555
	${INS} showmount usr/etc root other 0555
	${INS} spray usr/etc root other 0555
	${INS} exportfs usr/etc root other 0744
	rm rpc.yppasswdd
	
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

rpc.rusersd:	rpc.ruserd.c ${MISC}
	@-rm -f check_exit.o $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ rpc.ruserd.c ${MISC} ${RPCSVC_LIB} ${LIBS} ${LIBC}

# We don't want rpc.mountd to be stripped so that is why we don't use LDFLAGS
rpc.mountd:	rpc.mountd.c ${MISC}
	@-rm -f check_exit.o $@
	$(CC) ${CFLAGS} ${CFLAGS2} ${IDIRS} -o $@ rpc.mountd.c ${MISC} ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.rstatd:	rpc.rstatd.c ${MISC}
	@-rm -f check_exit.o $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ rpc.rstatd.c ${MISC} ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.rwalld:	rpc.rwalld.c ${MISC}
	@-rm -f check_exit.o $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ rpc.rwalld.c ${MISC} ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.sprayd:	rpc.sprayd.c ${MISC}
	@-rm -f check_exit.o $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ rpc.sprayd.c ${MISC} ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.yppasswdd:	rpc.ypassd.c logging.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ rpc.ypassd.c logging.o ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.rquotad:	rpc.rquota.c logging.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ rpc.rquota.c logging.o ${RPCSVC_LIB} ${LIBS} ${LIBC}

rpc.rusersd.c:	rpc.ruserd.c
	@-ln -f $? $@

exportfs:	exportfs.c issubdir.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ exportfs.c issubdir.o ${LIBS} ${LIBC}

rpc.rusersd.b:	rpc.rusersd.c ${BFAMISC}
	@-rm -f check_exit.o $@ _$*.c _$*.o /nfs/BFA/$*.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ rpc.rusersd.c ${BFAMISC} ${BLIBRPC} ${BLIBC} -B"-d/nfs/BFA/$*.B" 
	@-rm -f rpc.rusersd.c

rpc.mountd.b:	rpc.mountd.c ${BFAMISC}
	@-rm -f check_exit.o $@ _$*.c _$*.o /nfs/BFA/$*.B
	$(BFACC) ${BCFLAGS2} ${BLFLAGS} ${IDIRS} -o $@ rpc.mountd.c ${BFAMISC} ${BLIBRPC} ${BLIBC} -B"-d/nfs/BFA/$*.B" 

rpc.rstatd.b:	rpc.rstatd.c ${BFAMISC}
	@-rm -f check_exit.o $@ _$*.c _$*.o /nfs/BFA/$*.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ rpc.rstatd.c ${BFAMISC} ${BLIBRPC} ${BLIBC} -B"-d/nfs/BFA/$*.B" 

rpc.rwalld.b:	rpc.rwalld.c ${BFAMISC}
	@-rm -f check_exit.o $@ _$*.c _$*.o /nfs/BFA/$*.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ rpc.rwalld.c ${BFAMISC} ${BLIBRPC} ${BLIBC} -B"-d/nfs/BFA/$*.B" 

rpc.sprayd.b:	rpc.sprayd.c ${BFAMISC}
	@-rm -f check_exit.o $@ _$*.c _$*.o /nfs/BFA/$*.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ rpc.sprayd.c ${BFAMISC} ${BLIBRPC} ${BLIBC} -B"-d/nfs/BFA/$*.B" 

rpc.ypassd.b:	rpc.ypassd.c _logging.o
	@-rm -f $@ _$*.c _$*.o /nfs/BFA/$*.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ rpc.ypassd.c _logging.o  ${BLIBRPC} ${BLIBC} -B"-d/nfs/BFA/$*.B"

_logging.o:	${Nfs}/cmds/usr.etc/logging.c
	@-rm -f $@ _$*.c /nfs/BFA/logging.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/logging.c -B"-d/nfs/BFA/logging.B"

_check_exit.o:	${Nfs}/cmds/usr.etc/check_exit.c
	@-rm -f $@ _$*.c /nfs/BFA/check_exit.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/check_exit.c -B"-d/nfs/BFA/check_exit.B"

logging.o:	logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c logging.c

# DO NOT DELETE THIS LINE -- maketd depends upon it


