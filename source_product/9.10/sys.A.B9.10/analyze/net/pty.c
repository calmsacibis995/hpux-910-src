/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/pty.c,v $
 * $Revision: 1.13.83.3 $		$Author: root $
 * $State: Exp $		$Locker: rpc $
 * $Date: 93/09/17 16:28:45 $
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

#if !defined(lint)
static char rcsid[]="@(#) $Header: pty.c,v 1.13.83.3 93/09/17 16:28:45 root Exp $ (Hewlett-Packard)";
#endif


#include "../standard/inc.h"
#include "net.h"

/* sioserv/dptcore.h defines ASYNC and sioserv/msg_.h defines MAX_MSG_SIZE.
 * The tty code defines these also, so undefine them here.
 */
#undef ASYNC
#undef MAX_MSG_SIZE

#ifdef BUILDFROMH
#include <h/tty.h>
#include <h/pty.h>
#else
#include <sys/tty.h>
#include <sys/pty.h>
#endif BUILDFROMH

#include <sio/mux0.h>
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"

/* This is included for the pty/tty token defintions */
#include "../objects/y.tab.h"

long   npty;
struct tty *pt_tty, *vpt_tty;
struct pty_info *pty_info, *vpty_info;
struct tty *mux0_ttys, *vmux0_ttys;
int    max_mux;



#define X_PT_TTY    0
#define X_PTYINFO   1
#define X_MUX0_TTYS 2
#define X_NPTY	    3
#define X_MAX_MUX   4

struct nlist pty_nl[] = {
#ifdef OLD_NNC
	{ "_pt_tty"},
	{ "_pty_info"},
	{ "_mux0_ttys"},
	{ "_npty"}, 
	{ "_max_mux"}, 
#else  OLD_NNC
	{ "pt_tty"},
	{ "pty_info"},
	{ "mux0_ttys"},
	{ "npty"}, 
	{ "max_mux"}, 
#endif OLD_NNC
	{ "" }
};



/* do nlist of pty structures */
pty_nlist()
{
    struct nlist *np;
    for (np = pty_nl; *np->n_name != '\0' ; np++)
        np->n_value = lookup(np->n_name);
}


/* dump the addresses of the pty/tty stuff to output for the ":show sym"
 * command.   i is the column number we are printing at.
 */
ptydump_address(i)
register int i;
{
    register struct nlist *np;
    
    for (np = pty_nl; np->n_name && *np->n_name; np++, i++) {
	fprintf(outf," %8.8s: 0x%08x  ", np->n_name, np->n_value);
	if (i == 3) {
	    i = 0;
	    fprintf(outf,"\n");
	}
    }
    return (i);
}



/* get addresses of pty stuff from nlist structure */
pty_getaddrs()
{
    /* get config parameters */
    npty = get(pty_nl[X_NPTY].n_value);
    max_mux = get(pty_nl[X_MAX_MUX].n_value);

    /* get addresses of tables */
    vpt_tty = (struct tty *)pty_nl[X_PT_TTY].n_value;
    vpty_info = (struct pty_info *)pty_nl[X_PTYINFO].n_value;
    vmux0_ttys = (struct tty *)pty_nl[X_MUX0_TTYS].n_value;

    /* reserve space for tables */
    pt_tty = (struct tty *)net_malloc(npty*sizeof(struct tty));
    pty_info = (struct pty_info *)net_malloc(npty*sizeof(struct pty_info));
    mux0_ttys = (struct tty *)net_malloc(max_mux*MUXPORTS*sizeof(struct tty));
}

/* read in pty/tty tables */
pty_gettables()
{
    netvread(vpt_tty, pt_tty, npty*sizeof(struct tty));
    netvread(vpty_info, pty_info, npty*sizeof(struct pty_info));
    if (netvread(vmux0_ttys, mux0_ttys, max_mux*MUXPORTS*sizeof(struct tty))!=
	max_mux*MUXPORTS*sizeof(struct tty))
	    NERROR("error reading mux0_ttys", 0);
}



/* scan pty and tty data structures for integrity */
ptyscan()
{
    /* To Do: add integrity checks here */;
}



/* convert an index into a kernel virtual address */
pty_ntov(type, index)
int type, index;
{
    switch (type) {
        case TPTYINFO:
	    if (index >= npty) return(-1);
	    return ((int)(&vpty_info[index]));
	case TPTTY:
	    if (index >= npty) return(-1);
	    return ((int)(&vpt_tty[index]));
	case TTTY:
	    if (index >= (max_mux*MUXPORTS)) return(-1);
    	    return ((int)(&vmux0_ttys[index]));
    }
}

