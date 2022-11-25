#include <stdio.h>
#include <string.h>
#include <sio/llio.h>
#include <h/types.h>
#include <sio/iotree.h>
#include "aio.h"
#include "mgr_decode.h"

/* table used for quick lookup of analyze routines */

/*    This table contains an entry for each instance of a manager which is   */
/*    is analyzable. The order of the entries is important and is as         */
/*    follows:								     */
/*         o   The first entries are managers which have ports. These        */
/*             entries are in ascending port number order.		     */
/*	   o   The rest of the entries are associated with the analyzable    */
/*             pseudo-drivers. Note that there is no generic way to          */
/*             determine if the pseudo-driver was being used in the dumped   */
/*             system.							     */

struct fast_table_type {
    int			index;		 /* index into mgr_decode_table      */
    port_num_type	port_num;	 /* -1 if none			     */
    int			flag;		 /* temporary per port flag	     */
};

static struct fast_table_type	fast_table[100]; /* table declaration	     */
static int			fast_size = 0;	 /* # of fast_table entries  */

/* miscellaneous global thingies */
extern int	max_port;
static int 	an_mgr_first_time=1;	 /* do we have to do init this time? */

#define	AN_NUM_MGRS_SUPPORTED	\
	sizeof(mgr_decode_table)/sizeof(mgr_decode_table[0])
#define	streq(a, b)	(strcmp((a), (b)) == 0)

/******************************************************************************/
/**                                                                          **/
/**  This routine is called to initialize all of the manager analysis rtns   **/
/**  which are part of analyze. This routine may be called many times, but   **/
/**  it only does the initialization the first time. The initialization      **/
/**  consists of a revision check and the setting up of llio status and      **/
/**  message decoding routines. In addition, the mgr_port_table is set       **/
/**  up. This table is used by routines in this file as a short-hand way to  **/
/**  get info about a port.                                                  **/
/**                                                                          **/
/**  Note that *all* manager routines are initialized at once. The other way **/
/**  to do it was to only do the rev checks on the routines as I used them.  **/
/**  I decided against this because a routine may depend on a decoding       **/
/**  function supplied by a different routine. If the routine supplying the  **/
/**  decoding function is out of date, I think we want to know it up front.  **/
/**                                                                          **/
/******************************************************************************/

an_mgr_do_init(outf)
    FILE	*outf;	/* file descriptor to print to	*/
{
    int		i, j;		/* loop counters		*/
    char	name[16];	/* manager name			*/
    int 	(*decoder)();	/* manager analysis routine	*/

    /* check to see if initialization has already been done */
    if (!an_mgr_first_time) return;
    an_mgr_first_time = 0;

    /* print i/o manager analysis header */
    fprintf(outf,"\n");

    /* for each port that has an analysis rtn, make a fast_table entry */
    for (i = 1; i <= max_port; i++) {
	if (aio_get_mgr_name(i, name) == AIO_OK) {
	    for (j = 0; j < AN_NUM_MGRS_SUPPORTED; j++) {
		if (streq(mgr_decode_table[j].mgr_name, name)) {
		    fast_table[fast_size].index = j;
		    fast_table[fast_size].port_num = i;
		    fast_size++;
		    break;
		}
	    }
	}
    }

    /* make fast_table entries for managers which don't have ports */
    for (j = 0; j < AN_NUM_MGRS_SUPPORTED; j++) {
	if (mgr_decode_table[j].has_port == 0) {
	    fast_table[fast_size].index = j;
	    fast_table[fast_size].port_num = NONE;
	    fast_size++;
	}
    }

    /* initialize each installed analysis routine */
    for (j = 0; j < AN_NUM_MGRS_SUPPORTED; j++) {
	for (i = 0; i < fast_size ; i++) {
	    if (j == fast_table[i].index &&
	        (decoder = mgr_decode_table[j].analysis_rtn)) {
	        (*decoder)(AN_MGR_INIT, fast_table[i].port_num, outf);
		break;
	    }
	}
    }
}

/******************************************************************************/
/**                                                                          **/
/**  This routine calls the basic option for all "analyzable" managers in    **/
/**  the configuration. It is invoked in response to the runstring "z"       **/
/**  option.                                                                 **/
/**                                                                          **/
/******************************************************************************/

an_mgr_basic_analysis(outf)
    FILE *outf;
{
    int	i;		/* loop counter				*/
    int (*decoder)();	/* manager analysis routine		*/

    /* do the initialization */
    an_mgr_do_init(outf);

    /* call every instance of every analyzable module with the basic option */
    for (i = 0; i < fast_size; i++) {
	 decoder = mgr_decode_table[fast_table[i].index].analysis_rtn;
	 an_mgr_hdr_message(outf, i);
	 (*decoder)(AN_MGR_BASIC, fast_table[i].port_num,outf);
    }
}
/******************************************************************************/
/**                                                                          **/
/**  This routine calls the detail option for all "analyzable" managers in   **/
/**  the configuration. It is invoked in response to the runstring "Z"       **/
/**  option.								     **/
/**                                                                          **/
/******************************************************************************/

