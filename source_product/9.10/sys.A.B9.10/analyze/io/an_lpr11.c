/***************************************************************************/
/**                                                                       **/
/**   This is the lpr11 (AMIGO) i/o manager analysis routine.             **/
/**                                                                       **/
/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sio/llio.h>
#include <h/types.h>
#include <sio/iotree.h>
#include <sio/amg_def.h> /* AMIGO def.h */
#include <sio/amg_pda.h> /* AMIGO pda.h */
#include "aio.h"			        /* all analyze i/o defs*/

int    lpr11_an_rev  = 1;	/* global -- rev number of lpr11 DS   */
static char *me      = "lpr11";

#define	BOOLEAN(s)	((s) ? "true" : "false")
#define bit(x)  	(1 << (x))
#define	streq(a, b)	(strcmp((a), (b)) == 0)
#define	min(a, b)	(((a) < (b)) ? (a) : (b))
#define	BITS	(BYTE_MODE|CIOCA_S1S2|LOGCH_BREAK|BLOCKED|CONTINUE)

int an_lpr11(call_type, port_num, out_fd, option_string)
    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */
{
    int    		addr;		/* address of extern                  */
    int			code_rev;	/* revision of driver code	      */
    struct amg_pda      pda;		/* port data area                     */
    struct mgr_info_type mgr;		/* all generic manager info	      */

    int			an_lpr11_decode_message();
    int			an_lpr11_decode_status();

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
	    if (an_grab_extern("lpr11_an_rev", &addr) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, lpr11_an_rev,
				 out_fd);

            else {
	        if (an_grab_real_chunk(addr, &code_rev, sizeof(code_rev)) != 0)
		   aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, lpr11_an_rev,
				    out_fd);
	        if (code_rev != lpr11_an_rev)
		   aio_rev_mismatch(me, AIO_INCOMPAT_REVS, code_rev, 
				    lpr11_an_rev, out_fd);
            }

	    /* put my decoding routines into system lists        */
	    aio_init_llio_status_decode(SUBSYS_LPR11, an_lpr11_decode_status);
	    aio_init_message_decode(START_DM_MSG, START_DM_MSG+9,
	                            an_lpr11_decode_message);
	    break;

	case AN_MGR_BASIC:
	    an_lpr11_all(&pda, out_fd);
	    break;

	case AN_MGR_DETAIL:
	    an_lpr11_all(&pda, out_fd);
	    break;

	case AN_MGR_OPTIONAL:
	    an_lpr11_optional(&pda, out_fd, option_string);
	    break;

	case AN_MGR_HELP:
	    an_lpr11_help(stderr);
	    break;

    }
    return(0);

} /* an_lpr11 */


an_lpr11_optional(pda, out_fd, option_string)
    struct amg_pda      *pda;
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
	    an_lpr11_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (dam_flag)   an_lpr11_dam_info(pda, out_fd);
    if (ldm_flag)   an_lpr11_ldm_info(pda, out_fd);
    if (dm_flag)    an_lpr11_dm_info(pda, out_fd);
    if (all_flag)   an_lpr11_all(pda, out_fd);
}

an_lpr11_dam_info(pda, out_fd)
    struct amg_pda      *pda;
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
} /* an_lpr11_dam_info */


an_lpr11_ldm_info(pda, out_fd)
    struct amg_pda      *pda;
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

} /* an_lpr11_ldm_info */


