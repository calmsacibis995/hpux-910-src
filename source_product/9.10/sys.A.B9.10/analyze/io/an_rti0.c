/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/io/RCS/an_rti0.c,v $
 * $Revision: 1.1.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:26:35 $
 */

#ifdef OLDIOSERV
an_rti0()
{
}

#else
/***************************************************************************/
/**                                                                       **/
/**   This is the RTI Real Time Interface i/o manager analysis routine.   **/
/**                                                                       **/
/***************************************************************************/

#define SUBSYS_RTI0     254

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sio/llio.h>
#include <h/types.h>
#include <sio/iotree.h>
#include <h/buf.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include "an_rti.h"
#include "aio.h"			        /* all analyze i/o defs*/

int    rti0_an_rev  = 1;	/* global -- rev number of rti0 DS   */
static char *me      = "rti0";

#define	BOOLEAN(s)	((s) ? "true" : "false")
#define	streq(a, b)	(strcmp((a), (b)) == 0)
#define BITS    (BYTE_MODE|CIOCA_S1S2|LOGCH_BREAK|BLOCKED|CONTINUE)

int an_rti0(call_type, port_num, out_fd, option_string)
    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */
{
    int    		addr;		/* address of extern                  */
    int			code_rev;	/* revision of driver code	      */
    rti0_pda_type       pda;		/* port data area                     */
    struct mgr_info_type mgr;		/* all generic manager info	      */

    int			an_rti0_decode_message();
    int			an_rti0_decode_status();

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
	    if (an_grab_extern("rti0_an_rev", &addr) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, rti0_an_rev,
				 out_fd);

            else {
	        if (an_grab_real_chunk(addr, &code_rev, sizeof(code_rev)) != 0)
		   aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, rti0_an_rev,
				    out_fd);
	        if (code_rev != rti0_an_rev)
		   aio_rev_mismatch(me, AIO_INCOMPAT_REVS, code_rev, 
				    rti0_an_rev, out_fd);
            }

	    /* put my decoding routines into system lists        */
	    aio_init_llio_status_decode(SUBSYS_RTI0, an_rti0_decode_status);
	    aio_init_message_decode(START_CIO_MSG, START_CIO_MSG+9,
	                            an_rti0_decode_message);
	    break;

	case AN_MGR_BASIC:
	    an_rti0_pda(&pda, out_fd);
	    break;

	case AN_MGR_DETAIL:
	    an_rti0_all(&pda, out_fd);
	    break;

	case AN_MGR_OPTIONAL:
	    an_rti0_optional(&pda, out_fd, option_string);
	    break;

	case AN_MGR_HELP:
	    an_rti0_help(stderr);
	    break;

    }
    return(0);

} /* an_rti0 */

/****************************************************************************
 * an_rti0_optional handles the parsing of options to the RTI analysis
 * routine.  
 *
 * The current valid options are:
 *
 *	pda - Print out the port data information
 *
 *	procs - print out information on all current active processes in 
 *		the driver.  An active process is a slot with a positive
 *              process id.
 *
 *	daemon - Print out the rtidaemon structure which includes rtipanic 
 *		 messages and a list of processes to delete.
 *
 *	prefix - Print out any prefixes that are in the queue to be processed,
 *		 but have not been looked at yet.
 *
 *	io - Print out the virtual quads the driver uses.
 *
 *	help - Print out the help screen.
 *****************************************************************************/
