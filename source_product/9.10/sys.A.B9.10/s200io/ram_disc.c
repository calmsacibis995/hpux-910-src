/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/ram_disc.c,v $
 * $Revision: 1.4.84.5 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/10/11 08:02:24 $
 */
/* HPUX_ID: @(#)ram_disc.c	55.1     88/12/23  */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304

*/


/******************************************************************************/
/* This driver allows you to create up to 16 "ram_disc" volumes, doing a      */
/* "mkfs" on them and then "mount"ing them as a file system.  Be careful to   */
/* not use up too much ram on the "disc".  You still must have some left for  */
/* running normal processes.						      */
/*                                   - by -                                   */
/*                          System Software Operation                         */
/*                            Fort Collins, Co 80526                          */
/*			           Oct 14, 1986                               */
/* Note:                                                                      */
/*    There is a bug in 5.2 and earlier systems.  The "special" dev is left   */
/*    open if there is an error during a "mount" command.  This will make it  */
/*    impossible to deallocate a disc volume if a "mount" error occurs.  So   */
/*    be carefull to do a "mkfs" on the disc volume before trying to mount it.*/
/*                                                                            */
/* Revision History:                                                          */
/*      11-21-86 added the status request (dlb)                               */
/*      12-09-86 changed the ramfree request (dlb)                            */
/*                                                                            */
/******************************************************************************/
/* 
******   STEPS TO ADD THE RAM_DISC DRIVER TO YOUR KERNEL
*

STEP 1) Login as "root"
	# cd /etc/conf

STEP 2)  make a mod to the "/etc/master" file as follows:
 
* HPUX_ID: @(#)master	10.3     85/11/14 
*
* The following devices are those that can be specified in the system
* description file.  The name specified must agree with the name shown,
* or with an alias.
*
* name		handle		type	mask	block	char
*
cs80		cs80		3	3FB	0	4
.
.
.
ramdisc		ram		3	FB	4	20
.
.
Note: Major number 4 for block device and 20 for char (raw) device may
	need to be different on your system.  Reflect these different 
	numbers in the "mknod" command below. 

STEP 3)  modify the "/etc/conf/dfile...your_favorite" with the addition of 
	"ramdisc"

STEP 4) # ar -rv libmin.a ram_disc.o

STEP 5) # config dfile...your_favorite

STEP 6) # make -f config.mk

STEP 7) # mv /hp-ux /SYSBCKUP
        # mv ./hp-ux /hp-ux

STEP 8) # reboot
	and login as "root"

STEP 9) # /etc/mknod /dev/ram  b  4 0xVSSSSS      	(block device)
        # /etc/mknod /dev/rram c 20 0xVSSSSS     	(char device)
		Where V = volume number 0 - F    	(0 - 15)
		Where SSSSS = number of 256 byte sectors in volume (in hex).
		However, if (SSSSS & 1) then the interpretation of the
		entire minor number is 0xVSSSSF, where:
		V = volume number 0 - F
		SSSS = number of 64k byte chunks in volume.
		F contains mode bits as follows:
		If (F & 0x1) then these new semantics apply, else old.
		If (F & 0x2) then the B_NOCACHE bit will be set in bp->b_flags,
		       thus eliminating duplication of data in the buffer
		       cache, and so reducing memory waste.
		If (F & 0x4) then kernel pageable memory will be allocated
		       instead of physical ram. (Using kvalloc)
		Naturally, these two attributes can be combined to get
		an uncached, virtual ram disk. (rdg)

	I.E.
        # /etc/mknod /dev/ram128K   b  4 0x000200	(block 128Kb ram volume)
        # /etc/mknod /dev/rram128K  c 20 0x000200	(char 128Kb ram volume)

        # /etc/mknod /dev/ram1M     b  4 0x101000	(block 1Mb ram volume)
        # /etc/mknod /dev/rram1M    c 20 0x101000	(char  1Mb ram volume)

        # /etc/mknod /dev/ram2M     b  4 0x202000	(block 2Mb ram volume)
        # /etc/mknod /dev/rram2M    c 20 0x202000	(char  2Mb ram volume)

        # /etc/mknod /dev/ram4M     b  4 0x404000	(block 4Mb ram volume)
        # /etc/mknod /dev/rram4M    c 20 0x404000	(char  4Mb ram volume)

        # /etc/mknod /dev/ramAM     b  4 0xA0A000	(block 10Mb ram volume)
        # /etc/mknod /dev/rramAM    c 20 0xA0A000	(char  10Mb ram volume)
		(Note: I don't know if this works yet - don't have this much mem)
	# 1 meg virtual ram disk:
	# /etc/mknod /dev/vram1meg c 20  0x300105
	# 64 meg virtual uncached ram disk:
	# /etc/mknod /dev/vram64meg c 20 0x304007
	# 1 meg physical uncached ram disk:
	# /etc/mknod /dev/ram1meg c 20 0x301002   # old semantics
	# OR
	# /etc/mknod /dev/ram1meg c 20 0x300103   # new semantics


STEP 10)# mkfs /dev/ram128K 128 8 8 8192 1024 32 0 60 8192 (mkfs for 128Kb volume)
        # mkfs /dev/ram1M   1024          (make file system for 1Mb volume)
        # mkfs /dev/ram2M   2048          (make file system for 2Mb volume)
        # mkfs /dev/ram4M   4096          (make file system for 4Mb volume)

STEP 11)# mkdir /ram128K	
        # mount /dev/ram128K /ram128K			(mount 128K ram volume)

        # mkdir /ram1M
        # mount /dev/ram1M /ram1M			(mount 1Mb ram volume)

        # mkdir /ram2M
        # mount /dev/ram2M /ram2M			(mount 2Mb ram volume)

        # mkdir /ram4M
        # mount /dev/ram4M /ram4M			(mount 4Mb ram volume)

STEP 12) To unmount volume 
        # umount /dev/ram1M

 To make the control /dev for "ramstat".
	# /etc/mknod /dev/ram	    c 20 0x0            (status is raw dev only)

 To release memory of disc #1 (and destroying all files on volume)
        # ramstat -d 1 /dev/ram  
		-or-  if you use the above /dev/ram convention.
	# ramstat -d 1

 To get a status of all  memory volumes
	# ramstat /dev/ram
		-or-
	# ramstat

 To reset the access counters of a memory volume # 1.
	# ramstat -r 1 /dev/ram
		-or-
	# ramstat -r1
*****************************************************************************/

