/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/io/RCS/an_hpib0.c,v $
 * $Revision: 1.19.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:25:37 $
 *
 */
/***************************************************************************/
/**                                                                       **/
/**   This is the I/O manager analysis routine for hpib0.                 **/
/**                                                                       **/
/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sio/llio.h>
#include <h/types.h>
#include <sio/iotree.h>
#include <sio/hpib0.h>			/* hpib0's normal .h file	      */
#include "aio.h"			/* all analyze i/o defs		      */

int	hpib0_an_rev = DAM_REV_CODE;	/* global -- rev number of hpib0 DS   */
static char *me      = "hpib0";

#define	BOOLEAN(s)	((s) ? "true" : "false")
#define bit(x)  	(1 << (x))
#define	streq(a, b)	(strcmp((a), (b)) == 0)
#define	min(a, b)	(((a) < (b)) ? (a) : (b))
#define	BITS	(BYTE_MODE|CIOCA_S1S2|LOGCH_BREAK|BLOCKED|CONTINUE)
char *addr_class();

int
an_hpib0(call_type, port_num, out_fd, option_string)
    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */
{
    int    		addr;		/* address of extern                  */
    int			code_rev;	/* revision of driver code	      */
    hpib_pda_type	pda;		/* port data area                     */
    struct mgr_info_type mgr;		/* all generic manager info	      */

    int			an_hpib_decode_message();
    int			an_hpib_decode_status();

    /* grab the manager information */
    aio_get_mgr_info(AIO_PORT_NUM_TYPE, port_num, &mgr);

    /* grab the pda */
    if (an_grab_virt_chunk(0, mgr.pda_address, (char *)&pda, sizeof(pda)) != 0) {
	fprintf(stderr, "Couldn't get pda\n");
	return(0);
    }

    /* perform the correct action depending on what call_type is */
    switch (call_type) {

	case AN_MGR_INIT:
	    /* check driver rev against analysis rev -- complain if a problem */
	    if (an_grab_extern("hpib0_an_rev", &addr) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, 0, hpib0_an_rev, out_fd);
	    else if (an_grab_real_chunk(addr, &code_rev, sizeof(code_rev)) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, 0, hpib0_an_rev, out_fd);
	    else if (code_rev != hpib0_an_rev)
		aio_rev_mismatch(me, AIO_INCOMPAT_REVS, code_rev, hpib0_an_rev,
				 out_fd);

	    /* put my decoding routines into system lists        */
	    aio_init_llio_status_decode(SUBSYS_HPIB0, an_hpib_decode_status);
	    aio_init_message_decode(START_HPIB_MSG, START_HPIB_MSG+9,
	                            an_hpib_decode_message);
	    break;

	case AN_MGR_BASIC:
	    an_hpib0_all(&pda, out_fd);
	    break;

	case AN_MGR_DETAIL:
	    an_hpib0_all(&pda, out_fd);
	    break;

	case AN_MGR_OPTIONAL:
	    an_hpib0_optional(&pda, out_fd, option_string);
	    break;

	case AN_MGR_HELP:
	    an_hpib0_help(stderr);
	    break;

    }
    return(0);
}

