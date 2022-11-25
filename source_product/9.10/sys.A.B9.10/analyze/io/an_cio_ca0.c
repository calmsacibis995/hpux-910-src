/* Note:
	  rev 2 of cio_ca0 analyze demands the following revisions:
	     ../sio/cio_ca0.c	1.91.15.7 or better
	     ../sio/cio_ca0.h   1.3.15.5 or better
*/

#include <stdio.h>
#include <sio/llio.h>
#include <h/types.h>
#include <sio/iotree.h>
#include <sio/cio_ca0.h>		/* cio_ca0's normal .h file	      */
#include <machine/param.h>
#include "aio.h"			/* all analyze i/o defs		      */


int	cio_ca0_an_rev = 2;		/* global -- rev number of an_cio_ca0 */
static  pda_type	*pda;		/* port data area                     */

int an_cio_ca0 (call_type, port_num, out_fd, option_string)

    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */

{
    int    		addr;		/* address of extern                  */
    int			code_rev;	/* revision of driver code	      */
    caddr_t		al_pda_address; /* aligned pda address		      */
    pda_type		cam_pda;	/* port data area                     */
    struct mgr_info_type mgr;		/* all generic manager info	      */

    int			an_cio_ca0_decode_message();
    int			an_cio_ca0_decode_status();

    /* grab the manager information */
    aio_get_mgr_info(AIO_PORT_NUM_TYPE,port_num,&mgr);

    /* grab the pda (after getting an aligned pointer to it) */
    page_align(mgr.pda_address,al_pda_address);
    if (an_grab_virt_chunk(0,al_pda_address,&cam_pda,sizeof(cam_pda)) != 0)  {
	fprintf(stderr,"Couldn't get pda\n");
	return(0);
    }
    pda = &cam_pda;

    /* perform the correct action depending on what call_type is */
    switch (call_type)  {

	case AN_MGR_INIT:
	    /* check driver rev against analysis rev -- complain if a problem */
	    if (an_grab_extern("cio_ca0_an_rev",&addr) != 0)
		aio_rev_mismatch("cio_ca0",AIO_NO_REV_INFO,code_rev,cio_ca0_an_rev,out_fd);
	    else  {
		if (an_grab_real_chunk(addr,&code_rev,4) != 0)
		    aio_rev_mismatch("cio_ca0",AIO_NO_REV_INFO,code_rev,cio_ca0_an_rev,out_fd);
	        if (code_rev != cio_ca0_an_rev)
		    aio_rev_mismatch("cio_ca0",AIO_INCOMPAT_REVS,code_rev,cio_ca0_an_rev,out_fd);
	    }

	    /* put my decoding routines into system lists        */
	    aio_init_llio_status_decode(SUBSYS_CIO_CA0,an_cio_ca0_decode_status);
	    aio_init_message_decode(START_CIO_MSG,START_CIO_MSG+9,
	                            an_cio_ca0_decode_message);
	    break;

	case AN_MGR_BASIC:
	    an_cio_ca0_active(out_fd);
	    an_cio_ca0_queued(&mgr,out_fd);
	    an_cio_ca0_sanity(out_fd);
	    break;

	case AN_MGR_DETAIL:
	    an_cio_ca0_all(&mgr,out_fd);
	    break;

	case AN_MGR_OPTIONAL:
	    an_cio_ca0_optional(&mgr,out_fd,option_string);
	    break;

	case AN_MGR_HELP:
	    an_cio_ca0_help(out_fd);
	    break;

    }

    return(0);

}

an_cio_ca0_optional (mgr, out_fd, option_string)

    struct mgr_info_type *mgr;
    FILE		*out_fd;
    char		*option_string;

{
    char 		*token;
    int			active_flag=0;
    int			subc_flag=0;
    int			tid_flag=0;
    int			pda_flag=0;
    int			da_flag=0;
    int			pquad_flag=0;
    int			queued_flag=0;
    int			sanity_flag=0;
    int			logclist_flag=0;
    int			complist_flag=0;
    int			bcheck_flag=0;
    int			all_flag=0;
    int			state=0;
#define	FREE_STATE	0
#define TID_STATE	1
#define DA_STATE   	2
#define SUBC_STATE   	3
#define PQUAD_STATE   	4
    int			tid_num=-1;
    int			subc_num=-1;
    int			da_num=-1;
    int			pquad_num=-1;
    int			ct;
    int			temp;


    /* read through option string and set appropriate flags */
    token = strtok(option_string," ");

    do  {

	if (strcmp(token,"all") == 0) {
	    state = FREE_STATE;
	    all_flag = 1;
	}

	else if (strcmp(token,"sanity") == 0) {
	    state = FREE_STATE;
	    sanity_flag = 1;
	}

	else if (strcmp(token,"active") == 0) {
	    state = FREE_STATE;
	    active_flag = 1;
	}

	else if (strcmp(token,"queued") == 0) {
	    state = FREE_STATE;
	    queued_flag = 1;
	}

	else if (strcmp(token,"pda") == 0) {
	    state = FREE_STATE;
	    pda_flag = 1;
	}

	else if (strcmp(token,"buf") == 0) {
	    bcheck_flag = 1;
	}

	else if (strcmp(token,"subc") == 0) {
	    state = SUBC_STATE;
	    subc_flag = 1;
	}

	else if (strcmp(token,"pquad") == 0) {
	    state = PQUAD_STATE;
	    pquad_flag = 1;
	}

	else if (strcmp(token,"da") == 0) {
	    state = DA_STATE;
	    da_flag = 1;
	}

	else if (strcmp(token,"tid") == 0) {
	    state = TID_STATE;
	    tid_flag = 1;
	}

	else if (strcmp(token,"logclist") == 0) {
	    logclist_flag = 1;
	}

	else if (strcmp(token,"complist") == 0) {
	    complist_flag = 1;
	}

	/* no keyword match -- either a numeric parameter or a mistake */
	else  {

	    if (state != FREE_STATE)  {

		/* convert the token into a number if possible */
		ct = sscanf(token,"0x%x",&temp);
		if (ct < 1)
		    ct = sscanf(token,"%d",&temp);

		/* if we got a number, proceed */
		if (ct == 1)  {

		    /* stuff the correct kind of number */
		    if (state == TID_STATE)
			tid_num = temp;
		    else if (state == DA_STATE)
			da_num = temp;
		    else if (state == SUBC_STATE)
			subc_num = temp;
		    else if (state == PQUAD_STATE)
			pquad_num = temp;

		    /* check for tid number */
		    if ( (state == TID_STATE) && 
		         ( (tid_num < 0) || (tid_num >= NUMLOGC+16-1) ) ) {
			fprintf(stderr,"tid <%s> out of range\n",token);
			fprintf(stderr,"(valid range is 0 to %d (0x00 to 0x%x)\n",NUMLOGC+15-1,NUMLOGC+15-1);
			an_cio_ca0_help(out_fd);
			return(0);
		    }
			
		    /* check for da number */
		    if ( (state == DA_STATE) && ( (da_num < 0) || (da_num >= 16) ) ) {
			fprintf(stderr,"da number <%s> out of range\n",token);
			fprintf(stderr,"(valid range is 0 to 15 (0x0 to 0xf)\n");
			an_cio_ca0_help(out_fd);
			return(0);
		    }
			
		    /* check for subc number */
		    if ( (state == SUBC_STATE) &&
		         ( (subc_num < 0) || (subc_num >= 16) ) ) {
			fprintf(stderr,"subc number <%s> out of range\n",token);
			fprintf(stderr,"(valid range is 0 to 15 (0x0 to 0xf)\n");
			an_cio_ca0_help(out_fd);
			return(0);
		    }
			
		}

		/* blow off -- invalid option */
		else  {
		    fprintf(stderr,"invalid option <%s>\n",token);
		    an_cio_ca0_help(out_fd);
		    return(0);
		}

		state = FREE_STATE;

	    }

	    else  {
		fprintf(stderr,"invalid option <%s>\n",token);
		an_cio_ca0_help(out_fd);
		return(0);
	    }

	}

    } while ( (token = strtok(NULL," ")) != NULL);

    /* now process the flags that were set */
    if (all_flag)        an_cio_ca0_all(mgr,out_fd);
    if (da_flag)         an_cio_ca0_da(da_num,out_fd);
    if (subc_flag)       an_cio_ca0_subc(subc_num,out_fd);
    if (pquad_flag)	 an_cio_ca0_pquad_chain(pquad_num,0,"   ",out_fd);
    if (tid_flag)        an_cio_ca0_tid(tid_num,out_fd);
    if (sanity_flag)     an_cio_ca0_sanity(out_fd);
    if (active_flag)     an_cio_ca0_active(out_fd);
    if (queued_flag)     an_cio_ca0_queued(mgr,out_fd);
    if (pda_flag)        an_cio_ca0_pda(out_fd);
    if (complist_flag)   an_cio_ca0_compl_list(out_fd);
    if (logclist_flag)   an_cio_ca0_logc_list(out_fd);
    if (bcheck_flag)	 an_cio_ca0_bcheck (out_fd);

}