#ifdef _KERNEL
#include "../h/param.h"
#include "../h/errno.h"
#include "../h/buf.h"
#include "../h/pregion.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/vmmac.h"
#include "../s200io/ram.h"
#else
#include <sys/param.h>
#include <sys/errno.h>
unsigned minphys();	/* XXX needed only with user version of buf.h */
#include <sys/buf.h>
#include <sys/pregion.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/vmmac.h>
#include "ram.h"
#endif

extern dev_t rootdev;
int (*ram_saved_dev_init)();

/*
 * The following parameters are initialized in locore.s. They are set
 * to zero by default, and they will be reset by init_kdb() (in locore)
 * if the secondary loader indicates that it has set up a bootable ram
 * fs image.
 */

extern unsigned long ramfsbegin;
extern unsigned long ramfssize;

/*
**  one-time initialization code
*/
ram_init()
{
    register struct ram_descriptor *ram_des_ptr;
    register preg_t *prp;
    register int maj;
    char *ramdiskaddr;
    extern struct bdevsw bdevsw[];
    extern int ram_open();
    extern preg_t *io_map_c();


    /*
    **  set rootdev if ramfssize != 0
    */

    /*
    **  Is RAM fs image present?
    */

    if (ramfssize != 0) {
	/*
	**  determine the ram blocked driver major number
	*/
	for (maj = 0; bdevsw[maj].d_open != ram_open; )
		if (++maj >= nblkdev)
			panic("ram_init: ram_open not in bdevsw");

	prp = io_map_c((space_t)&kernvas, NULL, ramfsbegin, btorp(ramfssize),
		       PROT_URW,PHDL_CACHE_CB);

	if (prp == (preg_t *)0)
	    panic("ram_init: Could not map memory");

	ramdiskaddr = prp->p_vaddr;

#ifndef QUIETBOOT
	printf("RAM fs image size = %d\n",ramfssize);
#endif /* ! QUIETBOOT */

	ram_des_ptr = &ram_device[RAM_MAXVOLS - 1];
	ram_des_ptr->flag = RAM_BOOT;
	ram_des_ptr->addr = ramdiskaddr;
	ram_des_ptr->size = ramfssize / 256;
	ram_des_ptr->opencount = 0;

	/* Should this be the root device? */

	rootdev = makedev(maj,(((RAM_MAXVOLS - 1) & 0xf) << 20) | 0x000010);
    }

    /*
    **  call the next init procedure in the chain
    */

    (*ram_saved_dev_init)();
    return;
}

