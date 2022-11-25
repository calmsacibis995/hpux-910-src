/*
 * @(#)cct.h: $Revision: 1.8.83.3 $ $Date: 93/09/17 16:39:30 $
 * $Locker:  $
 */
#ifndef _SYS_CCT_INCLUDED
#define _SYS_CCT_INCLUDED


/* HPUX_ID: %W%		%E% */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
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

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#ifdef _KERNEL_BUILD
#include "../dux/protocol.h"  /* ADDRESS_SIZE */
#else  /* ! _KERNEL_BUILD */
#include <sys/protocol.h>
#endif /* _KERNEL_BUILD */

#define CCT_CPU_ARCH_300	0x01	/* Series 300 machine. */
#define CCT_CPU_ARCH_800	0x02	/* Series 800 machine. */
#define CCT_CPU_ARCH_700	0x03	/* Series 700 machine. */

/* Cluster Configuration Table 
 * Each site within a cluster is given a  cluster-wide unique site
 * number.  This number is used as an index for accessing the cluster 
 * configuration table.  
*/
struct cct {
	char status; /*site status*/
	u_char net_addr[ADDRESS_SIZE]; 
	u_char lan_card;            /* Lan card to use for server, for client
				    if non-zero then forward through server */
	long nak_time; /*latest arrival time of negative ack msg, in sec */  
	int total_buffers;
		/* The cpu_ variables are used for cpu dependent
		 * operations.  The cpu_model is a machine dependent
		 * value, usually found in machine/cpu.h
		 */
	short cpu_arch;		/* CPU architecture  e.g. S300, S800 */
	short cpu_model;	/* Family model number  e.g. 310, 840 */
	site_t swap_site;	/* cnode id of swap server */
	u_short	req_count;	/* Number of requests we've sent to this guy
				 * that we are waiting for */
	struct using_entry
		*req_waiting_Qhead,	/* First req (via a using entry) that
					 * is waiting to be sent to this guy */
		*req_waiting_Qtail;	/* Last reqest in waiting for que */
	int	site_seqno;	/* packet sequencer */
}; 

#define CCT_STATUS_MASK 0xf0    /*used for masking off the sub-states*/
#define CL_FAILED  0x00		/*site has crashed or disconnected */
#define CL_CLEANUP 0x01		/*site has crashed and all clients notified,
				  but is not cleaned up yet */
#define CL_INACTIVE   0x10 	/*site is not a member of cluster */
#define CL_ALIVE   0x21   	/*site is alive but may not be active*/
#define CL_ACTIVE 0x22		/*site is active*/
#define CL_RETRY 0x24		/* site is alive, but may not be active
				   and is currently being retried */
#define CL_IS_MEMBER 0x20	/*site is either ALIVE or ACTIVE or in RETRY */

/*site status*/
extern u_int my_site_status;

#define CCT_ROOT 	0x01	/*site is a root server, else a non-root*/
#define CCT_CLUSTERED  	0x02    /*site is running in clustered mode*/
#define CCT_STARTED	0x04	/*site id, name has been set; i.e. site has been
				 *pre-clustered */
#define CCT_SWPSERVER	0x08	/*site is a swap server		*/
#define CCT_SLWS	0x80	/*site is swapless*/

/* Useful Macros */
#define IM_SERVER (my_site == root_site)

/* 
 * function code for cluster system call
 */
#define CLUSTER_SETID	0
#define CLUSTER_CHMOD	1

#define P_CLEANUP PZERO+1	/*sleep priority for cleanup processes*/

#ifdef IS_CLUSTER	/*only define the following definitions in cluster.c*/


		/* Flags used to compare client and server kernels */
#define DUX_HAS_NFS	0x0001	/* Cnode has NFS 3.0 */
#define DUX_HAS_NFS3_2	0x0002	/* Cnode has NFS 3.2 additional features */
#define DUX_HAS_POSIX	0x0004	/* Cnode has POSIX system calls/semantics */
#define DUX_HAS_ACLS	0x0008	/* Cnode has Access Control Lists */
#define DUX_HAS_AUDIT	0x0010	/* Cnode has Auditing */
#define DUX_HAS_CDFS	0x0020	/* Cnode has CD File System */


struct clusterreq {		/*DUX MESSAGE STRUCTURE*/
	u_char  machine_id[ADDRESS_SIZE];
	u_char net_addr[ADDRESS_SIZE];
	int total_buffers;
	short cpu_arch;		/* CPU architecture  e.g. S300, S800 */
	short cpu_model;	/* Family model number  e.g. 310, 840 */
	int	client_flags;	/* Flags describing clients flag. */
	char release[UTSLEN];
	};	/*cluster request message*/


struct cluster_error_resp {	/*DUX MESSAGE STRUCTURE*/
	/* Message sent back if there are kernel incompatibilities */
	int	server_flags;
	char release[UTSLEN];
	char rootlink[ADDRESS_SIZE];	/* address of real server */
	};	/*error response to cluster request*/


struct clusterresp {		/*DUX MESSAGE STRUCTURE*/
	site_t	site_id;
	site_t  root_site;
	dev_t	root_dev;
	site_t   swap_site;
	u_char multiaddr[ADDRESS_SIZE];
	struct privgrp_map priv_global;		/*initial privgrp info*/
	struct privgrp_map privgrp_map[EFFECTIVE_MAXGRPS];
	char site_name[DIRSIZ+1];
	char	nodename[UTSLEN];	/*initial hostname*/
	struct timeval time;		
	struct timezone tz;
	};	/*response for cluster request*/ 
	
struct addmember {		/*DUX MESSAGE STRUCTURE*/
	site_t	site_id;
	u_char net_addr[ADDRESS_SIZE];
	int total_buffers;
	short cpu_arch;		/* CPU architecture  e.g. S300, S800 */
	short cpu_model;	/* Family model number  e.g. 310, 840 */
	site_t	swap_site;	/* swap server of this cluster node */
	u_char lan_card;
	char addmember_pad;
	};	/*request for adding a member, a multicast message*/

struct cct_entry {		/*DUX MESSAGE STRUCTURE*/
 	u_char net_addr[ADDRESS_SIZE];
 	site_t site_id;
 	char site_name[DIRSIZ+1];
 	site_t swap_site;
 	int knsp;
};			
#endif /* IS_CLUSTER */

#endif /* _SYS_CCT_INCLUDED */
