/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/drv_lan0.c,v $
 * $Revision: 1.7.84.5 $	$Author: randyc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/07/11 13:41:02 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */

#if  defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) drv_lan0.c $Revision: 1.7.84.5 $ $Date: 94/07/11 13:41:02 $";
#endif


/*
 * HPUX Ethernet Communications Controller interface
 */

#ifndef	TEMP_4.3_KLUDGE /* temprarily putting this stuff in --
                                notice if_n_def !  (skumar 7/27)*/
#define      NS_ASSERT(cond, string) if(!(cond)) panic((string))

#endif	/* TEMP_4.3_KLUDGE */
#include "../h/param.h"
#include "../h/domain.h"

#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/protosw.h"

#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/mbuf.h"

#include "../h/uio.h"
#include "../h/user.h"
#include "../h/file.h"
#undef u
#ifndef udot
#define udot u
#endif
#include "../h/proc.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/if_ether.h"
#include "../netinet/if_ieee.h"

#include "../s200io/lnatypes.h"    /* leafnode related declarations ..prabha */
#include "../net/raw_cb.h"

#include "../sio/ectest.h"
#include "../sio/netio.h"
#include "../wsio/timeout.h"
#include "../wsio/intrpt.h"
#include "../wsio/hpibio.h"
#include "../s200io/drvmac.h"     /* for drv macros & structures             */
#include "../s200io/drvhw.h"      /* for drvhw constants & structures        */
#include "../sio/lanc.h"
#include "../s200io/drvhw_ift.h"
#include "../h/malloc.h"

#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"

#ifdef LAN_MS
#include "../h/ms_macros.h"
#endif /* LAN_MS */

#include "../netinet/mib_kern.h"

void     drv_lan_dead();  /* sets LAN_DEAD and clears IFF_UP flags, */
                          /* and generates a MIB LINKDOWN event */

extern lan_ift * lan_dio_ift_ptr[];
extern int	num_lan_cards;  /* user's count of lan0_cards        */

/* global defining number of valid lanift entries                    */
int	toplu = -1; 		/* 'lu' takes values from 0 to toplu */

int drv_ifq_max; /* maximum size of output queue on 300 (all lan cards) */

extern 			bcopy();
extern 			spl6();
extern 			splimp();
extern void             splx();

extern void             schednetisr();
extern struct mbuf 	*m_get(), *m_getclr();
extern void             m_freem();

extern error_number     hw_read_local_address();
extern error_number     hw_read_stat();
extern error_number     hw_reset_card();
extern error_number     hw_reset_stat();
extern error_number     hw_self_test ();
extern error_number     hw_send();
extern error_number     hw_startup();

#ifdef LAN_MS
/* measurement system variables */
extern  int     ms_lan_read_queue;
extern  int     ms_lan_proto_return;
extern  int     ms_lan_netisr_recv;
extern  int     ms_lan_write_queue;
extern  int     ms_lan_write_complete;
#endif /* LAN_MS */

#ifdef DRVR_WB_TEST

/* WHITE_BOX_TEST TRIGGERS **************************************************
 *
 * Below , a trigger is defined for each procedure in this driver. The initial
 * values of the triggers are also set. The trigger flags that follow control
 * the corresponding triggers. The trigger is active iff the flag is set. By
 * writing to the /dev/kmem file from the user land, we can turn the triggers
 * on and off to create varied stress conditions. The frequency of the trigger
 * is controlled by writing into the variable immediately below (CAPITALS).
 *                                                ........prabha 02/09/87 */

/* DRIVER_WHITE_BOX_TEST trigger definitions ...pgc */

int	FIND_ADDRESS_COUNT      = 10000;
int	ADD_MULTICAST_COUNT     = 30;
int	DEL_MULTICAST_COUNT     = 30;
int	ENABLE_BCAST_COUNT      = 5;
int	DISABLE_BCAST_COUNT     = 5;
int	BCAST_STATE_COUNT       = 10;
int	DRV_RESTART_COUNT       = 15;
int	DRV_INIT_COUNT          = 2;
int	GET_BUFFER_COUNT        = 500;

/* the following TRIGGER COUNTS are set/reset externally by reading/writing /dev
/kmem to create stress conditions in the driver for testing.The corresponding
flags defined immediately below must be set for the trigger to be ON...prabha */

int	find_address_count;
int	add_multicast_count;
int	del_multicast_count;
int	enable_bcast_count;
int	disable_bcast_count;
int	bcast_state_count;
int	drv_restart_count;
int	drv_init_count;
int	get_buffer_count;

/* these following TRIGGER FLAGS are used to turn triggers on and off...prabha*/
/* all flags are reset - inactive - at  start. Turn them ON/OFF from user land*/

int	find_address_tflag      = 0;
int	add_multicast_tflag     = 0;
int	del_multicast_tflag     = 0;
int	enable_bcast_tflag      = 0;
int	disable_bcast_tflag     = 0;
int	bcast_state_tflag       = 0;
int	drv_restart_tflag       = 0;
int	drv_init_tflag          = 0;
int	get_buffer_tflag        = 0;
#endif /* DRVR_WB_TEST */

/*
 * 	HP-UX IEEE 802.3 and Ethernet interface
 */

/* *** structure to store logged saps and types for lan link *** */
struct logged_link *log_hash_ptr[1<<HASHBITS];


/*
 * these will change when the changes for integration w/lan0 are done
 */

error_number drv_enable_broadcast();
error_number drv_disable_broadcast();
error_number drv_restart();
error_number drv_init();

int	get_buffer();

int	lanift_init();

int	drv_hw_req();
int	drv_build_hdr();


