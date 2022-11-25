##
#	@(#)$Revision: 1.48.109.2 $	$Date: 92/03/06 16:09:16 $  
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

CSOURCE	= ypserv.c ypsrvancil.c ypserv_map.c ypsrv_proc.c trans_map.c yp_cache.c
OBJECTS = ypserv.o ypsrvancil.o ypserv_map.o ypsrv_proc.o trans_map.o yp_cache.o
BSOURCE	= ypserv.c ypsrvancil.c ypserv_map.c ypsrv_proc.c yp_cache.c
BFA_SRC = _ypserv.c _ypsrvancil.c _ypserv_map.c _ypsrv_proc.c _trans_map.c \
	_logging.c _yp_cache.o
BFA_OBJ = _ypserv.o _ypsrvancil.o _ypserv_map.o _ypsrv_proc.o _yp_cache.o

TARGETS	= ypserv longfiles
BFATARG	= ypserv.b longfiles.b

.SUFFIXES:.b
.c.o:
	@-rm -f $*.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $< 
.c:
	@-rm -f $*
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $* $< ${DBM_LIB} ${LIBS} ${LIBC}

.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? -B"-d/nfs/BFA/$*.B" ${BLIBDBM} ${BLIBC} 

all:		${TARGETS}

bfa:		${BFATARG}

ypserv:	${OBJECTS} logging.o
	@-rm -f $@
	$(CC) ${CFLAGS} -Wl,-a,archive ${LDFLAGS} ${IDIRS} -o $@ ${OBJECTS} logging.o ${DBM_LIB} ${LIBS} ${LIBC} 

ypserv.b:	${BSOURCE} _logging.o _trans_map.o
	@-rm -f /nfs/BFA/ypserv.B ${BFA_SRC} ${BFA_OBJ}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${BSOURCE} _logging.o _trans_map.o -B"-d/nfs/BFA/ypserv.B" ${BLIBDBM} ${BLIBC} 

_logging.o:     ${Nfs}/cmds/usr.etc/logging.c
	@-rm -f $@ _$*.c /nfs/BFA/logging.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/logging.c -B"-d/nfs/BFA/logging.B"

_trans_map.o:     ${Nfs}/cmds/usr.etc/ypserv/trans_map.c
	@-rm -f $@ _$*.c /nfs/BFA/trans_map.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/ypserv/trans_map.c -B"-d/nfs/BFA/trans_map.B"

logging.o:	../logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../logging.c

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
		${INS} ypserv usr/etc root other 0500 

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


