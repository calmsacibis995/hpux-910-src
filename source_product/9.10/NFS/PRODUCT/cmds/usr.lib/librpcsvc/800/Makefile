##
#	@(#)$Revision: 1.26.109.1 $	$Date: 91/11/19 14:24:41 $
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

CSOURCE	= ../mountxdr.c ../rstatxdr.c ../rusersxdr.c ../rwallxdr.c \
	../rex_xdr.c ../sm_inter.c ../sprayxdr.c ../util.c ../yppaswdxdr.c \
	../version.c ../rquotaxdr.c
800OBJS = mountxdr.o rstatxdr.o rusersxdr.o rwallxdr.o rex_xdr.o \
	  sm_inter.o sprayxdr.o util.o yppaswdxdr.o version.o rquotaxdr.o
BFAOBJS = mountxdr.b rstatxdr.b rusersxdr.b rwallxdr.b rex_xdr.b \
	  sm_inter.b sprayxdr.b util.b yppaswdxdr.b version.b rquotaxdr.b
800TARG	= librpcsvc800.a
BFATARG = librpcsvc800b.a

all:		${800TARG}

bfa:		${BFATARG}

${800TARG}:	${800OBJS}
	@-cp $@ ,$@ ; mv ,$@ $@
	@echo "building ${800TARG}"
	$(AR) r $@ $?

${BFATARG}:	${BFAOBJS}
	@-cp $@ ,$@ ; mv ,$@ $@
	@echo "building ${BFATARG}"
	$(AR) r $@ $?

rex_xdr.o:	../rex_xdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

sm_inter.o:	../sm_inter.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

mountxdr.o:	../mountxdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

rstatxdr.o:	../rstatxdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

rusersxdr.o: 	../rusersxdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

rwallxdr.o:	../rwallxdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

sprayxdr.o:	../sprayxdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

util.o:	../util.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

yppaswdxdr.o:	../yppaswdxdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

rquotaxdr.o:	../rquotaxdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

version.o:	../version.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

mountxdr.b:	../mountxdr.c
	rm -f /users/chm/BFA/mountxdr.B $@ _mountxdr.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/mountxdr.B"
	mv _mountxdr.o mountxdr.b

rstatxdr.b:	../rstatxdr.c
	rm -f /users/chm/BFA/rstatxdr.B $@ _rstatxdr.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/rstatxdr.B"
	mv _rstatxdr.o rstatxdr.b

rusersxdr.b: 	../rusersxdr.c
	rm -f /users/chm/BFA/rusersxdr.B $@ _rusersxdr.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/rusersxdr.B"
	mv _rusersxdr.o rusersxdr.b

rwallxdr.b:	../rwallxdr.c
	rm -f /users/chm/BFA/rwallxdr.B $@ _rwallxdr.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/rwallxdr.B"
	mv _rwallxdr.o rwallxdr.b

sprayxdr.b:	../sprayxdr.c
	rm -f /users/chm/BFA/sprayxdr.B $@ _sprayxdr.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/sprayxdr.B"
	mv _sprayxdr.o sprayxdr.b

util.b:	../util.c
	rm -f /users/chm/BFA/util.B $@ _util.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/util.B"
	mv _util.o util.b

yppaswdxdr.b:	../yppaswdxdr.c
	rm -f /users/chm/BFA/yppaswdxdr.B $@ _yppaswdxdr.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/yppaswdxdr.B"
	mv _yppaswdxdr.o yppaswdxdr.b

version.b:	../version.c
	rm -f /users/chm/BFA/version.B $@ _version.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/version.B"
	mv _version.o version.b

lint:;		lint -bnx ${CFLAGS} ${LDFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f *.b _*.c

clobber:	clean
	-rm -f ${800TARG} ${BFATARG} rex_xdr.o sm_inter.o

install:	all
	test "${INS}" && ${INS} ${800TARG}

trace:;	-rm -f ${800TARG}; $(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

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