/*
 * NOTE:
 * Each interface is referenced by a network interface structure,
 * (an ifnet structure), which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * In this driver the output q is NOT used because asynch i/o facilities don't
 * exist. We have one lan_ift structure for each interface card, and it contains
 * an ifnet structure in the arpcom struct (is_if) associated with that card.
 * The lanift structure (an array of lan_ift's) is declared  in drv_lan0.h
 * and is allocated in space.h for user configurability.
 */

/***************** FUNCTION : drv_enable_broadcast **********************
 *
 * [OVERVIEW]
 *      This procedure enables broadcast reception.
 *
 * [ERRORS]
 *      <none> implemented
 *
 * [GLOBAL VARIABLES MODIFIED]
 *      broadcast_state
 *
 * [GLOBAL VARIABLES EXAMINED]
 *      broadcast_state
 *
 * called by: lan0_control (thru' lanc_dda_ioctl).
 *
 ************************************************************************/

error_number
drv_enable_broadcast( my_global )
lan_gloptr    my_global;

{




#ifdef DRVR_WB_TEST
	if (enable_bcast_tflag) {
		if (--enable_bcast_count   < 0) {
			enable_bcast_count   =  ENABLE_BCAST_COUNT;
			return(E_FORCED_ERROR);
		}
	}
#endif /* DRVR_WB_TEST */


/*----------------------------- ALGORITHM -------------------------------
 *      -- call hw_update_broadcast only if the state changed
 *      if broadcast_state != TRUE
 *         broadcast_state := TRUE
 *         <hw_update_broadcast> (broadcast_state)
 *      return (E_NO_ERROR)
 *---------------------------------------------------------------------*/


	if ( !my_global->broadcast_state ) {

		/*
	 * the intent of the conditional call to hw_update_broadcast
	 * is that hw_update_broadcast might have to shut down the
	 * receiver and the frequency of doing this should be miminizied
	 */

		my_global->broadcast_state = TRUE;
	}
	return( E_NO_ERROR );

}


/***************** FUNCTION : drv_disable_broadcast *********************
 *
 * [OVERVIEW]
 *      This procedure disables broadcast reception.
 *
 * [ERRORS]
 *      <none> implemented
 *
 * [GLOBAL VARIABLES MODIFIED]
 *      broadcast_state
 *
 * [GLOBAL VARIABLES EXAMINED]
 *      broadcast_state
 *
 * called by: lan0_control (thru' lanc_dda_ioctl).
 *
 ************************************************************************/
error_number
drv_disable_broadcast( my_global )
lan_gloptr    my_global;

{



#ifdef DRVR_WB_TEST
	if (disable_bcast_tflag) {
		if (--disable_bcast_count  < 0) {
			disable_bcast_count  =  DISABLE_BCAST_COUNT;
			return(E_FORCED_ERROR);
		}
	}
#endif /* DRVR_WB_TEST */

/*----------------------------- ALGORITHM -------------------------------
 *      -- call hw_update_broadcast only if the state changed
 *      if broadcast_state != FALSE
 *         broadcast_state := FALSE;
 *         <hw_update_broadcast> (broadcast_state)
 *      return (E_NO_ERROR)
 *---------------------------------------------------------------------*/


	if ( my_global->broadcast_state ) {

		/*
	 * the intent of the conditional call to hw_update_broadcast
	 * is that hw_update_broadcast might have to shut down the
	 * receiver and the frequency of doing this should be miminizied
	 */

		my_global->broadcast_state = FALSE;
	}
	return( E_NO_ERROR );

}


/***************** FUNCTION : drv_read_perm_address ********************
 *
 * [OVERVIEW]
 *      This function returns in the variable ret_bufer the NOVRAM link
 *	level address burned into the lan interface card.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *      <none>
 *
 * [GLOBAL VARIABLES EXAMINED]
 *      my_global
 *
 * [RETURN VARIABLES]
 *	ret_buffer which contains a pointer to data_ptr->value.s.
 *
 * called by: lanc_dda_status in the file lanc_dda.c
 *
 ************************************************************************/

error_number
drv_read_perm_address( my_global, ret_buffer)
lan_gloptr my_global;
char *ret_buffer;

{
	link_address	addr;
	error_number	error = E_NO_ERROR;

	error = hw_read_local_address(&my_global->hwvars, addr);  /* get addr */
	if (error != E_NO_ERROR) { /* error */
		NS_LOG_INFO(LE_LAN_READ_NOVRAM_FAILURE, NS_LC_DISASTER,
			 NS_LS_DRIVER, 0, 1,
			 ((lan_ift *) (my_global->lan_ift_ptr))->is_if.if_unit,
			 0);
		drv_lan_dead (my_global);
	}
	else   /* copy address into return buffer */
		bcopy(addr, ret_buffer, sizeof(addr));
	return(error);
}


/***************** FUNCTION : drv_restart *******************************
 * [OVERVIEW]
 *      This procedure does an immediate reset to the LAN card and
 *      performs a self-test.  Restart is a destructive reset of
 *      the card.  However, the statistics will NOT be reset.
 *      The receiver is enabled after the restart.
 *
 *      The self-test will identify if the card is working correctly.
 *      It will not identify problems with external MAUs or network
 *      problems.
 *
 * [ERRORS]
 *      hw_self_test errors
 *      hw_startup errors
 *
 * [GLOBAL VARIABLES MODIFIED]
 *
 *
 * [GLOBAL VARIABLES EXAMINED]
 *      local_link_address
 *      broadcast_state
 *
 * called by: drv_init (initialisation), lanc_init (thru' drv_restart for ioctl)
 *
 ************************************************************************/

error_number
drv_restart( my_global, action )
register lan_gloptr   my_global;
int      action;