an_rti0_optional(pda, out_fd, option_string)
    rti0_pda_type      *pda;
    FILE		*out_fd;
    char		*option_string;
{
    char *token;
    int pda_flag = 0,
	procs_flag = 0,
	daemon_flag = 0,
	io_flag = 0,
	prefix_flag = 0,
	help_flag = 0,
	all_flag = 0;

    /* read through option string and set appropriate flags */
    token = strtok(option_string, " ");

    do {
	if      (streq(token,    "pda"))   pda_flag   = 1;
	else if (streq(token,    "daemon")) daemon_flag   = 1;
	else if (streq(token,    "prefix")) prefix_flag   = 1;
	else if (streq(token,    "procs")) procs_flag   = 1;
	else if (streq(token,    "io"))    io_flag   = 1;
	else if (streq(token,    "help"))  help_flag   = 1;
	else if (streq(token,    "all"))   all_flag   = 1;
	else  {
	    fprintf(stderr, "invalid option <%s>\n", token);
	    an_rti0_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    if (all_flag) {
	an_rti0_all(pda,out_fd);
	return;
    }
    if (pda_flag) an_rti0_pda(pda,out_fd);
    if (daemon_flag) an_rti0_daemon(pda,out_fd);
    if (prefix_flag) an_rti0_prefix(pda,out_fd);
    if (io_flag) an_rti0_io(pda,out_fd);
    if (procs_flag) an_rti0_procs(pda,out_fd);
    if (help_flag) an_rti0_help(out_fd);

}

/******************************************************************************
 * an_rti0_state prints out the state of the RTI driver.  The state is the
 * state of a current transfer if any.  The state of the driver is only 
 * a concern of the module rti0lmrepl.c
 *****************************************************************************/
an_rti0_state(state, out_fd)
    int                 state;
    FILE                *out_fd;
{
    char                *s;
    switch(state) {
        case CREATION :    s = "CREATION"; 		break;  	/* 0 */
        case BIND_LOWER :  s = "BIND_LOWER"; 		break;  	/* 1 */
	case UNBIND_LOWER: s = "UNBIND_LOWER";		break;		/* 11 */
        case IDENT_WAIT :  s = "IDENT_WAIT";       	break;  	/* 2 */
        case INIT_WAIT4RTIP :  s = "INIT_WAIT4RTIP";    break;  	/* 3 */
        case IO_REQ_READY :    s = "IO_REQ_READY";      break;  	/* 4 */
        case WAIT4DMA :         s = "WAIT4DMA";   	break;  	/* 5 */
        case WAIT4PREFIXQ :    	s = "WAIT4PREFIXQ";   	break;  	/* 6 */
        case SWAIT4ENTERBSL :  	s = "SWAIT4ENTERBSL";   break;  	/* 7 */
        case HWAIT4ENTERBSL :   s = "HWAIT4ENTERBSL";   break;  	/* 8 */
        case WAIT4DIAGJUMP :    s = "WAIT4DIAGJUMP";  	break;  	/* 9 */
        case WAIT4ERRSTRING :   s = "WAIT4ERRSTRING"	;break;         /* 10 */
        default    :            s = "UNKNOWN STATE";	break;    /* invalid */
    } /* switch */
    fprintf(out_fd,"%s",s);

} /* an_rti0_state */

/****************************************************************************
 * an_rti0_msg_descrpt() prints out a string that corresponds to the 
 * message descriptor valus passed in.
 ****************************************************************************/

an_rti0_msg_descrpt(msg_descrpt, out_fd)
    int                 msg_descrpt;
    FILE                *out_fd;
{
    char                *s;
    switch(msg_descrpt) {
        case ABORT_EVENT_MSG :   s = "ABORT_EVENT_MSG";       break;
        case CREATION_MSG :      s = "CREATION_MSG";          break;
        case DO_BIND_REQ_MSG :   s = "DO_BIND_REQ_MSG";       break;
        case DO_BIND_REPLY_MSG : s = "DO_BIND_REPLY_MSG";     break;
        case BIND_REQ_MSG :      s = "BIND_REQ_MSG";          break;
        case BIND_REPLY_MSG :    s = "BIND_REPLY_MSG";        break;
        case DIAG_REQ_MSG :      s = "DIAG_REQ_MSG";          break;
        case DIAG_REPLY_MSG :    s = "DIAG_REPLY_MSG";        break;
        case DIAG_EVENT_MSG :    s = "DIAG_EVENT_MSG";        break;
        case CIO_DMA_IO_REQ_MSG :s = "CIO_DMA_IO_REQ_MSG";    break;
        case CIO_DMA_IO_REPLY_MSG : s = "CIO_DMA_IO_REPLY_MSG";	break;
        case CIO_CTRL_REQ_MSG :    s = "CIO_CTRL_REQ_MSG";    	break;
        case CIO_CTRL_REPLY_MSG :  s = "CIO_CTRL_REPLY_MSG";	break; 
        case CIO_IO_EVENT_MSG :    s = "CIO_IO_EVENT_MSG";    	break;
        case CIO_INTERRUPT_MSG :   s = "CIO_INTERRUPT_MSG";  	break; 
        case CIO_POLL_DMA_REQ_MSG : s = "CIO_POLL_DMA_REQ_MSG";	break;
        case CIO_POLL_ASYNC_MSG :  s = "CIO_POLL_ASYNC_MSG";   	break;
        case CIO_DMA_DUMP_REQ_MSG : s = "CIO_DMA_DUMP_REQ_MSG";	break;

        default :                s = "UNKNOWN MSG DESCRIPTOR"; break;/*invalid*/
    }
    fprintf(out_fd," %s\n",s);

}

/****************************************************************************
 * an_rti0_pda() prints out all the information dealing with the pda.
 * 		 Just the constant information is displayed.  Any structures
 *		 contained in the pda are displayed with other options such
 *		 as "io" displaying vquads.
 ****************************************************************************/
an_rti0_pda(pda, out_fd)
    rti0_pda_type      *pda;
    FILE		*out_fd;
{
	fprintf(out_fd,"Port Data Area for rti0(%d)\n",pda->lu);
	fprintf(out_fd,"\tfoundcard(%d)            ",  pda->foundcard);
	if (pda->foundcard)
		fprintf(out_fd,"RTI card is present\n");
	else
		fprintf(out_fd,"No RTI card present\n");
	fprintf(out_fd,"\tstate(");
	an_rti0_state(pda->state,out_fd);
	fprintf(out_fd,")\n");
	fprintf(out_fd,"\tdiag_result.s.selftest(0x%4x)      ",
		pda->diag_result.s.selftest);
	if (pda->diag_result.s.selftest != 0x5555) 
		fprintf(out_fd,"Selftest Failed\n");
	else
		fprintf(out_fd,"Selftest Passed\n");

	fprintf(out_fd,"\tdiag_result.s.TOPselftest(0x%4x)      \n",
		pda->diag_result.s.TOPselftest);

	fprintf(out_fd,"\tmgr_index(%d)            \n",pda->mgr_index);
	fprintf(out_fd,"\tbound_to_cam(%d)         \n",pda->bound_to_cam);
	fprintf(out_fd,"\tdmaflags(0x%x)           ",pda->dmaflags);
	if (pda->dmaflags & DMAIO_BUSY) fprintf(out_fd,"DMAIO_BUSY ");
	if (pda->dmaflags & DMAIO_WANTED) fprintf(out_fd,"DMAIO_WANTED");
	fprintf(out_fd,"\n");
	fprintf(out_fd,"\trti_dead(%d)             ",pda->rti_dead);
	if (pda->rti_dead == 1) 
		fprintf(out_fd,"RTI Card is dead\n");
	else
		fprintf(out_fd,"RTI Card is alive\n");
	fprintf(out_fd,"\tcontrolopen(%d)          ",pda->controlopen);
	if (pda->controlopen)
		fprintf(out_fd,"Control File Open\n");
	else
		fprintf(out_fd,"Control File Closed\n");
	fprintf(out_fd,"\tcpuopen(%d)              ",pda->cpuopen);
	if (pda->cpuopen)
		fprintf(out_fd,"Cpu File Open\n");
	else
		fprintf(out_fd,"Cpu File Closed\n");
	fprintf(out_fd,"\tprobeopen(%d)            ",pda->probeopen);
	if (pda->probeopen)
		fprintf(out_fd,"Probe File Open\n");
	else
		fprintf(out_fd,"Probe File Closed\n");
	fprintf(out_fd,"\tinbootstrap(%d)          ",pda->inbootstrap);
	if (pda->inbootstrap)
		fprintf(out_fd,"Card In Boot Strap Loader\n");
	else
		fprintf(out_fd,"Card Executing pSOS\n");
	fprintf(out_fd,"\tinitialized(%d)          ",pda->initialized);
	if (pda->initialized)
		fprintf(out_fd,"Card is initialized\n");
	else
		fprintf(out_fd,"Card is NOT initialized\n");
	fprintf(out_fd,"\tdorequestqueue(%d)       ",pda->dorequestqueue);
	if (pda->dorequestqueue)
		fprintf(out_fd,"Need to grab RTI queue\n");
	else
		fprintf(out_fd,"RTI Queue empty\n");
	fprintf(out_fd,"\tsignaledaemon(%d)        ",pda->signaledaemon);
	if (pda->signaledaemon)
		fprintf(out_fd,"RTI daemon signaled\n");
	else
		fprintf(out_fd,"NO request for RTI daemon\n");
	fprintf(out_fd,"\tDMAfree.pp_index(0x%x)   ",pda->DMAfree.pp_index);
	if (pda->DMAfree.valid)
		fprintf(out_fd,"Active Process transfer\n");
	else 
		fprintf(out_fd,"No Process transfer\n");
	fprintf(out_fd,"\tDMAfree.dmabuflag(0x%x)  ",pda->DMAfree.dmabuflag);
	if (pda->DMAfree.valid) {
		fprintf(out_fd,"Process direction: ");
		switch (pda->DMAfree.dmabuflag) {
			case HOST: fprintf(out_fd,"HOST\n");
			case RTI: fprintf(out_fd,"RTI\n");
			default: fprintf(out_fd,"UNKNOWN\n");
		}
	}
	else
		fprintf(out_fd,"\n");
	fprintf(out_fd,"\tDMAfree.valid(%d)        ",pda->DMAfree.valid);
	if (pda->DMAfree.valid)
		fprintf(out_fd,"Active Process transfer\n");
	else 
		fprintf(out_fd,"No Process transfer\n");
	fprintf(out_fd,"\tDMAwakeup(0x%x)          ",pda->DMAwakeup);
	if (pda->DMAwakeup) 
		fprintf(out_fd,"Process Sleeping on DMA\n");
	else
		fprintf(out_fd,"No sleeping process\n");
	fprintf(out_fd,"\tDMAbiodone(0x%x)         ",pda->DMAbiodone);
	if (pda->DMAbiodone) 
		fprintf(out_fd,"Process Sleeping on buf structure\n");
	else
		fprintf(out_fd,"No sleeping process\n");
	fprintf(out_fd,"\tdmaioctlsleepflag(%d)    ",pda->dmaioctlsleepflag);
	if (pda->DMAbiodone) 
		fprintf(out_fd,"Process performing ioctl\n");
	else
		fprintf(out_fd,"No active ioctl's\n");
	fprintf(out_fd,"\ttarget_addr(0x%x)        \n",pda->target_addr);
	fprintf(out_fd,"\tlu(%d)                   \n", pda->lu);
	fprintf(out_fd,"\txtracio(%d)              ", pda->xtracio);
	if (pda->xtracio)
		fprintf(out_fd,"CIO Card\n");
	else 
		fprintf(out_fd,"\n");
	fprintf(out_fd,"\ttimer_id(%d)             \n",  pda->timer_id);
	fprintf(out_fd,"\treusemsg(0x%x)           \n",  pda->reusemsg);
	fprintf(out_fd,"\tpf_inprogress(%d)        ",  pda->pf_inprogress);
	if (pda->pf_inprogress)
		fprintf(out_fd,"Card in powerfail mode\n");
	else 	
		fprintf(out_fd,"\n");
	fprintf(out_fd,"\treuseframe(%d)           \n",  pda->reuseframe);
	fprintf(out_fd,"\tsyncwithprobe(%d)        ",  pda->syncwithprobe);
	if (pda->syncwithprobe)
		fprintf(out_fd,"Waiting for pROBE break response\n");
	else 
		fprintf(out_fd,"No break command\n");
	fprintf(out_fd,"\thw_addr_1(%d)       \n", pda->hw_addr_1);
	fprintf(out_fd,"\tdo_bind_msg_id(%d)  \n", pda->do_bind_msg_id);
	fprintf(out_fd,"\tmessage_id(%d)      \n", pda->message_id);
	fprintf(out_fd,"\ttransaction_num(%d) \n", pda->transaction_num);
	fprintf(out_fd,"\tdiag_port(%d)       \n", pda->diag_port);
	fprintf(out_fd,"\tconfig_port(%d)     \n", pda->config_port);
	fprintf(out_fd,"\tmy_port(%d)         \n", pda->my_port);
	fprintf(out_fd,"\tlm_port(%d)         \n", pda->lm_port);
	fprintf(out_fd,"\tlm_req_subq(%d)     \n", pda->lm_req_subq);
	fprintf(out_fd,"\tFirst RTIxfer(0x%x) \n", pda->driver_log.preuiomove_log_counter);
	fprintf(out_fd,"\tNext RTIxfer(0x%x) \n", pda->driver_log.log_counter);
	/*
	fprintf(out_fd,"\trtimemsize(0x%x)         \n",pda->rtimemsize);
	fprintf(out_fd,"\tQ_size(0x%x)             \n",pda->Q_size);
	fprintf(out_fd,"\tcpuid(0x%x)              \n",pda->cpuid);
	*/
}

/****************************************************************************
 * an_rti0_prefix() displays all the outstanding prefixs that the driver has 
 *		    read from the backplane driver, but has not yet processed.
 ****************************************************************************/
an_rti0_prefix(pda, out_fd)
    rti0_pda_type      *pda;
    FILE		*out_fd;
{
	struct RTIQelement *ptr, tprefix;

	/* Loop reading each prefix queued for processing. */
	ptr = pda->RTIQ;
	fprintf(out_fd,"Prefixes not yet processed\n\n");
	while (ptr != NULL) {
		if (an_grab_virt_chunk(0,ptr,&tprefix,sizeof(tprefix)) != 0) {
				fprintf(out_fd,"Unable to read prefix\n");
				ptr = NULL;
		}
		else {
			write_Q_element(&tprefix, out_fd);
			ptr = tprefix.link;
		}
	}
}

/****************************************************************************
 * an_rti0_daemon() displays the daemon structure used for communication 
 *		    between the driver and the rtidaemon.  Displayed are any 
 *		    processes that are to be deleted and any panic messages
 *		    sent up by the RTI card.
 ****************************************************************************/
an_rti0_daemon(pda, out_fd)
    rti0_pda_type      *pda;
    FILE		*out_fd;
{
	int i;

	/* Print out all info concerning panic messages to the daemon and
	   and processes slated for death.
	*/
	fprintf(out_fd,"Number of processes to delete (%d)\n",pda->daemon.num_minor);
	fprintf(out_fd,"\t Pid      Minor Lsb\n");
	for (i=0;i<pda->daemon.num_minor;i++) {
		fprintf(out_fd,"\t0x%x      0x%x\n",
			  pda->daemon.pid[i],
			  pda->daemon.minor_lsb[i]);
	}

	fprintf(out_fd,"\n");
	fprintf(out_fd,"Number of RTI error messages (%d)\n", 
			pda->daemon.numRTIerror);
	fprintf(out_fd,"Overflow flag (%d)\n", pda->daemon.overflow);
	for (i=0;i<pda->daemon.numRTIerror;i++) {
		switch (pda->daemon.err[i].type) {

			case '0':
				fprintf(out_fd,"\tERRPANIC ");
				fprintf(out_fd," pram(%d)",
					pda->daemon.err[i].pram);
				fprintf(out_fd,"  \"%s\"\n",
					pda->daemon.err[i].u.message);
				break;
			case '1':
				fprintf(out_fd,"\tERRINVTECT ");
				fprintf(out_fd," pram(%d)",
					pda->daemon.err[i].pram);
				fprintf(out_fd," segment(0x%x) ",
					pda->daemon.err[i].u.interrupt.segment);
				fprintf(out_fd," offset(0x%x)\n",
					pda->daemon.err[i].u.interrupt.offset);
				break;
			case '2':
				fprintf(out_fd,"\tERRDMATO ");
				fprintf(out_fd," pram(%d)\n",
					pda->daemon.err[i].pram);
				break;
			case '3':
				fprintf(out_fd,"\tERRINTERNAL ");
				fprintf(out_fd," pram(%d)",
					pda->daemon.err[i].pram);
				fprintf(out_fd,"  \"%s\"\n",
					pda->daemon.err[i].u.message);
				break;
			case '4':
				fprintf(out_fd,"\tERRBADCARD ");
				fprintf(out_fd," pram(%d)\n",
					pda->daemon.err[i].pram);
				break;
			default:
				fprintf(out_fd,"\tUnknown Type (%d)",
						pda->daemon.err[i].type);
				fprintf(out_fd," pram(%d)\n",
					pda->daemon.err[i].pram);
		}
	}
}

/****************************************************************************
 * an_rti0_io() Displays the contents of all vquads used by the RTI driver.
 ****************************************************************************/
an_rti0_io(pda, out_fd)
    rti0_pda_type      *pda;
    FILE		*out_fd;
{
	/* Print out the I/O state and The fields of each vquad along with 
	   the cpubuf.
	*/
	fprintf(out_fd,"rti0 Virtual Quads\n");

	fprintf(out_fd,"\tVquad1 (Prefix) \n");
	an_rti0_vquad(pda->vquad1, out_fd);

	fprintf(out_fd,"\tVquad2 (Prefix Queue) \n");
	an_rti0_vquad(pda->vquad2, out_fd);

	fprintf(out_fd,"\tVquad_data (RTI Data) \n");
	an_rti0_vquad(pda->vquad_data, out_fd);
}

/*****************************************************************************
 * an_rti0_procs()
 *****************************************************************************/
an_rti0_procs(pda, out_fd)
    rti0_pda_type      *pda;
    FILE		*out_fd;
{
	char type[6];
	int i, j;
	struct RTIQelement telement;

   for(i=0; i<=RTIMAXPROCS; i++) {
	if (pda->rtiproc[i].psospid) {
		fprintf(out_fd,"\n\nrti0 process in slot [%d]  \n", i);
		fprintf(out_fd,"\tpsospid(0x%x)            \n", pda->rtiproc[i].psospid);
		fprintf(out_fd,"\tiomode(");
		write_IO_mode(pda->rtiproc[i].iomode, out_fd);
		fprintf(out_fd,")\n");
		fprintf(out_fd,"\tsignalflags(0x%x)        ", 
				pda->rtiproc[i].signalflags);
		if (pda->rtiproc[i].writev_flags & RTISDEATH) 
				fprintf(out_fd,"DEATH ");
		if (pda->rtiproc[i].writev_flags & RTISMESG) 
				fprintf(out_fd,"MESSAGE");
		fprintf(out_fd,"\n");
		fprintf(out_fd,"\twritev_flags(0x%x)       ",
				pda->rtiproc[i].writev_flags);
		if (pda->rtiproc[i].writev_flags & WRITEV_BUSY)
				fprintf(out_fd,"WRITEV_BUSY ");
		if (pda->rtiproc[i].writev_flags & WRITEV_WANTED)
				fprintf(out_fd,"WRITEV_WANTED ");
		fprintf(out_fd,"\n");
		fprintf(out_fd,"\treadv_flags(0x%x)        ",
				pda->rtiproc[i].readv_flags);
		if (pda->rtiproc[i].readv_flags & READV_BUSY)
			fprintf(out_fd,"READV_BUSY ");
		if (pda->rtiproc[i].readv_flags & READV_WANTED)
			fprintf(out_fd,"READV_WANTED ");
		fprintf(out_fd,"\n");
		fprintf(out_fd,"\tpdying(%d)               ", 
				pda->rtiproc[i].pdying);
		if (pda->rtiproc[i].pdying) 
			fprintf(out_fd,"Process Dying\n");
		else
			fprintf(out_fd,"Process Active\n");
		fprintf(out_fd,"\taborting(%d)             ", 
				pda->rtiproc[i].aborting);
		if (pda->rtiproc[i].aborting)
			fprintf(out_fd,"Process is Aborting Request\n");
		else
			fprintf(out_fd,"Process is NOT Aborting Request\n");
		fprintf(out_fd,"\tabort_flags(0x%x)        ", 
				pda->rtiproc[i].abort_flags);
		if (pda->rtiproc[i].abort_flags & RTIMEDOUT)
			fprintf(out_fd,"RTIMEDOUT ");
		if (pda->rtiproc[i].abort_flags & RABORTED)
			fprintf(out_fd,"RABORTED");
		fprintf(out_fd,"\n");
		fprintf(out_fd,"\tptoldaemon(%d)           ", 
				pda->rtiproc[i].ptoldaemon);
		if (pda->rtiproc[i].ptelldaemon)
			fprintf(out_fd,"Daemon informed of process death\n");
		else 
			fprintf(out_fd,"\n");
		fprintf(out_fd,"\tptelldaemon(%d)          ", 
				pda->rtiproc[i].ptelldaemon);
		if (pda->rtiproc[i].ptelldaemon)
			fprintf(out_fd,"Tell Daemon process death\n");
		else 
			fprintf(out_fd,"\n");
		fprintf(out_fd,"\tphaltedcard(%d)          ", 
				pda->rtiproc[i].phaltedcard); 
		if (pda->rtiproc[i].phaltedcard)
			fprintf(out_fd,"RTI Card was halted\n");
		else
			fprintf(out_fd,"\n");
		fprintf(out_fd,"\tprocopen(%d)             ", 
				pda->rtiproc[i].procopen); 
		if (pda->rtiproc[i].procopen)
			fprintf(out_fd,"Process file is open\n");
		else
			fprintf(out_fd,"Process file is closed\n");

		fprintf(out_fd,"Signal Information for process [%d]\n",i);
		fprintf(out_fd,"\tFlags:                   ");
		if (pda->rtiproc[i].signalinfo.u.signal.RTISMESGflag)
			fprintf(out_fd,"RTISMESGflag ");
		if (pda->rtiproc[i].signalinfo.u.signal.RTISDEATHflag)
			fprintf(out_fd,"RTISDEATHflag");
		fprintf(out_fd,"\n");
		fprintf(out_fd,"\tmesgoverflow(%d)         ", 
			pda->rtiproc[i].signalinfo.u.signal.mesgoverflow); 
		if (pda->rtiproc[i].signalinfo.u.signal.mesgoverflow)
			fprintf(out_fd,"Message Queue Overflowed\n");
		else
			fprintf(out_fd,"\n");
		if (pda->rtiproc[i].signalinfo.u.signal.mesg1valid)
			fprintf(out_fd,"\tMessage 1 (%d)\n",
				pda->rtiproc[i].signalinfo.u.signal.mesg1); 
		else 
			fprintf(out_fd,"\tMessage one is invalid\n");
		if (pda->rtiproc[i].signalinfo.u.signal.mesg2valid)
			fprintf(out_fd,"\tMessage 2 (%d)\n",
				pda->rtiproc[i].signalinfo.u.signal.mesg2); 
		else 
			fprintf(out_fd,"\tMessage two is invalid\n");
		if (pda->rtiproc[i].signalinfo.u.signal.mesg3valid)
			fprintf(out_fd,"\tMessage 3 (%d)\n",
				pda->rtiproc[i].signalinfo.u.signal.mesg3); 
		else 
			fprintf(out_fd,"\tMessage three is invalid\n");

		for(j=0; j<2; j++) {
			if (j == 0)  {
				fprintf(out_fd,"HOST ppda & Request info\n");	
			}
			else	 {
				fprintf(out_fd,"RTI ppda & Request info\n");	
			}
			fprintf(out_fd,"\theadptr(0x%x)          \n",
				pda->rtiproc[i].headptr[j]);
			fprintf(out_fd,"\ttailptr(0x%x)          \n",
					pda->rtiproc[i].tailptr[j]);
			fprintf(out_fd,"\twriteaddr(0x%x)        \n",
					pda->rtiproc[i].writeaddr[j]);
			fprintf(out_fd,"\tendaddr(0x%x)          \n",
					pda->rtiproc[i].endaddr[j]);
			fprintf(out_fd,"\tppdasize(0x%x)         \n",
					pda->rtiproc[i].ppdasize[j]);
			fprintf(out_fd,"\tsleepflag(%d)          ",
					pda->rtiproc[i].sleepflag[j]);
			if (pda->rtiproc[i].sleepflag[j])
				fprintf(out_fd,"Process waiting on transfer\n");
			else
				fprintf(out_fd,"Process executing\n");
			fprintf(out_fd,"\taccessflag(0x%x)       ",
				pda->rtiproc[i].accessflag[j]);
			if (pda->rtiproc[i].accessflag[j] & B_BUSY)
				fprintf(out_fd,"B_BUSY ");
			if (pda->rtiproc[i].accessflag[j] & B_WANTED)
				fprintf(out_fd,"B_WANTED");
			fprintf(out_fd,"\n");
			fprintf(out_fd,"\tdoCheckDMA(%d)         \n",
				pda->rtiproc[i].doCheckDMA[j]);

			if (pda->rtiproc[i].RTIprefixw2[j]) {
				if (an_grab_virt_chunk(0, 
			    	     pda->rtiproc[i].RTIprefixw2[j], &telement, 
			    	     sizeof(telement)) != 0) {
				     fprintf(out_fd,"Unable to read request\n");
				}
				else {
					write_Q_element(&telement, out_fd);
				}
			}
			else {
				fprintf(out_fd,"\tNo outstanding Request\n");
			}
		}
	}
   }
}

/*****************************************************************************
 * an_rti0_all()
 *****************************************************************************/
an_rti0_all(pda, out_fd)
    rti0_pda_type      *pda;
    FILE		*out_fd;
{
    an_rti0_pda(pda, out_fd);
    an_rti0_procs(pda, out_fd);
    an_rti0_daemon(pda,out_fd);
    an_rti0_prefix(pda,out_fd);
    an_rti0_io(pda,out_fd);
}

/*****************************************************************************
 * an_rti0_decode_status()
 *****************************************************************************/
an_rti0_decode_status(llio_status, out_fd)
    llio_status_type	llio_status;
    FILE		*out_fd;
{

    fprintf(out_fd, "rti0 llio status = 0x%08x -- (", llio_status);
    fprintf(out_fd, "SUBSYS %d, ", llio_status.u.subsystem);

    if (llio_status.u.proc_num == -1)
	fprintf(out_fd, "LOCAL, ");
    else if (llio_status.u.proc_num == 0)
	fprintf(out_fd, "GLOBAL, ");
    else
	fprintf(out_fd, "%d?, ", llio_status.u.proc_num);

    fprintf(out_fd, "errnum %d)\n", llio_status.u.error_num);
}


/*****************************************************************************
 * an_rti0_help()
 *****************************************************************************/
an_rti0_help(out_fd)
    FILE		*out_fd;
{
    fprintf(out_fd, "\nvalid options:\n");
    fprintf(out_fd, "   pda     --  print out basic pda info\n");
    fprintf(out_fd, "   procs   --  display active processes and their requests\n");
    fprintf(out_fd, "   io      --  display vquads\n");
    fprintf(out_fd, "   daemon  --  display daemon structure\n");
    fprintf(out_fd, "   prefix  --  display prefixs waiting to be processed\n");
    fprintf(out_fd, "\n");
    fprintf(out_fd, "   all     --  all of the above\n");
    fprintf(out_fd, "   help    --  print this help screen\n");
}


/*****************************************************************************
 * an_rti0_decode_message()
 *****************************************************************************/
an_rti0_decode_message(mp, prefix, out_fd)
    rti0_msg_type	        *mp;
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
        an_rti0_decode_status(mp->u.do_bind_reply , out_fd);
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
        an_rti0_decode_status(mp->u.bind_reply.reply_status, out_fd);
        break;
    default:
	fprintf(out_fd, "UNKNOWN MESSAGE)\n");
	break;
    } /* switch */
} /* an_rti0_decode_message */


