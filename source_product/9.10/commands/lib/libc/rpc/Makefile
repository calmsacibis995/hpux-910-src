##
#	@(#)$Revision: 12.0 $	$Date: 89/09/25 16:08:02 $
#
# 	Makefile	1.1	85/05/30
##
# HPNFS:  Changed names of authunix_p to authux_pro,
# pmap_getpt to pmap_port, svc_auth_u to avc_authux,
# and clnt_peror to clnt_perr
# to match the names chosen by ISO 
# dds: 11/5/86
##
SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DLIBTRACE ${CFLAGS}
TLFLAGS	=
DCFLAGS	= -g -DDEBUG -DLIBTRACE ${CFLAGS}
DLFLAGS	= -g
BCFLAGS	= -DBFA ${CFLAGS}
BLFLAGS	= ${LDFLAGS}

##
#	C_SRC	--	source files common to kernel and user space
#	C_OBJ	--	object files common to kernel and user space
#	C_BFA	--	BFA object files common to kernel and user space
#	K_SRC	--	source files used only in kernel land
#	K_OBJ	--	object files used only in kernel land
#	K_BFA	--	BFA object files used only in kernel land
#	U_SRC	--	source files used only in user land
#	U_OBJ	--	object files used only in user land
#	U_BFA	--	BFA object files used only in user land
##
C_SRC	= authux_pro.c rpc_prot.c svc.c svc_auth.c svc_authux.c \
	xdr.c xdr_array.c xdr_mem.c
C_OBJ	= authux_pro.o rpc_prot.o svc.o svc_auth.o svc_authux.o \
	xdr.o xdr_array.o xdr_mem.o
C_BFA	= authux_pro.b rpc_prot.b svc.b svc_auth.b svc_authux.b \
	xdr.b xdr_array.b xdr_mem.b
K_SRC	= auth_kern.c clnt_kudp.c kudp_fsend.c subr_kudp.c svc_kudp.c xdr_mbuf.c
K_OBJ	= auth_kern.o clnt_kudp.o kudp_fsend.o subr_kudp.o svc_kudp.o xdr_mbuf.o
K_BFA	= auth_kern.b clnt_kudp.b kudp_fsend.b subr_kudp.b svc_kudp.b xdr_mbuf.b
U_SRC	= auth_none.c auth_unix.c bdresvport.c clnt_gen.c \
	clnt_perr.c clnt_raw.c clnt_simpl.c clnt_tcp.c clnt_udp.c \
	pmap_clnt.c pmap_maps.c pmap_port.c pmap_prot.c pmap_rmt.c \
	svc_raw.c svc_simple.c svc_tcp.c svc_udp.c rpc_data.c \
	xdr_float.c xdr_rec.c xdr_refer.c xdr_stdio.c rpc_alloc.c xdr_fh.c 
U_OBJ	= auth_none.o auth_unix.o bdresvport.o clnt_gen.o \
	clnt_perr.o clnt_raw.o clnt_simpl.o clnt_tcp.o clnt_udp.o \
	pmap_clnt.o pmap_maps.o pmap_port.o pmap_prot.o pmap_rmt.o \
	svc_raw.o svc_simple.o svc_tcp.o svc_udp.o rpc_data.o \
	xdr_float.o xdr_rec.o xdr_refer.o xdr_stdio.o rpc_alloc.o xdr_fh.o 
U_BFA	= auth_none.b auth_unix.b bdresvport.b clnt_gen.b \
	clnt_perr.b clnt_raw.b clnt_simpl.b clnt_tcp.b clnt_udp.b \
	pmap_clnt.b pmap_maps.b pmap_port.b pmap_prot.b pmap_rmt.b \
	svc_raw.b svc_simple.b svc_tcp.b svc_udp.b rpc_data.b \
	xdr_float.b xdr_rec.b xdr_refer.b xdr_stdio.b rpc_alloc.b xdr_fh.b 
