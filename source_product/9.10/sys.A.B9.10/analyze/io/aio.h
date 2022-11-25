/* different call_types that are allowed for i/o manager analysis rtns */

#define AN_MGR_INIT		1
#define AN_MGR_BASIC		2
#define AN_MGR_DETAIL		3
#define AN_MGR_OPTIONAL		4
#define AN_MGR_HELP    		5


/* error conditions */

#define AIO_OK			0
#define AIO_NO_REV_INFO 	1
#define AIO_INCOMPAT_REVS	2
#define AIO_NO_SUCH_PORT	3
#define AIO_NO_DECODING		4
#define AIO_INVALID_SUBQ	5
#define AIO_NO_SUCH_MESSAGE	6
#define AIO_NO_SUCH_ENTRY	7
#define AIO_NO_SUCH_OPTION	8
#define AIO_SUBSYS_USED		9
#define AIO_INVALID_RANGE       10
#define AIO_INVALID_ADDRESS	11
#define AIO_TOO_BIG             12
#define AIO_REVS_THE_SAME       13
#define AIO_INCOMPLETE          14


/* call types for aio_get_mgr_info */

#define AIO_PORT_NUM_TYPE	0
#define AIO_IOTREE_TYPE		1
#define AIO_IO_NODE_TYPE	2

#define AIO_SYMBOL_AND_OFFSET   0
#define AIO_SYMBOL_ONLY         1


/* structure definition for manager info block */

typedef struct mgr_info_type {
    char			mgr_name[16];
    char			hw_address[20];
    port_num_type		port_num;
    unsigned			pda_address;
    set_of_32			enabled_subqs;
    set_of_32			active_subqs;
    int				iotree_index;
    int				next_iotree_index;
    int				mgr_table_index;
    bit8			blocked;
    bit8			eim;
    bit8			in_poll_list;
    struct aio_TREE_entry	*io_tree_entry;
    struct aio_TREE_entry	*next_io_tree_entry;
} mgr_info_type;

/* structure definitions for MY copies of io data structures, */
/* a superset of the kernel's */

typedef struct {
    io_name_type		name;
} aio_name_entry;

typedef struct {
	io_name_ptr		kname;
	io_tree_entry		*kclass_link;
	class_opt_type		options;
	io_name_ptr		name;
	struct aio_TREE_entry	*class_link;
} aio_class_entry;

typedef struct {
	io_name_ptr		kname;
	int			(*entry)();
	int			(*attach)();
	int			(*init)();
	mgr_opt_type		options;
	int			b_major;
	int			c_major;
	io_tree_entry		*kmi_link;
	io_name_ptr		name;
	struct aio_TREE_entry	*mi_link;
} aio_mgr_entry;

typedef struct {
	io_name_ptr		kname;
	io_class_entry		*kclass;
	mod_opt_type		options;
	int			(*idy)();
	io_name_ptr		name;
	aio_class_entry		*class;
} aio_mod_entry;

typedef struct aio_TREE_entry {
    io_tree_entry    		*kparent;
    io_tree_entry    		*ksibling;
    io_tree_entry    		*kchild;
    io_conn_entry		*conn;
    ent_id_type			ent_id;
    int				mgr_index;
    int				lu;
    port_num_type		port;
    node_state_type		state;
    int				hdw_address;
    io_tree_entry    		*kmi_link;
    io_tree_entry    		*kclass_link;
    /* And my fields (local equivalents of the kernel pointers */
    struct aio_TREE_entry	*parent;
    struct aio_TREE_entry	*sibling;
    struct aio_TREE_entry	*child;
    struct aio_TREE_entry	*mi_link;
    struct aio_TREE_entry	*class_link;
    /* Replacement for conn -- manager & module pointers */
    io_mod_entry		*kmodule;
    io_mgr_entry		*kmanager;
    aio_mod_entry		*module;
    aio_mgr_entry		*manager;
} aio_tree_entry;

#include "machine/cpu.h"
typedef struct aio_native_mod_ENTRY {
    native_mod_entry		entry;
    struct aio_native_mod_ENTRY	*next;
    struct aio_native_mod_ENTRY	*next_bus;
    struct aio_native_mod_ENTRY	*other_port;
} aio_native_mod_entry;

typedef struct {
    io_tree_entry	*knode;
    io_tree_entry	*kgen_node;
    kern_flag_type	flags;
    dev_t		dev_num;
    dev_t		gen_dev_num;
    aio_tree_entry	*node;
    aio_tree_entry	*gen_node;
} aio_kern_dev_entry;