{

    /*-------------------------- LOCAL VARIABLES ------------------
     *      err       --  keeps the last error number returned
     *      s         -- save variable for interrupt level;
     *-----------------------------------------------------------*/

	error_number err = E_NO_ERROR;
	int	s, i;



#ifdef DRVR_WB_TEST
	if (drv_restart_tflag) {
		if (--drv_restart_count    < 0) {
			drv_restart_count    =  DRV_RESTART_COUNT;
			return(E_FORCED_ERROR);
		}
	}
#endif /* DRVR_WB_TEST */


    /*----------------------------- ALGORITHM --------------------
     *          <block interrupts>
     *          err = <hw_self_test>
     *          IF err == E_NO_ERROR THEN
     *               <hw_update_multicast>
     *               <hw_update_broadcast>
     *              err = <hw_startup> (& local_link_address)
     *          <unblock interrupts>
     *      return(err)
     *----------------------------------------------------------*/

/* Turned off interrupts during initialization to keep other parts
 * of the system from attempting to access the card and to keep
 * card from interrupting at an inopportune time.  Most code in
 * the system that calls this routine already disables interrupts.
 * However, some does not, and it should be done here so that future
 * callers of drv_restart do not have to remember to disable interrupts
 * (esp. callers of ifp->if_init).  Spl6 is used instead of splimp,
 * because this is being called at system powerup time, and it is
 * not clear whether it is safe to lower interrupt level from spl6
 * at this point in time.  The only cost to doing this is that the
 * system will be locked up during the reset, which can be at most
 * about 1.5 seconds.  This is not a real problem, since the reset
 * can only be done by superuser, and is not likely to be done real
 * frequently.
 */

	s = spl6();
	if ( (err = hw_self_test(&my_global->hwvars)) == E_NO_ERROR ) {
		err = hw_startup(&my_global->hwvars, my_global->local_link_address );
		if (err != E_NO_ERROR){
		      /* then hw_startup failed! mark card dead */
		      NS_LOG_INFO( LE_LAN_EVENT_DOD,
			  NS_LC_DISASTER, NS_LS_DRIVER, 0, 2,
			  ((lan_ift *) (my_global->lan_ift_ptr))->is_if.if_unit,
			  &my_global->hwvars.card_state);
		      drv_lan_dead (my_global);
		}
	} else  /* if hw_self_test failed, then */ {
		NS_LOG_INFO( LE_LAN_EVENT_STF, NS_LC_DISASTER, NS_LS_DRIVER, 0,
			  1,
		          ((lan_ift *) (my_global->lan_ift_ptr))->is_if.if_unit,
			  0);
		drv_lan_dead (my_global);
	}
	if (err == E_NO_ERROR && action == RESET_STATISTICS) {
		drv_save_stats ((lan_ift *)(my_global->lan_ift_ptr));
		for (i = START_STAT_VAL; i <= END_STAT_VAL; i++)
		    reset_hw_stat ((lan_ift *)(my_global->lan_ift_ptr), i);
        }

        /* -=- fix for DTS # INDaa09052 -=- */
        /* only turn on IFF_UP if we turned it off */
        if (err == E_NO_ERROR) {
           ((lan_ift * )(my_global->lan_ift_ptr))->hdw_state &= ~LAN_DEAD;
           if(my_global->hwvars.reset_iffup == TRUE) {
              ((struct ifnet *)(my_global->lan_ift_ptr))->if_flags |= IFF_UP;
              my_global->hwvars.reset_iffup = FALSE;
           }
        }

	splx(s);
	return(err);
}




/***************** FUNCTION : drv_init **********************************
 * [OVERVIEW]
 *      This routine initializes the driver and sets up the card
 *      to receive frames from the network.  The routine also
 *      initializes statistics.  After initialization is complete
 *      the driver will be in the following state:
 *
 *          o Incoming broadcast frames enabled.
 *          o Incoming multicast frames disabled.
 *          o Receiver enabled.
 *
 * [INPUTS]
 *      varies with different OS
 *
 * [GLOBAL VARIABLES MODIFIED]
 *      multicast list structure, broadcast_state.
 *      Protocol Globals record entry points.
 *
 * [GLOBAL VARIABLES ACCESSED]
 *      <None>
 *
 * called by: lanift_init at power up initialisation
 *
 ************************************************************************/

error_number
drv_init(my_global, card_location, new_address )
register lan_gloptr my_global;

word            card_location;  /* same as  my_isc in isc for the card */
link_address    new_address;

