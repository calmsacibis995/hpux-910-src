##
#	@(#)$Revision: 1.21.109.2 $	$Date: 92/03/06 16:49:24 $
#	NFSSRC Makefile	2.2 86/04/17
#	Makefile nfs.1.0 85/06/85 SAI; from 4.49 83/09/27
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

CSOURCE	= ../rup.c ../rusers.c
800TARG	= rup rusers
BFATARG = rup.b rusers.b

all:		${800TARG}

bfa:		${BFATARG}

rup:	../rup.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

rusers:	../rusers.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

rup.b:	../rup.c
	rm -f /users/chm/BFA/rup.B $@ _rup.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rup.B"

rusers.b:	../rusers.c
	rm -f /users/chm/BFA/rusers.B $@ _rusers.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rusers.B"


lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f *.b _*.c

clobber:	clean
	-rm -f ${800TARG} ${BFATARG}

install:	all
	test "${INS}" && ${INS} ${800TARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

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