an_hpib0_optional(pda, out_fd, option_string)
    hpib_pda_type	*pda;
    FILE		*out_fd;
    char		*option_string;
{
    char *token;
    int	 dam_flag	= 0;
    int	 da_flag	= 0;
    int	 dm_flag	= 0;
    int	 queue_flag	= 0;
    int	 req_flag	= 0;
    int	 req_num 	= -1;
    int	 all_flag	= 0;

    /* read through option string and set appropriate flags */
    token = strtok(option_string, " ");

    do {
	if      (streq(token,    "dam"))   dam_flag   = 1;
	else if (streq(token,     "da"))   da_flag    = 1;
	else if (streq(token,     "dm"))   dm_flag    = 1;
	else if (streq(token, "queued"))   queue_flag = 1;
	else if (streq(token,    "req"))   req_flag   = 1;
	else if (streq(token,    "all"))   all_flag   = 1;
	else if (strncmp(token, "req", strlen("req")) == 0) {
	    req_num = token[3] - '0';

	    if (!isdigit(token[3]) || req_num < 0 || req_num >= DAM_MAX_UIDS) {
	    	fprintf(stderr, "invalid request number <%s>\n", &token[3]);
	    	an_hpib0_help(stderr);
	    	return;
	    }
	}
	else  {
	    fprintf(stderr, "invalid option <%s>\n", token);
	    an_hpib0_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (dam_flag)   an_hpib0_dam_info(pda, out_fd);
    if (da_flag)    an_hpib0_da_info(pda, out_fd);
    if (dm_flag)    an_hpib0_dm_info(pda, out_fd);
    if (queue_flag) an_hpib0_queued(pda, out_fd);
    if (req_flag)   an_hpib0_req_info(pda, out_fd);
    if (req_num != -1) {
	fprintf(out_fd, "Request %d:         %sactive\n", req_num,
	    	pda->uid_to_req[req_num] ? "": "NOT ");
	an_hpib0_rcs(&pda->req_info[req_num], out_fd);
    }
    if (all_flag)   an_hpib0_all(pda, out_fd);
}

an_hpib0_dam_info(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    dam_info_table_type *d = &pda->dam_info;
    dam_lock_info_type  *dl = &d->dam_lock_info;

    fprintf(out_fd, "\n===========DAM INFO===========\n");
    fprintf(out_fd, "cur_dam_state        = %d -- ", d->cur_dam_state);
    switch (d->cur_dam_state) {
	case DAM_INIT:		fprintf(out_fd, "DAM_INIT\n");	break;
	case DAM_NORMAL:	fprintf(out_fd, "DAM_NORMAL\n");break;
	case DA_INIT:		fprintf(out_fd, "DA_INIT\n");	break;
	case DAM_LOCK:		fprintf(out_fd, "DAM_LOCK\n");	break;
	default:		fprintf(out_fd, "unknown\n");	break;
    }
    fprintf(out_fd, "cur_dam_substate     = %d -- ", d->cur_dam_substate);
    switch (d->cur_dam_substate) {
	case -1: fprintf(out_fd, "DAM_PRE_INIT\n");			break;
	case  0: fprintf(out_fd, "DAM_POST_CREATE/INIT_COMPLETE/DAM_LOCKED_OTHER\n");break;
	case  1: fprintf(out_fd, "SEND_SELFTEST/LOCKED_HW_FAILURE\n");	break;
	case  2: fprintf(out_fd, "CHECK_SELFTEST\n");			break;
	case  3: fprintf(out_fd, "GET_DA_TYPE\n");			break;
	case  4: fprintf(out_fd, "CHECK_DA_TYPE\n");			break;
	case  5: fprintf(out_fd, "SEND_READ_IDY\n");			break;
	case  6: fprintf(out_fd, "SEND_DA_INIT\n");			break;
	default: fprintf(out_fd, "unknown\n");				break;
    }
    fprintf(out_fd, "frame_release        = %5s\n", BOOLEAN(d->frame_release));
    fprintf(out_fd, "da_hw_address        = %5d, ",    d->da_hw_address);
    fprintf(out_fd, "dam_port_num         = %5d, ",    d->dam_port_num);
    fprintf(out_fd, "cam_port_num         = %d\n",     d->cam_port_num);
    fprintf(out_fd, "cam_req_subq         = %5d, ",    d->cam_req_subq);
    fprintf(out_fd, "dam_pf_num           = 0x%08x\n", d->dam_pf_num);

    if (dl->locking_port) {
	fprintf(out_fd, "lock.locking_port    = %5d, ", dl->locking_port);
	fprintf(out_fd, "lock.locking_subq    = %d\n",  dl->locking_subq);
	fprintf(out_fd, "lock.asyn_event_type = %d\n",  dl->asyn_event_type);
	fprintf(out_fd, "lock.event_port_num  = %5d, ", dl->event_port_num);
	fprintf(out_fd, "lock.event_subqueue  = %d\n",  dl->event_subqueue);
	fprintf(out_fd, "lock.poweron_request = %5s, ", BOOLEAN(dl->poweron_request));
	fprintf(out_fd, "lock.w_poweron_reply = %5s\n", BOOLEAN(dl->w_poweron_reply));
	fprintf(out_fd, "lock.reset_request   = %5s, ", BOOLEAN(dl->reset_request));
	fprintf(out_fd, "lock.reset_msg_subq  = %2d\n", dl->reset_msg_subq);
	fprintf(out_fd, "lock.reset_msg_uid   = %5d, ", dl->reset_msg_uid);
	fprintf(out_fd, "lock.reset_msg_tron  = 0x%x\n", dl->reset_msg_tron);
    }
    fprintf(out_fd, "\nmy_req:\n");
    an_hpib0_rcs(&d->my_req, out_fd);
}

an_hpib0_da_info(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    da_info_table_type *d = &pda->da_info;
    int i;

    fprintf(out_fd, "\n===========DA INFO===========\n");
    fprintf(out_fd, "ren           = %s\n",   BOOLEAN(d->ren));
    fprintf(out_fd, "atn           = %s\n",   BOOLEAN(d->atn));
    fprintf(out_fd, "ppoll_ist     = 0x%x\n", d->ppoll_ist);
    fprintf(out_fd, "ppoll_sense   = 0x%x\n", d->ppoll_sense);
    fprintf(out_fd, "ppoll_delay   = 0x%x\n", d->ppoll_delay);
    fprintf(out_fd, "srq_resp      = 0x%x\n", d->srq_resp);
    fprintf(out_fd, "card_address  = 0x%x\n", d->card_address);
    for (i = 0; i < 3; i++)
    	fprintf(out_fd, "interrupts[%d] = 0x%x\n", i, d->interrupts[i]);

    fprintf(out_fd, "\nCard identification:\n");
    fprintf(out_fd, "card_id      = %d\n", d->da_idy.card_id);
    fprintf(out_fd, "firmware_id  = %d\n", d->da_idy.firmware_id);
    fprintf(out_fd, "firmware_rev = %d\n", d->da_idy.firmware_rev);

    fprintf(out_fd, "mpx          = %s\n", BOOLEAN(d->da_idy.mpx));
    fprintf(out_fd, "psa          = %d\n", d->da_idy.psa);
    fprintf(out_fd, "mod          = %d\n", d->da_idy.mod);
    fprintf(out_fd, "max_bsize_hi = %d\n", d->da_idy.max_bsize_hi);
    fprintf(out_fd, "max_bsize_lo = %d\n", d->da_idy.max_bsize_lo);

    fprintf(out_fd, "\nInitialization vquads:\n");
    for (i = 0; i < DAM_INIT_VQUADS; i++) an_hpib0_vquad(&d->vquad[i], out_fd);
}

an_hpib0_dm_info(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    int i;
    dam_dm_info_entry_type  *d = pda->dm_info;
    struct mgr_info_type mgr;

    fprintf(out_fd, "\n===========DM INFO===========\n");

    for (i = 0; i < DAM_MAX_CONFIG_DM; i++) {
	if (d[i].port_number == 0) continue;
	fprintf(out_fd, "Bus address %d:\n", i);
	fprintf(out_fd, "  port_number    = %3d ", d[i].port_number);
	aio_get_mgr_info(AIO_PORT_NUM_TYPE, d[i].port_number, &mgr);
	fprintf(out_fd, "(%s at %s,", mgr.mgr_name, mgr.hw_address);
	fprintf(out_fd, " rev_code = %d)\n", d[i].dm_bind_info.hm_rev_code);
	fprintf(out_fd, "  cur_dm_state   = ");
	switch (d[i].cur_dm_state) {
	    case DAM_DM_NORMAL:	fprintf(out_fd, "DAM_DM_NORMAL");	break;
	    case DAM_DM_PF:	fprintf(out_fd, "DAM_DM_PF    ");	break;
	    default:		fprintf(out_fd, "unknown (%d)",
					d[i].cur_dm_state);		break;
	}
	fprintf(out_fd, ", hm_config_addr = %d/%d/%d\n",
		d[i].dm_bind_info.hm_config_addr_3,
		d[i].dm_bind_info.hm_config_addr_2,
		d[i].dm_bind_info.hm_config_addr_1);
	fprintf(out_fd, "  reply_subq     = %2d, ",
	    d[i].dm_bind_info.reply_subq);
	fprintf(out_fd, "hm_event_subq  = %d\n",
	    d[i].dm_bind_info.hm_event_subq);
    }
}

an_hpib0_req_info(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    int i;

    fprintf(out_fd, "\n===========REQUEST INFO===========\n");

    for (i = 0; i < DAM_MAX_UIDS; i++) {
	fprintf(out_fd, "\nRequest %d:         %sactive\n", i,
	    pda->uid_to_req[i] ? "": "NOT ");
	if (pda->uid_to_req[i]) an_hpib0_rcs(&pda->req_info[i], out_fd);
    }
}

an_hpib0_queued(pda, out_fd)
    hpib_pda_type *pda;
    FILE	  *out_fd;
{
    int	i;
    struct mgr_info_type mgr;

    aio_get_mgr_info(AIO_PORT_NUM_TYPE, pda->dam_info.dam_port_num, &mgr);
    fprintf(out_fd, "Subq summary: enabled subqs = 0x%08x\n",
	mgr.enabled_subqs);
    fprintf(out_fd, "              active subqs  = 0x%08x\n\n",
	mgr.active_subqs);

    /* walk through subqs and decode */
    for (i = POWERFAIL_SUBQUEUE; i <= LOCK_REQUEST_SUBQUEUE; i++)  {
	fprintf(out_fd, "   subq %2d:  ", i);

	switch (i) {
	    case POWERFAIL_SUBQUEUE:
	    	fprintf(out_fd, "(powerfail)              "); break;

	    case DAM_INIT_REPLY_SUBQUEUE:
	    	fprintf(out_fd, "(H/W init/configuration) "); break;

	    case DIAG_REQUEST_SUBQUEUE:
	    	fprintf(out_fd, "(diagnostics)            "); break;

	    case CAM_IO_REPLY_SUBQUEUE:
	    	fprintf(out_fd, "(CAM replies)            "); break;

	    case DM_IO_REQUEST_SUBQUEUE:
	    	fprintf(out_fd, "(DM requests)            "); break;

	    case LOCK_REQUEST_SUBQUEUE:
	    	fprintf(out_fd, "(lock requests)          "); break;

	    default:					      
	    	fprintf(out_fd, "                         "); break;
	}

	fprintf(out_fd," %sabled     ",
	    (mgr.enabled_subqs & ELEMENT_OF_32(i)) ? " en" : "dis");

	if (mgr.active_subqs & ELEMENT_OF_32(i)) {
    	    int msg, j = 0;

	    fprintf(out_fd,"msgs queued\n");
	    while (aio_queued_message(mgr.port_num, i, j++, &msg) == AIO_OK)
		aio_decode_message(msg, "        ", out_fd);
	}
	else
	    fprintf(out_fd, "no msgs queued\n");
    }
}

an_hpib0_all(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    an_hpib0_dam_info(pda, out_fd);
    an_hpib0_da_info(pda, out_fd);
    an_hpib0_dm_info(pda, out_fd);
    an_hpib0_queued(pda, out_fd);
    an_hpib0_req_info(pda, out_fd);
}

an_hpib_decode_status(llio_status, out_fd)
    llio_status_type	llio_status;
    FILE		*out_fd;
{
    char *s;

    fprintf(out_fd, "hpib llio status = 0x%08x -- (", llio_status);
    if (llio_status.u.subsystem == SUBSYS_HPIB0)
	fprintf(out_fd, "SUBSYS_HPIB0, ");
    else 
	fprintf(out_fd, "subsys %d?, ", llio_status.u.subsystem);

    switch (llio_status.u.proc_num) {
	case GLOBAL:	fprintf(out_fd, "GLOBAL, ");
	case LOCAL:	fprintf(out_fd, "LOCAL, ");
	default:	fprintf(out_fd, "%d?, ", llio_status.u.proc_num);
    }

    switch (llio_status.u.error_num) {
        case DAM_REQ_OK:		s = "DAM_REQ_OK";		break;
        case READ_EOI_EVEN:		s = "READ_EOI_EVEN (ok) ";	break;
        case READ_EOI_ODD:		s = "READ_EOI_ODD (ok) ";	break;
        case READ_COUNT_EVEN:		s = "READ_COUNT_EVEN (ok)";	break;
        case READ_COUNT_ODD:		s = "READ_COUNT_ODD (ok)";	break;
        case READ_LF_EVEN:		s = "READ_LF_EVEN (ok)";	break;
        case READ_LF_ODD:		s = "READ_LF_ODD (ok)";		break;
        case READ_MSA_EVEN:		s = "READ_MSA_EVEN (ok)";	break;
        case READ_MSA_ODD:		s = "READ_MSA_ODD (ok)";	break;
        case READ_CEND:			s = "READ_CEND (ok)";		break;
        case DA_NO_DEVICE_IDY:		s = "DA_NO_DEVICE_IDY (ok)";	break;
        case FIFO_EOI_COUNT_EVEN:	s = "FIFO_EOI_COUNT_EVEN (ok)";	break;
        case FIFO_EOI_COUNT_ODD:	s = "FIFO_EOI_COUNT_ODD (ok)";	break;
        case FIFO_EOI_LF_EVEN:		s = "FIFO_EOI_LF_EVEN (ok)";	break;
        case FIFO_EOI_LF_ODD:		s = "FIFO_EOI_LF_ODD (ok)";	break;
        case FIFO_EOI_MATCH_EVEN:	s = "FIFO_EOI_MATCH_EVEN (ok)";	break;
        case FIFO_EOI_MATCH_ODD:	s = "FIFO_EOI_MATCH_ODD (ok)";	break;
        case FIFO_MATCH_EVEN:		s = "FIFO_MATCH_EVEN (ok)";	break;
        case FIFO_MATCH_ODD:		s = "FIFO_MATCH_ODD (ok)";	break;
        case DIAG_SHORT_READ:		s = "DIAG_SHORT_READ (ok)";	break;
        case DAM_REQ_PTRS_ERR:		s = "DAM_REQ_PTRS_ERR";		break;
        case DAM_GET_MSG_FRAME_ERR:	s = "DAM_GET_MSG_FRAME_ERR";	break;
        case DAM_GET_IO_QUADS_ERR:	s = "DAM_GET_IO_QUADS_ERR";	break;
        case DM_SEND_ERROR:		s = "DM_SEND_ERROR";		break;
        case CAM_SEND_ERROR:		s = "CAM_SEND_ERROR";		break;
        case CAM_NO_RESOURCES:		s = "CAM_NO_RESOURCES";		break;
        case CAM_IO_ERROR:		s = "CAM_IO_ERROR";		break;
        case INVALID_TRON_UID:		s = "INVALID_TRON_UID";		break;
        case DAP_DOWNLOAD_ERR:		s = "DAP_DOWNLOAD_ERR";		break;
        case DAP_DEV_LOCKED_ERR:	s = "DAP_DEV_LOCKED_ERR";	break;
        case DAP_SW_IN_USE:		s = "DAP_SW_IN_USE";		break;
        case DAP_NOT_PRESENT:		s = "DAP_NOT_PRESENT";		break;
        case DA_FCODE_UNIMP:		s = "DA_FCODE_UNIMP";		break;
        case DA_NOT_SC:			s = "DA_NOT_SC";		break;
        case DA_NOT_CIC:		s = "DA_NOT_CIC";		break;
        case DA_CIC:			s = "DA_CIC";			break;
        case DA_CIC_OR_NOT_ADDR:	s = "DA_CIC_OR_NOT_ADDR";	break;
        case DA_DCL_DETECTED:		s = "DA_DCL_DETECTED";		break;
        case DA_IFC_DATA_ABORT:		s = "DA_IFC_DATA_ABORT";	break;
        case DA_DATA_FROZEN:		s = "DA_DATA_FROZEN";		break;
        case DA_LOOPBACK_FAILED:	s = "DA_LOOPBACK_FAILED";	break;
        case DA_PPOLL_INT_ENABLED:	s = "DA_PPOLL_INT_ENABLED";	break;
        case DA_SST_FAILED:		s = "DA_SST_FAILED";		break;
        case DA_IDY_FAILED:		s = "DA_IDY_FAILED";		break;
        case DA_NO_TSTAT:		s = "DA_NO_TSTAT";		break;
        case DA_DOD_PER:		s = "DA_DOD_PER";		break;
        case DA_DAP_ILLEGAL_DAL:	s = "DA_DAP_ILLEGAL_DAL";	break;
        case DA_DAP_LENGTH_EXCEEDED:	s = "DA_DAP_LENGTH_EXCEEDED";	break;
        case DA_IOS_EXCEEDED:		s = "DA_IOS_EXCEEDED";		break;
        case DA_INVALID_BLOCKSIZE:	s = "DA_INVALID_BLOCKSIZE";	break;
        case DIAG_INVALID_REG_OFFSET:	s = "DIAG_INVALID_REG_OFFSET";	break;
        case DIAG_BUF_FULL:		s = "DIAG_BUF_FULL";		break;
        case DIAG_MSA_READ:		s = "DIAG_MSA_READ";		break;
        case DIAG_NOT_ADDR:		s = "DIAG_NOT_ADDR";		break;
        case CS80_CMD_TO:		s = "CS80_CMD_TO";		break;
        case CS80_CMD_TO_NCIC:		s = "CS80_CMD_TO_NCIC";		break;
        case CS80_CMD_TO_NUN:		s = "CS80_CMD_TO_NUN";		break;
        case CS80_EX_TO:		s = "CS80_EX_TO";		break;
        case CS80_EX_TO_NCIC:		s = "CS80_EX_TO_NCIC";		break;
        case CS80_EX_TO_NUN:		s = "CS80_EX_TO_NUN";		break;
        case CS80_REP_TO:		s = "CS80_REP_TO";		break;
        case CS80_REP_TO_NCIC:		s = "CS80_REP_TO_NCIC";		break;
        case CS80_REP_TO_NUN:		s = "CS80_REP_TO_NUN";		break;
        case CS80_DRP_TO:		s = "CS80_DRP_TO";		break;
        case CS80_DRP_TO_NCIC:		s = "CS80_DRP_TO_NCIC";		break;
        case CS80_DRP_TO_NUN:		s = "CS80_DRP_TO_NUN";		break;
        case DAM_SW_ERROR:		s = "DAM_SW_ERROR";		break;
        case DAM_DAM_TABLE_ERR:		s = "DAM_DAM_TABLE_ERR";	break;
        case DAM_DM_TABLE_ERR:		s = "DAM_DM_TABLE_ERR";		break;
        case DAM_REQ_TABLE_ERR:		s = "DAM_REQ_TABLE_ERR";	break;
        case DAM_NO_UIDS_AVAIL:		s = "DAM_NO_UIDS_AVAIL";	break;
        case RS_BLOCK_UID_ERR:		s = "RS_BLOCK_UID_ERR";	 	break;
        case RS_BLOCK_VALID_ERR:	s = "RS_BLOCK_VALID_ERR";	break;
	case RS_BLOCK_PTR_ERR:		s = "RS_BLOCK_PTR_ERR";		break;
        case REQ_INFO_UID_ERR:		s = "REQ_INFO_UID_ERR";		break;
        case REQ_INFO_VALID_ERR:	s = "REQ_INFO_VALID_ERR";	break;
        case DAM_UNKNOWN_TSTAT:		s = "DAM_UNKNOWN_TSTAT";	break;
        case DIAG_CHECKSUM_ERROR: 	s = "DIAG_CHECKSUM_ERROR";	break;
        case DIAG_CARD_NOT_PRESENT:	s = "DIAG_CARD_NOT_PRESENT";	break;
        case CS80_CMD_BUFFER_ERROR:	s = "CS80_CMD_BUFFER_ERROR";	break;
	default:			s = "unrecognized?";		break;
    }
    fprintf(out_fd, "%s)\n", s);
}

an_hpib0_help(out_fd)
    FILE		*out_fd;
{
    fprintf(out_fd, "\nvalid options:\n");
    fprintf(out_fd, "   dam     --  decode device adapter manager state\n");
    fprintf(out_fd, "   da      --  decode device adapter state\n");
    fprintf(out_fd, "   dm      --  decode device manager state\n");
    fprintf(out_fd, "   queued  --  decode queued messages\n");
    fprintf(out_fd, "   req     --  decode active requests\n");
    fprintf(out_fd, "   req#    --  decode particular request (# = 0-7)\n");
    fprintf(out_fd, "   all     --  do all of the above\n");
    fprintf(out_fd, "   help    --  print this help screen\n");
}

an_hpib0_rcs(rcs, out_fd)
    req_info_ptr_type    rcs;
    FILE		*out_fd;
{
    int i;
    struct mgr_info_type mgr;

    fprintf(out_fd, "uid              = %5d, ", rcs->uid);
    fprintf(out_fd, "reply_subqueue   = %5d, ",  rcs->reply_subqueue);
    fprintf(out_fd, "diag_option      = %d\n", rcs->diag_option);
    fprintf(out_fd, "powerfail        = %5s, ", BOOLEAN(rcs->powerfail));
    fprintf(out_fd, "logch_break_mode = %5s\n", BOOLEAN(rcs->logch_break_mode));
    fprintf(out_fd, "original_request = ", rcs->original_request);
    switch (rcs->original_request) {
	case DM_IO_REQ:		fprintf(out_fd, "DM_IO_REQ\n");		break;
	case DA_INIT_IO_REQ:	fprintf(out_fd, "DA_INIT_IO_REQ\n");	break;
	case DA_INTRPT_REQ:	fprintf(out_fd, "DA_INTRPT_REQ\n");	break;
	case DIAG_IO_REQ:	fprintf(out_fd, "DIAG_IO_REQ\n");	break;
	case DM_DA_REQ:		fprintf(out_fd, "DM_DA_REQ\n");		break;
	default:		fprintf(out_fd, "unknown\n");		break;
    }
    if (rcs->original_request == DM_IO_REQ ||
	rcs->original_request == DM_DA_REQ ||
	rcs->original_request == DIAG_IO_REQ) {
	llio_std_header_type *l = &rcs->save_msg_header;

	fprintf(out_fd, "save_msg_header:\n");
	fprintf(out_fd, "  msg_descriptor = %5d, ", l->msg_descriptor);
        fprintf(out_fd, "from_port      = %d ", l->from_port);
	if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, l->from_port, &mgr) == AIO_OK)
		fprintf(out_fd, "(%s at %s)\n", mgr.mgr_name, mgr.hw_address);
	else if (aio_get_mgr_name(l->from_port, mgr.mgr_name) == AIO_OK)
		fprintf(out_fd, "(%s)\n", mgr.mgr_name);
	else
		fprintf(out_fd, "(unknown manager)\n");
	fprintf(out_fd, "  message_id     = %5d, ", l->message_id);
	fprintf(out_fd, "trn            = 0x%x\n", l->transaction_num);

        fprintf(out_fd, "save_io_ptrs:\n");
	an_hpib_decode_req_parm(&rcs->save_io_ptrs, "  ", out_fd);
    }
    fprintf(out_fd, "request_length   = %d\n", rcs->request_length);
    fprintf(out_fd, "vquad_chain      = 0x%08x, ", rcs->vquad_chain);
    fprintf(out_fd, "data_vquad_ptr   = 0x%08x\n", rcs->data_vquad_ptr);
    fprintf(out_fd, "request_block    = ");
    for (i = 0; i < DAM_STD_BLOCKSIZE;) {
	fprintf(out_fd, "%02x ", rcs->request_block[i] & 0xff);
	if ((++i % 16) == 0) fprintf(out_fd, "\n                   ");
    }
    fprintf(out_fd, "\nstatus_block     = ");
    for (i = 0; i < DAM_STD_BLOCKSIZE;) {
	fprintf(out_fd, "%02x ", rcs->status_block[i] & 0xff);
	if ((++i % 16) == 0) fprintf(out_fd, "\n                   ");
    }
    fprintf(out_fd, "\nlink             = 0x%08x\n", rcs->link);
    if (rcs->vquad_chain) {
    	struct cio_vquad_type vq, *vqp;

	fprintf(out_fd, "vquads:\n");
	vqp = rcs->vquad_chain;
	while (vqp && an_grab_virt_chunk(0, (unsigned)vqp, (char *)&vq,
		sizeof(vq)) == 0) {
	    an_hpib0_vquad(&vq, out_fd);
	    vqp = vq.link;
	}
    }
}