/*****************************************************************************
 * write_Q_element()
 *****************************************************************************/
write_Q_element(element, out_fd) 
struct RTIQelement *element;
FILE		*out_fd;	/* file descriptor of output file     */
{
	if (element == NULL) {
		fprintf(out_fd,"\tNo Outstanding Request\n");
		return;
	}
	switch (element->prefix.u.RTIheader.RTIprefixid) {
		case ISRTIENDOFQUEUE:
			fprintf(out_fd,"\tType:ISRTIENDOFQUEUE\n");
			break;	
		case ISRTIENTERBOOT:
			fprintf(out_fd,"\tType: ISRTIENTERBOOT\n");
			fprintf(out_fd,"\tReason: %d\n",
			     element->prefix.u.RTIenterboot.enterreason);
			break;	
		case ISRTIXFER:
				fprintf(out_fd,"\tType:ISRTIXFER\n");
				if (element->prefix.u.RTIxfer.RTIwrite == TRUE) 
					fprintf(out_fd,"\tDir: WRITE\n");
				else
					fprintf(out_fd,"\tDir: READ\n");	
				if (element->prefix.u.RTIxfer.blocking == TRUE) 
					fprintf(out_fd,"\tMode: BLOCKING\n");
				else
					fprintf(out_fd,"\tMode: NON-BLOCKING\n");	
				fprintf(out_fd,"\tRTIpid: 0x%x\n",
				    element->prefix.u.RTIxfer.RTIpid);
				fprintf(out_fd,"\tLength: %d\n",
				    element->prefix.u.RTIxfer.length);
				fprintf(out_fd,"\tRTIaddr: 0x%x\n",
				    element->prefix.u.RTIxfer.RTIaddr);
				fprintf(out_fd,"\tTrans #: %d\n",
				    element->prefix.u.RTIxfer.transaction);
				break;
		case ISRTISIGMESG:
				fprintf(out_fd,"\tISRTISIGMESG\n");
				fprintf(out_fd,"RTIpid:0x%x, ",
				    element->prefix.u.RTIsigmesg.RTIpid);
				fprintf(out_fd,"\tMesg: %c\n",
				    element->prefix.u.RTIsigmesg.mesg);
				break;
		case ISRTIDELETE:
				fprintf(out_fd,"\tISRTIDELETE\n");
				fprintf(out_fd,"\tDELETEpid: 0x%x\n",
				    element->prefix.u.RTIdelete.deletepid);
				break;
		case ISRTIERROR:
				fprintf(out_fd,"\tISRTIERROR\n");
				fprintf(out_fd,"\tType: ");
				if (element->prefix.u.RTIerror.type 
				     == ERRPANIC)
				     fprintf(out_fd,"ERRPANIC\n");
				else if (element->prefix.u.RTIerror.type 
				     == ERRINTVECT)
				     fprintf(out_fd,"ERRINTVECT\n");
				else if (element->prefix.u.RTIerror.type 
				     == ERRDMATO)
				     fprintf(out_fd,"ERRDMATO\n");
				else if (element->prefix.u.RTIerror.type 
				     == ERRINTERNAL)
				     fprintf(out_fd,"ERRINTERNAL\n");
				else if (element->prefix.u.RTIerror.type 
				     == ERRBADCARD)
				     fprintf(out_fd,"ERRBADCARD\n");
				else
				     fprintf(out_fd,"UNKNOWN VALUE\n");	
				
				break;
		case ISRTIABORT:
				fprintf(out_fd,"\tISRTIABORT\n");
				fprintf(out_fd,"\tRTIpid:0x%x, ",
				    element->prefix.u.RTIabort.RTIpid);
				fprintf(out_fd,"\tabort:%d\n",
				    element->prefix.u.RTIabort.abort);
				fprintf(out_fd,"\taborttype: ");
				if (element->prefix.u.RTIabort.aborttype ==
				    READ)
					fprintf(out_fd,"READ\n");
				else
					fprintf(out_fd,"WRITE\n");
				fprintf(out_fd,"\tTrans#: %d\n",
				    element->prefix.u.RTIabort.transaction);
				break;
		}
}

