# HPUX_ID: @(#)Makefile	1.1          88/04/13

##
##  HP-UX Kernel Makefile
##  For making kernel object files
##


CC = /bin/cc
AS = /bin/as
LD = /bin/ld
SHELL = /bin/sh

CFLAGS = +O1 +M -W2,-l,-i

COPTS = -Dhp9000s200 -Dhpux -Uvax -DHFS -DMC68030 -D_HPUX_SOURCE -DPSTAT -D__hp9000s300 -DKERNEL -D_KERNEL -D__hp9000s200 -DREGION -DKVM -DGENESIS -DAUTOCHANGER -DPOST_FREEZE -UPOST80 -DRTPRIO -DPROCESSLOCK -DWRITE_GUARD_7_0 -DSAVECORE_300 -DCONFIG_HP200 -DAPPLETALK -UUMM -DWOPR -DDSKLESS -DLOCAL_DISC -DGETMOUNT -DBSDJOBCTL -DPOSIX -DACLS -DAUDIT -DKERNEL_TUNE_FG -DFSD_KI -DNSYNC -DFS_TUNE1 -DAPEX -DTEST_TABLEWALK -DLOGICAL_IO -DAPOLLO_PCI -DCOPYBACK_CACHE -DMUST_PURGE -DCIMMERON -UMOVE16 -DTRANSPARENT_TRANS -DTEST_LIO -DKMAP -DNEWMAP -DDEVELOP -DFLAKEY_MC68040 -UNOCACHE -DEISA -DMTV -DEISA_8.0 -UCB_KERNEL_DATA -UCB_TTW -UCB_KVALLOC -UCB_FSBCACHE -DCB_USTACK -DCB_KSTACK -DCB_UDATA -DQUOTA -I/etc/conf/h -I/etc/conf/machine -I/usr/include/sys -I/usr/include/.unsupp/sys/

DRIVER_OBJS =

hp-ux:	$(DRIVER_OBJS)
	@date
	rm -rf libdrivers.a hp-ux
	ar rc libdrivers.a $(DRIVER_OBJS)
	sed 's/$$(LIBS)$$/libdrivers.a &/' > drivers.mk < config.mk
	$(MAKE) -f drivers.mk hp-ux

.c.o:
	@echo 'Compiling $< ...'
	$(CC) $(CFLAGS) $(COPTS) -c $<
