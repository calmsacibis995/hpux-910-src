# $Source: /source/hpux_source/kernel/sys.SWT68K_300/REG.300/RCS/makefile,v $
# $Revision: 1.11.84.55 $
# $State: Exp $   	$Locker:  $
# $Date: 94/12/21 14:32:04 $

#
# Available targets:
#	"make hp-ux" (default) - build an hp-ux from a bunch of .o's
#	"make fromlibs" - build hp-ux from libraries
#

#
# Definitions for kernel identification
#
UTSNAME_SYS=HP-UX
MODEL=S300
REVISION=B.09.10
DATE=`date`
REV_STRING="$(UTSNAME_SYS) (Source Special) $(MODEL) $(REVISION) $(DATE)"

#
# Configuration definitions
#
DEBUG= -DOSDEBUG -DCALL_KDB_ON_BREAK

NEW_IN_91= -DFILE_TRACE -DPFDAT32 -DPF_ASYNC -DUFS_GETSB -DUMEM_DRIVER \
	-UREQ_MERGING -DCDCASE -DMALLOC_VFDS -UQUIETBOOT -UXTERMINAL
NEW_IN_903=-DIOSCAN -DSDS -DSDS_BOOT -DSTRACE -DSAR \
	-DBNR_HPSOS -DPING_DEBUG
DUXD=-DLOCAL_DISC -DGETMOUNT 
MISC=-DBSDJOBCTL -DPOSIX -DACLS -DAUDIT -DQUOTA -UDDB -DAUTOCHANGER -DGENESIS
TUNE = -DFSD_KI -DNSYNC

LINKS=../.link.300
#XOBJS=`if [ -d /DDB ]; then echo /DDB/*.o; fi`
XOBJS=

LAN=-DCONFIG_HP200 -DAPPLETALK

IDENT=-D_KERNEL -Dhpux -D_HPUX_SOURCE -D_UNSUPPORTED \
	-D_KERNEL_BUILD -DPSTAT -D__hp9000s300

PARAM = $(TUNE) $(MISC) $(LAN)
        

# Uncomment the following line to enable S300 Kernel Instruction
# Coverage Analyzer (aka. ICA) to be built into the kernel.
#ICASYM=-DICA_300

# Can include: (Format of following lines are crucial to sed script) */
DEBUGONLY = -USYSCALLTRACE -DTRACE -DCLEANUP_DEBUG -DKERNEL_DEBUG_ONLY -UNS_QA 
#					       -DGPROF -DSYSCALLTRACE */
#					       -DRTPRIO_DEBUG -DTRACE */

# Define GPROF to compile in kernel gprof code
#GPROF=-DGPROF
GPROF=

COPTS = $(IDENT) $(PARAM) $(DUXD) $(NEW_IN_903) $(NEW_IN_91)
DEFS= $(COPTS)

#
# Added OFLAG so that -O can easily be changed to -g for debugging on the
# command line by vmake OFLAG=-g file_to_be_made_with_debug_info ld
# Set OFLAG to -G to build a kernel with profiling.
#
# disable link/unlink removal with -l (makes the debugger hard to use)
# disable instruction scheduling with -i (rearranges reads from I/O cards)
OFLAG=+O1 -W2,-l,-i
#OFLAG=-G

DGFLAGS = -c -g -Wc,-Nd7000,-Ns7000,-Np3000 -Wp,-H200000 $(COPTS)
CFLAGS = $(OFLAG) +M -Wc,-Nd3500,-Ns7000,-Np3000 -Wp,-H200000 -I. $(COPTS)

LINT_CFLAGS = -Na30000 -Nd30000 -Ns30000 -Wp,-H200000 $(COPTS)

SHELL = /bin/ksh
# Point this to an alternate collection of compilers if desired
SOURCE=
COMPILE_BIN=$(SOURCE)/bin
COMPILE_LIB=$(SOURCE)/lib
CC    = $(COMPILE_BIN)/cc -t pc2,$(COMPILE_LIB)/ \
			  -t a,$(COMPILE_BIN)/as20 \
			  -Wl,-a,archive -L $(COMPILE_LIB)
AS    = $(COMPILE_BIN)/as20
LD    = $(COMPILE_BIN)/ld
AR    = $(COMPILE_BIN)/ar
CP    = $(COMPILE_BIN)/cp
RM    = $(COMPILE_BIN)/rm
CPP   = $(COMPILE_LIB)/cpp
SIZE  = $(COMPILE_BIN)/size
LIBC  = $(COMPILE_LIB)/libc.a
BFA   = $(SOURCE)/bin/bfa/bfa
LINT  = $(COMPILE_BIN)/lint

# All the objects for HP-UX. */
#  KERNOBJS are the ones which MUST come first, and are always linked in. */
KERNOBJS= \
	exceptions.o locore.o name.o funcentry.o cdfs_hooks.o vers.o
KERNSRCS= \
	../machine/exceptions.s ../machine/locore.s ../s200io/name.c \
	../dux/funcentry.c vers.c ../cdfs/cdfs_hooks.c

# Roughly, the objects constituting the PM system */
PMOBJS= pm_getpid.o pm_proc.o pm_procdup.o pm_core.o
PMSRCS= ../sys/pm_getpid.c ../sys/pm_proc.c ../sys/pm_procdup.c \
	../sys/pm_core.c

# Roughly, the objects consitituting the VM system */
VMOBJS= swapconf.o vm_fault.o vm_kern.o vm_mmap.o vm_mon.o vm_msem.o \
	vm_page.o vm_pgcache.o vm_vfd.o vm_region.o vm_sched.o vm_sw.o \
	vm_swalloc.o vm_swp.o vm_text.o vm_unhash.o vm_vhand.o \
	vm_vdma.o sem_beta.o spinlock.o lif.o hdl_bits.o hdl_trans.o \
	hdl_policy.o hdl_vdma.o pte.o hdl_fault.o \
	vm_pregion.o vm_vas.o vm_machdep.o io_map.o vm_kvm.o \
	vm_devswap.o hdl_init.o hdl_mprotect.o vm_misc.o hdl_swap.o \
	kern_mallo.o vm_textcache.o atl.o cache.o tlb.o