an_cio_ca0_all (mgr, out_fd)

    struct mgr_info_type *mgr;
    FILE		*out_fd;

{
    an_cio_ca0_sanity(out_fd);
    an_cio_ca0_queued(mgr,out_fd);
    an_cio_ca0_pda(out_fd);
    an_cio_ca0_logc_list(out_fd);
    an_cio_ca0_compl_list(out_fd);
    an_cio_ca0_da(-1,out_fd);
    an_cio_ca0_tid(-1,out_fd);
}



an_cio_ca0_help (out_fd)

    FILE		*out_fd;

{
    fprintf(out_fd,"\nvalid options:\n");
    fprintf(out_fd,"   active        --  what requests are active on the hardware\n");
    fprintf(out_fd,"   buf           --  verify buffers are only owned by CAM\n");
    fprintf(out_fd,"   complist      --  decode completion list\n");
    fprintf(out_fd,"   da [da#]      --  decode info on this (or all) da's\n");
    fprintf(out_fd,"   logclist      --  display logchannel number list\n");
    fprintf(out_fd,"   pda           --  miscellaneous pda information\n");
    fprintf(out_fd,"   pquad x       --  try to decode this as a pquad chain\n");
    fprintf(out_fd,"   queued        --  what msgs are on the CAM's subqueues\n");
    fprintf(out_fd,"   sanity        --  do consistency checks on CAM data structures\n");
    fprintf(out_fd,"   subc [subc#]  --  decode info on this (or all) subchannels\n");
    fprintf(out_fd,"   tid [tid#]    --  decode info on this (or all) tid structures\n");
    fprintf(out_fd,"\n");
    fprintf(out_fd,"   all           --  do all of the above\n");
    fprintf(out_fd,"   help          --  print this help screen\n");
}

an_cio_ca0_sanity (out_fd)

    FILE		*out_fd;

{
    an_cio_ca0_logc_list(out_fd);
    an_cio_ca0_bcheck(out_fd);
}

an_cio_ca0_pda (out_fd)

    FILE		*out_fd;

{
    int			i,total;	/* temp */

    fprintf(out_fd,"\n");
    fprintf(out_fd,"    channel adapter hardware:\n");
    fprintf(out_fd,"      hpa address = 0x%08x\n",(int)HPA);
    fprintf(out_fd,"      spa address = 0x%08x\n",(int)SPA);
    fprintf(out_fd,"      eim = 0x%08x\n",EIM);
    fprintf(out_fd,"      module number = %d (0x%x)\n",CA_ADDRESS,CA_ADDRESS);

    fprintf(out_fd,"\n");
    fprintf(out_fd,"    timer stuff:\n");
    fprintf(out_fd,"      initialization timer id = %d\n",INIT_TIMER);
    fprintf(out_fd,"      module error timer id = %d\n",MERR_TIMER);

    fprintf(out_fd,"\n");
    fprintf(out_fd,"    miscellaneous stuff:\n");
    if (PFAIL_TRN == 0)
	fprintf(out_fd,"      last pfail trn = 0x%08x (no powerfails since boot)\n",PFAIL_TRN);
    else
	fprintf(out_fd,"      last pfail trn = 0x%08x\n",PFAIL_TRN);
    fprintf(out_fd,"      last quad added = 0x%08x\n",(int)LASTPQUAD);
    fprintf(out_fd,"      doing dump = ");
    if (DUMP) 
	fprintf(out_fd,"TRUE\n");
    else
	fprintf(out_fd,"FALSE\n");

    an_cio_ca0_em_fe(out_fd);

#ifdef SBTEST0
    /* temp stuff */
    total = 0;
    fprintf(out_fd,"\n");
    fprintf(out_fd,"     number of transactions processed = %d\n",NUMTRN);
    for (i=0 ; i<300 ; i++)
	if (BUFCT[i] != 0) {
	    fprintf(out_fd,"        %2d buflets used:  %d\n",i,BUFCT[i]);
	    total = total + BUFCT[i];
	}
    fprintf(out_fd,"\n     total number of cam_buffers allocated = %d\n\n", 
	    total);
    if (NUMTRN != total)
	fprintf(out_fd,"      mismatch in buflet counts - ERROR!!!!!\n");
#endif SBTEST0

#ifdef SBTEST1
    fprintf(out_fd,"\n     LOG_NEXT = %d (next index to use)\n",LOG_NEXT);
    for (i=LOG_NEXT-1 ; i>=0 ; i--)
	an_cio_ca0_decode_dma_log(out_fd,i);
    for (i=LOG_MAX-1 ; i>=LOG_NEXT ; i--) {
	if (LOG[i].pfail_trn == 0)
	    break;
	an_cio_ca0_decode_dma_log(out_fd,i);
    }
#endif SBTEST1
}

#ifdef SBTEST1
an_cio_ca0_decode_dma_log (out_fd, ndx)

    FILE		*out_fd;
    int			ndx;

{
    /* handle dma requests */
    if (LOG[ndx].dma.dma_type == 0)  {
	fprintf(out_fd,"        %3d   dma request msg      ",ndx);
	if (LOG[ndx].dma.subc == 0xff)
	    fprintf(out_fd,"              ");
	else
	    fprintf(out_fd,"subc %d        ",LOG[ndx].dma.subc);
	if (LOG[ndx].dma.logc == 0xff)
	    fprintf(out_fd,"            ");
	else
	    fprintf(out_fd,"logc 0x%2x      ",LOG[ndx].dma.logc);
      	fprintf(out_fd,"status = 0x%x\n",LOG[ndx].dma.status);
    }

    /* handle dma replies */
    else if (LOG[ndx].dma.dma_type == 1)  {
	fprintf(out_fd,"        %3d   dma reply msg        ",ndx);
	if (LOG[ndx].dma.subc == 0xff)
	    fprintf(out_fd,"              ");
	else
	    fprintf(out_fd,"subc %d        ",LOG[ndx].dma.subc);
	if (LOG[ndx].dma.logc == 0xff)
	    fprintf(out_fd,"            ");
	else
	    fprintf(out_fd,"logc 0x%2x      ",LOG[ndx].dma.logc);
      	fprintf(out_fd,"status = 0x%x\n",LOG[ndx].dma.status);
    }

    /* handle transactions active at pfail before call to cam_comp */
    else if (LOG[ndx].dma.dma_type == 2)  {
	fprintf(out_fd,"        %3d   active at pfail -- before cam_comp ",ndx);
	if (LOG[ndx].dma.subc == 0xff)
	    fprintf(out_fd,"              ");
	else
	    fprintf(out_fd,"subc %d        ",LOG[ndx].dma.subc);
	if (LOG[ndx].dma.logc == 0xff)
	    fprintf(out_fd,"\n");
	else
	    fprintf(out_fd,"logc 0x%2x\n",LOG[ndx].dma.logc);
    }

    /* handle transactions active at pfail after call to cam_comp */
    else if (LOG[ndx].dma.dma_type == 3)  {
	fprintf(out_fd,"        %3d   active at pfail -- after cam_comp  ",ndx);
	if (LOG[ndx].dma.subc == 0xff)
	    fprintf(out_fd,"              ");
	else
	    fprintf(out_fd,"subc %d        ",LOG[ndx].dma.subc);
	if (LOG[ndx].dma.logc == 0xff)
	    fprintf(out_fd,"\n");
	else
	    fprintf(out_fd,"logc 0x%2x\n",LOG[ndx].dma.logc);
    }

    /* log warning if completion list was non-null */
    else if (LOG[ndx].dma.dma_type == 4)
	fprintf(out_fd,"        %3d   completion list non-null after cam_comp!\n",ndx);

    /* handle powerfails */
    else
	fprintf(out_fd,"        %3d   power-on req msg     trn %08x\n",ndx,
		LOG[ndx].pfail_trn);
}
#endif SBTEST1

an_cio_ca0_em_fe (out_fd)

    FILE		*out_fd;

{
    int			in_em_fe;
    unsigned		addr;


    /* print the emulated fe mode status of the channel's pages */
    fprintf(out_fd,"\n");
    fprintf(out_fd,"    status of channel's address range:\n");
    addr = (unsigned)HPA;
    while (1) {

	/* find the status of the page and print it */
	fprintf(out_fd,"      page starting at 0x%08x -- ",addr);
	if (aio_em_fe(addr,&in_em_fe) != AIO_OK)
	    fprintf(out_fd,"has no pdir entry (problem!)\n");
	else if (in_em_fe)
	    fprintf(out_fd,"in emulated fe mode\n");
	else
	    fprintf(out_fd,"not in emulated fe mode\n");

	/* go to the next page */
	if (addr == ((unsigned)HPA+2048))
	    addr = (unsigned)SPA;
	else if (addr == ((unsigned)SPA+(3*2048)))
	    break;
	else
	    addr = addr + 2048;

    }

}

an_cio_ca0_logc_list (out_fd)

    FILE		*out_fd;

