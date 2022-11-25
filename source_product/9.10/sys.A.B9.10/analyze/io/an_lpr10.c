/***************************************************************************/
/**                                                                       **/
/**   This is the lpr10 (CIPER) i/o manager analysis routine.             **/
/**                                                                       **/
/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sio/llio.h>
#include <h/types.h>
#include <sio/iotree.h>
#include <sio/cpr_def.h> 			/* CIPER def.h file    */
#include <sio/cpr_pda.h> 		       	/* CIPER pda.h file    */
#include "aio.h"			        /* all analyze i/o defs*/

int    lpr10_an_rev  = 1;	/* global -- rev number of lpr10 DS   */
static char *me      = "lpr10";

#define	BOOLEAN(s)	((s) ? "true" : "false")
#define bit(x)  	(1 << (x))
#define	streq(a, b)	(strcmp((a), (b)) == 0)
#define	min(a, b)	(((a) < (b)) ? (a) : (b))
#define	BITS	(BYTE_MODE|CIOCA_S1S2|LOGCH_BREAK|BLOCKED|CONTINUE)

int an_lpr10(call_type, port_num, out_fd, option_string)
    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */
{
    int    		addr;		/* address of extern                  */
    int			code_rev;	/* revision of driver code	      */
    struct cpr_pda      pda;		/* port data area                     */
    struct mgr_info_type mgr;		/* all generic manager info	      */

    int			an_lpr10_decode_message();
    int			an_lpr10_decode_status();

    /* grab the manager information */
    if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, port_num, &mgr) != AIO_OK) {
    	fprintf(stderr,"Problem -- bad port number!\n");
        return(0);
    }

    /* grab the pda */
    if (an_grab_virt_chunk(0, mgr.pda_address, &pda, sizeof(pda)) != 0)  {
        fprintf(stderr, "Couldn't get pda\n");
        return(0);
    }

    /* perform the correct action depending on what call_type is */
    switch (call_type) {

	case AN_MGR_INIT:
	    /* check driver rev against analysis rev -- complain if a problem */
	    if (an_grab_extern("lpr10_an_rev", &addr) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, lpr10_an_rev,
				 out_fd);

            else {
	        if (an_grab_real_chunk(addr, &code_rev, sizeof(code_rev)) != 0)
		   aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, lpr10_an_rev,
				    out_fd);
	        if (code_rev != lpr10_an_rev)
		   aio_rev_mismatch(me, AIO_INCOMPAT_REVS, code_rev, 
				    lpr10_an_rev, out_fd);
            }

	    /* put my decoding routines into system lists        */
	    aio_init_llio_status_decode(SUBSYS_LPR10, an_lpr10_decode_status);
	    aio_init_message_decode(START_DM_MSG, START_DM_MSG+9,
	                            an_lpr10_decode_message);
	    break;

	case AN_MGR_BASIC:
	    an_lpr10_all(&pda, out_fd);
	    break;

	case AN_MGR_DETAIL:
	    an_lpr10_all(&pda, out_fd);
	    break;

	case AN_MGR_OPTIONAL:
	    an_lpr10_optional(&pda, out_fd, option_string);
	    break;

	case AN_MGR_HELP:
	    an_lpr10_help(stderr);
	    break;

    }
    return(0);

} /* an_lpr10 */


