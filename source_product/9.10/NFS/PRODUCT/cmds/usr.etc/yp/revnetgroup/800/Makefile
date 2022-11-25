##
#	@(#)$Revision: 1.21.109.1 $	$Date: 91/11/19 14:21:58 $  
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

CSOURCE	= ../revnetgrp.c ../getgroup.c ../table.c ../util.c
800OBJS = revnetgrp.o getgroup.o table.o util.o
BFAOBJS = revnetgrp.b getgroup.b table.b util.b
800TARG	= revnetgroup
BFATARG	= revnetgroup.b

all:		${800TARG}

bfa:		${BFATARG}

${800TARG}:	${800OBJS}
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ${800OBJS} ${LIBS} ${ST_LIBC}

${BFATARG}:	${CSOURCE}
	@-rm -f /users/chm/BFA/${800TARG}.B ${BFAOBJS}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${CSOURCE} -B"-d/users/chm/BFA/${800TARG}.B"

revnetgrp.o:	../revnetgrp.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $@ $? 

getgroup.o:	../getgroup.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $@ $? 

table.o:	../table.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $@ $? 

util.o:		../util.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $@ $?


lint:;		lint -bnx ${CFLAGS} ${LDFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f *.b _*.c /users/chm/BFA/revnetgroup.B

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

