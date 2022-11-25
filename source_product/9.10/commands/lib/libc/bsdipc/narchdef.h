/*
 * @(#)narchdef.h: $Revision: 35.1 $ $Date: 88/02/10 14:04:55 $
 * $Locker:  $
 * 
 */

/******************************************************************************
 ******************************************************************************
 **                             NARCHDEF.H       
 **
 **  AUTHOR:            Carl Dierschow, David Hendricks, Tim DeLeon
 **
 **  DATE BEGUN:        23 Feb 84
 **
 **  MODULE:            arch
 **  PROJECT:           leaf_project
 **  REVISION:          2.6
 **  SCCS NAME:         /users/fh/leaf/sccs/arch/header/s.narchdef.h
 **  LAST DELTA DATE:   85/06/19
 **
 **  DESCRIPTION:       Global type and constant definitions exported by the
 **                     architecture.
 **
 **                     Only files which reside in the network environment
 **                     should include this file.
 **                     
 ******************************************************************************
 *****************************************************************************/

#ifdef CONFIG_VAX
/* This is for debugging shared images.  You can disable this when debugging */
/* simple images.                                                            */
#define printf  axeprintf
#define nprintf axeprintf
#define fprintf axefprintf
#define nscanf  scanf
#endif CONFIG_VAX

#ifdef CONFIG_HP200
 
/*  This is here since it is identical to the QA define.                */
#ifdef QA
#define ASSERTION_CHECK 1
#endif

#ifdef  NTRIGGER
#define NPERFORM        1
#endif

/*      Software interrupt priorities.
 *      
 *      These levels were decided upon after a discussion with Mike Dunn.
 *      October 31, 1984.
 */

#define LAN_PROC_LEVEL          0
#define LAN_PROC_SUBLEVEL       5
#define TIMER_PROC_LEVEL        0
#define TIMER_PROC_SUBLEVEL     5

#endif


/* ------------------------------------------------------------------------- */
/*                                                                           */
/*     Various other simple types which need to be defined.  The pointers    */
/*     below can only point into network memory.                             */
/*                                                                           */
/* ------------------------------------------------------------------------- */

	  /* The maximum number of protocols in any particular connection    */
#define MAX_STACK_PROTOCOLS 4
#define INVALID_LEVEL       127
typedef byte    level_number;            /* 0..MAX_STACK_PROTOCOLS-1 */

					 /* Max number of octets in a packet */
#define MAX_PACKET_SIZE 32767
typedef word    packet_offset;                         /* 0..MAX_PACKET_SIZE */

/*      The packet_buffer structure is never really allocated.
 *      It is used only as an overlay structure for memory in which the size
 *      is variable.  Generally this is used for packets.
 */
typedef char          packet_buffer[MAX_PACKET_SIZE];
typedef char          * packet_ptr;


/* ------------------------------------------------------------------------- */
/*                                                                           */
/*      Addressing constants : The following list of constants below are     */
/*                             in addressing different architecture levels.  */
/*                                                                           */
/*      Reference:   Clark Johnson, HP-IND.    Canonical Addressing Standard */
/*                   HP-internal memo,   19 January 1984                     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*      IEEE802 remote_sap Addresses.   */

#define IEEE802_IP_ADR  006             /*  006 Octal.          */
#define IEEE802_EXP_ADR 252             /*  374 Octal.          */


/*      ETHERNET type_field Addresses.  */

#define ETH_IP_ADR      2048            /*  4000 Octal.         */
#define ETH_EXP_ADR     32773           /*  100005 Octal.       */


/*      Used to distiguish between an ethernet and ieee802 packet.  If the word
 *      in the ethernet type_field position is less than this value, assume it
 *      is IEEE802's frame length and that the packet is an ieee802 packet.
 */

#define ETH_START_ADR   1600


/*      Protocol maximum header sizes.  */
#ifdef CONFIG_LAN
#define LAN_HEADER_SIZE 24
#define LLP_HEADER_SIZE LAN_HEADER_SIZE
#endif

#define IP_HEADER_SIZE  20
#define TCP_HEADER_SIZE 24
	

/*
 * These protocol identifiers are used in a connection to specify a protocol
 * handler, and can be used to map into the protocol_globals structure.
 */
 
#define NULL_ID         0
#define LAN_ID          1
#define ASYNC_ID        2
#define SWITCH_ID       3       /* For VAX process switching */
#define DIRECT_ID       4
#define PROBE_ID        5
#define IP_ID           6
#define TCP_ID          7
#define SI_ID           8
#define BB_ID           9

