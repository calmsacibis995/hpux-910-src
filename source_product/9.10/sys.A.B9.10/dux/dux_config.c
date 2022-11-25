/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_config.c,v $
 * $Revision: 1.6.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:40:21 $
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

#ifndef _SYS_STDSYMS_INCLUDED
#    include "../h/stdsyms.h"
#endif   /* _SYS_STDSYMS_INCLUDED  */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/syscall.h"

#ifdef __hp9000s300
#include "../s200io/lnatypes.h"
#endif

#include "../h/user.h"
#include "../dux/dm.h"
#include "../h/dux_mbuf.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_hooks.h"

#ifdef __hp9000s300
#include "../h/socket.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../s200io/drvhw.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../sio/lanc.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../s200io/drvhw_ift.h"
#endif

#ifdef __hp9000s300
extern int lanift_init();
#include "../s200io/bootrom.h"
#include "../sio/lanc.h"
#include "../dux/duxlaninit.h"
#else
#include "../sio/llio.h"
#ifdef _WSIO
#include "../h/socket.h"
#include "../net/if.h"			/* needed by lanc.h	*/
#include "../netinet/in.h"		/* needed by lanc.h	*/
#include "../netinet/if_ether.h"	/* needed by lanc.h	*/
#include "../sio/lanc.h"		/* for lan_ift		*/
#include "../h/io.h"			/* for msus_type	*/
#else
#include "../machine/iodc.h"
#include "../sio/iotree.h"
#endif /* _WSIO vs non _WSIO */

#include "../netinet/if_ieee.h"

#endif /* s300 vs s700,s800 */

#include "../h/conf.h"

extern dev_t rootdev;
extern struct cdevsw cdevsw[];
extern int nchrdev;


#include "../dux/duxparam.h"
#include "../dux/cct.h"
#include "../dux/protocol.h"