{

    /*-------------------------- LOCAL VARIABLES ------------------
     *      my_global  --  pointer to the driver's global area
     *      err       --  keeps the last error number returned
     *      i         --  indexes through the multicast entries
     *------------------------------------------------------------*/

	error_number err1 = E_NO_ERROR;
	error_number err2 = E_NO_ERROR;



#ifdef DRVR_WB_TEST
	if (drv_init_tflag) {
		if (--drv_init_count       < 0) {
			drv_init_count       =  DRV_INIT_COUNT;
			return(E_FORCED_ERROR);
		}
	}
#endif /* DRVR_WB_TEST */

	/*----------------------------- ALGORITHM ------------------
     *      if (new_address not individual)
     *          return(E_NOT_INDIVIDUAL);
     *      -- initialize global variables
     *      initialize broadcast
     *      initialize multicast, build free list
     *
     *      initialize inbound and outbound lists
     *
     *      initialize protocol global record entry points
     *
     *
     *      -- hw init
     *      err1 = <hw_reset_card>
     *
     *      if new_address == NIL then
     *          err2 = <hw_read_local_address> (& local_link_address)
     *          if err2 != E_NO_ERROR then
     *             return(err2)
     *      else
     *          COPY_ADDRESS( new_address, local_link_address )
     *
     *      err2 = <drv_restart>
     *      return (error, whichever is != 0)
     *--------------------------------------------------------*/

	if (new_address != NIL && !INDIVIDUAL(new_address)) {
		return(E_NOT_INDIVIDUAL);
	}
	/* set backptr in hw_global area in my_global area...prabha 01/28/87 */
	my_global->hwvars.drvglob = (anyptr) my_global;
	/* initialize broadcast */
	my_global->broadcast_state = TRUE;

        /* -=- fix for DTS # INDaa09052 -=- */
        /* initialize reset_iffup */
        my_global->hwvars.reset_iffup = FALSE;

        /* initialise the count of multicast addresses in the list so far */
        ((lan_ift * ) (my_global->lan_ift_ptr))->num_multicast_addr = 0;

	err1 = hw_reset_card( my_global, card_location );

	if (err1 != E_NO_ERROR) {
        /* mark card dead, but can't return until address is copied */
#ifdef DEBUGONLY
		printf("\ndrv_init: error in hw_reset_card; ");
#endif /* DEBUGONLY */
		NS_LOG_INFO( LE_LAN_EVENT_DOD,
			  NS_LC_DISASTER, NS_LS_DRIVER, 0, 2,
			  ((lan_ift *) (my_global->lan_ift_ptr))->is_if.if_unit,
			  &my_global->hwvars.card_state);
		drv_lan_dead (my_global);
	}

	if (new_address == NIL) {
           /* copy card adrress into the local link address in my_global */
		err2 = hw_read_local_address(&my_global->hwvars,
		    my_global->local_link_address );
		if (err2 != E_NO_ERROR) {
#ifdef DEBUGONLY
		    printf("\ndrv_init: error in hw_read_local_address; ");
#endif /* DEBUGONLY */
		    NS_LOG_INFO(LE_LAN_READ_NOVRAM_FAILURE, NS_LC_DISASTER,
			 NS_LS_DRIVER, 0, 1,
			 ((lan_ift *) (my_global->lan_ift_ptr))->is_if.if_unit,
			 0);
                    drv_lan_dead (my_global);
		    return( err2 );
		}
	} else      {
		COPY_ADDRESS( new_address, my_global->local_link_address );
	}
        /* if hw_reset_card had ben in error, return now */
        if (err1 != E_NO_ERROR)
            return(err1);

        /* if we get here err1 & err2 both equal E_NO_ERROR */

	err1 = drv_restart (my_global, RESET_STATISTICS);

	if (err1 != E_NO_ERROR )  {
#ifdef DEBUGONLY
		printf("\ndrv_init: error in drv_restart; ");
#endif /* DEBUGONLY */
		NS_LOG_INFO( LE_LAN_EVENT_DOD,
			  NS_LC_DISASTER, NS_LS_DRIVER, 0, 2,
			  ((lan_ift *) (my_global->lan_ift_ptr))->is_if.if_unit,
			  &my_global->hwvars.card_state);
                drv_lan_dead (my_global);
	}
        if (err1 == E_NO_ERROR) /* then everything is ok, card is good */
		((lan_ift * )(my_global->lan_ift_ptr))->hdw_state &= ~LAN_DEAD;

	return(err1); /* return error from drv_restart, err2 == E_NO_ERROR */

}


/* BEGIN_IMS get_buffer
******************************************************************************
****
****		get_buffer
****
******************************************************************************
* Description
*    This routine is called by isr_proc/he_loopback when the card has a buffer
*    on board to allocate an mbuf and the cluster(s) for reading in input.
*
*    A mbuf is first allocated.
*    if the data size is more than one mbuf full, then another with a cluster
*    is linked to the first. In case the cluster is not available, a chain of
*    mbufs to contain the data is allocated.
*    If there is an error at any point, the buffers
*    obtained (if any) are returned and the routine returns error.
*
* TO DO:
*
*
* INPUT PARAMETERS
* is ->pointer to the lan_ift struct where the mbufs are to be attached;
* size - minimum number of bytes required to be allocated.
*
* Output Parameters
*
* m - pointer to the mbuf chain.
*
* Return Value
*
* Externally Callable Routines
*
* Called By
* hw routines isr_proc and hw_loopback.
*
* Test Module
*
* To Do List
*
* Notes
*
* Modification History
*
******************************************************************************
* END_IMS  Module Name */

get_buffer(is, size, m)
register lan_ift       *is;
register int	size;
register struct mbuf **m;
{
 register struct mbuf *m1;

/* LAN_MTU+7 is the max size expected now -as of 4/29/87; strictly a QA_check */
NS_ASSERT(((MLEN+MCLBYTES) >= LAN_MTU+8),"qa_check: pkt more than msg size?");

 *m = m_get(M_DONTWAIT, MT_DATA);
 if (*m == 0 )
    {
     return(ENOBUFS);
    }
 (*m)->m_len = 0;

 /* if one mbuf is too small, get another */
 if (size > MLEN)
    {
     m1 = m_get(M_DONTWAIT, MT_DATA);
     if ( m1 == 0)
	{
	 m_freem(*m);
	 return(ENOBUFS);
	}

     (*m)->m_next = m1;
     m1->m_len  = 0;

     /* if two mbufs are too small, make second one
	a cluster. Assume that one mbuf + one cluster
	will always be enough to handle the data...pcn*/
     if (size > (2 * MLEN)) {
	 MCLGETVAR_DONTWAIT(m1, ETHERMTU);
	 if (!M_HASCL(m1)) { /* can't get cluster -- get more mbufs */
	     size -= 2 * MLEN;
	     m1->m_len = 0;
	     while (size > 0) {
		 m1->m_next = m_get(M_DONTWAIT, MT_DATA);
		 if ((m1 = m1->m_next) == 0) {
		     m_freem(*m);
		     return(ENOBUFS);
	    }
		 m1->m_len  = 0;
		 size -= MLEN;
		}
	    } else m1->m_len = 0; /* set cluster's length to zero */
	}
    }
 return(0);
}