#ifdef QA
#define TH_ID           10      /* QA only.  Test Harness.              */
#define EG_ID           11      /* QA only.  Exception Generator.       */
#define TICL_ID         12      /* QA only.  TICL protocol.             */
#endif

#ifdef QA
#define NUM_PROTO_IDS   13
#else
#define NUM_PROTO_IDS   10
#endif

typedef byte    protocol_id;    
typedef word    connection_id;

typedef error_number (*rcv_proc)();
typedef error_number (*error_proc)();
typedef error_number (*send_proc)();
typedef error_number (*init_rcv_proc)();
typedef error_number (*ack_rcv_proc)();
typedef error_number (*init_send_proc)();
typedef error_number (*ack_send_proc)();
typedef error_number (*conn_proc)();
typedef error_number (*init_conn_proc)();
typedef error_number (*ack_conn_proc)();
typedef error_number (*abort_proc)();
typedef error_number (*t_o_proc)();
typedef error_number (*ack_create_proc)();
typedef error_number (*config_proc)();
typedef error_number (*simple_proc)();


/*************************************************************************** 
 *     
 *     This is the structure which contains the global information
 *     for a protocol.  Only the owning module should ever write into
 *     the area for any protocol.  Note that to use the xxx_gp pointers you
 *     must typechange the pointer to a structured pointer type.
 *                                                                 
 ***************************************************************************/

typedef struct {
	word            canonical_addr;
	abort_proc      abort_rtn;
	ack_conn_proc   ack_conn_rtn;
	ack_rcv_proc    ack_rcv_rtn;
	ack_send_proc   ack_send_rtn;
	conn_proc       conn_rtn;
	error_proc      error_rtn;
	init_conn_proc  init_conn_rtn;
	init_rcv_proc   init_rcv_rtn;
	init_send_proc  init_send_rtn;
	rcv_proc        rcv_rtn;
	send_proc       send_rtn;
	t_o_proc        t_o_rtn;
	config_proc     config_rtn;
	union {
		anyptr  async_gp;
		anyptr  lan_gp;
		anyptr  direct_gp;
		anyptr  probe_gp;
		anyptr  ip_gp;
		anyptr  icmp_gp;
		anyptr  tcp_gp;
		anyptr  ipc_gp;
		anyptr  th_gp;
		anyptr  eg_gp;
	} ids;
} prot_glob_entry;
typedef prot_glob_entry prot_glob_type[NUM_PROTO_IDS];


/* ------------------------------------------------------------------------- */
/*
/*      The CONNECTION_STATE record which holds all of the information
/*      needed to maintain a connection.  Pretty much all of this is
/*      read-only to a protocol handler.  Note that to use the xxx_infop pointer
/*      you must typechange the pointer to a structured pointer type.
/*
/* ------------------------------------------------------------------------- */

/*      Connection types         */
#define C_IPC_TCP_IP_LLP        0
#define C_DIRECT_LLP            1
#define C_PROBE_LLP             2
#define MAX_CONN_TYPE           3
	       
typedef byte connection_type;   /* 0 .. 3 */

typedef struct conn {
	struct conn     * next_connection; 
	connection_type conntype; 
	boolean         dgram_server_path; 
	boolean         active_created;
	node_name_type  remote_node_name;
	word            num_of_buffers;
	word            total_packet_size;
	packet_offset   total_header_size;
	level_number    numprotos; 
	struct {
		protocol_id     protocol;
		union {
			anyptr   async_infop;
			anyptr   lan_infop;
			anyptr   direct_infop;
			anyptr   probe_infop;
			anyptr   ip_infop;
			anyptr   icmp_infop;
			anyptr   tcp_infop;
			anyptr   ipc_infop;
			anyptr   th_infop;
			anyptr   eg_infop;
		} info;
	} protos[MAX_STACK_PROTOCOLS];
} connection_state;
typedef connection_state   * connection_ptr;


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Structures and constants needed by the config_rtn                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define CREDIT_WINDOW           10

typedef word    config_request;

typedef struct  {
	word    packet_count;           /* Amount to increase window by */
	word    byte_count;             /* Amount to increase window by */
} credit_win_type;

typedef struct {
	union   {
		credit_win_type credit;
	} req;
}  req_parm_type;


