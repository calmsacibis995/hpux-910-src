#	@(#)$Revision: 1.17.109.2 $	$Date: 92/03/06 16:31:21 $  
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
BLFLAGS	= ${LDFLAGS}

#
CSOURCE = rexd.c mount_nfs.c on.c where.c unix_login.c bsdtermio.c
OBJETCS = rexd.o mount_nfs.o on.o where.o unix_login.o bsdtermio.o
BFA_SRC = _rexd.c _mount_nfs.c _on.c _where.c _unix_login.c
BFA_OBJ = _rexd.o _mount_nfs.o _on.o _where.o _unix_login.o 
BSDOBJS = getwd.o killpg.o

TARGETS = on rexd
BFATARG = on.b rexd.b

# MISC	are targets which are needed for rpc.*.c; check_exit needs to be
#	a .c file since each may have different TRACEON settings.
#MISC	= ../logging.o ../check_exit.c
MISC	= bsdtermio.o
BFAMISC = _bsdtermio.o _logging.o 

.SUFFIXES:.b
.c.o:
	@-rm -f $*
	$(CC) ${CFLAGS} -Wc,-Nd2000 -DHFS ${LDFLAGS} ${IDIRS} -c $< 
.c:
	$(CC) ${CFLAGS} -Wc,-Nd2000 -DHFS ${LDFLAGS} ${IDIRS} -o $* $< ${RPCSVC_LIB} ${LIBS} ${LIBC} ${BSDOBJS}

.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	${BFACC} ${BCFLAGS} -Wc,-Nd2000 -DHFS ${BLFLAGS} ${IDIRS} -o $@ $? -B"-d/nfs/BFA/$*.B" ${BLIBRPC} ${BLIBC} ${BSDOBJS}
	
all:		${TARGETS}

bfa:		${BFATARG}

rexd: rexd.o mount_nfs.o unix_login.o logging.o ${MISC}
	@-rm -f check_exit.o $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(CC) ${CFLAGS} -Wc,-Nd2000 -DHFS ${LDFLAGS} ${IDIRS} -o rexd rexd.o mount_nfs.o unix_login.o ${MISC} ${RPCSVC_LIB} logging.o ${LIBS} ${LIBC} ${BSDOBJS} -lV3

on: on.o where.o ${MISC}
	@-rm -f check_exit.o $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(CC) ${CFLAGS} -Wc,-Nd2000 -DHFS ${LDFLAGS} ${IDIRS} -o on on.o where.o ${MISC} ${RPCSVC_LIB} ${LIBS}  ${LCURSES} ${LIBC} ${BSDOBJS}

under: under.o mount_nfs.o ${MISC}
	@-rm -f check_exit.o $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(CC) ${CFLAGS} -Wc,-Nd2000 -DHFS ${LDFLAGS} ${IDIRS} -o under under.o mount_nfs.o ${MISC} ${RPCSVC_LIB} logging.o ${LIBS} ${LIBC} ${BSDOBJS}

where: where_main.o where.o ${MISC}
	@-rm -f check_exit.o $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(CC) ${CFLAGS} -Wc,-Nd2000 -DHFS ${LDFLAGS} ${IDIRS} -o where where_main.o where.o ${MISC} ${RPCSVC_LIB} ${LIBS} ${LIBC} ${BSDOBJS}

where.b:	where_main.c where.c ${BFAMISC}
	@-rm -f /nfs/BFA/where.B _where_main.o _where.o _where.c _where_main.c where.b
	@-ar x ${LBSD} ${BSDOBJS}
	${BFACC} ${BCFLAGS} -Wc,-Nd2000 -DHFS ${BLFLAGS} ${IDIRS} -o $@ where_main.c where.c ${BFAMISC} -B"-d/nfs/BFA/where.B" ${BLIBRPC} ${BLIBC} ${BSDOBJS}

on.b:	on.c where.c ${BFAMISC}
	@-rm -f /nfs/BFA/on.B _on.c _where.c _on.o _where.o on.b
	@-ar x ${LBSD} ${BSDOBJS}
	${BFACC} ${BCFLAGS} -Wc,-Nd2000 -DHFS ${BLFLAGS} ${IDIRS} -o $@ on.c where.c ${BFAMISC} -B"-d/nfs/BFA/on.B" ${BLIBRPC} ${BLIBC} -lcurses ${BSDOBJS} 

rexd.b:	rexd.c mount_nfs.c unix_login.c ${BFAMISC}
	@-rm -f /nfs/BFA/rexd.B _rexd.c _mount_nfs.c _unix_login.c _rexd.o _mount_nfs.o _unix_login.o rexd.b
	@-ar x ${LBSD} ${BSDOBJS}
	${BFACC} ${BCFLAGS} -Wc,-Nd2000 -DHFS ${BLFLAGS} ${IDIRS} -o $@ rexd.c mount_nfs.c unix_login.c ${BFAMISC} -B"-d/nfs/BFA/rexd.B" ${BLIBRPC} ${BLIBC} ${BSDOBJS}

under.b:	under.c mount_nfs.c ${BFAMISC}
	@-rm -f /nfs/BFA/under.B _under.c $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(BFACC) ${BCFLAGS} -Wc,-Nd2000 -DHFS ${BLFLAGS} ${IDIRS} -o under.b under.c mount_nfs.c ${BFAMISC} -B"-d/nfs/BFA/$*.B" ${BLIBRPC} ${BLIBC} ${BSDOBJS} 

_logging.o:    ${Nfs}/cmds/usr.etc/logging.c
	@-rm -f $@ _$*.c /nfs/BFA/logging.B
	@-ar x ${LBSD} ${BSDOBJS}
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/logging.c -B"-d/nfs/BFA/logging.B"

_check_exit.o:  ${Nfs}/cmds/usr.etc/check_exit.c
	@-rm -f $@ _$*.c /nfs/BFA/check_exit.B
	@-ar x ${LBSD} ${BSDOBJS}
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/check_exit.c -B"-d/nfs/BFA/check_exit.B"

_bsdtermio.o:     ${Nfs}/cmds/usr.etc/rexd/bsdtermio.c
	@-rm -f $@ _$*.c /nfs/BFA/bsdtermio.B
	@-ar x ${LBSD} ${BSDOBJS}
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/rexd/bsdtermio.c -B"-d/nfs/BFA/bsdtermio.B"

logging.o:	../logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../logging.c $(RPCSVC_LIB}

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
		cp rexd rpc.rexd
		${INS} rpc.rexd usr/etc root other 0500 
		rm rpc.rexd
		${INS} on usr/etc root other 0555

#	depends upon $DESTDIR/usr/etc
install: rexd on
	install -s -m 500 rexd ${DESTDIR}/usr/etc/rpc.rexd
	install -s -m 555 on ${DESTDIR}/usr/bin/on

depend:;
	maketd $CFLAGS} ${IDIRS} ${CSOURCE} > ,makedep
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