an_lpr11_action(action, out_fd)
    int                 action;
    FILE		*out_fd;
{
    char                *s;
    switch(action) {
        case A_ERROR : 		s = "A_ERROR";		break;		/* 0 */
        case A_DO_BIND : 	s = "A_DO_BIND";	break;		/* 1 */
        case A_BIND : 		s = "A_BIND";		break;		/* 2 */
        case A_IDENT : 		s = "A_IDENT";		break;		/* 3 */
        case A_ABORT :       	s = "A_ABORT";		break;		/* 4 */
        case A_DAP_LOAD :       s = "A_DAP_LOAD";	break;		/* 5 */
	case A_CLEAR : 		s = "A_CLEAR";		break;		/* 6 */
        case A_IO :             s = "A_IO";	        break;	        /* 7 */
        case A_READ :		s = "A_READ";   	break;		/* 8 */
        case A_STAT :		s = "A_STAT";   	break;		/* 9 */
   	case A_STORE_READ : 	s = "A_STORE_READ";	break;	       /* 10 */	
	case A_LDM_REPLY :	s = "A_LDM_REPLY";	break;	       /* 11 */
	case A_STORE_STAT :	s = "A_STORE_STAT";	break;	       /* 12 */
	case A_P_ON :		s = "A_P_ON";		break;	       /* 13 */
        case A_QUICK :       	s = "A_QUICK";	        break;	       /* 14 */
        case A_DIAG :           s = "A_DIAG";	        break;	       /* 15 */
        case A_DIAG_REP :       s = "A_DIAG_REP";	break;	       /* 16 */
	case A_HARD_ID : 	s = "A_HARD_ID";	break;	       /* 17 */
	case A_STAT_REP :	s = "A_STAT_REP";	break;	       /* 18 */	
        default    :            s = "UNKNOWN ACTION";   break;    /* invalid */

    } /* switch */
    fprintf(out_fd," %s\n",s);

} /* an_lpr11_action */

an_lpr11_class(class, out_fd)
    int                 class;
    FILE		*out_fd;
{
    char                *s;
    switch(class) {
        case C_ERROR : 		s = "C_ERROR";		break;		/* 0 */
        case C_DO_BIND : 	s = "C_DO_BIND";        break;		/* 1 */
        case C_BIND_REPLY : 	s = "C_BIND_REPLY";  	break;		/* 2 */
        case C_OPEN : 	        s = "C_OPEN";           break;		/* 3 */
        case C_GOOD_DAM_REP :   s = "C_GOOD_DAM_REP";   break;		/* 4 */
  	case C_IO_REQ :		s = "C_IO_REQ";		break;		/* 5 */
	case C_READ_EARLY :	s = "C_READ_EARLY";	break;	 	/* 6 */
	case C_READ_IO : 	s = "C_READ_IO";	break;		/* 7 */
	case C_DEV_STAT_REQ : 	s = "C_DEV_STAT_REQ";	break;		/* 8 */
        case C_CLEAR_DEV :	s = "C_CLEAR_DEV"; 	break;		/* 9 */
        case C_P_ON_REQ :       s = "C_P_ON_REQ";       break;         /* 10 */
        case C_P_ON_REPLY :     s = "C_P_ON_REPLY";     break;	       /* 11 */
        case C_BAD_P_ON_TRN :   s = "C_BAD_P_ON_TRN";   break;	       /* 12 */
        case C_ABORT :		s = "C_ABORT"; 		break;	       /* 13 */
        case C_DIAG :		s = "C_DIAG";       	break;	       /* 14 */
        case C_DIAG_REP :     	s = "C_DIAG_REP";     	break;	       /* 15 */
        case C_QUICK_REQ :	s = "C_QUICK_REQ"; 	break;	       /* 16 */
	case C_DEV_CLOSE : 	s = "C_DEV_CLOSE";	break;	       /* 17 */
	case C_OLD_REPLY : 	s = "C_OLD_REPLY";	break;	       /* 18 */
        default :		s = "UNKNOWN MSG CLASS";break;	  /* invalid */
    }
    fprintf(out_fd," %s\n",s);

} /* an_lpr11_action */


an_lpr11_dm_io_inprog(dm_io_in_prog, out_fd)
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
	case DEV_STAT_IN_ACTION :
				s = "DEV_STAT_IN_ACTION"; break;        /* 7 */
	case SELF_TEST_IN_ACTION :
				s = "SELF_TEST_IN_ACTION"; break;	/* 8 */
	case WRITE_LB_IN_ACTION :
				s = "WRITE_LB_IN_ACTION"; break;        /* 9 */
	case READ_LB_IN_ACTION :
				s = "READ_LB_IN_ACTION"; break;        /* 10 */
	case DIAG_STAT_IN_ACTION :	
				s = "DIAG_STAT_IN_ACTION"; break;      /* 11 */
	case HARD_ID :		s = "HARD_ID";		   break;      /* 12 */
        default :               s = "UNKNOWN DM_IO_IN_PROG";break; /* invalid */
    } /* switch */
    fprintf(out_fd," %s\n",s);

} /* an_lpr11_dm_io_inprog */


