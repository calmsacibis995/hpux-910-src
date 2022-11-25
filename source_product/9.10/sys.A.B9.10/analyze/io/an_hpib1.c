/***************************************************************************/
/**                                                                       **/
/**   This is the I/O manager analysis routine for hpib1.                 **/
/**                                                                       **/
/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sio/llio.h>
#include <h/types.h>
#include <machine/cpu.h>
#include <sio/dma_A.h>
#include <sio/iotree.h>
#define	DEBUG
#include <sio/hpib1.h>			/* hpib1's normal .h file	      */
#undef	DEBUG
#include "aio.h"			/* all analyze i/o defs		      */

int	hpib1_an_rev = HPIB1_REV_CODE;	/* global -- rev number of hpib1 DS   */
static char *me      = "hpib1";

#define	BOOLEAN(s)	((s) ? "true" : "false")
#define bit(x)  	(1 << (x))
#define	streq(a, b)	(strcmp((a), (b)) == 0)
hpib_tran_type *an_hpib1_trans();

int
an_hpib1(call_type, port_num, out_fd, option_string)
    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */
{
    int    		addr;		/* address of extern                  */
    int			code_rev;	/* revision of driver code	      */
    hpib_pda_type	pda;		/* port data area                     */
    struct mgr_info_type mgr;		/* all generic manager info	      */

    extern int		an_hpib_decode_message();
    extern int		an_hpib_decode_status();

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
	    if (an_grab_extern("hpib1_an_rev", &addr) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, 0, hpib1_an_rev, out_fd);
	    else if (an_grab_real_chunk(addr, &code_rev, sizeof(code_rev)) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, 0, hpib1_an_rev, out_fd);
	    else if (code_rev != hpib1_an_rev)
		aio_rev_mismatch(me, AIO_INCOMPAT_REVS, code_rev, hpib1_an_rev,
			out_fd);

	    /* put my decoding routines into system lists        */
	    aio_init_llio_status_decode(SUBSYS_HPIB1, an_hpib_decode_status);
	    aio_init_message_decode(START_HPIB_MSG, START_HPIB_MSG+9,
	                            an_hpib_decode_message);
	    break;

	case AN_MGR_BASIC:
	    an_hpib1_all(&pda, out_fd);
	    break;

	case AN_MGR_DETAIL:
	    an_hpib1_all(&pda, out_fd);
	    break;

	case AN_MGR_OPTIONAL:
	    an_hpib1_optional(&pda, out_fd, option_string);
	    break;

	case AN_MGR_HELP:
	    an_hpib1_help(stderr);
	    break;

    }
    return(0);
}