/* BEGIN_IMS lla_open *
********************************************************************
****							        ****
****		                lla_open(dev, flag)             ****
****							        ****
********************************************************************
* Input Parameters:
*                       dev - Major/Minor number
*			flag - filw operation flags
*
* Output Parameters:	(none)
*
* Description
*	Opens a device for direct driver access.
*	The indicated device will be either Ethernet or IEEE 802.3 only.
*	After acquiring the file pointer fp, the file operation table
*	is replaced with a lla operation table so that subsequent
*	read/write/ioctl/select/close requests will go through
*	lla code.
*
*
* Return Values
*	ENXIO		: device for non-existent lu or protocol
*	EPROTONOSUPPORT : protocol not supported
*	ENOBUFS		: no mbuf available for sockaddr structure
*
* Globals Referenced:
*	User structure, global file descriptors, user file descriptors.
*
* Algorithm
*	if unit number invalid return ENXIO
*	return any error value
*
* To Do List:
*
********************************************************************
* END_IMS socket */


#define LANd_PROTOCOL(a)  ((a >> 24) - DIO_IEEE802)
#define LANd_SC(a)        ((a >> 16) & 0xff)
extern int    toplu;

lla_open(dev, flag)
int  dev;
int  flag;

{  /* Beginning of lla_open function */
/******************
 * Local variables.
 ******************/

struct	ifnet           *ifnet_ptr;
int	lan_unit;                         /* result to user */
word	select_code;


if (udot.u_nsp)
  { /* Remote 'open' requests not allowed; this code gets into */
    /* the u area which is not valid for a remote request.     */
  return(ENXIO);
  }
select_code = LANd_SC(dev);
for (lan_unit = 0; lan_unit <= toplu; lan_unit++)
  { /* for loop to search for select code */
  if (select_code == ((landrv_ift *)lan_dio_ift_ptr[lan_unit])->lan_vars.hwvars.select_code)
    break;
  }
if (lan_unit > toplu)
  return (ENXIO);

if (!(flag & FREAD))
   return (EINVAL);

if ((~(FREAD | FWRITE | FNDELAY) & flag) != 0)
   return (EINVAL);

if ((LANd_PROTOCOL(dev) != LAN0_PROTOCOL_ETHER) &&
    (LANd_PROTOCOL(dev) != LAN0_PROTOCOL_IEEE))
   return (ENXIO);


ifnet_ptr = (struct ifnet *) lan_dio_ift_ptr[lan_unit];

return (lanc_lla_open(ifnet_ptr, dev, flag));  /* call common open function */

}  /* Ending of lla_open function */


/****************************************************************************
 *
 *                reset_hw_stat ( lu, stat )
 *
 *   Description: call hw_reset_stat with the same params & map errors to EINVAL
 *
 *   input:  lu:        - logical unit on which to reset stat
 *           stat       - statistic to reset
 *
 *   return: error, mapped to EINVAL
 *
 *   called by : lan0_control. LINK LEVEL ACCESS needs the error mapping
 *
 ***********************************************************************/
reset_hw_stat( is, stat )
register lan_ift       *is;
int	stat;
{
	int	error = 0;
	int	lu = is->is_if.if_unit;

	NS_ASSERT (((lu >= 0) && (lu <= toplu)), "reset_hw_stat: lu out of bounds");

	error = hw_reset_stat (is, stat);
	if (error)
		error = EINVAL; /* map to EINVAL */

	return ( error );
}
/************************** end reset_hw_stat ******************************/


/******************* FUNCTION lanift_init *********************************
 *
 * [OVERVIEW]
 *           This function is called at initialisation by the system. As
 * it encounters a card (of type LAN) it calls this routine with the isc.
 * The isc contains the card_location (select code). In here we will initialise
 * the lan_ift itself, the ifnets in it, attach the ifnets to a global list,
 * attach the arpcom (in lan_ift) to it's list, initialise the input Q and the
 * vquad list, initialise the lan_global area and the hw_globals area. In short,
 * we do most of the driver initialisation here. The lu for each card also is
 * alloacted here.
 *
 * [INPUT]
 *      isc    -- the driver interface space structure, filled in partly by
 *                the s200 initialisation  code.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *
 *
 * [GLOBAL VARIABLES EXAMINED]
 *
 * called by: kernel initialise routines at power up.
 *
 ************************************************************************/

lanift_init(isc)
struct isc_table_type *isc;
{



	error_number err;
	error_number err1 = 0;
	error_number err2 = 0;
	lan_ift *is;
        landrv_ift *dis;
	mib_xEntry *mxe_ptr;
	register struct ifnet *ifp;

	/* limit cards to MAX_LAN_CARDS */
	if (num_lan_cards > MAX_LAN_CARDS) {
		num_lan_cards = MAX_LAN_CARDS;
		printf("Max # of LAN cards limited to %d\n", num_lan_cards);
	}
	/* isc->my_isc contains the card location */
	if (++toplu > num_lan_cards - 1) {
	    --toplu; /* decrement it  so system doesn't panic */
	    printf ("SYSTEM CONFIGURED FOR ONLY %d CARD(S)\nLAN-CARD WITH SEL CODE %d IGNORED\n", num_lan_cards, isc->my_isc);
	    return(ENXIO);
	}
#ifdef DRVR_WB_TEST
	if (toplu == 0)
		init_drv_trig(); /* call only once */
#endif /* DRVR_WB_TEST */

#ifdef LAN_MS
    /* login the measurement system ids                                 */