/*
**  one-time linking code
*/
ram_link()
{
	extern int (*dev_init)();

	ram_saved_dev_init = dev_init;
	dev_init = ram_init;
}

/* 
**  Open the ram device.
*/
ram_open(dev, flag)
dev_t dev;
int flag;
{
	register unsigned long size;
	register struct ram_descriptor *ram_des_ptr;

	/* check if status open */
	if (RAM_MINOR(dev) == 0)
		return(0);

	/* check if greater than max number of volumes */
	if ((size = RAM_DISC(dev)) > RAM_MAXVOLS)
		return(EINVAL);

	ram_des_ptr = &ram_device[size];

	/* check the size of the ram disc less than 16 sectors */
	if ((size = RAM_SIZE(dev)) < 16)
		return(EINVAL);

	/* check if already allocated */
	if (ram_des_ptr->addr != NULL) {

		/* then check if size changed */
		if (  ram_des_ptr->size != size
		  && (ram_des_ptr->flag & RAM_BOOT) == 0)
			return(EINVAL);

		/* bump open count */
		ram_des_ptr->opencount++;
	} else { 
	    if (RAM_VIRTUAL(dev)) 
	    {
		if ((ram_des_ptr->addr = 
			(char *)kvalloc(btorp(size<<LOG2SECSIZE))) == NULL) 
			return(ENOMEM);
	    }
	    else 
	    {
		/* Check to see if the request is too large */
		if (clockmemreserve(btorp(size<<LOG2SECSIZE)) == 0)
			return(ENOMEM);

		/* allocate the memory for the ram disc */
		if ((ram_des_ptr->addr = 
			(char *)sys_memall(size<<LOG2SECSIZE)) == NULL) {
			lockmemunreserve(btorp(size<<LOG2SECSIZE));
			return(ENOMEM);
		}

		/*
		 * The call to clockmemreserve allocated lockable memory ONLY
 	    	 * to see if there was enough available (i.e. keep this driver 
		 * from sucking up memory we ain't got.  The call to sys_memall
                 * also gets us lockable memory because we steal from that pool
		 * for P_SYS pages.  So, we have allocated the lockable  memory
 		 * twice.  So, we need to give one set back.
		 */

		lockmemunreserve(btorp(size<<LOG2SECSIZE));
	    }

	    /* save size in 256 byte "sectors" */
	    ram_des_ptr->size = size;

	    /* open count should be zero */
	    if (ram_des_ptr->opencount++) {
		panic("ram_open count wrong\n");
	    }
	    ram_des_ptr->flag = 0;
	}
	return(0);
}

