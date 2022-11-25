/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/netmux.c,v $
 * $Revision: 1.14.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:28:27 $
 *
 */
/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

#include "../standard/inc.h"
#include "net.h"  	/*includes global network definitions*/

/* sioserv/dptcore.h defines ASYNC and sioserv/msg_.h defines MAX_MSG_SIZE.
 * The tty code defines these also, so undefine them here.
 */
#undef ASYNC
#undef MAX_MSG_SIZE

/*for tty struct accessed in mux_data_struct*/
#ifdef BUILDFROMH
#include <h/tty.h>
#else
#include <sys/tty.h>
#endif BUILDFROMH

/*main data structures*/
#include <sio/mux0.h>  

/*includes cam/mux token definitions*/
#include "../objects/y.tab.h"

#include "../standard/defs.h" 	/*                                   */
#include "../standard/types.h"      /*                                   */
#include "../standard/externs.h"    /*                                   */

int    max_mux;
struct mux_hwdata   *mux0_hwdata,*vmux0_hwdata;
struct mux_data_struct     *mux_data,*vmux_data;
struct cam_data_struct     *cam_data,*vcam_data;
int    num_cio_ca0;

#define X_MUX0_HWDATA 0 
#define X_MUX_DATA    1
#define X_CAM_DATA    2
#define X_MAX_MUX     3
#define X_NUM_CIO_CA0 4

struct nlist mux_nl[] = {
#ifdef OLD_NNC
        {"_mux0_hwdata"},
        {"_mux_data"},
        {"_cam_data"},
        {"_max_mux"},
        {"_num_cio_ca0"},
#else
        {"mux0_hwdata"},
        {"mux_data"},
        {"cam_data"},
        {"max_mux"},
        {"num_cio_ca0"},
#endif OLD_NNC
        {""}
};


/*********************************************************/

/* do nlist of mux structures */
mux_nlist()
{
    struct nlist *np;

    for (np = mux_nl; *np->n_name != '\0' ; np++)
        np->n_value = lookup(np->n_name);
}


/* dump the addresses of the cam/mux stuff to output for the ":show sym"
 * command.   i is the column number we are printing at.
 */
muxdump_address(i)
register int i;
{
    register struct nlist *np;
    
    for (np = mux_nl; np->n_name && *np->n_name; np++, i++) {
	fprintf(outf," %8.8s: 0x%08x  ", np->n_name, np->n_value);
	if (i == 3) {
	    i = 0;
	    fprintf(outf,"\n");
	}
    }
    return (i);
}



/* get addresses of mux stuff from nlist structure */
mux_getaddrs()
{
    /* get config parameters */
    num_cio_ca0 = get(mux_nl[X_NUM_CIO_CA0].n_value);
    max_mux = get(mux_nl[X_MAX_MUX].n_value);

    /* get addresses of tables */
    vmux_data = (struct mux_data_struct *)mux_nl[X_MUX_DATA].n_value;
    vmux0_hwdata = (struct mux_hwdata *)mux_nl[X_MUX0_HWDATA].n_value;
    vcam_data = (struct cam_data_struct *)mux_nl[X_CAM_DATA].n_value;

    /* reserve space for tables */
    mux_data = (struct mux_data_struct *)net_malloc(max_mux*sizeof(struct mux_data_struct));
    mux0_hwdata = (struct mux_hwdata *)net_malloc(max_mux*MUXPORTS*sizeof(struct mux_hwdata));
    cam_data = (struct cam_data_struct *)net_malloc(num_cio_ca0*sizeof(struct cam_data_struct));
}

/* read in cam/mux tables */
mux_gettables()
{
    netvread(vmux_data, mux_data, max_mux*sizeof(struct mux_data_struct));
    netvread(vcam_data, cam_data, num_cio_ca0*sizeof(struct cam_data_struct));
    if (netvread(vmux0_hwdata, mux0_hwdata, max_mux*MUXPORTS*sizeof(struct mux_hwdata))!=
	max_mux*MUXPORTS*sizeof(struct mux_hwdata))
	    NERROR("error reading mux0_hwdata", 0);
}



/* scan cam and mux data structures for integrity */
muxscan()
{
    /* To Do: add integrity checks here */;
}



/* convert an index into a kernel virtual address */
mux_ntov(type, index)
int type, index;
{
    switch (type) {
        case TMUXDATA:
	    if (index >= max_mux) return(-1);
	    return ((int)(&vmux_data[index]));
	case TMUXCAMDATA:
	    if (index >= num_cio_ca0) return(-1);
	    return ((int)(&vcam_data[index]));
	case TMUXHWDATA:
	    if (index >= (max_mux*MUXPORTS)) return(-1);
    	    return ((int)(&vmux0_hwdata[index]));
    }
}

/* return non-zero if vaddr is a valid kernel address for the
 * indicated data structure.
 */
