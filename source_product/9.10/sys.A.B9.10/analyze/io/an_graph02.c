/***************************************************************************
 **                                                                       **
 **  This file contains the I/O manager analysis routines that are        **
 **  common to both graph0 and graph2.					  **
 **                                                                       **
 ***************************************************************************/


#include <stdio.h>
#include <h/types.h>
#include <sio/llio.h>
#include <sio/graph02.h>		/* graph0's normal .h file	     */
#include <sio/graph2.h>			/* for GRAPH_SEMA_*		     */
#include <sio/gr_data.h>		/* the framebuffer's normal .h file  */
#include <h/clist.h>			/* the hil structures use clists     */
#include <sio/hil.h>			/* the hil's normal .h file	     */
#include "aio.h"		        /* all analyze i/o defs              */


#define AN_GRAPH0    0			/* Used for recognizing the driver   */
#define AN_GRAPH2    2			/*   currently being analyzed	     */
#define streq(a, b)  (strcmp((a), (b)) == 0)




an_graph02_check_revs (me, rev_var_name, rev_var_value, out_fd)
    char  *me;
    char  *rev_var_name;
    int   rev_var_value;
    FILE  *out_fd;
/*
 *  This routine checks driver data stucture revision against analyze 
 *  routines revision and complains if there is a problem.
 */

{
    unsigned  addr;		/* address of extern                 */
    int	      code_rev;		/* revision of driver code	     */

    if (an_grab_extern (rev_var_name, &addr) != 0) 
	aio_rev_mismatch(me, AIO_NO_REV_INFO, code_rev, rev_var_value, out_fd);
    else {
	if (an_grab_real_chunk(addr, (char *)&code_rev, sizeof(code_rev)) != 0)
	    aio_rev_mismatch (me, AIO_NO_REV_INFO, code_rev, 
			      rev_var_value, out_fd);
	else {
	    if (code_rev != rev_var_value)
		aio_rev_mismatch (me, AIO_INCOMPAT_REVS, code_rev,
				  rev_var_value, out_fd);
	}
    }
}



an_graph02_optional (pda, which_graph, option_string, out_fd)
    graph_pda_type	  *pda;
    int			  which_graph;
    char		  *option_string;
    FILE		  *out_fd;
/*
 *  This routine parses through the driver's optional command string
 *  setting flags corresponding to the chosen options.  The flags are then
 *  used to call the routines necessary to carry out the chosen commands.
 */