{
    int			i;
    int			j;
    char		logc_array[NUMLOGC+NUMDA];


    /* print logchannel allocation list */
    fprintf(out_fd,"\n");
    fprintf(out_fd,"    logchannel number free list status:\n");

    /* last allocated */
    if (LOGCHEAD != LOGCHTAIL)  {
        fprintf(out_fd,"       last logchannel numbers allocated:\n");
        fprintf(out_fd,"           ");
	i = LOGCHEAD;
	do  {
	    fprintf(out_fd,"0x%2x   ",LOGCHLIST[i]);
	    if (i == 0)
		i = NUMLOGC - 2;
	    else
		i--;
	}  while (i != LOGCHTAIL);
	fprintf(out_fd,"\n");

    }

    /* last freed */
    fprintf(out_fd,"       last logchannel numbers released:\n");
    fprintf(out_fd,"           ");
    i = LOGCHTAIL;
    for (j=0 ; j<50 ; j++)  {
        fprintf(out_fd,"0x%2x   ",LOGCHLIST[i]);
        if (i == 0)
	    i = NUMLOGC - 2;
        else
	    i--;
    }
    fprintf(out_fd,"\n");

    /* initialize marking array */
    for (i=NUMDA ; i<NUMDA+NUMLOGC ; i++)
	 logc_array[i] = 0;

    /* run through the free list and mark entries */
    i = LOGCHEAD + 1;
    if (i == (NUMLOGC-2))
	i = 0;
    do  {
        logc_array[LOGCHLIST[i]]++;
        if (i == (NUMLOGC-2))
	    i = 0;
        else
	    i++;
    }  while (i != LOGCHTAIL);
    logc_array[LOGCHLIST[LOGCHTAIL]]++;
    logc_array[NUMLOGC+NUMDA-1]++;

    /* not on free list */
    fprintf(out_fd,"       logchannel numbers not on free list:\n");
    fprintf(out_fd,"           ");
    for (i=NUMDA ; i<(NUMDA+NUMLOGC) ; i++)
	if (logc_array[i] == 0)
            fprintf(out_fd,"0x%2x   ",i);
    fprintf(out_fd,"\n");

    /* check used logchannel numbers to see if they are active */
    for (i=NUMDA ; i<(NUMDA+NUMLOGC) ; i++)
	if ( (logc_array[i] == 0) && ((int)TID[i].msg == 0) )
	    fprintf(out_fd,"             no transaction active on logc 0x%2x!\n",i);

    /* check for duplicates -- on free list and active! */
    for (i=NUMDA ; i<(NUMDA+NUMLOGC) ; i++)
	if ( ((int)TID[i].msg != 0) && (logc_array[i] != 0) )
	    fprintf(out_fd,"       logc 0x%2x active and on free list!\n",i);

    /* check for duplicates -- on free list multiple times! */
    for (i=NUMDA ; i<(NUMDA+NUMLOGC) ; i++)
	if (logc_array[i] > 1)
	    fprintf(out_fd,"       logc 0x%2x on free list multiple times!\n",
		    i);

}

an_cio_ca0_compl_list (out_fd)

    FILE		*out_fd;

{
    unsigned		addr;
    unsigned 		next;
    lnkst_type 		head;
    int			i;


    /* grab the completion list header and make sure its not on free list! */
    addr = (unsigned)LISTHEAD;
    aio_mem_on_free_list(addr,32,"   ERROR: ",out_fd);
    if (an_grab_virt_chunk(0,addr,&head,sizeof(lnkst_type)) != 0)  {
	fprintf(out_fd,"    problem with completion header!!\n");
	return;
    }

    /* print the list header */
    addr = (unsigned)head.link;
    fprintf(out_fd,"\n\n    completion list address = 0x%08x\n",LISTHEAD);
    fprintf(out_fd,"      semaphore value = 0x%08x  ",addr);
    if (addr & 1)
	fprintf(out_fd,"(free)\n");
    else
	fprintf(out_fd,"(in use)\n");

    /* print completion entries until eoc */
    addr = (unsigned)head.status;
    while ( (addr & 1) == 0)  {
	an_cio_ca0_compl_entry(addr,&next,"      ",out_fd);
	fprintf(out_fd,"\n");
	addr = next;
    }

    /* grab the dummy list header and make sure its not on free list! */
    addr = (unsigned)DLISTHEAD;
    aio_mem_on_free_list(addr,32,"   ERROR: ",out_fd);
    if (an_grab_virt_chunk(0,addr,&head,sizeof(lnkst_type)) != 0)  {
	fprintf(out_fd,"    problem with completion header!!\n");
	return;
    }

    /* print the list header */
    addr = (unsigned)head.link;
    fprintf(out_fd,"    dummy completion list address = 0x%08x\n",DLISTHEAD);
    fprintf(out_fd,"      semaphore value = 0x%08x  ",addr);
    if (addr & 1)
	fprintf(out_fd,"(free)\n");
    else
	fprintf(out_fd,"(in use)\n");

}

an_cio_ca0_compl_entry (entry_addr,next,prefix,out_fd)

    unsigned		entry_addr;
    unsigned		*next;
    char		*prefix;
    FILE		*out_fd;

{
    lnkst_type		entry;


    /* grab the completion list entry and verify that we own it */
    fprintf(out_fd,"%sentry address   = 0x%08x\n",prefix,entry_addr);
    if (an_grab_virt_chunk(0,entry_addr,&entry,sizeof(lnkst_type)) != 0)  {
        fprintf(out_fd,"%s  problem with completion entry!!\n",prefix);
	*next = 1;
        return(1);
    }
    aio_mem_on_free_list(entry_addr,32,prefix,out_fd);

    /* print the contents */
    fprintf(out_fd,"%s  link to next element = 0x%08x\n",prefix,
                   (unsigned)entry.link);
    fprintf(out_fd,"%s  status               = 0x%08x\n",prefix,entry.status);
    fprintf(out_fd,"%s  save_link            = 0x%08x\n",prefix,entry.lastlink);
    fprintf(out_fd,"%s  residue              = 0x%08x\n",prefix,entry.residue);
    fprintf(out_fd,"%s  tid                  = 0x%x  (%2d)\n",prefix,
                   entry.tid,entry.tid);
    
    fprintf(out_fd,"%s  type                 = %2d  ",prefix,entry.type);
    switch (entry.type)  {
       case RESIDUE:	fprintf(out_fd,"(residue)\n"); break;
       case COMPLETION:	fprintf(out_fd,"(completion)\n"); break;
       default:		fprintf(out_fd,"(unknown!!)\n");
    }

    /* advance to the next entry */
    *next = entry.link;
	
}

an_cio_ca0_on_compl_list (entry_addr, head_addr)

    unsigned		entry_addr;
    unsigned		head_addr;

{
    unsigned		addr;
    lnkst_type		entry;


    /* grab the completion list header */
    if (an_grab_virt_chunk(0,head_addr,&entry,sizeof(lnkst_type)) != 0)
	return(0);

    /* process completion entries until eoc or our entry is found */
    addr = (unsigned)entry.status;
    while ( (addr & 1) == 0)  {

	/* check for our entry */
 	if (addr == entry_addr)
	    return(1);

	/* grab the completion list entry */
	if (an_grab_virt_chunk(0,addr,&entry,sizeof(lnkst_type)) != 0)
	    return(0);

	/* move to next element */
	addr = entry.link;
    }

    return(0);
}

an_cio_ca0_queued (mgr, out_fd)

    struct mgr_info_type *mgr;
    FILE		*out_fd;

{
    int			i;
    int			j;
    int        		msg;

    /* print the state of the CAM's subqueues */
    fprintf(out_fd,"Subq summary:  enabled subqs = 0x%08x\n",mgr->enabled_subqs);
    fprintf(out_fd,"               active subqs  = 0x%08x\n\n",mgr->active_subqs);

    /* walk through subqs and decode */
    for (i=0 ; i<NUM_SUBQS ; i++)  {

	fprintf(out_fd,"   subq %2d:  ",i);

	switch (i)  {
	    case PFAIL_SUBQ:
	        fprintf(out_fd,"(powerfail)              "); break;
	    case CONFIG_SUBQ:
	        fprintf(out_fd,"(configuration)          "); break;
	    case DIAGNOSTICS_SUBQ:
	        fprintf(out_fd,"(diagnostics)            "); break;
	    case 3:
	        fprintf(out_fd,"(completions)            "); break;
	    case TIMER_INIT_SUBQ:
	        fprintf(out_fd,"(initialization)         "); break;
	    case TIMER_LOGC_ABORT_SUBQ:
	        fprintf(out_fd,"(logc abort)             "); break;
	    case TIMER_SUBC_SUBQ:
	        fprintf(out_fd,"(subchannel)             "); break;
	    case TIMER_MERR_SUBQ:
	        fprintf(out_fd,"(module error)           "); break;
	    default:
	        fprintf(out_fd,"(requests from subc %2d)  ",i-FIRST_IOSUBQ);
	}
	    
	if (mgr->enabled_subqs & ELEMENT_OF_32(i))
	    fprintf(out_fd," enabled     ");
	else
	    fprintf(out_fd," disabled    ");

	if (mgr->active_subqs & ELEMENT_OF_32(i)) {
	    fprintf(out_fd,"msgs queued\n");
	    for (j=0 ; j<30 ; j++)  {
		if (aio_queued_message(mgr->port_num,i,j,&msg) != AIO_OK) {
		    fprintf(out_fd,"\n");
		    break;
		}
		fprintf(out_fd,"\n");
		aio_decode_message(msg,"        ",out_fd);
	    }

	}
	else
	    fprintf(out_fd,"no msgs queued\n");

    }

}