an_mgr_detail_analysis (outf)
    FILE *outf;		/* file descriptor to print to		*/
{
    int	i;		/* loop counter				*/
    int (*decoder)();	/* manager analysis routine		*/

    /* do the initialization */
    an_mgr_do_init(outf);

    /* call every instance of every analyzable module with the detail option */
    for (i = 0; i < fast_size; i++) {
	 decoder = mgr_decode_table[fast_table[i].index].analysis_rtn;
	 an_mgr_hdr_message(outf, i);
	 (*decoder)(AN_MGR_DETAIL, fast_table[i].port_num, outf);
    }
}

/******************************************************************************/
/**                                                                          **/
/**   This routine displays the generic help screen. A brief synopsis of     **/
/**   syntax is followed by a list of available managers and whether or not  **/
/**   they are analyzable.                                                   **/
/**                                                                          **/
/******************************************************************************/

an_mgr_help(outf)
    FILE		 *outf;	/* file descriptor to print to	*/
{
    int			 i;		/* loop counter			*/
    int			 ct	= 0;	/* index for fast_table		*/
    int			 skipct	= 0;	/* time to print a blank line?	*/
    struct mgr_info_type mgr;		/* all manager info		*/

    /* print header */
    fprintf(outf,"\nio [mgr_name] | [port n] | [help] | [mgr options] | [> | >> filename]\n");
    fprintf(outf,"\n");
    fprintf(outf,"Port   Hardware Address   Manager Name  Analyzable?\n");
    fprintf(outf,"----   ----------------   ------------  -----------\n");
    fprintf(outf,"\n");

    /* display all managers that have ports */
    for (i = 1; i <= max_port; i++) {
	if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, i, &mgr) == AIO_OK) {
	    fprintf(outf, "%3d         %-10s      %-10s ", i, mgr.hw_address,
		mgr.mgr_name);
	    if (ct < fast_size && fast_table[ct].port_num == i) {
		fprintf(outf, "    YES!\n");
		ct++;
	    }
	    else
		fprintf(outf, "     no\n");

	    if ((++skipct % 5) == 0) fprintf(outf,"\n");
	}
    }

    /* now do the pseudo-drivers which we have analysis routines for */
    if (ct < fast_size) fprintf(outf, "\n");

    for (i = ct; i < fast_size; i++) {
	fprintf(outf, "analyzable pseudo driver: %s\n",
		mgr_decode_table[fast_table[i].index].mgr_name);
    }
}

/******************************************************************************/
/**                                                                          **/
/**   This routine is called when a user has typed "io" during an            **/
/**   interactive session. The rest of the line is passed in to this         **/
/**   routine in runstring. This routine parses the rest of the line and     **/
/**   handles:                                                               **/
/**       o   obvious errors -- a help menu is printed                       **/
/**       o   a request for generic help                                     **/
/**       o   the redirection file is opened (if specified)                  **/
/**       o   calling all manager analysis routines that were specified      **/
/**           with the help option                                           **/
/**       o   calling all manager analysis routines that were specified      **/
/**           with the remaining (manager_specific) options                  **/
/**                                                                          **/
/******************************************************************************/
#define	START		0
#define	PORT		1
#define FILE_WRITE	2
#define FILE_APPEND	3