    if (ms_lan_read_queue ==-2)
       ms_lan_read_queue = ms_grab_id("lan_read_queue: timeval, count, unit, protocol, addr");
    if (ms_lan_proto_return == -2)
        ms_lan_proto_return = ms_grab_id("lan_proto_return: timeval, count, unit, protocol, addr");
    if (ms_lan_netisr_recv == -2)
        ms_lan_netisr_recv = ms_grab_id("lan_netisr_recv: timeval, count, unit,protocol, addr");
    if (ms_lan_write_queue == -2)
       ms_lan_write_queue = ms_grab_id("lan_write_queue: timeval, count, unit, protocol, addr");
    if (ms_lan_write_complete == -2)
        ms_lan_write_complete = ms_grab_id("lan_write_complete: timeval, count, unit, protocol, addr");
#endif /* LAN_MS */

	/* get lan_ift corresponding to toplu */
	MALLOC(dis,landrv_ift *,sizeof(landrv_ift),M_DYNAMIC,M_WAITOK);
	bzero(dis,sizeof(landrv_ift));
	is=(lan_ift *)dis;
	lan_dio_ift_ptr[toplu]=is;
	MALLOC(mxe_ptr,mib_xEntry *,sizeof(mib_xEntry),M_DYNAMIC,M_WAITOK);
	bzero(mxe_ptr,sizeof(mib_xEntry));
	is->mib_xstats_ptr=mxe_ptr;
	is->mjr_num = 18;        /* use IEEE number; Ethernet is 19 */

	is->hw_req=drv_hw_req;
	is->lantimer.flag=0;
	is->broadcast_filter=1;
	is->multicast_filter=1;
        dis->lan_vars.local_link_address = &(is->station_addr[0]);

	/* attach  lan_ift & associated strucures */
	lanc_attach(lan_dio_ift_ptr[toplu],toplu,ETHERMTU,ACT_8023,ACF_ETHER);

	is->is_ac.ac_build_hdr = drv_build_hdr;
	is->af_supported = (1<<AF_UNSPEC) | (1<<AF_INET) | (1<<AF_NS);
#ifdef APPLETALK
	is->af_supported |= (1<<AF_APPLETALK);
#endif

	/* set the lu additionally in lan_ift for the hw driver */
	is->lu = toplu;

	/* set up the back pointer in the my_globals area */
	dis->lan_vars.lan_ift_ptr = (anyptr) is;

	/* initialize the sap list */
	lanc_init_log_struct();

	is->is_scaninterval = LANWATCHINTERVAL;

	/* now initilise the associated hwvars & lan_global areas */
	/* could return error because hw_reset_card failed, or
         hw_read_local_address failed or because hw_restart failed. In
         all cases, print an error message and leave all associated
         structures initialised. Mark the card DOWN..prabha-01/21/87 */

	is->hdw_state &= ~LAN_DEAD; /* assume card is okay */
	err1 = drv_init (&(dis->lan_vars), isc->my_isc, NIL);
	if (err1) {
#ifdef DEBUGONLY
		printf("\nerror in drv_init: CARD %d; SEL CODE = %d\n", toplu, isc->my_isc);
#endif /* DEBUGONLY */
	}
        ifp = &is->is_if;
	/* initialise the ifnets; Mark IFF_IEEE, IFF_ETHER & IFF_ROUTE down */
#ifdef	XXX
        ifp->if_flags &= ~(IFF_ETHER | IFF_IEEE | IFF_ROUTE);
#endif /* XXX */
	err2 = lanc_init(ifp);
        /* if no error, then mark IFF_ETHER and IFF_IEEE up */
#ifdef	XXX
        if (err2 == 0)
	    ifp->if_flags |= (IFF_ETHER | IFF_IEEE);
#endif /* XXX */

	/* now copy link level address into the arpcom struct */
	bcopy ( dis->lan_vars.local_link_address,
	    is->is_addr, sizeof(is->is_addr));

	/* return an error that is != 0; ...prabha 01/28/87 */
	err =   (err1 != 0) ? err1 : err2;
	if (err != 0)
		printf("BAD/MISSING LAN HARDWARE @ SEL CODE %d\n", isc->my_isc);
#ifdef DRVR_TEST
	else
		printf("lan-card %d, sel code = %d initialised\n", toplu, isc->my_isc);
#endif /* DRVR_TEST */
	return(err);
} /* lanift_init */



void
drv_lan_dead (my_global)
lan_gloptr   my_global;
{
    lanc_mib_event ((lan_ift *) (my_global->lan_ift_ptr), NMV_LINKDOWN);
    ((lan_ift *) (my_global->lan_ift_ptr))->hdw_state |= LAN_DEAD;

    /* -=- fix for DTS # INDaa9052 -=- */
    /* only turn off IFF_UP if if it is currently up */
    if(((struct ifnet *)(my_global->lan_ift_ptr))->if_flags & IFF_UP) {
        ((struct ifnet *)(my_global->lan_ift_ptr))->if_flags &= ~IFF_UP;
        my_global->hwvars.reset_iffup = TRUE;
    }
}


#ifdef DRVR_WB_TEST
/*****************************************************************
 * This routine initialises the trigger variables for white box test.
 * This is QA code required to stress the system.
 *
 * inputs : none;
 * outputs: none;
 *****************************************************************/
