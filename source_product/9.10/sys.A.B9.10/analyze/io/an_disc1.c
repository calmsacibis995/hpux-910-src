/***************************************************************************/
/**                                                                       **/
/**   This is the I/O manager analysis routine for disc1.                 **/
/**                                                                       **/
/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sio/llio.h>
#include <h/types.h>
#include <h/buf.h>
#include <sio/iotree.h>
#include <sio/disc1.h>			/* disc1's normal .h file	      */
#include "aio.h"			/* all analyze i/o defs		      */

int	disc1_an_rev = 1;	/* global -- rev number of disc1 DS   */
static char *me      = "disc1";

#define	BOOLEAN(s)	((s) ? "true" : "false")
#define bit(x)  	(1 << (x))
#define	streq(a, b)	(strcmp((a), (b)) == 0)
#define	min(a, b)	(((a) < (b)) ? (a) : (b))
#define	BITS	(BYTE_MODE|CIOCA_S1S2|LOGCH_BREAK|BLOCKED|CONTINUE)
char *addr_class();

int
an_disc1(call_type, port_num, out_fd, option_string)
    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */
{
    int    		addr;		/* address of extern                  */
    int			code_rev;	/* revision of driver code	      */
    struct disc1_pda	pda;		/* port data area                     */
    struct mgr_info_type mgr;		/* all generic manager info	      */

    int			an_disc1_decode_message();
    int			an_disc1_decode_status();

    /* grab the manager information */
    if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, port_num, &mgr)!= AIO_OK)
    {  fprintf (stderr, " Problem-- Bad port num\n");
       return(0);
    }

    /* grab the pda */
    if (an_grab_virt_chunk(0, mgr.pda_address, (char *)&pda, sizeof(pda)) != 0) {
	fprintf(stderr, "Couldn't get pda\n");
	return(0);
    }

    /* perform the correct action depending on what call_type is */
    switch (call_type) {

	case AN_MGR_INIT:
	    /* check driver rev against analysis rev -- complain if a problem */
	    if (an_grab_extern("disc1_an_rev", &addr) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, 0, disc1_an_rev, out_fd);
	    else if (an_grab_real_chunk(addr, &code_rev, sizeof(code_rev)) != 0)
		aio_rev_mismatch(me, AIO_NO_REV_INFO, 0, disc1_an_rev, out_fd);
	    else if (code_rev != disc1_an_rev)
		aio_rev_mismatch(me, AIO_INCOMPAT_REVS, code_rev, disc1_an_rev,
				 out_fd);

	    break;

	case AN_MGR_BASIC:
	    an_disc1_all(&pda, out_fd, 0);
	    break;

	case AN_MGR_DETAIL:
	    an_disc1_all(&pda, out_fd, 1);
	    break;

	case AN_MGR_OPTIONAL:
	    an_disc1_optional(&pda, out_fd, option_string);
	    break;

	case AN_MGR_HELP:
	    an_disc1_help(stderr);
	    break;

    }
    return(0);
}

