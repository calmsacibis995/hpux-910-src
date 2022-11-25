/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/drivers/RCS/scsi_ccs.c,v $
 * $Revision: 1.2 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/07/14 13:17:24 $
 */

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


/*
 *	SCSI Common Command Set
 *	   Contains Routines Used by Drivers
 */

#include <param.h>
#include <buf.h>
#include <timeout.h>
#include <hpibio.h>
#include <dir.h>
#include <_types.h>
#include <user.h>
#include <systm.h>
#include <proc.h>
#include <conf.h>
#include <file.h>
#include <dma.h>
#include <intrpt.h>
#include <iobuf.h>
#include <tryrec.h>
#include <bootrom.h>
#include <scsi.h>
#include <autox.h>
#include <_scsi.h>

/*
 * six_byte_cmd and ten_byte_cmd rules:
 *   the lba (logical block address) and length
 *   must be precise upon entry to these routines
 */
six_byte_cmd(bp, cmd, LUN, addr, len, ctl)
register struct buf *bp;
{
	struct {
		char	opcode;
		char	lba_msb;
		char	lba_mid;
		char	lba_lsb;
		char	length;
		char	control;
	} cdb;	/* command descriptor block */
	cdb.opcode  = cmd;
		/* 6 byte commands allow only 21 bits of addressing */
	if (addr & ~0x1FFFFF)	{
		msg_printf("SCSI 6_byte_cmd Error: address out of bounds\n");
		return (1);
		}
	cdb.lba_msb = (LUN<<5) | (addr>>16);  /* Ugh */
	cdb.lba_mid = 0xFF & addr>>8;
	cdb.lba_lsb = 0xFF & addr;
	if (len & ~0xFF)	{
		msg_printf("SCSI 6_byte_cmd Error: length too large\n");
		return (1);
		}
	cdb.length  = len;
	cdb.control = ctl;

	scsi_if_transfer(bp, &cdb, 6, B_WRITE, MUST_FHS);
}

ten_byte_cmd(bp, cmd, LUN, addr, length, ctl)
register struct buf *bp;
{
	struct {
		unsigned char	opcode;
		unsigned char	lun;
		unsigned long	lba;
		char		resv;
		unsigned char	len_msb;  /* alignment problems */
		unsigned char	len_lsb;
		unsigned char	control;
	} cdb; /* command descriptor block */

	cdb.opcode  = cmd;
	cdb.lun     = (int) (LUN<<5);
	cdb.lba     = addr;
	cdb.resv    = 0;
	cdb.len_msb = (unsigned char) (length>>8);
	cdb.len_lsb = (unsigned char) (length&0xFF);
	cdb.control = ctl;

	scsi_if_transfer(bp, &cdb, 10, B_WRITE, MUST_FHS);
}

scsi_request_sense(bp)
register struct buf *bp;
{
	SCSI_TRACE(CMDS)("scsi_request_sense\n");

     	bp->b_flags |= B_READ;	/* transfer direction,
				 * Variable length data
				 */
	/* data byte count is variable (vendor dependent) */
	bp->b_resid = bp->b_bcount = sizeof(struct xsense);  
	bp->b_queue->b_xcount = bp->b_bcount;

	six_byte_cmd(bp,CMDrequest_sense,m_unit(bp->b_dev),0,bp->b_bcount,0);
}

/*
 * Request Capacity of logical unit
 * Returns: logical block address and block length (in bytes)
 *          of last logical block of the logical unit
 */
scsi_read_capacity(bp)
register struct buf *bp;
{
	SCSI_TRACE(CMDS)("scsi_read_capacity\n");

     	bp->b_flags |= B_READ;	/* transfer direction */
	bp->b_resid = bp->b_bcount = SZ_CMDread_capacity;
	bp->b_queue->b_xcount = bp->b_bcount;

	ten_byte_cmd(bp, CMDread_capacity, m_unit(bp->b_dev), 0, 0, 0);
}

/*
 * READ  extended (10-byte) CDB format
 * WRITE extended (10-byte) CDB format
 * BTW: SCSI (DAD) allows logical sector addressing only!
 */
scsi_xfer_cmd(bp)
register struct buf *bp;
{
	register int blk_cnt;

	/* Round block count up to next whole sector - if necessary */
	blk_cnt = (bp->b_bcount + ((1<<bp->b_log2blk)-1)) >> bp->b_log2blk;
if (SCSI_DEBUG&CMDS)	{
     	if (bp->b_flags&B_READ)
		msg_printf("scsi_read  ");
	else
		msg_printf("scsi_write ");
	msg_printf("busaddr %x unit %x blk offset %x count (in blks)%x\n",
			bp->b_ba, m_unit(bp->b_dev), bp->b_un2.b_sectno,
			blk_cnt);
	}

	ten_byte_cmd(bp,
		bp->b_flags & B_READ ? CMDread_ext : CMDwrite_ext,
		m_unit(bp->b_dev), bp->b_un2.b_sectno,
		blk_cnt, 0);
}


