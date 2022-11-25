##
#	@(#)$Revision: 12.0 $	$Date: 89/09/25 16:06:34 $
# (#)Makefile 1.1 86/02/03 SMI; from UCB 4.4 6/10/83
# (#)Makefile	2.1 86/04/11 NFSSRC
#
SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DLIBTRACE ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DLIBTRACE ${CFLAGS}
DLFLAGS	= -g
BCFLAGS	= -DBFA ${CFLAGS}

CSOURCE	= ../gtnetgrent.c ../getrpcent.c ../innetgr.c 
800OBJS = gtnetgrent.o getrpcent.o innetgr.o 
BFAOBJS = gtnetgrent.b getrpcent.b innetgr.b 
800TARG = ../../libc800.a
BFATARG = ../../libc800b.a

all:		${800TARG}

bfa:		${BFATARG}

${800TARG}: 	${800OBJS}
	TMPLIB=../../libc.tmp$$$$; \
    	cp ${800TARG} $$TMPLIB; \
    	mv -f $$TMPLIB ${800TARG}; \
	chmod u+w ${800TARG}
	@echo "adding to ${800TARG}"
	$(AR) r $@ $?
	@-chmod 664 $@

${BFATARG}:	${BFAOBJS}
	TMPLIB=../../libc.tmpb$$$$; \
    	cp ${BFATARG} $$TMPLIB; \
    	mv -f $$TMPLIB ${BFATARG}; \
	chmod u+w ${BFATARG}
	@echo "building ${BFATARG}"
	$(AR) r $@ $?
	@-chmod 664 $@

gtnetgrent.o:	../gtnetgrent.c 
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

getrpcent.o: 	../getrpcent.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

innetgr.o: 	../innetgr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

gtnetgrent.b:	../gtnetgrent.c 
	rm -f /users/chm/BFA/gtnetgrent.B $@ _gtnetgrent.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c  $? -B"-d/users/chm/BFA/gtnetgrent.B"
	mv _gtnetgrent.o gtnetgrent.b

getrpcent.b: 	../getrpcent.c
	rm -f /users/chm/BFA/getrpcent.B $@ _getrpcent.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c  $? -B"-d/users/chm/BFA/getrpcent.B"
	mv _getrpcent.o getrpcent.b

innetgr.b: 	../innetgr.c
	rm -f /users/chm/BFA/innetgr.B $@ _innetgr.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c  $? -B"-d/users/chm/BFA/innetgr.B"
	mv _innetgr.o innetgr.b

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm *.b _*.c 

clobber:	clean

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
	@rm -f ,eddep ,makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- maketd depends upon it