#ifdef __hp9000s300
extern struct msus msus;
dux_lanift_init()
{
    /*
    ** DUX lan initialization no longer calls drv_init(~).
    ** It only sets up pointers to the new S300 LAN
    ** driver's lanift[] area, which holds the
    ** specific logical units (e.g. cards) lan_global
    ** and hw_globals structures. Previously, for the
    ** leafnode LAN driver, the routine dux_lan_init(isc)
    ** was called via the LAN drivers iosw table:
    **
    ** e.g.	    struct drv_table_type lan_iosw[] = {
    **		    dux_lan_init,
    **		       :
    **		       :
    **		    }
    **
    **
    ** It now looks like:
    **
    **				    lan_ift lanift[NUM_LAN_CARDS]
    **	    -------------------------------------------------
    **	    |						    |
    **	    |	    int lu;				    |
    **	    |	    int flags;				    |
    **	    |	       :				    |
    **	    |	    lan_global lan_vars;		    |
    **	    |	       :				    |
    **	    |	       :				    |
    **	    |						    |
    **	    |						    |
    **	    |	    typedef struct {			    |
    **	    |	    -----------------------------------	    |
    **	    |	    |				      |	    |
    **	    |	    |link_address local_link_address; |	    |
    **	    |	    |		    :		      |	    |
    **	    |	    |anyptr lan_dio_ift_ptr;	      |	    |
    **	    |	    |		    :		      |	    |
    **	    |	    |hw_globals hwvars;		      |	    |
    **	    |	    |		    :		      |	    |
    **	    |	    |		    :		      |	    |
    **	    |	    |				      |	    |
    **	    |	    |				      |	    |
    **	    |	    | typedef struct{		      |	    |
    **	    |	    |	    -----------------------   |	    |
    **	    |	    |	    |	     :		  |   |	    |
    **	    |	    |	    | word select_code;	  |   |	    |
    **	    |	    |	    |	     :		  |   |	    |
    **	    |	    |	    |	     :		  |   |	    |
    **	    |	    |	    |			  |   |	    |
    **	    |	    |	    -----------------------   |	    |
    **	    |	    |	    }  hw_globals, *hw_gloptr;|	    |
    **	    |	    |				      |	    |
    **	    |	    |				      |	    |
    **	    |	    -----------------------------------	    |
    **	    |			}  lan_global, *lan_gloptr; |
    **	    |						    |
    **	    |						    |
    **	    -------------------------------------------------
    **
    **	    NOTE: Is is assumed that the lan cards were previously
    **		  initialized by the LAN driver initialization
    **		  routine lanift_init(isc).  - daveg
    */

    extern struct ifnet *dux_lan_cards[];
    extern int num_dux_lan_cards;
    extern lan_ift *lan_dio_ift_ptr[];
    extern int num_lan_cards;
    extern int lanselectcode;

    register int lanift_selcode;
    register short i;
    register landrv_ift *lp;

    lan_gloptr myglobal;

    struct msus duxmsus;

    char maj;

    /*
    ** Get a copy of the MSUS (Mass Storage Unit Specifier), left by boot ROM.
    */
    duxmsus = msus;

   /*
    * lanselectcode is an undocumented configurable parameter in space.h
    * By default, it is -1.  Otherwise, it specifies a LAN select code
    * for a diskless node that boots off a disk
    */
   if (lanselectcode != -1) {
	for (i=0; i < num_lan_cards; i++) {
		lp = (landrv_ift *)lan_dio_ift_ptr[i];
	 	lanift_selcode = (int)lp->lan_vars.hwvars.select_code;
		if (lanselectcode == lanift_selcode) {

	/* found LAN - now pretend we booted from it in the first place */

   			duxmsus.dir_format  = LAN_DIRFORMAT;
			duxmsus.device_type = LAN_DEVICETYPE;
			duxmsus.sc          = lanselectcode;
			break;
		}
	}
	if (i < 0) {
		/* didn't find it */
		panic("invalid _lanselectcode");
	}
    }
    /*
    **  We have to decide if this was a diskless boot.
    */
   if (duxmsus.dir_format == LAN_DIRFORMAT &&
       ((duxmsus.device_type == LAN_DEVICETYPE) ||
        (duxmsus.device_type == SRM_DEVICETYPE)) &&
       (duxmsus.sc >= 0 && duxmsus.sc <= 31)) {
	/*
	** OK, this was a diskless boot.
	** Now build a new rootdev, that has the LAN card info.
	*/

	/* Find maj no. for LAN card */
	for (i = 0; i < nchrdev; i++) {
		if (cdevsw[i].d_open == DUXCALL(LLA_OPEN)) {
			maj = i;
			break;
		}
	}

	rootdev = makedev(maj,
		  makeminor(duxmsus.sc, duxmsus.ba, duxmsus.unit, duxmsus.vol));

	/*
	** We now have to scan all of the cards that were found
	** by the LAN card initialization to find a matching one.
	*/
	for (i=0; i < num_lan_cards; i++) {
	    lp = (landrv_ift *)lan_dio_ift_ptr[i];
            lanift_selcode = (int)lp->lan_vars.hwvars.select_code;

	    /*
	    ** Does the select code specified by the boot rom match up
	    ** with the select code specified by the LAN initialization
	    ** routines? If yes, then this must be the one to utilize.
	    */
	    if (lanift_selcode == duxmsus.sc) {
		if ((lp->lan_vars.hwvars.card_state != HARDWARE_UP) ||
		    (lp->lanc_ift.hdw_state & LAN_DEAD))
		    panic("LAN hardware failure");

	    	dux_lan_glob = &(lp->lan_vars);
	    	dux_hwvars   = &(lp->lan_vars.hwvars);
	    	myglobal = Myglob_dux_lan_gloptr;

		/* Set the pointer to the LAN card */
	        dux_lan_cards[0] = (struct ifnet *)lp;
		num_dux_lan_cards++;

   		msg_printf("Local Link Address = 0x%02x%02x%02x%02x%02x%02x\n",
			dux_lan_glob->local_link_address[0],
			dux_lan_glob->local_link_address[1],
			dux_lan_glob->local_link_address[2],
			dux_lan_glob->local_link_address[3],
			dux_lan_glob->local_link_address[4],
			dux_lan_glob->local_link_address[5]);
	   	break;
	   } /* if right card */
	} /* scan for cards */
    } /* if diskless boot */
}
#endif /* hp9000s300 */

#ifdef __hp9000s800
#ifdef _WSIO

extern dev_t duxlan_dev[];

