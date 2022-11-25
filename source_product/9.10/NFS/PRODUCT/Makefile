#	@(#)$Revision: 1.11.109.1 $	$Date: 91/11/19 13:51:19 $
##
#	Makefile for NFS 800 services and man pages 
#
SHELL 	= /bin/sh

RPCSVC_LIB = ${ST_NFS}/cmds/usr.lib/librpcsvc/800/librpcsvc800.a
DBM_LIB = ${ST_NFS}/cmds/usr.lib/libdbm/800/libdbm800.a

PRODUCT	=  cmds man
NFSINC  =  nfs.h nfs_clnt.h rnode.h
RPCINC  = auth.h auth_unix.h clnt.h pmap_clnt.h pmap_prot.h rpc.h rpc_msg.h \
	  svc.h svc_auth.h types.h xdr.h 
RPCSVC  = mount.h rstat.h rusers.h rwall.h spray.h yp_prot.h ypclnt.h \
	  yppasswd.h ypv1_prot.h
SYSTEM  = customize copyright pif revlist

all prod trace debug clean clobber pfa:
	for dir in ${PRODUCT}; do \
	    test -d $${dir} && ( cd $${dir}; $(MAKE) -k $@ ) \
	done

install: 
	cd ${ST_NFS}/etc/conf/lib ; ${INS} libnfs.a 
	cd ${ST_NFS}/system/50971A ; ${INS} ${SYSTEM} 
	cd ${ST_CMDS}/usr/include/nfs ; ${INS} ${NFSINC} 
	cd ${ST_CMDS}/usr/include/rpc ; ${INS} ${RPCINC} 
	cd ${ST_CMDS}/usr/include/rpcsvc ; ${INS} ${RPCSVC}
	for dir in ${PRODUCT}; do \
		test -d $${dir} && ( cd $${dir} ; $(MAKE) -k $@ ) \
	done 