an_cio_ca0_active (out_fd)

    FILE		*out_fd;

{
    int			i;

    for (i=0 ; i<NUMDA ; i++)
	an_cio_ca0_1subc(i,0,out_fd);
}
 



an_cio_ca0_subc (subc_num, out_fd)

    int			subc_num;
    FILE		*out_fd;

{
    int			i;		/* loop counter		*/

    if (subc_num != -1)
	an_cio_ca0_1subc(subc_num,1,out_fd);
    
    else
	for (i=0 ; i<16 ; i++)
	    an_cio_ca0_1subc(i,1,out_fd);
}


an_cio_ca0_da (da_num, out_fd)

    int			da_num;
    FILE		*out_fd;

{
    int			i;		/* loop counter		*/

    if (da_num != -1)
	an_cio_ca0_1da(da_num,out_fd);
    
    else
	for (i=0 ; i<16 ; i++)
	    an_cio_ca0_1da(i,out_fd);
}
 



an_cio_ca0_tid (tid_num, out_fd)

    int			tid_num;
    FILE		*out_fd;

{
    int			i;		/* loop counter		*/

    if (tid_num != -1)
	an_cio_ca0_1tid(tid_num,out_fd);
    
    else
	for (i=0 ; i<NUMLOGC+16-1 ; i++)
	    an_cio_ca0_1tid(i,out_fd);
}

an_cio_ca0_1subc (subc, always, out_fd)

    int			subc;
    int			always; 	/* 0 - only print if active */
    FILE		*out_fd;

{
    int			i;
    cammsg_type		msg;

    /* print info on a subchannel transaction */
    if ( (SUBC[subc].islogc == 0) && ((always) || ((int)TID[subc].msg)) ) {
	an_cio_ca0_1da(subc,out_fd);
	an_cio_ca0_1tid(subc,out_fd);
    }

    /* print info on a logchannel transaction */
    else if (SUBC[subc].islogc)  {

	/* scan logchannels to see if any are active (set always) */
	for (i=16 ; i<(NUMLOGC+16-1) ; i++)
	    if ((int)TID[i].msg) {
		if (an_grab_virt_chunk(0,(int)TID[i].msg,&msg,sizeof(msg))!=0) {
		    fprintf(stderr,"Couldn't get message\n");
		    return(0);
		}
		if (msg.u.dma_req.da_number == subc) {
		    always = 1;
		    break;
	 	}
	    }

	/* if always set (or active logc was found) print da and tid info */
	if (always) {
	    an_cio_ca0_1da(subc,out_fd);
	    for (i=16 ; i<(NUMLOGC+16-1) ; i++)
		if ((int)TID[i].msg) {
		    if (an_grab_virt_chunk(0,(int)TID[i].msg,&msg,sizeof(msg))){
			fprintf(stderr,"Couldn't get message\n");
			return(0);
		    }
		    if (msg.u.dma_req.da_number == subc)
			an_cio_ca0_1tid(i,out_fd);
		}
	}
    }
}

an_cio_ca0_1da (da, out_fd)

    int			da;
    FILE		*out_fd;

{
    char		mgr_name[20];

    fprintf(out_fd,"\nDevice Adapter decoding for da %d (0x%x):\n",
	    da,da);

    /* decode port field */
    fprintf(out_fd,"     Subc->port = %-3d                ",SUBC[da].port);
    if (aio_get_mgr_name(SUBC[da].port,mgr_name) == AIO_OK)
	fprintf(out_fd,"port to send events to  <%s>\n",mgr_name);
    else
	fprintf(out_fd,"unused subchannel\n");

    /* decode status field */
    fprintf(out_fd,"     Subc->status = %d                ",SUBC[da].status);
    if (SUBC[da].status == DA_IDLE) {
	fprintf(out_fd,"DA_IDLE -- ");
	if (SUBC[da].islogc)
	    fprintf(out_fd,"no meaning in logc mode\n");
        else if ( (SUBC[da].ispoll) && (TID[da].msg) )
	    fprintf(out_fd,"card being used in poll mode\n");
	else
	    fprintf(out_fd,"card not being used\n");
    }
    else if (SUBC[da].status == DA_BUSY) {
	fprintf(out_fd,"DA_BUSY -- ");
	if (SUBC[da].islogc)
	    fprintf(out_fd,"doing a bad abort or a selftest\n");
	else 
	    fprintf(out_fd,"doing a subc trn or a selftest\n");
    }
    else if (SUBC[da].status == DA_PFAIL)
	fprintf(out_fd,"DA_PFAIL -- doing a pfail\n");
    else if (SUBC[da].status == DA_NOTPRESENT)
	fprintf(out_fd,"DA_NOTPRESENT -- card not there\n");
    else if (SUBC[da].status == DA_SICKCARD)
	fprintf(out_fd,"DA_SICKCARD -- card doesn't pass selftest\n");
    else
	fprintf(out_fd,"unknown status!!!!\n");

    /* decode islogc field */
    fprintf(out_fd,"     Subc->islogc = %d                ",SUBC[da].islogc);
    if (SUBC[da].islogc)
	fprintf(out_fd,"currently in logchannel mode\n");
    else
	fprintf(out_fd,"currently in subchannel mode\n");

    /* decode ismux field */
    fprintf(out_fd,"     Subc->ismux = %d                 ",SUBC[da].ismux);
    if (SUBC[da].ismux == 0)
	fprintf(out_fd,"not a mux card\n");
    else
	fprintf(out_fd,"is a mux card\n");

    /* decode ispoll field */
    fprintf(out_fd,"     Subc->ispoll = %d                ",SUBC[da].ispoll);
    if (SUBC[da].ispoll == 0)
	fprintf(out_fd,"not in poll mode\n");
    else
	fprintf(out_fd,"in poll mode\n");

    /* decode level1_card field */
    fprintf(out_fd,"     Subc->level1_card = %d           ",SUBC[da].level1_card);
    if (SUBC[da].level1_card == 0)
	fprintf(out_fd,"not a level 1 card\n");
    else
	fprintf(out_fd,"is a level 1 card\n");

    /* decode powfail field */
    fprintf(out_fd,"     Subc->powfail = 0x%08x      ",SUBC[da].powfail);
    if ( (SUBC[da].status == DA_PFAIL) && (SUBC[da].powfail !=0) )
	fprintf(out_fd,"pfail trn this DAM must give CAM\n");
    else if (SUBC[da].powfail == 0)
	fprintf(out_fd,"no pfails or bad aborts since boot\n");
    else
	fprintf(out_fd,"last pfail trn on this device adapter\n");

    /* decode timer_status field */
    fprintf(out_fd,"     Subc->timer_status = %d          ",SUBC[da].timer_status);
    if (SUBC[da].timer_status == TIMER_INIT)
	fprintf(out_fd,"TIMER_INIT (just allocated)\n");
    else if (SUBC[da].timer_status == TIMER_INACTIVE)
	fprintf(out_fd,"TIMER_INACTIVE (not in use)\n");
    else if (SUBC[da].timer_status == TIMER_STEST_DCL)
	fprintf(out_fd,"TIMER_STEST_DCL (device clear just done for selftest)\n");
    else if (SUBC[da].timer_status == TIMER_STEST_DEN)
	fprintf(out_fd,"TIMER_STEST_DEN (device enable just done for selftest)\n");
    else if (SUBC[da].timer_status == TIMER_SUBC_ABORT)
	fprintf(out_fd,"TIMER_SUBC_ABORT (doing destroy subchannel)\n");
    else if (SUBC[da].timer_status == TIMER_ABORT_DCL)
	fprintf(out_fd,"TIMER_ABORT_DCL (device clear just done for worst-case abort)\n");
    else if (SUBC[da].timer_status == TIMER_ABORT_DEN)
	fprintf(out_fd,"TIMER_ABORT_DEN (device enable just done for worst-case abort)\n");
    else if (SUBC[da].timer_status == TIMER_RFC)
	fprintf(out_fd,"TIMER_RFC (dma path -- waiting for subchannel to come ready)\n");
    else
	fprintf(out_fd,"unknown timer state!!!!!\n");

    /* decode timer field */
    fprintf(out_fd,"     Subc->timer = %-3d               ",SUBC[da].timer);
    fprintf(out_fd,"timer id reserved for this subchannel\n");

    /* decode stest_retry field */
    if (SUBC[da].stest_subq != 0)  {
        fprintf(out_fd,"     Subc->stest_retry = %-2d          ",
		       SUBC[da].stest_retry);
	fprintf(out_fd,"last self test or bad abort took %d seconds\n",
	               15-SUBC[da].stest_retry);
    }

    /* decode stest_subq field */
    fprintf(out_fd,"     Subc->stest_subq = %-2d           ",SUBC[da].stest_subq);
    if (SUBC[da].stest_subq == 0)
	fprintf(out_fd,"unimportant -- no self tests\n");
    else
	fprintf(out_fd,"subq to send selftest ctrl reply to\n");

    /* decode lastpquad field */
    if (SUBC[da].islogc) {
        fprintf(out_fd,"     Subc->lastpquad = 0x%08x    ",SUBC[da].lastpquad);
	fprintf(out_fd,"final quad in last subc chain started\n");
    }

    /* decode subq field */
    fprintf(out_fd,"     Subc->subq = %-2d                 ",SUBC[da].subq);
    fprintf(out_fd,"subq to send events to\n");

}

