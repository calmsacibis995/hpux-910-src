/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/recovery.c,v $
 * $Revision: 1.12.83.4 $        $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/09 13:34:45 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/mp.h"		/* For definition of ON_ICS */
#include "../h/reboot.h"
#ifdef __hp9000s300
#include "../wsio/intrpt.h"	/* has definition of struct interrupt */
#include "../wsio/timeout.h"

#include "../s200io/lnatypes.h"
#endif /* __hp9000s300 */
#define FALSE	0
#define TRUE 	1


#include "../dux/dmmsgtype.h"
#include "../dux/dm.h"
#include "../dux/protocol.h"
#include "../dux/cct.h"			/*Cluster configuration table*/
#include "../dux/nsp.h"
#undef timeout

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */

#ifdef DUX_SENDALIVE
#ifdef __hp9000s300
struct sw_intloc alive_intloc;	     /* for sw_trigger of send_alive message */
#endif /* __hp9000s300 */
extern int send_alive_period;        /* how often to send an alive msg */
int send_alive();
#endif DUX_SENDALIVE

struct mbuf send_alive_mbuf;     /* static mbuf for send_alive function  */
extern init_static_mbuf();
int dux_send_alive();

#ifdef __hp9000s300
struct sw_intloc chk_alive_intloc;   /* for sw_trigger of check_alive message */
#endif /* __hp9000s300 */
extern int check_alive_period;	     /* how often to check for alive msgs     */
int dux_check_alive();

#ifdef __hp9000s300
struct sw_intloc retry_alive_intloc; /* for sw_trigger of retry of alive msgs */
#endif /* __hp9000s300 */
int retry_alive();
int dux_retry_alive();
extern int retry_alive_period;    /* how long alive msgs shd be retried    */
int retry_alive_timeout = 1;	  /* interval between retries */
				  /* Series 800 lan break detection */
				  /* assumes this will always be 1 */
int retries_till_LBC = 4;	  /* how many retries before we do LAN brk ck */
u_char alive_retries[MAXSITE];	  /* count retries of alive packets	      */
int retrysites;			  /* count total # of sites in CL_RETRY    */
int failure_sent;		  /* flag- set when broadcast failure      */

extern volatile int free_nsps;
struct mbuf dux_boot_mbuf;

#ifdef __hp9000s300
struct sw_intloc cleanup_intloc;  /* for sw_trigger of init_cleanup message*/
#endif /* hp9000s300 */
int dux_init_cleanup();

extern int using_debugger;		/* must be set by recovery_on/off */

extern volatile struct cct clustab[];	/*cluster configuration table*/
extern site_t my_site;
extern site_t root_site;
extern site_t swap_site;
extern char clustcast_addr[];

int failed_sites = 0;		/*no of failed sites*/
int cleanup_running = 0;	/*0-no cleanup process running, 1- running*/

int fs_cleanup(), swap_cleanup(), mcleanup(), pid_cleanup(), cleanup_serving_array(), cleanup_using_array(); 

/* message specific structure for the DM_MARK_FAILED message */
struct dux_failmsg {
	site_t failed_site;
};

struct cleanuplist {
	int (*func)();
	};

struct cleanuplist cleanuplist[] = 
{
	cleanup_serving_array,	/* network cleanup */
	fs_cleanup,		/* file system */
	mcleanup,		/* mount table */ 	
	swap_cleanup, 		/*   VM  system*/
	pid_cleanup, 		/* Process IDs */
	NULL,
};