VMSRCS= swapconf.c ../sys/vm_fault.c ../sys/vm_kern.c ../sys/vm_mmap.c \
	../sys/vm_mon.c ../sys/vm_msem.c ../sys/vm_page.c \
	../sys/vm_pgcache.c ../sys/vm_vfd.c \
	../sys/vm_region.c ../sys/vm_sched.c ../sys/vm_sw.c \
	../sys/vm_swalloc.c ../sys/vm_swp.c ../sys/vm_text.c \
	../sys/vm_unhash.c ../sys/vm_vhand.c ../sys/vm_vdma.c \
	../sys/sem_beta.c ../sys/spinlock.c ../sys/lif.c \
	../machine/hdl_bits.c ../machine/hdl_trans.c \
	 ../machine/hdl_policy.c ../machine/hdl_vdma.c ../machine/pte.c \
	../machine/hdl_fault.c ../sys/vm_pregion.c ../sys/vm_vas.c \
	../machine/vm_machdep.c ../machine/io_map.c ../sys/vm_kvm.c \
	../sys/vm_devswap.c ../machine/hdl_init.c ../machine/hdl_mprotect.c \
	../sys/vm_misc.c ../machine/hdl_swap.c \
	../sys/kern_mallo.c ../sys/vm_textcache.c ../machine/atl.c \
	../machine/cache.s ../machine/tlb.s

# Required objects to build a kernel */
KREQ= \
	clocks.o clock.o float.o init_main.o init_sent.o interrupts.o \
	kern_acct.o kern_clock.o kern_dscrp.o kern_exec.o \
	kern_fork.o kern_mman.o kern_proc.o kern_prot.o kern_resrc.o \
	kern_sig.o kern_subr.o kern_synch.o kern_time.o kern_xxx.o \
	machdep.o mem.o umem.o quot_scall.o quota.o \
	quota_ufs.o subr_mcnt.o subr_misc.o subr_prf.o subr_rmap.o \
	subr_xxx.o sys_gen.o sys_mchdep.o sys_plock.o sys_proc.o \
	sys_prvgrp.o sysent_old.o trap.o kern_exit.o ufs_alloc.o \
	ufs_bmap.o ufs_dsort.o ufs_fifo.o ufs_inode.o ufs_lockf.o \
	ufs_mchdep.o ufs_subr.o ufs_tables.o utssys.o \
	ufs_setacl.o vfs.o vfs_bio.o dbc_bio.o vfs_conf.o vfs_dev.o \
	vfs_dnlc.o vfs_io.o vfs_lookup.o vfs_pathnm.o vfs_pn_ct.o \
	vfs_pn_nc.o vfs_scalls.o meas_sys.o \
	vfs_vnode.o vfs_xxx.o vfs_vm.o \
	ufs_vfsops.o ufs_pmount.o ufs_vnops.o ufs_dir.o \
	sys_ki.o audctl.o audit.o audswitch.o \
	audwrite.o getaudid.o getaudproc.o getevent.o kern_audwr.o \
	pstat.o meas_drivr.o kern_db.o \
	netfunc.o netisr.o tty_bk.o tty_tb.o queue.o dgram_aud.o \
	nm_gen.o nm_sys.o net_diag.o netdiag1.o m68040_float.o \
        netmp.o dux_opend.o dux_sdo.o
# ddb.o ddb_rs232.o ddb_asm.o XXX put these back if you want to run ddb

KREQSRC= \
	../machine/clocks.s ../machine/clock.c ../machine/float.c \
	../sys/init_main.c ../sys/init_sent.c  ../machine/interrupts.s \
	../sys/kern_acct.c ../sys/kern_clock.c \
	../sys/kern_dscrp.c ../sys/kern_exec.c \
	../sys/kern_fork.c ../sys/kern_mman.c ../sys/kern_proc.c \
	../sys/kern_prot.c ../sys/kern_resrc.c ../sys/kern_sig.c \
	../sys/kern_subr.c ../sys/kern_synch.c ../sys/kern_time.c \
	../sys/kern_xxx.c ../machine/machdep.c ../machine/mem.c \
	../ufs/quot_scall.c ../ufs/quota.c ../ufs/quota_ufs.c \
        ../sys/subr_mcnt.c ../machine/umem.c \
	../sys/subr_misc.s ../sys/subr_prf.c ../sys/subr_rmap.c \
	../sys/subr_xxx.c ../sys/sys_gen.c ../machine/sys_mchdep.c \
	../sys/sys_plock.c ../sys/sys_proc.c ../sys/sys_prvgrp.c \
	../machine/sysent_old.c ../machine/trap.c ../sys/kern_exit.c \
	../ufs/ufs_alloc.c ../ufs/ufs_bmap.c ../ufs/ufs_dsort.c \
	../ufs/ufs_fifo.c ../ufs/ufs_inode.c ../ufs/ufs_lockf.c \
	../machine/ufs_mchdep.c ../ufs/ufs_subr.c ../ufs/ufs_tables.c \
	../sys/utssys.c ../ufs/ufs_setacl.c ../sys/vfs.c ../sys/vfs_bio.c \
	../sys/vfs_conf.c ../sys/vfs_dev.c ../sys/vfs_dnlc.c ../sys/vfs_io.c \
	../sys/vfs_lookup.c ../sys/vfs_pathnm.c ../sys/vfs_pn_ct.c \
	../sys/vfs_pn_nc.c ../sys/vfs_scalls.c ../sys/meas_sys.c \
	../sys/vfs_vnode.c ../sys/vfs_xxx.c ../sys/vfs_vm.c \
	../ufs/ufs_vfsops.c ../sys/dbc_bio.c \
	../ufs/ufs_pmount.c ../ufs/ufs_vnops.c ../ufs/ufs_dir.c \
	../sys/sys_ki.c ../sys/audctl.c ../sys/audit.c ../sys/audswitch.c \
	../sys/audwrite.c ../sys/getaudid.c ../sys/getaudproc.c \
	../sys/getevent.c ../sys/kern_audwr.c ../sys/pstat.c \
	../sys/meas_drivr.c ../sys/kern_db.c ../sys/netfunc.c \
	../net/netisr.c ../sys/tty_bk.c ../sys/tty_tb.c ../machine/queue.s \
	../sys/dgram_aud.c ../netinet/nm_gen.c ../netinet/nm_sys.c \
	../sys/net_diag.c ../sys/netdiag1.c \
        ../net/netmp.c ../dux/dux_sdo.c ../dux/dux_opend.c
# ../machine/ddb.c ../machine/ddb_rs232.c ../machine/ddb_asm.s

# System V communication modules */
SYSV= \
	sysV_ipc.o sysV_map.o sysV_msg.o sysV_sem.o sysV_shm.o
SYSVSRC= \
	../sys/sysV_ipc.c ../sys/sysV_map.c ../sys/sysV_msg.c \
	../sys/sysV_sem.c ../sys/sysV_shm.c