an_lpr11_state(state, out_fd)
    int                 state;
    FILE                *out_fd;
{
    char                *s;
    switch(state) {
        case S_DO_BIND :    	s = "S_DO_BIND"; 	break;  	/* 0 */
        case S_BIND :    	s = "S_BIND"; 		break;  	/* 1 */
        case S_DEV_OPEN :       s = "S_DEV_OPEN";       break;  	/* 2 */
        case S_ID :    		s = "S_ID";    		break;  	/* 3 */
        case S_DAP :      	s = "S_DAP";      	break;  	/* 4 */
        case S_CLEAR :      	s = "S_CLEAR";      	break;  	/* 5 */
        case S_WORK :      	s = "S_WORK";      	break;  	/* 6 */
        case S_IO :      	s = "S_IO";      	break;  	/* 7 */
        case S_DIAG :           s = "S_DIAG";  		break;          /* 8 */
        case S_P_ON_1 :         s = "S_P_ON_1";         break;          /* 9 */
        case S_P_ON_2 :         s = "S_P_ON_2";         break;         /* 10 */
        case S_HARD_ID :        s = "S_HARD_ID";        break;         /* 11 */
        case S_CLOSE_WAIT :     s = "S_CLOSE_WAIT";     break;         /* 12 */
        case S_1_P_ON_CLOSE :   s = "S_1_P_ON_CLOSE";   break;         /* 13 */
        case S_2_P_ON_CLOSE :   s = "S_2_P_ON_CLOSE";   break;         /* 14 */
        case S_DEV_STAT :   	s = "S_DEV_STAT";   	break;         /* 15 */
        default    :            s = "UNKNOWN STATE";	break;    /* invalid */
    } /* switch */
    fprintf(out_fd," %s\n",s);

} /* an_lpr11_state */


an_lpr11_msg_descrpt(msg_descrpt, out_fd)
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
	case POWER_ON_REQ_MSG :  s = "POWER_ON_REQ_MSG";      break;   /* 17 */
	case POWER_ON_REPLY_MSG :s = "POWER_ON_REPLY_MSG";    break;   /* 18 */
	case TIMER_EVENT_MSG : 	 s = "TIMER_EVENT_MSG";	      break;   /* 19 */
        case DIAG_REQ_MSG :      s = "DIAG_REQ_MSG";          break;   /* 20 */	
        case DIAG_REPLY_MSG :    s = "DIAG_REPLY_MSG";        break;   /* 21 */	
        case DIAG_EVENT_MSG :    s = "DIAG_EVENT_MSG";        break;   /* 22 */	
        case STATUS_REQ_MSG :    s = "STATUS_REQ_MSG";        break;   /* 23 */	
        case STATUS_REPLY_MSG :  s = "STATUS_REPLY_MSG";      break;   /* 24 */	
        case RESET_REQ_MSG :     s = "RESET_REQ_MSG";         break;   /* 25 */	
        case RESET_REPLY_MSG :   s = "RESET_REPLY_MSG";       break;   /* 26 */	
        case LOCK_REQ_MSG :      s = "LOCK_REQ_MSG";          break;   /* 27 */	
        case LOCK_REPLY_MSG :    s = "LOCK_REPLY_MSG";        break;   /* 28 */	
        case UNLOCK_REQ_MSG :    s = "UNLOCK_REQ_MSG";        break;   /* 29 */	
        case UNLOCK_REPLY_MSG :  s = "UNLOCK_REPLY_MSG";      break;   /* 30 */	
	case CIO_DMA_IO_REQ_MSG :s = "CIO_DMA_IO_REQ_MSG";    break;  /* 100 */
	case CIO_DMA_IO_REPLY_MSG :
	                         s = "CIO_DMA_IO_REPLY_MSG";  break;  /* 101 */
	case CIO_CTRL_REQ_MSG :  s = "CIO_CTRL_REQ_MSG";      break;  /* 102 */
	case CIO_CTRL_REPLY_MSG :
	                         s = "CIO_CTRL_REPLY_MSG";    break;  /* 103 */
	case CIO_IO_EVENT_MSG :	 s = "CIO_IO_EVENT_MSG";      break;  /* 104 */
	case CIO_INTERRUPT_MSG : s = "CIO_INTERRUPT_MSG";     break;  /* 105 */
	case CIO_POLL_DMA_REQ_MSG :
	                         s = "CIO_POLL_DMA_REQ_MSG";  break;  /* 106 */
	case CIO_POLL_ASYNC_MSG :s = "CIO_POLL_ASYNC_MSG";    break;  /* 107 */
	case CIO_DMA_DUMP_REQ_MSG :
	                         s = "CIO_DMA_DUMP_REQ_MSG";  break;  /* 108 */

        case HPIB_IO_REQ_MSG :   s = "HPIB_IO_REQ_MSG";       break;  /* 110 */
        case HPIB_IO_REPLY_MSG : s = "HPIB_IO_REPLY_MSG";     break;  /* 111 */
        case HPIB_IO_EVENT_MSG : s = "HPIB_IO_EVENT_MSG";     break;  /* 112 */
        case DM_IO_REQ_MSG :     s = "DM_IO_REQ_MSG";         break;  /* 120 */
        case DM_IO_REPLY_MSG :   s = "DM_IO_REPLY_MSG";       break;  /* 121 */
        case DM_IO_EVENT_MSG :   s = "DM_IO_EVENT_MSG";       break;  /* 122 */
        default :                s = "UNKNOWN MSG DESCRIPTOR"; break;/*invalid*/
    }
    fprintf(out_fd," %s\n",s);

} /* an_lpr11_msg_descrpt */