an_cio_ca0_1tid (tid, out_fd)

    int			tid;
    FILE		*out_fd;

{
    int			garb;

    /* print tid number */
    fprintf(out_fd,"\nTransaction decoding for ");
    if (tid < 16)
	fprintf(out_fd,"subc %d (0x%x):\n",tid,tid);
    else
	fprintf(out_fd,"logc %d (0x%x):\n",tid,tid);

    /* decode msg */
    fprintf(out_fd,"     Tid->msg = 0x%08x           ",(int)TID[tid].msg);
    if ((int)TID[tid].msg == 0)
	fprintf(out_fd,"nothing active here\n");
    else if ( (tid < 16) && (SUBC[tid].ispoll) )  {
	fprintf(out_fd,"poll chain element\n");
	an_cio_ca0_poll_chain((int)TID[tid].msg,"        ",&garb,out_fd);
    }
    else {
	fprintf(out_fd,"active -- original request message\n");
	aio_decode_message(TID[tid].msg,"        ",out_fd);
    }

    /* decode buffers */
    if ( (tid >= NUMDA) || (SUBC[tid].ispoll == 0) ) {
        fprintf(out_fd,"     Tid->buffers = 0x%08x       ",(int)TID[tid].buffers);
        if ((int)TID[tid].msg == 0)
	    fprintf(out_fd,"meaningless -- transaction inactive\n");
	else if ((int)TID[tid].buffers == 0)
	    fprintf(out_fd,"strange!\n");
        else {
	    fprintf(out_fd,"allocated cam buffers\n");
	    an_cio_ca0_cam_buffers((int)TID[tid].buffers,"         ",out_fd);
	}
    }

    /* decode align_quads */
    if ( (tid >= NUMDA) || (SUBC[tid].ispoll == 0) ) {
        fprintf(out_fd,"     Tid->align_quads = 0x%08x   ",(int)TID[tid].align_quads);
        if ((int)TID[tid].msg == 0)
	    fprintf(out_fd,"meaningless -- transaction inactive\n");
	else if ((int)TID[tid].align_quads == 0)
	    fprintf(out_fd,"no quads on read alignment chain\n");
        else {
	    fprintf(out_fd,"read alignment chain\n");
	    an_cio_ca0_align_pquad_chain((int)TID[tid].align_quads,"     ",
					 out_fd);
	}
    }

    /* decode timer */
    if (tid >= NUMDA) {
        fprintf(out_fd,"     Tid->timer = %-3d                ",TID[tid].timer);
        if (TID[tid].timer == 0)
	    fprintf(out_fd,"no timer active (no logc abort)\n");
        else
	    fprintf(out_fd,"timer active (logc abort)\n");
    }

    /* decode trn */
    if (tid < NUMDA) {
        fprintf(out_fd,"     Tid->trn = 0x%08x           ",TID[tid].trn);
        if ( (SUBC[tid].ismux) && (SUBC[tid].ispoll) )
	    fprintf(out_fd,"trn of last poll message\n");
        else
	    fprintf(out_fd,"trn of last self test control request\n");
    }

    /* decode msgid */
    if (tid < NUMDA) {
        fprintf(out_fd,"     Tid->msgid = 0x%08x         ",TID[tid].msgid);
        if ( (SUBC[tid].ismux) && (SUBC[tid].ispoll) )
	    fprintf(out_fd,"msgid of last poll message\n");
        else
	    fprintf(out_fd,"msgid of last self test control request\n");
    }

    /* decode wtc_buffer */
    if (tid >= NUMDA) {
        fprintf(out_fd,"     Tid->wtc_buffer = 0x%08x    ",
                      (int)TID[tid].wtc_buffer);
        if (TID[tid].wtc_buffer == 0)
	    fprintf(out_fd,"no destroy logchannel going on\n");
        else {
	    fprintf(out_fd,"doing a destroy logchannel -- this is address of wtc quad\n");
	    an_cio_ca0_pquad_chain((unsigned)TID[tid].wtc_buffer,0,
				   "     ",out_fd);
        }
    }

    /* decode residues_freed */
    if ((int)TID[tid].msg != 0)  {
        fprintf(out_fd,"     Tid->residues_freed = %-3d       ",
                       TID[tid].residues_freed);
	fprintf(out_fd,"number of residues already processed\n");
	fprintf(out_fd,"                                     (if any requested)\n");
    }

    /* decode abort */
    if (tid >= NUMDA) {
        fprintf(out_fd,"     Tid->abort = %d                  ",TID[tid].abort);
        if (TID[tid].abort)
	    fprintf(out_fd,"logchannel abort in progress\n");
        else
	    fprintf(out_fd,"no logchannel abort going on\n");
    }

}

an_cio_ca0_vquad_chain (vquad_addr, prefix, out_fd)

    int			vquad_addr;
    char		*prefix;
    FILE		*out_fd;

{
    int			next;
    char		nprefix[80];

    strcpy(nprefix,prefix);
    strcat(nprefix,"   ");
    while (vquad_addr != 0)  {
	if (an_cio_ca0_vquad(vquad_addr,nprefix,&next,out_fd) == 0) 
	    vquad_addr = next;
	else  {
	    fprintf(out_fd,"%svquad at 0x%08x is suspect\n\n",nprefix,vquad_addr);
	    return(-1);
	}
	fprintf(out_fd,"\n");
	if (vquad_addr != 0)
	    fprintf(out_fd,"%snext vquad address = 0x%08x\n",prefix,vquad_addr);
    }
}

an_cio_ca0_vquad (vquadaddr, prefix, nextlink, out_fd)

    int				vquadaddr;
    char			*prefix;
    int				*nextlink;
    FILE			*out_fd;
{
    struct cio_vquad_type	vquad;

    /* grab this vquad */
    if (an_grab_virt_chunk(0,vquadaddr,&vquad,sizeof(cio_vquad_type))!=0) {
	fprintf(stderr,"Couldn't get vquad\n");
	return(-1);
    }

    /* interpret this vquad */
    fprintf(out_fd,"%scommand       = 0x%08x     ",prefix,
	    vquad.command.cio_cmd);
    an_cio_ca0_cio_cmd(vquad.command,out_fd);


    fprintf(out_fd,"%scount         = 0x%08x     size of transfer\n",prefix,
	    vquad.count);


    fprintf(out_fd,"%sresidue       = %d              ",prefix, vquad.residue);
    if (vquad.residue == 0)
	fprintf(out_fd,"no residue calculation requested\n");
    else
	fprintf(out_fd,"residue calculation requested\n");


    fprintf(out_fd,"%slink          = 0x%08x     ",prefix, (int)vquad.link);
    if ((int)vquad.link == 0)
	fprintf(out_fd,"end of this vquad chain\n");
    else
	fprintf(out_fd,"next vquad in the chain\n");


    fprintf(out_fd,"%sbuffer        = %d.0x%08x   data buffer\n",prefix,
	    vquad.buffer.u.data_sid,(int)vquad.buffer.u.data_vba);


    fprintf(out_fd,"%saddress class = %d              ",prefix,
	    (int)vquad.addr_class);
    switch (vquad.addr_class) {
	case VIRTUAL_BUFFER:  fprintf(out_fd,"VIRTUAL_BUFFER\n");
			      break;
	case VIRTUAL_BLOCKS:  fprintf(out_fd,"VIRTUAL_BLOCKS - unsupported\n");
			      return(-1);
			      break;
	case CONTIGUOUS_REAL: fprintf(out_fd,"CONTIGUOUS_REAL\n");
			      break;
	default:	      fprintf(out_fd,"nothing!\n");
			      return(-1);
    }


    /* return the address of the next vquad in the chain (if any) */
    *nextlink = vquad.link;
    return(0);
}

an_cio_ca0_align_pquad_chain (pquad_addr, prefix, out_fd)

    int			pquad_addr;
    char		*prefix;
    FILE		*out_fd;

