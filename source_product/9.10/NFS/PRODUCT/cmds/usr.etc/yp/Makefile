##
#	@(#)$Revision: 1.47.109.1 $	$Date: 91/11/19 14:20:33 $  
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

CSOURCE	= makedbm.c stdhosts.c yppush.c ypset.c yppoll.c ypxfr.c
TARGETS	= makedbm stdhosts yppush ypset yppoll ypxfr
BFATARG	= makedbm.b stdhosts.b yppush.b ypset.b yppoll.b ypxfr.b

SUBDIR	= revnetgroup

SCRIPT	= ypinit ypmake ypxfr_1perday ypxfr_2perday ypxfr_1perhour

.SUFFIXES:.b
.c:
	@-rm -f $*
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $* $< ${DBM_LIB} ${LIBS} ${LIBC}
.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBDBM}  ${BLIBC} -B"-d/nfs/BFA/$*.B"

all:		${TARGETS}

bfa:		${BFATARG}

ypxfr:	ypxfr.c trans_map.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ypxfr.c trans_map.o ${DBM_LIB} ${LIBS} ${LIBC}

ypxfr.b: ypxfr.c _trans_map.o
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ypxfr.c _trans_map.o ${BLIBDBM}  ${BLIBC} -B"-d/nfs/BFA/$*.B"

_trans_map.o:     ${Nfs}/cmds/usr.etc/ypserv/trans_map.c
	@-rm -f $@ _$*.c /nfs/BFA/trans_map.B
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -c -o $@ ${Nfs}/cmds/usr.etc/ypserv/trans_map.c -B"-d/nfs/BFA/trans_map.B"

trans_map.o: ../ypserv/trans_map.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c ../ypserv/trans_map.c ${DBM_LIB} ${LIBS} ${LIBC}

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
	${INS} makedbm usr/etc/yp root other 0500
	${INS} stdhosts usr/etc/yp root other 0500
	${INS} yppoll usr/etc/yp root other 0555
	${INS} yppush usr/etc/yp root other 0500
	${INS} ypset usr/etc/yp root other 0500
	${INS} ypxfr usr/etc/yp root other 0500
	${INS} ypinit etc/newconfig root other 0500
	${INS} ypmake etc/newconfig root other 0500


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