/*
 * READ / WRITE short form (6-byte) CDB format
 * Not desirable: implemented for devices that do not support
 *	read_extended or write_extended.
 */
scsi_short_xfer_cmd(bp)
register struct buf *bp;
{
	register int blk_cnt;

	/* Round block count up to next whole sector - if necessary */
	blk_cnt = (bp->b_bcount + ((1<<bp->b_log2blk)-1)) >> bp->b_log2blk;
if (SCSI_DEBUG&CMDS)	{
     	if (bp->b_flags&B_READ)
		msg_printf("scsi_short_read  ");
	else
		msg_printf("scsi_short_write ");
	msg_printf("busaddr %x unit %x blk offset %x count (in blks)%x\n",
			bp->b_ba, m_unit(bp->b_dev), bp->b_un2.b_sectno,
			blk_cnt);
	}

	six_byte_cmd(bp,
		bp->b_flags & B_READ ? CMDread : CMDwrite, m_unit(bp->b_dev), 
		bp->b_un2.b_sectno, blk_cnt, 0);
}


scsi_test_unit(bp)
register struct buf *bp;
{
	SCSI_TRACE(CMDS)("scsi_test_unit\n");

	bp->b_resid = bp->b_bcount = 0;		/* no data phase */

	six_byte_cmd(bp, CMDtest_unit_ready, m_unit(bp->b_dev), 0, 0, 0);
}

scsi_format_unit(bp)
register struct buf *bp;
{
	int interleave = (int)bp->b_parms;

	SCSI_TRACE(CMDS)("scsi_format_unit\n");

     	bp->b_flags |= B_READ;
	bp->b_resid = bp->b_bcount = 0;
	six_byte_cmd(bp, CMDformat_unit, m_unit(bp->b_dev), 0,
		interleave&0xFF, 0);
}

scsi_mode_sense_cmd(bp)
register struct buf *bp;
{
	int page_code = (int)bp->b_parms;
	SCSI_TRACE(CMDS)("scsi_mode_sense (Pg) %d\n", page_code);

     	bp->b_flags |= B_READ;	/* data transfer direction,
				 * Variable length data
				 */
	/* data byte count is vendor dependent */
	bp->b_resid = bp->b_bcount = SZ_VAR_CNT;
	bp->b_queue->b_xcount = bp->b_bcount;

	six_byte_cmd(bp, CMDmode_sense, m_unit(bp->b_dev), 
			page_code<<8, SZ_VAR_CNT, 0);
}

/*
** Since b_parms is a char, we can only do up to 255 bytes of mode select data.
*/
scsi_mode_select(bp)
register struct buf	*bp;
{
	SCSI_TRACE(CMDS)("scsi_mode_select\n");

	bp->b_flags &= ~B_READ;		/* B_WRITE */
	bp->b_resid = bp->b_bcount = (unsigned char)bp->b_parms;
	bp->b_queue->b_xcount = bp->b_bcount;

	six_byte_cmd(bp, CMDmode_select, m_unit(bp->b_dev), 0, bp->b_resid, 0);
}

scsi_inquiry(bp)
register struct buf *bp;
{
	SCSI_TRACE(CMDS)("scsi_inquiry\n");

     	bp->b_flags |= B_READ;	/* data transfer direction,
				 * Variable length data
				 */
	bp->b_resid = bp->b_bcount = SZ_INQUIRY; 
	bp->b_queue->b_xcount = bp->b_bcount;

	six_byte_cmd(bp, CMDinquiry, m_unit(bp->b_dev), 0, SZ_INQUIRY, 0);
}

scsi_start_stop(bp)
register struct buf *bp;
{
	SCSI_TRACE(CMDS)("scsi_start_stop\n");

     	bp->b_flags |= B_READ;

	bp->b_bcount = 0;
	six_byte_cmd(bp, CMDstart_stop_unit, m_unit(bp->b_dev), 0, 
		(int)bp->b_parms&0x3, 0);
}

