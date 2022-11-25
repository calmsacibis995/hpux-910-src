#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <utmp.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/sysmacros.h>
#include <string.h>

#if     defined(DUX) || defined(DISKLESS)
#include <cluster.h>
#include <sys/nsp.h>
#include <sys/param.h>
#endif

#ifdef DEBUG
#include "sys/reboot.h"
#else
#include <sys/reboot.h>
#endif /* DEBUG */

#if defined(SecureWare) && defined(B1)
#include <sys/security.h>
#include <sys/audit.h>
#endif

#ifdef LOCAL_DISK
#include <netdb.h>
#include <sys/pstat.h>
#endif /* LOCAL_DISK */

#ifdef LOCAL_DISK
/*
 * General use defines
 */
#define TRUE 1
#define FALSE 0

#define PSBURST 10				/* # pstat structs to get    */

/*
 * Globals used for root server and swap server reboot information
 *   Made global so that the active_clients() routine could use the same
 *   variables, especially mysite, rootserver, and swapserver
 */
extern cnode_t 	mysite;			/* cnode id for executing site	  */
extern int 	rootserver;		/* boolean for root server	  */
extern int 	num_active_clients;	/* # clients of this root/swap svr*/
extern cnode_t active_clients[MAXCNODE]; /* active client cnode ids 	  */
extern struct	entry
  {
	    char *name;			/* name of cnode 		  */
	    int	donot_reboot;		/* true if shouldn't be rebooted  */
  } cnode_array[MAXCNODE];		/* names of cnodes in clusterconf */

extern int 	swapserver;		/* boolean for swap server	  */

/*
 *  Routine declarations:
 */
int	clients_rebooted();		/* waits for client reboot 	  */
void 	get_cnode_names();		/* initializes global cnode_array */

#ifdef __STDC__

void	print_active_clients(FILE *);	/* prints names of active clients */
int 	kill_process(char *);		/* kills a process		  */
unsigned int sleep (unsigned int);
void 	*malloc (size_t);

#else /* __STDC__ */

void	print_active_clients();		/* prints names of active clients */
int 	kill_process();			/* kills a process		  */
unsigned int sleep ();
void 	*malloc ();

#endif /*__STDC__ */

#endif /* LOCAL_DISK */

extern char hostname [];
extern int debugLevel;
extern FILE *debugStream;

#ifdef LOCAL_DISK
/*
 * clients_rebooted() -- This routine uses the global rootserver and
 * swapserver varaibles and determines via cnodes() or swapclients()
 * if the server has any active clients.  
 * 
 * This routine waits up to 600 seconds for the clients to all reboot and
 * issues a warning about the delinquent clients every 15 seconds.  (A
 * swap server only waits up to 300 seconds.)  Since the routine quits as
 * soon as all clients have rebooted, it will only wait for a long time in
 * the event of a problem.  In this case the wait period is long enough that
 * the user may have time to go get it fixed.  The wait time for a swap
 * server is half as long as a root server.  This was done since the root
 * server has to wait for all swap servers and they have to wait for all
 * of their swap clients.  Seems like this could take up to two times longer
 * on a root server.
 * 
 * It returns 1 if all clients have rebooted and 0 otherwise.
 * The executing site does not count as a client.
 *
 * This routine leaves a current and valid information in 
 * num_active_clients and in active_clients.
 *
 * NOTE:  THIS ROUTINE IS ALSO USED IN SHUTDOWN(1M) 
 * 	ANY CHANGES MADE HERE SHOULD ALSO BE MADE FOR SHUTDOWN!!!
 */