ram_close(dev)
dev_t dev;
{
	register struct ram_descriptor *ram_des_ptr;
	register i;

	/* check if status open */
	if (RAM_MINOR(dev) != 0) {
		ram_des_ptr = &ram_device[RAM_DISC(dev)];
	
		if (--ram_des_ptr->opencount < 0) 
			panic("ram_close count less than zero\n");
	}
/* 
   NOTE: 5.2 9000/300 and earlier systems may have a bug that the memory 
   cannot be released if there was ever a "mount" error because the open 
   count will never reach zero -- so be carefull to do a "mkfs" before
   a "mount".
*/
	/* free all ram volumes with flag set and open count = 0 */
	ram_des_ptr = &ram_device[0];
	for (i = 0; i < RAM_MAXVOLS; i++, ram_des_ptr++) {
		if ((ram_des_ptr->flag & RAM_RETURN) == 0) 
			continue;
		if (ram_des_ptr->opencount != 0) 
			continue;

		/* release the system memory */
		if (RAM_VIRTUAL(dev)) 
		    kvfree(ram_des_ptr->addr,
			   btorp(ram_des_ptr->size<<LOG2SECSIZE));
		else
		    sys_memfree(ram_des_ptr->addr, ram_des_ptr->size<<LOG2SECSIZE);

		/* zero the whole entry */
		bzero((char *)ram_des_ptr, sizeof(struct ram_descriptor));
	}
}

ram_strategy(bp)
register struct buf *bp;
{
	register block_d7;
	register char *addr;
	register struct ram_descriptor *ram_des_ptr;
	
	/* check if status request, return the ram_device structure */
	if (RAM_MINOR(bp->b_dev) == 0) { 
		if ((bp->b_flags & B_PHYS) && /* must be char (raw) device */
			(bp->b_flags & B_READ) &&
			(bp->b_bcount == sizeof(ram_device))) {
 			bp->b_resid = bp->b_bcount; /*normally done by bpcheck*/

			/* return the "ram_device" structure to the caller */
			bcopy(&ram_device[0], bp->b_un.b_addr, sizeof(ram_device)); 
		} else {
			bp->b_error = EIO;
			bp->b_flags = B_ERROR;
		}
		goto done;
	}
	/* do the normal reads and writes to ram disc */
	ram_des_ptr = &ram_device[RAM_DISC(bp->b_dev)];

	/* sanity check if we got the memory */
	if ((addr = ram_des_ptr->addr) == NULL) {
		panic("no memory in ram_strategy\n");
	}
	/* make sure the request is within the domain of the "disc" */
	if (bpcheck(bp, ram_des_ptr->size, LOG2SECSIZE, 0)) 
		return;

	/* calculate address to do the transfer */
	addr += bp->b_un2.b_sectno<<LOG2SECSIZE;

	/* for debugging file system only */
	block_d7 = bp->b_un2.b_sectno>>2;

	if (bp->b_flags & B_READ) {
		pbcopy(addr, bp->b_un.b_addr, bp->b_bcount);
		switch (bp->b_bcount/1024) {
		case 1: ram_des_ptr->rd1k++;
			break;
		case 2: ram_des_ptr->rd2k++;
			break;
		case 3: ram_des_ptr->rd3k++;
			break;
		case 4: ram_des_ptr->rd4k++;
			break;
		case 5: ram_des_ptr->rd5k++;
			break;
		case 6: ram_des_ptr->rd6k++;
			break;
		case 7: ram_des_ptr->rd7k++;
			break;
		case 8: ram_des_ptr->rd8k++;
			break;
		default: ram_des_ptr->rdother++;
		}
	} else { /* WRITE */
		pbcopy(bp->b_un.b_addr, addr, bp->b_bcount);
		switch (bp->b_bcount/1024) {
		case 1: ram_des_ptr->wt1k++;
			break;
		case 2: ram_des_ptr->wt2k++;
			break;
		case 3: ram_des_ptr->wt3k++;
			break;
		case 4: ram_des_ptr->wt4k++;
			break;
		case 5: ram_des_ptr->wt5k++;
			break;
		case 6: ram_des_ptr->wt6k++;
			break;
		case 7: ram_des_ptr->wt7k++;
			break;
		case 8: ram_des_ptr->wt8k++;
			break;
		default: ram_des_ptr->wtother++;
		}
	}
done:
 	bp->b_resid -= bp->b_bcount;
	if (RAM_NOCACHE(bp->b_dev))
	    bp->b_flags |= B_NOCACHE;
	
	biodone(bp);
}

