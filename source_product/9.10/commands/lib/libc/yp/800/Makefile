##
#	@(#)$Revision: 12.0 $	$Date: 89/09/25 16:12:05 $
#       Makefile     1.1     86/02/03
#	(#)Makefile	2.1 86/04/14 NFSSRC
#
SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DLIBTRACE ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DLIBTRACE ${CFLAGS}
DLFLAGS	= -g 
BCFLAGS	= -DBFA ${CFLAGS}

CSOURCE	= ../ypprot_err.c ../yp_bind.c ../yperrstrng.c ../yp_enum.c \
	  ../yp_match.c ../ypxdr.c ../yp_all.c ../yp_master.c ../yp_order.c \
	  ../ypv1_xdr.c
800OBJS	= ypprot_err.o yp_bind.o yperrstrng.o yp_enum.o yp_match.o ypxdr.o \
	  yp_all.o yp_master.o yp_order.o ypv1_xdr.o
BFAOBJS	= ypprot_err.b yp_bind.b yperrstrng.b yp_enum.b yp_match.b ypxdr.b \
	  yp_all.b yp_master.b yp_order.b ypv1_xdr.b
800TARG	= ../../libc800.a
BFATARG = ../../libc800b.a

all:		${800TARG}

bfa:            ${BFATARG}

${800TARG}: 	${800OBJS}
	TMPLIB=../../libc.tmp$$$$; \
    	cp ${800TARG} $$TMPLIB; \
    	mv -f $$TMPLIB ${800TARG}; \
	chmod u+w ${800TARG}
	@echo "adding to ${800TARG}"
	ar r $@ $?
	@-chmod 664 $@

${BFATARG}:	${BFAOBJS}
	TMPLIB=../../libc.tmpb$$$$; \
    	cp ${BFATARG} $$TMPLIB; \
    	mv -f $$TMPLIB ${BFATARG}; \
	chmod u+w ${BFATARG}
	@echo "building ${BFATARG}"
	ar r $@ $?
	@-chmod 664 $@

ypprot_err.o:	../ypprot_err.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

yp_bind.o: 	../yp_bind.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

yperrstrng.o:	../yperrstrng.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

yp_enum.o: 	../yp_enum.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

yp_match.o: 	../yp_match.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

ypxdr.o:	../ypxdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

yp_all.o: 	../yp_all.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

yp_master.o: 	../yp_master.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

yp_order.o: 	../yp_order.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

ypv1_xdr.o:	../ypv1_xdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

ypprot_err.b:	../ypprot_err.c
	rm -f /users/chm/BFA/ypprot_err.B $@ _ypprot_err.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/ypprot_err.B"
	mv _ypprot_err.o ypprot_err.b

yp_bind.b: 	../yp_bind.c
	rm -f /users/chm/BFA/yp_bind.B $@ _yp_bind.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/yp_bind.B"
	mv _yp_bind.o yp_bind.b

yperrstrng.b:	../yperrstrng.c
	rm -f /users/chm/BFA/yperrstrng.B $@ _yperrstrng.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/yperrstrng.B"
	mv _yperrstrng.o yperrstrng.b

yp_enum.b: 	../yp_enum.c
	rm -f /users/chm/BFA/yp_enum.B $@ _yp_enum.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/yp_enum.B"
	mv _yp_enum.o yp_enum.b

yp_match.b: 	../yp_match.c
	rm -f /users/chm/BFA/yp_match.B $@ _yp_match.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/yp_match.B"
	mv _yp_match.o yp_match.b

ypxdr.b:	../ypxdr.c
	rm -f /users/chm/BFA/ypxdr.B $@ _ypxdr.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/ypxdr.B"
	mv _ypxdr.o ypxdr.b

yp_all.b: 	../yp_all.c
	rm -f /users/chm/BFA/yp_all.B $@ _yp_all.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/yp_all.B"
	mv _yp_all.o yp_all.b

yp_master.b: 	../yp_master.c
	rm -f /users/chm/BFA/yp_master.B $@ _yp_master.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/yp_master.B"
	mv _yp_master.o yp_master.b

yp_order.b: 	../yp_order.c
	rm -f /users/chm/BFA/yp_order.B $@ _yp_order.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/yp_order.B"
	mv _yp_order.o yp_order.b

ypv1_xdr.b:	../ypv1_xdr.c
	rm -f /users/chm/BFA/ypv1_xdr.B $@ _ypv1_xdr.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/ypv1_xdr.B"
	mv _ypv1_xdr.o ypv1_xdr.b

lint:;		lint -bnx ${CFLAGS} ${LDFLAGS} ${IDIRS} ${CSOURCE}

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
	rm -f ,eddep ,makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- maketd depends upon it

