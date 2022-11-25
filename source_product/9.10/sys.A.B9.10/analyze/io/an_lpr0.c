/***************************************************************************/
/**                                                                       **/
/**   This is the lpr0 (Line printer LDM) i/o manager analysis routine.   **/
/**                                                                       **/
/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <h/types.h>
#include <h/lprio.h>        /* Printer LDM def.h */
#include <sio/llio.h>
#include <sio/iotree.h>
#include <sio/lpr0.h>       /* Printer LDM def.h */
#include <sio/lpr0_spc.h>   /* Printer LDM pda.h */
#include <sio/lp_dmmeta.h>  /* LPR0 LDM msg defs */
#include "aio.h"		               /* all analyze i/o defs */

int    lpr0_an_rev   = 2;	    /* analyze rev number of lpr0 DS   */
static char *me      = "lpr0";

#define	BOOLEAN(s)	((s) ? "true" : "false")
#define bit(x)  	(1 << (x))
#define	streq(a, b)	(strcmp((a), (b)) == 0)
#define	min(a, b)	(((a) < (b)) ? (a) : (b))
#define	BITS	(BYTE_MODE|CIOCA_S1S2|LOGCH_BREAK|BLOCKED|CONTINUE)

int an_lpr0(call_type, port_num, out_fd, option_string)
    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */
{
    int    		addr;		/* address of extern                  */
    int			code_rev;	/* revision of driver code	      */
    struct lp_pda       pda;		/* port data area                     */
    struct mgr_info_type mgr;		/* all generic manager info	      */

    int			an_lpr0_decode_message();
    int			an_lpr0_decode_status();

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
	    if (an_grab_extern("lpr0_an_rev", &addr) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, lpr0_an_rev,
				 out_fd);

            else {
	        if (an_grab_real_chunk(addr, &code_rev, sizeof(code_rev)) != 0)
		   aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, lpr0_an_rev,
				    out_fd);
	        if (code_rev != lpr0_an_rev)
		   aio_rev_mismatch(me, AIO_INCOMPAT_REVS, code_rev, 
				    lpr0_an_rev, out_fd);
            }

	    /* put my decoding routines into system lists        */
	    aio_init_llio_status_decode(SUBSYS_LPR0, an_lpr0_decode_status);
	    aio_init_message_decode(START_DM_MSG, START_DM_MSG+9,
	                            an_lpr0_decode_message);
	    break;

	case AN_MGR_BASIC:
	    an_lpr0_all(&pda, out_fd);
	    break;

	case AN_MGR_DETAIL:
	    an_lpr0_all(&pda, out_fd);
	    break;

	case AN_MGR_OPTIONAL:
	    an_lpr0_optional(&pda, out_fd, option_string);
	    break;

	case AN_MGR_HELP:
	    an_lpr0_help(stderr);
	    break;

    }
    return(0);

} /* an_lpr0 */