/* ------------------------------------------------------------------------- */
/*                                                                           */
/*     Some structures needed by the Path utilities                          */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*      Service types.
 */
#define NO_SERVICE      0
#define VT_SERVICE      1
#define IPC_SR_SERVICE  2
#define MSRFA_SERVICE   3
#define MSRDEV_SERVICE  4 
	       
typedef byte service_type;      /* 0..4 */

typedef byte internal_service;          /* 0..1 */


typedef enum {  IEEE802_ID,
		ETHERNET_ID     } llp_id;

typedef struct {
	llp_id          llp_protocol;
	link_address    dest_addr;
} lan_info;

typedef struct {
	protocol_id     protocol;
	union {
		lan_info        lan_addr;
		inet_address    ip_addr;
	} addrs;
} address_list[MAX_STACK_PROTOCOLS];


/*
 *      The constant below are used for identifing the position in the stack
 *      of specific protocols.  Note that the protocol stack actually starts
 *      at 0.
 *
 *      WARNING: These defines should only be used by the path utilities
 *               and probe routines.
 */

#define LLP_LEVEL       0

#define NET_LEVEL       1
#define PROBE_LEVEL     1
#define DIRECT_LEVEL    1
#define IP_LEVEL        NET_LEVEL

#define XPRT_LEVEL      2
#define TCP_LEVEL       XPRT_LEVEL

#define SER_LEVEL       3
#define SI_LEVEL        SER_LEVEL


/* ------------------------------------------------------------------------- */
/*                                                                           */
/*     The definition of the protect token, dispatch token, and timer token, */
/*     plus a few global tokens.  Nobody except the protection, task & timer */
/*     utilities should EVER look inside these records, even for reading!    */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/* $IRS$ protect_token */
typedef struct ptok {
	word            locked;
	struct ptok     * next_p_tok;
} protect_token;
typedef protect_token   * prottok_ptr;
/* $IRS$ */

/* $IRS$ dispatch_token */
typedef struct dtok {
	byte            dflags;
#ifdef QA
	struct dtok     *next_d_tok;
#endif
#ifdef CONFIG_VAX
	byte            event_flag;
	byte            complete_event_flag;
	struct dsc$descriptor_s *cluster; /* Ptr to VAX specific string struct*/
#else
	int             sav_pri;
#endif
} dispatch_token, *dispatch_ptr; 
typedef ordptr dispatch_id;
/* $IRS$ */

/* $IRS$ time_stamp */
#ifdef CONFIG_VAX
/* VAX timer services require quad word types which have the lsw in the      */
/* upper unsdword.                                                           */
typedef struct {    
	unsdword        time;
	unsdword        cycles;
} time_stamp;
#else
typedef struct {
	unsdword        cycles;
	unsdword        time;
} time_stamp;
#endif
/* $IRS$ */

/* $IRS$ timer_token */
typedef struct ttok {
	struct ttok     * next_t_tok;
	struct ttok     * next_active;
	time_stamp      waketime;
	unsdword        wakeident;
	connection_ptr  csrp;
	t_o_proc        t_o_handler;
	level_number    level;
} timer_token;
typedef timer_token     * timertok_ptr;
/* $IRS$  */


/****************************************************************************
 *
 *      External declarations for architecture modules
 *
 *           These declarations are the standard by which functions types
 *           are checked and enforced.  If a module accidentally mistypes
 *           a function the compiler will issue a "function redefined"
 *           message.
 *
 ***************************************************************************/

/****************************************************************************
 *
 *      Exported user interface calls                  
 *
 ***************************************************************************/
   extern error_number exec_in_net_envt     ();
#ifndef CONFIG_VAX
   extern         void convert_user_ptr     (); 
   extern         void convert_kernel_ptr   (); 
   extern error_number evaluate_user_ptr    ();
   extern         void copy_data_from_user  ();  
   extern         void copy_data_to_user    ();
   extern         void copy_network_data    (); 
   extern         void copy_block_of_data   (); 
#endif
   extern         void add_offset_to_longptr(); 
   extern         char onechar              ();  /* is this still used? */
   extern         void setonechar           (); 
   extern         void clearbytes           (); 