/* return non-zero if vaddr is a valid kernel address for the
 * indicated data structure.
 */
pty_validv(type, vaddr)
int type, vaddr;
{
    switch (type) {
        case TPTYINFO:
	    if ((vaddr < vpty_info) || (vaddr >= (int)(&vpty_info[npty])))
		return(0);
	    break;
	case TPTTY:
	    if ((vaddr < vpt_tty) || (vaddr >= (int)(&vpt_tty[npty])))
		return(0);
	    break;
	case TTTY:
	    if ((vaddr < vmux0_ttys) || (vaddr >= (int)(&vmux0_ttys[npty])))
		return(0);
	    break;
    }
    return (1);
}



/* Here's where the formatting is done: */

ptydump_pty_info(ptyp, vaddr)
register struct pty_info *ptyp;
uint vaddr;
{
	int ctr,tempctr;

        fprintf(outf,"\n");
	fprintf(outf, "pty_info at 0x%08x\n", vaddr);

	fprintf(outf, "pty_selr  :  ");
	fprintf(outf, "*pty_selp  = 0x%08x   ", ptyp->pty_selr.pty_selp);
	fprintf(outf, "pty_selflag= 0x%x\n", ptyp->pty_selr.pty_selflag);

	fprintf(outf, "pty_selw  :  ");
	fprintf(outf, "*pty_selp  = 0x%08x   ", ptyp->pty_selw.pty_selp);
	fprintf(outf, "pty_selflag= 0x%x\n", ptyp->pty_selw.pty_selflag);

	fprintf(outf, "pty_sele  :  ");
 	fprintf(outf, "*pty_selp  = 0x%08x   ", ptyp->pty_sele.pty_selp);
	fprintf(outf, "pty_selflag= 0x%x\n\n", ptyp->pty_sele.pty_selflag);

	fprintf(outf, "exclusive  = 0x%x   ", ptyp->exclusive);
	fprintf(outf, "ptmsleep   = 0x%x    ",ptyp->ptmsleep);
	fprintf(outf, "*u_procp   = 0x%08x\n",ptyp->u_procp);

	fprintf(outf, "trapbusy   = 0x%x   ",ptyp->trapbusy);
	fprintf(outf, "trapwait   = 0x%x    ",ptyp->trapwait);
	fprintf(outf, "trapnoshake= 0x%x\n",ptyp->trapnoshake);
	fprintf(outf, "trappending= 0x%x   ",ptyp->trappending);
	fprintf(outf, "tioctrap   = 0x%x    ", ptyp->tioctrap);

	fprintf(outf, "tiocsigmode= 0x%x\n", ptyp->tiocsigmode);
	fprintf(outf, "tioctty    = 0x%x   ", ptyp->tioctty);
	fprintf(outf, "tiocpkt    = 0x%x    ", ptyp->tiocpkt);
	fprintf(outf, "tiocmonitor= 0x%x\n", ptyp->tiocmonitor);
	fprintf(outf, "tiocremote = 0x%x\n\n", ptyp->tiocremote);

	fprintf(outf, "pktbyte    = 0x%02x  ", ptyp->pktbyte);

	fprintf(outf, "sendpktbyte= 0x%x\n\n", ptyp->sendpktbyte);

	fprintf(outf, "trapinfo  :  ");
	fprintf(outf, "request    = 0x%08x\n\n", ptyp->trapinfo.request);

	fprintf(outf, "ioctl_buf  : \n");
	tempctr = 0;
	for(ctr=0; ctr<IOCPARM_MASK+1; ctr++) {
             fprintf(outf, "0x%02x",ptyp->ioctl_buf[ctr]);
	     ++tempctr;
             if (tempctr == 16) {
	        tempctr = 0;
                fprintf(outf,"\n");	
	     }
             else 
                fprintf(outf," ");
	}
        fprintf(outf,"\n");

	fprintf(outf, "pty_state = 0x%08x\n", ptyp->pty_state);
}

