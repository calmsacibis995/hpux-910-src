##
#	@(#)$Revision: 12.0 $	$Date: 89/09/25 16:11:17 $
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

CSOURCE	= ypprot_err.c yp_bind.c yperrstrng.c \
	yp_enum.c yp_match.c ypxdr.c \
	yp_all.c yp_master.c yp_order.c ypv1_xdr.c
OBJECTS	= ypprot_err.o yp_bind.o yperrstrng.o \
	yp_enum.o yp_match.o ypxdr.o \
	yp_all.o yp_master.o yp_order.o ypv1_xdr.o
BFA_OBJ	= ypprot_err.b yp_bind.b yperrstrng.b \
	yp_enum.b yp_match.b ypxdr.b \
	yp_all.b yp_master.b yp_order.b ypv1_xdr.b

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

${BFATARG}:	${BFA_OBJ}
	@echo "building ${BFATARG}"
	ar r $@ $?

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

ypprot_err.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
ypprot_err.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
ypprot_err.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
ypprot_err.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
ypprot_err.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
ypprot_err.o: ../../../include/rpcsvc/yp_prot.h
ypprot_err.o: ../../../include/rpcsvc/ypclnt.h ../../../include/sys/types.h
yp_bind.o: ../../../include/errno.h ../../../include/fcntl.h
yp_bind.o: ../../../include/netinet/in.h ../../../include/nl_types.h
yp_bind.o: ../../../include/rpc/auth.h ../../../include/rpc/auth_unix.h
yp_bind.o: ../../../include/rpc/clnt.h ../../../include/rpc/pmap_prot.h
yp_bind.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
yp_bind.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
yp_bind.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
yp_bind.o: ../../../include/rpcsvc/yp_prot.h ../../../include/rpcsvc/ypclnt.h
yp_bind.o: ../../../include/rpcsvc/ypv1_prot.h ../../../include/stdio.h
yp_bind.o: ../../../include/sys/errno.h ../../../include/sys/socket.h
yp_bind.o: ../../../include/sys/types.h ../../../include/time.h
yperrstrng.o: ../../../include/nl_types.h ../../../include/rpcsvc/ypclnt.h
yp_enum.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
yp_enum.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
yp_enum.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
yp_enum.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
yp_enum.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
yp_enum.o: ../../../include/rpcsvc/yp_prot.h ../../../include/rpcsvc/ypclnt.h
yp_enum.o: ../../../include/rpcsvc/ypv1_prot.h ../../../include/sys/types.h
yp_enum.o: ../../../include/time.h
yp_match.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
yp_match.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
yp_match.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
yp_match.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
yp_match.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
yp_match.o: ../../../include/rpcsvc/yp_prot.h ../../../include/rpcsvc/ypclnt.h
yp_match.o: ../../../include/rpcsvc/ypv1_prot.h ../../../include/sys/types.h
yp_match.o: ../../../include/time.h
ypxdr.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
ypxdr.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
ypxdr.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
ypxdr.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
ypxdr.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
ypxdr.o: ../../../include/rpcsvc/yp_prot.h ../../../include/rpcsvc/ypclnt.h
ypxdr.o: ../../../include/sys/types.h
yp_all.o: ../../../include/netinet/in.h ../../../include/nl_types.h
yp_all.o: ../../../include/rpc/auth.h ../../../include/rpc/auth_unix.h
yp_all.o: ../../../include/rpc/clnt.h ../../../include/rpc/rpc.h
yp_all.o: ../../../include/rpc/rpc_msg.h ../../../include/rpc/svc.h
yp_all.o: ../../../include/rpc/svc_auth.h ../../../include/rpc/types.h
yp_all.o: ../../../include/rpc/xdr.h ../../../include/rpcsvc/yp_prot.h
yp_all.o: ../../../include/rpcsvc/ypclnt.h ../../../include/rpcsvc/ypv1_prot.h
yp_all.o: ../../../include/sys/types.h ../../../include/time.h
yp_master.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
yp_master.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
yp_master.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
yp_master.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
yp_master.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
yp_master.o: ../../../include/rpcsvc/yp_prot.h ../../../include/rpcsvc/ypclnt.h
yp_master.o: ../../../include/rpcsvc/ypv1_prot.h ../../../include/sys/types.h
yp_master.o: ../../../include/time.h
yp_order.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
yp_order.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
yp_order.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
yp_order.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
yp_order.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
yp_order.o: ../../../include/rpcsvc/yp_prot.h ../../../include/rpcsvc/ypclnt.h
yp_order.o: ../../../include/rpcsvc/ypv1_prot.h ../../../include/sys/types.h
yp_order.o: ../../../include/time.h
ypv1_xdr.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
ypv1_xdr.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
ypv1_xdr.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
ypv1_xdr.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
ypv1_xdr.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
ypv1_xdr.o: ../../../include/rpcsvc/yp_prot.h ../../../include/rpcsvc/ypclnt.h
ypv1_xdr.o: ../../../include/rpcsvc/ypv1_prot.h ../../../include/sys/types.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
