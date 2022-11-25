/*
 * @(#)libio.h: $Revision: 1.4.83.4 $ $Date: 93/09/17 18:28:27 $
 * $Locker:  $
 */

#ifndef _SYS_LIBIO_INCLUDED /* allows multiple inclusion */
#define _SYS_LIBIO_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _WSIO  

/*
  Definitions for libIO.a are found here
*/
#ifdef _KERNEL_BUILD
#include "../h/io.h"
#include "../machine/cpu.h"
#include "../h/errno.h"
#else /* ! _KERNEL_BUILD */
#include <sys/io.h>
#include <machine/cpu.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif /* _KERNEL_BUILD */

/* generic libIO.a data types */

#define MAX_IO_PATH_ELEMENTS 15

typedef struct {
	int	addr[MAX_IO_PATH_ELEMENTS];
	int	num_elements;
} hdw_path_type;

typedef struct {
	unsigned char	my_isc;
	char		bus_type;
	int		if_id;
	int		ftn_no;
	int		c_major;
	int		b_major;
	hdw_path_type	hdw_path;
} isc_entry_type;

/* defines for io_search_isc "search_type" argument */

#define SEARCH_FIRST		    0	/* beginning of search */
#define SEARCH_NEXT		    1	/* continuation of search */
#define SEARCH_SINGLE		    2	/* independent search */

/* defines for io_search_isc "search_key" argument */

#define KEY_ALL			0	/* match all entries */
#define KEY_MY_ISC		1	/* match on my_isc */
#define KEY_BUS_TYPE		2	/* match on bus type */
#define KEY_IF_ID		3	/* match on interface id */
#define KEY_HDW_PATH		4	/* match on hardware path */

/* defines for io_search_isc "search_qual" argument */

#define QUAL_NONE		0x0000	/* no qualifiers */
#define QUAL_INIT		0x0001	/* match only initialized entries */
#define QUAL_FTN_NO		0x0002	/* match only entries with the
					   specified function number */

#define QUAL_MASK	(QUAL_INIT|QUAL_FTN_NO)

#ifdef __hp9000s700

/* defines for io_get_table/io_free_table "which_table" argument */

#define T_IO_MOD_TABLE		0	/* VSC Module Table */
#define T_MEM_DATA_TABLE	1	/* Page zero memory configuration info*/

/* Native I/O Module Table for io_get_table(T_IO_MOD_TABLE) */

typedef struct {
	iodc_hversion_type	iodc_hversion;
	iodc_spa_type		iodc_spa;
	iodc_type_type		iodc_type;
	iodc_sversion_type	iodc_sversion;
	u_short			iodc_reserved;
	u_char			iodc_rev;
	u_char			iodc_dep;
	u_short			iodc_check;
	u_short			iodc_length;
	caddr_t			hpa;
	u_int                   more_pgs;
	union {
	    struct {
		isc_entry_type	isc_table;
		int			next_ftn;
		int			ftn_no;
	    } fio;
	    struct {
		int			ba_mod_table[NUM_SBUS_MODS];
	    } ba;
	} type;
} return_vsc_mod_entry;

/*
** Page Zero memory configuration information for 
** io_get_table(T_MEM_DATA_TABLE) 
*/

typedef struct {
	int	spa1:9;
	int	spa2:9;
	int	reserved:9;
	int	size:3;
	int	enabled1:1;
	int	enabled2:1;
} mem_data_entry;

union return_mem_entry
{
	int		int_data;
	mem_data_entry	mem_data;	
};

union return_mem_entry	return_mem_table[32];

#endif /* __hp9000s700 */

/* defines for libIO.a function return values */

/* Warnings */
#define END_OF_SEARCH		 0x001	/* end of search reached */
/* Success */
#define SUCCESS			 0	/* the function worked */
/* Errors */
#define ALREADY_OPEN		-1	/* /dev/config is already open */
#define BAD_ADDRESS_FORMAT	-2	/* hdw_path invalid */
#define INVALID_KEY		-3	/* invalid "search_key" argument */
#define INVALID_ENTRY_DATA	-4	/* invalid data in isc_entry */
#define INVALID_QUAL		-5	/* invalid "search_qual" argument */
#define INVALID_TYPE		-6	/* invalid "type" argument */
#define LENGTH_OUT_OF_RANGE	-7	/* path length invalid */
#define NO_HARDWARE_PRESENT	-8 	/* no hardware at specified path */
#define NO_MATCH		-9 	/* no entries match search criteria */
#define NO_SEARCH_FIRST		-10	/* no SEARCH_FIRST before SEARCH_NEXT */
#define NOT_OPEN		-11	/* /dev/config is not open */
#define OUT_OF_MEMORY		-12	/* unable to malloc memory */
#define PERMISSION_DENIED	-13	/* permission denied on file */
#define NO_SUCH_FILE		-14	/* /dev/config does not exist */
#define SYSCALL_ERROR		-15	/* non-specific system call error */
#define INVALID_FLAG            -16	/* flag in open is invalid */