pty_dumptty(ttyp, vaddr)
register struct tty *ttyp;
uint vaddr;
{
#define kernelize(a)	((uint)&a - (uint)ttyp + vaddr)
	int tmpctr,ctr;

	fprintf(outf, "t_rawq: ");
	fprintf(outf, "c_cc    = %-3d         ", ttyp->t_rawq.c_cc);
	fprintf(outf, "c_cf     = 0x%08x  ", ttyp->t_rawq.c_cf);
	fprintf(outf, "c_cl    = 0x%08x\n", ttyp->t_rawq.c_cl);
	fprintf(outf, "t_canq: ");
	fprintf(outf, "c_cc    = %-3d         ", ttyp->t_canq.c_cc);
	fprintf(outf, "c_cf     = 0x%08x  ", ttyp->t_canq.c_cf);
	fprintf(outf, "c_cl    = 0x%08x\n", ttyp->t_canq.c_cl);
	fprintf(outf, "t_outq: ");
	fprintf(outf, "c_cc    = %-3d         ", ttyp->t_outq.c_cc);
	fprintf(outf, "c_cf     = 0x%08x  ", ttyp->t_outq.c_cf);
	fprintf(outf, "c_cl    = 0x%08x\n", ttyp->t_outq.c_cl);
	fprintf(outf, "t_tbuf: ");
	fprintf(outf, "c_ptr   = 0x%08x  ", ttyp->t_tbuf.c_ptr);
	fprintf(outf, "c_count  = 0x%02x        ", ttyp->t_tbuf.c_count);
	fprintf(outf, "c_size  = 0x%02x\n", ttyp->t_tbuf.c_size);
	fprintf(outf, "t_rbuf: ");
	fprintf(outf, "c_ptr   = 0x%08x  ", ttyp->t_rbuf.c_ptr);
	fprintf(outf, "c_count  = 0x%02x        ", ttyp->t_rbuf.c_count);
	fprintf(outf, "c_size  = 0x%02x\n\n", ttyp->t_rbuf.c_size);

	fprintf(outf, "t_rsel  = 0x%x         ", ttyp->t_rsel);
	fprintf(outf, "t_wsel   = 0x%x\n\n", ttyp->t_wsel);

	fprintf(outf, "t_dev   = 0x%08x  ", ttyp->t_dev);
	fprintf(outf, "t_iflag  = 0x%04x     ", ttyp->t_iflag);
	fprintf(outf, "t_oflag = 0x%04x\n", ttyp->t_oflag);
	fprintf(outf, "t_cflag = 0x%04x      ", ttyp->t_cflag);
	fprintf(outf, "t_lflag  = 0x%04x     ", ttyp->t_lflag);
	fprintf(outf, "t_hp    = 0x%x\n", ttyp->t_hp);
	fprintf(outf, "t_state = 0x%08x  ", ttyp->t_state);
	fprintf(outf, "t_lmode  = 0x%x        ", ttyp->t_lmode);
	fprintf(outf, "t_pgrp  = 0x%x\n", ttyp->t_pgrp);
	fprintf(outf, "t_line  = 0x%x         ", ttyp->t_line);
	fprintf(outf, "t_delct  = 0x%x        ", ttyp->t_delct);
	fprintf(outf, "t_col   = 0x%x\n", ttyp->t_col);
	fprintf(outf, "t_row   = 0x%x         ", ttyp->t_row);
	fprintf(outf, "t_rocount= 0x%x        ", ttyp->t_rocount);
	fprintf(outf, "t_rocol = 0x%x\n", ttyp->t_rocol);
	fprintf(outf, "t_stpcnt= 0x%x         ", ttyp->t_stpcnt);
	fprintf(outf, "t_delay  = 0x%x\n\n", ttyp->t_delay);

  	fprintf(outf, "t_cc :\n");
 	tmpctr = 0;
	for(ctr=0; ctr<NLDCC; ctr++) {
             fprintf(outf, "0x%02x",ttyp->t_cc[ctr]);
	     ++tmpctr;
             if (tmpctr == 16) {
                tmpctr = 0;
                fprintf(outf,"\n");
             }
             else
                fprintf(outf," ");
        }

     	fprintf(outf,"\n\n");
  	fprintf(outf, "utility  = 0x%08x ", ttyp->utility);
	fprintf(outf, "t_tdstate = 0x%08x ", ttyp->t_tdstate);
	fprintf(outf, "t_pidprc = 0x%x\n", ttyp->t_pidprc);
	fprintf(outf, "cb_flags = 0x%x        ", ttyp->t_blmode.user.cb_flags);
	fprintf(outf, "cb_trig1c= 0x%02x        ",ttyp->t_blmode.user.cb_trig1c);
	fprintf(outf, "cb_trig2c = 0x%02x\n",ttyp->t_blmode.user.cb_trig2c);
	fprintf(outf, "cb_alertc= 0x%02x       ",ttyp->t_blmode.user.cb_alertc);
	fprintf(outf, "cb_termc = 0x%02x        ",ttyp->t_blmode.user.cb_termc);
	fprintf(outf, "cb_replen = 0x%x\n",ttyp->t_blmode.user.cb_replen);
        fprintf(outf,"\n");
	tmpctr = 0;
  	fprintf(outf, "cb_reply :\n");
	for(ctr=0; ctr<NBREPLY; ctr++) {
		fprintf(outf, "0x%02x",ttyp->t_blmode.user.cb_reply[ctr]);
                ++tmpctr;
                if (tmpctr == 16) {
                   tmpctr = 0;
                   fprintf(outf,"\n");
	        }
                else
                   fprintf(outf," ");
	}

	fprintf(outf,"\n");
	fprintf(outf, "trigger_checked = %d    ",ttyp->t_blmode.trigger_checked);
	fprintf(outf, "second_trigger  = %d    ",ttyp->t_blmode.second_trigger);
	fprintf(outf, "timing_xfer     = %d\n",ttyp->t_blmode.timing_xfer);
	fprintf(outf, "receiving_xfer  = %d    ",ttyp->t_blmode.receiving_xfer);
	fprintf(outf, "xfer_received   = %d    ",ttyp->t_blmode.xfer_received);
	fprintf(outf, "discarding_xfer = %d\n",ttyp->t_blmode.discarding_xfer);
	fprintf(outf, "xfer_error      = %d    ",ttyp->t_blmode.xfer_error);

	fprintf(outf, "t_readlen       = 0x%02x ", ttyp->t_readlen);
	fprintf(outf, "t_nuio          = 0x%08x\n", ttyp->nuio);
	fprintf(outf, "t_tflag         = 0x%02x\n\n ", ttyp->tflag);
}

