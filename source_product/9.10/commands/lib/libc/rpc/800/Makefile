##
#	@(#)$Revision: 12.0 $	$Date: 89/09/25 16:10:48 $
# 	Makefile	1.1	85/05/30
##
# HPNFS:  Changed names of authunix_p to authux_pro,
# pmap_getpt to pmap_port, svc_auth_u to svc_authux,
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
C_SRC	= ../authux_pro.c ../rpc_prot.c ../svc.c ../svc_auth.c ../svc_authux.c \
	  ../xdr.c ../xdr_array.c ../xdr_mem.c
C_OBJ	= authux_pro.o rpc_prot.o svc.o svc_auth.o svc_authux.o \
	  xdr.o xdr_array.o xdr_mem.o
C_BFA	= authux_pro.b rpc_prot.b svc.b svc_auth.b svc_authux.b \
	  xdr.b xdr_array.b xdr_mem.b
K_SRC	= ../auth_kern.c ../clnt_kudp.c ../kudp_fsend.c ../subr_kudp.c \
	  ../svc_kudp.c ../xdr_mbuf.c
K_OBJ	= auth_kern.o clnt_kudp.o kudp_fsend.o subr_kudp.o svc_kudp.o xdr_mbuf.o
K_BFA	= auth_kern.b clnt_kudp.b kudp_fsend.b subr_kudp.b svc_kudp.b xdr_mbuf.b
U_SRC	= ../auth_none.c ../auth_unix.c ../bdresvport.c ../clnt_gen.c \
	  ../clnt_perr.c ../clnt_raw.c ../clnt_simpl.c ../clnt_tcp.c \
	  ../clnt_udp.c ../pmap_clnt.c ../pmap_maps.c ../pmap_port.c \
	  ../pmap_prot.c ../pmap_rmt.c ../svc_raw.c ../svc_simple.c \
	  ../svc_tcp.c ../svc_udp.c ../xdr_float.c ../xdr_rec.c \
	  ../xdr_refer.c ../xdr_stdio.c ../rpc_alloc.c ../xdr_fh.c ../rpc_data.c
U_OBJ	= auth_none.o auth_unix.o bdresvport.o clnt_gen.o \
	  clnt_perr.o clnt_raw.o clnt_simpl.o clnt_tcp.o clnt_udp.o \
	  pmap_clnt.o pmap_maps.o pmap_port.o pmap_prot.o pmap_rmt.o \
	  svc_raw.o svc_simple.o svc_tcp.o svc_udp.o rpc_data.o \
	  xdr_float.o xdr_rec.o xdr_refer.o xdr_stdio.o rpc_alloc.o xdr_fh.o
U_BFA	= auth_none.b auth_unix.b bdresvport.b clnt_gen.b \
	  clnt_perr.b clnt_raw.b clnt_simpl.b clnt_tcp.b clnt_udp.b \
	  pmap_clnt.b pmap_maps.b pmap_port.b pmap_prot.b pmap_rmt.b \
	  svc_raw.b svc_simple.b svc_tcp.b svc_udp.b rpc_data.b\
	  xdr_float.b xdr_rec.b xdr_refer.b xdr_stdio.b rpc_alloc.b xdr_fh.b
##
#	CSOURCE	--	source files needed for user land compilation
#	800OBJS	--	object files needed for user land compilation
#       BFAOBJS --      BFA object files needed for user land compilation
##
CSOURCE	= ${C_SRC} ${U_SRC}
800OBJS	= ${C_OBJ} ${U_OBJ}
BFAOBJS = ${C_BFA} ${U_BFA}
800TARG	= ../../libc800.a
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

bdresvport.o:	../bdresvport.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

clnt_gen.o:	../clnt_gen.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

authux_pro.o: 	../authux_pro.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

rpc_prot.o: 	../rpc_prot.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

svc.o: 	../svc.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

svc_auth.o: 	../svc_auth.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

svc_authux.o:	../svc_authux.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

xdr.o: 	../xdr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

xdr_array.o: 	../xdr_array.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