an_mgr_decode(outf, runstring)
    FILE	*outf;		/* file descriptor to print to	*/
    char	*runstring;	/* runstring to decode/process  */
{
    int		i, j;			/* loop counters		*/
    int		port;			/* port number			*/
    int 	(*decoder)();		/* decoder routine		*/
    int		state = START;		/* parser state			*/
    int		flag;			/* temporary flag		*/
    int		redir_flag	= 0;	/* are we doing redirection?	*/
    int		help_flag	= 0;	/* was help specified?      	*/
    char	*token;			/* token I am parsing		*/
    char	option_string[255];	/* options I will send out	*/
    FILE 	*newf = outf;		/* file supplied for redirection*/

    /* do some initialization */
    an_mgr_do_init(outf);
    option_string[0] = NULL;
    for (i = 0; i < fast_size; i++) fast_table[i].flag = 0;

    /* grab the first token -- if the null string, do help */
    if ((token = strtok(runstring, " ")) == NULL) {
	an_mgr_help(outf);
	return;
    }

    /* walk through the runstring and record each option */
    do {

	/* process token depending on what state we are in */
	if (state == START) {

	    /* process a help option */
	    if (streq(token, "help"))		help_flag = 1;

	    /* process a port token */
	    else if (streq(token,"port"))	state = PORT;

	    /* process a redirection token */
	    else if (streq(token, ">") || streq(token,">>")) {
		if (redir_flag++) {
		    fprintf(outf, "\nOnly one redirection allowed\n");
		    an_mgr_help(outf);
		    return;
		}
		if (strcmp(token,">") == 0)
		    state = FILE_WRITE;
		else
		    state = FILE_APPEND;
	    }

	    /* otherwise, not a keyword so either match a manager name or add */
	    /*    to option list					      */
	    else {
		for (flag = i = 0; i < AN_NUM_MGRS_SUPPORTED; i++) {
		    if (streq(token, mgr_decode_table[i].mgr_name))
			for (j = 0; j < fast_size; j++)
			    if (fast_table[j].index == i)
				flag = fast_table[j].flag = 1;
		}
		if (flag == 0) {
		    (void)strcat(option_string, token);
		    (void)strcat(option_string, " ");
		}
	    }
        }

	else if (state == PORT) { /* this token better be a port number! */
	    if (stoi(token, &port, outf) || port < 1 || port > max_port) {
		fprintf(outf,"\nPort number <%s> is out of range\n", token);
		an_mgr_help(outf);
		return;
	    }
	    for (j = 0; j < fast_size; j++) {
		if (fast_table[j].port_num == port) {
		    fast_table[j].flag = 1;
		    state = START;
		    break;
		}
	    }
	    if (state != START) {
		fprintf(outf, "\nPort number <%s> is not analyzable\n", token);
		an_mgr_help(outf);
		return;
	    }
	}
	else if (state == FILE_WRITE || state == FILE_APPEND) {
	    char *mode = (state == FILE_APPEND) ? "a+" : "w+";

	    if ((newf = fopen(token, mode)) == NULL) {
		fprintf(outf, "\nCan't open file %s\n",token);
		an_mgr_help(outf);
		return;
	    }
	    state = START;
	}
    } while ((token = strtok(NULL, " ")) != NULL);


    /* now check to see if we hit end of line with something left undone */
    if (state == PORT) {
	fprintf(outf,"\nNo port number specified after `port'\n");
	an_mgr_help(outf);
	return;
    }
    else if (state == FILE_WRITE || state == FILE_APPEND) {
	fprintf(outf,"\nNo filename specified after a redirection symbol\n");
	an_mgr_help(outf);
	return;
    }

    /* call all of the manager analysis routines that were marked */
    for (flag = i = 0; i < fast_size; i++) {

	if (fast_table[i].flag == 1) { /* grab analysis rtn and print header */
	    flag = 1;
	    an_mgr_hdr_message(newf, i);
	    decoder = mgr_decode_table[fast_table[i].index].analysis_rtn;

	    /* if this is for help, eliminate duplicate calls and call mgr */
	    if (help_flag || option_string[0] == NULL) {
		for (j = i; j < fast_size; j++) {
		    if (fast_table[j].index == fast_table[i].index)
			fast_table[j].flag = 0;
		}
		(*decoder)(AN_MGR_HELP, fast_table[i].port_num, newf,
			option_string);
	    }

	    /* otherwise, call routine with option string */
	    else
		(*decoder)(AN_MGR_OPTIONAL, fast_table[i].port_num, newf,
			option_string);
	}
    }

    /* either only help was specified or only garbage was spec'd */
    if (flag == 0) {
	if (option_string[0] != NULL && help_flag == 0)
	    fprintf(outf,"\nNo valid managers specified\n");
	an_mgr_help(outf);
    }
}

/******************************************************************************/
/**                                                                          **/
/**   This routine is called before a manager analysis routine is called.    **/
/**   It prints a header message containing the name of the manager, the     **/
/**   port number, and the hardware address.				     **/
/**                                                                          **/
/******************************************************************************/

an_mgr_hdr_message(outf, index)
    FILE	*outf;			/* file descriptor to print to	*/
    int		index;			/* index into fast_table	*/
{
    struct mgr_info_type mgr;		/* manager info stuff		*/
    port_num_type	 port_num;	/* port number			*/

    /* grab the port number */
    port_num = fast_table[index].port_num;

    /* if manager has a port, grab info about it and print */
    if (port_num != NONE) {
	(void)aio_get_mgr_info(AIO_PORT_NUM_TYPE, port_num, &mgr);
	fprintf(outf, "\nAnalysis of %s   <port %d>    <hw address %s>:\n",
		mgr.mgr_name, port_num, mgr.hw_address);
    }

    else /* a pseudo-driver, print */
	fprintf(outf, "\nAnalysis of pseudo-driver %s:\n",
		 mgr_decode_table[fast_table[index].index].mgr_name);
}