mux_validv(type, vaddr)
int type, vaddr;
{
    switch (type) {
        case TMUXDATA:
	   if ((vaddr < vmux_data) || (vaddr >= (int)(&vmux_data[max_mux])))
         	return(0);
	   break;
	case TMUXCAMDATA:
	   if ((vaddr < vcam_data) || (vaddr >= (int)(&vcam_data[num_cio_ca0])))
		return(0);
	   break;
	case TMUXHWDATA:
	   if ((vaddr < vmux0_hwdata) || (vaddr >= (int)(&vmux0_hwdata[max_mux])))
		return(0);
	   break;
    }
    return (1);
}

muxdump_mux_data(muxp,vaddr)
register struct mux_data_struct *muxp;
uint vaddr;
{
#define kernelize(a)     ((uint)&a - (uint)muxp + vaddr)

  uint phyaddr;

     fprintf(outf,"mux_data at 0x%08x\n",vaddr);

     fprintf(outf,"is_config  = 0x%02x        ",muxp->is_config);
     fprintf(outf,"is_busy    = 0x%02x        ",muxp->is_busy);
     fprintf(outf,"is_broken  = 0x%02x\n",muxp->is_broken);
     fprintf(outf,"is_loaded  = 0x%02x        ",muxp->is_loaded);
     fprintf(outf,"is_polled  = 0x%02x        ",muxp->is_polled);
     fprintf(outf,"stop_poll  = 0x%02x\n",muxp->stop_poll);
     fprintf(outf,"is_pfail   = 0x%02x        ",muxp->is_pfail);
     fprintf(outf,"outbuf_flag= 0x%02x\n\n",muxp->outbuf_flag);
     
     fprintf(outf,"inbufp     = 0x%08x  ",muxp->inbufp);
     fprintf(outf,"outbuf1p   = 0x%08x  ",muxp->outbuf1p);
     fprintf(outf,"outbuf2p   = 0x%08x\n",muxp->outbuf2p);
     fprintf(outf,"outbufp    = 0x%08x  ",muxp->outbufp);
     fprintf(outf,"genbufp    = 0x%08x  ",muxp->genbufp);
     fprintf(outf,"ttyp       = 0x%08x\n\n",muxp->ttyp);

     fprintf(outf,"cycle_fail = 0x%04x      ",muxp->cycle_fail);
     fprintf(outf,"total_fail = 0x%02x        ",muxp->total_fail);
     fprintf(outf,"semaphore  = %-8d\n",muxp->semaphore);

     fprintf(outf,"cam_port   = %-10d  ",muxp->cam_port);
     fprintf(outf,"trn        = 0x%08x  ",muxp->trn);
     fprintf(outf,"da_number  = %-2d\n",muxp->da_number);
     fprintf(outf,"result     = %-2d          ",muxp->result);
     fprintf(outf,"dma_done   = %d\n\n",muxp->dma_done);

     fprintf(outf,"chain_head data :\n");
     fprintf(outf,"pquad_chain= 0x%08x  ",muxp->chain_head.pquad_chain);
     fprintf(outf,"is_ok      = %-10d  ",muxp->chain_head.llio_status.is_ok);
     fprintf(outf,"da_number  = 0x%02x ",muxp->chain_head.da_number);
     fprintf(outf,"\n\n");

     fprintf(outf,"gen_quad data : \n");
     fprintf(outf,"cio_cmd    = 0x%08x  ",muxp->gen_quad.command.cio_cmd);
     fprintf(outf,"count      = %-10d  ",muxp->gen_quad.count);
     fprintf(outf,"residue    = %d\n",muxp->gen_quad.residue);
     fprintf(outf,"link       = 0x%08x  ",muxp->gen_quad.link);
     fprintf(outf,"buffer     = 0x%08x  ",muxp->gen_quad.buffer);
     fprintf(outf,"addr_class = 0x%02x\n\n",muxp->gen_quad.addr_class);
}   

     
camdump_cam_data(camp,vaddr)
register struct cam_data_struct *camp;
uint vaddr;
{
    int ctr;

    fprintf(outf,"\n");
    fprintf(outf,"cam_data at 0x%08x\n",vaddr);
    fprintf(outf,"mux_lus = "); 
    for (ctr = 0;ctr < 16;ctr++)
       fprintf(outf,"%d  ",camp->mux_lus[ctr]);
    fprintf(outf,"\n");

    fprintf(outf,"port_num      = %-2d    ",camp->port_num);
    fprintf(outf,"sync_fail   = %d\n\n",camp->sync_fail);

    fprintf(outf,"msg_header data :\n");
    fprintf(outf,"msg_descriptor= 0x%04x  ",camp->my_msg.msg_header.msg_descriptor);
    fprintf(outf,"message_id  = 0x%04x  ",camp->my_msg.msg_header.message_id);
    fprintf(outf,"transaction_num = 0x%-04x\n",camp->my_msg.msg_header.transaction_num);
    fprintf(outf,"from_port     = %d\n",camp->my_msg.msg_header.from_port);
}

