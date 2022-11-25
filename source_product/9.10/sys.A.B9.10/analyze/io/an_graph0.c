

/***************************************************************************/
/**                                                                       **/
/**  This is the I/O manager analysis routine for the graph0 port server  **/
/**                                                                       **/
/***************************************************************************/



#include <stdio.h>
#include <h/types.h> 
#include <sio/llio.h>
#include <sio/graph02.h>		/* graph0's normal .h file	     */
#include "aio.h"			/* all analyze i/o defs		     */

#define AN_WHICH_GRAPH          0
static  char *me              = "graph0";
static  char *my_rev_var_name = "graph0_an_rev";
int     graph0_an_rev         = 4;      /* global -- rev number of graph0 DS */


int an_graph0 (call_type, port_num, out_fd, option_string)
    int		   call_type;		/* type of actions to perform	     */
    port_num_type  port_num;		/* manager port number		     */
    FILE	   *out_fd;    		/* file descriptor of output file    */
    char	   *option_string;	/* driver-spec options user typed    */

/*
 *  This routine is the interface routine for analyze to the graph0 analysis 
 *  routines.  It first grabs the generic manager info, then the pda.  It     
 *  then performs the correct action based upon the value of call_type.  
 *  All an_graph02 routines may be found in an_graph02.c
 */

{
    graph_pda_type	  pda;		/* port data area                    */
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
	    an_graph02_check_revs (me, my_rev_var_name, graph0_an_rev, out_fd);
	    break;
	case AN_MGR_BASIC:
	    an_graph02_pda_info (&pda, AN_WHICH_GRAPH, out_fd);
	    break;
	case AN_MGR_DETAIL:
	    an_graph02_all (&pda, AN_WHICH_GRAPH, out_fd);
	    break;
	case AN_MGR_OPTIONAL:
	    an_graph02_optional (&pda, AN_WHICH_GRAPH, option_string, out_fd);
	    break;
	case AN_MGR_HELP:
	    an_graph02_help (out_fd);
	    break;
    }
    return(0);
}
