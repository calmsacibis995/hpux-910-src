#	@(#)$Revision: 1.11.109.1 $	$Date: 91/11/19 14:43:45 $
##
#	Makefile for installing NFS man pages
#

MAN1	=   domainname.1 rup.1 rusers.1 ypcat.1 ypmatch.1 yppasswd.1 ypwhich.1
MAN1M	=   fsirand.1m makedbm.1m mountd.1m nfsd.1m nfsstat.1m \
            portmap.1m rpcinfo.1m rstatd.1m rusersd.1m rwall.1m rwalld.1m \
	    showmount.1m spray.1m sprayd.1m ypinit.1m ypmake.1m  \
	    yppasswdd.1m yppoll.1m yppush.1m ypserv.1m ypset.1m ypxfr.1m \
	    pcnfsd.1m vhe_altlg.1m vhe_mnter.1m vhe_u_mnt.1m
MAN2	=   getdmname.2 getfh.2 nfssvc.2
MAN3	=   getrpcent.3c gnetgrent.3c grpcport.3n mount.3n rnusers.3n rpc.3c \
	    rstat.3n rwall.3n spray.3n xdr.3c ypclnt.3c yppasswd.3n
MAN4	=   exports.4 netgroup.4 rmtab.4 rpc.4 ypfiles.4 vhe_files.4
MAN7	=   nfs.7

all prod trace debug pfa clean clobber:

install:
	test ${INS} && { \
		( cd man1 ; ${INS} ${MAN1} ) ; \
		( cd man1m ; ${INS} ${MAN1M} ) ; \
		( cd man2 ; ${INS} ${MAN2} ) ; \
		( cd man3 ; ${INS} ${MAN3} ) ; \
		( cd man4 ; ${INS} ${MAN4} ) ; \
		( cd man7 ; ${INS} ${MAN7} ) ; \
	}