/**********************************************************************/
int
clients_rebooted()
/**********************************************************************/
{

    int wait_time;	/* total seconds to wait for client reboots */
    unsigned int sleep_time; 	/* time to sleep between checks  */

    /*
     * Initialize Wait times
     */
    if (rootserver) 
	/* 
	 *  rootserver should wait 1.5 minutes for swapserver reboot 
	 */
	wait_time = 600;	

    else
	/* 
	 *  but swap servers only have to wait 1 minute 
	 */
	wait_time = 300; 

    /*
     * While there's still active clients and while our time is not up,
     * Re-Calculate num_active_clients on EACH loop.  Use cnodes(2) if
     * it's a root server, use swapclients if its just a swap server.
     */
    while (((num_active_clients = rootserver? cnodes(active_clients): 
		swapclients(active_clients)) > 1) && wait_time > 0) 
      { 

	if (debugLevel > 2)
	  (void) fprintf (debugStream, "In clients_rebooted wait loop\n");

	/*
	 * Only print which clients we're waiting for every 15 seconds
	 */
	if (wait_time % 15 == 0) 
	  {
	    /*
	     * Explain what we're waiting for
	     */
	    (void) fprintf(stderr,
   "\nWaiting a maximum of %d min, %d sec for the following cnodes to reboot\n",
	       (wait_time / 60), (wait_time % 60));
	    print_active_clients(stderr);
	  }

	/*
	 * Sleep 5 seconds and try again
	 */
	sleep_time = 5;	

	/* 
	 *  decrement total tally 
	 */
	wait_time = wait_time - sleep_time;	

	/*  
 	 *  sleep (3) returns the unslept time.
	 */
	do 
	  {
	    sleep_time = sleep(sleep_time);
	  } while (sleep_time > 0);
      }

    /*  If num_active_clients is > 1 then not all clients rebooted.  If it 
     *  is 1, then they all rebooted.
     */
    return ((num_active_clients > 1) ? FALSE : TRUE);
}

/*
 * get_cnode_names() -- This routine uses getcctent() to read in the names
 * of all possible cnodes in the cluster that this cnode may have to reboot.  
 *
 * Copy the name of every cnode in clusterconf into the global cnode_array[] 
 * array at the appropriate index location so that cnode names may be 
 * determined via cnode_array[cnode_id].name.
 *
 * For root servers, set the donot_reboot flag for any clients of auxiliary
 * swap servers, as they'll be rebooted by their swap servers.
 *
 * This routine leaves a null pointer at a "valid" index location 
 * in case of a getcctent or a malloc error, therefore, users of the 
 * cnode_array[] should always check for NULL pointers!
 *
 * This routine relies on global variable space being initialized to 0.
 */
/**********************************************************************/
void 	
get_cnode_names()
/**********************************************************************/
{
    struct cct_entry *cct_ent;	/* /etc/clusterconf entry pointer */
    int mallocErr = FALSE;

    setccent();			/* rewind clusterconf pointer, just in case */

    while ((cct_ent = getccent()) != NULL) 
      {
	/* 
	 *  Malloc space for cnode name at the appropriate index location 
	 */
	if ((cnode_array[cct_ent->cnode_id].name = 
		(char *)malloc(strlen(cct_ent->cnode_name) + 1)) == NULL) 
	  {
	    /* 
	     *  WARN, but go on.  (cnode_array users must chk for null) 
	     *  Actually, just set a boolean here.  Then after we're done,
	     *  output an error message.  Otherwise, we might get a whole lot
	     *  of these errors.
	     */
	    mallocErr = TRUE;
	  }

	else 
	  {
	    /* 
	     *  Copy in cnode name
	     */
	    (void) strcpy(cnode_array[cct_ent->cnode_id].name, 
			  cct_ent->cnode_name);

	    /*
	     * If this is another cnode's swap client, set don't reboot 
	     * flag for root server.
	     *
	     *  This implies that 
	     *
	     *    - if we are not the root server then we think  we're 
	     *      reboooting everything.
	     *    - if the system swaps to me then we think we will reboot it
	     *    - if the system swaps to itself then we think we will reboot 
	     *      it
	     */

	    if (rootserver && cct_ent->swap_serving_cnode != mysite
		&& cct_ent->swap_serving_cnode != cct_ent->cnode_id) 
	      cnode_array[cct_ent->cnode_id].donot_reboot = TRUE;
	  }
      }

    endccent();		/* close clusterconf */

    if (mallocErr)
      (void) fprintf (stderr, 
     "Warning: not all cluster system names could be obtained-- continuing.\n");

    return;
}

/*
 * print_active_clients -- Routine prints list of active clients using
 * 	contents of global variables num_active_clients and active_clients
 *
 *	THIS ROUTINE ASSUMES THAT THESE VARIABLES HAVE ALREADY BEEN 
 *	POPULATED VIA cnodes(2) OR swapclients(2) !!!
 */
/**********************************************************************/
void 
print_active_clients(stream)
FILE *stream;		/* file stream to which fprintf should write */
/**********************************************************************/
{
    int index;
    for (index = 0; index < num_active_clients; index++) 
      {
	/* 
	 * Don't warn about ourselves! 
	 */
	if (active_clients[index] == mysite) 
	    continue;	

	/* 
	 * If we can't get site information, bail and go on to the next site
	 * Otherwise, print the cnode name.
	 */
	if (cnode_array[active_clients[index]].name)
	  (void) fprintf(stream, "\t%s\n", 
			 cnode_array[active_clients[index]].name);
      }
}
#endif /* LOCAL_DISK */