an_hpib0_vquad(vqp, out_fd)
    struct cio_vquad_type *vqp;
    FILE		*out_fd;
{
    int cmd = vqp->command.cio_cmd & ~BITS;

    fprintf(out_fd, "\tcommand  = 0x%08x -- ", vqp->command.cio_cmd);
    switch (cmd) {
	case CIOCA_IDY  & ~BITS: fprintf(out_fd, "CIOCA_IDY");	break;
	case CIOCA_DLD  & ~BITS: fprintf(out_fd, "CIOCA_DLD");	break;
	case CIOCA_DLD1 & ~BITS: fprintf(out_fd, "CIOCA_DLD1");	break;
	case CIOCA_PAU  & ~BITS: fprintf(out_fd, "CIOCA_PAU");	break;
	case CIOCA_DIS  & ~BITS: fprintf(out_fd, "CIOCA_DIS");	break;
	case CIOCA_RS   & ~BITS: fprintf(out_fd, "CIOCA_RS");	break;
	case CIOCA_RS_D & ~BITS: fprintf(out_fd, "CIOCA_RS_D");	break;
	case CIOCA_WC   & ~BITS: fprintf(out_fd, "CIOCA_WC");	break;
	case CIOCA_RD   & ~BITS: fprintf(out_fd, "CIOCA_RD");	break;
	case CIOCA_WD   & ~BITS: fprintf(out_fd, "CIOCA_WD");	break;
	case CIOCA_RTS  & ~BITS: fprintf(out_fd, "CIOCA_RTS");	break;
	case CIOCA_WTC  & ~BITS: fprintf(out_fd, "CIOCA_WTC");	break;
	case CIOCA_RDS  & ~BITS: fprintf(out_fd, "CIOCA_RDS");	break;
	case CIOCA_RDS1 & ~BITS: fprintf(out_fd, "CIOCA_RDS1");	break;
	case CIOCA_CLC  & ~BITS: fprintf(out_fd, "CIOCA_CLC");	break;
	case CIOCA_LNKST& ~BITS: fprintf(out_fd, "CIOCA_LNKST");break;
	default:		 fprintf(out_fd, "unknown");	break;
    }
    if (vqp->command.cio_cmd & BYTE_MODE)   fprintf(out_fd, " | BYTE_MODE");
    if (vqp->command.cio_cmd & CIOCA_S1S2)  fprintf(out_fd, " | CIOCA_S1S2");
    if (vqp->command.cio_cmd & LOGCH_BREAK) fprintf(out_fd, " | LOGCH_BREAK");
    if (vqp->command.cio_cmd & BLOCKED)	    fprintf(out_fd, " | BLOCKED");
    if (vqp->command.cio_cmd & CONTINUE)    fprintf(out_fd, " | CONTINUE");
    fprintf(out_fd,", link     = 0x%08x\n", vqp->link);
    fprintf(out_fd, "\tcount    = %d, ", vqp->count);
    fprintf(out_fd, "residue = %d\n", vqp->residue);
    fprintf(out_fd, "\tbuffer   = %d.0x%08x %s\n", vqp->buffer.u.data_sid,
    	vqp->buffer.u.data_vba, addr_class(vqp->addr_class));

    if (vqp->count > 0) {
    	bit8  buf[DAM_STD_BLOCKSIZE];
	char *prefix = "\t           ";

	an_hpib_dumpv(vqp->buffer.u.data_sid, vqp->buffer.u.data_vba,
		       vqp->count, prefix, out_fd);
	if (an_grab_virt_chunk(vqp->buffer.u.data_sid,
		(unsigned)vqp->buffer.u.data_vba, (char *)buf,
		min(sizeof(buf), vqp->count)) == 0) {
	    if (cmd == (CIOCA_CLC & ~BITS) && vqp->count > 2)
	        an_27110_commands(buf[2], vqp->count-3, &buf[3], prefix,out_fd);
	    else if (cmd == (CIOCA_WC & ~BITS))
	        an_27110_commands(buf[0], vqp->count-1, &buf[1], prefix,out_fd);
	    else if (cmd == (CIOCA_RS & ~BITS) || cmd == (CIOCA_RS_D & ~BITS))
	        an_27110_status(buf[0], prefix, out_fd);
	}
    }
    fprintf(out_fd, "\n");
}

