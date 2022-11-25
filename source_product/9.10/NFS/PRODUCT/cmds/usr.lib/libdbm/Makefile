##
#	@(#)$Revision: 1.34.109.1 $	$Date: 91/11/19 14:24:20 $
# (#)Makefile 1.1 85/05/31 SMI; from UCB 4.1 83/06/27
#
SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DLIBTRACE ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DLIBTRACE ${CFLAGS}
DLFLAGS	= -g 
BCFLAGS	= -DBFA ${CFLAGS}
BLFLAGS	= ${LDFLAGS}

CSOURCE	= dbm.c version.c
OBJECTS = dbm.o version.o
BFA_OBJ	= dbm.b version.b

TARGETS	= libdbm.a
BFATARG	= libdbmb.a

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
	@-rm -f $@
	@echo "building ${TARGETS}"
	ar r $@ $?
	@-chmod 664 $@

${BFATARG}:	${BFA_OBJ}
	@-rm -f $@
	@echo "building ${BFATARG}"
	ar r $@ $?
	@-chmod 664 $@

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f *.b _*.c

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	-rm -f ${TARGETS}; $(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	-rm -f ${TARGETS}; $(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

# We don't ship this library so don't install it
linstall:	all

depend:;
	maketd ${CCOPTS} ${CSOURCE} > ,makedep
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