DREQ= \
	audio.o cons.o dma.o hil_code.o hil_drv.o hpib.o iomap.o ite.o \
	ites.o ite_pars.o ite_scroll.o ite_driv.o kbd_code.o \
	kbd_table.o dio2.o kernel.o graf.o partab.o selcode.o \
	timeout.o tt0.o tty.o tty_conf.o tty_sgtty.o \
	tty_subr.o tty_tty.o dmachain.o ite_tiger.o \
	ite_ren.o ite_color.o ite_font.o ite_top.o \
	ite_alpha.o ite_sti.o ite_dav.o ite_gator.o ite_hyper.o \
	ite_gen.o gr_98736.o eisa_int.o dconfig.o

DREQSRC= \
	../wsio/audio.c ../s200io/cons.c \
	../s200io/dma.c ../graf/hil_code.c \
	../graf/hil_drv.c ../wsio/hpib.c ../s200io/iomap.c \
	../graf/ite.c ../graf/ites.s ../graf/ite_pars.c \
	../graf/ite_scroll.c ../graf/ite_driv.c ../graf/kbd_code.c \
	../graf/kbd_table.c ../s200io/dio2.c ../s200io/kernel.c \
	../graf/graf.c ../s200io/partab.c ../wsio/selcode.c \
	../wsio/timeout.c ../sys/tt0.c ../sys/tty.c \
	../sys/tty_conf.c ../sys/tty_sgtty.c ../sys/tty_subr.c \
	../sys/tty_tty.c ../s200io/dmachain.s ../graf/ite_tiger.c \
	../graf/ite_ren.c ../graf/ite_color.c ../graf/ite_font.c \
	../graf/ite_top.c ../graf/ite_alpha.c ../graf/ite_sti.c \
	../graf/ite_dav.c ../graf/ite_gator.c ../graf/ite_hyper.c \
	../graf/ite_gen.c ../graf/gr_98736.c ../wsio/eisa_int.c \
	../wsio/dconfig.c

MIN= \
	cs80.o scsi_if.o scsi.o scsi_ccs.o dil_hpib.o dil.o hpibio.o \
	mux.o muxs.o pci.o pdi.o pdis.o rje.o ttycomn.o hshpib.o \
	simons.o ti9914.o ti9914s.o parallel.o apollo_pci.o \
	sds.o sds_parse.o

MINSRC= \
	../wsio/cs80.c ../s200io/scsi_if.c ../s200io/scsi.c \
	../s200io/scsi_ccs.c ../s200io/dil_hpib.c ../wsio/dil.c \
	../wsio/hpibio.c ../s200io/mux.c ../s200io/muxs.s ../s200io/pci.c \
	../s200io/pdi.c ../s200io/pdis.s ../s200io/rje.c ../s200io/ttycomn.c \
	../wsio/hshpib.c ../s200io/simons.s ../s200io/ti9914.c \
	../s200io/ti9914s.s ../s200io/parallel.c ../s200io/apollo_pci.c \
	../wsio/sds.c ../wsio/sds_parse.c

DILSRM= \
	dil_gpio.o srm.o

DILSRMSRC= \
	../s200io/dil_gpio.c ../s200io/srm.c

DEVELOP= \
	amigo.o ciper.o lph.o mf.o mfd.o pt.o tp.o tty_pty.o stp.o dos.o \
	dragon.o vme.o bcdvme.o bcdvmeas.o ram_disc.o vtest.o vtestisr.o \
	scsitape.o autox.o autoch.o eisa.o eeprom.o io_subr.o \
	etest.o eisa_sysbrd.o strace.o

DEVELOPSRC= \
	../s200io/amigo.c ../s200io/ciper.c ../s200io/lph.c ../s200io/mf.c \
	../s200io/mfd.s ../s200io/pt.c ../s200io/tp.c ../sys/tty_pty.c \
	../s200io/stp.c ../s200io/dos.c ../s200io/dragon.c ../s200io/vme.c \
	../s200io/bcdvme.c ../s200io/bcdvmeas.s ../s200io/ram_disc.c \
	../s200io/vtest.c ../s200io/vtestisr.c ../s200io/scsitape.c \
	../s200io/autox.c ../sio/autoch.c ../wsio/eisa.c ../wsio/eeprom.c \
	../wsio/etest.c ../wsio/io_subr.c ../wsio/eisa_sysbrd.c \
	../s200io/strace.c

CDFS=	\
	cdfs_clnup.o cdfs_cdnod.o cdfs_subr.o duxcd_lkup.o \
	cdfs_vfsop.o cdfs_vnops.o cdfs_vm.o duxcd_vnop.o duxcdstrat.o

CDFSSRC= \
	../dux/cdfs_clnup.c ../cdfs/cdfs_cdnod.c ../dux/duxcd_lkup.c \
	../cdfs/cdfs_subr.c ../cdfs/cdfs_vfsop.c ../cdfs/cdfs_vnops.c \
	../cdfs/cdfs_vm.c ../dux/duxcd_vnop.c ../dux/duxcdstrat.c

#
# The following list of DUX objects appears to be the minimal set
# required to get a kernel to link, regardless of whether or not the
# system is configured for diskless.  Why is this so much larger than
# on an 800??
#
DUXREQ = \
	dux_exec.o dux_fifo.o dux_lkmsg.o dux_lockf.o dux_lookup.o \
	dux_mount.o dux_pseudo.o dux_scalls.o \
	dux_sync.o dux_vnops.o fixed_rmap.o lookuptab.o sitemap.o \
	syncio.o

DUXREQSRC = \
	../dux/dux_exec.c ../dux/dux_fifo.c ../dux/dux_lkmsg.c \
	../dux/dux_lockf.c ../dux/dux_lookup.c ../dux/dux_mount.c \
	../dux/dux_pseudo.c ../dux/dux_scalls.c \
	../dux/dux_sync.c ../dux/dux_vnops.c \
	../dux/fixed_rmap.c ../dux/lookuptab.c ../dux/sitemap.c \
	../dux/syncio.c

#
# The following list of DUX objects are not required to get a kernel to
# link.
#
DSKLESSOBJS= \
	cluster.o clocksync.o dm.o dux_audit.o dux_bigio.o dux_config.o \
	dux_hooks.o dux_mbuf.o \
	dux_rds.o dux_rdu.o duxstrat.o fs_cleanup.o \
	netbuf.o nsp.o printtime.o protocol.o recovery.o rmswap.o \
	selftest.o unsp.o dux_getpid.o test.o

