#	@(#)9.1	88/01/14
##
#	Makefile for installing NFS include files
#

SHELL	= /bin/sh

NFS	=   nfs.h nfs_clnt.h rnode.h
RPC	=   auth.h auth_unix.h clnt.h pmap_clnt.h pmap_prot.h rpc.h rpc_msg.h \
	    svc.h svc_auth.h types.h xdr.h
RPCSVC	=   mount.h rstat.h rusers.h rwall.h spray.h yp_prot.h ypclnt.h \
	    yppasswd.h ypv1_prot.h

all prod trace debug pfa clean clobber:

install:
	test ${INS} && { \
		( cd nfs ; ${INS} ${NFS} ) ; \
		( cd rpc ; ${INS} ${RPC} ) ; \
		( cd rpcsvc ; ${INS} ${RPCSVC} ) ; \
	}