{
    char *token;
    int	 pda_flag = 0;
    int	 grp_flag = 0;
    int  gcd_flag = 0;
    int  hil_flag = 0;
    int	 all_flag = 0;

    /* read through option string and set appropriate flags */
    if ((token = strtok(option_string, " ")) == NULL)  {
	an_graph02_help(stderr);
	return;
    }

    do {
	if 	(streq(token,    "pda"))    pda_flag   = 1;
	else if (streq(token,    "grp"))    grp_flag   = 1;
	else if (streq(token,    "gcdesc")) gcd_flag   = 1;
	else if (streq(token,    "hil"))    hil_flag   = 1;
	else if (streq(token,    "all"))    all_flag   = 1;
	else  {
	    fprintf(stderr, "invalid option <%s>\n", token);
	    an_graph02_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (pda_flag) an_graph02_pda_info(pda, which_graph, out_fd);
    if (grp_flag) an_graph02_grp_info(pda, which_graph, out_fd);
    if (gcd_flag) an_graph02_gcd_info(pda, which_graph, out_fd);
    if (hil_flag) an_graph02_hil_info(pda, which_graph, out_fd);
    if (all_flag) an_graph02_all(pda, which_graph, out_fd);

}


an_graph02_help (out_fd)
    FILE  *out_fd;
/*
 *  This is called to print out the help message to the user.  It simply
 *  describes the available options.
 */

{
    fprintf(out_fd,"\nvalid options:\n");
    fprintf(out_fd,"   pda      --  print pda information\n");
    fprintf(out_fd,"   grp      --  print the graphics display information\n");
    fprintf(out_fd,"   hil      --  print the HIL loop information\n");
    fprintf(out_fd,"   gcdesc   --  print the GCDESCRIBE information\n");
    fprintf(out_fd,"\n");
    fprintf(out_fd,"   all      --  do all of the above\n");
    fprintf(out_fd,"   help     --  print this help screen\n");
}




an_graph02_all (pda, which_graph, out_fd)
    graph_pda_type  *pda;
    int		    which_graph;
    FILE	    *out_fd;
/*
 *  This routine calls all the routines necessary to print out all the 
 *  graph0, framebuffer, and hil data.  It doesn't bother
 *  trying to print out the GCDESC or HIL option if the gr_data information
 *  is not available, because the gr_data struct is accessed for these 
 *  two options
 */

{
    an_graph02_pda_info (pda, which_graph, out_fd);
    if (an_graph02_grp_info (pda, which_graph, out_fd) == 0) {
        an_graph02_gcd_info (pda, which_graph, out_fd);
        an_graph02_hil_info (pda, which_graph, out_fd);
    }
}



an_graph02_pda_info (pda, which_graph, out_fd)
    graph_pda_type	*pda;
    int			which_graph;
    FILE		*out_fd;
/*
 *  Carries out the PDA command option.
 */

{
    fprintf(out_fd, "\n\n========  Port Data Area information  ======== \n\n");
    an_graph02_dump_pda_struct (pda, which_graph, out_fd);
}




an_graph02_grp_info (pda, which_graph, out_fd)
    graph_pda_type  *pda;
    int		    which_graph;
    FILE	    *out_fd;    
/*
 *  Carries out the GRP command option.  It first tries to get the gr_data
 *  information.  If successful, the info is printed for one or both 
 *  displays, depending on the driver (graph0 or graph2).
 */

{
    struct gr_data  grp_a;
    struct gr_data  grp_b;

    if (an_graph02_get_framebuf_data (pda, &grp_a, &grp_b, which_graph) != 0) {
	fprintf (stderr, "Could not get framebuffer data. Cannot continue.\n");
	return(-1);
    } else {
	fprintf(out_fd, "\n\n===  Per Graphics Display Information  === \n\n");
	if (which_graph == AN_GRAPH2) {
	    fprintf(out_fd, ">>>  for first display  >>>\n\n");
	}
	an_graph02_dump_grp_struct (&grp_a, which_graph, out_fd);
	if ((which_graph == AN_GRAPH2) && (pda->grp_b != NULL)) {
	    fprintf(out_fd, "\n\n>>>  for second display  >>>\n\n");
	    an_graph02_dump_grp_struct (&grp_b, which_graph, out_fd);
	}
    }
    return(0);
}




an_graph02_gcd_info (pda, which_graph, out_fd)
    graph_pda_type	*pda;
    int			which_graph;
    FILE		*out_fd;    
/*
 *  Carries out the GCDESC command option.  It first tries to get the gr_data
 *  information.  If successful, the info is printed for one or both 
 *  displays, depending on the driver (graph0 or graph2).
 */

{
    struct gr_data  grp_a;
    struct gr_data  grp_b;

    if (an_graph02_get_framebuf_data (pda, &grp_a, &grp_b, which_graph) != 0) {
	fprintf (stderr, "Could not get framebuffer data. Cannot continue.\n");
    } else {
	fprintf(out_fd, "\n\n========  GCDESCRIBE information  ======== \n\n");
	if (which_graph == AN_GRAPH2) {
	    fprintf(out_fd, ">>>  for first display  >>>\n\n");
	}
	an_graph02_dump_gcd_struct (&grp_a.desc, out_fd);
	if ((which_graph == AN_GRAPH2) && (pda->grp_b != NULL)) {
	    fprintf(out_fd, "\n\n>>>  for second display  >>>\n\n");
	    an_graph02_dump_gcd_struct (&grp_b.desc, out_fd);
	}
    }
}


an_graph02_hil_info (pda, which_graph, out_fd)
    graph_pda_type	*pda;
    int			which_graph;
    FILE		*out_fd;    
/*
 *  Carries out the HIL command option.  Since analyze is basically port (pda)
 *  oriented, only the hil loops associated with the given pda are analyzed.
 *  The lu information is extracted via the pda and grp structures.  
 *  If this series of data retrievals is successful, the proper hil loop 
 *  data (given the lu) is then grabbed from the core.  The information is
 *  then printed for one or both HIL loops, depending on the driver
 *  (graph0 or graph2).
 */

{
    unsigned	    addr;
    struct gr_data  grp_a, grp_b;
    struct hilrec   *hilp, hil_a, hil_b;

    if (an_grab_extern ("hil_loop_data", &addr) != 0) {
	fprintf(stderr,"Couldn't find hil_loop_data. Cannot continue.\n");
	return(-1);		
    } 
    if (an_graph02_get_framebuf_data (pda, &grp_a, &grp_b, which_graph) != 0) {
	fprintf (stderr, 
		 "Could not get lu info from gr_data. Cannot continue.\n");
	return(-1);
    }
    hilp = (struct hilrec *)(addr) + grp_a.lu;
    if (an_grab_real_chunk((unsigned)hilp, 
			   (char *)&hil_a, sizeof(struct hilrec)) != 0) {
	fprintf(stderr,"Couldn't get HIL data for lu = %d\n", grp_a.lu);
	return(-1);
    }
    fprintf(out_fd, "\n\n============  HIL information  ============= \n\n");
    if (which_graph == AN_GRAPH2) {
	fprintf(out_fd, ">>>  for first hil loop  >>>\n\n");
    }
    an_graph02_dump_hil_struct (&hil_a, out_fd);
    if ((which_graph == AN_GRAPH2) && (pda->grp_b != NULL)) {
	hilp = (struct hilrec *)(addr) + grp_b.lu;
	if (an_grab_real_chunk((unsigned)hilp, 
			       (char *)&hil_b, sizeof(struct hilrec)) != 0) {
	    fprintf(stderr,"Couldn't get HIL data for lu = %d\n", grp_b.lu);
	    return(-1);
	}
	fprintf(out_fd, "\n\n>>>  for second hil loop  >>>\n\n");
	an_graph02_dump_hil_struct (&hil_b, out_fd);
    }
}




an_graph02_get_framebuf_data (pda, grp_02, grp_2, which_graph)
    graph_pda_type  *pda;
    struct gr_data  *grp_02;
    struct gr_data  *grp_2;
    int		    which_graph;
/*
 *  This routines attempts to grab the gr_data structures out of the core
 *  file.  It complains if grp_a in the pda is NULL, or (when graph2 is being
 *  analyzed) if grp_b is NULL, or (when graph0 is being analyzed) if
 *  grp_b is not NULL.  The function fails if any attempt at getting
 *  the data fails.
 */

{
    if (pda->grp_a == NULL) {
        fprintf(stderr,"Problem: framebuffer data pointer grp_a is NULL\n");
    } else {
	if (an_grab_real_chunk((unsigned)pda->grp_a, 
			       (char *)grp_02, sizeof(struct gr_data)) != 0) {
	    fprintf(stderr,"Couldn't get framebuffer info, grp_a\n");
	    return(-1);
	}
    }
    if (which_graph == AN_GRAPH2) {
	if (pda->grp_b == NULL) {
	    fprintf(stderr, "Warning: framebuf data pointer grp_b is NULL\n");
	} else {
	    if (an_grab_real_chunk((unsigned)pda->grp_b, 
				   (char *)grp_2, sizeof(struct gr_data))!=0) {
		fprintf(stderr,"Couldn't get framebuffer info, grp_b\n");
		return(-1);
	    }
	}
    } else {
	if (pda->grp_b != NULL) {
	    fprintf(stderr, "Problem: graph0 installed, grp_b is not NULL\n");
	}
    }
    return(0);
}
    


an_graph02_dump_pda_struct (pda, which_graph, out_fd)
    graph_pda_type  *pda;
    int		    which_graph;
    FILE	    *out_fd;
/*
 *  This routine take a PDA structure and dumps it out in a pretty form.
 *  It only prints the portion of the PDA that corresponds with the 
 *  particular driver being analyzed.
 */

{
    fprintf(out_fd, "grp_a          = 0x%-16X", pda->grp_a);
    if (which_graph == AN_GRAPH0) {
	fprintf(out_fd, "- Address of display's gr_data struct\n");
    } else {
	fprintf(out_fd, "- Address of display A's gr_data struct\n");
	fprintf(out_fd, "grp_b          = 0x%-16X", pda->grp_b);
	fprintf(out_fd, "- Address of display B's gr_data struct\n");
    }
    fprintf(out_fd, "my_port        = 0x%-16X", pda->my_port);
    fprintf(out_fd, "- ldm port number\n");
    fprintf(out_fd, "portserver     = ");
    switch (pda->portserver) {
        case SUBSYS_GRAPH0:
	    fprintf(out_fd, "%-18s", "SUBSYS_GRAPH0"); break;
	case SUBSYS_GRAPH2:
	    fprintf(out_fd, "%-18s", "SUBSYS_GRAPH2"); break;
	default:		
	    fprintf(out_fd, "%-18s", "Unknown value"); break;
    }
    fprintf(out_fd, "- which portserver\n");
    fprintf(out_fd, "lu             = 0x%-16d", pda->lu);
    fprintf(out_fd, "- logical unit number\n");
    fprintf(out_fd, "eim            = 0x%-16X", pda->eim);
    fprintf(out_fd, "- address of the interrupt (EIR) register\n");
    fprintf(out_fd, "g_state        = ");
    switch (pda->g_state) {
	case GST_UNINITIALIZED:	
	    fprintf(out_fd, "%-18s", "GST_UNINITIALIZED"); break;
	case GST_NORMAL:	
	    fprintf(out_fd, "%-18s", "GST_NORMAL");        break;
	case GST_PFAIL:		
	    fprintf(out_fd, "%-18s", "GST_PFAIL");         break;
	default:		
	    fprintf(out_fd, "%-18s", "Unknown value");     break;
    }
    fprintf(out_fd, "- configuration state\n");
    fprintf(out_fd, "left_to_bind   = %-18d", pda->left_to_bind);
    fprintf(out_fd, "- num display's on this card still unbound\n");    
    fprintf(out_fd, "hpa_fb         = 0x%-16X", pda->hpa_fb);
    fprintf(out_fd, "- base of HPA space (Module 0)\n");
    fprintf(out_fd, "hpa_cs         = 0x%-16X", pda->hpa_cs);
    fprintf(out_fd, "- base of HPA space (Module 1)\n");
    fprintf(out_fd, "spa_fb         = 0x%-16X", pda->spa_fb);
    fprintf(out_fd, "- base of SPA space (Module 0)\n");
    fprintf(out_fd, "spa_cs         = 0x%-16X", pda->spa_cs);
    fprintf(out_fd, "- base of SPA space (Module 1)\n");
    fprintf(out_fd, "iodc_spa       = 0x%-16X", pda->iodc_spa);
    fprintf(out_fd, "- base of IO dependent code SPA\n");
    if (which_graph == AN_GRAPH2) {
	fprintf(out_fd, "card_sema      = ");
	if (pda->card_sema) {
	    fprintf(out_fd, "\n");	
	    if (pda->card_sema & GRAPH_SEMA_ITE_A_WANTED) {
	        fprintf(out_fd, "      GRAPH_SEMA_ITE_A_WANTED |\n");
		fprintf(out_fd, " - ITE is wanted on display A\n");
	    }
	    if (pda->card_sema & GRAPH_SEMA_ITE_B_WANTED) {
	        fprintf(out_fd, "      GRAPH_SEMA_ITE_B_WANTED |\n");
		fprintf(out_fd, " - ITE is wanted on display B\n");
	    }
	    if (pda->card_sema & GRAPH_SEMA_ITE_A_BUSY) {
	        fprintf(out_fd, "      GRAPH_SEMA_ITE_A_BUSY |\n");
		fprintf(out_fd, " - ITE is busy on display A\n");
	    }
	    if (pda->card_sema & GRAPH_SEMA_ITE_B_BUSY) {
	        fprintf(out_fd, "      GRAPH_SEMA_ITE_B_BUSY |\n");
		fprintf(out_fd, " - ITE is busy on display B\n");
	    }
	    if (pda->card_sema & GRAPH_SEMA_RESETTING) {
	        fprintf(out_fd, "      GRAPH_SEMA_ITE_RESETTING |\n");
		fprintf(out_fd, " - ITE is resetting\n");
	    }
	    if (pda->card_sema & GRAPH_SEMA_DMA) {
	        fprintf(out_fd, "      GRAPH_SEMA_DMA\n");
		fprintf(out_fd, " - DMA is active on card\n");
	    }
	} else {
            fprintf(out_fd, "%-18d", 0);
	    fprintf(out_fd, "- if non-zero, contains semaphore bits\n");
	}
	fprintf(out_fd, "dma_owner --\n");
	fprintf(out_fd, "      owner_a      = %-14d", pda->dma_owner.owner_a);
	fprintf(out_fd, "- if non-zero, display A is doing the DMA\n");
	fprintf(out_fd, "      owner_b      = %-14d", pda->dma_owner.owner_b);
	fprintf(out_fd, "- if non-zero, display B is doing the DMA\n");
	fprintf(out_fd, "      owner_killed = %-14d",
		pda->dma_owner.owner_killed);
	fprintf(out_fd, "- if non-zero, DMA interrupted by signal\n\n");
	fprintf(out_fd, "dma_error      = %-18d", pda->dma_error);
	fprintf(out_fd, "- if non-zero, a problem occurred during DMA\n");
	fprintf(out_fd, "dma_proc       = 0x%-16X", pda->dma_proc);
	fprintf(out_fd, "- pointer to process doing the DMA\n");
    }
    fprintf(out_fd, "diag_proc      = 0x%-16X", pda->diag_proc);
    fprintf(out_fd, "- pointer to process doing the diagnostics\n");
    fprintf(out_fd, "hil_state --\n");
    fprintf(out_fd, "      G2_HIL_A_RESETTING = %-8d",
	    pda->hil_state & G2_HIL_A_RESETTING?1:0);
    fprintf(out_fd, "- if non-zero, display A's HIL is resetting\n");
    fprintf(out_fd, "      G2_HIL_B_RESETTING = %-8d",
	    pda->hil_state & G2_HIL_B_RESETTING?1:0);
    fprintf(out_fd, "- if non-zero, display B's HIL is resetting\n\n");
    fprintf(out_fd, "max_prewait    = %-18d", pda->max_prewait);
    fprintf(out_fd, "- max prewait for a DMA cmd (ticks, scaled)\n");
    fprintf(out_fd, "max_postwait   = %-18d", pda->max_postwait);
    fprintf(out_fd, "- max postwait for a DMA cmd (ticks, scaled)\n");
    fprintf(out_fd, "max_postwait_reset  = %-13d", pda->max_postwait_reset);
    fprintf(out_fd, "- max postwait after RESET (ticks, scaled)\n");
    fprintf(out_fd, "max_hilreset   = %-18d", pda->max_hilreset);
    fprintf(out_fd, "- max wait for 8042 to reset (ticks, scaled)\n");
    fprintf(out_fd, "\n\n");
}


	

an_graph02_dump_grp_struct (grp, which_graph, out_fd)
    struct gr_data  *grp;
    int	       	    which_graph;
    FILE	    *out_fd;
/*
 *  This routine take a gr_data (per display data) structure and dumps 
 *  it out in a pretty form.  It only prints the portion of the data 
 *  that corresponds with the particular driver being analyzed.
 */

{
    int i;

    fprintf(out_fd, "pdap               = 0x%-16X", grp->pdap);
    fprintf(out_fd, "- pointer to port server's data area\n");
    fprintf(out_fd, "frame_buffer       = 0x%-16X", grp->frame_buffer);
    fprintf(out_fd, "- pointer to beginning of framebuffer\n");
    fprintf(out_fd, "ctl_space          = 0x%-16X", grp->ctl_space);
    fprintf(out_fd, "- pointer to beginning of control space\n");
    if (which_graph == AN_GRAPH2) {
	fprintf(out_fd, "interlocked_fb     = 0x%-16X", grp->interlocked_fb);
	fprintf(out_fd, "- pointer to other display's framebuffer\n");
	fprintf(out_fd, "interlocked_cs     = 0x%-16X", grp->interlocked_cs);
	fprintf(out_fd, "- pointer to other display's ctrl space\n");
    }
    fprintf(out_fd, "size_frame_buffer  = 0x%-16X", grp->size_frame_buffer);
    fprintf(out_fd, "- size of framebuffer in bytes\n");
    fprintf(out_fd, "size_ctl_space     = 0x%-16X", grp->size_ctl_space);
    fprintf(out_fd, "- size of control space in bytes\n");
    fprintf(out_fd, "hpa                = 0x%-16X", grp->hpa);
    fprintf(out_fd, "- pointer to devices HPA space\n");
    fprintf(out_fd, "lu                 = %-18d",   grp->lu);
    fprintf(out_fd, "- device's logical unit number\n");
    if (which_graph == AN_GRAPH2) {
	fprintf(out_fd, "which_display      = %-18s",
		(grp->which_display == G2_DISPLAY_A) ? "G2_DISPLAY_A" :
		"G2_DISPLAY_B");
	fprintf(out_fd, "- is this info for display A or B ?\n");
    }
    fprintf(out_fd, "state              = ");
    switch (grp->state) {
	case GPU_UNINITIALIZED:	
	    fprintf(out_fd, "%-18s", "GPU_UNINITIALIZED");	break;
	case GPU_NORMAL:	
	    fprintf(out_fd, "%-18s", "GPU_NORMAL");		break;
	case GPU_POWER_DOWN:    
	    fprintf(out_fd, "%-18s", "GPU_POWER_DOWN");		break;
	case GPU_DEAD:		
	    fprintf(out_fd, "%-18s", "GPU_DEAD");		break;
	default:		
	    fprintf(out_fd, "%-18s", "Unknown value");		break;
    }
    fprintf(out_fd, "- current state of display\n");
    fprintf(out_fd, "fe_bank            = 0x%-16X", grp->fe_bank);
    fprintf(out_fd, "- 98550 bank address\n");
    fprintf(out_fd, "locking_proc       = 0x%-16X", grp->locking_proc);
    fprintf(out_fd, "- process that owns graphics lock\n");
    fprintf(out_fd, "g_locking_blocksig_set = 0x%-16X", grp->g_locking_blocksig_set);
    fprintf(out_fd, "- is it locked via GCLOCK_BLOCKSIG\n");
    fprintf(out_fd, "g_locking_caught_sigs  = 0x%-16X", grp->g_locking_caught_sigs);
    fprintf(out_fd, "- signals to block during lock\n");
    fprintf(out_fd, "static_colormap    = 0x%-16X", grp->static_colormap);
    fprintf(out_fd, "- proc last changing colormap to static\n");
    fprintf(out_fd, "sel_proc           = 0x%-16X", grp->sel_proc);
    fprintf(out_fd, "- proc that failed a recent select()\n");
    fprintf(out_fd, "sel_flag           = 0x%-16X", grp->sel_flag);
    fprintf(out_fd, "- non-zero if >1 process failed select()\n");
    fprintf(out_fd, "sel_qnum           = 0x%-16X", grp->sel_qnum);
    fprintf(out_fd, "- no. of pending exceptional conditions\n");
    fprintf(out_fd, "gpu_reset()        = 0x%-16X", grp->gpu_reset);
    fprintf(out_fd, "- pointer to card GCRESET function\n");
    fprintf(out_fd, "graph_service()    = 0x%-16X", grp->graph_service);
    fprintf(out_fd, "- pointer to port server service routine\n");
    fprintf(out_fd, "graph_exit_hook()  = 0x%-16X", grp->graph_exit_hook);
    fprintf(out_fd, "- pointer to port server exit routine\n");
    fprintf(out_fd, "dma_restrictions() = 0x%-16X", grp->dma_restrictions);
    fprintf(out_fd, "- non-zero--points to dma restr. routine\n");
    if (which_graph == AN_GRAPH2) {
	fprintf(out_fd, "dma_func()         = 0x%-16X", grp->dma_func);
	fprintf(out_fd, "- pointer to DMA execution function\n");
	fprintf(out_fd, "quad_head          = 0x%-16X", grp->quad_head);
	fprintf(out_fd, "- points to head of DMA quad chain\n");
    }
    fprintf(out_fd, "diag_func()        = 0x%-16X", grp->diag_func);
    fprintf(out_fd, "- pointer to port server diag. routine\n");
    fprintf(out_fd, "curr_max_slot      = 0x%-16X", grp->curr_max_slot);
    fprintf(out_fd, "- no. fastlock slots currently allocated\n");
    fprintf(out_fd, "%-39s", "devs[MAX_FRAMEDEVS] --");
    fprintf(out_fd, "- array of 'devs' that have device open\n");
    for (i=0; i<MAX_FRAMEDEVS; i++)
	fprintf(out_fd, "              [%d]  = 0x%X\n", i, grp->devs[i]);
    fprintf(out_fd, "\n");
    fprintf(out_fd, "desc               = %-18s", "('gcdesc' option)");
    fprintf(out_fd, "- describes the graphics device\n");
    if (which_graph == AN_GRAPH2) {
	fprintf(out_fd, "%-39s", "dma_params --");
	fprintf(out_fd, "- copy of user's DMA params\n");
	fprintf(out_fd, "      mem_addr     = 0x%-16x", 
		grp->dma_params.mem_addr);
	fprintf(out_fd, "- DMA from this address\n");
	fprintf(out_fd, "      fb_addr      = 0x%-16x", 
		grp->dma_params.fb_addr);
	fprintf(out_fd, "- DMA to this address\n");
	fprintf(out_fd, "      length       = %-18d", grp->dma_params.length);
	fprintf(out_fd, "- length of DMA transfer\n");
	fprintf(out_fd, "      width        = %-18d", grp->dma_params.width);
	fprintf(out_fd, "- actual DMA transfer width\n");
	fprintf(out_fd, "      skipcount    = %-18d", 
		grp->dma_params.skipcount);
	fprintf(out_fd, "- unneeded DMA transfer width\n");
	fprintf(out_fd, "      flags        = 0x%-16X", grp->dma_params.flags);
	fprintf(out_fd, "- DMA attribute flags\n\n");
    }
    fprintf(out_fd, "%-39s", "slot_pid[curr_max_slot] --");
    fprintf(out_fd, "- fastlock slots defined, if any\n");
    if (grp->curr_max_slot <= 0) {
	fprintf(out_fd, "     -- empty");
    }	
    for (i=0; i < grp->curr_max_slot; i++)
        fprintf(out_fd, "          [%d]  = 0x%X\n", i, grp->slot_pid[i]);
    fprintf(out_fd, "\n");
    fprintf(out_fd, "shmid              = %-18d", grp->shmid);
    fprintf(out_fd, "- id for fastlock shared memory page\n");
    fprintf(out_fd, "lockp              = 0x%-16X", grp->lockp);
    fprintf(out_fd, "- pointer to fastlock info structure\n\n");

/*  This will have to wait for furthur an_grab_virt calls  */
/*  fprintf(out_fd, "    locking_slot   = 0x%-16X", 
	    grp->lockp->locking_slot);
    fprintf(out_fd, "- currently locked fastlock slot\n");
    fprintf(out_fd, "%-39s", "    slot_array[curr_max_slot] --");
    fprintf(out_fd, "- fastlock slots defined, if any\n\n");
    for (i=0; i < grp->curr_max_slot; i++)
        fprintf(out_fd, "             [%d]  = 0x%X\n", i, 
		grp->lockp->slot_array[i]); */
}



an_graph02_dump_gcd_struct (desc, out_fd)
    crt_frame_buffer_t *desc;
    FILE	       *out_fd;
/*
 *  This routine take a crt_frame_buffer_t_data (graphics data) structure 
 *  and dumps it out in a pretty form.  
 */

{
    fprintf(out_fd, "crt_id             = ");
    switch (desc->crt_id) {
        case S9000_ID_98720: fprintf(out_fd, "%-18s", "S9000_ID_98720"); break;
        case S9000_ID_98730: fprintf(out_fd, "%-18s", "S9000_ID_98730"); break;
	case S9000_ID_98550: fprintf(out_fd, "%-18s", "S9000_ID_98550"); break;
	default            : fprintf(out_fd, "%-18s", "Unknown value");  break;

    }
    fprintf(out_fd, "- display identifier\n");
    fprintf(out_fd, "crt_map_size       = 0x%-16X", desc->crt_map_size);
    fprintf(out_fd, "- size in bytes of frame buffer area\n");
    fprintf(out_fd, "crt_x              = %-18d", desc->crt_x);
    fprintf(out_fd, "- width in pixels (displayed part)\n");
    fprintf(out_fd, "crt_y              = %-18d", desc->crt_y);
    fprintf(out_fd, "- length in pixels (displayed part)\n");
    fprintf(out_fd, "crt_total_x        = %-18d", desc->crt_total_x);
    fprintf(out_fd, "- width in pixels: total area\n");
    fprintf(out_fd, "crt_total_y        = %-18d", desc->crt_total_y);
    fprintf(out_fd, "- length in pixels: total area\n");
    fprintf(out_fd, "crt_x_pitch        = %-18d", desc->crt_x_pitch);
    fprintf(out_fd, "- length of one row in bytes\n");
    fprintf(out_fd, "crt_bits_per_pixel = %-18d", desc->crt_bits_per_pixel);
    fprintf(out_fd, "- no. of frame buffer bits/screen pixel\n");
    fprintf(out_fd, "crt_bits_used      = %-18d", desc->crt_bits_used);
    fprintf(out_fd, "- number of those bits that are used\n");
    fprintf(out_fd, "crt_planes         = %-18d", desc->crt_planes);
    fprintf(out_fd, "- number of frame buffer planes\n");
    fprintf(out_fd, "crt_plane_size     = 0x%-16X", desc->crt_plane_size);
    fprintf(out_fd, "- size in bytes of a frame buffer plane\n");
    fprintf(out_fd, "crt_name           = %-18s", desc->crt_name);
    fprintf(out_fd, "- null terminated product name\n");
    fprintf(out_fd, "crt_attributes --\n");
    fprintf(out_fd, "      CRT_Y_EQUALS_2X         = %-7d",
	    desc->crt_attributes & CRT_Y_EQUALS_2X?1:0);
    fprintf(out_fd, "- pixel height is 2x width\n");
    fprintf(out_fd, "      CRT_BLOCK_MOVER_PRESENT = %-7d",
	    desc->crt_attributes & CRT_BLOCK_MOVER_PRESENT?1:0);
    fprintf(out_fd, "- hardware Block Mover exists\n");
    fprintf(out_fd, "      CRT_ADVANCED_HARDWARE   = %-7d",
	    desc->crt_attributes & CRT_ADVANCED_HARDWARE?1:0);
    fprintf(out_fd, "- hardware Transform Engine exists\n");
    fprintf(out_fd, "      CRT_CAN_INTERRUPT       = %-7d",
	    desc->crt_attributes & CRT_CAN_INTERRUPT?1:0);
    fprintf(out_fd, "- device can generate interrupts\n");
    fprintf(out_fd, "      CRT_GRAPHICS_ON_OFF     = %-7d",
	    desc->crt_attributes & CRT_GRAPHICS_ON_OFF?1:0);
    fprintf(out_fd, "- device has GCON, GCOFF capability\n");
    fprintf(out_fd, "      CRT_ALPHA_ON_OFF        = %-7d",
	    desc->crt_attributes & CRT_ALPHA_ON_OFF?1:0);
    fprintf(out_fd, "- has GCAON, GCAOFF capability\n");
    fprintf(out_fd, "      CRT_VARIABLE_Y_SIZE     = %-7d",
	    desc->crt_attributes & CRT_VARIABLE_Y_SIZE?1:0);
    fprintf(out_fd, "- lines in the frame buffer is variable\n");
    fprintf(out_fd, "      CRT_ODD_BYTES_ONLY      = %-7d",
	    desc->crt_attributes & CRT_ODD_BYTES_ONLY?1:0);
    fprintf(out_fd, "- use only odd frame buffer bytes\n");
    fprintf(out_fd, "      CRT_NEEDS_FLUSHING      = %-7d",
	    desc->crt_attributes & CRT_NEEDS_FLUSHING?1:0);
    fprintf(out_fd, "- graphics hardware cache needs flushing\n");
    fprintf(out_fd, "      CRT_DMA_HARDWARE        = %-7d",
	    desc->crt_attributes & CRT_DMA_HARDWARE?1:0);
    fprintf(out_fd, "- \"display card\" can support h/w DMA\n\n");
    fprintf(out_fd, "crt_frame_base     = 0x%-16X", desc->crt_frame_base);
    fprintf(out_fd, "- 1st word in the frame buffer memory\n");
    fprintf(out_fd, "crt_control_base   = 0x%-16X", desc->crt_control_base);
    fprintf(out_fd, "- 1st word of the device ctrl. registers\n");
    fprintf(out_fd, "crt_region         = 0x%-16X", desc->crt_region);
    fprintf(out_fd, "- other regions assoc. with frame buffer\n\n\n");
}



an_graph02_dump_hil_struct (hil, out_fd)
    struct hilrec  *hil;
    FILE           *out_fd;
/*
 *  This routine take a hilrec (hil loop data) structure 
 *  and dumps it out in a pretty form.  
 */

{
    int i;

    fprintf(out_fd, "hilbase                = 0x%-12X", hil->hilbase);
    fprintf(out_fd, "- pointer to 8042 command/data regs\n");
    fprintf(out_fd, "unit                   = %-14d", hil->unit);
    fprintf(out_fd, "- HIL link logical unit number\n");
    fprintf(out_fd, "has_ite_kbd            = %-14d", hil->has_ite_kbd);
    fprintf(out_fd, "- ITE config'd, has cooked keyboard\n");
    fprintf(out_fd, "hilkbd_took_ite_kbd    = %-14d", 
	    hil->hilkbd_took_ite_kbd);
    fprintf(out_fd, "- ITE config'd, hilkbd has keyboard\n");
    fprintf(out_fd, "active_device          = %-14d", hil->active_device);
    fprintf(out_fd, "- device curr sending auto-poll packet\n");
    fprintf(out_fd, "command_device         = %-14d", hil->command_device);
    fprintf(out_fd, "- device being sent a link command\n");
    fprintf(out_fd, "command_ending         = %-14d", hil->command_ending);
    fprintf(out_fd, "- next data should be 'Command Returned'\n");
    fprintf(out_fd, "loopbusy               = %-14d", hil->loopbusy);
    fprintf(out_fd, "- the HIL loop semaphore \n");
    fprintf(out_fd, "command_in_progress    = %-14d", 
	    hil->command_in_progress);
    fprintf(out_fd, "- HIL link command in progress\n");
    fprintf(out_fd, "waiting_for_loop       = %-14d", hil->waiting_for_loop);
    fprintf(out_fd, "- someone waiting for semaphore\n");
    fprintf(out_fd, "%-39s", "packet[packet_length] --");
    fprintf(out_fd, "- buffer accumulating auto-poll input\n");
    if (hil->packet_length > 0) {
	for (i=0; i < hil->packet_length; i++) {
	    fprintf(out_fd, "    [%2d]  = 0x%-6X", i, hil->packet[i]);
	    if ((i+1)%3 == 0) 
	        fprintf(out_fd, "\n");
	}
	fprintf(out_fd, "\n");    
    } else {
	fprintf(out_fd, "     -- empty\n");
    }
    fprintf(out_fd, "packet_length          = %-14d", hil->packet_length);
    fprintf(out_fd, "- length of current auto-poll packet\n");
    fprintf(out_fd, "%-39s", "loop_response[response_length] --");
    fprintf(out_fd, "- buffer accumulating link command resp\n");
    if (hil->packet_length > 0) {
	for (i=0; i < hil->response_length; i++) {
	    fprintf(out_fd, "    [%2d]  = 0x%-6X", i, hil->loop_response[i]);
	    if ((i+1)%3 == 0) 
	        fprintf(out_fd, "\n");
	}
	fprintf(out_fd, "\n");    
    } else {
	fprintf(out_fd, "     -- empty\n");
    }
    fprintf(out_fd, "response_length        = %-14d", hil->response_length);
    fprintf(out_fd, "- length of current command response\n");
    fprintf(out_fd, "cooked_mask            = 0x%-12X", hil->cooked_mask);
    fprintf(out_fd, "- bitmask of devices that can \"cook\"\n");
    fprintf(out_fd, "current_mask           = 0x%-12X", hil->current_mask);
    fprintf(out_fd, "- bitmask of devices currently cooked\n");
    fprintf(out_fd, "num_ldevices           = %-14d", hil->num_ldevices);
    fprintf(out_fd, "- number of devices on HIL loop\n");
    fprintf(out_fd, "repeat_rate            = %-14d", 
	    (256-hil->repeat_rate)*10);
    fprintf(out_fd, "- keyboard repeat rate in milli-sec\n");
    fprintf(out_fd, "repeat_delay           = %-14d", 
	    (256-hil->repeat_delay)*10);
    fprintf(out_fd, "- keyboard repeat delay in milli-sec\n");
    fprintf(out_fd, "%-39s", "loopdevices[num_ldevices] --");
    fprintf(out_fd, "- array of per-device information\n");
    if (hil->num_ldevices == 0) {
	fprintf(out_fd, "     -- empty\n\n");
    }	
    for (i = 0; i < hil->num_ldevices; i++) {
	fprintf(out_fd, "\n   [%2d]  %-30s", i, "hdevqueue --");
	fprintf(out_fd, "- data input queue for device\n");
	fprintf(out_fd, "               c_cc        = %-10d", 
		hil->loopdevices[i].hdevqueue.c_cc);
	fprintf(out_fd, "- clist character count\n");
	fprintf(out_fd, "               c_cf        = 0x%-8X", 
		hil->loopdevices[i].hdevqueue.c_cf);
	fprintf(out_fd, "- pointer to first cblock\n");
	fprintf(out_fd, "               c_cl        = 0x%-8X", 
		hil->loopdevices[i].hdevqueue.c_cl);
	fprintf(out_fd, "- pointer to last cblock\n\n");
	fprintf(out_fd, "         %-30s", "hil_sel --");
	fprintf(out_fd, "- info needed to support select()\n");
	fprintf(out_fd, "               hil_selp    = 0x%-8X", 
		hil->loopdevices[i].hil_sel.hil_selp);
	fprintf(out_fd, "- proc that failed recent read select()\n");
	fprintf(out_fd, "               hil_selflag = 0x%-8X", 
		hil->loopdevices[i].hil_sel.hil_selflag);
	fprintf(out_fd, "- set if >1 process failed read select()\n\n");
	fprintf(out_fd, "         open_flag     = 0x%-12X", 
		hil->loopdevices[i].open_flag);
	fprintf(out_fd, "- set if a process has device open\n");
	fprintf(out_fd, "         sleep         = 0x%-12X", 
		hil->loopdevices[i].sleep);	
	fprintf(out_fd, "- a process is sleeping waiting for data\n\n");
    }
}