DSKLESSSRCS= \
	../dux/cluster.c ../dux/clocksync.c ../dux/dm.c ../dux/dux_audit.c \
	../dux/dux_bigio.c ../dux/dux_config.c ../dux/dux_hooks.c \
	../dux/dux_mbuf.c ../dux/dux_rds.c ../dux/dux_rdu.c ../dux/duxstrat.c \
	../dux/fs_cleanup.c ../dux/netbuf.c ../dux/nsp.c ../dux/printtime.c \
	../dux/protocol.c ../dux/recovery.c ../dux/rmswap.c ../dux/selftest.c \
	../dux/unsp.c ../dux/dux_getpid.c ../dux/test.c

LANOBJS= \
	drv_lan0.o drvhw.o drvst.o lanc.o lanc_input.o drv_init.o lanc_dda.o

LANSRCS= \
	../s200io/drv_lan0.c ../s200io/drvhw.c ../s200io/drvst.s \
	../sio/lanc.c ../sio/lanc_input.c ../s200io/drv_init.c ../sio/lanc_dda.c

UIPCOBJS= \
	uipc_domain.o uipc_init.o uipc_mbuf.o uipc_proto.o uipc_socket.o \
	uipc_socket2.o uipc_syscall.o uipc_usrreq.o sys_socket.o netioctl.o \
	uipc_compat.o

UIPCSRCS= \
	../sys/uipc_domain.c ../sys/uipc_init.c ../sys/uipc_mbuf.c \
	../sys/uipc_proto.c ../sys/uipc_socket.c ../sys/uipc_socket2.c \
	../sys/uipc_syscall.c ../sys/uipc_usrreq.c ../sys/sys_socket.c \
	../sys/netioctl.c ../sys/uipc_compat.c

NIPCOBJS= \
	nipc_compat.o nipc_err.o nipc_fops.o nipc_hpdsn.o nipc_init.o \
	nipc_misc.o nipc_name.o nipc_path.o nipc_new_sys.o nipc_sou.o \
	nipc_sr.o nipc_srd.o nipc_syscall.o prb_name.o pxp.o

NIPCSRCS= \
	../nipc/nipc_compat.c \
	../nipc/nipc_err.c ../nipc/nipc_fops.c ../nipc/nipc_hpdsn.c \
	../nipc/nipc_init.c ../nipc/nipc_misc.c ../nipc/nipc_name.c \
	../nipc/nipc_path.c ../nipc/nipc_new_sys.c ../nipc/nipc_sou.c \
	../nipc/nipc_sr.c ../nipc/nipc_srd.c ../nipc/nipc_syscall.c \
	../nipc/prb_name.c ../nipc/pxp.c

INETOBJS= \
	if_ether.o in.o in_cksum.o scksum.o in_pcb.o in_proto.o inet_init.o \
	ip_icmp.o ip_input.o ip_output.o raw_ip.o tcp_debug.o tcp_input.o \
 	tcp_output.o tcp_subr.o tcp_timer.o tcp_usrreq.o udp_usrreq.o \
 	nm_icmp.o nm_ip.o nm_tcp.o nm_udp.o tty_nvs.o

INETSRCS= \
	../netinet/if_ether.c ../netinet/in.c \
	../machine/in_cksum.c ../machine/scksum.s \
	../netinet/in_pcb.c ../netinet/in_proto.c ../netinet/inet_init.c \
	../netinet/ip_icmp.c ../netinet/ip_input.c ../netinet/ip_output.c \
	../netinet/raw_ip.c ../netinet/tcp_debug.c ../netinet/tcp_input.c \
	../netinet/tcp_output.c ../netinet/tcp_subr.c ../netinet/tcp_timer.c \
  	../netinet/tcp_usrreq.c ../netinet/udp_usrreq.c ../netinet/nm_icmp.c \
  	../netinet/nm_ip.c ../netinet/nm_tcp.c ../netinet/nm_udp.c \
	../sys/tty_nvs.c

NETOBJS= \
	af.o if.o if_loop.o if_ni.o net_init.o raw_cb.o raw_usrreq.o \
	route.o nm_if.o

NETSRCS= \
	../net/af.c ../net/if.c ../net/if_loop.c ../net/if_ni.c \
	../net/net_init.c ../net/raw_cb.c ../net/raw_usrreq.c \
	../net/route.c ../net/nm_if.c ../net/netmp.c

NFSOBJS= \
	auth_kern.o authux_pro.o clnt_kudp.o fifo_vnops.o \
	klm_kprot.o klm_lckmgr.o kudp_fsend.o nfs_export.o nfs_server.o \
	nfs_subr.o nfs_vers.o nfs_vfsops.o nfs_vnops.o nfs_xdr.o \
	pmap_kport.o pmap_prot.o rpc_prot.o spec_subr.o spec_vnops.o \
	subr_kudp.o svc.o svc_auth.o svc_authux.o svc_kudp.o xdr.o \
	xdr_array.o xdr_mbuf.o xdr_mem.o

NFSSRCS= \
	../rpc/auth_kern.c ../rpc/authux_pro.c \
	../rpc/clnt_kudp.c ../nfs/fifo_vnops.c ../nfs/klm_kprot.c \
	../nfs/klm_lckmgr.c ../rpc/kudp_fsend.c ../ufs/fs_debug.c \
	../nfs/nfs_export.c ../nfs/nfs_server.c ../nfs/nfs_subr.c \
	../nfs/nfs_vers.c \
	../nfs/nfs_vfsops.c ../nfs/nfs_vnops.c ../nfs/nfs_xdr.c \
	../rpc/pmap_kport.c ../rpc/pmap_prot.c ../rpc/rpc_prot.c \
	../nfs/spec_subr.c ../nfs/spec_vnops.c ../rpc/subr_kudp.c ../rpc/svc.c \
	../rpc/svc_auth.c ../rpc/svc_authux.c ../rpc/svc_kudp.c ../rpc/xdr.c \
	../rpc/xdr_array.c ../rpc/xdr_mbuf.c ../rpc/xdr_mem.c

# S300 Kernel Instruction Coverage Analyzer (aka. ICA)
ICAOBJS= ica.o kern_ica.o
ICASRCS= ../machine/ica.c ../machine/kern_ica.c

DEBUG_NMS = debug.nms.o
DEBUG_NMSSRC = debug.nms.c

OBJS= \
	conf.o $(PMOBJS) $(VMOBJS) $(KREQ) $(SYSV) $(DREQ) \
	$(MIN) $(DILSRM) $(DEVELOP) $(CDFS) $(DUXREQ)      \
	$(LANOBJS) $(UIPCOBJS) $(NIPCOBJS) $(INETOBJS)     \
	$(NETOBJS) $(NFSOBJS) $(DEBUG_NMS) $(DSKLESSOBJS) $(ICAOBJS)

