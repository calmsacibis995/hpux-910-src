# @(#) $Revision: 64.5 $       

# ************************************************************************
# THIS SCRIPT IS A WORKING MODEL ONLY !!  You must  customize  this script
# and eliminate the next two lines before executing it.
	echo "mkdev: template version -- customize script before using it"
	exit 1
# ************************************************************************

#set -x
#cd /dev 
#mknod=/etc/mknod
#chmod=/bin/chmod
#chown=/bin/chown
#chgrp=/bin/chgrp

# MISCELLANEOUS:
#dev=console;	$mknod $dev	c	0	0x000000
#
#		$chmod 622 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#
#dev=tty;	$mknod $dev	c	2	0x000000
#
#		$chmod 666 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#
#dev=mem;	$mknod $dev	c	3	0x000000
#
#		$chmod 640 $dev
#		$chown bin $dev
#		$chgrp sys $dev
#
#dev=kmem;	$mknod $dev	c	3	0x000001
#
#		$chmod 640 $dev
#		$chown bin $dev
#		$chgrp sys $dev
#
#dev=null;	$mknod $dev	c	3	0x000002
#
#		$chmod 666 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#
#dev=swap;	$mknod $dev	c	8	0x000000
#
#		$chmod 640 $dev
#		$chown bin $dev
#		$chgrp sys $dev
#dev=dsk/0s0;	$mknod $dev	b	255	0xffffff
#
#		$chmod 640 $dev
#		$chown bin $dev
#		$chgrp sys $dev
#

# TERMINALS:
#	0xScPoAc where
#		Sc = card select code
#		Po = port number (98642); 00 for other cards
#		Ac = access type
#	Access type bit fields: 0000 0DCB
#		D = 0  modem (or modem eliminator)
#		D = 1  direct connect
#		C = 0  US modems
#		C = 1  CCITT mode (for European modem support)
#		B = 0  call-in port
#		B = 1  call-out port (for cu* devices)
#
#dev=cua01;	$mknod $dev	c	1	0xSc0001
#
#		$chmod 666 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#
#dev=cul01;	$mknod $dev	c	1	0xSc0001
#
#		$chmod 666 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#
#dev=tty01;	$mknod $dev	c	1	0xSc0000
#dev=tty0x;	$mknod $dev	c	1	0xScPo04
#
#		$chmod 666 tty*
#		$chown bin tty*
#		$chgrp bin tty*
#

# DISK DEVICES
# Device names for disks are assigned sequentially with
# the names /dev/[r]dsk/Ns0 where N is incremented for
# each new device. 0s0 is the root device.
# CS/80 DISKS:
#dev=dsk/1s0;	$mknod $dev	b	0	0xScBa00 
#		$mknod r$dev	c	4	0xScBa00
# SCSI DISKS:
#dev=dsk/2s0;	$mknod $dev	b	7	0xScBa00 
#		$mknod r$dev	c	47	0xScBa00
# Amigo disks
# 9121.0
#dev=dsk/3s0;	$mknod $dev	b	2	0xScBa00
#	    	$mknod r$dev	c	11	0xScBa00
# 9121.1
#dev=dsk/4s0;	$mknod $dev	b	2	0xScBa10
#	    	$mknod r$dev	c	11	0xScBa10
#
#		$chmod 640 dsk/* rdsk/*
#		$chown bin dsk/* rdsk/*
#		$chgrp sys dsk/* rdsk/*
#

# CARTRIDGE TAPE DEVICES
# Device names for cartridge tapes are assigned sequentially
# with the names /dev/[r]ct/cN where N is incremented for
# each new device.
#dev=ct/c0;	$mknod $dev	b	0	0xScBa00
#		$mknod r$dev	c	4	0xScBa00
# Cartridge tape on a dual controller
#dev=ct/c1;	$mknod $dev	b	0	0xScBa10
#		$mknod r$dev	c	4	0xScBa10
#
#		$chmod 666 ct/* rct/*
#		$chown bin ct/* rct/*
#		$chgrp bin ct/* rct/*
#