an_lpr0_optional(pda, out_fd, option_string)
    struct lp_pda       *pda;
    FILE		*out_fd;
    char		*option_string;
{
    char *token;
    int	 ldm_flag	= 0;
    int	 dm_flag	= 0;
    int	 all_flag	= 0;

    /* read through option string and set appropriate flags */
    if ((token = strtok(option_string, " ")) == NULL)  {
	an_lpr0_help(stderr);
	return;
    }

    do {
	if 	(streq(token,    "ldm"))   ldm_flag   = 1;
	else if (streq(token,     "dm"))   dm_flag    = 1;
	else if (streq(token,    "all"))   all_flag   = 1;
	else  {
	    fprintf(stderr, "invalid option <%s>\n", token);
	    an_lpr0_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (ldm_flag)   an_lpr0_ldm_info(pda, out_fd);
    if (dm_flag)    an_lpr0_dm_info(pda, out_fd);
    if (all_flag)   an_lpr0_all(pda, out_fd);
}

an_lpr0_dm_info(pda, out_fd)
    struct lp_pda       *pda;
    FILE		*out_fd;
{

    struct mgr_info_type mgr;		/* all generic manager info	      */

    /* Get manager information */
    if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, pda->p_dm_port, &mgr) != AIO_OK) {
       fprintf(out_fd,"Problem -- bad port number!\n");
       return(0);
       }

    else {
       fprintf(out_fd, "====DM Manager Info ====\n");
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

} /* an_lpr0_dm_info */

an_lpr0_state(state, out_fd)
    int                 state;
    FILE                *out_fd;
{
    char                *s;
    switch(state) {
        case LDM_PRIMORDIAL : 	s = "LDM_PRIMORDIAL"; 	  break;  	
        case LDM_CREATED :  	s = "LDM_CREATED";        break;
        case LDM_BINDING :	s = "LDM_BINDING";	  break;
        case LDM_NORMAL : 	s = "LDM_NORMAL";         break;		
        case LDM_UNBOUND :      s = "LDM_UNBOUND";        break;
        default    :            s = "UNKNOWN PORT STATE"; break;  /* invalid */
    } /* switch */
    fprintf(out_fd," %s\n",s);

} /* an_lpr0_state */


an_lpr0_msg_descrpt(msg_descrpt, out_fd)
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

} /* an_lpr0_msg_descrpt */


an_lpr0_ldm_info(pda, out_fd)
    struct lp_pda       *pda;
    FILE		*out_fd;
{
    struct lprio        *pri = &pda->p_lpr;
    struct lppos        *pos = &pda->p_lpp;
    struct mgr_info_type 	mgr;
    register int        port_state; 

    fprintf(out_fd, "\n======LPR0 LDM INFO==========\n");
    fprintf(out_fd, "p_dev          = 0x%08x\n", pda->p_dev); 		
    fprintf(out_fd, "p_flags        = 0x%08x\n", pda->p_flags); 		
    fprintf(out_fd, "p_hwstatus     = 0x%08x\n", pda->p_hwstatus); 		
    fprintf(out_fd, "p_tid          = 0x%08x\n", pda->p_tid); 		
    fprintf(out_fd, "p_mid          = 0x%08x\n", pda->p_mid); 		
    fprintf(out_fd, "p_power_tid    = 0x%08x\n", pda->p_power_tid); 		
    fprintf(out_fd, "p_power_mid    = 0x%08x\n", pda->p_power_mid); 		
    fprintf(out_fd, "p_lpr.ind      = %d\n", pri->ind);
    fprintf(out_fd, "p_lpr.col      = %d\n", pri->col);
    fprintf(out_fd, "p_lpr.line     = %d\n", pri->line);
    fprintf(out_fd, "p_lpr.bksp     = %d\n", pri->bksp);
    fprintf(out_fd, "p_lpr.open_ej  = %d\n", pri->open_ej);
    fprintf(out_fd, "p_lpr.close_ej = %d\n", pri->close_ej);
    fprintf(out_fd, "p_lpr.raw_mode = %d\n", pri->raw_mode);
    fprintf(out_fd, "p_lpp.p_ccc    = %d\n", pos->p_ccc);
    fprintf(out_fd, "p_lpp.p_mcc    = %d\n", pos->p_mcc);
    fprintf(out_fd, "p_lpp.p_mlc    = %d\n", pos->p_mlc);
    fprintf(out_fd, "p_subq         = %d\n", pda->p_subq);
    fprintf(out_fd, "port_state     = ");
    port_state = pda->p_pstate;
    an_lpr0_state(port_state, out_fd); 

    aio_decode_llio_status(pda->p_llio_status, out_fd);
    fprintf(out_fd, "p_reply_subq   = %d\n", pda->p_reply_subq);
    fprintf(out_fd, "p_error        = 0x%08x\n", pda->p_error); 		
    fprintf(out_fd, "p_count        = 0x%08x\n", pda->p_count); 		
    fprintf(out_fd, "p_save_count   = 0x%08x\n", pda->p_save_count); 		
    fprintf(out_fd, "p_ignore_events= 0x%08x\n", pda->p_ignore_events);
    fprintf(out_fd, "p_timer_id     = 0x%08x\n", pda->p_timer_id);
    fprintf(out_fd, "p_get_stat     = 0x%08x\n", pda->p_get_stat);
    fprintf(out_fd, "p_in_midst     = 0x%08x\n", pda->p_in_midst);
    fprintf(out_fd, "p_save_n       = 0x%08x\n", pda->p_save_n);
    fprintf(out_fd, "p_save_cp      = 0x%08x\n", pda->p_save_cp);
    fprintf(out_fd, "p_dm_rev       = 0x%08x\n", pda->p_dm_rev);
    fprintf(out_fd, "p_rawcount     = 0x%08x\n", pda->p_rawcount);
    fprintf(out_fd, "p_blocksize    = 0x%08x\n", pda->p_blocksize);
 
    /* Get DM manager information */
    if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, pda->p_dm_port, &mgr) != AIO_OK) {
       fprintf(out_fd,"Problem -- bad port number!\n");
       return(0);
       }

    else {
       fprintf(out_fd, "==== DM Manager Info ====\n");
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
} /* an_lpr0_ldm_info */


