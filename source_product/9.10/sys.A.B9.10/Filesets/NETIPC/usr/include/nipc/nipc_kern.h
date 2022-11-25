/* 
 * kernel only Netipc types and constants
 */
/*
 * Definitions to provide the kernel opt routines with access to the kernel
 * opt structure.  See the opt macros defined below.
 */
/* allow for multiple include of this .h file */
#ifndef NIPC_KERN.H
#define NIPC_KERN.H

#define NIPC_MAX_PROTOADDR	sizeof(struct sockaddr)

struct nipc_protoaddr {
	int	len;
	short	data[NIPC_MAX_PROTOADDR/sizeof(short)];
};

#define	NIPC_CONTROL_WLEN	2	/* # bytes passed to ipccontrol */
#define	NIPC_CONTROL_RLEN	2	/* # bytes returned from ipccontrol */
#define NIPC_TCP_ADDRLEN	2	/* sizeof a port number */
#define NIPC_PXP_ADDRLEN	2	/* sizeof a PXP port number */

/* socket types known only in kernel */
/* other types supported are in ns_ipc.h */
#define NS_REQUEST			4
#define NS_REPLY			5

#define NIPC_DEFAULT_RECV_THRESHOLD	1
#define NIPC_DEFAULT_SEND_THRESHOLD	1
#define NIPC_DEFAULT_CONN_REQ_BACK 	1
#define NIPC_DEFAULT_MAX_RECV_SIZE	100
#define NIPC_DEFAULT_MAX_SEND_SIZE	100
#define NIPC_DEFAULT_TIMEOUT		600	/*60 sec. in tenths of seconds*/
#define NIPC_MAX_TIMEO			1000000 /* enforced by itimerfix */
						/* and hence by ipccontrol */


/* 
 * limits, high and low, on user options
 */
#define KO_CONN_REQ_BACK_MAX	20
#define KO_CONN_REQ_BACK_MIN	1

/* structure at front of user space options to Netipc calls */
struct opthead {
	short	oh_length;
	short	oh_count;
};

/* structures inbedded in user space options to Netipc calls */
struct optentry {
	u_short	oe_kind;
	u_short	oe_offset;
	u_short	oe_length;
	u_short	oe_fill;
};

	
struct k_opt { 
	union { 
		struct {
			unsigned	ko_data_offset 		: 1;
			unsigned	ko_max_conn_req_back 	: 1;
			unsigned	ko_max_recv_size 	: 1;
			unsigned	ko_max_send_size 	: 1;
			unsigned	ko_protocol_address 	: 1;
		} 			f;
		int			i;
	} ko_flags;

	u_short				ko_data_offset;
	u_short				ko_max_conn_req_back;
	short				ko_max_recv_size;
	short				ko_max_send_size;
	struct nipc_protoaddr		*ko_protocol_address;
	struct nipc_protoaddr		ko_protoaddr;
};


#define KOPT_DEFINE(kopt,code) 		(kopt)->ko_flags.f.code = 1;
#define KOPT_DEFINED(kopt,code) 	(kopt)->ko_flags.f.code
#define KOPT_FETCH( kopt,code ) 	(kopt)->code 
#define	KOPT_UNDEFINE(kopt,code) 	(kopt)->ko_flags.f.code = 0;
#define	KOPT_ANY_DEFINED(kopt) 		(kopt)->ko_flags.i

/* constants passed as the 'code' parameter to the above macros */
#define KO_DATA_OFFSET			ko_data_offset
#define KO_MAX_CONN_REQ_BACK		ko_max_conn_req_back
#define KO_MAX_RECV_SIZE		ko_max_recv_size
#define KO_MAX_SEND_SIZE		ko_max_send_size
#define KO_PROTOCOL_ADDRESS		ko_protocol_address


extern short nipc_error_mappings[];

#define NETIPC_RETURN(err) 	return(nipc_error_mappings[err])


/*
 * 	path report constants
 */

#define	PATH_VERSION	0	/* path report version number		*/
#define	PATH_MAX_LEN	500	/* maximum path report length in bytes	*/
#define PATH_LOOPOK	1	/* include loopback in path report      */
#define PATH_NOLOOP	0	/* don't include loopback in path report*/

/*
 *	path goodness values 
 */

#define	PATH_NO_PATH		0	/* no usable paths found 	     */
#define PATH_HAVE_ROUTE		1	/* proto supported and have a route  */
#define	PATH_LOCAL_NET		2	/* proto supported and local network */
#define PATH_LOCAL_SUBNET	3	/* proto supported and local subnet  */
#define	PATH_THIS_HOST		4	/* proto supported and local host    */

 
#endif /* NIPC_KERN.H */