scsi_read_element_status(bp)
register struct buf *bp;
{
	struct {
		char    opcode;
		char    code;
		short   starting_element;
		short   num_elements;
		char    resv;
		char	allocation_msb;
		short	allocation_lsw;
		char    resv1;
		char    control;
	} cdb;  /* command descriptor block */

	SCSI_TRACE(CMDS)("scsi_read_element_status\n");

     	bp->b_flags |= B_READ;

	cdb.opcode		= CMDread_element_status;
	cdb.code		= (m_unit(bp->b_dev) <<5) | (bp->b_parms & 0xf);
	cdb.starting_element	= 0;
	cdb.num_elements	= 0;
	cdb.resv		= 0;
	cdb.allocation_msb	= ((bp->b_bcount >> 16) & 0xff);
	cdb.allocation_lsw	= (bp->b_bcount & 0xffff);
	cdb.resv1		= 0;
	cdb.control		= 0;

        scsi_if_transfer(bp, &cdb, 12, B_WRITE, MUST_FHS);
}
scsi_move_medium(bp)
register struct buf *bp;
{
	struct {
		char    opcode;
		char    code;
		short   transport;
		short   source;
		short   destination;
		short   resv;
		char    invert;
		char    control;
	} cdb;  /* command descriptor block */

	SCSI_TRACE(CMDS)("scsi_move_medium\n");

     	bp->b_flags |= B_READ;

	cdb.opcode	= CMDmove_medium;
	cdb.code	= 0;
	cdb.transport	= ((struct move_medium_parms *)(bp->b_s2))->transport;
	cdb.source	= ((struct move_medium_parms *)(bp->b_s2))->source;
	cdb.destination	= ((struct move_medium_parms *)(bp->b_s2))->destination;
	cdb.resv	= 0;
	cdb.invert	= ((struct move_medium_parms *)(bp->b_s2))->invert;
	cdb.control	= 0;

	bp->b_bcount = 0;

        scsi_if_transfer(bp, &cdb, 12, B_WRITE, MUST_FHS);
}

scsi_init_element_status(bp)
register struct buf *bp;
{
	SCSI_TRACE(CMDS)("scsi_init_element_status\n");

     	bp->b_flags |= B_READ;

	bp->b_bcount = 0;
	six_byte_cmd(bp, CMDinit_element_status, m_unit(bp->b_dev), 0, 0, 0);
}

scsi_reserve(bp)
register struct buf *bp;
{
	struct {
		char    opcode;
		char    code;
		char    lba_msb;
		char    lba_mid;
		char    lba_lsb;
		char    control;
	} cdb;  /* command descriptor block */

	SCSI_TRACE(CMDS)("scsi_reserve\n");

     	bp->b_flags |= B_WRITE;
	cdb.opcode   = CMDreserve;
	cdb.code     = ((struct reserve_parms *)(bp->b_s2))->element;
	cdb.lba_msb  = ((struct reserve_parms *)(bp->b_s2))->resv_id;
	cdb.lba_mid  = (char)((unsigned char) 
			(((struct reserve_parms *)(bp->b_s2))->ell)) >> 8;
	cdb.lba_lsb  = (char)(((struct reserve_parms *)(bp->b_s2))->ell & 0xff);
	cdb.control  = 0;

        scsi_if_transfer(bp, &cdb, 6, B_WRITE, MUST_FHS);
}

scsi_release(bp)
register struct buf *bp;
{
	struct {
		char    opcode;
		char    code;
		char    lba_msb;
		char    lba_mid;
		char    lba_lsb;
		char    control;
	} cdb;  /* command descriptor block */

	SCSI_TRACE(CMDS)("scsi_release\n");

     	bp->b_flags |= B_WRITE;
	cdb.opcode   = CMDrelease;
	cdb.code     = ((struct reserve_parms *)(bp->b_s2))->element;
	cdb.lba_msb  = ((struct reserve_parms *)(bp->b_s2))->resv_id;
	cdb.lba_mid  = 0;
	cdb.lba_lsb  = 0;
	cdb.control  = 0;

        scsi_if_transfer(bp, &cdb, 6, B_WRITE, MUST_FHS);
}


scsi_prevent_media(bp)
register struct buf *bp;
{
        char cdb[6];

        cdb[0] = CMDprevent_allow_media;
        cdb[1] = 0x00;
        cdb[2] = 0x00;
        cdb[3] = 0x00;
        cdb[4] = 0x01;
        cdb[5] = 0x00;

	SCSI_TRACE(CMDS)("scsi_prevent_media\n");

     	bp->b_flags |= B_WRITE;
        scsi_if_transfer(bp, cdb, 6, B_WRITE, MUST_FHS);
}


scsi_allow_media(bp)
register struct buf *bp;
{
        char cdb[6];

        cdb[0] = CMDprevent_allow_media;
        cdb[1] = 0x00;
        cdb[2] = 0x00;
        cdb[3] = 0x00;
        cdb[4] = 0x00;
        cdb[5] = 0x00;

	SCSI_TRACE(CMDS)("scsi_allow_media\n");

     	bp->b_flags |= B_WRITE;
        scsi_if_transfer(bp, cdb, 6, B_WRITE, MUST_FHS);
}