#ifndef _KERNEL
/* Type definitions for all the functions */

extern int  io_init();
extern void io_end();

#ifdef __hp9000s700
extern int  io_get_table();
extern void io_free_table();
#endif /*  __hp9000s700 */

extern int  io_search_isc();

/* Unsupported functions */
extern char *hdw_path_to_string();
extern void do_print_libIO_status();
extern int  print_libIO_status();
extern int  string_to_hdw_path();
#endif /* not _KERNEL */

/* Function indices */

#define _IO_END			 0

#ifdef __hp9000s700
#define _IO_FREE_TABLE		 1
#define _IO_GET_TABLE		 2
#endif /* __hp9000s700 */

#define _IO_INIT		 3
#define _IO_SEARCH_ISC		 4
#define _PRINT_LIBIO_STATUS	 5
#define _HDW_PATH_TO_STRING	 6
#define _STRING_TO_HDW_PATH	 7

#else /* _WSIO */

/*
  Definitions for libIO.a are found here
*/

#ifdef _KERNEL_BUILD
#include "../sio/iotree.h"
#include "../machine/cpu.h"
#include "../h/errno.h"
#else /* ! _KERNEL_BUILD */
#include <sio/iotree.h>
#include <machine/cpu.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif /* _KERNEL_BUILD */

/* AUTOBAHN: how should this really be defined? */
typedef int	node_id_type;

#define ROOT_NODE	0	/* node id of the root of the io_tree */
#define DUMMY_NODE	-3	/* node id of the "dummy" root node */
#define MAX_TRIES	3	/* how many times to try ioctl/read */

/* generic libIO.a data types */

typedef struct {
	int	addr[MAX_IO_PATH_ELEMENTS];
	int	num_elements;
} hdw_path_type;

typedef struct {
	io_name_type	name[MAX_IO_PATH_ELEMENTS];
	int		num_elements;
} module_path_type;

typedef struct {
	node_id_type	node_id;
	io_name_type	manager;
	int		c_major;
	int		b_major;
	mgr_opt_type	mgr_options;
	int		mgr_index;
	io_name_type	module;
	mod_opt_type	mod_options;
	io_name_type	class;
	class_opt_type	class_options;
	conn_opt_type	conn_options;
	int		lu;
	ent_id_type	ent_id;
	hdw_path_type	hdw_path;
	node_id_type	parent;
	node_id_type	sibling;
	node_id_type	child;
	port_num_type	port;
	node_state_type	node_state;
} io_node_type;

/* defines for io_add_path "action" argument */

#define ADD_PATH_ONLY		0	/* add the module path only */
#define ADD_PATH_AND_BIND	1	/* add path and force bind */

/* defines for io_search_tree "search_type" argument */

#define SEARCH_FIRST		    0	/* beginning of search */
#define SEARCH_FIRST_AND_SUBTREE    1	/* search first including subtree */
#define SEARCH_NEXT		    2	/* continuation of search */
#define SEARCH_SINGLE		    3	/* independent search */

/* defines for io_search_tree "search_key" argument */

#define KEY_ALL			0	/* match all nodes */
#define KEY_NODE_ID		1	/* match on node_id */
#define KEY_MANAGER_NAME	2	/* match on manager name */
#define KEY_MODULE_NAME		3	/* match on module name */
#define KEY_CLASS_NAME		4	/* match on class name */
#define KEY_HDW_PATH		5	/* match on hardware path */
#define KEY_PORT_NUMBER		6	/* match on port number */
#define KEY_C_MAJOR		7	/* match on character major number */
#define KEY_B_MAJOR		8	/* match on block major number */

/* defines for io_search_tree "search_qual" argument */

#define QUAL_NONE		0x0000	/* no qualifiers */
#define QUAL_BOUND		0x0001	/* match only bound nodes */
#define QUAL_B_MAJOR		0x0002	/* match only nodes with the
					   specified b_major */
#define QUAL_C_MAJOR		0x0004	/* match only nodes with the
					   specified c_major */
#define QUAL_CLASS_NAME		0x0008	/* match only nodes with the
					   specified class name */
#define QUAL_FROM_FILE		0x0010	/* match only /etc/ioconfig nodes */
#define QUAL_FROM_GEN		0x0020	/* match only uxgen io{} nodes */
#define QUAL_FROM_PROBE		0x0040	/* match only nodes with hardware */
#define QUAL_LU			0x0080	/* match only nodes with the
					   specified lu */