SRCS= \
	conf.c $(PMSRCS) $(VMSRCS) $(KREQSRC) $(SYSVSRC) $(DREQSRC)  \
	$(MINSRC) $(DILSRMSRC) $(DEVELOPSRC) $(CDFSSRC) $(DUXREQSRC) \
	$(LANSRCS) $(UIPCSRCS) $(NIPCSRCS) $(INETSRCS) $(NETSRCS)    \
	$(NFSSRCS) $(DEBUG_NMSSRC) $(DSKLESSSRCS) $(ICASRCS)

LDFLAGS= -n -x
KLIBS= $(LIBC)
SRCDIRS= \
	cdfs dux machine net netinet nfs rpc sys sys ufs ufs s200io wsio
target: hp-ux

ident:
	@echo ${COPTS} | sed -e s/-[UD][_]*hp9000s[238]00//g -e s/-DKERNEL// -e s/-D_KERNEL// -e s/-D_HPUX_SOURCE//

objs: ${LINKS} $(OBJS) $(KERNOBJS)

${LINKS}:
	/bin/rm -f ../sys.300 ../h.300 ../mach.300 ../graf.300 ../s200 ${LINKS}
	ln -s h ../h.300
	if /bin/ksh -c "test -d ../mach"; then      \
	/bin/mv ../mach ../machine;             \
	fi
	ln -s machine ../mach.300
	ln -s machine ../s200
	ln -s graf ../graf.300
	ln -s sys ../sys.300
	touch ${LINKS}

# Compile & link of large kernel */
hp-ux.old: ${LINKS} $(KERNOBJS) $(OBJS)
	@${RM} -f $@
	@echo Linking hp-ux...
	@$(LD) $(LDFLAGS) -e _start -o hp-ux $(KERNOBJS) $(OBJS) $(XOBJS) $(KLIBS)
	@echo hp-ux made.

# Fast link of large kernel */
ld:
	@${RM} -f hp-ux
	@echo Linking hp-ux...
	@$(LD) $(LDFLAGS) -e _start -o hp-ux $(KERNOBJS) $(OBJS) $(XOBJS) $(KLIBS)
	@echo hp-ux made.

# Just link pieces from libraries and a few .o files.
# Useful in a UDL clone.
it: conf.c
	@${RM} -f hp-ux		# force config.mk to rebuild hp-ux
	@echo Loading hp-ux...
	$(MAKE) -f config.mk
	size hp-ux

# Create tags */
tags:
	tagsin $(SRCDIRS)

lint: /tmp
	@${LINT} -hbn -I. ${LINT_CFLAGS} \
		../machine/Locore.c conf.c param.c ${SRCS} | \
		grep -v 'nonportable character comparison'

#  This library isn't built by this makefile but is necessary for the
#  config.mk makefile.  It gets delivered into /etc/conf/libk.a.

libk.a: /etc/conf/libk.a
	@${RM} -f $@
	cp /etc/conf/libk.a libk.a

# Libraries to build when requested */
LIBS= libdskless.a libvm.a libsysV.a libdevelop.a libdil_srm.a \
	liblan.a libuipc.a libnipc.a libinet.a libnet.a \
	libduxreq.a libkern.a libdebug.a libpm.a libnfs.a libcdrom.a \
	libkreq.a libdreq.a libmin.a libinet.a 

libpm.a : ${PMOBJS}
	@${RM} -f $@
	@echo making archive library libpm.a
	@$(AR) -r libpm.a ${PMOBJS}

libkern.a : $(KERNOBJS)
	@${RM} -f $@
	@echo making archive library libkern.a
	@$(AR) -r libkern.a ${KERNOBJS}

libvm.a : ${VMOBJS}
	@${RM} -f $@
	@echo making archive library libvm.a
	@$(AR) -r libvm.a ${VMOBJS}

libkreq.a : $(KERNOBJS) ${KREQ}
	@${RM} -f $@
	@echo making archive library libkreq.a
	@$(AR) -r libkreq.a $(KERNOBJS) ${KREQ}

libsysV.a : ${SYSV}
	@${RM} -f $@
	@echo making archive library libsysV.a
	@$(AR) -r libsysV.a ${SYSV}

libdreq.a : ${DREQ}
	@${RM} -f $@
	@echo making archive library libdreq.a
	@$(AR) -r libdreq.a ${DREQ}

libmin.a : ${MIN}
	@${RM} -f $@
	@echo making archive library libmin.a
	@$(AR) -r libmin.a ${MIN}

libdil_srm.a : ${DILSRM}
	@${RM} -f $@
	@echo making archive library libdil_srm.a
	@$(AR) -r libdil_srm.a ${DILSRM}

libdevelop.a : ${DEVELOP}
	@${RM} -f $@
	@echo making archive library libdevelop.a
	@$(AR) -r libdevelop.a ${DEVELOP}

libcdrom.a : ${CDFS}
	@${RM} -f $@
	@echo making archive library libcdrom.a
	@$(AR) -r libcdrom.a ${CDFS}

libdskless.a : ${DSKLESSOBJS}
	@${RM} -f $@
	@echo making archive library libdskless.a
	@$(AR) -r libdskless.a ${DSKLESSOBJS}

libdebug.a : ${DEBUG_NMS}
	@${RM} -f $@
	@echo making archive library libdebug.a
	@$(AR) -r libdebug.a ${DEBUG_NMS}

libduxreq.a : ${DUXREQ}
	@${RM} -f $@
	@echo making archive library libduxreq.a
	@$(AR) -r libduxreq.a ${DUXREQ}

# Somebody in networking should name these the way they SHOULD be */
liblan.a : ${LANOBJS}
	@${RM} -f $@
	@echo making archive library liblan.a
	@$(AR) -r liblan.a ${LANOBJS}

libuipc.a : ${UIPCOBJS}
	@${RM} -f $@
	@echo making archive library libuipc.a
	@$(AR) -r libuipc.a ${UIPCOBJS}

libnipc.a : ${NIPCOBJS}
	@${RM} -f $@
	@echo making archive library libnipc.a
	@$(AR) -r libnipc.a ${NIPCOBJS}

libinet.a : ${INETOBJS}
	@${RM} -f $@
	@echo making archive library libinet.a
	@$(AR) -r libinet.a ${INETOBJS}

libnet.a : ${NETOBJS}
	@${RM} -f $@
	@echo making archive library libnet.a
	@$(AR) -r libnet.a ${NETOBJS}

