##
#	@(#)$Revision: 1.31.109.2 $	$Date: 92/03/06 15:18:26 $  
# (#)Makefile 1.1 86/02/05 Copyright (c) 1984 by Sun Microsystems, Inc.
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

CSOURCE	= ../ypserv.c ../ypserv_map.c ../ypsrv_proc.c ../trans_map.c \
	  ../ypsrvancil.c ../../logging.c ../yp_cache.c
800OBJS = ypserv.o ypserv_map.o ypsrv_proc.o trans_map.o ypsrvancil.o \
	logging.o yp_cache.o
BFAOBJS = ypservb.b ypserv_map.b ypsrv_proc.b trans_map.b ypsrvancil.b \
	logging.b yp_cache.b
800TARG	= ypserv longfiles
BFATARG	= ypserv.b longfiles.b

all:		${800TARG}

bfa:		${BFATARG}

ypserv:	${800OBJS}
	LDOPTS="-a archive"
	$(CC) ${CFLAGS}  -Wl,-a,archive ${LDFLAGS} ${IDIRS} -o ypserv ${800OBJS} ${DBM_LIB}         ${LIBS} ${ST_LIBC} 

ypserv.b:	${CSOURCE} 
	@-rm -f /users/chm/BFA/ypserv.B ${BFAOBJS}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${CSOURCE} -B"-d/users/chm/BFA/ypserv.B" ${BLIBDBM} ${BLIBC} 

longfiles:	../longfiles.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${LIBS} ${ST_LIBC}

longfiles.b:	../longfiles.c
	rm -f /users/chm/BFA/longfiles.B $@ _longfiles.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBC} -B"-d/users/chm/BFA/longfiles.B"

ypserv.o:	../ypserv.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

ypsrvancil.o:	../ypsrvancil.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

ypserv_map.o:	../ypserv_map.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $? 

ypsrv_proc.o:	../ypsrv_proc.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

trans_map.o:	../trans_map.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $? 

yp_cache.o:	../yp_cache.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $? 

logging.o:	../../logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $? 

lint:;		lint -bnx ${CFLAGS} ${LDFLAGS} ${IDIRS} ${CSOURCE}

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
