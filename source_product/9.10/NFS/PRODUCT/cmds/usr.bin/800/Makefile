##
#	@(#)$Revision: 1.20.109.1 $	$Date: 91/11/19 14:07:16 $  
#/* NFSSRC (#)Makefile	2.2 86/04/17 */
#	Makefile nfs.1.0 85/06/28 SAI; from 4.29 83/08/20
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

CSOURCE	= ../ypcat.c ../ypmatch.c ../yppasswd.c ../ypwhich.c 
800TARG	= ypcat ypmatch yppasswd ypwhich
BFATARG	= ypcat.b ypmatch.b yppasswd.b ypwhich.b

all:		${800TARG}

bfa:		${BFATARG}

ypcat:	../ypcat.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

ypwhich:	../ypwhich.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

ypmatch:	../ypmatch.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

yppasswd:	../yppasswd.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

ypcat.b:	../ypcat.c
	rm -f /users/chm/BFA/ypcat.B $@ _ypcat.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/ypcat.B"

ypwhich.b:	../ypwhich.c
	rm -f /users/chm/BFA/ypwhich.B $@ _ypwhich.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/ypwhich.B"

ypmatch.b:	../ypmatch.c
	rm -f /users/chm/BFA/ypmatch.B $@ _ypmatch.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/ypmatch.B"

yppasswd.b:	../yppasswd.c
	rm -f /users/chm/BFA/yppasswd.B $@ _yppasswd.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/yppasswd.B"

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