{
    int			next;
    int			cur;
    char		nprefix[80];

    strcpy(nprefix,prefix);
    strcat(nprefix,"   ");
    an_vtor(0,pquad_addr,&cur);
    while (cur)  {
	if (an_cio_ca0_pquad(cur,0,nprefix,&next,1,out_fd) == 0) 
	    cur = next;
	else  {
	    fprintf(out_fd,"%spquad at 0x%08x is suspect\n\n",nprefix,cur);
	    return(-1);
	}
	fprintf(out_fd,"\n");
	if (cur)
	    fprintf(out_fd,"%snext pquad address = 0x%08x\n",prefix,cur);
    }
}

an_cio_ca0_pquad_chain (pquad_addr, muxcard, prefix, out_fd)

    int			pquad_addr;
    int			muxcard;
    char		*prefix;
    FILE		*out_fd;

{
    int			next;
    int			cur;
    char		nprefix[80];

    strcpy(nprefix,prefix);
    strcat(nprefix,"   ");
    an_vtor(0,pquad_addr,&cur);
    while ((cur & 1) == 0)  {
	if (an_cio_ca0_pquad(cur,muxcard,nprefix,&next,0,out_fd) == 0) 
	    cur = next;
	else  {
	    fprintf(out_fd,"%spquad at 0x%08x is suspect\n\n",nprefix,cur);
	    return(-1);
	}
	fprintf(out_fd,"\n");
	if ((cur & 1) == 0)
	    fprintf(out_fd,"%snext pquad address = 0x%08x\n",prefix,cur);
    }
}

an_cio_ca0_pquad (pquadaddr, muxcard, prefix, nextlink, link_type, out_fd)

    int				pquadaddr;
    int				muxcard;
    char			*prefix;
    int				*nextlink;
    int				link_type;	/* 0: return pquad.link */
						/* 1: return pquad.next_align */
    FILE			*out_fd;
{
    pquad_type			pquad;
    pquad_type			pquad2;
    int				logc;
    char			nprefix[80];
    int				i;

    /* initialization */
    strcpy(nprefix,prefix);
    strcat(nprefix,"                       ");

    /* grab this pquad and make sure we own it */
    if (an_grab_real_chunk(pquadaddr,&pquad,sizeof(pquad_type))!=0) {
	fprintf(stderr,"Couldn't get pquad\n");
	return(-1);
    }

    /* return the address of the next quad in the chain (if any) */
    if (link_type == 0)
	*nextlink = pquad.link;
    else
	*nextlink = (int *)pquad.next_align;

    /* interpret this pquad */
    fprintf(out_fd,"%slink          = 0x%08x     ",prefix, (int)pquad.link);
    if (((int)pquad.link & 1) == 1)
	fprintf(out_fd,"end of this pquad chain\n");
    else
	fprintf(out_fd,"next pquad in the chain\n");


    fprintf(out_fd,"%scommand       = 0x%08x     ",prefix,pquad.command);
    an_cio_ca0_cio_cmd(pquad.command,out_fd);


    fprintf(out_fd,"%sbuffer        = 0x%08x     ",prefix,(int)pquad.address);
    if (pquad.command == INIT_LC)  {
	logc = ((int)pquad.count & 0x7f0) >> 4;
	if (logc == 0x7f)
	    fprintf(out_fd,"dummy entry\n");
	else {
	    fprintf(out_fd,"first quad -- ");
	    if (an_grab_real_chunk(pquad.address,&pquad2,sizeof(pquad_type))!=0)
		fprintf(out_fd,"couldn't get pquad\n");
	    else
	        an_cio_ca0_cio_cmd(pquad2.command,out_fd);
	}
    }
    else if ((pquad.command == INT_LNKST) || (pquad.command  == CIOCA_LNKST)) {
	fprintf(out_fd,"completion list head\n");
    }
    else  {
	fprintf(out_fd,"data buffer\n");
	if ( (pquad.command == CIOCA_CLC)  ||
	     (pquad.command == CIOCA_RS)   ||
	     (pquad.command == CIOCA_RS_D) ||
	     (pquad.command == CIOCA_WTC) )
	    an_cio_ca0_dump_buffer(pquad.address,pquad.count,nprefix,out_fd);
    }



    fprintf(out_fd,"%scount         = 0x%08x     ",prefix,pquad.count);
    if (pquad.command == INIT_LC)  {
	fprintf(out_fd,"logc chain ram address ");
	if (logc == 0x7f)
	    fprintf(out_fd,"(dummy)\n");
	else
	    fprintf(out_fd,"(logc# 0x%x)\n",logc);
    }
    else if ((pquad.command == INT_LNKST) || (pquad.command  == CIOCA_LNKST)) {
	fprintf(out_fd,"completion list entry\n");
	if (pquad.address == DLISTHEAD)
	    fprintf(out_fd,"%sdummy completion list\n",nprefix);
	else if (!muxcard)  {
	    an_cio_ca0_compl_entry(pquad.count,&i,nprefix,out_fd);
	    if (an_cio_ca0_on_compl_list(pquad.count,pquad.address))
		fprintf(out_fd,"%scurrently on completion list\n",nprefix);
	    else
		fprintf(out_fd,"%scurrently not on completion list\n",nprefix);
	}
    }
    else  {
	fprintf(out_fd,"size of transfer\n");
    }


    /* mux driver uses a 4-word pquad only! */
    if (muxcard)
        return(0);

    if ( (pquad.command != INIT_LC) &&
         (pquad.command != INT_LNKST) &&
         (pquad.command != CIOCA_LNKST) ) {
        fprintf(out_fd,"%svquad         = 0x%08x     ",prefix,(int)pquad.vquad);
        fprintf(out_fd,"vquad of last pquad\n");
    }


    /* decode next_align */
    if ( (pquad.command != INIT_LC) &&
         (pquad.command != INT_LNKST) &&
         (pquad.command != CIOCA_LNKST) ) {
        fprintf(out_fd,"%snext_align    = 0x%08x     ",prefix,
		(int)pquad.next_align);
        if ((int)pquad.next_align)
	    fprintf(out_fd,"if in align chain, next pquad\n");
        else
	    fprintf(out_fd,"if in align chain, end of chain\n");
    }

    return(0);
}

an_cio_ca0_poll_chain (structaddr, prefix, nextlink, out_fd)

    int				structaddr;
    char			*prefix;
    int				*nextlink;
    FILE			*out_fd;
{
    struct cio_poll_chain	da_list;

    /* grab this da_list element */
    if (an_grab_virt_chunk(0,structaddr,&da_list,sizeof(cio_poll_chain))!=0) {
	fprintf(stderr,"Couldn't get poll_chain element\n");
	return(0);
    }

    /* decode this element and print it out */
    fprintf(out_fd,"%slink = 0x%08x  ",prefix,(int)da_list.link);
    if ((int)da_list.link == 0)
	fprintf(out_fd,"last element in the chain\n");
    else
	fprintf(out_fd,"pointer to next element in the chain\n");

    fprintf(out_fd,"%s",prefix);
    aio_decode_llio_status(da_list.llio_status,out_fd);

    fprintf(out_fd,"%sda number = %d (0x%x)\n",prefix,
	    (int)da_list.da_number,(int)da_list.da_number);

    fprintf(out_fd,"%spquad chain = 0.0x%08x\n",prefix,(int)da_list.pquad_chain);
    an_cio_ca0_pquad_chain((int)da_list.pquad_chain,1,prefix,out_fd);

    /* return the link field */
    *nextlink = (int)da_list.link;
}

an_cio_ca0_dump_buffer (paddr, count, prefix, out_fd)

    unsigned		paddr;
    int			count;
    char		*prefix;
    FILE		*out_fd;
{
    int			i;
    int			limit;
    int			buffer[8];


    /* try to get the buffer */
    if (an_grab_real_chunk(paddr,buffer,32) != 0)
	fprintf(out_fd,"%s couldn't get buffer!\n",prefix);

    else {

	/* print prefix and calculate number of words to print */
	fprintf(out_fd,"%s",prefix);
	if (count < 32)
	    limit = count / 4;
	else
	    limit = 8;

	/* print buffer as hex words -- newline after 4 elements */
	for (i=0 ; i<limit ; i++) {
	    if (i == 4)
		fprintf(out_fd,"\n%s",prefix);
	    fprintf(out_fd,"0x%08x ",buffer[i]);
	}
        fprintf(out_fd,"\n");

    }
}