muxdump_mux0_hwdata(muxhwp,vaddr)
register struct mux_hwdata *muxhwp;
uint vaddr;
{
    int ctr;

    fprintf(outf,"\n");
    fprintf(outf,"mux_hwdata at 0x%08x\n\n",vaddr);

    fprintf(outf,"t_hwdata :\n");
    fprintf(outf,"char_mask    = 0x%02x       ",muxhwp->t_hwdata.char_mask);
    fprintf(outf,"speed        = 0x%02x        ",muxhwp->t_hwdata.speed);
    fprintf(outf,"xon_in       = 0x%04x\n",muxhwp->t_hwdata.xon_in);
    fprintf(outf,"xon_out      = 0x%04x     ",muxhwp->t_hwdata.xon_out);
    fprintf(outf,"delay_cnt    = 0x%08x  ",muxhwp->t_hwdata.delay_cnt);
    fprintf(outf,"q_len        = 0x%02x\n",muxhwp->t_hwdata.q_len);
    fprintf(outf,"stop_wait    = 0x%02x       ",muxhwp->t_hwdata.stop_wait);
    fprintf(outf,"modem_wait   = 0x%08x  ",muxhwp->t_hwdata.modem_wait);
    fprintf(outf,"open         = 0x%02x\n",muxhwp->t_hwdata.open);
    fprintf(outf,"modem_state  = 0x%02x       ",muxhwp->t_hwdata.modem_state);
    fprintf(outf,"disconnected = %d           ",muxhwp->t_hwdata.disconnected);
    fprintf(outf,"direct       = %-3d\n",muxhwp->t_hwdata.direct);
    fprintf(outf,"limit        = %d          ",muxhwp->t_hwdata.limit);
    fprintf(outf,"start        = %d           ",muxhwp->t_hwdata.start);
    fprintf(outf,"restart      = %-3d\n",muxhwp->t_hwdata.restart);
    fprintf(outf,"stopped      = %d          ",muxhwp->t_hwdata.stopped);

    fprintf(outf,"old_cflag    = 0x%04x      ",muxhwp->t_hwdata.old_cflag);
    fprintf(outf,"unit         = %d\n\n",muxhwp->t_hwdata.unit.S3);

    fprintf(outf,"acc_states(state,pcnt) :\n");
    for (ctr = 0;ctr < 4;ctr++) 
       fprintf(outf,"                         0x%02x  0x%02x\n",
		       muxhwp->t_hwdata.acc[ctr].state,
                       muxhwp->t_hwdata.acc[ctr].pcnt);
    fprintf(outf,"\n\n");

    fprintf(outf,"t_mdmdata : \n");
    fprintf(outf,"mdm_timer    = ");
    for (ctr = 0;ctr < NMTIMER;ctr++) {
        fprintf(outf,"0x%04x  ",muxhwp->t_mdmdata.mdm_timer.m_timers[ctr]);
    }
    fprintf(outf,"\n");
    fprintf(outf,"mdm_flag     = 0x%08x\n\n",muxhwp->t_mdmdata.mdm_flag);
}

muxdump(type,vaddr)
int type;
uint vaddr;
{
    switch (type) {
        case TMUXDATA:
	    muxdump_mux_data 
		(&mux_data[(struct mux_data_struct *)vaddr - vmux_data], vaddr);
	    break;
	case TMUXCAMDATA:
	    camdump_cam_data
		(&cam_data[(struct cam_data_struct *)vaddr - vcam_data], vaddr);
	    break;
	case TMUXHWDATA:
	    muxdump_mux0_hwdata
		(&mux0_hwdata[(struct mux_hwdata *)vaddr - vmux0_hwdata], vaddr);
	    break;
    }
}



/* Front-end for all cam/mux displays.  Called by the parser */
mux_display(type, param1, opt, redir, path)
int type, param1, opt, redir;
char *path;
{
    int nextv;
    register uint vaddr;

    NESAV;
    NECLR;
    
    NDBG(("mux_display(%#x,%#x,%#x,%#x,%#x)",type,param1,opt,redir,path));

    if (redir) {
        if ((outf = fopen(path,((redir == 2)?"a+":"w+"))) == NULL) {
            fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
            goto out;
        }
    }

    switch (opt) {

    case 'n':
        /* Get vaddr from index */
	vaddr = mux_ntov(type, param1);
        if (vaddr == -1) {
	    NERROR("index out of range", 0);
            goto reset;
	}
        break;
    case '\0' :
        /* Check validity of address */
        vaddr = param1;
	NESETVA(vaddr);
        if (!mux_validv(type, vaddr)) {
            NERROR("invalid address", 0);
            goto reset;
        }
        break;
    case 'a' :
        /* Display all, get first address */
	nextv = 0;
        vaddr = mux_ntov(type, nextv++); /* get first address */
	if (vaddr == -1) {
	    NERROR("no data structures in kernel", 0);
            goto reset;
	}
	NESETVA(vaddr);
        break;
    default: 
        /* bad option */
        NERROR("bad option", 0);
        goto reset;
    }

    NESETVA(vaddr);
    muxdump(type, vaddr);

    if (opt == 'a') {
        /* display all structures of type type */
	while ((vaddr = mux_ntov(type, nextv++)) != -1) {
            if (got_sigint) break;    /* break out of loop on sigint */
   	    NESETVA(vaddr);
	    muxdump(type, vaddr);
	}
    }
    
reset:
    /* close file if we redirected output */
    if (redir)
        fclose(outf);

out:
    outf = stdout;

    NERES;
}