/*****************************************************************************
 * write_IO_mode()
 *****************************************************************************/
write_IO_mode(value, out_fd)
int value;
FILE	*out_fd;
{
	char *s;

		switch(value) {
			case NONBLOCKINGIO:
					s = "NONBLOCKING";
					break;
			case SEMIBLOCKINGIO:
					s = "SEMIBLOCKING";
					break;
			case BLOCKINGIO:
					s = "BLOCKING";
					break;
			default:
					s = "UNKNOWN";
		}
		fprintf(out_fd,"%s",s);
}

/*****************************************************************************
 * an_rti0_vquad()
 *****************************************************************************/
an_rti0_vquad(vqp, out_fd)
    struct cio_vquad_type vqp;
    FILE		*out_fd;
{
    int cmd = vqp.command.cio_cmd & ~BITS;

    fprintf(out_fd, "\tcommand  = 0x%08x -- ", vqp.command.cio_cmd);
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
    if (vqp.command.cio_cmd & BYTE_MODE)   fprintf(out_fd, " | BYTE_MODE");
    if (vqp.command.cio_cmd & CIOCA_S1S2)  fprintf(out_fd, " | CIOCA_S1S2");
    if (vqp.command.cio_cmd & LOGCH_BREAK) fprintf(out_fd, " | LOGCH_BREAK");
    if (vqp.command.cio_cmd & BLOCKED)	    fprintf(out_fd, " | BLOCKED");
    if (vqp.command.cio_cmd & CONTINUE)    fprintf(out_fd, " | CONTINUE");
    fprintf(out_fd,", link     = 0x%08x\n", vqp.link);
    fprintf(out_fd, "\tcount    = %d, ", vqp.count);
    fprintf(out_fd, "residue = %d\n", vqp.residue);
    fprintf(out_fd, "\tbuffer   = %d.0x%08x %s\n", vqp.buffer.u.data_sid,
    	vqp.buffer.u.data_vba, addr_class(vqp.addr_class));
}

#endif OLDIOSERV