/* this routine is put in here because I want it to be in the profiles */
/* bcopy could just as well be used if profiling is not used */

asm("	global	_pbcopy			# physio enforces word alignmemt! ");
asm("_pbcopy:				# 0 thru 256 Kbytes!!!		");
asm("	movm.l	4(%sp),%d0/%a0-%a1	# d0 = src; a0 = dst; a1 = cnt	");
asm("	exg	%d0,%a1			# d0 = cnt; a1 = src		");
asm("	subq.l	&1,%d0			# make a counter		");
asm("	blt	Llpcopy4		# less or = zero?		");
asm("	ror.l	&2,%d0							");
asm("	bra	Llpcopy2		# move 4 bytes at a time	");
asm("Llpcopy1:								");
asm("	mov.l	(%a1)+,(%a0)+		# move large block		");
asm("Llpcopy2:								");
asm("	dbra	%d0,Llpcopy1						");
asm("	swap	%d0			# get remaining bytes		");
asm("	rol.w	&2,%d0			# position to low bits		");
asm("Llpcopy3:								");
asm("	mov.b	(%a1)+,(%a0)+		# 1 to 4 bytes last bytes	");
asm("	dbra	%d0,Llpcopy3						");
asm("Llpcopy4:								");
asm("	rts								");

ram_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	return physio(ram_strategy, NULL, dev, B_READ, minphys, uio);
}

ram_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	return physio(ram_strategy, NULL, dev, B_WRITE, minphys, uio);
}

ram_ioctl(dev, cmd, addr, flag)
dev_t dev;
int cmd;
caddr_t addr;
int flag;
{
	register struct ram_descriptor *ram_des_ptr;
	register volume;

	/* check if dev is the status dev */
	if (RAM_MINOR(dev) != 0) 
		return(EIO);

	/* check if 0 - 15 disc volume */
	volume = *(int *)addr;
	if ((volume % RAM_MAXVOLS) != volume)
		return(EIO);

	/* calculate which ram volume it is */
	ram_des_ptr = &ram_device[volume]; 

	/* if not allocated, then return error */
	if (ram_des_ptr->addr == NULL) { 
		return(ENOMEM);
	} 
	switch(cmd) {

	/* mark for memory release on last close */ 
	case RAM_DEALLOCATE: 

		/* Don't allow RAM_RETURN to be set if RAM_BOOT is set */

		if ((ram_des_ptr->flag & RAM_BOOT) != 0)
		    return(EIO);

		ram_des_ptr->flag |= RAM_RETURN;
		break;

	/* clear out access counts */
	case RAM_RESETCOUNTS: 
		ram_des_ptr->rd8k = 	0;
		ram_des_ptr->rd7k = 	0;
		ram_des_ptr->rd6k = 	0;
		ram_des_ptr->rd5k = 	0;
		ram_des_ptr->rd4k = 	0;
		ram_des_ptr->rd3k = 	0;
		ram_des_ptr->rd2k = 	0;
		ram_des_ptr->rd1k = 	0;
		ram_des_ptr->rdother = 	0;
		ram_des_ptr->wt8k = 	0;
		ram_des_ptr->wt7k = 	0;
		ram_des_ptr->wt6k = 	0;
		ram_des_ptr->wt5k = 	0;
		ram_des_ptr->wt4k = 	0;
		ram_des_ptr->wt3k = 	0;
		ram_des_ptr->wt2k = 	0;
		ram_des_ptr->wt1k = 	0;
		ram_des_ptr->wtother = 	0;
		break;
	default:
		return(EIO);
	}
	return(0);
}