/*
 * dux_lanift_init() --
 *    For diskless clients:
 *	 This routine initializes rootdev and sets dux_lan_cards[0] to point to
 *	 the LAN data structure for the boot device.
 *    For diskless servers:
 *	  This routine searches for other LAN cards and adds them into
 *        dux_lan_cards incrementing num_dux_lan_cards.
 *
 *    For both clients and servers:
 *	 This routine initializes the LAN device and sets up the lan
 *	 to route packets with the DISKLESS XSAP address to go to the
 *	 dpintr() routine [in protocol.c].
 */
dux_lanift_init()
{
    extern int dpintr();	/* in protocol.c */
    extern struct msus_type msus;
    extern struct ifnet *ifnet;
    extern struct ifnet *dux_lan_cards[];
    extern int num_dux_lan_cards;
    extern int lan2_open();
    int print_lla = 0;
    unsigned char maj;
    unsigned long minor_num;
    struct ifnet *ifp;
    int i;

    for (maj = 0; maj < nchrdev; maj++)
    {
	if (cdevsw[maj].d_open == lan2_open)
	   break; /* found it */
    }

    if (maj == nchrdev)
	panic("dux_lanift_init: LAN driver not configured");

    if (num_dux_lan_cards == 0)  /* Client */
    {
	int sc;

	if (msus.sversion != LAN_SV_ID)
	    return;

	if (msus.bus_type != PA_CORE)
	{
	    printf("Bus type 0x%02x not supported for diskless\n",
		msus.bus_type);
	    panic("Invalid bus type for diskless");
	}

	/*
	 * msus.mod_path[0] =  vsc#
	 * msus.mod_path[1] = slot#
	 * msus.mod_path[2] = func#
	 * msus.mod_path[3] = high 4 bytes of boot server lla
	 * msus.mod_path[4] = low 2 bytes of boot server lla (in the
	 *		      high order 2 bytes of mod_path[4]).
	 */
	sc = ((msus.mod_path[0]&0x07) << 4) | (msus.mod_path[1]&0x0f);
	minor_num = makeminor(
			sc,			           /* vsc */
			(msus.mod_path[2] & 0x0f), 0);	   /* func */
	duxlan_dev[0] = rootdev = makedev(maj, minor_num);

	/*
	 * We now have to scan all of the cards that were found
	 * by the LAN card initialization to find a matching one.
	 */
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	{
	    /*
	     * The s700 lan driver stores the high 12 bits of the
	     * minor number in hdw_path[0].  The low order 12 bits
	     * are always 0.
	     */
	    lan_ift *lp = (lan_ift *)ifp;

	    if (lp->hdw_path[0] == ((minor(duxlan_dev[0])>>12) & 0x0fff))
	    {
		    break; /* found it */
	    }
	}
	dux_lan_cards[0] = ifp;
	num_dux_lan_cards++;
	print_lla = 1;
    }
    else { /* Server */

#ifdef DUX_MULT_LAN_CARDS
	/*
	 * We now scan all of the cards that were found
	 * by the LAN card initialization and add them to our array of
	 * lan cards to use for dux.
	 */
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	{
	    lan_ift *lp = (lan_ift *)ifp;

	    /* Make sure we skip the card we already have */
	    if (ifp == dux_lan_cards[0]) {
		continue;
	    }

	    /* see if it is a lan card */
	    if (bcmp(ifp->if_name, "lan", 3)) {
		continue;
	    }
	    /*
	     * The s700 lan driver stores the high 12 bits of the
	     * minor number in hdw_path[0].  The low order 12 bits
	     * are always 0.
	     */

	    minor_num = lp->hdw_path[0]<<12;
	    duxlan_dev[num_dux_lan_cards] = makedev(maj, minor_num);
	    dux_lan_cards[num_dux_lan_cards] = ifp;
	    num_dux_lan_cards++;
	}
#endif /* DUX_MULT_LAN_CARDS */
    }

    if (dux_lan_cards[0] == ((struct ifnet *) 0))
	panic("dux_lan_init: Unable to initialize dux_lan_cards[0]");

    for (i = 0; i < num_dux_lan_cards; i++) {
        lanc_init(dux_lan_cards[i]);
        lanc_log_protocol(LAN_CANON, IEEEXSAP_DUX, dpintr, dux_lan_cards[i],
	    0, 1, 0, 0, 0, 0);
    }

    if (print_lla)
    {
	extern unsigned char *get_lan_addr();
	unsigned char *my_addr;
	my_addr = get_lan_addr(0);

	msg_printf("Local Link Address = 0x%02x%02x%02x%02x%02x%02x\n",
	    my_addr[0], my_addr[1], my_addr[2],
	    my_addr[3], my_addr[4], my_addr[5]);
    }
}
#else /* not _WSIO */
dux_lanift_init()
{
    extern struct ifnet *dux_lan_cards[];
    extern int num_dux_lan_cards;
    extern int dpintr();	/* in protocol.c */
    register lan_ift *lp;
    register struct ifnet *ifp;

    for (ifp = ifnet; ifp; ifp = ifp->if_next)	/* search for ifnet */
    {
	if ((bcmp(ifp->if_name, "lan", 3) == 0))  /* lan associated */
	{
	    lp = (lan_ift *) ifp;	/* cast to lan_ift */
	    if ((duxlan_major[0] == lp->mjr_num) &&
		(duxlan_mgr_index[0] == lp->mgr_index))
		break;			/* got it */
	}
    }

    dux_lan_cards[0] = ifp;
    num_dux_lan_cards++;
    if (dux_lan_cards[0] == ((struct ifnet *) 0))
	panic("dux_lan_init:  Unable to initialize dux_lan_cards[0]");

    lanc_init(dux_lan_cards[0]);

    /* logging Canonical Address (Extended SAP) for DUX */
    /* This is a work_around to fix the problem that dux_lan_init
    ** is called on the client in rootinit and on the server in
    ** init (cluster).  More consideration should be given when we
    ** put DUX running on ICS. -- V. H.
    */
    lanc_log_protocol(LAN_CANON, IEEEXSAP_DUX, dpintr, dux_lan_cards[0],
	0, 1, 0, 0, 0, 0);
}
#endif /* _WSIO vs non _WSIO */
#endif /* __hp9000s300 */


