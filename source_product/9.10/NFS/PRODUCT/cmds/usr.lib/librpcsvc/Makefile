##
#	@(#)$Revision: 1.44.109.1 $	$Date: 91/11/19 14:24:47 $
# (#)Makefile 1.1 86/02/05 SMI;
# (#)Makefile	2.1 86/04/14 NFSSRC
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

CSOURCE	= mountxdr.c rstatxdr.c rusersxdr.c rwallxdr.c rquotaxdr.c \
	rex_xdr.c sm_inter.c sprayxdr.c util.c yppaswdxdr.c version.c
OBJECTS = mountxdr.o rstatxdr.o rusersxdr.o rwallxdr.o rquotaxdr.o \
	rex_xdr.o sm_inter.o sprayxdr.o util.o yppaswdxdr.o version.o
BFA_OBJ	= mountxdr.b rstatxdr.b rusersxdr.b rwallxdr.b rquotaxdr.b\
	rex_xdr.b sm_inter.b sprayxdr.b util.b yppaswdxdr.b version.b

TARGETS	= librpcsvc.a
BFATARG	= librpcsvcb.a

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
	@-cp $@ ,$@ ; mv ,$@ $@
	@echo "building ${TARGETS}"
	ar r $@ $?
	@-chmod 664 $@

${BFATARG}:	${BFA_OBJ}
	@-cp $@ ,$@ ; mv ,$@ $@
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

linstall:	all
		${INS} librpcsvc.a usr/lib root other 0444

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