an_lpr0_all(pda, out_fd)
    struct lp_pda      *pda;
    FILE		*out_fd;
{
    an_lpr0_ldm_info(pda, out_fd);
    an_lpr0_dm_info(pda, out_fd);

} /* an_lpr0_all  */

an_lpr0_decode_status(llio_status, out_fd)
    llio_status_type	llio_status;
    FILE		*out_fd;
{
    char *s;

    fprintf(out_fd, "lpr0 llio status = 0x%08x -- (", llio_status);
    if (llio_status.u.subsystem == SUBSYS_LPR0)
	fprintf(out_fd, "SUBSYS_LPR0, ");
    else 
	fprintf(out_fd, "subsys %d?, ", llio_status.u.subsystem);

    if (llio_status.u.proc_num == -1)
	fprintf(out_fd, "LOCAL, ");

    else if (llio_status.u.proc_num == 0)
	fprintf(out_fd, "GLOBAL, ");

    else
	fprintf(out_fd, "%d?, ", llio_status.u.proc_num);

    switch (llio_status.u.error_num)  {
        case CPR_DM_NOT_ONLINE :        s = "CPR_DM_NOT_ONLINE";        break;
        case CPR_DM_PAPER_OUT :		s = "CPR_DM_PAPER_OUT";		break;
	case CPR_DM_PAPER_JAM :		s = "CPR_DM_PAPER_JAM";		break;
	case CPR_DM_PLATEN_OPEN : 	s = "CPR_DM_PLATEN_OPEN";	break;
	case CPR_DM_RIBBON_FAIL :	s = "CPR_DM_RIBBON_FAIL";	break;
	case CPR_DM_SELF_TEST_FAIL : 	s = "CPR_DM_SELF_TEST_FAIL";	break;
	case CPR_DM_POWER_FAIL :	s = "CPR_DM_POWER_FAIL";	break;
	default:			s = "unrecognized error?";      break;
    }
    fprintf(out_fd, "%s)\n", s);
}

an_lpr0_help(out_fd)
    FILE		*out_fd;
{
    fprintf(out_fd, "\nvalid options:\n");
    fprintf(out_fd, "   ldm     --  decode logical device manager state\n");
    fprintf(out_fd, "   dm      --  decode device manager state\n");
    fprintf(out_fd, "   all     --  do all of the above\n");
    fprintf(out_fd, "   help    --  print this help screen\n");
}


an_lpr0_decode_message(mp, prefix, out_fd)
    lp_message_type	        *mp;
    char 			*prefix;
    FILE			*out_fd;
{
    llio_std_header_type *l = &mp->msg_header;

    /* Message structure */
    switch(l->msg_descriptor) {
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
        
        an_lpr0_decode_status(mp->u.dm_reply.reply_status,out_fd);
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
} /* an_lpr0_decode_message */