an_lpr11_dm_info(pda, out_fd)
    struct amg_pda      *pda;
    FILE		*out_fd;
{
    do_bind_req_type    *d = &pda->amg_info;
    amg_message_type    	amg_msg;
    struct mgr_info_type 	mgr;
    register bit8		stat;
    short 			i;

    fprintf(out_fd, "\n======LPR11 DM INFO==========\n");
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
    fprintf(out_fd, "debug            = %5s, ", 
            BOOLEAN(pda->booleans.debug));
    fprintf(out_fd, "amg_return_frame = %5s\n", 
            BOOLEAN(pda->booleans.amg_return_frame));
    fprintf(out_fd, "dap_loaded       = %5s, ", 
            BOOLEAN(pda->booleans.dap_loaded));
    fprintf(out_fd, "io_done          = %5s\n", 
            BOOLEAN(pda->booleans.io_done));
    fprintf(out_fd, "timer_on         = %5s, ", 
            BOOLEAN(pda->booleans.timer_on));
    fprintf(out_fd, "nodiags          = %5s\n", 
            BOOLEAN(pda->booleans.nodiags));
    fprintf(out_fd, "nodiags1         = %5s, ", 
            BOOLEAN(pda->booleans.nodiags1));
    fprintf(out_fd, "error            = %5s\n", 
            BOOLEAN(pda->booleans.error));

    /* Current action */
    fprintf(out_fd, "current action   = ");
    an_lpr11_action(pda->this_action, out_fd);
  
    /* Last action */
    fprintf(out_fd, "last action      = ");
    an_lpr11_action(pda->last_action, out_fd);

    /* Current type of io DM is doing */
    fprintf(out_fd, "current type of IO DM was doing= ");
    an_lpr11_dm_io_inprog(pda->dm_io_in_prog, out_fd);

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
    if (an_grab_virt_chunk(0, pda->req_msg_ptr, &amg_msg, sizeof(amg_msg)) != 0)
       fprintf(stderr, "Couldn't get message\n");

    else
       {
       /* Message header info */
       fprintf(out_fd, "msg_descriptor       = ");
       an_lpr11_msg_descrpt(amg_msg.msg_header.msg_descriptor, out_fd); 
       fprintf(out_fd, "message_id           = %d\n", 
            amg_msg.msg_header.message_id); 
       fprintf(out_fd, "transaction_num      = 0x%08x\n", 
            amg_msg.msg_header.transaction_num); 
       }

    /* Message structure */
    aio_decode_message(pda->req_msg_ptr, "     ", out_fd);

    /* incoming subq and message class */
    fprintf(out_fd, "input_msg_subq       = %d\n", pda->input_msg_subq);
    fprintf(out_fd, "msg_class            = ");
    an_lpr11_class(pda->msg_class, out_fd);

    /* Compensate for a 2235A bug that lies about it being online event
       when it admits it is out of paper or not ready */
    if ((pda->dap_status.stat_hi_index & 0x02) ||
       ((pda->dap_status.stat_hi_index & 0x40)==0))
       pda->dap_status.stat_hi_index = (pda->dap_status.stat_hi_index & 0x7f);

    /* Device status type */
    stat = pda->dap_status.stat_hi_index;

    /* Printer off-line */
    if (!online(stat))
	fprintf(out_fd, "Device Status : Printer off-line\n");

    else {
    	if (online(stat))
	    fprintf(out_fd, "Device Status : Printer on-line\n");
        if (ready_for_data(stat))
	    fprintf(out_fd, "Device Status : Printer Ready for data\n");
    	if (paper_out(stat))
	    fprintf(out_fd, "Device Status : Printer paper-out\n");
    	if (power_fail(stat))
	    fprintf(out_fd, "Device Status : Printer powerfail\n");
    }

    fprintf(out_fd, "id.id_high           = 0x%08x\n", pda->id.id_high);
    fprintf(out_fd, "id.id_low    	  = 0x%08x\n", pda->id.id_low);
    fprintf(out_fd, "ldm_generic          = 0x%08x\n", pda->ldm_generic);

    /* States and actions */
    fprintf(out_fd, "amg_old_state        = ");
    an_lpr11_state(pda->amg_old_state, out_fd);
    
    fprintf(out_fd, "Current amg_state    = ");
    an_lpr11_state(pda->amg_state, out_fd);

    fprintf(out_fd, "state prior to powerfail p_on_state = ");
    an_lpr11_state(pda->p_on_state, out_fd);

    fprintf(out_fd,
            "Action DM was working on at powerfail p_on_action = ");
    an_lpr11_action(pda->p_on_action, out_fd);

    fprintf(out_fd, "p_on_class           = ");
    an_lpr11_class(pda->p_on_class, out_fd);

    /* transactions */
    fprintf(out_fd, "p_on_trn             = 0x%08x\n", pda->p_on_trn);
    fprintf(out_fd, "pfl_prog             = %d\n", pda->pfl_prog);

    aio_decode_llio_status(pda->llio_info, out_fd);

    fprintf(out_fd, "transfer_count       = %d\n", pda->transfer_count);
    fprintf(out_fd, "amg_dm_rev_level     = 0x%08x\n", pda->amg_dm_rev_level);
    fprintf(out_fd, "amg_subqs            = 0x%08x\n", pda->amg_subqs);
    fprintf(out_fd, "current transaction #= 0x%08x\n", pda->trn);
    fprintf(out_fd, "last trans # before PF = 0x%08x\n", pda->trn);
    fprintf(out_fd, "current message id   = %d\n", pda->mid);
    fprintf(out_fd, "timer_id             = %d\n", pda->timer_id);

    /* address and length of data for LDM */
    fprintf(out_fd, "ldm_data_len         = %d\n", pda->ldm_data_len);
    fprintf(out_fd, "retry_ct             = %d\n", pda->retry_ct);
  
    /* DAP - related information - must be 64 byte aligned. */
    fprintf(out_fd, "lp_log11.lp_log_pointer= %d\n",
        pda->lp_log11.lp_log_pointer);

    for (i = 0; i < 64; i++)
    	fprintf(out_fd, "lp_log11.lplog[%d] = 0x%08x\n", i, pda->lp_log11.lplog[i]);

    switch (pda->dap_status.halt_code) {
    case SUCCESS :     fprintf(out_fd, "dap_stat.halt_code = SUCCESS\n"); break;
    case TIMEOUT :     fprintf(out_fd, "dap_stat.halt_code = TIMEOUT\n"); break;
    case EARLY_READ :  fprintf(out_fd, "dap_stat.halt_code = EARLY_READ\n");
                       break;
    case GOT_STATUS_EARLY : fprintf(out_fd, 
                       "dap_stat.halt_code = GOT_STATUS_EARLY\n"); 
                       break;
    case DIAG_CLEAR :  fprintf(out_fd, "dap_stat.halt_code = DIAG_CLEAR\n");
                       break;
    case DIAG_ID :     fprintf(out_fd, "dap_stat.halt_code = DIAG_ID\n"); break;
    case DIAG_LOOP :   fprintf(out_fd, "dap_stat.halt_code = DIAG_LOOP\n");
                       break;
    case NO_READ :     fprintf(out_fd, "dap_stat.halt_code = NO_READ\n");  
                       break;
    }

    fprintf(out_fd, "dap_stat                        = 0x%08x\n",
        pda->dap_stat);
    fprintf(out_fd, "dap_status.dsj_index            = 0x%08x\n",
        pda->dap_status.dsj_index);
    fprintf(out_fd, "dap_status.stat_hi_index        = 0x%08x\n",
        pda->dap_status.stat_hi_index);
    fprintf(out_fd, "dap_status.stat_lo_index        = 0x%08x\n",
        pda->dap_status.stat_lo_index);
    fprintf(out_fd, "dap_status.misc_hi_index        = 0x%08x\n",
        pda->dap_status.misc_hi_index);
    fprintf(out_fd, "dap_status.misc_lo_index        = 0x%08x\n",
        pda->dap_status.misc_lo_index);
    fprintf(out_fd, "dap_status.hpib_id_high         = 0x%08x\n",
        pda->dap_status.hpib_id_high);
    fprintf(out_fd, "dap_status.hpib_id_low          = 0x%08x\n",
        pda->dap_status.hpib_id_low);
    fprintf(out_fd, "dap_parms[0]                    = 0x%08x\n",
        pda->dap_parms[0]);
    fprintf(out_fd, "dap_parms[1]                    = 0x%08x\n",
        pda->dap_parms[1]);
    fprintf(out_fd, "dap_area_ptr                    = 0x%08x\n",
        pda->dap_area_ptr);

    /* Buffer area for holding user data while adding blk and pckt hdr. */
    fprintf(out_fd, "current_length                  = %d\n",
            pda->buff_area.current_length);
    fprintf(out_fd, "start_buff_ptr                  = 0x%08x\n",
            pda->buff_area.start_buff_ptr);
    fprintf(out_fd, "current_buff_ptr                = 0x%08x\n",
            pda->buff_area.current_buff_ptr);

} /* an_lpr11_dm_info */

