# $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/makefile,v $
# $Revision: 66.6 $

# Common makefile for libc/rpc subdirectory

COMOBJS = \
	auth_none.o	\
	auth_unix.o	\
	authux_pro.o	\
	bdresvport.o	\
	clnt_gen.o	\
	clnt_perr.o	\
	clnt_raw.o	\
	clnt_simpl.o	\
	clnt_tcp.o	\
	clnt_udp.o	\
	pmap_clnt.o	\
	pmap_maps.o	\
	pmap_port.o	\
	pmap_prot.o	\
	pmap_rmt.o	\
	rpc_alloc.o	\
	rpc_data.o	\
	rpc_prot.o	\
	svc.o		\
	svc_auth.o	\
	svc_authux.o	\
	svc_raw.o	\
	svc_simple.o	\
	svc_tcp.o	\
	svc_udp.o	\
	xdr.o		\
	xdr_array.o	\
	xdr_float.o	\
	xdr_mem.o	\
	xdr_fh.o	\
	xdr_rec.o	\
	xdr_refer.o	\
	xdr_stdio.o

OBJS300 = 

COMOBJS_P = $(COMOBJS:.o=_p.o)
OBJS300_P = $(OBJS300:.o=_p.o)

COMOBJS_SL = $(COMOBJS:.o=_sl.o)
OBJS300_SL = $(OBJS300:.o=_sl.o)

COMOBJS_PIC = $(COMOBJS:.o=_pic.o)
OBJS300_PIC = $(OBJS300:.o=_pic.o)

default:	archive

archive:
	if hp9000s300; then			\
		objs="$(COMOBJS) $(OBJS300)";	\
	elif hp9000s800; then			\
		objs="$(COMOBJS)";		\
	fi;					\
	$(MAKE) OBJS="$$objs" DIR=tmp rpc

shared:
	if hp9000s300; then			\
		objs="$(COMOBJS_SL) $(OBJS300_SL)";	\
	elif hp9000s800; then			\
		objs="$(COMOBJS_SL)";		\
	fi;					\
	$(MAKE) OBJS="$$objs" DIR=tmp_sl rpc

profile:
	if hp9000s300; then				\
		objs="$(COMOBJS_P) $(OBJS300_P)";	\
	elif hp9000s800; then				\
		objs="$(COMOBJS_P)";			\
	fi;						\
	$(MAKE) OBJS="$$objs" DIR=tmp_p rpc

archive_pic:
	if hp9000s300; then				\
		objs="$(COMOBJS_PIC) $(OBJS300_PIC)";	\
	elif hp9000s800; then				\
		objs="$(COMOBJS_PIC)";			\
	fi;						\
	$(MAKE) OBJS="$$objs" DIR=tmp_pic rpc

rpc: $(OBJS)
	@echo "\tLinking objects to ../$(DIR)..."
	@if [ ! -d ../$(DIR) ]; then		\
	    mkdir ../$(DIR) >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS) ../$(DIR)
	touch ../$(DIR)/OK.$@

clean:
	/bin/rm -f $(COMOBJS) $(OBJS300) $(COMOBJS_P) $(OBJS300_P)
	/bin/rm -f $(COMOBJS_SL) $(OBJS300_SL) $(COMOBJS_PIC) $(OBJS300_PIC)

clobber:
	@echo "Clobbering is done by the top-level libc makefile"

#
# Define rule so make knows how to create different flavors of .o's
#
.c_p.o:
	$(CC) $(CFLAGS) $(PRFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c_pic.o:
	$(CC) $(CFLAGS) -c $< -o $@

# 
# Clear SUFFIXES, then reset ordering so if a .c and .s source exist for a
# file, the .s will be compiled first
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