xdr_mem.o:	../xdr_mem.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

auth_none.o:	../auth_none.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

auth_unix.o:	../auth_unix.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

clnt_perr.o: 	../clnt_perr.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

clnt_raw.o: 	../clnt_raw.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

clnt_simpl.o: 	../clnt_simpl.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

clnt_tcp.o: 	../clnt_tcp.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

clnt_udp.o:	../clnt_udp.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

pmap_clnt.o: 	../pmap_clnt.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

pmap_maps.o: 	../pmap_maps.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

pmap_port.o: 	../pmap_port.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

pmap_prot.o: 	../pmap_prot.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

pmap_rmt.o:	../pmap_rmt.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

svc_raw.o: 	../svc_raw.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

svc_simple.o:	../svc_simple.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

svc_tcp.o: 	../svc_tcp.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

svc_udp.o: 	../svc_udp.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

xdr_float.o: 	../xdr_float.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

xdr_rec.o: 	../xdr_rec.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

xdr_refer.o: 	../xdr_refer.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

xdr_stdio.o: 	../xdr_stdio.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

rpc_alloc.o: 	../rpc_alloc.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

xdr_fh.o:	../xdr_fh.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

rpc_data.o:	../rpc_data.c
	$(CC) ${CFLAGS} ${IDIRS} -c $@ $?

authux_pro.b: 	../authux_pro.c
	rm -f /users/chm/BFA/authux_pro.B $@ _authux_pro.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/authux_pro.B"
	mv _authux_pro.o authux_pro.b

rpc_prot.b: 	../rpc_prot.c
	rm -f /users/chm/BFA/rpc_prot.B $@ _rpc_prot.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/rpc_prot.B"
	mv _rpc_prot.o rpc_prot.b

svc.b: 	../svc.c
	rm -f /users/chm/BFA/svc.B $@ _svc.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/svc.B"
	mv _svc.o svc.b

svc_auth.b: 	../svc_auth.c
	rm -f /users/chm/BFA/svc_auth.B $@ _svc_auth.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/svc_auth.B"
	mv _svc_auth.o svc_auth.b

svc_authux.b:	../svc_authux.c
	rm -f /users/chm/BFA/svc_authux.B $@ _svc_authux.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/svc_authux.B"
	mv _svc_authux.o svc_authux.b

xdr.b: 	../xdr.c
	rm -f /users/chm/BFA/xdr.B $@ _xdr.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/xdr.B"
	mv _xdr.o xdr.b

xdr_array.b: 	../xdr_array.c
	rm -f /users/chm/BFA/xdr_array.B $@ _xdr_array.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/xdr_array.B"
	mv _xdr_array.o xdr_array.b

xdr_mem.b:	../xdr_mem.c
	rm -f /users/chm/BFA/xdr_mem.B $@ _xdr_mem.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/xdr_mem.B"
	mv _xdr_mem.o xdr_mem.b

auth_none.b:	../auth_none.c
	rm -f /users/chm/BFA/auth_none.B $@ _auth_none.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/auth_none.B"
	mv _auth_none.o auth_none.b

auth_unix.b:	../auth_unix.c
	rm -f /users/chm/BFA/auth_unix.B $@ _auth_unix.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/auth_unix.B"
	mv _auth_unix.o auth_unix.b

clnt_perr.b: 	../clnt_perr.c
	rm -f /users/chm/BFA/clnt_perr.B $@ _clnt_perr.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/clnt_perr.B"
	mv _clnt_perr.o clnt_perr.b

clnt_raw.b: 	../clnt_raw.c
	rm -f /users/chm/BFA/clnt_raw.B $@ _clnt_raw.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/clnt_raw.B"
	mv _clnt_raw.o clnt_raw.b

clnt_simpl.b: 	../clnt_simpl.c
	rm -f /users/chm/BFA/clnt_simpl.B $@ _clnt_simpl.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/clnt_simpl.B"
	mv _clnt_simpl.o clnt_simpl.b

