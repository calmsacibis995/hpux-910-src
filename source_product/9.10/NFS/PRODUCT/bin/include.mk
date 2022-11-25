# @(#)$Revision: 1.44.109.1 $	$Date: 91/11/19 13:55:47 $
# FILE: include.mk  --  Makefile to generate user include files
#
###############################################################################
###############################################################################
########### Remove kern_prof.h for release ####################################
###############################################################################
#

SCCS = ${Nfs}/rcs/include
#800INC = ${Nfs}/include/800

# GET is for files in the commands shared system (RCS files with certain 
# tags defined), CNDGET is for CND files that are RCS files but do not have 
# the RCS tags defined and SCCSGET is for kernel files that are still using 
# SCCS.

GET = ${Nfs}/bin/Get -rS300/6_5D -t
CNDGET = ${Nfs}/bin/Get -r37 -t
SCCSGET = ${Nfs}/bin/Get -t

CP = /bin/cp
SHELL = /bin/sh

DEFLIST = -DLAN -DLOCKF -DHFS -DMOREFILES -DPRIVGRP -DRTPRIO -DPROCESSLOCK -DSHMEM -DSEMA -DMESG -UASYNCIO -Dhpux -Dhp9000s200 -USDF -Uhp9000s500 -UBELL5_2 -DDUX -DNFS -DHP_NFS -DHPNFS -DMBUF_XTRA_STATS -DCONFIG_HP200 -DLANRFA -DNS -DBSDIPC -DNS_QA -DNS_NAMING_GLOBALS -DNSDIAG -DNSCONFIG -DPXP -DMBUFFLAGS -DNSIPC -DARCH42 -DNENHANCE -DINET -DSOCKET -DCOMPAT -USYSCALLTRACE -DKERNEL_TUNE_DLB -DMBTHRESHOLD -UTRACE -DACLS -DAUDIT -DBSDJOBCTL -DDISKLESS -DNFS3_2 -DPOSIX -DSYMLINKS -UBSD_ONLY -UDISPATCHLCK -UHPUXTTY -UINTRLVE -UKERNEL -ULOCORE -UOLDHPUXTTY -UPERFORM_MEM -UPLOCKSIGNAL -UQUOTA -UTEMPORARY_OBJECT_COMPAT -Uhp9000s800 -Uspectrum -Uvax

#800DEF	= -DHFS -Dhp9000s800 -Dunix -Dhpux -DNLS16 -DNLS -Uhp9000s200 -Uhp9000s500 -USDF -Uvax -Updp11 -Uu3b -Uu3b5 -UBELL5_2 -DNFS -DHP_NFS -DHPNFS


STRIP = ${Nfs}/bin/unifdef $(DEFLIST) -Uvax -Uspectrum -UKERNEL
#800STRIP = ${Nfs}/bin/unifdef $(800DEF) -Dspectrum -UKERNEL
FILES = checklist.h ctype.h bsdtty.h errno.h fcntl.h memory.h mntent.h \
	mnttab.h net/if.h netdb.h netinet/in.h nlist.h nl_ctype.h \
	nl_types.h pwd.h setjmp.h signal.h stdio.h string.h time.h \
	unistd.h utmp.h sys/dir.h sys/dk.h sys/errno.h sys/file.h \
	sys/fs.h sys/ino.h sys/inode.h sys/ioctl.h sys/ipc.h sys/param.h \
	sys/sem.h sys/signal.h sys/socket.h sys/stat.h sys/sysmacros.h \
	sys/termio.h sys/time.h sys/types.h sys/vmmeter.h sys/vnode.h \
	machine/param.h sys/sitemap.h ndir.h sys/user.h sys/acl.h \
	sys/getaccess.h sys/wait.h sys/bsdtty.h sys/vfs.h

# We don't want to get header files for the s800 since they might not be
# the ones being used for the current release.
#	800/checklist.h 800/ctype.h \
#	800/errno.h 800/memory.h 800/mntent.h 800/mnttab.h 800/netdb.h \
#	800/pwd.h 800/setjmp.h 800/signal.h 800/stdio.h 800/string.h \
#	800/time.h 800/utmp.h 800/nlist.h 800/sys/socket.h 800/net/if.h \
#	800/netinet/in.h 800/ndir.h 