an_lpr10_optional(pda, out_fd, option_string)
    struct cpr_pda      *pda;
    FILE		*out_fd;
    char		*option_string;
{
    char *token;
    int	 dam_flag	= 0;
    int	 ldm_flag	= 0;
    int	 dm_flag	= 0;
    int	 all_flag	= 0;

    /* read through option string and set appropriate flags */
    token = strtok(option_string, " ");

    do {
	if      (streq(token,    "dam"))   dam_flag   = 1;
	else if (streq(token,    "ldm"))   ldm_flag   = 1;
	else if (streq(token,     "dm"))   dm_flag    = 1;
	else if (streq(token,    "all"))   all_flag   = 1;
	else  {
	    fprintf(stderr, "invalid option <%s>\n", token);
	    an_lpr10_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (dam_flag)   an_lpr10_dam_info(pda, out_fd);
    if (ldm_flag)   an_lpr10_ldm_info(pda, out_fd);
    if (dm_flag)    an_lpr10_dm_info(pda, out_fd);
    if (all_flag)   an_lpr10_all(pda, out_fd);
}

an_lpr10_dam_info(pda, out_fd)
    struct cpr_pda      *pda;
    FILE		*out_fd;
{
    bind_reply_type     *d = &pda->dam_info;

    fprintf(out_fd, "\n===========DAM INFO===========\n");
    fprintf(out_fd, "lm_rev_code          = %d\n", d->lm_rev_code);
    fprintf(out_fd, "lm_queue_depth       = %d\n", d->lm_queue_depth);
    fprintf(out_fd, "lm_low_req_subq      = %d\n", d->lm_low_req_subq);
    fprintf(out_fd, "lm_hi_req_subq       = %d\n", d->lm_hi_req_subq);
    fprintf(out_fd, "lm_freeze_data       = %s\n", BOOLEAN(d->lm_freeze_data));
    fprintf(out_fd, "lm_alignment         = %s\n", BOOLEAN(d->lm_alignment));
    aio_decode_llio_status(d->reply_status, out_fd);
} /* an_lpr10_dam_info */


an_lpr10_ldm_info(pda, out_fd)
    struct cpr_pda      *pda;
    FILE		*out_fd;
{
    bind_req_type       *d = &pda->ldm_info;

    fprintf(out_fd, "\n==========LDM INFO===========\n");
    fprintf(out_fd, "reply_subq     = %d\n", d->reply_subq);
    fprintf(out_fd, "hm_event_subq  = %d\n", d->hm_event_subq);
    fprintf(out_fd, "hm_subsys_num  = %d\n", d->hm_subsys_num);
    fprintf(out_fd, "hm_meta_lang   = %d\n", d->hm_meta_lang);
    fprintf(out_fd, "hm_rev_code    = %d\n", d->hm_rev_code);
    fprintf(out_fd, "hm_config_addr = %d%d%d\n",
            d->hm_config_addr_3,
            d->hm_config_addr_2,
            d->hm_config_addr_1);
    fprintf(out_fd, "ldm_port_num   = %d\n", pda->ldm_port_num);

} /* an_lpr10_ldm_info */


an_lpr10_action(action, out_fd)
    int                 action;
    FILE		*out_fd;
{
    char                *s;
    switch(action) {
        case A_ERROR : 		s = "A_ERROR";		break;		/* 0 */
        case A_BIND : 		s = "A_BIND";		break;		/* 1 */
        case A_DO_BIND : 	s = "A_DO_BIND";	break;		/* 2 */
        case A_IDENT : 		s = "A_IDENT";		break;		/* 3 */
        case A_DEV_OPEN_REP :	s = "A_DEV_OPEN_REP";   break;		/* 4 */
        case A_READ :		s = "A_READ";   	break;		/* 5 */
        case A_SAVE_DEV_STAT :  s = "A_SAVE_DEV_STAT";  break;		/* 6 */
        case A_ABORT :       	s = "A_ABORT";		break;		/* 7 */
        case A_HARD_STAT :      s = "A_HARD_STAT";	break;		/* 8 */
        case A_SOFT_STAT :      s = "A_SOFT_STAT";	break;		/* 9 */
        case A_GOOD_CLR :       s = "A_GOOD_CLR";	break;	       /* 10 */
        case A_QUICK :       	s = "A_QUICK";	        break;	       /* 11 */
        case A_IO :             s = "A_IO";	        break;	       /* 12 */
        case A_DEV_STAT :       s = "A_DEV_STAT";	break;	       /* 13 */
        case A_GOOD_IO_REP :    s = "A_GOOD_IO_REP";	break;	       /* 14 */
        case A_DEV_STAT_REP :   s = "A_DEV_STAT_REP";	break;	       /* 15 */
        case A_DEV_CLOSE_REP :  s = "A_DEV_CLOSE_REP";	break;	       /* 16 */
        case A_DE_EST_LINK :    s = "A_DE_EST_LINK";	break;	       /* 17 */
        case A_DIAG :           s = "A_DIAG";	        break;	       /* 18 */
        case A_DIAG_REP :       s = "A_DIAG_REP";	break;	       /* 19 */
        case A_DAP_LOAD :       s = "A_DAP_LOAD";	break;	       /* 20 */
        case A_P_ON_1 :		s = "A_P_ON_1";	        break;	       /* 21 */
        case A_REP_N_DAP :      s = "A_REP_N_DAP";      break;	       /* 22 */
        case A_P_ON_2 :		s = "A_P_ON_2";	        break;	       /* 23 */
        case A_STAT_CLR :       s = "A_STAT_CLR";	break;	       /* 24 */
        case A_SEND_EOJ :	s = "A_SEND_EOJ";	break;	       /* 25 */
        default    :            s = "UNKNOWN ACTION";   break;    /* invalid */

    } /* switch */
    fprintf(out_fd," %s\n",s);

} /* an_lpr10_action */

an_lpr10_class(class, out_fd)
    int                 class;
    FILE		*out_fd;
{
    char                *s;
    switch(class) {
        case C_ERROR : 		s = "C_ERROR";		break;		/* 0 */
        case C_DO_BIND_REQ : 	s = "C_DO_BIND_REQ";    break;		/* 1 */
        case C_BIND_REP : 	s = "C_BIND_REP";  	break;		/* 2 */
        case C_DEV_OPEN : 	s = "C_DEV_OPEN";       break;		/* 3 */
        case C_WRONG_CLR_RESP :	s = "C_WRONG_CLR_RESP"; break;		/* 4 */
        case C_QUICK_REQ :	s = "C_QUICK_REQ"; 	break;		/* 5 */
        case C_IO_REQ_WRR :	s = "C_IO_REQ_WRR"; 	break;		/* 6 */
        case C_IO_REQ_WORR :	s = "C_IO_REQ_WORR"; 	break;		/* 7 */
        case C_DEV_REQ_WORR :	s = "C_DEV_REQ_WORR"; 	break;		/* 8 */
        case C_DEV_REQ_WRR :	s = "C_DEV_REQ_WRR"; 	break;		/* 9 */
        case C_CLEAR_DEV :	s = "C_CLEAR_DEV"; 	break;		/* 10 */
        case C_DEV_CLOSE_WRR :	s = "C_DEV_CLOSE_WRR"; 	break;		/* 11 */
        case C_HARD_STAT :	s = "C_HARD_STAT"; 	break;		/* 12 */
        case C_SOFT_STAT :	s = "C_SOFT_STAT"; 	break;		/* 13 */
        case C_ABORT :		s = "C_ABORT"; 		break;		/* 14 */
        case C_RCV_RDY :	s = "C_RCV_RDY";        break;		/* 15 */
        case C_DEV_STAT :	s = "C_DEV_STAT";       break;		/* 16 */
        case C_DIAG :		s = "C_DIAG";       	break;		/* 17 */
        case C_DIAG_REP :     	s = "C_DIAG_REP";     	break;		/* 18 */
        case C_GOOD_DAM_REP :   s = "C_GOOD_DAM_REP";   break;		/* 19 */
        case C_P_ON_REQ :       s = "C_P_ON_REQ";       break;		/* 20 */
        case C_P_ON_REPLY :     s = "C_P_ON_REPLY";     break;		/* 21 */
        case C_JOB_STAT :       s = "C_JOB_STAT";       break;		/* 22 */
        case C_DEV_CLOSE_WORR : s = "C_DEV_CLOSE_WORR"; break;		/* 23 */
        case C_BAD_P_ON_TRN :   s = "C_BAD_P_ON_TRN";   break;		/* 24 */
    }
    fprintf(out_fd," %s\n",s);

} /* an_lpr10_action */


an_lpr10_dm_io_inprog(dm_io_in_prog, out_fd)
    int                 dm_io_in_prog;
    FILE		*out_fd;
{
    char                *s;
    switch(dm_io_in_prog) {
        case NO_ACTION :        s = "NO_ACTION";	break;		/* 0 */
        case DAM_READ :         s = "DAM_READ";	        break;		/* 1 */
        case DAM_WRITE : 	s = "DAM_WRITE";	break;		/* 2 */
        case DOWNLOAD_IN_ACTION : s = "DOWNLOAD_IN_ACTION"; break;  	/* 3 */
        case IDENTIFY_IN_ACTION : s = "IDENTIFY_IN_ACTION"; break;  	/* 4 */
        case DIAG_READ :        s = "DIAG_READ"; 	break;  	/* 5 */
        case DIAG_WRITE :       s = "DIAG_WRITE"; 	break;  	/* 6 */
        default :               s = "UNKNOWN DM_IO_IN_PROG";break; /* invalid */
    } /* switch */
    fprintf(out_fd," %s\n",s);

} /* an_lpr10_dm_io_inprog */


an_lpr10_state(state, out_fd)
    int                 state;
    FILE                *out_fd;
{
    char                *s;
    switch(state) {
        case S_DO_BIND :    	s = "S_DO_BIND"; 	break;  	/* 0 */
        case S_BIND :    	s = "S_BIND"; 		break;  	/* 1 */
        case S_DEV_OPEN :       s = "S_DEV_OPEN";       break;  	/* 2 */
        case S_DEV_CLR_REP :    s = "S_DEV_CLR_REP";    break;  	/* 3 */
        case S_RCV_RDY_1 :      s = "S_RCV_RDY_1";      break;  	/* 4 */
        case S_DEV_STAT_REP :   s = "S_DEV_STAT_REP";   break;  	/* 5 */
        case S_WORK :        	s = "S_WORK";   	break;  	/* 6 */
        case S_IO :        	s = "S_IO";   		break;  	/* 7 */
        case S_DEV_STAT :       s = "S_DEV_STAT";       break;  	/* 8 */
        case S_TROUBLE_STATE :  s = "S_TROUBLE_STATE";  break;  	/* 9 */
        case S_EST_LINK :       s = "S_EST_LINK";  	break;         /* 10 */
        case S_READ_DEV_CLEAR : s = "S_READ_DEV_CLEAR"; break;         /* 11 */
        case S_DE_EST_LINK :    s = "S_DE_EST_LINK"; 	break;         /* 12 */
        case S_DAM_CLR_REP :    s = "S_DAM_CLR_REP"; 	break;         /* 13 */
        case S_READ_DAM_CLEAR : s = "S_READ_DAM_CLEAR"; break;         /* 14 */
        case S_RCV_RDY_2 :      s = "S_RCV_RDY_2";      break;         /* 15 */
        case S_READ_DEV_STAT :  s = "S_READ_DEV_STAT";  break;         /* 16 */
        case S_DIAG :           s = "S_DIAG";  		break;         /* 17 */
        case S_P_ON_1 :         s = "S_P_ON_1";         break;         /* 18 */
        case S_P_ON_2 :         s = "S_P_ON_2";         break;         /* 19 */
        case S_P_ON_3 :         s = "S_P_ON_3";         break;         /* 20 */
        case S_P_ON_4 :         s = "S_P_ON_4";         break;         /* 21 */
        case S_P_ON_5 :         s = "S_P_ON_5";         break;         /* 22 */
        case S_DAP_REPLY :      s = "S_DAP_REPLY";      break;         /* 23 */
        case S_EOJ :            s = "S_EOJ";      	break;         /* 24 */
        case S_JOB_STAT_REP :   s = "S_JOB_STAT_REP";   break;         /* 25 */
        case S_RCV_RDY_3 :      s = "S_RCV_RDY_3";      break;         /* 26 */
        default    :            s = "UNKNOWN STATE";	break;    /* invalid */
    } /* switch */
    fprintf(out_fd," %s\n",s);

} /* an_lpr10_state */


an_lpr10_msg_descrpt(msg_descrpt, out_fd)
    int                 msg_descrpt;
    FILE                *out_fd;
{
    char                *s;
    switch(msg_descrpt) {
        case ABORT_EVENT_MSG :   s = "ABORT_EVENT_MSG";       break;    /* 1 */
        case CREATION_MSG :      s = "CREATION_MSG";          break;    /* 2 */
        case DO_BIND_REQ_MSG :   s = "DO_BIND_REQ_MSG";       break;    /* 3 */
        case DO_BIND_REPLY_MSG : s = "DO_BIND_REPLY_MSG";     break;    /* 4 */
        case BIND_REQ_MSG :      s = "BIND_REQ_MSG";          break;    /* 5 */
        case BIND_REPLY_MSG :    s = "BIND_REPLY_MSG";        break;    /* 6 */
        case DIAG_REQ_MSG :      s = "DIAG_REQ_MSG";          break;   /* 20 */	
        case DIAG_REPLY_MSG :    s = "DIAG_REPLY_MSG";        break;   /* 21 */	
        case DIAG_EVENT_MSG :    s = "DIAG_EVENT_MSG";        break;   /* 22 */	
        case HPIB_IO_REQ_MSG :   s = "HPIB_IO_REQ_MSG";       break;  /* 110 */
        case DM_IO_REQ_MSG :     s = "DM_IO_REQ_MSG";         break;  /* 120 */
        case DM_IO_REPLY_MSG :   s = "DM_IO_REPLY_MSG";       break;  /* 121 */
        case DM_IO_EVENT_MSG :   s = "DM_IO_EVENT_MSG";       break;  /* 122 */
        default :                s = "UNKNOWN MSG DESCRIPTOR"; break;/*invalid*/
    }
    fprintf(out_fd," %s\n",s);

} /* an_lpr10_msg_descrpt */


an_lpr10_dm_info(pda, out_fd)
    struct cpr_pda      *pda;
    FILE		*out_fd;
{
    do_bind_req_type    *d = &pda->cpr_info;
    cpr_message_type    	cpr_msg;
    struct mgr_info_type 	mgr;
    register unsigned short 	le;
    register unsigned short     he;
    short 			i;

    fprintf(out_fd, "\n======LPR10 DM INFO==========\n");
    fprintf(out_fd, "reply_subq     = %d\n",d->reply_subq);
    fprintf(out_fd, "mgr_port_num   = %d\n",d->mgr_port_num);
    fprintf(out_fd, "config_addr    = %d%d%d\n",
            d->config_addr_3,
            d->config_addr_2,
            d->config_addr_1);
    fprintf(out_fd, "lm_port_num    = %d\n",d->lm_port_num);

    /* Bound and Release frames */
    fprintf(out_fd, "====Bound and Release frame booleans====\n");
    fprintf(out_fd, "ldm_bound        = %5s, ",
            BOOLEAN(pda->booleans.ldm_bound));
    fprintf(out_fd, "dam_bound        = %5s\n",
            BOOLEAN(pda->booleans.dam_bound));
    fprintf(out_fd, "lock_state       = %5s, ",
    	    BOOLEAN(pda->booleans.lock_state));
    fprintf(out_fd, "more_write       = %5s\n",
            BOOLEAN(pda->booleans.more_write));
    fprintf(out_fd, "reset_in_action  = %5s, ", 
            BOOLEAN(pda->booleans.reset_in_action));
    fprintf(out_fd, "cpr_return_frame = %5s\n", 
            BOOLEAN(pda->booleans.cpr_return_frame));
    fprintf(out_fd, "hard_or_soft     = %5s, ", 
            BOOLEAN(pda->booleans.hard_or_soft));
    fprintf(out_fd, "timer_on         = %5s\n", 
            BOOLEAN(pda->booleans.timer_on));
    fprintf(out_fd, "outstanding_io   = %5s, ", 
            BOOLEAN(pda->booleans.outstanding_io));
    fprintf(out_fd, "p_fail_1         = %5s\n", 
            BOOLEAN(pda->booleans.p_fail_1));
    fprintf(out_fd, "p_fail_2         = %5s, ", 
            BOOLEAN(pda->booleans.p_fail_2));
    fprintf(out_fd, "p_fail_aborted   = %5s\n", 
            BOOLEAN(pda->booleans.p_fail_aborted));
    fprintf(out_fd, "dap_loaded       = %5s, ", 
            BOOLEAN(pda->booleans.dap_loaded));
    fprintf(out_fd, "nodiags          = %5s\n", 
            BOOLEAN(pda->booleans.nodiags));
    fprintf(out_fd, "nodiags1         = %5s\n", 
            BOOLEAN(pda->booleans.nodiags1));

    /* Current action */
    fprintf(out_fd, "current action   = ");
    an_lpr10_action(pda->this_action, out_fd);
  
    /* Last action */
    fprintf(out_fd, "last action      = ");
    an_lpr10_action(pda->last_action, out_fd);

    /* Current type of io DM is doing */
    fprintf(out_fd, "current type of IO DM was doing= ");
    an_lpr10_dm_io_inprog(pda->dm_io_in_prog, out_fd);

    /* Get manager information */
    if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, d->mgr_port_num, &mgr) != AIO_OK) {
       fprintf(out_fd,"Problem -- bad port number!\n");
       return(0);
       }

    else {
       fprintf(out_fd, "====Manager Info ====\n");
       fprintf(out_fd,"manager name       = %s\n", mgr.mgr_name);
       fprintf(out_fd,"hardware address   = %s\n", mgr.hw_address);
       fprintf(out_fd,"pda address        = 0x%08x\n", mgr.pda_address);
       fprintf(out_fd,"enabled_subqs      = 0x%08x\n", mgr.enabled_subqs);
       fprintf(out_fd,"active_subqs       = 0x%08x\n", mgr.active_subqs);
       fprintf(out_fd,"iotree_index       = %d\n", mgr.iotree_index);
       fprintf(out_fd,"next_iotree_index  = %d\n", mgr.next_iotree_index);
       fprintf(out_fd,"mgr_table_index    = %d\n", mgr.mgr_table_index);
       fprintf(out_fd,"blocked            = 0x%08x\n", mgr.blocked);
       fprintf(out_fd,"eim                = 0x%08x\n", mgr.eim);
       fprintf(out_fd,"in_poll_list       = 0x%08x\n", mgr.in_poll_list);
    }

    /* Retrieve message from core file */
    if (an_grab_virt_chunk(0, pda->req_msg_ptr, &cpr_msg, sizeof(cpr_msg)) != 0)
       fprintf(stderr, "Couldn't get message\n");

    else
       {
       /* Message header info */
       fprintf(out_fd, "msg_descriptor       = ");
       an_lpr10_msg_descrpt(cpr_msg.msg_header.msg_descriptor, out_fd); 
       fprintf(out_fd, "message_id           = %d\n", 
            cpr_msg.msg_header.message_id); 
       fprintf(out_fd, "transaction_num      = 0x%08x\n", 
            cpr_msg.msg_header.transaction_num); 
       }

    /* Message structure */
    aio_decode_message(pda->req_msg_ptr, "     ", out_fd);

    /* incoming subq and message class */
    fprintf(out_fd, "input_msg_subq       = %d\n", pda->input_msg_subq);
    fprintf(out_fd, "input_msg_class      = ");
    an_lpr10_class(pda->input_msg_class, out_fd);
    fprintf(out_fd, "reply_ct             = %d\n", pda->reply_ct);

    /* Device status type */
    le = pda->device_status.u.soft.low_errors;
    he = pda->device_status.u.soft.high_errors;

    /* Printer off-line */
    if (!online(le))
	fprintf(out_fd, "Device Status : Printer off-line\n");

    /* Soft errors */
    if (le != 0) {
    	if (online(le))
	    fprintf(out_fd, "Device Status : Printer on-line\n");
    	if (paper_out(le))
	    fprintf(out_fd, "Device Status : Printer paper-out\n");
    	if (paper_jam(le))
	    fprintf(out_fd, "Device Status : Printer paper jam\n");
    	if (platen_open(le))
	    fprintf(out_fd, "Device Status : Printer platen open\n");
    	if (ribbon_problem(le))
	    fprintf(out_fd, "Device Status : Printer ribbon problem\n");
    	if (self_test_failed(le))
	    fprintf(out_fd, "Device Status : Printer self-test failed\n");
    	if (data_loss(le))
	    fprintf(out_fd, "Device Status : Printer data-Loss\n");
    	if (power_fail(le))
	    fprintf(out_fd, "Device Status : Printer powerfail\n");
    }

    /* Hard errors */
    if (he != 0) {
    	if ((!data_overrun(he)) && (!transport_error(he)))
	    fprintf(out_fd, "Device Status : General irrecoverable data error\n");
        if (data_overrun(he))
	    fprintf(out_fd, "Device Status : data overrun\n");
        if (transport_error(he))
	    fprintf(out_fd, "Device Status : Transmission error\n");
    }


    fprintf(out_fd, "dev_stat = %5s, ", 
            BOOLEAN(pda->status_available.dev_stat));
    fprintf(out_fd, "env_stat = %5s\n", 
            BOOLEAN(pda->status_available.env_stat));
    fprintf(out_fd, "job_stat = %5s, ", 
            BOOLEAN(pda->status_available.job_stat));
    fprintf(out_fd, "generic_stat = %5s\n", 
            BOOLEAN(pda->status_available.generic_stat));

    fprintf(out_fd, "ldm_generic  = 0x%08x\n", pda->ldm_generic);
    fprintf(out_fd, "num_retries          = %d\n", pda->num_retries);

    /* States and actions */
    fprintf(out_fd, "cpr_old_state        = ");
    an_lpr10_state(pda->cpr_old_state, out_fd);
    
    fprintf(out_fd, "Current cpr_state    = ");
    an_lpr10_state(pda->cpr_state, out_fd);

    fprintf(out_fd, "state prior to powerfail p_on_state = ");
    an_lpr10_state(pda->p_on_state, out_fd);

    fprintf(out_fd,
            "Action DM was working on at powerfail p_on_action = ");
    an_lpr10_action(pda->p_on_action, out_fd);

    /* transactions */
    fprintf(out_fd, "p_on_trn             = 0x%08x\n", pda->p_on_trn);
    fprintf(out_fd, "pfl_prog             = %d\n", pda->pfl_prog);
    fprintf(out_fd, "operation state      = %d\n", pda->operation_state);

    aio_decode_llio_status(pda->llio_info, out_fd);

    fprintf(out_fd, "cpr_send_num         = %d\n", pda->cpr_send_num);
    fprintf(out_fd, "cpr_rcv_num          = %d\n", pda->cpr_rcv_num);
    fprintf(out_fd, "cpr_rcv_rdy_ct       = %d\n", pda->cpr_rcv_rdy_ct);
    fprintf(out_fd, "send_packet_num      = %d\n", pda->send_packet_num);
    fprintf(out_fd, "packet_size          = %d\n", pda->packet_size);
    fprintf(out_fd, "packet_head_size     = %d\n", pda->packet_head_size);
    fprintf(out_fd, "dev_clear_rec_num    = %d\n", pda->dev_clear_rec_num);
    fprintf(out_fd, "hpib_cmd_length      = %d\n", pda->hpib_cmd_length);
    fprintf(out_fd, "transfer_count       = %d\n", pda->transfer_count);
    fprintf(out_fd, "cpr_dm_rev_level     = 0x%08x\n", pda->cpr_dm_rev_level);
    fprintf(out_fd, "cpr_subqs            = 0x%08x\n", pda->cpr_subqs);
    fprintf(out_fd, "current transaction #= 0x%08x\n", pda->trn);
    fprintf(out_fd, "timer_id             = %d\n", pda->timer_id);
    fprintf(out_fd, "max_transfer_size    = %d\n", pda->max_transfer_size);
    
  
    /* DAP - related information - must be 64 byte aligned. */
    fprintf(out_fd, "lp_log10.lp_log_pointer= %d\n",
        pda->lp_log10.lp_log_pointer);

    for (i = 0; i < 64; i++)
    	fprintf(out_fd, "lp_log10.lplog[%d] = 0x%08x\n", i, pda->lp_log10.lplog[i]);

    fprintf(out_fd, "dap_stat                        = %d\n", pda->dap_stat);
    fprintf(out_fd, "dap_status.halt_code            = 0x%08x\n",
        pda->dap_status.halt_code);
    fprintf(out_fd, "dap_status.high_timeout_counter = 0x%08x\n",
        pda->dap_status.high_timeout_counter);
    fprintf(out_fd, "dap_status.low_timeout_counter  = 0x%08x\n",
        pda->dap_status.low_timeout_counter);
    fprintf(out_fd, "dap_status.dsj_index_1          = 0x%08x\n",
        pda->dap_status.dsj_index_1);
    fprintf(out_fd, "dap_status.dsj_index_2          = 0x%08x\n",
        pda->dap_status.dsj_index_2);
    fprintf(out_fd, "dap_status.dsj_index_3          = 0x%08x\n",
        pda->dap_status.dsj_index_3);
    fprintf(out_fd, "dap_status.hpib_id_types        = 0x%08x\n",
        pda->dap_status.hpib_id_bytes);
    fprintf(out_fd, "dap_parms[1]                    = 0x%08x\n",
        pda->dap_parms[1]);
    fprintf(out_fd, "dap_area_ptr                    = 0x%08x\n",
        pda->dap_area_ptr);

    /* Buffer area for holding user data while adding blk and pckt hdr. */
    fprintf(out_fd, "active                = %s\n",
            BOOLEAN(pda->buff_area.active));
    fprintf(out_fd, "ready                 = %s\n",
            BOOLEAN(pda->buff_area.ready));
    fprintf(out_fd, "start_buff_ptr        = 0x%08x\n",
            pda->buff_area.start_buff_ptr);
    fprintf(out_fd, "current_pos_ptr       = 0x%08x\n",
            pda->buff_area.current_pos_ptr);
    fprintf(out_fd, "current_length        = 0x%08x\n",
            pda->buff_area.current_length);
    fprintf(out_fd, "ctrl_overhead         = 0x%08x\n",
            pda->buff_area.ctrl_overhead);

    /* data buffer for writes */
    fprintf(out_fd, "send_rec.send_rec_head.head_len   = 0x%08x\n",
            pda->send_rec.send_rec_head.head_len);
    fprintf(out_fd, "send_rec.send_rec_head.record_num = 0x%08x\n",
            pda->send_rec.send_rec_head.record_num);

    switch (pda->send_rec.send_rec_head.code) {
       case RECEIVE_READY : fprintf(out_fd,
            "send_rec.send_rec_head.code = RECEIVE_READY\n");
            break;
       case DEV_CLEAR : fprintf(out_fd,
            "send_rec_head.code = DEV_CLEAR or CLEAR_RESP\n");
            break;
       case REPORT_DEV_STAT : fprintf(out_fd,
            "send_rec_head.code =REPORT_DEV_STAT or DEV_STAT_REPORT\n");
            break;
       case ENV_STAT_REPORT : fprintf(out_fd,
            "send_rec_head.code = ENV_STAT_REPORT or REPORT_ENV_STAT\n");
            break;
       case CONFIGURATION : fprintf(out_fd,
            "send_rec_head.code = CONFIGURATION\n");
            break;
       case START_OF_JOB : fprintf(out_fd,
            "send_rec_head.code = START_OF_JOB\n");
            break;
       case END_OF_JOB : fprintf(out_fd,
            "send_rec_head.code = END_OF_JOB\n");
            break;
       case REPORT_JOB_STAT : fprintf(out_fd,
            "send_rec_head.code = REPORT_JOB_STAT or JOB_STAT_REPORT\n");
            break;
       case SILENT_RUN : fprintf(out_fd,
       	    "send_rec_head.code = SILENT_RUN\n");
            break;
       case CPR_WRITE : fprintf(out_fd,
            "send_rec_head.code = CPR_WRITE\n");
            break;
       case CPR_READ : fprintf(out_fd,
            "send_rec_head.code = CPR_READ/READ_RESP/LAST_CIPER_COMMAND\n");
            break;
       case INIT_CPR_COM_VAL : fprintf(out_fd,
            "send_rec_head.code = INIT_CPR_COM_VAL\n");
            break;
       default : fprintf(out_fd,"UNKNOWN CIPER COMMAND %d\n",
             pda->send_rec.send_rec_head.code); 
    }

    switch (pda->send_rec.send_rec_head.host_or_perf) {
       case HOST : fprintf(out_fd,
                           "send_rec.send_rec_head.host_or_perf = HOST\n");
                   break;
       case PERIPHERAL : fprintf(out_fd,
                         "send_rec.send_rec_head.host_or_perf = PERIPHERAL\n");
                   break;
    }

    fprintf(out_fd, "send_rec.send_rec_head.start_of_block         = %s\n",
            BOOLEAN(pda->send_rec.send_rec_head.start_of_block));
    fprintf(out_fd, "send_rec.send_rec_head.end_of_block           = %s\n",
            BOOLEAN(pda->send_rec.send_rec_head.end_of_block));

    switch (pda->send_rec.send_rec_head.data_type) {
       case UNUSED : fprintf(out_fd,
                     "send_rec.send_rec_head.data_type = UNUSED\n");
    } 

} /* an_lpr10_dm_info */

