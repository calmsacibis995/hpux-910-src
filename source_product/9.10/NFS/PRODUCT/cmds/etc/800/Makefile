##
#	@(#)$Revision: 1.28.109.2 $	$Date: 92/03/06 17:36:47 $
#	Makefile 1.1 86/02/03 (C) 1983 SMI; from UCB 4.27 83/07/06
#	/* (#)Makefile	2.1 86/04/17 NFSSRC */ 
#
SHELL	= /bin/ksh

# Inetd should be build by us.  We will have to find a way of doing
# it for us versus the integration group at ISO.
# This file (Makefile) is invoked by makefile. If you add new targets you
# must modify makefile.
CSOURCE	= ../ypbind.c ../biod.c ../portmap.c ../pcnfsd.c ../nfsd.c  ../fsirand.c
OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DTRACEON ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DTRACEON ${CFLAGS}
DLFLAGS	= -g
BCFLAGS	= -DBFA ${CFLAGS}
BLFLAGS	= ${LDFLAGS}

800TARG	= ypbind biod portmap pcnfsd nfsd fsirand
BFATARG	= ypbind.b biod.b portmap.b pcnfsd.b nfsd.b fsirand.b

all:		${800TARG}

bfa:		${BFATARG}

nfsd:	../nfsd.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${LIBS} ${ST_LIBC} 

logging.o:	../../usr.etc/logging.c
	$(CC) ${CFLAGS} ${IDIRS} -c $?

biod:	../biod.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${LIBS} ${ST_LIBC} 

inetd:	../inetd.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${LIBS} ${ST_LIBC} 

portmap:	../portmap.c
	$(CC) ${CFLAGS}  -Wl,-a,archive ${LDFLAGS} ${IDIRS} -o $@ $? ${LIBS} ${ST_LIBC}

pcnfsd:	../pcnfsd.c logging.o
	$(CC) ${CFLAGS}  ${LDFLAGS} ${IDIRS} -o $@ ../pcnfsd.c logging.o ${LIBS} ${ST_LIBC} 

fsirand:	../fsirand.c
	$(CC) ${CFLAGS}  ${LDFLAGS} ${IDIRS} -o $@ $? ${LIBS} ${ST_LIBC} 

ypbind:	../ypbind.c logging.o
	$(CC) ${CFLAGS}  -Wl,-a,archive ${LDFLAGS} ${IDIRS} -o $@ ../ypbind.c logging.o ${LIBS} ${ST_LIBC} 

nfsd.b:	../nfsd.c
	rm -f /users/chm/BFA/nfsd.B $@ _nfsd.c 
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $?  ${BLIBC} -B"-d/users/chm/BFA/nfsd.B"

ypbind.b:	../ypbind.c logging.b
	rm -f /users/chm/BFA/ypbind.B $@ _ypbind.c 
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../ypbind.c logging.o  ${BLIBC} -B"-d/users/chm/BFA/ypbind.B"

logging.b:	../../usr.etc/logging.c
	rm -f /users/chm/BFA/logging.B $@ _logging.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/logging.B"
	mv _logging.o logging.b 

biod.b:	../biod.c
	rm -f /users/chm/BFA/biod.B $@ _biod.c 
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $?  ${BLIBC}  -B"-d/users/chm/BFA/biod.B"

inetd.b:	../inetd.c
	rm -f /users/chm/BFA/inetd.B $@ _inetd.c 
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $?  ${BLIBC} -B"-d/users/chm/BFA/inetd.B"

portmap.b:	../portmap.c
	rm -f /users/chm/BFA/portmap.B $@ _portmap.c 
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $?  ${BLIBC} -B"-d/users/chm/BFA/portmap.B"

pcnfsd.b:	../pcnfsd.c logging.b
	rm -f /users/chm/BFA/pcnfsd.B $@ _pcnfsd.c 
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../pcnfsd.c logging.o  ${BLIBC} -B"-d/users/chm/BFA/pcnfsd.B"

fsirand.b:	../fsirand.c
	rm -f /users/chm/BFA/fsirand.B $@ _fsirand.c 
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $?  ${BLIBC}  -B"-d/users/chm/BFA/fsirand.B"

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b

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