all:	$(FILES)

checklist.h: $(SCCS)/checklist.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/checklist.h: $(SCCS)/checklist.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

ctype.h: $(SCCS)/ctype.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/ctype.h: $(SCCS)/ctype.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

fcntl.h: $(SCCS)/fcntl.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

memory.h: $(SCCS)/memory.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/memory.h: $(SCCS)/memory.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

mnttab.h: $(SCCS)/mnttab.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/mnttab.h: $(SCCS)/mnttab.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

mntent.h: $(SCCS)/mntent.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/mntent.h: $(SCCS)/mntent.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

ndir.h: $(SCCS)/ndir.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/ndir.h: $(SCCS)/ndir.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

netdb.h: $(SCCS)/netdb.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/netdb.h: $(SCCS)/netdb.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

netinet/in.h: $(SCCS)/netinet/in.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/netinet/in.h: $(SCCS)/netinet/in.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

nl_ctype.h: $(SCCS)/nl_ctype.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

nl_types.h: $(SCCS)/nl_types.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

nlist.h: $(SCCS)/nlist.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/nlist.h: $(SCCS)/800/nlist.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

pwd.h: $(SCCS)/pwd.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/pwd.h: $(SCCS)/pwd.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

setjmp.h: $(SCCS)/setjmp.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/setjmp.h: $(SCCS)/setjmp.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

stdio.h: $(SCCS)/stdio.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/stdio.h: $(SCCS)/stdio.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

string.h: $(SCCS)/string.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/string.h: $(SCCS)/string.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

sys/dir.h: $(SCCS)/sys/dir.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/dk.h: $(SCCS)/sys/dk.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/file.h: $(SCCS)/sys/file.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/fs.h: $(SCCS)/sys/fs.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/ino.h: $(SCCS)/sys/ino.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/inode.h: $(SCCS)/sys/inode.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/ioctl.h: $(SCCS)/sys/ioctl.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/ipc.h: $(SCCS)/sys/ipc.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/sem.h: $(SCCS)/sys/sem.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/socket.h: $(SCCS)/sys/socket.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/sys/socket.h: $(SCCS)/sys/socket.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

sys/stat.h: $(SCCS)/sys/stat.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/sysmacros.h: $(SCCS)/sys/sysmacros.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/types.h: $(SCCS)/sys/types.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/vmmeter.h: $(SCCS)/sys/vmmeter.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

time.h: $(SCCS)/time.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/time.h: $(SCCS)/sys/time.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/time.h: $(SCCS)/time.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

unistd.h: $(SCCS)/unistd.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

utmp.h: $(SCCS)/utmp.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/utmp.h: $(SCCS)/utmp.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

errno.h: $(SCCS)/errno.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

bsdtty.h: $(SCCS)/bsdtty.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/errno.h: $(SCCS)/errno.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

signal.h: $(SCCS)/signal.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/signal.h: $(SCCS)/signal.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

sys/errno.h: $(SCCS)/sys/errno.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/param.h: $(SCCS)/sys/param.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/signal.h: $(SCCS)/sys/signal.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/termio.h: $(SCCS)/sys/termio.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

termio.h: $(SCCS)/termio.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/vnode.h: $(SCCS)/sys/vnode.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

net/if.h: $(SCCS)/net/if.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

#800/net/if.h: $(SCCS)/net/if.h,v
#	cd $(@D); $(GET) $?
#	-$(800STRIP) $@ >tmp
#	mv -f tmp $@; chmod 444 $@
#	chgrp smurfs $@;chown nfsmgr $@

machine/param.h: $(SCCS)/machine/param.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/sitemap.h: $(SCCS)/sys/sitemap.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/user.h: $(SCCS)/sys/user.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/acl.h: $(SCCS)/sys/acl.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/getaccess.h: $(SCCS)/sys/getaccess.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/wait.h:	$(SCCS)/sys/wait.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/bsdtty.h:	$(SCCS)/sys/bsdtty.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@

sys/vfs.h:	$(SCCS)/sys/vfs.h,v
	cd $(@D); $(GET) $?
	-$(STRIP) $@ >tmp
	mv -f tmp $@; chmod 444 $@
	chgrp smurfs $@;chown nfsmgr $@
