##
#	@(#)$Revision: 1.67.109.2 $	$Date: 92/03/06 17:38:30 $
#	Makefile 1.1 86/02/03 (C) 1983 SMI; from UCB 4.27 83/07/06
#	/* (#)Makefile	2.1 86/04/17 NFSSRC */ 
#
SHELL	= /bin/sh

# C programs that live in $cwd and do not need explicit make lines.

STD	= nfsd biod fsirand 
STD_SRC	= nfsd.c biod.c portmap.c fsirand.c
STD_BFA	= nfsd.b biod.b portmap.b fsirand.b

TARGETS	= pcnfsd ypbind ${STD} portmap
BFATARG	= ${STD_BFA} pcnfsd.b ypbind.b
CSOURCE	= ${STD_SRC} pcnfsd ypbind.c

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DTRACEON ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DTRACEON ${CFLAGS}
DLFLAGS	= -g
BCFLAGS	= -DBFA ${CFLAGS}
BLFLAGS	= ${LDFLAGS}

.SUFFIXES:.b
.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $?  ${BLIBC} -B"-d/nfs/BFA/$*.B"

all:		${TARGETS}

bfa:		${BFATARG}

${STD}:		${STD_SRC}
	@-rm -f $@.o $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $@.c ${LIBC}

portmap:	portmap.c logging.o 
	@-rm -f $@.o $@
	$(CC) ${CFLAGS} -Wl,-a,archive ${LDFLAGS} ${IDIRS} -o $@ $@.c ${LIBC}

pcnfsd:		pcnfsd.c logging.o
	@-rm -f $@.o $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $@.c logging.o ${LIBC}

ypbind:		ypbind.c logging.o
	@-rm -f $@.o $@
	$(CC) ${CFLAGS} -Wl,-a,archive  ${LDFLAGS} ${IDIRS} -o $@ $@.c logging.o ${LIBC}

pcnfsd.b:		pcnfsd.c _logging.o
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ pcnfsd.c _logging.o ${BLIBC} -B"-d/nfs/BFA/$*.B"

ypbind.b:		ypbind.c _logging.o
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ypbind.c _logging.o ${BLIBC} -B"-d/nfs/BFA/$*.B"

_logging.o:     ${Nfs}/cmds/usr.etc/logging.c
	@-rm -f $@ _$*.c /nfs/BFA/logging.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/logging.c -B"-d/nfs/BFA/logging.B"

logging.o:	../usr.etc/logging.c
	@-rm -f $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../usr.etc/logging.c

lint:;		lint -bnx $ ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
	${INS} biod etc root other 0544 
	${INS} fsirand etc root other 0544 
	${INS} nfsd etc root other 0544 
	${INS} portmap etc root other 0544 
	${INS} pcnfsd etc root other 0544 
	${INS} ypbind etc root other 0544 
	${INS} netgroup etc/newconfig root other 0644 
	${INS} netnfsrc etc/newconfig root other 0544 
	${INS} rpc etc/newconfig root other 0644 
	${INS} vhe_list etc/newconfig root other 0444 

depend:;
	maketd ${CFLAGS} ${IDIRS} ${STD_SRC} > ,makedep
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