an_hpib1_optional(pda, out_fd, option_string)
    hpib_pda_type	*pda;
    FILE		*out_fd;
    char		*option_string;
{
    char *token;
    int	 dam_flag	= 0;
    int	 da_flag	= 0;
    int	 dm_flag	= 0;
    int	 queue_flag	= 0;
    int	 trans_flag	= 0;
    int	 all_flag	= 0;

    /* read through option string and set appropriate flags */
    token = strtok(option_string, " ");

    do {
	if      (streq(token,    "dam"))   dam_flag   = 1;
	else if (streq(token,     "da"))   da_flag    = 1;
	else if (streq(token,     "dm"))   dm_flag    = 1;
	else if (streq(token, "queued"))   queue_flag = 1;
	else if (streq(token,  "trans"))   trans_flag = 1;
	else if (streq(token,    "all"))   all_flag   = 1;
	else  {
	    fprintf(stderr, "invalid option <%s>\n", token);
	    an_hpib1_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (dam_flag)   an_hpib1_dam_info(pda, out_fd);
    if (da_flag)    an_hpib1_da_info(pda, out_fd);
    if (dm_flag)    an_hpib1_dm_info(pda, out_fd);
    if (queue_flag) an_hpib1_queued(pda, out_fd);
    if (trans_flag) an_hpib1_trans_info(pda, out_fd);
    if (all_flag)   an_hpib1_all(pda, out_fd);
}

an_hpib1_dam_info(p, out_fd)
    hpib_pda_type	*p;
    FILE		*out_fd;
{
    int i;

    fprintf(out_fd, "\n===========DAM INFO===========\n");
    for (i = 0; i < MAX_CS80_DEVICES; i++) {
	llio_std_header_type *l = &p->cs80_timer_msg[i];
	struct mgr_info_type mgr;

	fprintf(out_fd, "cs80_device_busy[%d]  = %s\n", i,
		BOOLEAN(p->cs80_device_busy[i]));

	if (p->cs80_device_busy[i]) {
	    fprintf(out_fd, "cs80_timer_msg[%d]:\n", i);
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
	}
    }

    fprintf(out_fd, "port_bound           = %5s\n", BOOLEAN(p->port_bound));
    fprintf(out_fd, "reset_during_lock    = %5s\n", BOOLEAN(p->reset_during_lock));
    fprintf(out_fd, "in_pon_cycle         = %5s\n", BOOLEAN(p->in_pon_cycle));
    fprintf(out_fd, "power_on_trn_num     = 0x%08x\n", p->power_on_trn_num);
    fprintf(out_fd, "power_on_msg_id      = 0x%04x\n", p->power_on_msg_id);
    fprintf(out_fd, "power_on_port_num    = %d\n",   p->power_on_port_num);
    fprintf(out_fd, "nested_pon           = %5s\n", BOOLEAN(p->nested_pon));
    fprintf(out_fd, "nested_pon_trn_num   = 0x%08x\n", p->nested_pon_trn_num);
    fprintf(out_fd, "nested_pon_msg_id    = 0x%04x\n", p->nested_pon_msg_id);
    fprintf(out_fd, "nested_pon_port_num  = %d\n",   p->nested_pon_port_num);

    fprintf(out_fd, "port_locked          = %5s\n", BOOLEAN(p->port_locked));
    fprintf(out_fd, "port_being_locked    = %5s\n",
	BOOLEAN(p->port_being_locked));
    fprintf(out_fd, "doing_diagnostics    = %5s\n",
	BOOLEAN(p->doing_diagnostics));
    fprintf(out_fd, "lock_tptr            = 0x%08x\n", p->lock_tptr);
    if (p->lock_tptr) (void)an_hpib1_trans(p->lock_tptr, out_fd);

    fprintf(out_fd, "queued_pf_aborts     = 0x%08x\n", p->queued_pf_aborts);
    if (p->queued_pf_aborts) (void)an_hpib1_trans(p->queued_pf_aborts, out_fd);

    fprintf(out_fd, "probe_id             = 0x%02x\n", p->probe_id);
    fprintf(out_fd, "probe_addr           = %d\n",     p->probe_addr);
    fprintf(out_fd, "probe_option         = %d\n",     p->probe_option);

    fprintf(out_fd, "hpa                  = 0x%08x\n", p->hpa);
    fprintf(out_fd, "eim                  = 0x%08x\n", p->eim);
    fprintf(out_fd, "port_num             = %d\n",     p->port_num);
    
    fprintf(out_fd, "bus_trans.owner      = 0x%08x\n", p->bus_trans.owner);
    fprintf(out_fd, "bus_trans.queued     = 0x%08x\n", p->bus_trans.queued);
    fprintf(out_fd, "bus_trans.free_list  = 0x%08x\n", p->bus_trans.free_list);

    fprintf(out_fd, "isr.in_fifo_avail    = 0x%08x ", p->isr.in_fifo_avail);
    an_display_sym(p->isr.in_fifo_avail, out_fd, AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd, "\nisr.out_fifo_idle    = 0x%08x ", p->isr.out_fifo_idle);
    an_display_sym(p->isr.out_fifo_idle, out_fd, AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd, "\nisr.talker           = 0x%08x ", p->isr.talker);
    an_display_sym(p->isr.talker, out_fd, AIO_SYMBOL_AND_OFFSET);

    fprintf(out_fd, "\ndma_mode             = %d -- ",    p->dma_mode);
    switch (p->dma_mode) {
	case DMA_IDLE:	  fprintf(out_fd, "DMA_IDLE\n");	break;
	case DMA_ACTIVE:  fprintf(out_fd, "DMA_ACTIVE\n");	break;
	case DMA_PAUSED:  fprintf(out_fd, "DMA_PAUSED\n");	break;
	default:	  fprintf(out_fd, "unknown mode\n");	break;
    }
    fprintf(out_fd, "granting_lock        = %5s\n", BOOLEAN(p->granting_lock));
    fprintf(out_fd, "lock_grant_timer_id  = %d\n",    p->lock_grant_timer_id);
    fprintf(out_fd, "hw_poll_timer_id     = %d\n",    p->hw_poll_timer_id);

    fprintf(out_fd, "dm_count             = %d\n",    p->dm_count);
    fprintf(out_fd, "dm_pon_count         = %d\n",    p->dm_pon_count);
    fprintf(out_fd, "dm_max_index         = %d\n",    p->dm_max_index);
}

an_hpib1_da_info(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    da_state_type *d = &pda->da_state;
    int i;

    fprintf(out_fd, "\n===========DA STATE===========\n");
    fprintf(out_fd, "delay         = 0x%x\n", d->delay);
    fprintf(out_fd, "io_ctrl_reg   = 0x%x\n", d->io_ctrl_reg);
    fprintf(out_fd, "modes         = 0x%x\n", d->modes);
    fprintf(out_fd, "hpib_addr     = 0x%x\n", d->hpib_addr);
    fprintf(out_fd, "amigo_idy1    = 0x%x\n", d->amigo_idy1);
    fprintf(out_fd, "amigo_idy2    = 0x%x\n", d->amigo_idy2);
    fprintf(out_fd, "ppoll_sense   = 0x%x\n", d->ppoll_sense);
    fprintf(out_fd, "ppoll_int_log = 0x%x\n", d->ppoll_int_log);
    fprintf(out_fd, "misc_int_log  = 0x%x\n", d->misc_int_log);
    fprintf(out_fd, "misc_int_enb  = 0x%x\n", d->misc_int_enable);
    fprintf(out_fd, "ppoll_int_enb = 0x%x\n", d->ppoll_int_enable);
    fprintf(out_fd, "ppoll_wait_ct = 0x%x\n", d->ppoll_wait_cnt);
    for (i = 0; i < HPIB_MAX_PPOLL; i++)
    	fprintf(out_fd, "ppoll_wait[%d] = 0x%x\n", i, d->ppoll_wait[i]);
}

an_hpib1_dm_info(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    int                i;
    hpib_dm_info_type  d;
    struct mgr_info_type mgr;

    fprintf(out_fd, "\n===========DM INFO===========\n");

    for (i = 0; i < MAX_HPIB_DEV_MGRS; i++) {
	if (pda->dm_array[i] == 0) continue;
	if (an_grab_virt_chunk(0, pda->dm_array[i], (char *)&d, sizeof(d)) != 0) {
	    fprintf(stderr, "Couldn't get pda->dm_array[%d] (0x%x)\n", i,
		pda->dm_array[i]);
	    continue;
	}
	fprintf(out_fd, "Index %d:\n", d.dm_index);

	fprintf(out_fd, "  port_num       = %3d ", d.port_num);
	aio_get_mgr_info(AIO_PORT_NUM_TYPE, d.port_num, &mgr);
	fprintf(out_fd, "(%s at %s)\n", mgr.mgr_name, mgr.hw_address);
	fprintf(out_fd, "  reply_subq     = %2d, ", d.bind_info.reply_subq);
	fprintf(out_fd, "hm_event_subq  = %d\n", d.bind_info.hm_event_subq);
	fprintf(out_fd, "  in_pon_cycle   = %s\n", BOOLEAN(d.in_pon_cycle));
	fprintf(out_fd, "  misc_int_enb   = 0x%x\n", d.misc_int_enable);
	fprintf(out_fd, "  ppoll_int_enb  = 0x%x\n", d.ppoll_int_enable);
	fprintf(out_fd, "  unsol_ppoll_enb= 0x%x\n",
		d.unsolicited_ppoll_int_enable);
	fprintf(out_fd, "  misc_int_log   = 0x%x\n", d.misc_int_log);
	fprintf(out_fd, "  ppoll_int_log  = 0x%x\n", d.ppoll_int_log);
    }
}

an_hpib1_trans_info(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    int i;
    hpib_xfr_type *xp = &pda->xfr;
    hpib_tran_type *trp;

    fprintf(out_fd, "\n===========TRANSACTION INFO===========\n");

    fprintf(out_fd, "OWNER            = 0x%08x\n", pda->bus_trans.owner);
    if (pda->bus_trans.owner) (void)an_hpib1_trans(pda->bus_trans.owner, out_fd);

    fprintf(out_fd, "\n===========XFR===========\n");
    fprintf(out_fd, "aligned_buf   = 0x%08x\n", xp->aligned_buf);
    fprintf(out_fd, "buf_cnt       = 0x%08x\n", xp->buf_cnt);
    fprintf(out_fd, "buf_cnt       = 0x%08x\n", xp->buf_cnt);
    fprintf(out_fd, "sid.vaddr     = %d.0x%08x\n", xp->sid, xp->vaddr);
    fprintf(out_fd, "init_cnt      = 0x%08x\n", xp->init_cnt);
    fprintf(out_fd, "count         = 0x%08x\n", xp->count);
    fprintf(out_fd, "s_scratch     = 0x%04x\n", xp->s_scratch);
    fprintf(out_fd, "a_scratch     = %02x", xp->a_scratch[0]);
    for (i = 1; i < 4; i++) fprintf(out_fd, ".%x", xp->a_scratch[i]);
    fprintf(out_fd, "\nuses_intr     = %s\n", BOOLEAN(xp->uses_intr));
    fprintf(out_fd, "dap_eoi_mode  = %s\n", BOOLEAN(xp->dap_eoi_mode));
    fprintf(out_fd, "out_fifo_size = 0x%02x\n", xp->out_fifo_size);
    fprintf(out_fd, "qstat         = 0x%02x\n", xp->qstat);
    fprintf(out_fd, "t_cmd         = 0x%02x\n", xp->t_cmd);
    fprintf(out_fd, "last_sec_addr = 0x%02x\n", xp->last_sec_addr);
    fprintf(out_fd, "rd.end_conds  = 0x%08x\n", xp->ctrl.rd.end_conds);
    fprintf(out_fd, "rd.last_byte  = 0x%08x\n", xp->ctrl.rd.last_byte);
    fprintf(out_fd, "rd.match_byte = 0x%08x\n", xp->ctrl.rd.match_byte);
    fprintf(out_fd, "wr.top_bits   = 0x%08x\n", xp->ctrl.wr.top_bits);
    fprintf(out_fd, "wr.end_top_bit= 0x%08x\n\n", xp->ctrl.wr.end_top_bits);

    fprintf(out_fd, "QUEUED           = 0x%08x\n", trp = pda->bus_trans.queued);
    while (trp) trp = an_hpib1_trans(trp, out_fd);

    fprintf(out_fd, "FREE_LIST        = 0x%08x\n",
	trp = pda->bus_trans.free_list);
    while (trp) trp = an_hpib1_trans(trp, out_fd);
}

an_hpib1_queued(pda, out_fd)
    hpib_pda_type *pda;
    FILE	  *out_fd;
{
    int	i;
    struct mgr_info_type mgr;

    aio_get_mgr_info(AIO_PORT_NUM_TYPE, pda->port_num, &mgr);
    fprintf(out_fd, "\n============SUBQUEUE SUMMARY================\n");
    fprintf(out_fd, "   enabled subqs = 0x%08x, ", mgr.enabled_subqs);
    fprintf(out_fd, "   active subqs  = 0x%08x\n\n", mgr.active_subqs);

    /* walk through subqs and decode */
    for (i = 0; i < 32; i++)  {
	fprintf(out_fd, "   subq %2d:  ", i);

	switch (i) {
	    case HPIB_POWERON_Q:
	    	fprintf(out_fd, "(powerfail)              "); break;

	    case HPIB_LOCK_Q:	/* ABORT & CONFIG, too */
	    	fprintf(out_fd, "(lock/abort/config)      "); break;

	    case HPIB_NON_DEST_DIAG_Q:
	    	fprintf(out_fd, "(diagnostics)            "); break;

	    case HPIB_INT_EVENT_Q:
	    	fprintf(out_fd, "(events)                 "); break;

	    case HPIB_TIMER_Q:
	    	fprintf(out_fd, "(timer events)           "); break;

	    case HPIB_HW_POLL_Q:
	    	fprintf(out_fd, "(h/w polls)              "); break;

	    case HPIB_LOCK_GRANT_Q:
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

an_hpib1_all(pda, out_fd)
    hpib_pda_type	*pda;
    FILE		*out_fd;
{
    an_hpib1_dam_info(pda, out_fd);
    an_hpib1_da_info(pda, out_fd);
    an_hpib1_dm_info(pda, out_fd);
    an_hpib1_queued(pda, out_fd);
    an_hpib1_trans_info(pda, out_fd);
}

an_hpib1_help(out_fd)
    FILE		*out_fd;
{
    fprintf(out_fd, "\nvalid options:\n");
    fprintf(out_fd, "   dam     --  decode device adapter manager state\n");
    fprintf(out_fd, "   da      --  decode device adapter state\n");
    fprintf(out_fd, "   dm      --  decode device manager state\n");
    fprintf(out_fd, "   queued  --  decode queued messages\n");
    fprintf(out_fd, "   trans   --  decode transactions\n");
    fprintf(out_fd, "   all     --  do all of the above\n");
    fprintf(out_fd, "   help    --  print this help screen\n");
}

hpib_tran_type *
an_hpib1_trans(trp, out_fd)
    hpib_tran_type *trp;
    FILE	   *out_fd;
{
    hpib_tran_type trans;

    if (an_grab_virt_chunk(0, trp, (char *)&trans, sizeof(trans)) != 0) {
	fprintf(out_fd, "Not a valid pointer\n");
	return(0);
    }
    trp = &trans;

    fprintf(out_fd, "use_dma          = %5s, ", BOOLEAN(trp->use_dma));
    fprintf(out_fd, "loopback         = %5s\n", BOOLEAN(trp->loopback));
    fprintf(out_fd, "dam_req:\n");
    an_hpib_decode_dam_req(&trp->dam_req, "   ", out_fd);
    fprintf(out_fd, "cmd_residue      = %d\n", trp->cmd_residue);
    fprintf(out_fd, "data_residue     = %d\n", trp->data_residue);
    fprintf(out_fd, "status_residue   = %d\n", trp->status_residue);
    fprintf(out_fd, "reply_status     = ");
    aio_decode_llio_status(trp->reply_status, out_fd);
    fprintf(out_fd, "handler          = 0x%08x ", trp->handler);
    an_display_sym(trp->handler, out_fd, AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd, "\n");
    fprintf(out_fd, "dm_p             = 0x%08x\n", trp->dm_p);
    fprintf(out_fd, "state            = %d -- ", trp->state);
    fprintf(out_fd, "%s\n", h1_tran_states[(int)trp->state]);
    fprintf(out_fd, "link             = 0x%08x\n", trp->link);
    fprintf(out_fd, "msg_ptr          = 0x%08x\n", trp->msg_ptr);
    if (trp->msg_ptr != 0) aio_decode_message(trp->msg_ptr, "      ", out_fd);
    fprintf(out_fd, "prev_state       = %d -- ", trp->prev_state);
    fprintf(out_fd, "%s\n", h1_tran_states[(int)trp->prev_state]);
    fprintf(out_fd, "timer_used       = %5s, ", BOOLEAN(trp->timer_used));
    fprintf(out_fd, "timer_id         = %d\n", trp->timer_id);
    fprintf(out_fd, "dap.p_buf        = 0x%08x\n", trp->dap.p_buf);
    fprintf(out_fd, "dap.s_buf        = 0x%08x\n", trp->dap.s_buf);
    fprintf(out_fd, "dap.pc_ptr       = 0x%08x\n", trp->dap.pc_ptr);
    fprintf(out_fd, "dap.timer_pc_ptr = 0x%08x\n", trp->dap.timer_pc_ptr);
    fprintf(out_fd, "dap.timer_sindex = 0x%08x\n", trp->dap.timer_sindex);

    fprintf(out_fd, "chain            = 0x%08x\n", trp->chain);	/* XXX */
    fprintf(out_fd, "\n");
    return(trp->link);
}