an_cio_ca0_cam_buffers (bufaddr, prefix, out_fd)

    int		bufaddr;
    char	*prefix;
    FILE	*out_fd;
{

    cam_buffer  buf;
    int		next;


    /* print buffers */
    next = bufaddr;
    do {
	/* grab the buffer */
	if (an_grab_virt_chunk(0,next,&buf,sizeof(cam_buffer)))
	    break;

	/* decode the buffer */
	fprintf(out_fd,"%sbuffer at 0x%08x:\n",prefix,next);
	if ((int)buf.next_buffer)
	    fprintf(out_fd,"%s   next_buffer  =  0x%08x\n",
	    	    prefix,(int)buf.next_buffer);
	else
	    fprintf(out_fd,"%s   next_buffer  =  0  (end of chain)\n",prefix);

	if ((int)buf.first_initlc)
	    fprintf(out_fd,"%s   first_initlc =  0x%08x (init_lc chain)\n",
	    	    prefix,(int)buf.first_initlc);
	else
	    fprintf(out_fd,"%s   first_quad   =  0  (not in use)\n",prefix);

	if ((int)buf.first_quad)
	    fprintf(out_fd,"%s   first_quad   =  0x%08x (quad chain)\n",
	    	    prefix,(int)buf.first_quad);
	else
	    fprintf(out_fd,"%s   first_quad   =  0  (not in use)\n",prefix);

	fprintf(out_fd,"\n");

	/* advance to the next buffer */
	next = (int)buf.next_buffer;

    } while (next != 0);

    /* grab the first buffer again! */
    if (an_grab_virt_chunk(0,bufaddr,&buf,sizeof(cam_buffer)))
	return;

    /* print the subchannel part of logchannel chain if there */
    if ((int)buf.first_initlc) {
	fprintf(out_fd,"%sSubchannel Part of Logchannel Chain at 0x%08x:\n",
		prefix,(int)buf.first_initlc);
	an_cio_ca0_pquad_chain(buf.first_initlc,0,"         ",out_fd);
	fprintf(out_fd,"\n");
    }

    /* print the other part of the chain */
    if ((int)buf.first_quad) {
	fprintf(out_fd,"%sI/O Quad Chain at 0x%08x:\n",
		prefix,(int)buf.first_quad);
	an_cio_ca0_pquad_chain(buf.first_quad,0,"         ",out_fd);
    }

}

an_cio_ca0_bcheck (out_fd)

    FILE	*out_fd;
{

    int		tid;
    int 	trouble = 0;
    cam_buffer  buf;
    int		bufaddr;


    fprintf(out_fd,"\nBuffer Checking:\n");

    /* check quad chains in use */
    for (tid=0 ; tid<(NUMDA+NUMLOGC-1) ; tid++)  {

	if ( ( ( (tid<NUMDA) && (SUBC[tid].ispoll==0) && (TID[tid].msg!=0) ) ||
	     ( (tid >= NUMDA) && (TID[tid].msg != 0) ) ) ) {

	     /* grab the buffer if transaction active */
	     if ( ((int)TID[tid].buffers != 0) &&
                  (an_grab_virt_chunk(0,(int)TID[tid].buffers,&buf,
				      sizeof(cam_buffer)) != 0) ) {

		/* check completion and residue status buffers */
		if (an_cio_ca0_pquad_chain_bcheck(buf.first_quad) != 0) {
		    fprintf(out_fd,"  TID[0x%x].buffers has a problem!\n",tid);
		    an_cio_ca0_pquad_chain(buf.first_quad,0,"     ",out_fd);
		    trouble = 1;
		}

		/* check all big buffers */
		bufaddr = (int)TID[tid].buffers;
		do {
                    if (an_grab_virt_chunk(0,bufaddr,&buf,sizeof(cam_buffer)))
			break;
		    if (aio_mem_on_free_list(bufaddr,CAM_BUF_SIZE,stderr))
		        break;
		    bufaddr = (int)buf.next_buffer;
		} while (bufaddr != 0);

	     }
	}

	/* check wtc chain for destroying logchannels */
	if ( (tid >= NUMDA) && (TID[tid].wtc_buffer != 0) &&
	     (an_cio_ca0_pquad_chain_bcheck(TID[tid].wtc_buffer) != 0) ) {
	    fprintf(out_fd,"  TID[0x%x].wtc_buffer has a problem!\n",tid);
	    an_cio_ca0_pquad_chain(TID[tid].wtc_buffer,0,"     ",out_fd);
	    trouble = 1;
	}

    }

    /* print a message if no problems */
    if (trouble == 0)
	fprintf(out_fd,"  no CAM-owned buffer also owned by i/o services\n\n");
}

an_cio_ca0_pquad_chain_bcheck (pquad_addr)

    int			pquad_addr;

{
    int			next;
    int			cur;

    an_vtor(0,pquad_addr,&cur);
    while ((cur & 1) == 0)  {
	if (an_cio_ca0_pquad_bcheck(cur,&next) == 0) 
	    cur = next;
	else
	    return(1);
    }
}

an_cio_ca0_pquad_bcheck (pquadaddr, nextlink)

    int				pquadaddr;
    int				*nextlink;
{
    pquad_type			pquad;


    /* grab this pquad */
    if (an_grab_real_chunk(pquadaddr,&pquad,sizeof(pquad_type))!=0)
	return(1);

    /* return the address of the next quad in the chain (if any) */
    *nextlink = pquad.link;

    /* check lnkst completion entry */
    if ( ((pquad.command == INT_LNKST) || (pquad.command  == CIOCA_LNKST))  &&
	 (pquad.count == LISTHEAD) &&
         (aio_mem_on_free_list(pquad.count,32,"    ",stderr)) ) 
	    return(1);

    return(0);
}

an_cio_ca0_cio_cmd(cmd, out_fd)

    cio_cmd_type 	cmd;
    FILE		*out_fd;
{

#define	BITS	(BYTE_MODE|CIOCA_S1S2|LOGCH_BREAK|BLOCKED|CONTINUE)

    switch (cmd.cio_cmd & ~BITS) {
	case CIOCA_IDY  & ~BITS: fprintf(out_fd,"CIOCA_IDY");	break;
	case CIOCA_DLD  & ~BITS: fprintf(out_fd,"CIOCA_DLD");	break;
	case CIOCA_DLD1 & ~BITS: fprintf(out_fd,"CIOCA_DLD1");	break;
	case CIOCA_PAU  & ~BITS: fprintf(out_fd,"CIOCA_PAU");	break;
	case CIOCA_DIS  & ~BITS: fprintf(out_fd,"CIOCA_DIS");	break;
	case CIOCA_RS   & ~BITS: fprintf(out_fd,"CIOCA_RS");	break;
	case CIOCA_RS_D & ~BITS: fprintf(out_fd,"CIOCA_RS_D");	break;
	case CIOCA_WC   & ~BITS: fprintf(out_fd,"CIOCA_WC");	break;
	case CIOCA_RD   & ~BITS: fprintf(out_fd,"CIOCA_RD");	break;
	case CIOCA_WD   & ~BITS: fprintf(out_fd,"CIOCA_WD");	break;
	case CIOCA_RTS  & ~BITS: fprintf(out_fd,"CIOCA_RTS");	break;
	case CIOCA_WTC  & ~BITS: fprintf(out_fd,"CIOCA_WTC");	break;
	case CIOCA_RDS  & ~BITS: fprintf(out_fd,"CIOCA_RDS");	break;
	case CIOCA_RDS1 & ~BITS: fprintf(out_fd,"CIOCA_RDS1");	break;
	case CIOCA_CLC  & ~BITS: fprintf(out_fd,"CIOCA_CLC");	break;
	case CIOCA_LNKST& ~BITS: fprintf(out_fd,"CIOCA_LNKST");	break;
	case INT_LNKST  & ~BITS: fprintf(out_fd,"INT_LNKST");	break;
	case INIT_LC    & ~BITS: fprintf(out_fd,"INIT_LC");	break;
	default:		 fprintf(out_fd,"unknown");	break;
    }
    if (cmd.cio_cmd & BYTE_MODE)   fprintf(out_fd," | BYTE_MODE");
    if (cmd.cio_cmd & CIOCA_S1S2)  fprintf(out_fd," | CIOCA_S1S2");
    if (cmd.cio_cmd & LOGCH_BREAK) fprintf(out_fd," | LOGCH_BREAK");
    if (cmd.cio_cmd & BLOCKED)	   fprintf(out_fd," | BLOCKED");
    if (cmd.cio_cmd & CONTINUE)    fprintf(out_fd," | CONTINUE");
    fprintf(out_fd,"\n");
}

an_cio_ca0_decode_message (msg, prefix, out_fd)

    cammsg_type		*msg;
    char		*prefix;
    FILE		*out_fd;

