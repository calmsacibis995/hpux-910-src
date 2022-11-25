##
#	@(#)$Revision: 12.1 $	$Date: 92/01/22 17:51:51 $
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

CSOURCE	= gtnetgrent.c getrpcent.c innetgr.c realpath.c exportent.c 
OBJECTS	= gtnetgrent.o getrpcent.o innetgr.o realpath.o exportent.o
BFA_OBJ	= gtnetgrent.b getrpcent.b innetgr.b realpath.b exportent.b

TARGETS	= ../libc.a
BFATARG	= ../libcb.a

.SUFFIXES:.b
.c.o:
	@-rm -f $*
	$(CC) ${CFLAGS} ${IDIRS} -c $<
.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} ${IDIRS} -c -o $*.b $? -B"-d/nfs/BFA/$*.B"

all:		${TARGETS}

bfa:		${BFATARG}

${TARGETS}:	${OBJECTS}
	TMPLIB=../libc.tmp$$$$; \
    	cp ${TARGETS} $$TMPLIB; \
    	mv -f $$TMPLIB ${TARGETS}; \
	chmod u+w ${TARGETS}
	@echo "adding to ${TARGETS}"
	ar r $@ $?
	@-chmod 664 $@

${BFATARG}:	${BFA_OBJ}
	@echo "building ${BFATARG}"
	ar r $@ $?
	@-chmod 664 $@

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm *.b _*.c 

clobber:	clean

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

# We don't provide libc so we don't want to install it
linstall:	all

depend:;
	maketd ${CCOPTS} ${CSOURCE} > ,makedep
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

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
