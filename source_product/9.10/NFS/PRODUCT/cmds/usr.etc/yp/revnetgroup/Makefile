##
#	@(#)$Revision: 1.34.109.1 $	$Date: 91/11/19 14:22:04 $  
#  (#)Makefile 1.1 86/02/05 Copyright (c) 1985 by Sun Microsystems, Inc.
#  (#)Makefile	2.1 86/04/16 NFSSRC
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

CSOURCE	= revnetgrp.c getgroup.c table.c util.c
OBJECTS = revnetgrp.o getgroup.o table.o util.o
BFA_SRC = _revnetgrp.c _getgroup.c _table.c _util.c
BFA_OBJ = _revnetgrp.o _getgroup.o _table.o _util.o

TARGETS	= revnetgroup
BFATARG	= revnetgrp.b

.SUFFIXES:.b
.c.o:
	@-rm -f $*.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $< ${LIBC}

all:		${TARGETS}

bfa:		${BFATARG}

${TARGETS}:	${OBJECTS}
	@-rm -f $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ${OBJECTS} ${LIBC}

${BFATARG}:	${CSOURCE}
	@-rm -f /BFA/${TARGETS}.B ${BFA_SRC} ${BFA_OBJ}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${CSOURCE} -B"-d/BFA/${TARGETS}.B"

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f *.b _*.c /BFA/revnetgroup.B

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

linstall:	all
		${INS} revnetgroup usr/etc/yp root other 0500

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

depend:;
	maketd ${CFLAGS} ${CSOURCE} > ,makedep
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