/*  
 *  This code was originally taked from the ps code.
 *
 * kill_process() -- This subroutine takes a string which contains a
 *			process name (or a portion of a process name)
 *			and uses pstat to search active processes 
 *			for a command line which contains the string
 *			passed in process_name.
 *
 *			A 0 is returned if a process is actually sent
 *			a kill signal, a 1 is returned if no process
 *			was found which met the criteria.  If an 
 *			error occured in the pstat routines, then an
 *			appropriate error message is sent and a -1 is
 *			returned.
 *
 *			kill_process kills ALL processes it can find
 *			which match the criteria, not just the first
 *			one it finds.
 */
/**********************************************************************/
int
kill_process(process_name)
char *process_name;
/**********************************************************************/
{
    int return_value = 0;		/* initialize to "found"	     */
    int proc_count;			/* # of procs returned by pstat()    */
    struct pst_status proc_buf[PSBURST];/* buffer of pstat structs	     */
    int proc_index = 0;			/* index into pidlist 		     */
    int loopIndex;                      /* index for going through procs     */
    int termLoop;                       /* counter for killing the proc      */
    short gotIt;                        /* boolean for if proc killed or not */
    errno = 0;				/* initialize errno		     */

    /*
     * Use pstat to get each burst of process information
     * 
     *  Start at the top of the proc list 
     */
    while ((proc_count = pstat(PSTAT_PROC, proc_buf, 
		sizeof(struct pst_status), PSBURST, proc_index)) > 0)
      {
	/*
	 * Walk thru each process looking for process_name
	 */
	for (loopIndex = 0; loopIndex < proc_count; loopIndex++)
	  {

	   /*  
	    *  In the most general case, this will kill a process whose command
	    *  name has the string anywhere in it.  Thus, if the string we are
	    *  looking for is rbootd, commands like "rbootd", "/etc/rbootd", 
	    *  and "rbootdr1" will all be killed.   However, this will also
	    *  pick up strings like "/etc/rbootd /dev/lan0" which an ordinary
	    *  search for the last part of the string being the command name
	    *  would not catch.
	    *
	    *  If the name we're looking for is in the command, then attempt 
	    *  to kill the process with a SIGTERM.  Do this in a loop for 
	    *  either 5 times, or until the process is gone.  If this didn't 
	    *  successfully kill the process, issue a SIGKILL to it.  Use a 
	    *  boolean to figure out if the process was killed with the SIGTERM.
	    */
           
	   if (strstr (proc_buf [loopIndex].pst_cmd, process_name))
	     {
	       gotIt = FALSE;

	       for (termLoop = 0; termLoop < 5; termLoop ++)
		 {
		   if (debugLevel > 2)
		     {
		       fprintf (debugStream, "kill:1: %s : %i trying\n",
				proc_buf [loopIndex].pst_cmd,
				proc_buf [loopIndex].pst_pid);
		     }

	           if (((kill (proc_buf [loopIndex].pst_pid, SIGTERM)) == -1)
			       && (errno == ESRCH))
		     {
		       if (debugLevel > 2)
		           fprintf (debugStream, "kill:1: %i was killed\n",
				proc_buf [loopIndex].pst_pid);

		       gotIt = TRUE;
	               return_value = 0;	
		       break;
		     }
		   else
		     {
		       if (debugLevel > 2)
		           fprintf (debugStream, "kill:1: %i was not killed : errno = %i\n",
				proc_buf [loopIndex].pst_pid, errno);
		       sleep(1);
	             }		
		 }

	       if (! gotIt)
		 {
	           (void) kill (proc_buf [loopIndex].pst_pid, SIGKILL);
		   sleep (3);

	           if (((kill (proc_buf [loopIndex].pst_pid, SIGKILL)) == -1)
			       && (errno == ESRCH))
		     return_value = 0;
		   
		   else
		     {
		       return_value = -1;
		       if (debugLevel)
			   fprintf(debugStream, "kill:2: %i not killed; errno = %i\n",
				   proc_buf [loopIndex].pst_pid, errno);
		     }	
		 }
	      }
	  }
	/*
	 * Set the index appropriately to get the next burst of process stats
	 */
	proc_index = proc_buf[proc_count-1].pst_idx+1;
      }

    /* 
     *  -1 if process couldn't be killed; 0 if process killed or not found
     */
    return return_value;	
}				/* END OF KILL_PROCESS */