char *
addr_class(class)
    addr_class_type class;
{
    static char buf[40];

    switch (class) {
	case VIRTUAL_BUFFER:  return("VIRTUAL_BUFFER");
	case VIRTUAL_BLOCKS:  return("VIRTUAL_BLOCKS");
	case CONTIGUOUS_REAL: return("CONTIGUOUS_REAL");
	default:	      sprintf(buf, "unknown addr_class %d\n", class);
			      return(buf);
    }
    /*NOTREACHED*/
}

an_hpib_decode_dam_req(rqp, prefix, out_fd)
    dam_req_type	*rqp;
    char		*prefix;
    FILE		*out_fd;
{
    fprintf(out_fd, "%sreply_subq = %d, ", prefix, rqp->reply_subq);
    fprintf(out_fd, "device_number = %d, ", rqp->device_number);
    fprintf(out_fd, "transfer_dir = ");
    switch (rqp->transfer_dir) {
	case NO_DATA:	fprintf(out_fd, "NO_DATA\n");	break;
	case READ_DATA:	fprintf(out_fd, "READ_DATA\n");	break;
	case WRITE_DATA:fprintf(out_fd, "WRITE_DATA\n");break;
	default:	fprintf(out_fd, "unknown (%d)\n",
			rqp->transfer_dir);	break;
    }
    fprintf(out_fd, "%srequest = ", prefix);
    switch (rqp->request) {
	case CS80_EXECUTE:
	    fprintf(out_fd, "CS80_EXECUTE\n");
	    fprintf(out_fd, "%stimeout_value = %d, ", prefix,
		rqp->u.cs80_req.timeout_value);
	    fprintf(out_fd, "transparent = %s\n", 
		BOOLEAN(rqp->u.cs80_req.transparent));
	    break;

	case HPIB_EXECUTE:
	    fprintf(out_fd, "HPIB_EXECUTE\n");
	    fprintf(out_fd, "%stermination_conds = 0x%x, ", prefix,
		rqp->u.hpib_req.termination_conds);
	    fprintf(out_fd, "matched_byte = 0x%x\n", 
		rqp->u.hpib_req.matched_byte);
	    break;

	case MT7970_EXECUTE:
	    fprintf(out_fd, "MT7970_EXECUTE, motion_cmd = 0x%x\n",
		rqp->u.mt7970_req.motion_cmd);
	    fprintf(out_fd, "%stermination_conds = 0x%x, ", prefix,
		rqp->u.mt7970_req.termination_conds);
	    fprintf(out_fd, "require_break = %s\n", 
		BOOLEAN(rqp->u.mt7970_req.require_break));
	    break;

	case DA_REQUEST:
	    fprintf(out_fd, "DA_REQUEST 0x%x:", rqp->u.da_req);
	    an_27110_commands(rqp->u.da_req, 0, NULL, " ", out_fd);
	    break;

	case DAP_SEND:
	    fprintf(out_fd, "DAP_SEND\n");
	    break;

	case DAP_EXECUTE:
	    fprintf(out_fd, "DAP_EXECUTE, require_break = %s\n",
		BOOLEAN(rqp->u.dap_execute_req.require_break));
	    break;

	case SET_BLOCKING:
	    fprintf(out_fd, "SET_BLOCKING\n");
	    break;

	case CLEAR_BLOCKING:
	    fprintf(out_fd, "CLEAR_BLOCKING\n");
	    break;

	default:
	    fprintf(out_fd, "unknown (%d)\n", rqp->request);
	    break;
    }
    an_hpib_decode_req_parm(&rqp->request_parms, prefix, out_fd);
}