an_disc1_optional(pda, out_fd, option_string)
    struct disc1_pda	*pda;
    FILE		*out_fd;
    char		*option_string;
{
    char *token;
    int	 device_info_flag = 0;
    int	 queued_flag	= 0;
    int	 ioq_flag	= 0;
    int	 request_flag	= 0;
    int  diag_msg_flag  = 0;
    int	 all_flag	= 0;
    int  ioq_all_flag	= 0;

    /* read through option string and set appropriate flags */
    token = strtok(option_string, " ");

    do {
	if      (streq(token,    "device"))   device_info_flag = 1;
	else if (streq(token,    "ioq"))   ioq_flag= 1;
	else if (streq(token,    "ioq_all"))   ioq_all_flag= 1;
	else if (streq(token,    "diag_msg"))   diag_msg_flag= 1;
	else if (streq(token,    "queued"))   queued_flag= 1;
	else if (streq(token,    "request"))   request_flag   = 1;
	else if (streq(token,    "all"))   all_flag   = 1;
	else  {
	    fprintf(stderr, "invalid option <%s>\n", token);
	    an_disc1_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (device_info_flag)   an_disc1_device_info (pda, out_fd);
    if (ioq_flag)   an_disc1_ioq_info (pda, out_fd, 0);
    if (ioq_all_flag)   an_disc1_ioq_info (pda, out_fd, 1);
    if (diag_msg_flag)   an_disc1_diag_info(pda, out_fd, 1);
    if (queued_flag) an_disc1_queued (pda, out_fd); 
    if (request_flag)   an_disc1_request_info(pda, out_fd);
    if (all_flag)   an_disc1_all(pda, out_fd);
}



/************************** DEVICE INFO *******************************/
/* This routine decodes the device specific information.  Such as HPIB*/
/* address, device type, and its unit information.                    */
/**********************************************************************/

an_disc1_device_info (pda, out_fd)

    struct disc1_pda	*pda;
    FILE		*out_fd;

{
    struct d1_dcs	*d = &pda->dcs;
    int		 	u;



    fprintf(out_fd, "\n===========DEVICE INFO===========\n");
    fprintf(out_fd, "HPIB address (dev num) = %d\n", d->hw );
    fprintf(out_fd, "MID (mid-1 equals LU) = %d\n", d->mid);
    fprintf(out_fd, "HPIB dam entry point = %d\n", d->hpibn);
    fprintf(out_fd, "d_state = " );
    switch (d->d_state) {
	case D_UNCONF:     fprintf(out_fd, "D_UNCONF\n" );  break;
	case D_RESET :     fprintf(out_fd, "D_RESET\n" );  break;
	case D_BUSYIO:     fprintf(out_fd, "D_BUSYIO\n" );  break;
	case D_IDLE  :     fprintf(out_fd, "D_IDLE \n" );  break;
	case D_RELEASE:    fprintf(out_fd, "D_RELEASE\n" );  break;
	case D_DUMP   :    fprintf(out_fd, "D_DUMP\n" );  break;
	case D_CTDLOAD:    fprintf(out_fd, "D_CTDLOAD\n" );  break;
	case D_TO_UNCONF:  fprintf(out_fd, "D_TO_UNCONF\n" );  break;
	default:	   fprintf(out_fd, "unknown\n"); break;
   }
   fprintf(out_fd, "c_state = " );
   switch (d->c_state) {
	case C_NOTHING:	   fprintf(out_fd, "C_NOTHING\n");  break;
	case C_DESCRIBE:   fprintf(out_fd, "C_DESCRIBE\n");  break;
	case C_DEVINIT:    fprintf(out_fd, "C_DEVINIT\n");  break;
	case C_CTDINIT:    fprintf(out_fd, "C_CTDINIT\n");  break;
	case C_DEVICEIO:   fprintf(out_fd, "C_DEVICEIO\n");  break;
	case C_DEVCLEAR:   fprintf(out_fd, "C_DEVCLEAR\n");  break;
	case C_RELEASE:    fprintf(out_fd, "C_RELEASE\n");  break;
	case C_TRANS_IO:   fprintf(out_fd, "C_TRANS_IO\n");  break;
	case C_PARITYON:   fprintf(out_fd, "C_PARITYON\n");  break;
	case C_CTD_DESCRIBE:   fprintf(out_fd, "C_CTD_DESCRIBE\n");  break;
	case C_DUMP:           fprintf(out_fd, "C_DUMP\n");  break;
	case C_DISCINIT:      fprintf(out_fd, "C_DISCINIT\n");  break;
	default:	   fprintf(out_fd, "unknown\n"); break;
   }
   fprintf(out_fd, "i_state = " );
   switch (d->i_state) {
	case I_UNCONF:	   fprintf(out_fd, "I_UNCONF\n");  break;
	case I_IN_CONF:	   fprintf(out_fd, "I_IN_CONF\n");  break;
	case I_RECONF:	   fprintf(out_fd, "I_RECONF\n");  break;
	case I_CONF:	   fprintf(out_fd, "I_CONF\n");  break;
	default:	   fprintf(out_fd, "unknown\n"); break;
   }
   fprintf(out_fd, "open count = %d\n", d->open_cnt );

    switch (d->mode) {
	case NORMAL_MODE:       fprintf(out_fd, "mode of open = NORMAL MODE\n");  break;
	case TRANSPARENT_MODE:  fprintf(out_fd, "mode of open = TRANSPARENT MODE\n");  break;
	case EXCLUSIVE_MODE:    fprintf(out_fd, "mode of open = EXCLUSIVE MODE\n");  break;
	default:	        fprintf(out_fd, "unknow\n"); break;
 }
    fprintf(out_fd, "devnum (minor num) = 0x%x\n", d->devnum);
    fprintf(out_fd, "model (index)      = %d\n", d->model);
    fprintf(out_fd, "model_num          = 0x%x\n", d->model_num);

/**** unit information of a device ***************/

    for (u=0; u<MAXUNITS; u++)
    {  fprintf(out_fd, "\n---------- Information about unit %d  -------\n", u);
       switch(d->units[u].type) {
	  case CS80DISC:  fprintf(out_fd, "type     = CS80DISC\n"); break;
	  case CS80CTD:   fprintf(out_fd, "type     = CS80CTD\n"); break;
	  case NO_UNIT:   fprintf(out_fd, "type     = NO_UNIT\n"); break;
	  default:        fprintf(out_fd, "type     = unknown\n"); break;
        }
       if (d->units[u].type == CS80DISC)
	  fprintf(out_fd, "SETRPS sent flag   = %d\n", d->units[u].rpssent);
       fprintf(out_fd, "bytes per block   = %d\n", d->units[u].blksz);
       fprintf(out_fd, "number of shifts  = %d\n", d->units[u].bs);
       fprintf(out_fd, "max single vec addr = %d\n", d->units[u].maxsva);
       fprintf(out_fd, "index to disc_parms = %d\n", d->units[u].dparms);
       fprintf(out_fd, "pointer to sec table= 0x%x\n", d->units[u].sect);
       fprintf(out_fd, "block open flags    = 0x%x\n", d->b_opens[u]);
       fprintf(out_fd, "char  open flags    = 0x%x\n", d->c_opens[u]);
    } /* for MAXUNITS */

  fprintf (out_fd, "\n**** powerfail information ****\n");
  fprintf (out_fd, "powerfail transaction number = %d\n", d->pf_transaction_num);
  fprintf (out_fd, "powerfail message id = %d\n", d->pf_message_id);



} /* an_disc1_device_info */

/************************ request_info ************************************/
/* This routine decodes the current request to a device.  The message to  */
/* the DAM is decoded along with the actual CS80 command, and also the    */
/* report block from DAM.                                                 */
/**************************************************************************/

an_disc1_request_info(pda, out_fd)
  
  struct disc1_pda	*pda;
  FILE			*out_fd;

{
  struct d1_dcs		*d = &pda->dcs;
  struct d1msg 		msg , *msgp;

    unsigned long rw_len, rw_addr;
			/* length and address of read/write operation */
    char *trans_dir;	/* data transfer direction (read, write, none) */
    unsigned char t_cmd; /* actual transparent command opcode */
    unsigned char rw_cmd; /* read/write command opcode */
    unsigned char util_cmd; /* utility command name */
    unsigned char *len_ptr; /* pointer to read/write operation length */
    unsigned short *addr_ptr; /* pointer to address of read/write operation */
    char *transp_cmd;	/* name of transparent command */
    char *readwrite_cmd;/* name of read/write command */
    dam_req_type *dam_ptr;	/* pointer to HPIB dam io request */
    union d1_rqblk *rq_ptr, request_b;  /* pointer to CS80 request block */
    struct d1_rpblk rpblk, *rpblk_p;
    char *prefix = "";

/* it is negative */
fprintf (out_fd, " transaction number = %d\n", d->tid);

       switch(d->io_func_code){
	  case DA_IDY_DEVICE:  fprintf(out_fd, "io_fcode = AMIGO_ID\n"); break;
	  case 0x40:   fprintf(out_fd, "io_fcode = CS80_ND\n"); break;
	  case 0x4c:   fprintf(out_fd, "io_fcode = CS80_RD\n"); break;
	  case 0x48:   fprintf(out_fd, "io_fcode = CS80_WD\n"); break;
	  case 0x60:   fprintf(out_fd, "io_fcode = CS80_TRAN_ND\n"); break;
	  default:        fprintf(out_fd, "io_fcode = unknown\n"); break;
        }

fprintf (out_fd, "retries = %d\n", d->retries);

if (an_grab_virt_chunk(0, d->msg, &msg, sizeof(msg)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");

msgp = &msg;
    /*
     * Print the global command part first; HPIB device number,
     * and data transfer direction (read or write data)...
     */

    dam_ptr = &msgp->u0.dam_req;

/* dcs->req and dam_ptr->request_parms.cmd_ptr.u.data_vba are pointing to the */
/* same thing.  So, whichever we try to decode, it should contain the same    */
/* result.                                                                    */

/*if (an_grab_virt_chunk(0, dam_ptr->request_parms.cmd_ptr.u.data_vba, &request_b, sizeof(request_b)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");
*/

if (an_grab_virt_chunk(0, d->req, &request_b, sizeof(request_b)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");

 rq_ptr = &request_b; 

    fprintf(out_fd, "HPIB device num = 0x%x\n", dam_ptr->device_number); /* HPIB device number */

    if (dam_ptr->transfer_dir == NO_DATA) /* data transfer direction */
	trans_dir = "NO_DATA";
    else if (dam_ptr->transfer_dir == READ_DATA)
	trans_dir = "READ_DATA";
    else
	trans_dir = "WRITE_DATA";
    fprintf(out_fd, "transfer direction = %s\n", trans_dir);
    fprintf(out_fd, "exec bytes req = %d\n\n", dam_ptr->request_parms.data_count);
    /*
     * Now try to decode the actual CS80 command, and print whatever is
     * easy to decode...
     * cmd_ptr.u.data_vba is actually pointing to dcs->req
     */

    if (dam_ptr->request == CS80_EXECUTE) { /* CS80 disk request */
	printf ("CS80_execute command\n");
	if (dam_ptr->u.cs80_req.transparent) { /* not read or write */
	    fprintf(out_fd, "transparent CS80 req\n ");
	    if ((t_cmd = rq_ptr->cs2.cmd1) ==
			POCMD1)		/* parity enable */
		transp_cmd = "parity";
	    else if (t_cmd == CLCMD1)	/* controller clear */
		transp_cmd = "controller clear";
	    else
		transp_cmd = "unknown";
	    fprintf(out_fd, "command = %s\n", transp_cmd);

	} else {	/* could be describe, read or write */
	    if ((rw_cmd = rq_ptr->cs2.cmd2) ==
			DESCRIBE) 		/* describe unit command */
		fprintf(out_fd, "DESCRIBE cmd, unit %d\n", (rq_ptr->cs2.cmd1) & 0x0f);
	    else if (rw_cmd == EXTDESCRIBE)
		fprintf(out_fd, "EXTDESCRIBE cmd, unit %d\n", rq_ptr->cs2.cmd1);
	    else if (rw_cmd == CS80LOAD) 	/* load tape */
		fprintf(out_fd, "LOAD cmd, parm1 %d, tape number(1-8) %d\n",
			rq_ptr->dinit.parm2, rq_ptr->dinit.parm3);
	    else if (rw_cmd == UNLOAD) 		/* unload tape */
		fprintf(out_fd, "UNLOAD cmd\n");
	    else if (rw_cmd == SETOPTION) 	/* set tape options */
		fprintf(out_fd, "SETOPTION cmd\n");
	    else if (rw_cmd == SETRELEASE)
		fprintf(out_fd, "SETRELEASE cmd, unit %d\n", rq_ptr->dinit.unit);
	    else if (rw_cmd == SETRPS)
		fprintf(out_fd, "SETRPS cmd, unit %d\n", rq_ptr->dinit.unit);
	    else if (rw_cmd == RELEASE )
		fprintf(out_fd, "RELEASE cmd, unit %d\n", rq_ptr->cs2.cmd1);

	    else if (rw_cmd == (INITUTILITY & 0xf0)) {	/* utility command */
		fprintf(out_fd, "INITUTILITY cmd\n ");
		if ((util_cmd = rq_ptr->dinit.parm2) == 
			RETURN_EXCALIBUR_TAPE_INFORMATION)
		    fprintf(out_fd, "RETURN_EXCALIBUR_TAPE_INFORMATION utility, 0x%x\n",
				rw_cmd);
		else if (util_cmd == RETURN_POWER_FAIL_STATUS)
		    fprintf(out_fd, "RETURN_POWER_FAIL_STATUS utility, 0x%x\n",
				rw_cmd);
		else
		    fprintf(out_fd, "undecoded utility (0x%x)\n", util_cmd);
	    } else if (rw_cmd == SETADDRESS) { /* read/write-type command */
		  
		/* Get the command, and the length and address of transfer */
		len_ptr = &rq_ptr->io.len[0];
		rw_len = ((*len_ptr << 24) & 0xff000000) +
			 (((*(len_ptr + 1)) << 16) & 0xff0000) +
			 (((*(len_ptr + 2)) << 8) & 0xff00) +
			 ((*(len_ptr + 3)) & 0xff);

		addr_ptr = &rq_ptr->io.mid_addr;
	        rw_addr = ((*addr_ptr << 16) & 0xffff0000) +
			  ((*(addr_ptr + 1)) & 0xffff);

		if (rq_ptr->io.slen == WRITEMARK) {	/* write file mark */
		    fprintf(out_fd, "WRITEMARK cmd, addr %d\n",
			    rw_addr);
		} else {
	            switch (rq_ptr->io.cmd) {
		    case LOCATENWRITE:
		        readwrite_cmd = "LOCATENWRITE";
		        break;

		    case LOCATENREAD:
		        readwrite_cmd = "LOCATENREAD";
		        break;

		    case LOCATENVERIFY:
		        readwrite_cmd = "LOCATENVERIFY";
		        break;

		    default: 
		        readwrite_cmd = "undecoded";
		        break;
		    }

		    fprintf(out_fd, "%s cmd\nlen = %d\naddr = %d\n",
			    readwrite_cmd, rw_len, rw_addr);
		}
	    }
	    else				/* currently undecoded */
		fprintf(out_fd, "undecoded cmd\n");
	}
    } else if (dam_ptr->request == DA_REQUEST) {
						/* generic HIPB request */
	if (dam_ptr->u.da_req == AMIGO_ID)
	    fprintf(out_fd, "AMIGO_ID HPIB da_request: 0x%X\n", dam_ptr->u.da_req);
	else
	    fprintf(out_fd, "unknown HPIB da_request: 0x%X\n", dam_ptr->u.da_req);
    } else					/* unknown HPIB request */
	fprintf(out_fd, "unknown HPIB request\n");
fprintf (out_fd, "residue = %d\n", d->residue);

/* the end of decoding the cs80 command */
/* now, decode the device report block  */

if (an_grab_virt_chunk(0, d->rpt, &rpblk, sizeof(rpblk)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");
rpblk_p = &rpblk;

fprintf(out_fd, "\n******** report block decoding ********\n" );

an_cs80_status ( &rpblk_p->qstat, prefix, out_fd);

fprintf(out_fd, "\n******** HPIB DAM reply decoding ********\n" );

/* llio_status decode for HPIB1 is not available yet.  */

if (aio_decode_llio_status (d->hpib_reply.reply_status, out_fd) != AIO_OK)
	fprintf (out_fd, " something wrong in the llio status routine\n");


}

/********************** diag_info *********************************************/
/* This routine decodes the diagnostic message sent to the diagnostic port    */
/******************************************************************************/

an_disc1_diag_info(pda, out_fd)
  
  struct disc1_pda	*pda;
  FILE			*out_fd;

{
  /* decode the diag message frame */

  struct d1_dcs		*d = &pda->dcs;
  struct d1msg 		dmsg , *dmsgp;
  diag_event_type	*devent;
  struct hwe_buf	*hwe_ptr;
  char *prefix = "";
  int			i;


if (an_grab_virt_chunk(0, d->dmsg, &dmsg, sizeof(dmsg)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");

dmsgp = &dmsg;

if (dmsgp->hdr.msg_descriptor == DIAG_EVENT_MSG)
	fprintf (out_fd, "msg descriptor = DIAG_EVENT_MSG\n");
else
	fprintf (out_fd, "msg descriptor = %d\n", dmsgp->hdr.msg_descriptor);

fprintf (out_fd, "hdr.message_id = %d\n", dmsgp->hdr.message_id);
fprintf (out_fd, "hdr.from_port  = %d\n", dmsgp->hdr.from_port  );
fprintf (out_fd, "hdr.transaction_num = %d\n", dmsgp->hdr.transaction_num);

devent = &dmsgp->u0.diag_event;

if (devent->diag_class == DIAG_HW_EVENT)
	fprintf (out_fd, "diag class = DIAG_HW_EVENT\n" );
else if (devent->diag_class == DIAG_SW_EVENT)
	fprintf (out_fd, "diag class = DIAG_SW_EVENT\n" );
else
	fprintf (out_fd, "diag class = %d\n", devent->diag_class );
fprintf (out_fd, "llio_status.is_ok = 0x%x\n\n", devent->llio_status.is_ok);
fprintf (out_fd, "hw_status_len = %d\n", devent->hw_status_len);

hwe_ptr = (struct hwe_buf *) devent->u.bit8_buff;

an_cs80_status ( &hwe_ptr->hw_status.cs80_reply[0], prefix, out_fd);
fprintf (out_fd, "\n");

/*for (i=0; i<D1_CS80_STATUS_LEN; i++)
	fprintf (out_fd, "%x", hwe_ptr->hw_status.cs80_reply[i]);
fprintf (out_fd, "\n");
*/

fprintf (out_fd, "mgr_info_len = %d\n", devent->mgr_info_len );
for (i=0; i<CS80_MAX_CMD_LEN; i++)
	fprintf (out_fd, "%x", hwe_ptr->mgr_info.cmd_bytes[i]);
fprintf (out_fd, "\n");


if (hwe_ptr->mgr_info.subsys == SUBSYS_DISC1)
	fprintf (out_fd, "mgr_info.subsys = SUBSYS_DISC1\n");
else
	fprintf (out_fd, "mgr_info.subsys = %d\n", hwe_ptr->mgr_info.subsys);
fprintf (out_fd, "mgr_info.key = %d\n", hwe_ptr->mgr_info.key);
fprintf (out_fd, "mgr_info.levels = %d\n", hwe_ptr->mgr_info.levels);
fprintf (out_fd, "mgr_info.io_func_code = 0x%x\n", hwe_ptr->mgr_info.io_func_code);
       switch(hwe_ptr->mgr_info.io_func_code){
	  case DA_IDY_DEVICE:  fprintf(out_fd, "io_fcode = AMIGO_ID\n"); break;
	  case 0x40:   fprintf(out_fd, "io_fcode = CS80_ND\n"); break;
	  case 0x4c:   fprintf(out_fd, "io_fcode = CS80_RD\n"); break;
	  case 0x48:   fprintf(out_fd, "io_fcode = CS80_WD\n"); break;
	  case 0x60:   fprintf(out_fd, "io_fcode = CS80_TRAN_ND\n"); break;
	  default:        fprintf(out_fd, "io_fcode = unknown\n"); break;
        }
fprintf (out_fd, "mgr_info.io_dev_addr = %d\n", hwe_ptr->mgr_info.dev_addr);
fprintf (out_fd, "mgr_info.timeout = %d\n", hwe_ptr->mgr_info.timeout);


}



/************************* ioq_info *****************************************/
/* This routine decodes the pending request in ioqs (if exist).  The flag   */
/* 'all' indicates whether all the requests ( from the head of the list to  */
/* the end of the list ) should be decoded or just a head of the list is    */
/* decoded.                                                                 */
/****************************************************************************/

an_disc1_ioq_info(pda, out_fd, all)
  
  struct disc1_pda	*pda;
  FILE			*out_fd;
  int			all;

{
  struct d1_dcs		*d = &pda->dcs;
  struct buf		buffer1, buffer2, *ioQ1, *ioQ2;

fprintf(out_fd, "********** ioq information ***************\n");
if (an_grab_virt_chunk(0, d->ioq1, &buffer1, sizeof(buffer1)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");
ioQ1 = &buffer1;

if (an_grab_virt_chunk(0, d->ioq2, &buffer2, sizeof(buffer2)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");
ioQ2 = &buffer2;
/* if just a head of the list */
if ( !all ) {
  if (ioQ1->b_actf == NULL)
    fprintf (out_fd, "no request for ioq1\n");
  else
  {  if (an_grab_virt_chunk(0, ioQ1->b_actf, &buffer1, sizeof(buffer1)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");
	ioQ1 = &buffer1;
	fprintf(out_fd, "----------- ioq1 information ----------\n");
	an_print_buffer (ioQ1, out_fd);
  }
  fprintf (out_fd, "*** end of ioq1 information ***\n\n");

 if (ioQ2->b_actf == NULL)
    fprintf (out_fd, "no request for ioq2\n");
 else
 {  if (an_grab_virt_chunk(0, ioQ2->b_actf, &buffer2, sizeof(buffer2)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");
	ioQ2 = &buffer2;
	fprintf(out_fd, "----------- ioq2 information ----------\n");
	an_print_buffer (ioQ2, out_fd);
  }
  fprintf (out_fd, "*** end of ioq2 information ***\n\n");
}
else /* if all */
{
  if (ioQ1->b_actf == NULL)
    fprintf (out_fd, "no request for ioq1\n");
  else
  {  while (ioQ1->b_actf != NULL)
     {	if (an_grab_virt_chunk(0, ioQ1->b_actf, &buffer1, sizeof(buffer1)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");
	ioQ1 = &buffer1;
	fprintf(out_fd, "----------- ioq1 information ----------\n");
	an_print_buffer (ioQ1, out_fd);
     } /* while */
   fprintf (out_fd, "*** end of ioq1 information ***\n\n");
   } /* else */
 if (ioQ2->b_actf == NULL)
    fprintf (out_fd, "no request for ioq2\n");
 else
 {  while (ioQ2->b_actf != NULL)
    {   if (an_grab_virt_chunk(0, ioQ2->b_actf, &buffer2, sizeof(buffer2)) != 0)
	fprintf (out_fd, " Cannot get the virtual address\n");
	ioQ2 = &buffer2;
	fprintf(out_fd, "----------- ioq2 information ----------\n");
	an_print_buffer (ioQ2, out_fd);
     }
     fprintf (out_fd, "*** end of ioq2 information ***\n\n");
  } /* else */
  }
} /* an_disc1_ioq_info */


/************************ print_buffer ***************************************/
/* print the content of the buffer.                                          */
/*****************************************************************************/

an_print_buffer (ioQ_ptr, out_fd)
struct buf	*ioQ_ptr;
FILE		*out_fd;

{
fprintf(out_fd, "b_flags  	= 0x%x\n", ioQ_ptr->b_flags);
fprintf(out_fd, "b_forw	 	= 0x%x\n", ioQ_ptr->b_forw);
fprintf(out_fd, "b_back		= 0x%x\n", ioQ_ptr->b_back);
fprintf(out_fd, "b_actf	 	= 0x%x\n", ioQ_ptr->b_actf);
fprintf(out_fd, "b_actl  	= 0x%x\n", ioQ_ptr->b_actl);
fprintf(out_fd, "b_active      	= %d\n", ioQ_ptr->b_active);
fprintf(out_fd, "b_bcount	= %d\n", ioQ_ptr->b_bcount);
fprintf(out_fd, "b_bufsize	= %d\n", ioQ_ptr->b_bufsize);
fprintf(out_fd, "b_error         = %d\n", ioQ_ptr->b_error);
fprintf(out_fd, "b_dev		= 0x%x\n", ioQ_ptr->b_dev);
fprintf(out_fd, "b_addr		= 0x%x\n", ioQ_ptr->b_un.b_addr);
fprintf(out_fd, "b_resid         = 0x%x\n", ioQ_ptr->b_resid);
fprintf(out_fd, "b_blkno         = %d\n", ioQ_ptr->b_blkno);


}

/******************* queued_info *********************************************/
/* This routine decodes the port number and  any queued up messages for this */
/* port.                                                                     */
/*****************************************************************************/

an_disc1_queued (pda, out_fd)
  struct disc1_pda	*pda;
  FILE			*out_fd;
{
    struct d1_dcs		*d = &pda->dcs;
    int	i;
    struct mgr_info_type mgr;

    fprintf (out_fd, " **** port number information ****\n");
    fprintf(out_fd, "LDM port num for this device = %d\n", d->ldm_port);
    fprintf(out_fd, "DAM port num for this device = %d\n", d->dam_port);
    fprintf(out_fd, "DAM subqueue for this device = %d\n", d->dam_subq);

    aio_get_mgr_info(AIO_PORT_NUM_TYPE, d->ldm_port, &mgr);
    fprintf(out_fd, "Subq summary: enabled subqs = 0x%08x\n",
	mgr.enabled_subqs);
    fprintf(out_fd, "              active subqs  = 0x%08x\n\n",
	mgr.active_subqs);

    /* walk through subqs and decode */
    for (i = D1POWERFAIL_SUBQ; i < NUM_SUBQ; i++)  {
	fprintf(out_fd, "   subq %2d:  ", i);

	switch (i) {
	    case D1POWERFAIL_SUBQ:
	    	fprintf(out_fd, "(PWERFAIL_SUBQ)              "); break;

	    case CONFIG_SUBQ:
	    	fprintf(out_fd, "(CONFIG_SUBQ)"); break;

	    case NO_DEST_DIAG_SUBQ:
	    	fprintf(out_fd, "(NON_DEST_DIAG_SUBQ)"); break;

	    case LOCKER_SUBQ:
	    	fprintf(out_fd, "(LOCKER_SUBQ)            "); break;

	    case REPLY_SUBQ:
	    	fprintf(out_fd, "(REPLY_SUBQ)            "); break;

	    case HPIB_EVENT_SUBQ:
	    	fprintf(out_fd, "(HPIB_EVENT_SUBQ)          "); break;

	    case DISC1_TIMER_SUBQ:
	    	fprintf(out_fd, "(DISC1_TIMER_SUBQ)          "); break;

	    case PFTIMER_SUBQ:
	    	fprintf(out_fd, "(PFTIMER_SUBQ)          "); break;

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



/*******************************************************************/
an_disc1_all (pda, out_fd, all)
  struct disc1_pda	*pda;
  FILE			*out_fd;
  int			all;

{
an_disc1_device_info (pda, out_fd);
fprintf (out_fd, "\n");
an_disc1_request_info (pda, out_fd);
fprintf (out_fd, "\n");
an_disc1_ioq_info (pda, out_fd, all);
fprintf (out_fd, "\n");
an_disc1_diag_info (pda, out_fd);
fprintf (out_fd, "\n");
an_disc1_queued (pda, out_fd);
fprintf (out_fd, "\n");


}

/********************************************************************/

an_disc1_help (out_fd)
  FILE			*out_fd;

{
fprintf (out_fd, "device 	  ------ decodes the configured device info\n");
fprintf (out_fd, "request 	  ------ decodes the current request \n");
fprintf (out_fd, "ioq 		  ------ decodes the head of the ioq request \n");
fprintf (out_fd, "ioq_all 	  ------ decodes all the requests in ioqs\n");
fprintf (out_fd, "queued 	  ------ decodes the queued messages \n");
fprintf (out_fd, "diag_msg 	  ------ decodes the diag message \n");
fprintf (out_fd, "all             ------ all of above\n");
fprintf (out_fd, "help            ------ printfs out this help info\n");

}



/****************************************************************/