an_lpr11_all(pda, out_fd)
    struct amg_pda      *pda;
    FILE		*out_fd;
{
    an_lpr11_dam_info(pda, out_fd);
    an_lpr11_ldm_info(pda, out_fd);
    an_lpr11_dm_info(pda, out_fd);
}

an_lpr11_decode_status(llio_status, out_fd)
    llio_status_type	llio_status;
    FILE		*out_fd;
{
    char *s;

    fprintf(out_fd, "lpr11 llio status = 0x%08x -- (", llio_status);
    if (llio_status.u.subsystem == SUBSYS_LPR11)
	fprintf(out_fd, "SUBSYS_LPR11, ");
    else 
	fprintf(out_fd, "subsys %d?, ", llio_status.u.subsystem);

    if (llio_status.u.proc_num == -1)
	fprintf(out_fd, "LOCAL, ");

    else if (llio_status.u.proc_num == 0)
	fprintf(out_fd, "GLOBAL, ");

    else
	fprintf(out_fd, "%d?, ", llio_status.u.proc_num);

    switch (llio_status.u.error_num)  {
        case AMG_DM_NOT_ON_LINE :       s = "AMG_DM_NOT_ON_LINE";       break;
        case AMG_DM_PAPER_OUT :		s = "AMG_DM_PAPER_OUT";		break;
        case AMG_DM_PAPER_JAM :		s = "AMG_DM_PAPER_JAM";		break;
        case AMG_DM_PLATEN_OPEN : 	s = "AMG_DM_PLATEN_OPEN";	break;	
	case AMG_DM_RIBBON_FAIL : 	s = "AMG_DM_RIBBON_FAIL";	break;	
	case AMG_DM_SELF_TEST_FAIL :	s = "AMG_DM_SELF_TEST_FAIL";	break;
	case AMG_DM_POWER_FAIL : 	s = "AMG_DM_POWER_FAIL";	break;
	case AMG_DM_STATUS_INVALID :	s = "AMG_DM_STATUS_INVALID";	break;
	case AMG_DM_DEVICE_ADAPTER_FAILURE :
				   s = "AMG_DM_DEVICE_ADAPTER_FAILURE"; break; 
	case AMG_DM_READ_STATUS : 	s = "AMG_DM_READ_STATUS";	break;
	case AMG_DM_SYSTEM_POWERFAIL : 	s = "AMG_DM_SYSTEM_POWERFAIL";	break;
	case AMG_DM_HARDWARE_FAILURE : 	s = "AMG_DM_HARDWARE_FAILURE";	break;
	default:			s = "unrecognized error?";      break;
    }
    fprintf(out_fd, "%s)\n", s);
}