ptydump_pt_tty(pt, vaddr)
register struct tty *pt;
uint vaddr;
{
	fprintf(outf, "pt_tty at 0x%08x\n\n", vaddr);
	pty_dumptty(pt, vaddr);
}

ptydump_mux0_ttys(mux0_tty, vaddr)
struct tty *mux0_tty;
uint vaddr;
{
        fprintf(outf, "mux0_tty at 0x%08x\n\n", vaddr);
	pty_dumptty(mux0_tty, vaddr);
}



ptydump(type, vaddr)
int type;
uint vaddr;
{
    switch (type) {
        case TPTYINFO:
	    ptydump_pty_info 
		(&pty_info[(struct pty_info *)vaddr - vpty_info], vaddr);
	    break;
	case TPTTY:
	    ptydump_pt_tty
		(&pt_tty[(struct tty *)vaddr - vpt_tty], vaddr);
	    break;
	case TTTY:
	    ptydump_mux0_ttys
		(&mux0_ttys[(struct tty *)vaddr - vmux0_ttys], vaddr);
	    break;
    }
}



/* Front-end for all pty/tty displays.  Called by the parser */
pty_display(type, param1, opt, redir, path)
int type, param1, opt, redir;
char *path;
{
    int nextv;
    register uint vaddr;

    NESAV;
    NECLR;
    
    NDBG(("pty_display(%#x,%#x,%#x,%#x,%#x)",type,param1,opt,redir,path));

    if (redir) {
        if ((outf = fopen(path,((redir == 2)?"a+":"w+"))) == NULL) {
            fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
            goto out;
        }
    }

    switch (opt) {

    case 'n':
        /* Get vaddr from index */
	vaddr = pty_ntov(type, param1);
        if (vaddr == -1) {
	    NERROR("index out of range", 0);
            goto reset;
	}
        break;
    case '\0' :
        /* Check validity of address */
        vaddr = param1;
	NESETVA(vaddr);
        if (!pty_validv(type, vaddr)) {
            NERROR("invalid address", 0);
            goto reset;
        }
        break;
    case 'a' :
        /* Display all, get first address */
	nextv = 0;
        vaddr = pty_ntov(type, nextv++); /* get first address */
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
    ptydump(type, vaddr);

    if (opt == 'a') {
        /* display all structures of type type */
	while ((vaddr = pty_ntov(type, nextv++)) != -1) {
            if (got_sigint) break;    /* break out of loop on sigint */
   	    NESETVA(vaddr);
	    ptydump(type, vaddr);
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