# MAGNETIC TAPE DEVICES: 
# Device names for magnetic tapes are assigned sequentially
# with the names /dev/[r]mt/N[hml]{n} where N is incremented
# for each new device.
#	0xScBaFg where
#	      Fg = flag bit fields
#		0xC0 = for extracting density bits
#		0x30 = for extracting unit number bits
#		0x08 = don't accept block that crosses eot
#		0x04 = don't enable immediate report
#		0x02 = don't reposition on read-only closes
#		0x01 = don't rewind on close (n)
#
#		0x00 = 800 bpi;  low density (l)
#		0x40 = 1600 bpi; medium density (m)
#		0x80 = 6250 bpi; high density (h)
#
# 7970/7971 (800 BPI)
#dev=rmt/0m;	$mknod $dev	c	5	0xScBa02
#dev=rmt/0mn;	$mknod $dev	c	5	0xScBa03
# 7970/7971 (1600 BPI)
#dev=rmt/0m;	$mknod $dev	c	5	0xScBa12
#dev=rmt/0mn;	$mknod $dev	c	5	0xScBa13
# 7974 opt 800 (800 BPI)
#dev=rmt/0l;	$mknod $dev	c	9	0xScBa02
#dev=rmt/0ln;	$mknod $dev	c	9	0xScBa03
# 7974/7978 (1600 BPI)
#dev=rmt/0m;	$mknod $dev	c	9	0xScBa42
#dev=rmt/0mn;	$mknod $dev	c	9	0xScBa43
# 7978 (6250 BPI)
#dev=rmt/0h;	$mknod $dev	c	9	0xScBa82
#dev=rmt/0hn;	$mknod $dev	c	9	0xScBa83
#
#		$chmod 666  rmt/*
#		$chown bin  rmt/*
#		$chgrp bin  rmt/*
#

# PRINTERS: 
#	0xScBa0F where
#	       F = flag bit fields
#		0x1 = Raw Mode
#		0x2 = Printers without overprint capability
#		0x4 = Upper-case only printers
#		0x8 = No page eject on open and close
#
#dev=lp;	$mknod $dev	c	7	0xScBa0F
#
#		$chmod 600 $dev
#		$chown lp $dev
#		$chgrp bin $dev
#
#dev=ciper;	$mknod $dev	c	26	0xScBa0F
#
#		$chmod 600 $dev
#		$chown lp $dev
#		$chgrp bin $dev
#

# GRAPHICS DRIVERS
#dev=console;	$mknod $dev	c	12	0x000000
#
#		$chmod 622 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#
#dev=crtxx;	$mknod $dev	c	12	0xSc0200
#
#		$chmod 622 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#

# SRM
#	0xScNa00 where
#		Sc = the select code of the local interface card
#		Na = the node address of the remote SRM
#dev=srm_nodeNa;	$mknod $dev	c	13	0xScNa00
#
#		$chmod 666 srm_node*
#		$chown bin srm_node*
#		$chgrp sys srm_node*
#

# PTYMAS and PTYSLV
# 	pattern:
#		ptym/pty[p-z][0-f]	master pseudo terminals
#		pty/tty[p-z][0-f]	slave pseudo terminals
#
#dev=ptym/ptyp0;	$mknod $dev	c	16	0xSc0000
#dev=pty/ttyp0;		$mknod $dev	c	17	0xSc0000
#dev=ptym/ptyp1;	$mknod $dev	c	16	0xSc0001
#dev=pty/ttyp1;		$mknod $dev	c	17	0xSc0001
#
#		$chmod 666 ptym/* pty/*
#		$chown bin ptym/* pty/*
#		$chgrp bin ptym/* pty/*
#

# IEEE802 and ETHERNET
#dev=lan;	$mknod $dev	c	18	0xSc0000
#dev=ieee;	$mknod $dev	c	18	0xSc0000
#dev=ether;	$mknod $dev	c	19	0xSc0000
#
#		$chmod 640 lan ieee ether
#		$chown bin lan ieee ether
#		$chgrp sys lan ieee ether
#

# PLOTTERS / DIGITIZERS / PRINTERS (RAW MODE) / HP-IB(4):
#dev=hpib-device;  $mknod $dev	c	21	0xScBa00
#dev=hpib-channel; $mknod $dev	c	21	0xSc1F00
#
#		$chmod 666 hpib-*
#		$chown bin hpib-*
#		$chgrp bin hpib-*
#