init_drv_trig()
{  /* initialise all trigger counts */
	find_address_count   =  FIND_ADDRESS_COUNT;
	add_multicast_count  =  ADD_MULTICAST_COUNT;
	del_multicast_count  =  DEL_MULTICAST_COUNT;
	enable_bcast_count   =  ENABLE_BCAST_COUNT;
	disable_bcast_count  =  DISABLE_BCAST_COUNT;
	bcast_state_count    =  BCAST_STATE_COUNT;
	drv_restart_count    =  DRV_RESTART_COUNT;
	drv_init_count       =  DRV_INIT_COUNT;
	get_buffer_count     =  GET_BUFFER_COUNT;
	lla_open_count      =   LLA_OPEN_COUNT;
	lanc_dda_ioctl_count     =  LAN0_IOCTL_COUNT;
	lan0_control_count   =  LAN0_CONTROL_COUNT;
	lan0_status_count    =  LAN0_STATUS_COUNT;
	lan0_setaddr_count   =  LAN0_SETADDR_COUNT;
}
#endif /* DRVR_WB_TEST */

/* save old stats */
drv_save_stats(dift)
   landrv_ift *dift;
{
   mib_xEntry *mib_xptr= ((lan_ift *)dift)->mib_xstats_ptr;
   unsdword *statarr= &(dift->lan_vars.hwvars.statistics[0]);

   mib_xptr->if_sCInDiscards += statarr[INT_MISS];
   mib_xptr->if_sCInErrors += statarr[UNDELIVERABLE];
   mib_xptr->if_sCOutErrors += statarr[UNSENDABLE];
}


/* get MIB statistics */
int
drv_get_mibstats(ift,dat)
   lan_ift *ift;
   char *dat;
{
   int s;
   struct ifnet *ifnet_ptr=(struct ifnet *)ift;
   unsdword *statarr= &(((landrv_ift *)ift)->lan_vars.hwvars.statistics[0]);
   mib_xEntry *mib_xptr= (ift->mib_xstats_ptr);
   mib_ifEntry *mib_ptr= &(mib_xptr->mib_stats);
   char *DIO_descr = "lan%d Hewlett-Packard DIO LAN Interface Hw Rev 98643\0";

   mib_ptr->ifMtu=ifnet_ptr->if_mtu;
   mib_ptr->ifOutQlen=ifnet_ptr->if_snd.ifq_len;
   bcopy ((caddr_t)ift->is_addr, (caddr_t)mib_ptr->ifPhysAddress, 6);
   mib_ptr->ifPhysAddress[6]=NULL;
   mib_ptr->ifPhysAddress[7]=NULL;
   sprintf ((caddr_t)mib_ptr->ifDescr,64,(caddr_t)DIO_descr,ifnet_ptr->if_unit);
   s=splimp();
   if (!(((struct arpcom *)ifnet_ptr)->ac_flags & ACF_ETHER) &&
         !(((struct arpcom *)ifnet_ptr)->ac_flags & ACF_IEEE8023) &&
         ! (((struct arpcom *)ifnet_ptr)->ac_flags & ACF_SNAP8023) ) {
      mib_ptr->ifType=UNKNOWN;
   } else if ((((struct arpcom *)ifnet_ptr)->ac_flags & ACF_ETHER)) {
        mib_ptr->ifType = ETHERNET_CSMACD;
   } else {
        mib_ptr->ifType = IEEE8023_CSMACD;
   }
   mib_ptr->ifInErrors=statarr[UNDELIVERABLE] + mib_xptr->if_sCInErrors +
         mib_xptr->if_DInErrors;
   mib_ptr->ifOutErrors=statarr[UNSENDABLE] + mib_xptr->if_sCOutErrors +
         mib_xptr->if_DOutErrors;
   mib_ptr->ifInDiscards=statarr[INT_MISS] + mib_xptr->if_sCInDiscards +
         mib_xptr->if_DInDiscards;
   splx(s);
   bcopy((caddr_t)mib_ptr,(caddr_t)dat,sizeof(mib_ifEntry));
   return(0);
}

int
drv_get_stats(ift,fis)
   lan_ift *ift;
   struct fis *fis;
{
   landrv_ift *dift=(landrv_ift *)ift;
   unsdword *statarr= &(dift->lan_vars.hwvars.statistics[0]);

   fis->vtype= INTEGERTYPE;
   switch(fis->reqtype) {
      case RX_FRAME_COUNT:
         fis->value.i= statarr[ALL_RECEIVE];
         break;
      case TX_FRAME_COUNT:
         fis->value.i= statarr[ALL_TRANSMIT];
         break;
      case UNDEL_RX_FRAMES:
         fis->value.i= statarr[UNDELIVERABLE];
         break;
      case UNTRANS_FRAMES:
         fis->value.i= statarr[UNSENDABLE];
         break;
      case BAD_CONTROL_FIELD:
         fis->value.i= ift->BAD_CONTROL;
         break;
      case UNKNOWN_PROTOCOL:
         fis->value.i= ift->UNKNOWN_PROTO;
         break;
      case RX_BAD_CRC_FRAMES:
         fis->value.i= statarr[FCS_ERRORS];
         break;
      case CARRIER_LOST:
         fis->value.i= statarr[LOST_CARRIER];
         break;
      case NO_HEARTBEAT:
         fis->value.i= statarr[NO_HEART_BEAT];
         break;
      case MISSED_FRAMES:
         fis->value.i= statarr[INT_MISS];
         break;
      case ALIGNMENT_ERRORS:
         fis->value.i= statarr[INT_FRAMMING];
         break;
      case DEFERRED:
         fis->value.i= statarr[INT_DEFERRED];
         break;
      case LATE_COLLISIONS:
         fis->value.i= statarr[INT_LATE];
         break;
      case EXCESS_RETRIES:
         fis->value.i= statarr[INT_RETRY];
         break;
      case ONE_COLLISION:
         fis->value.i= statarr[INT_ONE];
         break;
      case MORE_COLLISIONS:
         fis->value.i= statarr[INT_MORE];
         break;
      case COLLISIONS:
         fis->value.i= statarr[TX_COLLISIONS];
         break;
      case LOCAL_ADDRESS:
         fis->vtype=PHYSADDR_SIZE;
         LAN_COPY_ADDRESS(ift->station_addr,fis->value.s);
         break;
      case PERMANENT_ADDRESS:
         fis->vtype=PHYSADDR_SIZE;
         return(drv_read_perm_address(&(dift->lan_vars),fis->value.s));
         break;
      case MULTICAST_ADDRESSES:
         fis->value.i=ift->num_multicast_addr;
         break;
      case DEVICE_STATUS:
         if(dift->lan_vars.hwvars.card_state==HARDWARE_UP) {
            fis->value.i=ACTIVE;
         } else {
            fis->value.i=FAILED;
         }
         break;
      default:
         return(EINVAL);
   }
   return(0);
}