clnt_tcp.b: 	../clnt_tcp.c
	rm -f /users/chm/BFA/clnt_tcp.B $@ _clnt_tcp.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/clnt_tcp.B"
	mv _clnt_tcp.o clnt_tcp.b

clnt_udp.b:	../clnt_udp.c
	rm -f /users/chm/BFA/clnt_udp.B $@ _clnt_udp.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/clnt_udp.B"
	mv _clnt_udp.o clnt_udp.b

pmap_clnt.b: 	../pmap_clnt.c
	rm -f /users/chm/BFA/pmap_clnt.B $@ _pmap_clnt.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/pmap_clnt.B"
	mv _pmap_clnt.o pmap_clnt.b

pmap_maps.b: 	../pmap_maps.c
	rm -f /users/chm/BFA/pmap_maps.B $@ _pmap_maps.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/pmap_maps.B"
	mv _pmap_maps.o pmap_maps.b

pmap_port.b: 	../pmap_port.c
	rm -f /users/chm/BFA/pmap_port.B $@ _pmap_port.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/pmap_port.B"
	mv _pmap_port.o pmap_port.b

pmap_prot.b: 	../pmap_prot.c
	rm -f /users/chm/BFA/pmap_prot.B $@ _pmap_prot.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/pmap_prot.B"
	mv _pmap_prot.o pmap_prot.b

pmap_rmt.b:	../pmap_rmt.c
	rm -f /users/chm/BFA/pmap_rmt.B $@ _pmap_rmt.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/pmap_rmt.B"
	mv _pmap_rmt.o pmap_rmt.b

svc_raw.b: 	../svc_raw.c
	rm -f /users/chm/BFA/svc_raw.B $@ _svc_raw.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/svc_raw.B"
	mv _svc_raw.o svc_raw.b

svc_simple.b:	../svc_simple.c
	rm -f /users/chm/BFA/svc_simple.B $@ _svc_simple.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/svc_simple.B"
	mv _svc_simple.o svc_simple.b

svc_tcp.b: 	../svc_tcp.c
	rm -f /users/chm/BFA/svc_tcp.B $@ _svc_tcp.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/svc_tcp.B"
	mv _svc_tcp.o svc_tcp.b

svc_udp.b: 	../svc_udp.c
	rm -f /users/chm/BFA/svc_udp.B $@ _svc_udp.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/svc_udp.B"
	mv _svc_udp.o svc_udp.b

xdr_float.b: 	../xdr_float.c
	rm -f /users/chm/BFA/xdr_float.B $@ _xdr_float.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/xdr_float.B"
	mv _xdr_float.o xdr_float.b

xdr_rec.b: 	../xdr_rec.c
	rm -f /users/chm/BFA/xdr_rec.B $@ _xdr_rec.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/xdr_rec.B"
	mv _xdr_rec.o xdr_rec.b

xdr_refer.b: 	../xdr_refer.c
	rm -f /users/chm/BFA/xdr_refer.B $@ _xdr_refer.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/xdr_refer.B"
	mv _xdr_refer.o xdr_refer.b

xdr_stdio.b: 	../xdr_stdio.c
	rm -f /users/chm/BFA/xdr_stdio.B $@ _xdr_stdio.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/xdr_stdio.B"
	mv _xdr_stdio.o xdr_stdio.b

rpc_alloc.b: 	../rpc_alloc.c
	rm -f /users/chm/BFA/rpc_alloc.B $@ _rpc_alloc.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/rpc_alloc.B"
	mv _rpc_alloc.o rpc_alloc.b

xdr_fh.b:	../xdr_fh.c
	rm -f /users/chm/BFA/xdr_fh.B $@ _xdr_fh.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/xdr_fh.B"
	mv _xdr_fh.o xdr_fh.b

rpc_data.b:	../rpc_data.c
	rm -f /users/chm/BFA/rpc_data.B $@ _rpc_data.c
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/rpc_data.B"
	mv _rpc_data.o rpc_data.b

lint:;		lint -bnx ${CFLAGS} ${LDFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f *.b _*.c

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