extern short rootlink[];

#ifdef _WSIO
/*
 * This code handles the configurable parameters that allow "diskless"
 * boots from a disk.  To boot off a disk, set "rootlink" in space.h
 * (or set it with adb(1)).
 *
 * This stuff works by faking the environment that would be created
 * by booting from LAN, namely the server Link Level LAN address
 * (i.e. 0x080009001234) that is normally stored by the boot ROM.
 *
 * By default, rootlink contain -1's.
 */
int
find_root(remoteroot)
int	*remoteroot;
{
    unsigned char maj = major(rootdev);

    /*
     * If the rootdev hasn't been set yet (NODEV), or the major number
     * of rootdev equals that of the LAN interface, then this is a
     * remote boot and we always want to go remote.
     *
     * We always direct the cluster request to the link address of
     * the boot server that supplied the kernel.  This isn't quite
     * correct if we booted from a swap server instead of the root
     * server, but ws_cluster() detects this and corrects rootlink
     * to the actual rootserver link address in such a situation.
     */
    if (rootdev == NODEV ||
	(maj < nchrdev && cdevsw[maj].d_open == DUXCALL(LLA_OPEN))) {
#ifdef __hp9000s300
	/*
	 * The LAN address on the Series 300 is located in the
	 * Bootrom F_AREA.
	 */
	extern u_char bootrom_f_area[];

	bcopy(bootrom_f_area, rootlink, ADDRESS_SIZE);
#else /* hp9000s700 */
	/*
	 * On the Series 700, the LAN address of the boot server starts
	 * at mod_path[3] and continues into the high order 2 bytes
	 * of mod_path[4].
	 */
	extern struct msus_type msus;

	bcopy(&msus.mod_path[3], rootlink, ADDRESS_SIZE);
#endif

	*remoteroot = 1;	/* TRUE - Remote Root */
    }
    else {
	/*
	 * We booted locally, check 'rootlink' to see if it
	 * was explicitly initialized, indicating that we should
	 * go remote, even though we may have booted from some
	 * other device.
	 */
	if ((rootlink[0] & rootlink[1] & rootlink[2]) == -1) {
	    *remoteroot = 0;	/* FALSE - Local Root */
	}
	else {
	    /*
	     * rootlink already contains the LAN address
	     * that we should use.
	     */
	    *remoteroot = 1;	/* TRUE - Remote Root */
	}
    }
}
#else