{
    int			i;
    int			reg;
    char		nprefix[80];

    /* for each msg type, decode it */
    switch (msg->msg_header.msg_descriptor)  {

	case CIO_DMA_IO_REQ_MSG:   
	    fprintf(out_fd,"CIO_DMA_IO_REQ_MSG\n");
	    fprintf(out_fd,"%sreply_subq = %d    da_number = %d\n",
	            prefix,msg->u.dma_req.reply_subq,
	            msg->u.dma_req.da_number);
	    i = (int)msg->u.dma_req.vquad_chain;
	    fprintf(out_fd,"%svquad_chain address = 0x%08x\n",prefix,i);
	    an_cio_ca0_vquad_chain(i,prefix,out_fd);
	    break;

	case CIO_DMA_IO_REPLY_MSG:
	    fprintf(out_fd,"CIO_DMA_IO_REPLY_MSG\n");
	    fprintf(out_fd,"%s",prefix);
	    aio_decode_llio_status(msg->u.dma_reply.llio_status,out_fd);
	    i = (int)msg->u.dma_reply.vquad_chain;
	    fprintf(out_fd,"%svquad_chain address = 0x%08x\n",prefix,i);
	    an_cio_ca0_vquad_chain(i,prefix,out_fd);
	    break;

	case CIO_CTRL_REQ_MSG:   
	    fprintf(out_fd,"CIO_CTRL_REQ_MSG\n");
	    fprintf(out_fd,"%sreply_subq = %d    da_number = %d\n",
	            prefix,msg->u.ctrl_req.reply_subq,
	            msg->u.ctrl_req.da_number);
	    fprintf(out_fd,"%sctrl_func = %d -- ",prefix,
	            msg->u.ctrl_req.ctrl_func);

	    switch (msg->u.ctrl_req.ctrl_func)  {

		case CIO_AEK_COMMAND:      
		    fprintf(out_fd,"CIO_AEK_COMMAND");
		    break;
		case CIO_DA_SELFTEST:     
		    fprintf(out_fd,"CIO_DA_SELFTEST");
		    break;
		case CIO_DA_STATUS:    
		    fprintf(out_fd,"CIO_DA_STATUS");
		    break;
		case CIO_DA_ASYNC_ENABLE:
		    fprintf(out_fd,"CIO_DA_ASYNC_ENABLE");
		    break;
		case CIO_DA_ASYNC_DISABLE:  
		    fprintf(out_fd,"CIO_DA_ASYNC_DISABLE");
		    break;
		case CIO_SET_DA_BLOCKSIZE: 
		    fprintf(out_fd,"CIO_SET_DA_BLOCKSIZE");
		    break;
		case CIO_DA_PARITY_ENABLE:
		    fprintf(out_fd,"CIO_DA_PARITY_ENABLE");
		    break;
		case CIO_DA_LOGCH_ENABLE:
		    fprintf(out_fd,"CIO_DA_LOGCH_ENABLE");
		    break;
		case CIO_DA_LOGCH_DISABLE:  
		    fprintf(out_fd,"CIO_DA_LOGCH_DISABLE");
		    break;
		case CIO_GET_DIRECT_IO_PTR:
		    fprintf(out_fd,"CIO_GET_DIRECT_IO_PTR");
		    break;
		case CIO_GET_DA_TYPE:
		    fprintf(out_fd,"CIO_GET_DA_TYPE");
		    break;
		default:
		    fprintf(out_fd,"unknown!!");

	    }

	    if (msg->u.ctrl_req.ctrl_func == CIO_SET_DA_BLOCKSIZE)
		fprintf(out_fd,"     ctrl_parm (blocksize) = %d\n",
			msg->u.ctrl_req.ctrl_parm);
	    else
		fprintf(out_fd,"\n");

	    break;

	case CIO_CTRL_REPLY_MSG:
	    fprintf(out_fd,"CIO_CTRL_REPLY_MSG\n");
	    fprintf(out_fd,"%s",prefix);
	    aio_decode_llio_status(msg->u.ctrl_reply.llio_status,out_fd);
	    fprintf(out_fd,"%sctrl_info = 0x%08x\n",
			prefix,msg->u.ctrl_reply.ctrl_info);
	    break;

	case CIO_IO_EVENT_MSG: 
	    fprintf(out_fd,"CIO_IO_EVENT_MSG\n");
	    fprintf(out_fd,"%sevent = %d ",prefix,msg->u.io_event.event);
	    if (msg->u.io_event.event == CIO_TRANSPARENT_STATUS)  {
		fprintf(out_fd,"(CIO_TRANSPARENT_STATUS)\n");
		fprintf(out_fd,"%sbyte info:  ",prefix);
		for (i=0 ; i<16 ; i++)
		    fprintf(out_fd,"0x02%x  ",msg->u.io_event.u.byte_info[i]);
		fprintf(out_fd,"    ");
		switch (msg->u.io_event.u.byte_info[0])  {
		    case CIO_RTS_STATUS_IDL:
			fprintf(out_fd,"(idle)\n");
			break;
		    case CIO_RTS_STATUS_SWI:
			fprintf(out_fd,"(logchannel switch)\n");
			break;
		    case CIO_RTS_STATUS_EOD:
			fprintf(out_fd,"(end of data)\n");
			break;
		    case CIO_RTS_STATUS_LCD:
			fprintf(out_fd,"(logchannel destroyed)\n");
			break;
		    case CIO_RTS_STATUS_AES:
			fprintf(out_fd,"(asynchronous event sensed)\n");
			break;
		    case CIO_RTS_STATUS_ERT:
			fprintf(out_fd,"(error trap)\n");
			break;
		    default:
			fprintf(out_fd,"(unknown!!)\n");
		}
	    }
	    else if (msg->u.io_event.event == CIO_ARQ_STATUS)  {
		fprintf(out_fd,"(CIO_ARQ_STATUS)\n");
		fprintf(out_fd,"%sint_info[0] (status register) = 0x%08x    ",
			prefix,msg->u.io_event.u.int_info[0]);
		reg = msg->u.io_event.u.int_info[0] & 0xf0;
		switch (reg)  {
		    case CIO_SUBC_STATUS_RFC:
			fprintf(out_fd,"(ready for command)\n");
			break;
		    case CIO_SUBC_STATUS_AES:
			fprintf(out_fd,"(asynchronous event sensed)\n");
			break;
		    case CIO_SUBC_STATUS_DPE:
			fprintf(out_fd,"(data parity error)\n");
			break;
		    case CIO_SUBC_STATUS_SCD:
			fprintf(out_fd,"(subchannel destroyed)\n");
			break;
		    case CIO_SUBC_STATUS_SCR:
			fprintf(out_fd,"(subchannel request)\n");
			break;
		    case CIO_SUBC_STATUS_DOD:
			fprintf(out_fd,"(dead or dying)\n");
			break;
		    case CIO_SUBC_STATUS_PER:
			fprintf(out_fd,"(protocol error)\n");
			break;
		    case CIO_SUBC_STATUS_ERT:
			fprintf(out_fd,"(error trap)\n");
			break;
		    default:
			fprintf(out_fd,"(unknown!!)\n");
		}
	    }
	    else
		fprintf(out_fd,"(unknown!!!)\n");

	    break;

	case CIO_INTERRUPT_MSG:
	    fprintf(out_fd,"CIO_INTERRUPT_MSG -- unused\n");
	    break;

	case CIO_POLL_DMA_REQ_MSG:
	    fprintf(out_fd,"CIO_POLL_DMA_REQ_MSG\n");
	    i = (int)msg->u.poll_req.link;
	    fprintf(out_fd,"%spoll_chain_pointer = 0x%08x\n",prefix,i);
	    while (i != 0)  {
		an_cio_ca0_poll_chain(i,prefix,&reg,out_fd);
		i = reg;
		if (i != 0)
		    fprintf(out_fd,"\n");
	    }
	    break;

	case CIO_POLL_ASYNC_MSG:
	    fprintf(out_fd,"CIO_POLL_ASYNC_MSG\n");
	    i = (int)msg->u.async_reply.link;
	    fprintf(out_fd,"%spoll_chain_pointer = 0x%08x\n",prefix,i);
	    while (i != 0)  {
		an_cio_ca0_poll_chain(i,prefix,&reg,out_fd);
		i = reg;
		if (i != 0)
		    fprintf(out_fd,"\n");
	    }
	    break;

	case CIO_DMA_DUMP_REQ_MSG:
	    fprintf(out_fd,"CIO_DMA_DUMP_REQ_MSG\n");
	    fprintf(out_fd,"%scam_port = %d    da_number = %d",
	            prefix,msg->u.dump_req.cam_port,
	            msg->u.dump_req.da_number);
	    i = (int)msg->u.dump_req.vquad_chain;
	    fprintf(out_fd,"vquad_chain address = 0x08x\n",i);
	    an_cio_ca0_vquad_chain(i,prefix,out_fd);
	    break;

        default:
	    fprintf(out_fd,"unknown message type!\n");

    }
}

an_cio_ca0_decode_status (llio_status, out_fd)

    llio_status_type	llio_status;
    FILE		*out_fd;

{
    fprintf(out_fd,"cio_ca0 llio_status = <0x%08x> means ",llio_status);

    switch (llio_status.u.error_num)  {

        case CIO_DAM_HAS_PENDING_REQUEST :
            fprintf(out_fd,"CIO_DAM_HAS_PENDING_REQUEST\n");
	    break;

        case CIO_UNSTARTED_ABORT :
            fprintf(out_fd,"CIO_UNSTARTED_ABORT\n");
	    break;

        case CIO_ABORTED_WHILE_ACTIVE :
            fprintf(out_fd,"CIO_ABORTED_WHILE_ACTIVE\n");
	    break;

        case CIO_LVL1_CARD :
            fprintf(out_fd,"CIO_LVL1_CARD\n");
	    break;

	default :
	    fprintf(out_fd,"NOTHING! -- can't decode!\n");

    }

}