/*
** Send ALIVE datagrams to dest_site. Since this is such an important event, 
** it was felt the req'd mbuf should be a static structure that is always 
** available and reused. This routine cannot be reentered since it uses
** this static structure.  want_response indicates that the P_ALIVE_MSG,
** which forces a quick protocol response from the dest_site, should be
** set.  Otherwise, the ALIVE msg is sent with the P_DATAGRAM flag set
** and no reply will be sent.  Since datagrams are not retried from the 
** protocol level, these msgs are retried via the dux_retry_alive routine.
*/
dux_send_alive( want_response, dest_site ) 
int want_response;	/* TRUE or FALSE */
site_t dest_site;	/* single site or CLUSTERCAST */
{
register struct mbuf *reqp;
struct dm_header *dmp;
register struct proto_header *ph;
extern struct mbuf send_alive_mbuf;
int err;

	/*
	** Initialize the static mbuf.
	*/
	err = init_static_mbuf( DM_EMPTY, &send_alive_mbuf );
	if( err == -1 )
	{
		goto errout;
	}

	if( (reqp =  &send_alive_mbuf) != NULL )
	{
		dmp = DM_HEADER(reqp);
		ph = &(dmp->dm_ph);
		if (want_response) ph->p_flags = P_ALIVE_MSG;
		else ph->p_flags = P_DATAGRAM;
		ph->p_srcsite = my_site;
		dm_send(reqp, DM_DATAGRAM, DM_ALIVE, dest_site,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

#ifdef DUX_SENDALIVE
		timeout (send_alive,
			(caddr_t)&send_alive_period,
			send_alive_period*hz);
#endif DUX_SENDALIVE
	}
	else
	{
		/*
		** We should always have this mbuf available and
		** initialized. Something has stomped on this mbuf.
		*/
errout:
		panic("send_alive: static DM_ALIVE mbuf is NULL!");
	}
}


#ifdef DUX_SENDALIVE
/*
** Call send_alive.  Must sw_trigger down prior to calling hw_send
** and/or getting a mbuf.
*/
send_alive()
{
/*
** WARNING:  if this is ifdef'd TRUE, then you'd better pass some parameters
** into dux_send_alive.  (probably FALSE and DM_CLUSTERCAST).
*/
#ifdef __hp9000s300
	sw_trigger(&alive_intloc,
		   dux_send_alive,
		   0,
		   LAN_PROC_LEVEL,
		   LAN_PROC_SUBLEVEL+1);
#else
    /* timeout is running under spl2() on S800, it shouldn't interrupt
     * others getting mbuf.
     */
	dux_send_alive(FALSE,DM_CLUSTERCAST);
#endif /* hp9000s300 */

}
#endif DUX_SENDALIVE


/* When  "I am alive" message arrives the server site, the network
 * driver should recognize it and invoke the following function.  This 
 * function will be executed as an interrupt.
 */
register_alive (reqp)
	dm_message reqp;
{
	struct dm_header *dmp = DM_HEADER(reqp);
        struct proto_header *ph = &(dmp->dm_ph);
	site_t site;
	int s;

	site = ph->p_srcsite;
	dm_release(reqp,0);	
	if ((site >= 1) && (site < MAXSITE)) {
		if ((clustab[site].status & CCT_STATUS_MASK) == CL_IS_MEMBER) {
				s = splimp();
				clustab[site].status = CL_ACTIVE;
				splx(s);
		}
	}
}


/*
** check_alive() is now a simple function used to sw_trigger down
** the priority level of the invoking timeout, prior to calling
** dux_check_alive (which was the check_alive() code). This is necesary
** because dux_check_alive() could call retry_alive which could call 
** declared_failed(site), which could call init_cleanup(), which does 
** call dm_alloc to get a mbuf.  If mbuf allocation code is re-entered 
** prior to updating pointers, then bad things happen later on.
*/

check_alive() 
{
#ifdef __hp9000s300
    sw_trigger(&chk_alive_intloc, dux_check_alive, 0, 0, LAN_PROC_SUBLEVEL);
#else
    /* timeout is running under spl2() on S800, it shouldn't interrupt
     * others getting mbuf.
     */
    dux_check_alive();
#endif /* hp9000s300 */
}

/*
** dux_check_alive
*/
dux_check_alive() 
{
	register struct cct *cctp;	/*cluster to cluster conf table*/
	register site_t site;
	int s;
	register u_int is_root = IM_SERVER;

	/* 
	** If we're the root site, then we want to check the status of
	** every other site in the cluster.  If we're not the root site
	** then we want to check the status of the root site only.
	*/
	if (is_root) {
	    cctp = clustab+1;
	    site = 1;
	} else { /* not root */
	    site = root_site;
	    cctp = &clustab[root_site];
	}

	/*
	** Always check at least once in case we're a client.  If we're root
	** go thru the entire table.
	*/
	do {
	   if (site != my_site) 
		switch(cctp->status)
		{
		case CL_ACTIVE:
			s = splimp();
			cctp->status = CL_ALIVE;
			splx(s);
			break;

		case CL_ALIVE:
			alive_retries[site] = 0;   /* clear retries count   */
			s = splimp();
			cctp->status = CL_RETRY;   /* indicate retrying     */
			splx(s);
			retrysites++; /* indicate retry_alive shd be called */
			dux_send_alive(TRUE,site); /* send an alive request */
			break;
		case CL_RETRY:
			retrysites++;
			break;
		default:
			/* if state CL_INACTIVE or CL_FAILED then ignore it  */
			break;
		}
		site++;
	} while ((++cctp < clustab+MAXSITE) && (is_root));

	/* schedule next check */
	timeout (check_alive, (caddr_t)&check_alive_period, 
		check_alive_period*hz);
}	

/*
** retry_alive() is a simple function used to sw_trigger down
** the priority level of the invoking timeout, prior to calling
** dux_retry_alive.  This is necesary because dux_retry_alive() 
** could call declared_failed(site), which could call init_cleanup(), 
** which does call dm_alloc to get a mbuf.  If mbuf allocation code 
** is re-entered prior to updating pointers, then bad things happen 
** later on.  Also, dux_retry_alive, could call dux_send_alive which
** messes with a static mbuf and, therefore, cannot be reentered.
*/

retry_alive() 
{
	/* if there are any sites to retry, then trigger down and retry */
	if (retrysites){

	  if (using_debugger) {
		  retrysites =0;
		  timeout (retry_alive, (caddr_t)&retry_alive_timeout, 
			   retry_alive_timeout*hz);
		  return;
	  }

#ifdef __hp9000s300
	    sw_trigger(&retry_alive_intloc, dux_retry_alive, 0,
					LAN_PROC_LEVEL, LAN_PROC_SUBLEVEL );
#else
    /* timeout is running under spl2() on S800, it shouldn't interrupt
     * others getting mbuf.
     */
	   dux_retry_alive();
#endif /* hp9000s300 */

	}
	timeout (retry_alive, (caddr_t)&retry_alive_timeout, 
		retry_alive_timeout*hz);
}

dux_retry_alive()
{
    register struct cct *cctp;	/*cluster to cluster conf table*/
    register site_t site;
    int err;
    int num_alive_retries = retry_alive_period/retry_alive_timeout;
    register u_int is_root = IM_SERVER;

    /* clear retry sites incase we're all done after this */
    retrysites = 0;

    /* 
    ** If we're the root site, then we want to check the status of
    ** every other site in the cluster.  If we're not the root site
    ** then we want to check the status of the root site only.
    */
    if (is_root) {
	cctp = clustab+1;
	site = 1;
    } else { /* not root */
	site = root_site;
	cctp = &clustab[root_site];
    }
    /*
    ** Always check at least once in case we're a client.  If we're root
    ** go thru the entire table.
    */
    do {
       if ((cctp->status == CL_RETRY) && (site != my_site)) {
	    retrysites++;
	    /*
	    ** If we haven't hit the max poss. number of retries, then simply
	    ** try again...
	    */
	    if ((alive_retries[site]++) < num_alive_retries) {
		/*
		** Part way thru retries, run LAN check break as a work around
		** for LAN hardware bug.  This resets card if it's locked up
		** and lets us respond to the other nodes alive msgs.
		*/
#ifdef __hp9000s800
		/*
		** On the Series 800, we have to "set up" for lan break
		** detection 3 seconds before we check for a lan
		** break.  WARNING: For simplicity, this code assumes that
		** retry_alive_timeout is one second.
		*/
		if ((alive_retries[site] == (retries_till_LBC + 1 - 3)) ||
	    	     alive_retries[site] == (num_alive_retries - 3)) {
			init_check_lan_break(site);
		}
#endif hp9000s800
		if (alive_retries[site] == (retries_till_LBC + 1)) {
		   if ((check_lan_break(site)) == 0) {
			/* legit lan break, reset to alive */
			clustab[site].status = CL_ALIVE;
			break;
		   } /* else no lan break but card's reset, so try sending */
		}
		dux_send_alive(TRUE,site); 
	    } else {  /* hit max retries */

		/*
		** Prior to declaring the site dead, lets determine if we 
		** have a LAN break or MAU disconnect.
		*/
		err = check_lan_break( site );
		switch( err ) {
		case -1:			/* No Lan Break */
		    /* 
		    ** If site is no longer marked CL_RETRY, then some packet 
		    ** must have been received while we were messing with the 
		    ** LAN, or site has been declared dead.
		    */
		    if (clustab[site].status != CL_RETRY) break;

		    if (is_root) {
			/* declare site dead and broadcast to the world */
			declare_failed(site);
			break;
		    } 

		    else { /* client */
			/* I'm gonna die because I can't talk to someone */
			broadcast_failure();

			/* Usually it's the root site */
			if (site == root_site) {
			    dux_boot("lost contact with root server");
			} 

			/* It could be another node at clustertime though! */
			/* (NOTE:  This won't happen because the deaf site */
			/*  retry code is not implemented, but we'll leave */
			/*  it in for future use and/or reference)	   */
			else {
			    printf("Cannot contact cnode id %d\n",site);
			    panic("cannot join cluster -- cannot contact all cluster nodes");
			}
		    } /* end else I'm client */
		    break; /* never reached */

		case 0:				/* Broken Lan */
		    clustab[site].status = CL_ALIVE;
		    break;

		default:
		    panic("Unknown value returned from check_lan_break");
		}	/* end of switch on lan break result */
	    }   /* end of else hit max retries */
	}   /* end of if status == CL_REPLY */
	site++;
    } while ((++cctp < clustab+MAXSITE) && (is_root));

}  /* end of dux_retry_alive */

/*
 * Mark a site as failed, and, if we're the root server, tell the world.
 *	(note:  if this routine is called from broadcast_failure, we're
 *		running at splimp()!
 */
declare_failed(site)
site_t site;
{
	int s;

#ifdef CLEANUP_DEBUG
	/* note failure in error log */
	msg_printf("Cnode %d declared inactive in cluster\n",site);
#endif CLEANUP_DEBUG

	/* 
	** if still an active member of cluster, declare him head and
	** cleanup, if not an active member, then he's already been,
	** or is being cleaned up.  The only case in which we should
	** encounter an inactive client is when the DM_MARK_FAILED
	** is received *after* the site's been declared dead from a
	** DM_FAILURE.  This will never happen on the root site since
	** only the root site sends DM_MARK_FAILED.
	*/
	if ((clustab[site].status & CCT_STATUS_MASK) == CL_IS_MEMBER) {
	    s = splimp();
	    clustab[site].status = CL_FAILED;
	    /* 
	    ** If cleanup is already running, no need to restart it.
	    ** If failed_sites was > 0, then we've already timed out 
	    ** init_cleanup, and it will start cleanup for us.
	    */
	    if ((failed_sites++ == 0) && (!cleanup_running)) {
		splx(s);
		init_cleanup();	/* cleanup code sends DM_MARK_FAILED msg */
	    } else splx(s);
	}
}

/*
** Since init_cleanup runs under timeout and also calls dm_alloc
** it must be triggered down.
*/
init_cleanup()
{
#ifdef __hp9000s300 
	sw_trigger( &cleanup_intloc, dux_init_cleanup, 0,
		LAN_PROC_LEVEL, LAN_PROC_SUBLEVEL + 1 );
#else
    /* timeout is running under spl2() on S800, it shouldn't interrupt
     * others getting mbuf.
     */
	dux_init_cleanup();
#endif /* hp9000s300 */
}


/*
 * invoke a network server process to do cleanup operations. The cleanup
 * process will continuously looking for cleanup work to do until there is no
 * work.
 */
dux_init_cleanup()
{
	register dm_message reqp;
	register struct dm_funcentry *funcentry = dm_functions + DM_CLEANUP;
	
	/*Allocate a dummy dm msg for invoking network server process.
	 *If no mbuf is available, retry one second later.
	 *Set limited flag for nsp to 1
	 */
	if ((reqp = dm_alloc (0, DONTWAIT)) != NULL)
	{
	   DM_SOURCE (reqp) = 0;
	   if(invoke_nsp(reqp, funcentry->dm_callfunc, LIMITED_NOT_OK) != 0)
	   {
		panic("invoke_cleanup: no dux server process");
	   }

	}
	else /*no mbuf, retry one second later*/ 
		timeout (init_cleanup, (caddr_t)0, hz);
}



/*
 * Reboot behavior upon losing contact with server or being declared
 * dead is configurable through dfile with the "reboot_option" parameter.
 *
 *	reboot_option = 2  --> panic  (dump core if possible)
 *	reboot_option = 1  --> reboot
 *	reboot_option = 0  --> reboot -h (sync, shutdown, and halt)
 */
extern int reboot_option;

/*
 * The invoked network server process runs this code to cleanup resources used 
 * by the  failed sites.  Nsp is invoked as site 0.
 */
invoke_cleanup(reqp)
dm_message reqp;
{
	site_t site;
	struct cct *cctp;
	int s;	

	/* Make sure that we aren't already running cleanup! */
	s=splimp();
	if (!cleanup_running){
		cleanup_running = 1;
		splx(s);
	}
	else goto exit;
	
	/* First sleep at a priority level above the level slept at by
	 * dm_wait.  This permits processing of all replies that may have
	 * been received before the cleanup.
	 */
	sleep ((caddr_t)&lbolt, PRIDM+1);

	/*Scan the clustab[] repeatedly to look for failed sites until
	 *all failed sites are dealt with. 
	 */
	s = splimp();
	while (failed_sites > 0){
		splx(s);
		site = 1;
		for (cctp = clustab+1; cctp < clustab+MAXSITE; cctp++) {
			/*
			 * If  there are still some network server processes 
			 * running for the failed site, send a singal to kill 
			 * those processes and retry the cleanup again.
			 * Don't worry about cleanup nsp or serve_clusterreq
			 * nsp since both are owned by site "0".
	 		 */
		        if ((cctp->status == CL_FAILED) || 
			    (cctp->status == CL_CLEANUP))  {
				/*
				 * If failed site is the root server or my swap
				 * server, just abort clean up and do
				 * a normal reboot.
				 */
				if (site == root_site) {
					printf("lost contact with root server\n");
					/*
					 * Allow user to configure
					 * default behavior upon losing
					 * contact with root server.
					 * Some want it clients to reboot,
					 * others want to see the
					 * failure diagnostic left on the
					 * client's console.
					 */
					do_dux_boot();
				}
#if defined(FULLDUX) || defined(LOCAL_DISC) /* multiple swap servers really */
				if (site == swap_site) {
					printf("lost contact with swap server\n");
					do_dux_boot();
				}
#endif FULLDUX || LOCAL_DISC

				/* clean up the using array before trying to
				** kill all the nsps since nsps won't die if
				** they happen to be in dm_wait when the site
				** goes down unless we mark responses as DM_DONE
				*/
				(void)cleanup_using_array (site);
				/* 
				** if still CL_FAILED, then we haven't
				** finished DM_MARK_FAILED message yet...
				*/
				if (cctp->status == CL_FAILED) {
				    /* only sent by root server */
				    if (IM_SERVER) {
					send_mark_failed(site);
				    }
				    else {
					    cctp->status = CL_CLEANUP;
				    }
				}
				/*
				** Try to kill all csps.  Only try to
				** cleanup code if the MARK_FAILED msg
				** has been sent.
				*/
				if ((no_nsp(site)) && 
				    (cctp->status == CL_CLEANUP)) {
					u.u_site = site;
					clean_up (site);
					s = splimp();
					/* 
					** Only decrement failed sites, if
					** we really finished cleanup!
					*/
					if (cctp->status != CL_CLEANUP)
						failed_sites--;
					splx (s);
				}

			}
			site++;
		}
		/*delay for one second hoping that nsp's for the failed site
		 *will be terminated by then.
		 */ 
		sleep ((caddr_t)&lbolt, P_CLEANUP); 
		s = splimp(); /*raise priority to prevent race condition*/
	}
	/*still at priority level splimp */
	cleanup_running = 0; /*indicate cleanup nsp is gone*/
exit:	splx (s);
	/*release the dummy dmmsg which is used only for invoking nsp*/
	/* if one was allocated-it's NULL if called from prepare_new_site! */
	if (reqp != NULL) dm_release (reqp, 1);
}

/*
** SEND_MARK_FAILED:  Sends a DM_MARK_FAILED message if none has been sent
** 			yet.  If one has been sent, the checks the return
**			code in the request mbuf.  If it is DM_DONE, then
**			set the site's status to CL_CLEANUP to allow cleanup
** 			to continue.
*/
send_mark_failed(site)
site_t site;
{
    static dm_message request[MAXSITE]; /* Array of pointers to request mbufs */
					/* Must be static to retain info from */
					/* call to call & to initialize to 0  */
    struct dux_failmsg *reqdata;

    if (request[site] == NULL) {
	/* no msg has been sent, try to allocate mbuf */
	request[site] = dm_alloc(sizeof(struct dux_failmsg),DONTWAIT);
	if (request[site] == NULL) return;  /* try again later */
	else {
	    /* send message */
	    reqdata = DM_CONVERT(request[site], struct dux_failmsg);
	    reqdata->failed_site = site;
	    dm_send(request[site],DM_RELEASE_REPLY,DM_MARK_FAILED,
		    DM_CLUSTERCAST,DM_EMPTY,NULL,NULL,NULL,NULL,NULL, 
		    NULL,NULL);
	} /* end of else (dm_alloc returned non null) */
    } /* end of if NULL */

    /* we now have valid request pointer.  see if request is done */
    if ((DM_HEADER(request[site]))->dm_flags & DM_DONE) {
	/* dm_send has finished */
	dm_release(request[site],1);
	request[site] = NULL;
	clustab[site].status = CL_CLEANUP;
	return;
    } else {
	/* it's not done yet... */
	return;
    } /* end of else not done */
}

clean_up(site)
site_t site;
{
	register int i, err = 0;

	/* now, go ahead & clean site up... */
	/* We reschedule cleanup when we encounter an error, because the
	 * cleanup routines assume that the previous cleanup routines
	 * executed completely without errors.  For example, the mount
	 * table cleanup code assumes that the filesystem cleanup has
	 * completed successfully.
	 */
	for (i = 0; ((cleanuplist[i].func != NULL) && (err == 0)); i++)
	    err = ((*cleanuplist[i].func)(site));

	if (err)
	    return;
	else {
	    clustab[site].status = CL_INACTIVE;
	    wakeup ((caddr_t)&clustab[site]);
	}
}

/*
 * turn the recovery function on/off for testing purposes
 */

recovery_on()
{
	/* delay the checking so that other sites have chance to send in
	 * alive message
         */
        if (using_debugger) {
		using_debugger = FALSE;
	       	timeout (check_alive, (caddr_t)&check_alive_period, 
			check_alive_period * hz);
	       	timeout (retry_alive, (caddr_t)&retry_alive_period, 
			retry_alive_period * hz);
	}
}

recovery_off()
{
	using_debugger = TRUE;
	untimeout (check_alive, (caddr_t)&check_alive_period);
	untimeout (retry_alive, (caddr_t)&retry_alive_period);
}



/*
 *Attempt to tell the world that I have failed.  Don't bother waiting for
 *any responses, because if the message does get lost they will eventually
 *declare me as dead anyway
 */
broadcast_failure()
{
register struct mbuf *reqp;
struct dm_header *dmp;
register struct proto_header *ph;
extern struct mbuf send_alive_mbuf;
int err, s;

	/*
	** If not clustered then no sense in sending this message.
	*/
	if( my_site_status & CCT_CLUSTERED )
	{
		/*
		** Initialize the static mbuf. We are clustered.
		*/
		s = spl6();

		/* set failure_sent flag to block any incoming
		** messages. This could occur if we retry a
		** request after we braodcast failure and the
		** server sends a suicide message back
		*/
		
		failure_sent = 1;

		/*
		** Since this is called from the reboot code, we
		** will simply use the send_alive static mbuf, we won't be
		** needing it any more!
		*/
		err = init_static_mbuf( DM_EMPTY, &send_alive_mbuf );
		if( err == -1 )
		{
			splx(s);
			return;
		}

		if( (reqp =  &send_alive_mbuf) != NULL )
		{
			dmp = DM_HEADER(reqp);
			ph = &(dmp->dm_ph);
			ph->p_srcsite = my_site;

			dm_send(reqp,
			        DM_DATAGRAM,
				DM_FAILURE,
				DM_CLUSTERCAST,
				DM_EMPTY,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		}
		splx(s);
	}
	else /* Not clustered, just return */
	{
		return;
	}
	
}



/*
 * If called as op DM_FAILURE, declare calling site dead and clean it up.
 * If called as op DM_MARK_FAILED, be sure calling site is root server,
 * then send reply and cleanup site requested.  Both cases are called
 * UNDER INTERRUPT!!!
 */
record_failure(request)
dm_message request;
{
    struct dux_failmsg *reqtp;
    register site_t failed_site, src_site;
    int s;
    /* 
    ** Is this a request from root server to mark a site as failed? 
    ** Or is this a broadcast DM_FAILURE at panic time from failing node ?
    */
    src_site = DM_SOURCE(request);	/* who sent this message? */
    if (DM_OP_CODE(request) == DM_MARK_FAILED) {
	/* Get id of failing site before releasing dmmsg */
	reqtp = DM_CONVERT(request, struct dux_failmsg);
	failed_site = reqtp->failed_site;
	/* Only the root server is allowed to send this message */
	if (src_site == root_site) {
	    dm_quick_reply(0);
	} else { 		/* requesting site not root server */
	    msg_printf("Non-root server cnode id %d attempted to mark failed\n",src_site);
	    /* do quick reply to release mbuf--no return code check */
	    /* this should never happen! */
	    dm_quick_reply(0);
	    return;
	}
    } else if (DM_OP_CODE(request) == DM_FAILURE) {
	    failed_site = src_site;  /* DM_FAILURE,get source site */
	    /* don't need reply for DM_FAILURE, so need to release mbuf */
	    dm_release(request, 0);  
	    /* If I'm not clustered yet, blow this datagram off! */
	    if (!(my_site_status & CCT_CLUSTERED)) return;
    } else panic("unknown op in record_failure");

    /* raise priority till after declared failed so no DM_ADD_MEMBER recd */
    s=splimp();

    /* don't do anything if site id is invalid */
    if ((failed_site < 1) || (failed_site > MAXSITE)) {
	splx(s);
	return;
    }

    /* redundancy check -- can't record our own failure */
    if (failed_site == my_site) {
	/* if were the root site and somebody's declaring us dead */
  	if (my_site == root_site) {
	    /* blow it off */
#ifdef CLEANUP_DEBUG
	    msg_printf("Site %d sent this node (root) a FAILURE packet!\n",
								    src_site);
#endif CLEANUP_DEBUG
	    splx(s);
	    return;
	}   /* if root_site */
	else {
	    /* if we're not root, might as well panic, since whole cluster 
	    ** will declare me dead anyway!
	    */
	    if (src_site == root_site)
	    {
		dux_boot("No longer a valid cnode.  Declared dead by root server.");
		splx(s);
		return;
	    }
	    else
	      panic ("Declared dead by unexpected failure packet.");
	}   /* else not root_site */
    }   /* if my_site */

    declare_failed (failed_site);
    splx(s);
}


/*
** The following "dux_boot" code is used so that we can call boot() when we
** receive some dux message that makes it so that we are no longer a valid
** cnode.  This can occur because we received a suicide packet or because
** the recovery code (under timeout) has determined that we can't talk to
** the root server, but the lan subsystem is working fine.  In both cases,
** the determination that we should reboot occur while we are on the ICS.
** If we call boot() straight away, then any local discs will not the cleaned
** up, thus causing an fsck when reboot.  Therefore, we have the limited nsp
** call boot() for us and then we can sync or disc(s).
*/


/*
**				INIT_DUX_BOOT
**
** This routine initializes the static mbuf that is used to call the limited
** nsp to reboot us when we are no longer a valid cluster member either
** because we are declared dead or because we lose contact with the root
** server.
*/
init_dux_boot()
{
    register struct mbuf *reqp;
    register struct proto_header *ph;

    /*
    ** Initialize the static mbuf.
    */

    if ((init_static_mbuf(DM_EMPTY, &dux_boot_mbuf) != -1) &&
	((reqp = &dux_boot_mbuf) != 0))
    {
	ph = &(DM_HEADER(reqp)->dm_ph);
	ph->p_flags = P_DATAGRAM;
    }
    else
    {
	/*
	** We should always have this mbuf available and
	** initialized. Something has stomped on this mbuf.
	*/
	panic("Couldn't initialize static dux_boot_mbuf");
    }
}


/*
**				DO_DUX_BOOT
**
** Call boot based upon the configurable variable "reboot_option".  See
** comment about "reboot_option" above.
*/
do_dux_boot()
{
    if (reboot_option == 1)
	boot(RB_BOOT, 0);
    else if (reboot_option == 0)
	boot(RB_HALT, 0);
    else
	panic("Diskless panic");
}


/*
**			       	  DUX_BOOT
**
** If we are on the ICS, schedule the limited nsp to call boot() in a user
** context.  If dux_boot() is called repeatedly, it is likely that we weren't
** successful in invoking the limited nsp, therefore on the third call,
** we call do_dux_boot() directly.
*/
dux_boot(s)
char *s;
{
    static int dux_boot_called = 0;

    printf("%s\n", s);

    if (!ON_ICS || ((++dux_boot_called) > 2))
    {
	do_dux_boot();
    }
    else
    {
        /*
        ** Set free_nsps to 0 to force the limited nsp to execute us.
        */
        free_nsps = 0;

        invoke_nsp(&dux_boot_mbuf, do_dux_boot, LIMITED_OK);
    }
}