struct device_entry boot_device;

	extern int duxlan_major[];
	extern int duxlan_mgr_index[];

find_root(remoteroot)
int	*remoteroot;
{
    io_port_info_type   root_info;
    u_char		    *server_linkaddr;

    load_real(&boot_device, IODC_BOOT_DEV,
	      sizeof (struct device_entry)/sizeof (int));

    if (boot_device.class == CL_NULL)
	panic("find_root: boot device class is null\n");

    if (io_port_info(IOP_ROOT, &root_info) != 0)
	panic("find_root: root device info invalid\n");

    if (strcmp(root_info.class, "lan")) {	/* root not on lan */
#ifdef BOOT_DEBUG
	printf("root on %s\n", root_info.module);
#endif /* BOOT_DEBUG */
	*remoteroot = 0;
	return;
    }

    duxlan_major[0] = root_info.c_major;
    duxlan_mgr_index[0] = root_info.mgr_index;

    server_linkaddr = ((unsigned char *) &(boot_device.path.layer[0])) + 6;

#ifdef BOOT_DEBUG
    for (i=0; i<6; i++)
	printf(".%x", server_linkaddr[i]);
    printf(" = server's link address\n");
#endif /* BOOT_DEBUG */

    if (server_linkaddr[0] != 0x08 ||
	server_linkaddr[1] != 0x00 ||
	server_linkaddr[2] != 0x09){
	if ((rootlink[0] & rootlink[1] & rootlink[2]) != -1)
	    *remoteroot = 1;
	else
	    *remoteroot = 0;
	return;
    }

    bcopy((caddr_t)server_linkaddr, (caddr_t)rootlink, ADDRESS_SIZE);
    *remoteroot = 1;
}
#endif /* _WSIO vs. _SIO */

extern struct cct clustab[];

/*
 * sitels() --
 *     sfills a user buffer with the site numbers for all active sites,
 *     ending with 0, and returns the number of sites in the cluster.
 *     If the user buffer is NULL, only the site count is returned.
 */
sitels()
{
    struct a {
	site_t *buffer;
    };

    register site_t *u_clist = ((struct a *) u.u_ap)->buffer;
    register struct cct *sp;
    register int site_count;

    if ((my_site_status & CCT_CLUSTERED) == 0) {
	u.u_r.r_val1 = 0;
	return;
    }

    site_count = 0;

    if (u_clist == NULL) {
	for (sp = &(clustab[1]); sp < &(clustab[MAXSITE]); sp++)
	    if (sp->status & CL_IS_MEMBER)
		site_count++;
    }
    else {
	site_t site_num = 1;

	for (sp = &(clustab[1]); sp < &(clustab[MAXSITE]); site_num++, sp++) {
	    if (sp->status & CL_IS_MEMBER) {
		site_count++;
		if (u.u_error = copyout(&site_num, u_clist, sizeof (site_t)))
		    return;
	        u_clist++;
	    }
	}

	/*
	 * Terminate the site list with the site number 0
	 */
	site_num = 0;
	if (u.u_error = copyout(&site_num, u_clist, sizeof (site_t)))
	    return;
    }

    u.u_r.r_val1 = site_count;
}

/*
 * swap_clients() --
 *     fills a user buffer with the site numbers for all active swap
 *     clients, ending with 0, and returns the number of sites swapping
 *     to this swap server.  If the user buffer is NULL, only the site
 *     count is returned.
 */