#define QUAL_LDM		0x0100	/* match only nodes that are LDMs */
#define QUAL_MODULE_NAME	0x0200	/* match only nodes with the
					   specified module name */
#define QUAL_BASIC		0x0400	/* apply qualifiers to basic search */
#define QUAL_SUBTREE		0x0800	/* apply qualifiers to subtree search */
#define QUAL_MGR_EXISTS		0x1000	/* match only nodes with manager
					   which is NOT nonexistent */

#define QUAL_MASK	(QUAL_BOUND|QUAL_B_MAJOR|QUAL_C_MAJOR| \
			 QUAL_CLASS_NAME|QUAL_FROM_FILE| \
			 QUAL_FROM_GEN|QUAL_FROM_PROBE|QUAL_LU|QUAL_LDM| \
			 QUAL_MODULE_NAME|QUAL_BASIC|QUAL_SUBTREE| \
			 QUAL_MGR_EXISTS)

/* defines for io_get_connectivity "type" argument */

#define CONNECT_PARENTS		0	/* return list of valid parents */
#define CONNECT_CHILDREN	1	/* return list of valid children */

/* defines for io_get_table/io_free_table "which_table" argument */

#define T_IO_CLASS_TABLE	0	/* I/O Device Class Table */
#define T_IO_KERN_DEV_TABLE	1	/* I/O Kernel Device Table */
#define T_IO_MGR_TABLE		2	/* I/O Manager Table */
#define T_IO_SW_MOD_TABLE	3	/* I/O Software Module Table */
#define T_IO_MOD_TABLE		4	/* Native I/O Module Table */

/* I/O Device Class Table for io_get_table(T_IO_CLASS_TABLE) */

typedef struct {
	io_name_type		name;
	class_opt_type		options;
} io_class_type;

/* I/O Kernel Device Table for io_get_table(T_IO_KERN_DEV_TABLE) */

typedef struct {
	node_id_type	node_id;
	node_id_type	gen_node_id;
	kern_flag_type	flags;
	dev_t		dev_num;
	dev_t		gen_dev_num;
} kern_dev_type;

/* I/O Manager Table for io_get_table(T_IO_MGR_TABLE) */

typedef struct {
	io_name_type		name;
	mgr_opt_type		options;
	int			c_major;
	int			b_major;
} io_mgr_type;

/* I/O Software Module Table for io_get_table(T_IO_SW_MOD_TABLE) */

typedef struct {
	io_name_type		mod_name;
	io_name_type		class_name;
	mod_opt_type		options;
	int			c_major;
	int			b_major;
} io_sw_mod_type;

/* Native I/O Module Table for io_get_table(T_IO_MOD_TABLE) */

typedef struct {
	iodc_hversion_type	iodc_hversion;
	iodc_spa_type		iodc_spa;
	iodc_type_type		iodc_type;
	iodc_sversion_type	iodc_sversion;
	u_short			iodc_reserved;
	u_char			iodc_rev;
	u_char			iodc_dep;
	u_short			iodc_check;
	u_short			iodc_length;
	int			fixed;
	int			next;
	caddr_t			hpa;
	union {
	    struct {
		caddr_t		spa;
		int		spa_size;
		int		mem_size;
		u_int		io_status;
		u_int		io_err_info;
		u_int		io_err_resp;
		u_int		io_err_req;
	    } mem;
	    struct {
		caddr_t		spa;
		int		spa_size;
		int		eim;
		caddr_t		poll_entry;
	    } io, other;
	    struct {
		int		next_bus;
		caddr_t		io_low;
		caddr_t		io_high;
		int		is_lower_port;
		u_int		io_status;
		u_int		io_err_info;
		u_int		io_err_resp;
		u_int		io_err_req;
		int		other_port;
	    } bc;
	} type;
} mod_entry_type;

typedef struct {
	node_id_type	parent_node;
	io_name_type	manager;
	io_name_type	module;
	io_name_type	class;
	int		lu;
	int		hdw_address;
} add_node_type;

/* AUTOBAHN: temporary hack until we remove NO_SUCH_MODULE from iotree.h */
#undef NO_SUCH_MODULE

/* defines for libIO.a function return values */