libnfs.a : ${NFSOBJS}
	@${RM} -f $@
	@echo making archive library libnfs.a
	@$(AR) -r libnfs.a ${NFSOBJS}

fromlibs: conf.c ${LINKS} $(LIBS) libk.a
	@${RM} -f hp-ux
	make -f config.mk
	@echo Done.

#
# used for quick link of changed *.o's in build
# directory without checking library and header dependencies.
#
hp-ux: config.internl conf.c
	@${RM} -f hp-ux
	make -f config.mk
	@size hp-ux


#
# Generic descriptions of building .o's
#
cdfs_cdnod.o cdfs_subr.o cdfs_vfsop.o cdfs_vm.o \
cdfs_vnops.o: ../cdfs/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../cdfs/$(@:.o=.c)

cdfs_clnup.o clocksync.o cluster.o dm.o dux_audit.o dux_bigio.o \
dux_config.o dux_exec.o dux_fifo.o dux_getpid.o dux_hooks.o dux_lkmsg.o \
dux_lockf.o dux_lookup.o dux_mbuf.o dux_mount.o dux_opend.o dux_pseudo.o \
dux_rds.o dux_rdu.o dux_scalls.o dux_sdo.o dux_sync.o dux_vnops.o \
duxcd_lkup.o duxcd_vnop.o duxcdstrat.o duxk_lkup.o duxk_vnops.o duxkstrat.o \
duxstrat.o fixed_rmap.o fs_cleanup.o lookuptab.o netbuf.o nsp.o printswap.o \
printtime.o protocol.o recovery.o rmswap.o selftest.o sitemap.o syncio.o \
test.o unsp.o: ../dux/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../dux/$(@:.o=.c)

gr_98736.o graf.o hil_code.o hil_drv.o ite.o ite_alpha.o ite_dav.o \
ite_driv.o ite_font.o ite_gator.o ite_gen.o ite_hyper.o ite_pars.o \
ite_ren.o ite_scroll.o ite_sti.o ite_tiger.o ite_top.o kbd_code.o \
kbd_table.o: ../graf/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../graf/$(@:.o=.c)

ite_color.o: ../graf/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../graf/$(@:.o=.c)

kfs_knode.o kfs_subr.o kfs_vfsops.o kfs_vnops.o: ../kfs/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../kfs/$(@:.o=.c)

atl.o clock.o float.o hdl_bits.o hdl_fault.o hdl_init.o hdl_mprotect.o \
hdl_policy.o hdl_swap.o hdl_trans.o hdl_vdma.o ica.o in_cksum.o io_map.o \
kern_ica.o machdep.o mem.o umem.o pte.o sys_mchdep.o sysent_old.o ufs_mchdep.o \
ddb.o ddb_rs232.o \
vm_machdep.o: ../machine/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../machine/$(@:.o=.c)

af.o if.o if_loop.o if_ni.o net_init.o netisr.o netmp.o nm_if.o raw_cb.o \
raw_usrreq.o route.o: ../net/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../net/$(@:.o=.c)

if_ether.o in.o in_pcb.o in_proto.o inet_init.o ip_icmp.o ip_input.o \
ip_output.o nm_gen.o nm_icmp.o nm_ip.o nm_sys.o nm_tcp.o nm_udp.o raw_ip.o \
tcp_debug.o tcp_input.o tcp_output.o tcp_subr.o tcp_timer.o tcp_usrreq.o \
udp_usrreq.o: ../netinet/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../netinet/$(@:.o=.c)

fifo_vnops.o klm_kprot.o klm_lckmgr.o nfs_export.o nfs_server.o nfs_subr.o \
nfs_vers.o \ nfs_vfsops.o nfs_vnops.o nfs_xdr.o spec_subr.o spec_vnops.o \
: ../nfs/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../nfs/$(@:.o=.c)

nipc_compat.o nipc_err.o nipc_fops.o nipc_hpdsn.o nipc_init.o nipc_misc.o \
nipc_name.o nipc_new_sys.o nipc_path.o nipc_sou.o nipc_sr.o nipc_srd.o \
nipc_syscall.o prb_name.o pxp.o: ../nipc/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../nipc/$(@:.o=.c)

auth_kern.o authux_pro.o clnt_kudp.o kudp_fsend.o pmap_kport.o pmap_prot.o \
rpc_prot.o subr_kudp.o svc.o svc_auth.o svc_authux.o svc_kudp.o xdr.o \
xdr_array.o xdr_mbuf.o xdr_mem.o: ../rpc/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../rpc/$(@:.o=.c)

amigo.o apollo_pci.o autox.o bcdvme.o ciper.o cons.o \
dil_gpio.o dil_hpib.o dio2.o dma.o dos.o dragon.o drv_init.o drv_lan0.o \
drvhw.o iomap.o \
kernel.o lph.o mf.o mux.o parallel.o partab.o pci.o pdi.o pt.o ram_disc.o \
rje.o scsi.o scsi_ccs.o scsi_if.o scsitape.o srm.o strace.o \
stp.o ti9914.o tp.o ttycomn.o vme.o vtest.o \
vtestisr.o: ../s200io/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../s200io/$(@:.o=.c)

autoch.o lanc.o lanc_dda.o lanc_input.o: ../sio/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../sio/$(@:.o=.c)

init_main.o kern_clock.o netioctl.o pm_procdup.o subr_prf.o subr_xxx.o \
sys_proc.o tt0.o tty.o tty_conf.o tty_pty.o tty_subr.o \
tty_tty.o: ../sys/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../sys/$(@:.o=.c)

audctl.o audit.o audswitch.o audwrite.o dgram_aud.o getaudid.o getaudproc.o \
getevent.o init_sent.o kern_acct.o kern_audwr.o kern_db.o kern_dscrp.o \
kern_exec.o kern_exit.o kern_fork.o kern_mallo.o kern_mman.o kern_proc.o \
kern_prot.o kern_resrc.o kern_sig.o kern_subr.o kern_synch.o kern_time.o \
kern_xxx.o lif.o meas_drivr.o meas_sys.o net_diag.o netdiag1.o netfunc.o \
pm_core.o pm_getpid.o pm_proc.o pstat.o sem_beta.o spinlock.o subr_rmap.o \
sysV_ipc.o sysV_map.o sysV_msg.o sysV_sem.o sysV_shm.o sys_gen.o sys_ki.o \
sys_plock.o sys_prvgrp.o sys_socket.o tty_bk.o tty_nvs.o \
tty_sgtty.o tty_tb.o uipc_compat.o uipc_domain.o uipc_init.o uipc_mbuf.o \
uipc_proto.o uipc_socket.o uipc_socket2.o uipc_syscall.o uipc_usrreq.o \
utssys.o vfs.o vfs_bio.o dbc_bio.o vfs_conf.o vfs_dev.o vfs_dnlc.o vfs_io.o \
vfs_lookup.o vfs_pathnm.o vfs_pn_ct.o vfs_pn_nc.o vfs_scalls.o vfs_vm.o \
vfs_vnode.o vfs_xxx.o vm_devswap.o vm_fault.o vm_kern.o vm_kvm.o vm_misc.o \
vm_mmap.o vm_mon.o vm_msem.o vm_page.o vm_pgcache.o vm_pregion.o vm_region.o \
vm_sched.o vm_sw.o vm_swalloc.o vm_swp.o vm_text.o vm_textcache.o \
vm_unhash.o vm_vas.o vm_vdma.o vm_vfd.o vm_vhand.o : ../sys/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../sys/$(@:.o=.c)