swap_clients()
{
    struct a {
	site_t *buffer;
    };

    register site_t *u_clist = ((struct a *)u.u_ap)->buffer;
    register struct cct *sp;
    register int site_count;

    if ((my_site_status & CCT_CLUSTERED) == 0) {
	u.u_r.r_val1 = 0;
	return;
    }

    site_count = 0;

    if (u_clist == NULL) {
	for (sp = &(clustab[1]); sp < &(clustab[MAXSITE]); sp++)
	    if ((sp->status & CL_IS_MEMBER) && sp->swap_site == my_site)
		site_count++;
    }
    else {
	site_t site_num = 1;
	for (sp = &(clustab[1]); sp < &(clustab[MAXSITE]); site_num++, sp++) {
	    if ((sp->status & CL_IS_MEMBER) && sp->swap_site == my_site) {
		site_count++;
		if (u.u_error = copyout(&site_num, u_clist, sizeof (site_t)))
		    return;
		u_clist++;
	    }
	}

	/*
	 * Terminate the site list with the site number 0
	 */
	site_num = 0;
	if (u.u_error = copyout(&site_num, u_clist, sizeof (site_t)))
	    return;
    }

    u.u_r.r_val1 = site_count;
}

#ifdef RMTPROCESS
/*
**				RMTPROCESS
**
** This function is system call number 183.
** This system call is for a full DUX implementation of RPE.
** It may never be utilized since the decision for 6.0 S300 was
** to utilize the ARPA/Berkeley interface instead.
*/

rmtprocess()
{
    dm_message unsp_request, unsp_response;	/* UNSP request/reply	*/

    register struct a {
	unsigned int req_site_id;	/* Request site id	*/
	unsigned int resp_site_id;	/* Response site id	*/
	unsigned int mod_argv_envp_size;
	char *slave_pty_name;		/* Pty to use on slave	*/
	unsigned int spare0;
	unsigned int spare1;
    } *uap = (struct a *) u.u_ap;	/* Ptr to u area args	*/

    struct rmt_cmd_info {
	unsigned int um_id;		/* Place holder - unsp	*/
	unsigned int req_site_id;
	unsigned int resp_site_id;
	unsigned int mod_argv_envp_size;
	char slave_pty_name[3];
	char spare_space[29];
    } *reqp;				/* Request pointer	*/

    struct unsp_message *unsp_ptr;

#ifdef RMTPROCESS_DEBUG
    printf("INSIDE OF KERNEL rmtprocess system call\n");
#endif RMTPROCESS_DEBUG
    if (!(my_site_status & CCT_CLUSTERED))
	return;

    /*
     * Allocate a dm message (mbuf). The rmt_cmd_info struct is
     * exactly the same size as the unsp_message struct. This
     * way we can simply overlay one struct over the other, template
     * fashion.
     */
    unsp_request = dm_alloc(sizeof (struct unsp_message), WAIT);
    reqp = DM_CONVERT(unsp_request, struct rmt_cmd_info);

    reqp->req_site_id = uap->req_site_id;
    reqp->resp_site_id = uap->resp_site_id;
    reqp->mod_argv_envp_size = uap->mod_argv_envp_size;

    u.u_error = copyin((caddr_t)uap->slave_pty_name,
		       (caddr_t)reqp->slave_pty_name,
		       sizeof (reqp->slave_pty_name));

    /*
     * Now set the id field to -1 , 1st time this nsp is invoked!
     */
    unsp_ptr = DM_CONVERT(unsp_request, struct unsp_message);
    unsp_ptr->um_id = -1;
    DM_OP_CODE(unsp_request) = DM_RMTCMD;

#ifdef RMTPROCESS_DEBUG
    printf("Args are:\n");
    printf("	req_site_id = %d\n", reqp->req_site_id);
    printf("	resp_site_id = %d\n", reqp->resp_site_id);
    printf("	rmt_cmd = %s\n", reqp->rmt_cmd);
    printf("	slave_pty_name = %s\n", reqp->slave_pty_name);
#endif RMTPROCESS_DEBUG

    unsp_response = dm_send(unsp_request,
			    DM_REPEATABLE | DM_SLEEP |
			    DM_RELEASE_REQUEST | DM_INTERRUPTABLE,
			    DM_RMTCMD,
			    (site_t)uap->resp_site_id,
			    DM_EMPTY, NULL, NULL, 0, 0,
			    NULL, NULL, NULL);

    u.u_error = DM_RETURN_CODE(unsp_response);
    dm_release(unsp_response, 1);
    return(u.u_error);
}
#endif RMTPROCESS

/*
**			DSKLESS_STATS
**
** This function is system call number 184.
** It is fairly general purpose and is used to set/reset/read DUX
** specific statistics from user-land. This system call is not
** documented.
*/

