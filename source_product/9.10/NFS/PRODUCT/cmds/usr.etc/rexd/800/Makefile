#	@(#)$Revision: 1.8.109.1 $	$Date: 91/11/19 14:14:57 $  
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

CSOURCE = ../rexd.c ../mount_nfs.c ../on.c ../where.c ../unix_login.c \
	  ../bsdtermio.c 
800OBJS = rexd.o mount_nfs.o on.o where.o unix_login.o bsdtermio.o 
BFAOBJS = rexd.b mount_nfs.b on.b where.b unix_login.b bsdtermio.b 
BSDOBJS = getwd.o killpg.o

800TARG = on rexd  
BFATARG = on.b rexd.b 

# MISC	are targets which are needed for rpc.*.c; check_exit needs to be
#	a .c file since each may have different TRACEON settings.
#MISC	= ../../logging.o ../../check_exit.c
MISC	= bsdtermio.o
BFAMISC = ../bsdtermio.c

all:		${800TARG}

bfa:		${BFATARG}

rexd: rexd.o mount_nfs.o unix_login.o logging.o ${MISC}
	@-rm -f check_exit.o $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -o rexd rexd.o mount_nfs.o unix_login.o logging.o ${MISC} ${RPCSVC_LIB} ${LIBS} ${LIBC} ${BSDOBJS} -lV3

on: on.o where.o ${MISC}
	@-rm -f check_exit.o $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -o on on.o where.o ${MISC} ${RPCSVC_LIB} ${LIBS} ${LCURSES} ${LIBC} ${BSDOBJS}

under: under.o mount_nfs.o ${MISC}
	@-rm -f check_exit.o $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -o under under.o mount_nfs.o ${MISC} logging.o ${RPCSVC_LIB} ${LIBS} ${LIBC} ${BSDOBJS}

where: where_main.o where.o ${MISC}
	@-rm -f check_exit.o $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -o where where_main.o where.o ${MISC} ${RPCSVC_LIB} ${LIBS} ${LIBC} ${BSDOBJS}

where.b:	../where_main.c ../where.c ${BFAMISC}
	@-rm -f /users/chm/BFA/where.B where_main.b where.b where.b where_main.b
	@-ar x ${LBSD} ${BSDOBJS}
	${BFACC} ${BCFLAGS} -DHFS ${BLFLAGS} ${IDIRS} -o $@ ../where_main.c ../where.c ${BFAMISC} -B"-d/users/chm/BFA/where.B" ${BLIBRPC} ${BLIBC} ${BSDOBJS}

on.b:	../on.c ../where.c ${BFAMISC}
	@-rm -f /users/chm/BFA/on.B on.b where.b 
	@-ar x ${LBSD} ${BSDOBJS}
	${BFACC} ${BCFLAGS} -DHFS ${BLFLAGS} ${IDIRS} -o $@ ../on.c ../where.c ${BFAMISC} -B"-d/users/chm/BFA/on.B" ${BLIBRPC} ${BLIBC} ${LCURSES} ${BSDOBJS}

rexd.b:	../rexd.c ../mount_nfs.c ../unix_login.c ../../logging.c ${BFAMISC}
	@-rm -f /users/chm/BFA/rexd.B rexd.b mount_nfs.b unix_login.b 
	@-ar x ${LBSD} ${BSDOBJS}
	${BFACC} ${BCFLAGS} -DHFS ${BLFLAGS} ${IDIRS} -o $@ ../rexd.c ../mount_nfs.c ../unix_login.c ../../logging.c ${BFAMISC} -B"-d/users/chm/BFA/on.B" ${BLIBRPC} ${BLIBC} ${BSDOBJS}

under.b:	../under.c ../mount_nfs.c ${BFAMISC}
	@-rm -f /users/chm/BFA/under.B check_exit.b $@
	@-ar x ${LBSD} ${BSDOBJS}
	$(BFACC) ${BCFLAGS} -DHFS ${BLFLAGS} ${IDIRS} -o under.b ../under.c ../mount_nfs.c ${BFAMISC} -B"-d/users/chm/BFA/$*.B" ${BLIBRPC} ${BLIBC} ${BSDOBJS}

logging.o:	../../logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../../logging.c

rexd.o:	../rexd.c
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -c $?

mount_nfs.o:	../mount_nfs.c
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -c $?

on.o:	../on.c
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -c $?

where.o:	../where.c
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -c $?

where_main.o:	../where_main.c
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -c $?

unix_login.o:	../unix_login.c
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -c $?

under.o:	../under.c
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -c $?

bsdtermio.o:	../bsdtermio.c
	$(CC) ${CFLAGS} -DHFS ${LDFLAGS} ${IDIRS} -c $?

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${800TARG} ${BFATARG}

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