# GPIO
#dev=gpio;	$mknod $dev	c	22	0xSc0000
#
#		$chmod 666 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#

# KEYBOARDS / HIL DEVICES
#dev=raw_8042;	$mknod $dev	c	23	0x000000
#
#		$chmod 666 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#
#dev=mousen;	$mknod $dev	c	24	0x0000n0
#
#		$chmod 666 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#
#dev=nimitzn;	$mknod $dev	c	25	0x0000n0
#
#		$chmod 666 $dev
#		$chown bin $dev
#		$chgrp bin $dev
#

# AUTOCHANGERS:
#	0xScA0Sr where
#		Sc = card select code the mechanical autochanger
#		A  = SCSI address of the mechanical autochanger
#		Sr = surface number

#if [ ! -d ac ]		# directory for block devices
#then
#	mkdir ac
#	chmod 755 ac
#fi
#if [ ! -d rac ]		# directory for character (raw) devices
#then
#	mkdir rac
#	chmod 755 rac
#fi

#Sc=0e	# Select code of the mechanical autochanger
#A=3	# SCSI address of the mechanical autochanger
#dev=rac/ioctl;	$mknod $dev	c	55	0x${Sc}${A}000

#dev=ac/1a;	$mknod $dev	b	10	0x${Sc}${A}001
#dev=ac/1b;	$mknod $dev	b	10	0x${Sc}${A}002
#dev=ac/2a;	$mknod $dev	b	10	0x${Sc}${A}003
#dev=ac/2b;	$mknod $dev	b	10	0x${Sc}${A}004
#dev=ac/3a;	$mknod $dev	b	10	0x${Sc}${A}005
#dev=ac/3b;	$mknod $dev	b	10	0x${Sc}${A}006
#dev=ac/4a;	$mknod $dev	b	10	0x${Sc}${A}007
#dev=ac/4b;	$mknod $dev	b	10	0x${Sc}${A}008
#dev=ac/5a;	$mknod $dev	b	10	0x${Sc}${A}009
#dev=ac/5b;	$mknod $dev	b	10	0x${Sc}${A}00a
#dev=ac/6a;	$mknod $dev	b	10	0x${Sc}${A}00b
#dev=ac/6b;	$mknod $dev	b	10	0x${Sc}${A}00c
#dev=ac/7a;	$mknod $dev	b	10	0x${Sc}${A}00d
#dev=ac/7b;	$mknod $dev	b	10	0x${Sc}${A}00e
#dev=ac/8a;	$mknod $dev	b	10	0x${Sc}${A}00f
#dev=ac/8b;	$mknod $dev	b	10	0x${Sc}${A}010
#dev=ac/9a;	$mknod $dev	b	10	0x${Sc}${A}011
#dev=ac/9b;	$mknod $dev	b	10	0x${Sc}${A}012
#dev=ac/10a;	$mknod $dev	b	10	0x${Sc}${A}013
#dev=ac/10b;	$mknod $dev	b	10	0x${Sc}${A}014
#dev=ac/11a;	$mknod $dev	b	10	0x${Sc}${A}015
#dev=ac/11b;	$mknod $dev	b	10	0x${Sc}${A}016
#dev=ac/12a;	$mknod $dev	b	10	0x${Sc}${A}017
#dev=ac/12b;	$mknod $dev	b	10	0x${Sc}${A}018
#dev=ac/13a;	$mknod $dev	b	10	0x${Sc}${A}019
#dev=ac/13b;	$mknod $dev	b	10	0x${Sc}${A}01a
#dev=ac/14a;	$mknod $dev	b	10	0x${Sc}${A}01b
#dev=ac/14b;	$mknod $dev	b	10	0x${Sc}${A}01c
#dev=ac/15a;	$mknod $dev	b	10	0x${Sc}${A}01d
#dev=ac/15b;	$mknod $dev	b	10	0x${Sc}${A}01e
#dev=ac/16a;	$mknod $dev	b	10	0x${Sc}${A}01f
#dev=ac/16b;	$mknod $dev	b	10	0x${Sc}${A}020
#dev=ac/17a;	$mknod $dev	b	10	0x${Sc}${A}021
#dev=ac/17b;	$mknod $dev	b	10	0x${Sc}${A}022
#dev=ac/18a;	$mknod $dev	b	10	0x${Sc}${A}023
#dev=ac/18b;	$mknod $dev	b	10	0x${Sc}${A}024
#dev=ac/19a;	$mknod $dev	b	10	0x${Sc}${A}025
#dev=ac/19b;	$mknod $dev	b	10	0x${Sc}${A}026
#dev=ac/20a;	$mknod $dev	b	10	0x${Sc}${A}027
#dev=ac/20b;	$mknod $dev	b	10	0x${Sc}${A}028
#dev=ac/21a;	$mknod $dev	b	10	0x${Sc}${A}029
#dev=ac/21b;	$mknod $dev	b	10	0x${Sc}${A}02a
#dev=ac/22a;	$mknod $dev	b	10	0x${Sc}${A}02b
#dev=ac/22b;	$mknod $dev	b	10	0x${Sc}${A}02c
#dev=ac/23a;	$mknod $dev	b	10	0x${Sc}${A}02d
#dev=ac/23b;	$mknod $dev	b	10	0x${Sc}${A}02e
#dev=ac/24a;	$mknod $dev	b	10	0x${Sc}${A}02f
#dev=ac/24b;	$mknod $dev	b	10	0x${Sc}${A}030
#dev=ac/25a;	$mknod $dev	b	10	0x${Sc}${A}031
#dev=ac/25b;	$mknod $dev	b	10	0x${Sc}${A}032
#dev=ac/26a;	$mknod $dev	b	10	0x${Sc}${A}033
#dev=ac/26b;	$mknod $dev	b	10	0x${Sc}${A}034
#dev=ac/27a;	$mknod $dev	b	10	0x${Sc}${A}035
#dev=ac/27b;	$mknod $dev	b	10	0x${Sc}${A}036
#dev=ac/28a;	$mknod $dev	b	10	0x${Sc}${A}037
#dev=ac/28b;	$mknod $dev	b	10	0x${Sc}${A}038
#dev=ac/29a;	$mknod $dev	b	10	0x${Sc}${A}039
#dev=ac/29b;	$mknod $dev	b	10	0x${Sc}${A}03a
#dev=ac/30a;	$mknod $dev	b	10	0x${Sc}${A}03b
#dev=ac/30b;	$mknod $dev	b	10	0x${Sc}${A}03c
#dev=ac/31a;	$mknod $dev	b	10	0x${Sc}${A}03d
#dev=ac/31b;	$mknod $dev	b	10	0x${Sc}${A}03e
#dev=ac/32a;	$mknod $dev	b	10	0x${Sc}${A}03f
#dev=ac/32b;	$mknod $dev	b	10	0x${Sc}${A}040
#
#dev=rac/1a;	$mknod $dev	c	55	0x${Sc}${A}001
#dev=rac/1b;	$mknod $dev	c	55	0x${Sc}${A}002
#dev=rac/2a;	$mknod $dev	c	55	0x${Sc}${A}003
#dev=rac/2b;	$mknod $dev	c	55	0x${Sc}${A}004
#dev=rac/3a;	$mknod $dev	c	55	0x${Sc}${A}005
#dev=rac/3b;	$mknod $dev	c	55	0x${Sc}${A}006
#dev=rac/4a;	$mknod $dev	c	55	0x${Sc}${A}007
#dev=rac/4b;	$mknod $dev	c	55	0x${Sc}${A}008
#dev=rac/5a;	$mknod $dev	c	55	0x${Sc}${A}009
#dev=rac/5b;	$mknod $dev	c	55	0x${Sc}${A}00a
#dev=rac/6a;	$mknod $dev	c	55	0x${Sc}${A}00b
#dev=rac/6b;	$mknod $dev	c	55	0x${Sc}${A}00c
#dev=rac/7a;	$mknod $dev	c	55	0x${Sc}${A}00d
#dev=rac/7b;	$mknod $dev	c	55	0x${Sc}${A}00e
#dev=rac/8a;	$mknod $dev	c	55	0x${Sc}${A}00f
#dev=rac/8b;	$mknod $dev	c	55	0x${Sc}${A}010
#dev=rac/9a;	$mknod $dev	c	55	0x${Sc}${A}011
#dev=rac/9b;	$mknod $dev	c	55	0x${Sc}${A}012
#dev=rac/10a;	$mknod $dev	c	55	0x${Sc}${A}013
#dev=rac/10b;	$mknod $dev	c	55	0x${Sc}${A}014
#dev=rac/11a;	$mknod $dev	c	55	0x${Sc}${A}015
#dev=rac/11b;	$mknod $dev	c	55	0x${Sc}${A}016
#dev=rac/12a;	$mknod $dev	c	55	0x${Sc}${A}017
#dev=rac/12b;	$mknod $dev	c	55	0x${Sc}${A}018
#dev=rac/13a;	$mknod $dev	c	55	0x${Sc}${A}019
#dev=rac/13b;	$mknod $dev	c	55	0x${Sc}${A}01a
#dev=rac/14a;	$mknod $dev	c	55	0x${Sc}${A}01b
#dev=rac/14b;	$mknod $dev	c	55	0x${Sc}${A}01c
#dev=rac/15a;	$mknod $dev	c	55	0x${Sc}${A}01d
#dev=rac/15b;	$mknod $dev	c	55	0x${Sc}${A}01e
#dev=rac/16a;	$mknod $dev	c	55	0x${Sc}${A}01f
#dev=rac/16b;	$mknod $dev	c	55	0x${Sc}${A}020
#dev=rac/17a;	$mknod $dev	c	55	0x${Sc}${A}021
#dev=rac/17b;	$mknod $dev	c	55	0x${Sc}${A}022
#dev=rac/18a;	$mknod $dev	c	55	0x${Sc}${A}023
#dev=rac/18b;	$mknod $dev	c	55	0x${Sc}${A}024
#dev=rac/19a;	$mknod $dev	c	55	0x${Sc}${A}025
#dev=rac/19b;	$mknod $dev	c	55	0x${Sc}${A}026
#dev=rac/20a;	$mknod $dev	c	55	0x${Sc}${A}027
#dev=rac/20b;	$mknod $dev	c	55	0x${Sc}${A}028
#dev=rac/21a;	$mknod $dev	c	55	0x${Sc}${A}029
#dev=rac/21b;	$mknod $dev	c	55	0x${Sc}${A}02a
#dev=rac/22a;	$mknod $dev	c	55	0x${Sc}${A}02b
#dev=rac/22b;	$mknod $dev	c	55	0x${Sc}${A}02c
#dev=rac/23a;	$mknod $dev	c	55	0x${Sc}${A}02d
#dev=rac/23b;	$mknod $dev	c	55	0x${Sc}${A}02e
#dev=rac/24a;	$mknod $dev	c	55	0x${Sc}${A}02f
#dev=rac/24b;	$mknod $dev	c	55	0x${Sc}${A}030
#dev=rac/25a;	$mknod $dev	c	55	0x${Sc}${A}031
#dev=rac/25b;	$mknod $dev	c	55	0x${Sc}${A}032
#dev=rac/26a;	$mknod $dev	c	55	0x${Sc}${A}033
#dev=rac/26b;	$mknod $dev	c	55	0x${Sc}${A}034
#dev=rac/27a;	$mknod $dev	c	55	0x${Sc}${A}035
#dev=rac/27b;	$mknod $dev	c	55	0x${Sc}${A}036
#dev=rac/28a;	$mknod $dev	c	55	0x${Sc}${A}037
#dev=rac/28b;	$mknod $dev	c	55	0x${Sc}${A}038
#dev=rac/29a;	$mknod $dev	c	55	0x${Sc}${A}039
#dev=rac/29b;	$mknod $dev	c	55	0x${Sc}${A}03a
#dev=rac/30a;	$mknod $dev	c	55	0x${Sc}${A}03b
#dev=rac/30b;	$mknod $dev	c	55	0x${Sc}${A}03c
#dev=rac/31a;	$mknod $dev	c	55	0x${Sc}${A}03d
#dev=rac/31b;	$mknod $dev	c	55	0x${Sc}${A}03e
#dev=rac/32a;	$mknod $dev	c	55	0x${Sc}${A}03f
#dev=rac/32b;	$mknod $dev	c	55	0x${Sc}${A}040
#
#		$chmod 640 ac/* rac/*
#		$chown bin ac/* rac/*
#		$chgrp sys ac/* rac/*
#

