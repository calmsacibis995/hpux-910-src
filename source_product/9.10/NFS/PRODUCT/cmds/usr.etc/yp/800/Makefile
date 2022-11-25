##
#	@(#)$Revision: 1.25.109.1 $	$Date: 91/11/19 14:20:26 $  
# (#)Makefile 1.1 86/02/05 (C) 1985 SMI
# (#)Makefile	2.1 86/04/16 NFSSRC
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

CSOURCE	= ../makedbm.c ../stdhosts.c ../yppush.c ../ypset.c ../yppoll.c \
	  ../ypxfr.c
800TARG	= makedbm stdhosts yppush ypset yppoll ypxfr
BFATARG	= makedbm.b stdhosts.b yppush.b ypset.b yppoll.b ypxfr.b 
SCRIPT	= ypinit ypmake.800 ypxfr_1perda ypxfr_1perhr ypxfr_2perda make.script

all:		${800TARG}

bfa:		${BFATARG}

makedbm:	../makedbm.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${DBM_LIB} ${LIBS} ${ST_LIBC}

stdhosts:	../stdhosts.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${DBM_LIB} ${LIBS} ${ST_LIBC}

yppush:	../yppush.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${DBM_LIB} ${LIBS} ${ST_LIBC}

ypset:	../ypset.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${DBM_LIB} ${LIBS} ${ST_LIBC}

yppoll:	../yppoll.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${DBM_LIB} ${LIBS} ${ST_LIBC}

ypxfr:	../ypxfr.c ../../ypserv/trans_map.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ../ypxfr.c ../../ypserv/trans_map.c ${DBM_LIB} ${LIBS} ${ST_LIBC}

makedbm.b:	../makedbm.c
	rm -f /users/chm/BFA/makedbm.B $@ _makedbm.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBDBM}  ${BLIBC} -B"-d/users/chm/BFA/makedbm.B"

stdhosts.b:	../stdhosts.c
	rm -f /users/chm/BFA/stdhosts.B $@ _stdhosts.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBDBM}  ${BLIBC} -B"-d/users/chm/BFA/stdhosts.B"

yppush.b:	../yppush.c
	rm -f /users/chm/BFA/yppush.B $@ _yppush.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBDBM}  ${BLIBC} -B"-d/users/chm/BFA/yppush.B"

ypset.b:	../ypset.c
	rm -f /users/chm/BFA/ypset.B $@ _ypset.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBDBM}  ${BLIBC} -B"-d/users/chm/BFA/ypset.B"

yppoll.b:	../yppoll.c
	rm -f /users/chm/BFA/yppoll.B $@ _yppoll.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBDBM}  ${BLIBC} -B"-d/users/chm/BFA/yppoll.B"

ypxfr.b:	../ypxfr.c ../../ypserv/trans_map.c
	rm -f /users/chm/BFA/ypxfr.B $@ _ypxfr.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../ypxfr.c ../../ypserv/trans_map.c ${BLIBDBM}  ${BLIBC} -B"-d/users/chm/BFA/ypxfr.B"

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${800TARG} ${BFATARG}

install:	all
	test "${INS}" && ${INS} ${800TARG} 
	cd ..; ${INS} ${SCRIPT}

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