##
#	CSOURCE	--	source files needed for user land compilation
#	OBJECTS	--	object files needed for user land compilation
#	BFA_OBJ	--	BFA object files needed for user land compilation
##
CSOURCE	= ${C_SRC} ${U_SRC}
OBJECTS	= ${C_OBJ} ${U_OBJ}
BFA_OBJ	= ${C_BFA} ${U_BFA}

TARGETS	= ../libc.a
BFATARG	= ../libcb.a

.SUFFIXES:.b
.c.o:
	@-rm -f $@
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

cleanbfa:;	-rm -f *.b _*.c

clobber:	clean

# We don't provide libc so we don't want to install it
linstall:	all

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

authux_pro.o: ../../../include/rpc/auth.h ../../../include/rpc/auth_unix.h
authux_pro.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
authux_pro.o: ../../../include/sys/types.h
rpc_prot.o: ../../../include/machine/param.h ../../../include/netinet/in.h
rpc_prot.o: ../../../include/rpc/auth.h ../../../include/rpc/clnt.h
rpc_prot.o: ../../../include/rpc/rpc_msg.h ../../../include/rpc/types.h
rpc_prot.o: ../../../include/rpc/xdr.h ../../../include/sys/param.h
rpc_prot.o: ../../../include/sys/types.h
svc.o: ../../../include/netinet/in.h ../../../include/nl_types.h
svc.o: ../../../include/rpc/auth.h ../../../include/rpc/clnt.h
svc.o: ../../../include/rpc/pmap_clnt.h ../../../include/rpc/rpc_msg.h
svc.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
svc.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
svc.o: ../../../include/sys/errno.h ../../../include/sys/types.h
svc.o: ../../../include/time.h
svc_auth.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
svc_auth.o: ../../../include/rpc/clnt.h ../../../include/rpc/rpc_msg.h
svc_auth.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
svc_auth.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
svc_auth.o: ../../../include/sys/types.h
svc_authux.o: ../../../include/netinet/in.h ../../../include/nl_types.h
svc_authux.o: ../../../include/rpc/auth.h ../../../include/rpc/auth_unix.h
svc_authux.o: ../../../include/rpc/clnt.h ../../../include/rpc/rpc_msg.h
svc_authux.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
svc_authux.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
svc_authux.o: ../../../include/stdio.h ../../../include/sys/types.h
svc_authux.o: ../../../include/time.h
xdr.o: ../../../include/nl_types.h ../../../include/rpc/types.h
xdr.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
xdr.o: ../../../include/sys/types.h
xdr_array.o: ../../../include/nl_types.h ../../../include/rpc/types.h
xdr_array.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
xdr_array.o: ../../../include/sys/types.h
xdr_mem.o: ../../../include/netinet/in.h ../../../include/rpc/types.h
xdr_mem.o: ../../../include/rpc/xdr.h ../../../include/sys/types.h
auth_none.o: ../../../include/rpc/auth.h ../../../include/rpc/types.h
auth_none.o: ../../../include/rpc/xdr.h ../../../include/sys/types.h
auth_unix.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
auth_unix.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/types.h
auth_unix.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
auth_unix.o: ../../../include/sys/types.h ../../../include/time.h
clnt_perr.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
clnt_perr.o: ../../../include/rpc/clnt.h ../../../include/rpc/rpc_msg.h
clnt_perr.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
clnt_perr.o: ../../../include/stdio.h ../../../include/sys/types.h
clnt_raw.o: ../../../include/netinet/in.h ../../../include/nl_types.h
clnt_raw.o: ../../../include/rpc/auth.h ../../../include/rpc/clnt.h
clnt_raw.o: ../../../include/rpc/rpc_msg.h ../../../include/rpc/types.h
clnt_raw.o: ../../../include/rpc/xdr.h ../../../include/sys/types.h
clnt_raw.o: ../../../include/time.h
clnt_simpl.o: ../../../include/arpa/trace.h ../../../include/netdb.h
clnt_simpl.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
clnt_simpl.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
clnt_simpl.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
clnt_simpl.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
clnt_simpl.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
clnt_simpl.o: ../../../include/stdio.h ../../../include/sys/socket.h
clnt_simpl.o: ../../../include/sys/types.h ../../../include/time.h
clnt_tcp.o: ../../../include/arpa/trace.h ../../../include/errno.h
clnt_tcp.o: ../../../include/netdb.h ../../../include/netinet/in.h
clnt_tcp.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
clnt_tcp.o: ../../../include/rpc/clnt.h ../../../include/rpc/pmap_clnt.h
clnt_tcp.o: ../../../include/rpc/rpc_msg.h ../../../include/rpc/types.h
clnt_tcp.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
clnt_tcp.o: ../../../include/sys/errno.h ../../../include/sys/socket.h
clnt_tcp.o: ../../../include/sys/types.h ../../../include/time.h
clnt_udp.o: ../../../include/arpa/trace.h ../../../include/errno.h
clnt_udp.o: ../../../include/netdb.h ../../../include/netinet/in.h
clnt_udp.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
clnt_udp.o: ../../../include/rpc/clnt.h ../../../include/rpc/pmap_clnt.h
clnt_udp.o: ../../../include/rpc/rpc_msg.h ../../../include/rpc/types.h
clnt_udp.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
clnt_udp.o: ../../../include/sys/errno.h ../../../include/sys/ioctl.h
clnt_udp.o: ../../../include/sys/socket.h ../../../include/sys/types.h
clnt_udp.o: ../../../include/time.h
pmap_clnt.o: ../../../include/net/if.h ../../../include/netinet/in.h
pmap_clnt.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
pmap_clnt.o: ../../../include/rpc/clnt.h ../../../include/rpc/pmap_clnt.h
pmap_clnt.o: ../../../include/rpc/pmap_prot.h ../../../include/rpc/rpc_msg.h
pmap_clnt.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
pmap_clnt.o: ../../../include/stdio.h ../../../include/sys/ioctl.h
pmap_clnt.o: ../../../include/sys/socket.h ../../../include/sys/types.h
pmap_clnt.o: ../../../include/time.h
pmap_maps.o: ../../../include/errno.h ../../../include/net/if.h
pmap_maps.o: ../../../include/netdb.h ../../../include/netinet/in.h
pmap_maps.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
pmap_maps.o: ../../../include/rpc/clnt.h ../../../include/rpc/pmap_clnt.h
pmap_maps.o: ../../../include/rpc/pmap_prot.h ../../../include/rpc/rpc_msg.h
pmap_maps.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
pmap_maps.o: ../../../include/stdio.h ../../../include/sys/errno.h
pmap_maps.o: ../../../include/sys/ioctl.h ../../../include/sys/socket.h
pmap_maps.o: ../../../include/sys/types.h ../../../include/time.h
pmap_port.o: ../../../include/arpa/trace.h ../../../include/net/if.h
pmap_port.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
pmap_port.o: ../../../include/rpc/clnt.h ../../../include/rpc/pmap_clnt.h
pmap_port.o: ../../../include/rpc/pmap_prot.h ../../../include/rpc/rpc_msg.h
pmap_port.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
pmap_port.o: ../../../include/stdio.h ../../../include/sys/ioctl.h
pmap_port.o: ../../../include/sys/socket.h ../../../include/sys/types.h
pmap_port.o: ../../../include/time.h
pmap_prot.o: ../../../include/rpc/pmap_prot.h ../../../include/rpc/types.h
pmap_prot.o: ../../../include/rpc/xdr.h ../../../include/sys/types.h
pmap_rmt.o: ../../../include/errno.h ../../../include/net/if.h
pmap_rmt.o: ../../../include/netinet/in.h ../../../include/nl_types.h
pmap_rmt.o: ../../../include/rpc/auth.h ../../../include/rpc/clnt.h
pmap_rmt.o: ../../../include/rpc/pmap_clnt.h ../../../include/rpc/pmap_prot.h
pmap_rmt.o: ../../../include/rpc/rpc_msg.h ../../../include/rpc/types.h
pmap_rmt.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
pmap_rmt.o: ../../../include/sys/errno.h ../../../include/sys/ioctl.h
pmap_rmt.o: ../../../include/sys/socket.h ../../../include/sys/types.h
pmap_rmt.o: ../../../include/time.h
svc_raw.o: ../../../include/netinet/in.h ../../../include/rpc/auth.h
svc_raw.o: ../../../include/rpc/clnt.h ../../../include/rpc/rpc_msg.h
svc_raw.o: ../../../include/rpc/svc.h ../../../include/rpc/types.h
svc_raw.o: ../../../include/rpc/xdr.h ../../../include/sys/types.h
svc_simple.o: ../../../include/netdb.h ../../../include/netinet/in.h
svc_simple.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
svc_simple.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
svc_simple.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
svc_simple.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
svc_simple.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
svc_simple.o: ../../../include/stdio.h ../../../include/sys/socket.h
svc_simple.o: ../../../include/sys/types.h ../../../include/time.h
svc_tcp.o: ../../../include/errno.h ../../../include/netinet/in.h
svc_tcp.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
svc_tcp.o: ../../../include/rpc/clnt.h ../../../include/rpc/rpc_msg.h
svc_tcp.o: ../../../include/rpc/svc.h ../../../include/rpc/types.h
svc_tcp.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
svc_tcp.o: ../../../include/sys/errno.h ../../../include/sys/socket.h
svc_tcp.o: ../../../include/sys/types.h ../../../include/time.h
svc_udp.o: ../../../include/errno.h ../../../include/netinet/in.h
svc_udp.o: ../../../include/nl_types.h ../../../include/rpc/auth.h
svc_udp.o: ../../../include/rpc/clnt.h ../../../include/rpc/rpc_msg.h
svc_udp.o: ../../../include/rpc/svc.h ../../../include/rpc/types.h
svc_udp.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
svc_udp.o: ../../../include/sys/errno.h ../../../include/sys/socket.h
svc_udp.o: ../../../include/sys/types.h
xdr_float.o: ../../../include/arpa/trace.h ../../../include/rpc/types.h
xdr_float.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
xdr_float.o: ../../../include/sys/types.h
xdr_rec.o: ../../../include/netinet/in.h ../../../include/nl_types.h
xdr_rec.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
xdr_rec.o: ../../../include/stdio.h ../../../include/sys/types.h
xdr_rec.o: ../../../include/time.h
xdr_refer.o: ../../../include/nl_types.h ../../../include/rpc/types.h
xdr_refer.o: ../../../include/rpc/xdr.h ../../../include/stdio.h
xdr_refer.o: ../../../include/sys/types.h
xdr_stdio.o: ../../../include/arpa/trace.h ../../../include/netinet/in.h
xdr_stdio.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
xdr_stdio.o: ../../../include/stdio.h ../../../include/sys/types.h
xdr_fh.o: ../../../include/errno.h ../../../include/netinet/in.h
xdr_fh.o: ../../../include/nfs/nfs.h ../../../include/rpc/auth.h
xdr_fh.o: ../../../include/rpc/auth_unix.h ../../../include/rpc/clnt.h
xdr_fh.o: ../../../include/rpc/rpc.h ../../../include/rpc/rpc_msg.h
xdr_fh.o: ../../../include/rpc/svc.h ../../../include/rpc/svc_auth.h
xdr_fh.o: ../../../include/rpc/types.h ../../../include/rpc/xdr.h
xdr_fh.o: ../../../include/rpcsvc/mount.h ../../../include/sys/errno.h
xdr_fh.o: ../../../include/sys/types.h ../../../include/time.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