an_lpr10_all(pda, out_fd)
    struct cpr_pda      *pda;
    FILE		*out_fd;
{
    an_lpr10_dam_info(pda, out_fd);
    an_lpr10_ldm_info(pda, out_fd);
    an_lpr10_dm_info(pda, out_fd);
}

an_lpr10_decode_status(llio_status, out_fd)
    llio_status_type	llio_status;
    FILE		*out_fd;
{
    char *s;

    fprintf(out_fd, "lpr10 llio status = 0x%08x -- (", llio_status);
    if (llio_status.u.subsystem == SUBSYS_CIPER_DM)
	fprintf(out_fd, "SUBSYS_CIPER_DM, ");
    else 
	fprintf(out_fd, "subsys %d?, ", llio_status.u.subsystem);

    if (llio_status.u.proc_num == -1)
	fprintf(out_fd, "LOCAL, ");

    else if (llio_status.u.proc_num == 0)
	fprintf(out_fd, "GLOBAL, ");

    else
	fprintf(out_fd, "%d?, ", llio_status.u.proc_num);

    switch (llio_status.u.error_num)  {
        case CPR_DM_NOT_ON_LINE :       s = "CPR_DM_NOT_ON_LINE";       break;
        case CPR_DM_PAPER_OUT :		s = "CPR_DM_PAPER_OUT";		break;
        case CPR_DM_PAPER_JAM :		s = "CPR_DM_PAPER_JAM";		break;
        case CPR_DM_PLATEN_OPEN : 	s = "CPR_DM_PLATEN_OPEN";	break;	
	case CPR_DM_RIBBON_FAIL : 	s = "CPR_DM_RIBBON_FAIL";	break;	
	case CPR_DM_SELF_TEST_FAIL :	s = "CPR_DM_SELF_TEST_FAIL";	break;
	case CPR_DM_DATA_LOST : 	s = "CPR_DM_DATA_LOST";		break;
	case CPR_DM_POWER_FAIL : 	s = "CPR_DM_POWER_FAIL";	break;
	case CPR_DM_BAD_HDR_LENGTH : 	s = "CPR_DM_BAD_HDR_LENGTH";	break;
	case CPR_DM_BAD_HTOP_NUM : 	s = "CPR_DM_BAD_HTOP_NUM";	break;
	case CPR_DM_HP_FIELD : 		s = "CPR_DM_HP_FIELD";		break;
	case CPR_DM_BAD_REC_CODE : 	s = "CPR_DM_BAD_REC_CODE";	break;
	case CPR_DM_BAD_DATA_TYPE : 	s = "CPR_DM_BAD_DATA_TYPE";	break;
	case CPR_DM_XPORT_SERV_ERR :	s = "CPR_DM_XPORT_SERV_ERR";	break;
	case CPR_DM_DATA_OVER :		s = "CPR_DM_DATA_OVER";		break;
	case CPR_DM_BLOCK_LABEL : 	s = "CPR_DM_BLOCK_LABEL";	break;
	case CPR_DM_TRANSPORT_ERROR : 	s = "CPR_DM_TRANSPORT_ERROR";	break;
	case CPR_DM_DATA_OVERRUN : 	s = "CPR_DM_DATA_OVERRUN";	break;
	case CPR_DM_SUCCESS_RETRY : 	s = "CPR_DM_SUCCESS_RETRY";	break;
	case CPR_DM_STATUS_INVALID : 	s = "CPR_DM_STATUS_INVALID";	break;
	case CPR_DM_DEVICE_ADAPTER_FAILURE :
			           s = "CPR_DM_DEVICE_ADAPTER_FAILURE"; break;
	case CPR_DM_READ_STATUS :	s = "CPR_DM_READ_STATUS";	break;
	case CPR_DM_PARITY_ERROR : 	s = "CPR_DM_PARITY_ERROR";	break;
	case CPR_DM_TRANSMISSION_ERROR :
				       s = "CPR_DM_TRANSMISSION_ERROR"; break; 
        case CPR_DM_LOGICAL_ABORT : 	s = "CPR_DM_LOGICAL_ABORT";	break;
	case CPR_DM_SYSTEM_POWERFAIL : 	s = "CPR_DM_SYSTEM_POWERFAIL";	break;
	case CPR_DM_INVALID_FUNCTION :	s = "CPR_DM_INVALID_FUNCTION";	break;
	case CPR_DM_PRIOR_ERROR_ABORT: 	s = "CPR_DM_PRIOR_ERROR_ABORT"; break;
	case CPR_DM_PROTOCOL_ERROR : 	s = "CPR_DM_PROTOCOL_ERROR";	break;
	case CPR_DM_HARDWARE_FAILURE : 	s = "CPR_DM_HARDWARE_FAILURE";	break;
	default:			s = "unrecognized error?";      break;
    }
    fprintf(out_fd, "%s)\n", s);
}