int
drv_build_hdr(ac,pkt_type,hdr_info,m)
   struct arpcom *ac;
   int pkt_type;
   caddr_t hdr_info;
   struct mbuf **m;
{
   lan_rheader *lanhdr;
   struct mbuf *hdr_mbuf_ptr;

   /* Always get a new mbuf since this driver won't benefit from link header
    * coalescing. */
   if ((hdr_mbuf_ptr = m_get(M_DONTWAIT, MT_HEADER)) == NULL)
	return(ENOBUFS);
   hdr_mbuf_ptr->m_next = *m;
   *m = hdr_mbuf_ptr;
   hdr_mbuf_ptr->m_len = 0;

   switch(pkt_type) {
      case ETHER_PKT:
         (*m)->m_len=ETHER_HLEN;
         break;
      case IEEE8023_PKT:
         (*m)->m_len=IEEE8023_HLEN;
         break;
      case ETHERXT_PKT:
         (*m)->m_len=ETHERXT_HLEN;
         break;
      case IEEE8023XSAP_PKT:
         (*m)->m_len=IEEE8023XSAP_HLEN;
         break;
      case SNAP8023_PKT:
         (*m)->m_len=SNAP8023_HLEN;
         break;
      default:
         return(EINVAL);
   }

   /* copy the header prototype */
   bcopy(hdr_info,mtod((*m),caddr_t),(*m)->m_len);

   /* fill in the source address */
   lanhdr=mtod((*m),lan_rheader *);
   LAN_COPY_ADDRESS(ac->ac_enaddr,lanhdr->source);

   return(0);
}


/* hardware specific entry point from lanc */
int
drv_hw_req(ift,req,dat)
   lan_ift *ift;
   u_long req;
   struct mbuf *dat;
{
   landrv_ift *dift=(landrv_ift *)ift;
   int level,i,retval=0;

   level=splimp();
   switch(req & LAN_REQ_MSK) {
      case LAN_REQ_PROTOCOL_LOG:
      case LAN_REQ_PROTOCOL_REMOVE:
      case LAN_REQ_MC:
      case LAN_REQ_DOG_STAT:
         /* Don't have to do anything for these.  We don't care about */
         /*    protocol logging at this level, and lanc_media_control() */
         /*    does all of the multicast work for us. We don't need the */
         /*    watchdog timer (but EISA does). */
         break;
      case LAN_REQ_GET_STAT:
         retval=drv_get_stats(ift,(struct fis *)dat);
         break;
      case LAN_REQ_GET_MIBSTAT:
         retval=drv_get_mibstats(ift,(char *)dat);
         break;
      case LAN_REQ_SET_STAT:
         for(i=START_STAT_VAL;i<=END_STAT_VAL;i++) {
            reset_hw_stat(ift,i);
         }
         break;
      case LAN_REQ_INIT:
         /* Call drv_restart to download card configuration information */
         /*  0 -> Do Not reset statistics during restart */
         retval=drv_restart(&(dift->lan_vars),0);
         break;
      case LAN_REQ_RESET:
         if(drv_restart(&(dift->lan_vars),RESET_STATISTICS)) {
            ift->hdw_state |= LAN_DEAD;
            retval=ENXIO;
         }
         break;
      case LAN_REQ_RF:
         if(ift->broadcast_filter && !(dift->lan_vars.broadcast_state)) {
            retval=drv_enable_broadcast(&(dift->lan_vars));
         }
         if(!(ift->broadcast_filter) && dift->lan_vars.broadcast_state) {
            retval=drv_disable_broadcast(&(dift->lan_vars));
         }
         if(ift->promiscuous_filter) {
            ift->promiscuous_filter=0;
            retval=EINVAL;
         }
         break;
      case LAN_REQ_RAM_NA_CHG:
         /* reset hardware but not stats */
         if(drv_restart(&(dift->lan_vars),0)) {
            ift->hdw_state |= LAN_DEAD;
            retval=ENXIO;
         }
         LAN_COPY_ADDRESS(ift->station_addr,dift->lanc_ift.is_addr);
         break;
      case LAN_REQ_WRITE:
         if(!hw_loopback(&(dift->lan_vars.hwvars),dat)) {
            m_freem(dat);
            splx(level);
            return(0);
         }
         if(IF_QFULL(&(ift->is_if.if_snd))) {
            IF_DROP(&(ift->is_if.if_snd));
            m_freem(dat);
            ift->mib_xstats_ptr->mib_stats.ifOutDiscards++;
            splx(level);
            return(ENOBUFS);
         }
         IF_ENQUEUE(&(ift->is_if.if_snd),dat);
         if(!(dift->lan_vars.flags & LAN_NO_TX_BUF)) {
            hw_send(ift);
         } else {
            drv_ifq_max=(ift->is_if.if_snd.ifq_len > drv_ifq_max)
                  ? ift->is_if.if_snd.ifq_len : drv_ifq_max;
         }
         break;
      default:
         retval=EINVAL;
         break;
   }
   splx(level);
   return(retval);
}