an_hpib_decode_message(mp, prefix, out_fd)
    hpib_dam_message_type	*mp;
    char			*prefix;
    FILE			*out_fd;
{
    llio_std_header_type *l = &mp->msg_header;

    switch (l->msg_descriptor) {
	case HPIB_IO_REQ_MSG:
	    fprintf(out_fd, "HPIB_IO_REQ_MSG\n");
	    an_hpib_decode_dam_req(&mp->u.dam_req, prefix, out_fd);
	    break;

	case HPIB_IO_REPLY_MSG:
	    fprintf(out_fd, "HPIB_IO_REPLY_MSG\n%s", prefix);
	    aio_decode_llio_status(mp->u.dam_reply.reply_status, out_fd);
	    an_hpib_decode_req_parm(&mp->u.dam_reply.request_parms,
		prefix, out_fd);
	    fprintf(out_fd, "%sdata_residue = %5d,", prefix,
		mp->u.dam_reply.data_residue);
	    fprintf(out_fd, " status_residue = %5d\n", 
		mp->u.dam_reply.status_residue);
	    break;

	case HPIB_IO_EVENT_MSG:
	    fprintf(out_fd, "HPIB_IO_EVENT_MSG\n");
	    fprintf(out_fd, "%sdevice_number = %d,", prefix,
		mp->u.dam_event.device_number);
	    fprintf(out_fd, "event_type = ");
	    switch (mp->u.dam_event.event_info) {
		case DEVICE_PPOLL:    fprintf(out_fd, "DEVICE_PPOLL");    break;
		case DAM_UNLOCK_READY:fprintf(out_fd, "DAM_UNLOCK_READY");break;
		case DEAD_OR_DYING:   fprintf(out_fd, "DEAD_OR_DYING");   break;
		case DA_INTERRUPT:    fprintf(out_fd, "DA_INTERRUPT");    break;
		case DIAG_INTERRUPT:  fprintf(out_fd, "DIAG_INTERRUPT");  break;
		default:	      fprintf(out_fd, "unknown (%d)",
					mp->u.dam_event.event_info);     break;
	    }
	    fprintf(out_fd, "\n%sevent_detail  = 0x%08x ", prefix,
		mp->u.dam_event.event_detail);
	    if (mp->u.dam_event.event_detail & bit(31)) fprintf(out_fd, "PP0 ");
	    if (mp->u.dam_event.event_detail & bit(30)) fprintf(out_fd, "PP1 ");
	    if (mp->u.dam_event.event_detail & bit(29)) fprintf(out_fd, "PP2 ");
	    if (mp->u.dam_event.event_detail & bit(28)) fprintf(out_fd, "PP3 ");
	    if (mp->u.dam_event.event_detail & bit(27)) fprintf(out_fd, "PP4 ");
	    if (mp->u.dam_event.event_detail & bit(26)) fprintf(out_fd, "PP5 ");
	    if (mp->u.dam_event.event_detail & bit(25)) fprintf(out_fd, "PP6 ");
	    if (mp->u.dam_event.event_detail & bit(24)) fprintf(out_fd, "PP7 ");
	    if (mp->u.dam_event.event_detail & bit(23)) fprintf(out_fd, "CIC ");
	    if (mp->u.dam_event.event_detail & bit(22)) fprintf(out_fd, "REM ");
	    if (mp->u.dam_event.event_detail & bit(20)) fprintf(out_fd, "SRQ ");
	    if (mp->u.dam_event.event_detail & bit(19)) fprintf(out_fd, "TLK ");
	    if (mp->u.dam_event.event_detail & bit(18)) fprintf(out_fd, "LTN ");
	    if (mp->u.dam_event.event_detail & bit(17)) fprintf(out_fd, "DAT ");
	    if (mp->u.dam_event.event_detail & bit(16)) fprintf(out_fd, "NCC ");
	    if (mp->u.dam_event.event_detail & bit(15)) fprintf(out_fd, "MSA ");
	    if (mp->u.dam_event.event_detail & bit(14)) fprintf(out_fd, "PTY ");
	    if (mp->u.dam_event.event_detail & bit(13)) fprintf(out_fd, "GET ");
	    if (mp->u.dam_event.event_detail & bit( 9)) fprintf(out_fd, "IFC ");
	    if (mp->u.dam_event.event_detail & bit( 8)) fprintf(out_fd, "DCL ");
	    fprintf(out_fd, "\n");
	    break;
	    
	case HPIB_DUMP_REQ_MSG:
	    fprintf(out_fd, "HPIB_DUMP_REQ_MSG\n");
	    an_hpib_decode_req_parm((req_parm_type *)&mp->u, prefix, out_fd);
	    break;

	default:
	    fprintf(out_fd, "unknown)\n");
	    break;
    }
}

an_hpib_decode_req_parm(r, prefix, out_fd)
    req_parm_type *r;
    char          *prefix;
    FILE          *out_fd;
{
    fprintf(out_fd, "%scmd_count    = %5d,", prefix, r->cmd_count);
    fprintf(out_fd, " cmd_ptr    = %d.0x%08x %s\n",
		r->cmd_ptr.u.data_sid, r->cmd_ptr.u.data_vba,
		addr_class(r->cmd_class));
    an_hpib_dumpv(r->cmd_ptr.u.data_sid, r->cmd_ptr.u.data_vba, r->cmd_count,
		prefix, out_fd);

    fprintf(out_fd, "%sdata_count   = %5d,", prefix, r->data_count);
    fprintf(out_fd, " data_ptr   = %d.0x%08x %s\n",
		r->data_ptr.u.data_sid, r->data_ptr.u.data_vba,
        	addr_class(r->data_class));
    an_hpib_dumpv(r->data_ptr.u.data_sid, r->data_ptr.u.data_vba,
		r->data_count, prefix, out_fd);

    fprintf(out_fd, "%sstatus_count = %5d,", prefix, r->status_count);
    fprintf(out_fd, " status_ptr = %d.0x%08x %s\n",
		r->status_ptr.u.data_sid, r->status_ptr.u.data_vba,
		addr_class(r->status_class));
    an_hpib_dumpv(r->status_ptr.u.data_sid, r->status_ptr.u.data_vba,
		r->status_count, prefix, out_fd);
}

an_hpib_dumpv(sid, vba, count, prefix, out_fd)
    int       sid;
    anyptr    vba;
    int       count;
    char     *prefix;
    FILE     *out_fd;
{
    char buf[64];
    int i;

    if (count > 0) {
	if (an_grab_virt_chunk(sid, (unsigned)vba, buf,
		min(sizeof(buf), count)) == 0) {
    	    for (i = 0; i < min(sizeof(buf), count); i++) {
		if ((i % 16) == 0) fprintf(out_fd, "%s", prefix);
		fprintf(out_fd, "%02x ", buf[i] & 0xff);
		if ((i % 16) == 15) fprintf(out_fd, "\n");
    	    }
	    if ((i % 16) != 0) fprintf(out_fd, "\n");
	}
	else
	    fprintf(out_fd, "%sdoes not point to useful data\n", prefix);
    }
}

/*
 * Interpretation of commands and status for the HP27110B HP-IB CIO card.
 * Taken from decode program.
 *
 * TODO: This should be in a separate file.
 */

#define SIZEOF(x)	(sizeof(x)/sizeof(x[0]))
#define bit16(addr)	((   *(addr)  <<  8) |  *((addr)+1))
#define bit24(addr)	((bit16(addr) <<  8) |  *((addr)+2))
#define bit32(addr)	((bit16(addr) << 16) | bit16((addr)+2))