fs_debug.o quot_scall.o quota.o quota_ufs.o ufs_alloc.o ufs_bmap.o \
ufs_dir.o ufs_dsort.o ufs_fifo.o ufs_fio.o ufs_inode.o ufs_lockf.o \
ufs_pmount.o ufs_setacl.o ufs_subr.o ufs_tables.o ufs_vfsops.o \
ufs_vnops.o: ../ufs/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../ufs/$(@:.o=.c)

audio.o eeprom.o eisa.o etest.o io_subr.o dil.o hpib.o hpibio.o eisa_int.o \
timeout.o selcode.o eisa_sysbrd.o sds.o dconfig.o hshpib.o cs80.o \
sds_parse.o: ../wsio/$$(@:.o=.c)
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../wsio/$(@:.o=.c)

#
# Special .o files
#
swapconf.o: swapconf.c
	@${RM} -f $@
	$(CC) $(CFLAGS) -c swapconf.c

funcentry.o: ../dux/funcentry.c
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../dux/funcentry.c
	@if [ -r libkreq.a -a ! -w libkreq.a ]; then	\
	    rm -f Tlibkreq.a;				\
	    cp libkreq.a Tlibkreq.a;			\
	    rm -f libkreq.a;				\
	    mv Tlibkreq.a libkreq.a;			\
	fi;						\
	if [ -w libkreq.a ]; then			\
	    ar -rv libkreq.a funcentry.o;		\
	fi

version.o: version.c
	@${RM} -f $@
	$(CC) $(CFLAGS) -c version.c

conf.o: conf.c ../h/space.h ../machine/space.h
	@${RM} -f $@
	$(CC) $(CFLAGS) -c conf.c

genassym.o: ../machine/genassym.c
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../machine/genassym.c

genassym: genassym.o
	@${RM} -f $@
	$(CC) $(CFLAGS) -o genassym genassym.o

assym.s: genassym
	@${RM} -f $@
	./genassym > assym.s

#
# This awk stuff is to make sure that mcount doesn't recursivly try to
# call itself even if subr_misc.c is compiled with -G
subr_mcnt.o: ../sys/subr_mcnt.c
	@${RM} -f $@ subr_mcnt.s
	$(CC) $(CFLAGS) -S ../sys/subr_mcnt.c
	awk '\
		BEGIN { foundmcount = 0 ; \
			deleted = 0 ; \
			} \
		/^_mcount:/ { foundmcount = 1 ; \
			} \
		{ \
			if (foundmcount == 0) { print $$0; next; } \
			if ($$1 == "mov.l") { deleted = 1 ; } \
			if ($$0 ~ /text$$/ && foundmcount) \
			{ \
				if (deleted == 0) { print $$0; } \
				deleted = 0; foundmcount = 0; next; \
			} \
			if (deleted == 0) { print $$0 ; } \
		}' < subr_mcnt.s | $(AS) -o subr_mcnt.o

#  Assembly routine are taken care of here
clocks.o: ../machine/clocks.s assym.s
	@${RM} -f $@
	cat assym.s ../machine/clocks.s | $(CPP) $(DEFS) | \
	    $(AS) -o clocks.o

exceptions.o: ../machine/exceptions.s assym.s
	@${RM} -f $@
	cat assym.s ../machine/exceptions.s | $(CPP) $(DEFS) | \
	    $(AS) -o exceptions.o
	@if [ -r libkreq.a -a ! -w libkreq.a ]; then	\
	    rm -f Tlibkreq.a;				\
	    cp libkreq.a Tlibkreq.a;			\
	    rm -f libkreq.a;				\
	    mv Tlibkreq.a libkreq.a;			\
	fi;						\
	if [ -w libkreq.a ]; then			\
	    ar -rv libkreq.a exceptions.o;		\
	fi

locore.o: ../machine/locore.s assym.s
	@${RM} -f $@
	cat assym.s ../machine/locore.s | $(CPP) $(DEFS) | \
	    $(AS) -o locore.o
	@if [ -r libkreq.a -a ! -w libkreq.a ]; then	\
	    rm -f Tlibkreq.a;				\
	    cp libkreq.a Tlibkreq.a;			\
	    rm -f libkreq.a;				\
	    mv Tlibkreq.a libkreq.a;			\
	fi;						\
	if [ -w libkreq.a ]; then			\
	    ar -rv libkreq.a locore.o;			\
	fi

ddb_asm.o: ../machine/ddb_asm.s assym.s
	@${RM} -f $@
	cat assym.s ../machine/ddb_asm.s | $(CPP) $(DEFS) | \
	    $(AS) -o ddb_asm.o

cache.o: ../machine/cache.s assym.s
	@${RM} -f $@
	cat assym.s ../machine/cache.s | $(CPP) $(DEFS) | \
	    $(AS) -o cache.o

interrupts.o: ../machine/interrupts.s assym.s
	@${RM} -f $@
	cat assym.s ../machine/interrupts.s | $(CPP) $(DEFS) | \
	    $(AS) -o interrupts.o

tlb.o: ../machine/tlb.s assym.s
	@${RM} -f $@
	cat assym.s ../machine/tlb.s | $(CPP) $(DEFS) | \
	    $(AS) -o tlb.o

queue.o: ../machine/queue.s
	@${RM} -f $@
	$(AS) -o queue.o < ../machine/queue.s

scksum.o: ../machine/scksum.s
	@${RM} -f $@
	$(AS) -o scksum.o < ../machine/scksum.s

bcdvmeas.o: ../s200io/bcdvmeas.s
	@${RM} -f $@
	$(CPP) $(DEFS) ../s200io/bcdvmeas.s | $(AS) -o bcdvmeas.o