an_lpr11_help(out_fd)
    FILE		*out_fd;
{
    fprintf(out_fd, "\nvalid options:\n");
    fprintf(out_fd, "   dam     --  decode device adapter manager state\n");
    fprintf(out_fd, "   ldm     --  decode logical device manager state\n");
    fprintf(out_fd, "   dm      --  decode device manager state\n");
    fprintf(out_fd, "   all     --  do all of the above\n");
    fprintf(out_fd, "   help    --  print this help screen\n");
}



an_lpr11_decode_message(mp, prefix, out_fd)
    amg_message_type	        *mp;
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
        an_lpr11_decode_status(mp->u.do_bind_reply , out_fd);
        break;
    case BIND_REQ_MSG :
	fprintf(out_fd, "BIND_REQ_MSG\n");
        fprintf(out_fd, "reply_subq     = 0x%08x\n ",
                mp->u.bind_req.reply_subq);
        fprintf(out_fd, "hm_event_subq  = 0x%08x\n ",
                mp->u.bind_req.hm_event_subq);
        fprintf(out_fd, "hm_subsys_num  = %d\n ",
                mp->u.bind_req.hm_subsys_num);
        fprintf(out_fd, "hm_meta_lang   = %d\n ",
                mp->u.bind_req.hm_meta_lang);
        fprintf(out_fd, "hm_rev_code    = %d\n ",
                mp->u.bind_req.hm_rev_code);
	    
        break;
    case BIND_REPLY_MSG :
	fprintf(out_fd, "BIND_REPLY_MSG\n");
        fprintf(out_fd, "lm_rev_code          = %d\n",
                mp->u.bind_reply.lm_rev_code);
        fprintf(out_fd, "lm_queue_depth       = %d\n",
                mp->u.bind_reply.lm_queue_depth);
        fprintf(out_fd, "lm_low_req_subq      = %d\n ",
                mp->u.bind_reply.lm_low_req_subq);
        fprintf(out_fd, "lm_hi_req_subq       = %d\n ",
                mp->u.bind_reply.lm_hi_req_subq);
        fprintf(out_fd, "lm_freeze_data       = %s\n",
                BOOLEAN(mp->u.bind_reply.lm_freeze_data));
        fprintf(out_fd, "lm_alignment         = %s\n",
                BOOLEAN(mp->u.bind_reply.lm_alignment));
        an_lpr11_decode_status(mp->u.bind_reply.reply_status, out_fd);
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
        fprintf(out_fd, "reply_subq             = %d\n ",
                mp->u.dm_req.reply_subq);

        switch (mp->u.dm_req.data_class) {
            case VIRTUAL_BUFFER :
                fprintf(out_fd, "data_class             = VIRTUAL BUFFER\n");
            case VIRTUAL_BLOCKS :
                fprintf(out_fd, "data_class             = VIRTUAL BLOCKS\n");
            case CONTIGUOUS_REAL:
                fprintf(out_fd, "data_class             = CONTIGUOUS REAL\n");
        }
                
        fprintf(out_fd, "data_len               = %d\n ",
                mp->u.dm_req.data_len);
        break;
    case DM_IO_REPLY_MSG :
	fprintf(out_fd, "DM_IO_REPLY_MSG\n");
        
        an_lpr11_decode_status(mp->u.dm_reply.reply_status,out_fd);
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
} /* an_lpr11_decode_message */