an_lpr10_help(out_fd)
    FILE		*out_fd;
{
    fprintf(out_fd, "\nvalid options:\n");
    fprintf(out_fd, "   dam     --  decode device adapter manager state\n");
    fprintf(out_fd, "   ldm     --  decode logical device manager state\n");
    fprintf(out_fd, "   dm      --  decode device manager state\n");
    fprintf(out_fd, "   all     --  do all of the above\n");
    fprintf(out_fd, "   help    --  print this help screen\n");
}



an_lpr10_decode_message(mp, prefix, out_fd)
    cpr_message_type	        *mp;
    char 			*prefix;
    FILE			*out_fd;
{
    llio_std_header_type *l = &mp->msg_header;

    /* Message structure */
    switch(l->msg_descriptor) {
    case CREATION_MSG : 		
	fprintf(out_fd, "CREATION_MSG\n");
	fprintf(out_fd, "server_data_len  = %d\n",
	        mp->u.creation_info.server_data_len);
	fprintf(out_fd, "max_msg_size     = %d\n",
	        mp->u.creation_info.max_msg_size);
	fprintf(out_fd, "num_msgs         = %d\n",
	        mp->u.creation_info.num_msgs);
	fprintf(out_fd, "num_subqueues    = 0x%08x\n",
	        mp->u.creation_info.num_subqueues);
	fprintf(out_fd, "port_num         = %d\n",
	        mp->u.creation_info.port_num);
        break;
    case DO_BIND_REQ_MSG :
	fprintf(out_fd, "DO_BIND_REQ_MSG\n");
	fprintf(out_fd, "reply_subq       = 0x%08x\n",
	        mp->u.do_bind_req.reply_subq);
	fprintf(out_fd, "mgr_port_num     = %d\n",
	        mp->u.do_bind_req.mgr_port_num);
	fprintf(out_fd, ", config_addr    = %d/%d/%d\n",
	        mp->u.do_bind_req.config_addr_3,
	        mp->u.do_bind_req.config_addr_2,
	        mp->u.do_bind_req.config_addr_1);
	fprintf(out_fd, "lm_port_num      = %d\n",
	        mp->u.do_bind_req.lm_port_num);
        break;
    case DO_BIND_REPLY_MSG :
	fprintf(out_fd, "DO_BIND_REPLY_MSG\n");
        an_lpr10_decode_status(mp->u.do_bind_reply , out_fd);
        break;
    case BIND_REQ_MSG :
	fprintf(out_fd, "BIND_REQ_MSG\n");
        fprintf(out_fd, "reply_subq     = 0x%08x\n ",                                           mp->u.bind_req.reply_subq);
        fprintf(out_fd, "hm_event_subq  = 0x%08x\n ",                                           mp->u.bind_req.hm_event_subq);
        fprintf(out_fd, "hm_subsys_num  = %d\n ",                                               mp->u.bind_req.hm_subsys_num);
        fprintf(out_fd, "hm_meta_lang   = %d\n ",                                               mp->u.bind_req.hm_meta_lang);
        fprintf(out_fd, "hm_rev_code    = %d\n ",                                               mp->u.bind_req.hm_rev_code);
	    
        break;
    case BIND_REPLY_MSG :
	fprintf(out_fd, "BIND_REPLY_MSG\n");
        fprintf(out_fd, "lm_rev_code          = %d\n",                                          mp->u.bind_reply.lm_rev_code);
        fprintf(out_fd, "lm_queue_depth       = %d\n",                                          mp->u.bind_reply.lm_queue_depth);
        fprintf(out_fd, "lm_low_req_subq      = %d\n ",                                         mp->u.bind_reply.lm_low_req_subq);
        fprintf(out_fd, "lm_hi_req_subq       = %d\n ",                                         mp->u.bind_reply.lm_hi_req_subq);
        fprintf(out_fd, "lm_freeze_data       = %s\n",                                          BOOLEAN(mp->u.bind_reply.lm_freeze_data));
        fprintf(out_fd, "lm_alignment         = %s\n",                                          BOOLEAN(mp->u.bind_reply.lm_alignment));
        an_lpr10_decode_status(mp->u.bind_reply.reply_status, out_fd);
        break;
    case DIAG_REQ_MSG :
	fprintf(out_fd, "DIAG_REQ_MSG\n");
	fprintf(out_fd, "Structure printout not implemented\n");
        break;
    case DIAG_REPLY_MSG :
	fprintf(out_fd, "DIAG_REPLY_MSG\n");
	fprintf(out_fd, "Structure printout not implemented\n");
        break;
    case DIAG_EVENT_MSG :
	fprintf(out_fd, "DIAG_EVENT_MSG\n");
	fprintf(out_fd, "Structure printout not implemented\n");
        break;
    case HPIB_IO_REQ_MSG :
	fprintf(out_fd, "HPIB_IO_REQ_MSG\n");
	fprintf(out_fd, "Structure printout not implemented\n");
    case HPIB_IO_REPLY_MSG :
	fprintf(out_fd, "HPIB_IO_REPLY_MSG\n");
	fprintf(out_fd, "Structure printout not implemented\n");
    case DM_IO_REQ_MSG :
	fprintf(out_fd, "DM_IO_REQ_MSG\n");
        fprintf(out_fd, "reply_subq             = %d\n ",                                       mp->u.dm_req.reply_subq);

        switch (mp->u.dm_req.data_class) {
            case VIRTUAL_BUFFER :
                fprintf(out_fd, "data_class             = VIRTUAL BUFFER\n");
            case VIRTUAL_BLOCKS :
                fprintf(out_fd, "data_class             = VIRTUAL BLOCKS\n");
            case CONTIGUOUS_REAL:
                fprintf(out_fd, "data_class             = CONTIGUOUS REAL\n");
        }
                
        fprintf(out_fd, "data_len               = %d\n ",                                       mp->u.dm_req.data_len);
        break;
    case DM_IO_REPLY_MSG :
	fprintf(out_fd, "DM_IO_REPLY_MSG\n");
        
        an_lpr10_decode_status(mp->u.dm_reply.reply_status,out_fd);
        fprintf(out_fd,"transfer_count          = %d\n ",
                mp->u.dm_reply.transfer_count);
        break;
    case DM_IO_EVENT_MSG :
	fprintf(out_fd, "DM_IO_EVENT_MSG\n");
        break;
    default:
	fprintf(out_fd, "UNKNOWN MESSAGE)\n");
	break;
    } /* switch */
} /* an_lpr10_decode_message */