static struct {
    char *descript;
    int   bytes;
} fcodes[] = {
    {  "Reserved",					0    },
    {  "Assert Remote Enable",				0    },
    {  "Deassert Remote Enable",			0    },
    {  "Assert Interface Clear",			0    },
    {  "Configure card response to Parallel Poll",	1    },
    {  "Read card response to Parallel Poll",		0    },
    {  "Configure card SRQ response to",		1    },
    {  "Read reason for interrupt",			0    },
    {  "Write interrupt mask",				3    },
    {  "Read interrupt mask",				0    },
    {  "Wait for interrupts",				3    },
    {  "Go online",					0    },
    {  "Go offline",					0    },
    {  "Assert Attention",				0    },
    {  "Deassert Attention",				0    },
    {  "Configure card interpretation of bus Parallel Poll sense", 2    },
    {  "Conduct Parallel Poll",				0    },
    {  "Configure HP-IB address to ",			1    },
    {  "Read HP-IB address and current status",		0    },
    {  "Configure card Amigo Identify response to",	2    },
    {  "Read last secondary address",			0    },
    {  "Enable freeze on interface message parity error", 0    },
    {  "Disable freeze on interface message parity error", 0    },
    {  "Loopback",					1    },
    {  "Microprocessor RAM access diagnostic",		2    },
    {  "External memory access diagnostic",		3    },
    {  "Enable miscellaneous interrupts mask",		3    },
    {  "Disable miscellaneous interrupts mask",		3    },
    {  "Perform Amigo Identify on address",		1    },
    {  "Go online, assert Interface Clear, assert Remote Enable", 0  },
    {  "Enable unsolicited Parallel Poll response detection on address", 1    },
    {  "Disable unsolicited Parallel Poll response detection on address",1    }
};

static char *operation[] = {
	"operation", "noop", "write", "read"
};

static char *crc_opt[] = {
	"(no CRC)", "(no CRC)", "(CRC write)", "(CRC read)"
};

/***********************************************************/
/* This function interprets command sequences on the 27110 */
/***********************************************************/

an_27110_commands(fcode, count, b, prefix, out_fd)
    unsigned char  fcode;
    int  	   count;
    unsigned char *b;
    char	  *prefix;
    FILE          *out_fd;
{
    int  i;

    fprintf(out_fd, "%s", prefix);
    switch (fcode >> 5) {
	case 0:					/* General transactions */
	    fprintf(out_fd, "%s", fcodes[fcode].descript);
	    if (count > 0 && fcodes[fcode].bytes > 0) {
		fprintf(out_fd, " 0x");
		for (i = 0; i < fcodes[fcode].bytes; i++)
		    fprintf(out_fd, "%.2x", b[i]);
	    }
	    break;

	case 2:			 	/* DAP's and CS/80 transactions	*/
	    if (fcode == 0x50)
		fprintf(out_fd, "DAP download for address %d, length 0x%x",
			b[0], bit16(&b[1]));
	    else if (fcode == 0x58) {
		fprintf(out_fd, "DAP execute for address %d", b[0]);
		if (count > 1) {
		    fprintf(out_fd, ", parameters 0x");
		    for (i = 1; i < count; i++) fprintf(out_fd, "%.2x", b[i]);
		}
	    }
	    else {
	    	fprintf(out_fd, "Concurrent CS/80 %s %s",
		    operation[(fcode & 0x0C) >> 2], crc_opt[fcode & 0x03]);
	    	fprintf(out_fd, " with %d-second timeout\n", bit16(&b[1]));
	    	if (count > 3)
		    an_cs80_commands(&b[3], count - 3, prefix, out_fd);
	    }
	    break;

	case 3:					/* more CS/80 transactions */
	    fprintf(out_fd, "Transparent concurrent CS/80 %s %s", 
	    	operation[(fcode & 0x0C) >> 2], crc_opt[fcode & 0x03]);
	    fprintf(out_fd, " with %d-second timeout\n", bit16(&b[1]));
	    if (count > 3)
		an_cs80_commands(&b[3], count - 3, prefix, out_fd);
	    

	case 4:					/* CS/80 transactions	*/
	    fprintf(out_fd, "Nonconcurrent CS/80 %s %s", 
	    	operation[(fcode & 0x0C) >> 2], crc_opt[fcode & 0x03]);
	    fprintf(out_fd, " with secondary 0x%x\n", b[6]);
	    if (count > 7)
		an_cs80_commands(&b[7], count - 7, prefix, out_fd);
	    break;

	case 5:					/* 7970E transactions	*/
	    fprintf(out_fd, "HP7970 Magnetic Tape %s of length %d %s", 
	    	operation[(fcode & 0x0C) >> 2], 
		(bit16(&b[1]) << 16) | bit16(&b[3]),
		crc_opt[fcode & 0x03]);
	    fprintf(out_fd, " for address %d with secondary 0x%x", b[5],b[6]);
	    break;

	case 6:					/* HP-IB transactions	*/
	    fprintf(out_fd, "Generic HP-IB %s of length %d %s\n",
	    	operation[(fcode & 0x0C) >> 2],
		(bit16(&b[1]) << 16) + bit16(&b[3]),
		crc_opt[fcode & 0x03]);
	    if (count > 7) an_hpib_commands(&b[7], count - 7, prefix, out_fd);
	    break;

	default:				/* Reserved		*/
	    fprintf(out_fd, "Reserved");
    }
    fprintf(out_fd, "\n");
}

static char *tstat_table[] = {
    /*  0 */ "No exceptional conditions",
    /*  1 */ "Read operation terminated on EOI with an even count",
    /*  2 */ "Read operation terminated on EOI with an odd count",
    /*  3 */ "Read operation terminated on count with an even count",
    /*  4 */ "Read operation terminated on count with an odd count",
    /*  5 */ "Read operation terminated on receipt of LF with an even count",
    /*  6 */ "Read operation terminated on receipt of LF with an odd count",
    /*  7 */ "Read operation terminated on receipt of MSA with an even count",
    /*  8 */ "Read operation terminated on receipt of MSA with an odd count",
    /*  9 */ "Transfer terminated by assertion of CEND",
    /*  A */ "Reserved",
    /*  B */ "Card is not System Controller and should be",
    /*  C */ "Card is not Controller in Charge and should be",
    /*  D */ "Card is Controller in Charge and should not be",
    /*  E */ "Card was not addressed",
    /*  F */ "CIO protocol error",
    /* 10 */ "DCL detected",
    /* 11 */ "Transfer terminated by assertion of IFC",
    /* 12 */ "HPIB chip on card failed",
    /* 13 */ "HPIB chip on card generated illegal DMA interrupt",
    /* 14 */ "Outbound data frozen by inbound data",
    /* 15 */ "Data error on loopback",
    /* 16 */ "Reserved",
    /* 17 */ "Cannot drop ATN while parallel poll interrupt enabled",
    /* 18 */ "Reserved",
    /* 19 */ "Reserved",
    /* 1A */ "Reserved",
    /* 1B */ "Reserved",
    /* 1C */ "Reserved",
    /* 1D */ "Reserved",
    /* 1E */ "Reserved",
    /* 1F */ "Reserved",
    /* 20 */ "No CS/80 command buffer",
    /* 21 */ "Transfer terminated on EOI & count with an even count",
    /* 22 */ "Transfer terminated on EOI & count with an odd count",
    /* 23 */ "Transfer terminated on EOI & LF with an even count",
    /* 24 */ "Transfer terminated on EOI & LF with an odd count",
    /* 25 */ "Transfer terminated on EOI & pattern match with an even count",
    /* 26 */ "Transfer terminated on EOI & pattern match with an odd count",
    /* 27 */ "Transfer terminated on pattern match with an even count",
    /* 28 */ "Transfer terminated on pattern match with an odd count",
    /* 29 */ "Write operation terminated by inbound data",
    /* 2A */ "Transfer terminated on receipt of DCL or SDC with an even count",
    /* 2B */ "Transfer terminated on receipt of DCL or SDC with an odd count",
    /* 2C */ "Reserved",
    /* 2D */ "Device adapter program download error",
    /* 2E */ "Reserved",
    /* 2F */ "No device adapter program downloaded",
    /* 30 */ "CS/80 timeout during command phase",
    /* 31 */ "CS/80 timeout during command phase; card is no longer CIC",
    /* 32 */ "CS/80 timeout during command phase; device is still addressed",
    /* 33 */ "Reserved",
    /* 34 */ "CS/80 timeout during execution phase",
    /* 35 */ "CS/80 timeout during execution phase; card is no longer CIC",
    /* 36 */ "CS/80 timeout during execution phase; device is still addressed",
    /* 37 */ "Reserved",
    /* 38 */ "CS/80 timeout during reporting phase",
    /* 39 */ "CS/80 timeout during reporting phase; card is no longer CIC",
    /* 3A */ "CS/80 timeout during reporting phase; device is still addressed",
    /* 3B */ "Reserved",
    /* 3C */ "CS/80 timeout during detailed report",
    /* 3D */ "CS/80 timeout during detailed report; card is no longer CIC",
    /* 3E */ "CS/80 timeout during detailed report; device is still addressed",
    /* 3F */ "Reserved",
    /* 40 */ "Amigo Identify timeout",
    /* 41 */ "Illegal device adapter program opcode",
    /* 42 */ "Attempt to branch out of a device adapter program",
    /* 43 */ "Device locked during error report",
};