#define STATS_RESET_PROTOCOL_INFO	0
#define STATS_READ_PROTOCOL_INFO	1
#define STATS_READ_MBUF_INFO		2
#define STATS_READ_CBUF_INFO		3
#define STATS_READ_INBOUND_OPCODES	4
#define STATS_RESET_INBOUND_OPCODES	5

#define STATS_READ_OUTBOUND_OPCODES	7
#define STATS_RESET_OUTBOUND_OPCODES	8

#define READIN_DEBUGGER_MBUF		9

dskless_stats()
{
    /*
    ** Specific to the diskless protocol statistics.
    */
    extern struct protocol_stats proto_stats;

    /*
    ** Specific to the diskless mbuf/cluster/netbuf statistics.
    */
    extern struct	dux_mbstat dux_mbstat;

#ifdef KERNEL_DEBUG_ONLY
    /*
    ** Specific to the diskless debugger
    */
    extern struct mbuf debugger_mbuf;
#endif KERNEL_DEBUG_ONLY

    /*
    ** Specific to the diskless dm layer op-code statistics.
    */
    extern struct proto_opcode_stats inbound_opcode_stats;
    extern struct proto_opcode_stats outbound_opcode_stats;

    register struct a
    {
	unsigned int stats_requested;
	char *user_info_ptr;
	unsigned int spare0;
	unsigned int spare1;
    } *uap = (struct a *) u.u_ap;	/* Ptr to u_area args	*/

    switch (uap->stats_requested)
    {
    case STATS_RESET_PROTOCOL_INFO:		/* 0 */
	bzero(&proto_stats, sizeof (struct protocol_stats));
	break;

    case STATS_READ_PROTOCOL_INFO:		/* 1 */
	if (uap->user_info_ptr != NULL) {
	    u.u_error = copyout((caddr_t)&proto_stats,
			        (caddr_t)uap->user_info_ptr,
			        sizeof (struct protocol_stats));
	}
	else
		u.u_error = EINVAL;
	break;

    case STATS_READ_MBUF_INFO:		/* 2 */
    case STATS_READ_CBUF_INFO:		/* 3 */
	if (uap->user_info_ptr != NULL) {
	    u.u_error = copyout((caddr_t)&dux_mbstat,
				(caddr_t)uap->user_info_ptr,
				sizeof (struct dux_mbstat));
	}
	else
	    u.u_error = EINVAL;
	break;

    case STATS_READ_INBOUND_OPCODES:		/* 4 */
	if (uap->user_info_ptr != NULL) {
	    u.u_error = copyout((caddr_t)&inbound_opcode_stats,
			        (caddr_t)uap->user_info_ptr,
			        sizeof (struct proto_opcode_stats));
	}
	else
	    u.u_error = EINVAL;
	break;

    case STATS_RESET_INBOUND_OPCODES:		/* 5 */
	bzero(&inbound_opcode_stats,
	      sizeof (struct proto_opcode_stats));
	break;

    case STATS_READ_OUTBOUND_OPCODES:		/* 7 */
	if (uap->user_info_ptr != NULL) {
	    u.u_error = copyout((caddr_t)&outbound_opcode_stats,
			        (caddr_t)uap->user_info_ptr,
			        sizeof (struct proto_opcode_stats));
	}
	else
	    u.u_error = EINVAL;
	break;

    case STATS_RESET_OUTBOUND_OPCODES:		/* 8 */
	bzero(&outbound_opcode_stats,
	      sizeof (struct proto_opcode_stats));
	break;

#ifdef KERNEL_DEBUG_ONLY
    case READIN_DEBUGGER_MBUF:			/* 9 */
	printf("dskless_stats: READIN_DEBUGGER_INPUT\n");
	if (uap->user_info_ptr != NULL) {
	   u.u_error = copyin((caddr_t)uap->user_info_ptr,
			      (caddr_t)&debugger_mbuf,
			      sizeof (struct mbuf));
	    deliver_debugger_action();
	}
	else
	    u.u_error = EINVAL;
	break;
#endif KERNEL_DEBUG_ONLY

    default:
	u.u_error = EINVAL;
    }
}