/* Warnings */
#define ADDED_CLASS		 0x001	/* class was added to kernel */
#define ADDED_CONNECTIVITY	 0x002	/* connectivity was added to kernel */
#define ADDED_MANAGER		 0x004	/* manager was added to kernel */
#define ADDED_MODULE		 0x008	/* module was added to kernel */
#define END_OF_FILE		 0x010	/* end of file reached on read */
#define END_OF_SEARCH		 0x020	/* end of search reached */
#define INCOMPLETE_PATH		 0x040	/* not a complete path to the device */
/* Success */
#define SUCCESS			 0	/* the function worked */
/* Errors */
#define ALREADY_OPEN		-1	/* /dev/config is already open */
#define BAD_ADDRESS_FORMAT	-2	/* hdw_path invalid */
#define BAD_CONNECTION		-3	/* module_path connection invalid */
#define BAD_MAGIC		-4	/* /etc/ioconfig magic number wrong */
#define CLASS_CONFLICT		-5	/* module has conflicting class */
#define COULD_NOT_BIND		-6	/* can't bind node */
#define COULD_NOT_UNBIND	-7	/* can't unbind node - can't remove */
#define IOCONFIG_EXISTS		-8	/* /etc/ioconfig already exists */
#define IOCONFIG_NONEXISTENT	-9	/* /etc/ioconfig does not exist */
#define INVALID_ACTION		-10	/* invalid "action" argument */
#define INVALID_FLAG		-11	/* invalid "flag" argument */
#define INVALID_KEY		-12	/* invalid "search_key" argument */
#define INVALID_NODE_DATA	-13	/* invalid data in io_node */
#define INVALID_QUAL		-14	/* invalid "search_qual" argument */
#define INVALID_TYPE		-15	/* invalid "type" argument */
#define LENGTH_OUT_OF_RANGE	-16	/* path length invalid */
#define NO_HARDWARE_PRESENT	-17	/* no hardware at specified path */
#define NO_MATCH		-18	/* no nodes match search criteria */
#define NO_SEARCH_FIRST		-19	/* no SEARCH_FIRST before SEARCH_NEXT */
#define NO_SUCH_FILE		-20	/* /dev/config does not exist */
#define NO_SUCH_MODULE		-21	/* module not in the kernel */
#define NO_SUCH_NODE		-22	/* specified node_id not found */
#define NODE_HAS_CHILD		-23	/* node has child - can't remove */
#define NOT_OPEN		-24	/* /dev/config is not open */
#define OUT_OF_MEMORY		-25	/* unable to malloc memory */
#define PATHS_NOT_SAME_LENGTH	-26	/* hdw_path & module_path lens differ */
#define PERMISSION_DENIED	-27	/* permission denied on file */
#define STRUCTURES_CHANGED	-28	/* structures changed since last call */
#define SYSCALL_ERROR		-29	/* non-specific system call error */

#ifndef _KERNEL
/* Type definitions for all the functions */

extern int  io_check_connectivity();
extern int  io_get_connectivity();
extern void io_free_connectivity();
extern int  io_init();
extern void io_end();
extern int  io_get_mod_info();
extern int  io_add_tree_node();
extern int  io_add_lu();
extern int  io_remove_tree_node();
extern int  io_add_path();
extern int  io_probe_hardware();
extern int  io_get_table();
extern void io_free_table();
extern int  io_search_tree();
extern int  ioconfig_open();
extern int  ioconfig_read();
extern int  ioconfig_write();
extern int  ioconfig_close();

/* Unsupported functions */
extern char *hdw_path_to_string();
extern char *mod_path_to_string();
extern void do_print_libIO_status();
extern int  print_libIO_status();
extern int  string_to_hdw_path();
extern int  string_to_mod_path();
extern int  ioconfig_open();
extern int  ioconfig_close();
extern int  ioconfig_read();
extern int  ioconfig_write();
extern int  update_ioconfig();
#endif /* not _KERNEL */

/* Function indices */

#define _IO_ADD_LU		 0
#define _IO_ADD_PATH		 1
#define _IO_ADD_TREE_NODE	 2
#define _IO_CHECK_CONNECTIVITY	 3
#define _IO_END			 4
#define _IO_FREE_CONNECTIVITY	 5
#define _IO_GET_CONNECTIVITY	 6
#define _IO_FREE_TABLE		 7
#define _IO_GET_MOD_INFO	 8
#define _IO_GET_TABLE		 9
#define _IO_INIT		10
#define _IO_PROBE_HARDWARE	11
#define _IO_REMOVE_TREE_NODE	12
#define _IO_SEARCH_TREE		13
#define _IOCONFIG_CLOSE		14
#define _IOCONFIG_OPEN		15
#define _IOCONFIG_READ		16
#define _IOCONFIG_WRITE		17
#define _PRINT_LIBIO_STATUS	18
#define _UPDATE_IOCONFIG	19
#define _HDW_PATH_TO_STRING	20
#define _MOD_PATH_TO_STRING	21
#define _STRING_TO_HDW_PATH	22
#define _STRING_TO_MOD_PATH	23

#endif /* _WSIO */
#endif /* not _SYS_LIBIO_INCLUDED */
