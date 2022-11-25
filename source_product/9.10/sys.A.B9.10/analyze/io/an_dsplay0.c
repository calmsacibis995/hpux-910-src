/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/io/RCS/an_dsplay0.c,v $
 * $Revision: 1.2.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:25:13 $
 */

/*****************************************************************************/
/**                                                                         **/
/**  This is the I/O manager analysis routine for the display0 port server  **/
/**                                                                         **/
/*****************************************************************************/


#include <stdio.h>
#include <h/types.h> 
#include <sio/llio.h>
#include <sio/display0.h>		/* display0's normal .h file	     */
#include "aio.h"			/* all analyze i/o defs		     */

static  char *me              = "display0";
static  char *my_rev_var_name = "display0_an_rev";
int     display0_an_rev       = 1;      /* global -- rev number of display0  */
					/*	     data structures	     */
#define streq(a, b)  (strcmp((a), (b)) == 0)


int an_display0(call_type, port_num, out_fd, option_string)
    int		   call_type;		/* type of actions to perform	     */
    port_num_type  port_num;		/* manager port number		     */
    FILE	   *out_fd;    		/* file descriptor of output file    */
    char	   *option_string;	/* driver-spec options user typed    */

/*
 *  This routine is the interface routine for analyze to the display0 analysis 
 *  routines.  It first grabs the generic manager info, then the pda.  It     
 *  then performs the correct action based upon the value of call_type.  
 */

{
    display_pda_type	  pda;		/* port data area                    */
    struct mgr_info_type  mgr;		/* all generic manager info	     */

    if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, port_num, &mgr) != AIO_OK) {
    	fprintf(stderr,"Problem -- bad port number!\n");
        return(0);
    }
    if (an_grab_virt_chunk(0, mgr.pda_address, 
			   (char *)&pda, sizeof(pda)) != 0)  {
	fprintf(stderr,"Couldn't get pda\n");
	return(0);
    }

    switch (call_type)  {
	case AN_MGR_INIT:
	    an_graph02_check_revs (me, my_rev_var_name, 
				   display0_an_rev, out_fd);
	    break;
	case AN_MGR_BASIC:
	    an_display0_pda_info (&pda, out_fd);
	    break;
	case AN_MGR_DETAIL:
	    an_display0_pda_info (&pda, out_fd);
	    break;
	case AN_MGR_OPTIONAL:
	    an_display0_optional (&pda, option_string, out_fd);
	    break;
	case AN_MGR_HELP:
	    an_display0_help (out_fd);
	    break;
    }
    return(0);
}


an_display0_optional (pda, option_string, out_fd)
    display_pda_type	  *pda;
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
    int	 all_flag = 0;

    /* read through option string and set appropriate flags */
    if ((token = strtok(option_string, " ")) == NULL)  {
	an_display0_help(stderr);
	return;
    }

    do {
	if 	(streq(token,    "pda"))    pda_flag   = 1;
	else if (streq(token,    "all"))    all_flag   = 1;
	else  {
	    fprintf(stderr, "invalid option <%s>\n", token);
	    an_display0_help(stderr);
	    return;
	}
    } while ((token = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (pda_flag) an_display0_pda_info(pda, out_fd);
    if (all_flag) an_display0_all(pda, out_fd);

}


an_display0_help (out_fd)
    FILE  *out_fd;
/*
 *  This is called to print out the help message to the user.  It simply
 *  describes the available options.
 */

{
    fprintf(out_fd,"\nvalid options:\n");
    fprintf(out_fd,"   pda      --  print pda information\n");
    fprintf(out_fd,"\n");
    fprintf(out_fd,"   all      --  do all of the above\n");
    fprintf(out_fd,"   help     --  print this help screen\n");
}




an_display0_all (pda, out_fd)
    display_pda_type  *pda;
    FILE	       *out_fd;
/*
 *  This routine calls all the routines necessary to print out all the 
 *  display0 associated data.  Atthis point, only the pda option is supported.
 */

{
    an_display0_pda_info (pda, out_fd);
}



an_display0_pda_info (pda, out_fd)
    display_pda_type	*pda;
    FILE		*out_fd;
/*
 *  Carries out the PDA command option.
 */

{
    fprintf(out_fd, "\n\n========  Port Data Area information  ======== \n\n");
    an_display0_dump_pda_struct (pda, out_fd);
}


an_display0_dump_pda_struct (pda, out_fd)
    display_pda_type  *pda;
    FILE	    *out_fd;
/*
 *  This routine take a PDA structure and dumps it out in a pretty form.
 */

{
    fprintf(out_fd, "port_state     = %-18d", pda->port_state);
    fprintf(out_fd, "- current state of the port\n");
    fprintf(out_fd, "my_port        = %-18d", pda->my_port);
    fprintf(out_fd, "- my port number\n");
    fprintf(out_fd, "graph_port     = %-18d", pda->graph_port);
    fprintf(out_fd, "- port no. of graph0/2 parent\n");
    fprintf(out_fd, "which_grp      = %-18d", pda->which_grp);
    fprintf(out_fd, "- 0 mean grp_a, 1 means grp_b\n");
    fprintf(out_fd, "display_lu     = %-18d", pda->display_lu);
    fprintf(out_fd, "- logical unit number of this display\n");
    fprintf(out_fd, "trans_num      = %-18d", pda->trans_num);
    fprintf(out_fd, "- same for all display0 messages	\n");
    fprintf(out_fd, "msg_id         = %-18d", pda->msg_id);
    fprintf(out_fd, "- message id of next message sent\n");
}