#ifdef CONFIG_VAX
   extern error_number switch_processes     ();
   extern error_number switch_TCP_abort     ();
   extern error_number switch_TCP_ack_conn  ();
   extern error_number switch_TCP_ack_rcv   ();
   extern error_number switch_TCP_ack_send  ();
   extern error_number switch_TCP_conn      ();
   extern error_number switch_TCP_error     ();
   extern error_number switch_TCP_init_conn ();
   extern error_number switch_TCP_init_rcv  ();
   extern error_number switch_TCP_init_send ();
   extern error_number switch_TCP_rcv       ();
   extern error_number switch_TCP_send      ();
   extern error_number switch_TCP_t_o       ();
   extern error_number switch_TCP_config    ();
   extern status_type  connect_process      ();
   extern status_type  connect_resources    ();
   extern        void  network_ctl_trap     ();
   extern status_type  customize_envt       ();
#endif

/****************************************************************************
 *
 *      Exported task calls                  
 *
 ***************************************************************************/
   extern  dispatch_id create_dispatch_token ();
   extern         void destroy_dispatch_token(); 
   extern error_number dispatch              ();
   extern error_number udispatch             ();
   extern error_number wakeup_user_task      ();
   extern         void complete_dispatch     (); 
   extern         void receive_task          (); 
   extern         void timer_task            (); 
   extern error_number network_exit          ();
   extern        dword get_process_id        ();
   extern         void init_task_module      (); 

/****************************************************************************
 *
 *      Exported path calls                  
 *
 ***************************************************************************/
   extern error_number fix_up_node_name          ();
   extern error_number create_name_outbound_path ();
   extern error_number create_passive_path       ();
   extern error_number create_addr_outbound_path ();
   extern error_number create_dgram_server_path  ();
   extern error_number find_inbound_path         ();
   extern error_number find_bb_path              ();
   extern         void attach_csr                (); 
   extern         void detach_csr                (); 
   extern error_number destroy_path              ();
   extern         void init_path_module          (); 
   extern error_number is_this_me                ();
   extern error_number get_network_info          (); /*not implemented 1st rel*/
   extern      boolean address_in_use            ();

/****************************************************************************
 *
 *      Exported port calls                  
 *
 ***************************************************************************/
   extern error_number allocate_dynamic_port     ();
   extern error_number reserve_static_port       ();
   extern         void destroy_port              (); 
   extern         void init_port_module          (); 

/****************************************************************************
 *
 *      Exported timer calls                  
 *
 ***************************************************************************/
   extern         void create_timer_token        (); 
   extern         void destroy_timer_token       (); 
   extern         void wake_me_in                (); 
   extern         void cancel_wakeup             (); 
   extern         void get_network_time          (); 
   extern         void init_timer_module         (); 

/****************************************************************************
 *
 *      Exported memory manager calls                  
 *
 ***************************************************************************/
   extern error_number allocate                  ();
   extern         void deallocate                (); 
   extern         void init_memory_module        (); 

/****************************************************************************
 *
 *      Exported prot calls                  
 *
 ***************************************************************************/
#ifdef CONFIG_VAX
   extern         void enter_critical_region     ();
   extern         void exit_critical_region      ();
#endif
   extern         void create_protect_token      (); 
   extern         void destroy_protect_token     (); 
   extern         void protect                   ();   
   extern         void unprotect                 (); 
   extern         void init_protect_module       (); 

/****************************************************************************
 *
 *      Exported pool calls                  
 *
 ***************************************************************************/
   extern error_number get_buffer                ();
   extern         void return_buffer             (); 
   extern error_number contribute_buffers        ();
#ifdef CONFIG_VAX
   extern         void hw_return_buffer          (); 
#endif

/****************************************************************************
 *
 *      Exported nodal manager calls                  
 *
 ***************************************************************************/

/* nodemgr.c    */
extern error_number     default_interface       ();
extern error_number     init_network            ();
extern error_number     read_network_config     ();
extern error_number     network_set_limit       ();
extern error_number     read_nusers_info        ();

/***************************************************************************
 *      Exported OSINT calls
 **************************************************************************/

#ifdef CONFIG_VAX
extern int              change_uic();
extern int              call_ipcbox();
extern int              multiprocess_disable();
extern int              multiprocess_enable();
extern int              get_mode();
#else
extern void             bomb();
#endif

/***************************************************************************
 *      Exported DRVMUNGE calls
 **************************************************************************/
extern int              net_hexdump();

/***************************************************************************
 *      Exported DIRECT calls
 **************************************************************************/
#ifdef CONFIG_VAX
extern error_number     vax_da_abort();
#endif

/* ------------------------ end of NARCHDEF.H ------------------------------ */