/************************************************/
/* This function interprets status on the 27110 */
/************************************************/

an_27110_status(tstat, prefix, out_fd)
    unsigned char  tstat;
    char          *prefix;
    FILE          *out_fd;
{
    fprintf(out_fd, "%s", prefix);
    if (tstat < SIZEOF(tstat_table))
	fprintf(out_fd, "[%s]\n", tstat_table[tstat]);
    else if (tstat == 0xFE)
	fprintf(out_fd, "[tstat not available]\n");
    else 
	fprintf(out_fd, "[invalid tstat]\n");
}

/*
 * Interpretation of CS/80 commands and status
 *
 * TODO: put this is a separate file.
 */

#include <memory.h>

#define bit(x)	(1 << (x))

struct cs80_report {
    unsigned char qstat;
    unsigned char id[2];
    unsigned char reject[2];
    unsigned char fault[2];
    unsigned char access[2];
    unsigned char info[2];
    unsigned char parms[10];
    unsigned char qstat2;
};

/*****************************************/
/* This function interprets CS/80 status */
/*****************************************/

#define P(S)	fprintf(out, "%s   %s\n", prefix, S)

/*ARGSUSED*/
an_cs80_status(b, prefix, out)
    unsigned char *b;
    char          *prefix;
    FILE          *out;
{
    struct cs80_report s;
    int i;

    (void)memcpy((char *)&s, (char *)b, sizeof(s));

    fprintf(out, "%sCS/80 qstat:    0x%02x - ", prefix, s.qstat);
    switch (s.qstat) {
	case  0: fputs("No error\n", out);		break;
	case  1: fputs("Hardware error\n", out);	break;
	case  2: fputs("Device power on\n", out);	break;
	default: fputs("Unrecognized qstat\n", out);	break;
    }

    fprintf(out, "%sIdentification: 0x%02x - ", prefix, bit16(s.id));
    if (s.id[1] == 0xff)
	fputs("No units have status\n", out);
    else {
	fprintf(out, "Volume 0x%x, ", s.id[0] >> 4);
	fprintf(out, "Unit 0x%x, ",   s.id[0] & 0xf);
	fprintf(out, "Unit w/status 0x%x\n", s.id[1]);
    }

    fprintf(out, "%sReject errors:  0x%02x\n", prefix, bit16(s.reject));
    if (s.reject[1] & bit(3)) P("Illegal execution msg length");
    if (s.reject[1] & bit(4)) P("Command sequence error");
    if (s.reject[1] & bit(5)) P("Incorrect message sequence");
    if (s.reject[1] & bit(6)) P("Illegal parameter");
    if (s.reject[1] & bit(7)) P("Parameter bounds");
    if (s.reject[0] & bit(0)) P("Address bounds");
    if (s.reject[0] & bit(1)) P("Module addressing");
    if (s.reject[0] & bit(2)) P("Transport service error");
    if (s.reject[0] & bit(2)) P("Illegal opcode");
    if (s.reject[0] & bit(5)) P("Channel parity error");

    fprintf(out, "%sFault errors:   0x%02x\n", prefix, bit16(s.fault));
    if (s.fault[1] & bit(0)) P("Retransmit");
    if (s.fault[1] & bit(1)) P("Powerfail");
    if (s.fault[1] & bit(3)) P("Release requested: Internal maintenance");
    if (s.fault[1] & bit(4)) P("Release requested: Diagnostic request");
    if (s.fault[1] & bit(5)) P("Release requested: Operator request");
    if (s.fault[1] & bit(7)) P("Diagnostic result (see params)");
    if (s.fault[0] & bit(1)) P("Unit fault");
    if (s.fault[0] & bit(4)) P("Controller fault");
    if (s.fault[0] & bit(6)) P("Copy data error (see params)");

    fprintf(out, "%sAccess errors:  0x%02x\n", prefix, bit16(s.access));
    if (s.access[1] & bit(3)) P("End of volume");
    if (s.access[1] & bit(4)) P("End of file");
    if (s.access[1] & bit(6)) P("Unrecoverable data (see parameters)");
    if (s.access[1] & bit(7)) P("Unrecoverable data overflow");
    if (s.access[0] & bit(2)) P("No data found");
    if (s.access[0] & bit(3)) P("Write protect");
    if (s.access[0] & bit(4)) P("Not ready");
    if (s.access[0] & bit(5)) P("No spares avalilable");
    if (s.access[0] & bit(6)) P("Uninitialized media");
    if (s.access[0] & bit(7)) P("Illegal parallel operation");

    fprintf(out, "%sInfo errors:    0x%02x\n", prefix, bit16(s.info));
    if (s.info[1] & bit(2)) P("Maintenance track overflow");
    if (s.info[1] & bit(4)) P("Recoverable data (see params)");
    if (s.info[1] & bit(5)) P("Marginal data (see params)");
    if (s.info[1] & bit(6)) P("Recoverable data overflow");
    if (s.info[0] & bit(0)) P("Auto-sparing invoked");
    if (s.info[0] & bit(3)) P("Latency induced");
    if (s.info[0] & bit(4)) P("Media wear");
    if (s.info[0] & bit(5)) P("Release req'd: Internal maintenance (see params)");
    if (s.info[0] & bit(6)) P("Release req'd: Diagnostic request (see params)");
    if (s.info[0] & bit(7)) P("Release req'd: Operator request (see params)");

    fprintf(out, "%sParameters:     ", prefix);
    for (i = 0; i < 10; i++) fprintf(out, "%.2x", s.parms[i]);
    fputc('\n', out);
}

/*******************************************/
/* This function interprets CS/80 commands */
/*******************************************/

#define LOCATENREAD     0x00	/* locate and read	*/
#define LOCATENWRITE    0x02	/* locate and write	*/
#define LOCATENVERIFY   0x04	/* locate and verify	*/
#define SPAREBLOCK      0x06	/* spare block		*/
#define COPY_DATA       0x08	/* copy data		*/
#define	COLDLOADREAD	0x0a	/* cold load read	*/
#define REQSTATUS	0x0d	/* request status	*/
#define RELEASE         0x0e    /* release device	*/
#define RELEASEDENIED   0x0f    /* release denied	*/
#define SETADDRESS	0x10	/* set address		*/
#define SETBLOCK	0x12	/* set block displacemt	*/
#define SETLENGTH	0x18	/* set length		*/
#define SETUNIT0	0x20	/* set unit 0		*/
#define UTILITYINIT	0x30	/* initiate utility	*/
#define DIAGINIT	0x33	/* initiate diagnostic	*/
#define NOOP     	0x34	/* no op		*/
#define DESCRIBE 	0x35	/* describe		*/
#define MEDIAINIT 	0x37	/* initialize media	*/
#define SETOPTION	0x38	/* set options		*/
#define SETRPS		0x39	/* set rps		*/
#define SETRETRY	0x3a	/* set retry time	*/
#define SETRELEASE	0x3b	/* set release		*/
#define SETBURST	0x3c	/* set burst		*/
#define SETMASK		0x3e	/* set status mask	*/
#define SETVOLUME0	0x40	/* set volume 0		*/
#define SETRTNMODE	0x48	/* set return addr mode	*/
#define WRITEMARK	0x49	/* write file mark	*/
#define UNLOAD          0x4a    /* unload		*/
#define CS80LOAD        0x4b    /* load			*/