dmachain.o: ../s200io/dmachain.s assym.s
	@${RM} -f $@
	cat assym.s ../s200io/dmachain.s|$(CPP) $(DEFS) | \
	    $(AS) -o dmachain.o

drvst.o: ../s200io/drvst.s
	@${RM} -f $@
	$(CPP) $(DEFS) ../s200io/drvst.s | $(AS) -o drvst.o

ites.o: ../graf/ites.s
	@${RM} -f $@
	$(CPP) $(DEFS) ../graf/ites.s | $(AS) -o ites.o

mfd.o: ../s200io/mfd.s
	@${RM} -f $@
	$(CPP) $(DEFS) ../s200io/mfd.s | $(AS) -o mfd.o

muxs.o: ../s200io/muxs.s
	@${RM} -f $@
	$(CPP) $(DEFS) ../s200io/muxs.s | $(AS) -o muxs.o

pdis.o: ../s200io/pdis.s
	@${RM} -f $@
	$(CPP) $(DEFS) ../s200io/pdis.s | $(AS) -o pdis.o

simons.o: ../s200io/simons.s assym.s
	@${RM} -f $@
	cat assym.s ../s200io/simons.s|$(CPP) $(DEFS)|$(AS) -o simons.o

ti9914s.o: ../s200io/ti9914s.s
	@${RM} -f $@
	cat assym.s ../s200io/ti9914s.s|$(CPP) $(DEFS)|$(AS) -o ti9914s.o

subr_misc.o: ../sys/subr_misc.s
	@${RM} -f $@
	$(CPP) $(DEFS) ../sys/subr_misc.s | $(AS) -o subr_misc.o

pciint.o: ../s200io/pciint.s
	@${RM} -f $@
	$(CPP) $(DEFS) ../s200io/pciint.s | $(AS) -o pciint.o

debug.nms.o: debug.nms.c
	@${RM} -f $@
	$(CC) $(CFLAGS) -g +y -c debug.nms.c
	@if [ -r libkreq.a -a ! -w libkreq.a ]; then	\
	    rm -f Tlibkreq.a;				\
	    cp libkreq.a Tlibkreq.a;			\
	    rm -f libkreq.a;				\
	    mv Tlibkreq.a libkreq.a;			\
	fi;						\
	if [ -w libdebug.a ]; then			\
	    ar -rv libdebug.a debug.nms.o;		\
	fi

cdfs_hooks.o: ../cdfs/cdfs_hooks.c
	@${RM} -f $@
	$(CC) $(CFLAGS) -c ../cdfs/cdfs_hooks.c
	@if [ -r libkreq.a -a ! -w libkreq.a ]; then	\
	    rm -f Tlibkreq.a;				\
	    cp libkreq.a Tlibkreq.a;			\
	    rm -f libkreq.a;				\
	    mv Tlibkreq.a libkreq.a;			\
	fi;						\
	if [ -w libkreq.a ]; then			\
	    ar -rv libkreq.a cdfs_hooks.o;		\
	fi

name.o: ../s200io/name.c
	@${RM} -f $@
	$(CC) $(CFLAGS) -DUTSNAME_REL='"'$(REVISION)'"' -c ../s200io/name.c
	@if [ -r libkreq.a -a ! -w libkreq.a ]; then	\
	    rm -f Tlibkreq.a;				\
	    cp libkreq.a Tlibkreq.a;			\
	    rm -f libkreq.a;				\
	    mv Tlibkreq.a libkreq.a;			\
	fi;						\
	if [ -w libkreq.a ]; then			\
	    ar -rv libkreq.a name.o;			\
	fi

vers.o: vers.c
	@${RM} -f $@
	$(CC) -c vers.c
	@if [ -r libkreq.a -a ! -w libkreq.a ]; then	\
	    rm -f Tlibkreq.a;				\
	    cp libkreq.a Tlibkreq.a;			\
	    rm -f libkreq.a;				\
	    mv Tlibkreq.a libkreq.a;			\
	fi;						\
	if [ -w libkreq.a ]; then			\
	    ar -rv libkreq.a vers.o;			\
	fi

vers.c:
	@${RM} -f $@
	@echo 'char version[] = "@(#)' $(REV_STRING)'";' >$@


m68040_float.o:
	@${RM} -f $@
	cd ../float; $(MAKE) AS=$(AS) LD=$(LD)
	cp ../float/fpsp.o $@

trap.o: ../machine/trap.c assym.s
	@${RM} -f $@ trap.s
	$(CC) $(CFLAGS) -S ../machine/trap.c
	cat assym.s trap.s | $(AS) -o trap.o

#
# config program and rules for creating a development conf.c with it
#
CONF_DEFS_C= -DSAVECORE_300 -DAUTOCHANGER
CONF_DEFS=   -DSAVECORE_300 -DAUTOCHANGER -DNIGHTLY_BUILD
CONF_DEFS_I= -DSAVECORE_300 -DAUTOCHANGER -DNIGHTLY_BUILD -DCONFIG_INTERNAL
conf.c: config dfile.REG master.REG
	@${RM} -f $@ config.mk
	./config -c conf.c -m master.REG -l config.mk -n .. . dfile.REG

config.customer: config.c
	@${RM} -f $@
	$(CC) -o config.customer $(CONF_DEFS_C) config.c

config: config.c
	@${RM} -f $@
	$(CC) -o config $(CONF_DEFS) config.c

config.internl: config.c
	@${RM} -f $@
	$(CC) -o config.internl $(CONF_DEFS_I) config.c
	ln config.internl config

kgmon: kgmon.c ../h/gprof.h
	@${RM} -f $@
	$(CC) -O -o kgmon kgmon.c

dependencies: ${LINKS} assym.s $(SRCS) $(KERNSRCS) ../machine/genassym.c
	@echo "\tUpdating dependency information..."
	@depends -v -O "-I. ${COPTS}" -u \
	    $(SRCS) $(KERNSRCS) ../machine/genassym.c
	@if /bin/ksh -c "test depend.data -nt makefile"; then	    \
	    echo "\tDependency data changed, updating makefile";    \
	    $(RM) -f tmp$$$$;					    \
	    /bin/mv makefile tmp$$$$;				    \
	    sed "/^# DO NOT DELETE THIS LINE/q" tmp$$$$ > makefile; \
	    $(RM) -f tmp$$$$;					    \
	    depends -m >> makefile;				    \
	fi

depend:
	@echo "Starting full dependencies make"
	/bin/make -kf makefile.depend

# DO NOT DELETE THIS LINE -- make depend uses it