an_cs80_commands(s, count, prefix, out)
    unsigned char *s;
    int   	   count;
    char          *prefix;
    FILE          *out;
{
    int i, j;

    for (i = 0; i < count; i++, s++) {
	fprintf(out, "%s0x%02x", prefix, *s);
	switch (*s) {
	    case LOCATENREAD:	fputs(" - Locate and read", out);	break;
	    case LOCATENWRITE:	fputs(" - Locate and write", out);	break;
	    case LOCATENVERIFY:	fputs(" - Locate and verify", out);	break;
	    case COLDLOADREAD:	fputs(" - Cold load read", out);	break;
	    case REQSTATUS:	fputs(" - Request status", out);	break;
	    case RELEASE:	fputs(" - Release", out);		break;
	    case RELEASEDENIED:	fputs(" - Release denied", out);	break;
	    case SETUNIT0:
	    case SETUNIT0+1:
	    case SETUNIT0+2:
	    case SETUNIT0+3:
	    case SETUNIT0+4:
	    case SETUNIT0+5:
	    case SETUNIT0+6:
	    case SETUNIT0+7:
	    case SETUNIT0+8:
	    case SETUNIT0+9:
	    case SETUNIT0+10:
	    case SETUNIT0+11:
	    case SETUNIT0+12:
	    case SETUNIT0+13:
	    case SETUNIT0+14:
	    case SETUNIT0+15:	fprintf(out, " - Set unit %d", *s&0xf);	break;
	    case NOOP:		fputs(" - Noop", out);			break;
	    case DESCRIBE:	fputs(" - Describe", out);		break;
	    case SETVOLUME0:
	    case SETVOLUME0+1:
	    case SETVOLUME0+2:
	    case SETVOLUME0+3:
	    case SETVOLUME0+4:
	    case SETVOLUME0+5:
	    case SETVOLUME0+6:
	    case SETVOLUME0+7:	fprintf(out, " - Set volume %d", *s&0x7);break;
	    case WRITEMARK:	fputs(" - Write file mark",out );	break;
	    case UNLOAD:	fputs(" - Unload", out);		break;

	    case SPAREBLOCK:
		fprintf(out, "%02x - Spare block", s[1]);
		fprintf(out, " (%s spare, %sretain data)", 
		    (s[1] & 4) ? "jump" : "skip", (s[1] & 1) ? "do not " : "");
		s++, i++;
		break;

	    case COPY_DATA:
		for (j = 0; j < 16; j++) fprintf(out, "%.2x", s[j+1]);
		fprintf(out, " - Copy data\n%sfrom unit %d, vol %d, ", prefix,
			(s[1] & 0x70) >> 4, s[1] & 7);
		an_cs80_address(s+2, out);
		s += 8;
		fprintf(out, "\n%sto unit %d, vol %d, ", prefix,
		    (s[1] & 0x70) >> 4, s[1] & 7);
		an_cs80_address(s+2, out);
		s += 8, i += 16;
		break;

	    case SETADDRESS:
	    case SETADDRESS+1:
		fprintf(out, "%02x%02x%02x%02x%02x%02x - Set address to ",
			s[1], s[2], s[3], s[4], s[5], s[6]);
		an_cs80_address(s, out);
		s += 6; i += 6;
		break;

	    case SETBLOCK:
		fprintf(out, "%02x%02x%02x%02x%02x%02x - Set block displacement 0x",
			s[1], s[2], s[3], s[4], s[5], s[6]);
		if (s[1] || s[2])
		    fprintf(out, "%02x%02x%02x%02x%02x%02x",
			s[1], s[2], s[3], s[4], s[5], s[6]);
		else
		    fprintf(out, "%x", bit32(&s[3]));
		s += 6; i += 6;
		break;

	    case SETLENGTH:
		fprintf(out, "%02x%02x%02x%02x - Set length to 0x%x",
			s[1], s[2], s[3], s[4], bit32(&s[1]));
		s += 4, i += 4;
		break;

	    case UTILITYINIT:
	    case UTILITYINIT+1:
	    case UTILITYINIT+2:
		for (j = 1; j < count-1-i; j++) fprintf(out, "%.2x", s[j]);
		fprintf(out, " - Initiate device-dependent utility 0x%02x",
			s[1]); 
		s += j-1, i += j-1;	/* Untested - XXX */
		break;

	    case DIAGINIT:
		fprintf(out, "%02x%02x%02x - Initiate diagnostic 0x%02x %d times",
			s[3], bit16(&s[1]));
		s += 3, i += 3;
		break;

	    case MEDIAINIT:
		fprintf(out, "%02x%02x - Initialize media (Options 0x%02x%02x)",
			s[1], s[2], s[1], s[2]);
		s += 2, i += 2;
		break;

	    case SETOPTION:
		fprintf(out, "%02x - Set options (", s[1]);
		fprintf(out, "%sable auto-sparing, %s spare, %sable char count)",
			(s[1] & 4) ? "En" : "Dis", (s[1] & 2) ? "skip" : "jump",
			(s[1] & 1) ? "en" : "dis");
		s++, i++;
		break;

	    case SETRPS:
		fprintf(out, "%02x%02x - Set RPS", s[1], s[2]);
		fprintf(out, " (time-to-target = %d us, window-size = %d us)",
			s[1] * 100, s[2] * 100);
		s += 2, i += 2;
		break;

	    case SETRETRY:
		fprintf(out, "%02x%02x - Set retry time to %d ms", s[1], s[2],
			bit16(&s[1]) * 10);
		s += 2, i += 2;
		break;

	    case SETRELEASE:
		fprintf(out, "%02x - Set release (", s[1]);
		if (s[1] & 0x80) fputs("suppress timeout", out);
		if (s[1] & 0x40) fputs("autorelease while idle", out);
		fputc(')', out);
		s++, i++;
		break;

	    case SETBURST:
	    case SETBURST+1:
		fprintf(out, "%02x - ", s[1]);
		if (s[1] == 0)
		    fputs("Deactivate burst mode", out);
		else
		    fprintf(out, "Set burst size 0x%x (EOI %s)",
			s[1], (*s & 1) ? "always" : "on last burst");
		s++, i++;
		break;

	    case SETMASK:
		fprintf(out, "%02x%02x%02x%02x%02x%02x%02x%02x - ",
		    s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8]);
		fprintf(out, "Set status mask to 0x%02x%02x%02x%02x%02x%02x%02x%02x",
		    s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8]);
		s += 8, i+= 8;
		break;

	    case SETRTNMODE:
		fprintf(out, "%02x - Set return addressing to %svector mode",
		    s[1], (s[1] & 1) ? "3-" : "single ");
		s++, i++;
		break;

	    case CS80LOAD:
		fprintf(out, "%02x%02x - Load cartridge %d", s[1], s[2], s[2]);
		s += 2, i += 2;
		break;

	    default:
		fputs(" - Unknown CS/80 command", out);
	}
    	fputc('\n', out);
    }
}

an_cs80_address(s, out)
    unsigned char *s;
    FILE *out;
{
    if (s[0] & 1) { /* 3-vector address */
	fprintf(out, "cylinder 0x%x, head 0x%x, sector 0x%x",
	    bit24(&s[1]), s[4], bit16(&s[5]));
    }
    else {
	if (s[1] || s[2])
	    fprintf(out, "block 0x%02x%02x%02x%02x%02x%02x",
		s[1], s[2], s[3], s[4], s[5], s[6]);
	else
	    fprintf(out, "block 0x%x", bit32(&s[3]));
    }
}

/*
 * Interpretation of HP-IB commands
 *
 * TODO: put this in a separate file.
 */

an_hpib_commands(s, count, prefix, out)
    unsigned char *s;
    int   	   count;
    char          *prefix;
    FILE          *out;
{
    int i, addr;
    int ppc = 0;	/* boolean -- have we seen PPC? */

    for (i = 0; i < count; i++, s++) {
	fprintf(out, "%s0x%02x - ", prefix, *s);
	switch(*s) {
	    case 0x01: fputs("Go to Local", out);		break;
	    case 0x04: fputs("Selective Device Clear", out);	break;
	    case 0x05: fputs("Parallel Poll Configure", out);	ppc = 1; break;
	    case 0x08: fputs("Group Execute Trigger", out);	break;
	    case 0x09: fputs("Take Control", out);		break;
	    case 0x11: fputs("Local Lockout", out);		break;
	    case 0x14: fputs("Device Clear", out);		break;
	    case 0x15: fputs("Parallel Poll Unconfigure", out); ppc = 0; break;
	    case 0x18: fputs("Serial Poll Enable", out);	break;
	    case 0x19: fputs("Serial Poll Disable", out);	break;
	    default:
		addr = *s & 0x1f;
		if ((*s & 0x60) == 0x40) {	/* Talk address */
		    fprintf(out, "Talk %d", addr);
	    	    if (addr == 30) fputs(" (Talk CIC)", out);
	    	    if (addr == 31) fputs(" (Untalk)", out);
		}
		else if ((*s & 0x60) == 0x20) {	/* Listen address */
		    fprintf(out, "Listen %d", addr);
	    	    if (addr == 30) fputs(" (Listen CIC)", out);
	    	    if (addr == 31) fputs(" (Unlisten)", out);
		}
		else if ((*s & 0x60) == 0x60) {	/* Secondary address */
		    fprintf(out, "Secondary %d", addr);
		    if (ppc) {
			if (addr & 0x10)
			    fputs(" (Parallel Poll Disable)", out);
			else
			    fprintf(out, " (Parallel Poll Enable sense %d, addr %d)",
				    (addr >> 3) & 1, addr & 7);
		    }
		}
		else
		    fputs("Unrecognized command", out);
		break;
	}
	fputc('\n', out);
    }
}
