#ifdef iostuff

#  include "../standard/inc.h"
#  include <machine/int.h>
#  include <string.h>
#  include <sio/llio.h>
#  include "aio.h"
#  include <sioserv/ioserv.h>
#  define BOOLEAN(s)	((s) ? "true" : "false")
#  define eq(a,b)	(strcmp((a),(b)) == 0)

/* analyze globals this file needs */
extern int		tolerate_error;

/* globals used only in this file and aio_rtns.c */
struct ioheader_type 	ioheader;
aio_tree_entry  	*io_tree_ptr;
aio_native_mod_entry	*native_mod_ptr;
int 	   		max_port;

struct msg_control 	Msg_control;
msg_frame_ptr 		first_frame; 		/* Local */
msg_frame_ptr 		vfirst_frame;		/* Virtual */
msg_frame_ptr 		vlast_frame;		/* Virtual */
imc_port_ptr		port_index;		/* Local */
imc_port_ptr		*vport_index;		/* Virtual */
aio_tree_entry		**pseudo_port_index;	/* Local */
io_tree_entry		**vpseudo_port_index;	/* Virtual */

struct iotree_type  	*iotree;
struct mgr_table_type 	*mgr_table;
int			*io_mem_map;
unsigned		io_mem_buf;
unsigned		io_mem_top;

static aio_class_entry  	*io_class_ptr;
static aio_mgr_entry  		*io_mgr_ptr;
static aio_mod_entry  		*io_mod_ptr;
static aio_name_entry		*io_names;
static aio_kern_dev_entry	*kern_dev_table;
static struct timer_map		*timer_list, *ktimer_list;
static int			 ncallout;
static int			*io_mem_map_dup;
static struct eirrswitch	eirr_switch[32];
static struct int_dispatch	int_dispatch[32];
static unsigned			int_always;
static unsigned			int_compl;
static unsigned			int_direct;
static char			*no_name = "no name";
static char			*no_mgr  = "no_mgr";
static char			*no_mod  = "no_mod";

extern void			exit();

/* message frame log table defines */
struct	frameinfo {
	char	f_type;
	int	f_frame;
	int	f_imcport;
	int	f_imcpdt;
} *frameinfo;

#define	FLOST	0
#define	FFREE	1
#define	FQUEUED 3

typedef struct pool_entry {
    msg_frame_ptr   backlink;
    io_message_type msg;
} pool_entry;

an_ioserv_help(o)	/* Just to give you an idea of what this file does... */
    FILE *o;
{
    fprintf(o, "\nvalid options:\n");
    fprintf(o, "   sum             -- summary (ioheader)\n");
    fprintf(o, "   imcport [port#] -- display port structure\n");
    fprintf(o, "   frame [frame#]  -- display message frame\n");
    fprintf(o, "   em_fe           -- display I/O pages in emulated fe mode\n");
    fprintf(o, "   detail          -- do a detailed check\n");
    fprintf(o, "   sanity          -- do a sanity check\n");
    fprintf(o, "   mem [buf size]  -- check memory freelists\n");
    fprintf(o, "   class [n]       -- show I/O class table\n");
    fprintf(o, "   eirr [n]        -- show eirr bit allocation\n");
    fprintf(o, "   tree [n]        -- show I/O tree\n");
    fprintf(o, "   kerndev         -- show kernel device table\n");
    fprintf(o, "   native          -- show native module table\n");
    fprintf(o, "   mgr [n]         -- show manager table\n");
    fprintf(o, "   mod [n]         -- show software module table\n");
    fprintf(o, "   timer [n]       -- show active timers\n");
    fprintf(o, "   ds              -- class+io_tree+mod+mgr+kerndev+native\n");
    fprintf(o, "   oldtree [n]     -- display old iotree entry\n");
    fprintf(o, "\n");
    fprintf(o, "   basic           -- do summary, cursory io_tree+native,");
    fprintf(o, " em_fe, & sanity checks\n");
    fprintf(o, "   all             -- do all of the above\n");
    fprintf(o, "   help            -- print this help screen\n");
}

static struct ktol_map {	/* Kernel to local pointer mapping */
    unsigned	kernel;
    caddr_t	local;
} *ktol_map;
static int ktol_size = 0, max_ktol_size;

void
insert_ktol(kaddr, laddr)
    unsigned kaddr;
    caddr_t  laddr;
{
    if (ktol_size < max_ktol_size) {
	ktol_map[ktol_size].kernel = kaddr;
	ktol_map[ktol_size].local  = laddr;
	ktol_size++;
    }
    else
	printf("ktol not big enough...\n");
}

int
ktol_compare(a, b)
    struct ktol_map *a, *b;
{
    return((int)a->kernel - (int)b->kernel);
}
    
caddr_t
an_ktol(kaddr)
    caddr_t kaddr;
{
    int low, high, mid, index;

    if (kaddr == 0) return(0);

    for (low = 0, high = ktol_size, index = NONE; low <= high; ) {
	mid = (low + high) >> 1;
	if ((unsigned)kaddr < ktol_map[mid].kernel)
	    high = mid - 1;
	else if ((unsigned)kaddr > ktol_map[mid].kernel)
	    low = mid + 1;
	else {
	    index = mid;
	    break;
	}
    }
    if (index == NONE) printf("an_ktol: can't find address 0x%x\n", kaddr);
    return ((index == NONE) ? 0 : ktol_map[index].local);
}

/*ARGSUSED*/
io_init(outf)
    FILE *outf;
{
    unsigned		 addr;
    int			 i;
    aio_tree_entry	 *next_io_tree_entry;
    aio_native_mod_entry *next_native_mod_entry;
    extern void		 *calloc(), qsort();
#   define		 CALLOC(a, b)	calloc((size_t)(a), (b))

    /* Grab the message control structure */
    (void)an_grab_extern("Msg_control", &addr);
    (void)an_vtor(0, addr, (int *)&addr);
    if (an_grab_real_chunk(addr, Msg_control, sizeof(Msg_control))) {
   	perror("io_init: Msg_control read");
   	if (!tolerate_error) exit(1);
    }

    /* Grab the port directory */
    vport_index =
	(imc_port_ptr *)CALLOC(Msg_control.max_port_num, sizeof(imc_port_ptr));
    if (an_grab_virt_chunk(0, Msg_control.port_index, (char *)vport_index,
		(int)(Msg_control.max_port_num * sizeof(imc_port_ptr))))  {
	perror("io_init: port_index read");
	if (!tolerate_error) exit(1);
    }
    port_index =
	(imc_port_ptr)CALLOC(Msg_control.max_port_num, sizeof(imc_port));
    for (i = 1; i < Msg_control.max_port_num; i++) {
	if (vport_index[i] == 0) continue;
	if (an_grab_virt_chunk(0, vport_index[i], (char *)&port_index[i],
		sizeof(imc_port))) {
	    printf("can't get imc_port entry!\n");
	    break;
	}
    }

    /* Grab the pseudo-port directory */
    vpseudo_port_index =
      (io_tree_entry **)CALLOC(Msg_control.max_pseudo, sizeof(io_tree_entry *));
    if (an_grab_virt_chunk(0, Msg_control.pseudo_port_index,
		(char *)vpseudo_port_index,
		(int)(Msg_control.max_pseudo * sizeof(io_tree_entry *))))  {
	perror("io_init: pseudo_port_index read");
	if (!tolerate_error) exit(1);
    }
    pseudo_port_index =
      (aio_tree_entry **)CALLOC(Msg_control.max_pseudo, sizeof(io_tree_entry *));

    /* Grab the ioheader */
    (void)an_grab_extern("ioheader", &addr);
    if (an_grab_real_chunk(addr, (char *)&ioheader, sizeof(ioheader)))  {
	perror("io_init: ioheader read");
	if (!tolerate_error) exit(1);
    }
    /* Fudge the number of names I need */
    ioheader.num_iotree_names = ioheader.num_io_class_recs +
	ioheader.num_io_mgr_recs + ioheader.num_io_sw_mod_recs;

    /* Initialize the kernel-to-local pointer mapping table */
    max_ktol_size = ioheader.num_iotree_names	/* Room for names */
		  + ioheader.num_io_mgr_recs	/* Room for io_mgr_table */
		  + ioheader.num_io_sw_mod_recs	/* Room for io_mod_table */
		  + ioheader.num_io_class_recs	/* Room for io_class_table */
		  + ioheader.num_io_tree_recs	/* Room for io_tree */
		  + ioheader.num_io_hw_mod_recs;/* Room for native_mod_table */
    ktol_map = (struct ktol_map *)CALLOC(max_ktol_size, sizeof(*ktol_map));

    /* Grab the names table */
    io_names = (aio_name_entry *)CALLOC(ioheader.num_iotree_names,
	sizeof(aio_name_entry));
    if (an_grab_extern("io_names", &addr)) {
	perror("io_init: io_names read");
	if (!tolerate_error) exit(1);
    }
    for (i = 0; i < ioheader.num_iotree_names && addr != 0; i++) {
	io_name_entry kentry;

	if (an_grab_virt_chunk(0, addr, (char *)&kentry, sizeof(kentry))) {
	    printf("can't get io_name entry!\n");
	    break;
	}
	insert_ktol(addr, &io_names[i]);
	(void)strcpy(io_names[i], kentry.name);
	addr = (unsigned)kentry.next;
    }

    /* Grab the class table */
    io_class_ptr = (aio_class_entry *)CALLOC(ioheader.num_io_class_recs,
	sizeof(aio_class_entry));
    if (an_grab_extern("io_class_ptr", &addr)) {
	perror("io_init: io_class_ptr read");
	if (!tolerate_error) exit(1);
    }
    if (an_grab_virt_chunk(0, addr, (char *)&addr, sizeof(addr))) {
	perror("io_init: *io_class_ptr read");
	if (!tolerate_error) exit(1);
    }
    for (i = 0; i < ioheader.num_io_class_recs && addr != 0; i++) {
	io_class_entry kentry;

	if (an_grab_virt_chunk(0, addr, (char *)&kentry, sizeof(kentry))) {
	    printf("Can't get io_class entry!\n");
	    break;
	}
	insert_ktol(addr, &io_class_ptr[i]);
	io_class_ptr[i].kname		= kentry.name;
	io_class_ptr[i].kclass_link	= kentry.class_link;
	io_class_ptr[i].options		= kentry.options;
	addr = (unsigned)kentry.next;
    }

    /* Grab the manager table */
    io_mgr_ptr = (aio_mgr_entry *)CALLOC(ioheader.num_io_mgr_recs,
	sizeof(aio_mgr_entry));
    if (an_grab_extern("io_mgr_ptr", &addr)) {
	perror("io_init: io_mgr_ptr read");
	if (!tolerate_error) exit(1);
    }
    if (an_grab_virt_chunk(0, addr, (char *)&addr, sizeof(addr))) {
	perror("io_init: *io_mgr_ptr read");
	if (!tolerate_error) exit(1);
    }
    for (i = 0; i < ioheader.num_io_mgr_recs && addr != 0; i++) {
	io_mgr_entry kentry;

	if (an_grab_virt_chunk(0, addr, (char *)&kentry, sizeof(kentry))) {
	    printf("Can't get io_mgr entry!\n");
	    break;
	}
	insert_ktol(addr, &io_mgr_ptr[i]);
	io_mgr_ptr[i].kname	= kentry.name;
	io_mgr_ptr[i].entry	= kentry.entry;
	io_mgr_ptr[i].attach	= kentry.attach;
	io_mgr_ptr[i].init	= kentry.init;
	io_mgr_ptr[i].options	= kentry.options;
	io_mgr_ptr[i].b_major	= kentry.b_major;
	io_mgr_ptr[i].c_major	= kentry.c_major;
	io_mgr_ptr[i].kmi_link	= kentry.mi_link;
	addr = (unsigned)kentry.next;
    }

    /* Grab the io module table */
    io_mod_ptr = (aio_mod_entry *)CALLOC(ioheader.num_io_sw_mod_recs,
	sizeof(aio_mod_entry));
    if (an_grab_extern("io_mod_ptr", &addr)) {
	perror("io_init: io_mod_ptr read");
	if (!tolerate_error) exit(1);
    }
    if (an_grab_virt_chunk(0, addr, (char *)&addr, sizeof(addr))) {
	perror("io_init: *io_mod_ptr read");
	if (!tolerate_error) exit(1);
    }
    for (i = 0; i < ioheader.num_io_sw_mod_recs && addr != 0; i++) {
	io_mod_entry kentry;

	if (an_grab_virt_chunk(0, addr, (char *)&kentry, sizeof(kentry))) {
	    printf("Can't get io_mod entry!\n");
	    break;
	}
	insert_ktol(addr, &io_mod_ptr[i]);
	io_mod_ptr[i].kname	= kentry.name;
	io_mod_ptr[i].kclass	= kentry.class;
	io_mod_ptr[i].options	= kentry.options;
	io_mod_ptr[i].idy	= kentry.idy;
	addr = (unsigned)kentry.next;
    }

    /* Grab the io_tree */
    io_tree_ptr = (aio_tree_entry *)CALLOC(ioheader.num_io_tree_recs,
	sizeof(aio_tree_entry));
    if (an_grab_extern("io_tree_ptr", &addr)) {
	perror("io_init: io_tree_ptr read");
	if (!tolerate_error) exit(1);
    }
    if (an_grab_virt_chunk(0, addr, (char *)&addr, sizeof(addr))) {
	perror("io_init: *io_tree_ptr read");
	if (!tolerate_error) exit(1);
    }
    next_io_tree_entry = &io_tree_ptr[-1];
    an_get_io_tree_entry(addr, &next_io_tree_entry);

    /* Grab the native_mod_table */
    native_mod_ptr = (aio_native_mod_entry *)CALLOC(ioheader.num_io_hw_mod_recs,
        sizeof(aio_native_mod_entry));
    if (an_grab_extern("native_mod_ptr",&addr)) {
	perror("io_init: native_mod_ptr read");
	if (!tolerate_error) exit(1);
    }
    if (an_grab_virt_chunk(0, addr, (char *)&addr, sizeof(unsigned))) {
	perror("io_init: *native_mod_ptr read");
	if (!tolerate_error) exit(1);
    }
    next_native_mod_entry = &native_mod_ptr[-1];
    an_get_native_mod_entry(addr, &next_native_mod_entry);

    /* Grab the kernel device table */
    kern_dev_table = (aio_kern_dev_entry *)CALLOC(ioheader.num_kern_dev_recs,
	sizeof(aio_kern_dev_entry));
    if (an_grab_extern("kern_dev_table", &addr)) {
	perror("io_init: kern_dev_table read");
	if (!tolerate_error) exit(1);
    }
    for (i = 0; i < ioheader.num_kern_dev_recs; i++) {
	kern_dev_entry kentry;

	if (an_grab_virt_chunk(0, addr, (char *)&kentry, sizeof(kentry))) {
	    printf("Can't get kern_dev_table entry!\n");
	    break;
	}
	kern_dev_table[i].knode		= kentry.node;
	kern_dev_table[i].kgen_node	= kentry.gen_node;
	kern_dev_table[i].flags		= kentry.flags;
	kern_dev_table[i].dev_num	= kentry.dev_num;
	kern_dev_table[i].gen_dev_num	= kentry.gen_dev_num;
	addr += sizeof(kern_dev_entry);
    }

    /* Grab the timer table */
    if (an_grab_extern("ncallout", &addr)) {
	perror("io_init: ncallout");
	if (!tolerate_error) exit(1);
    }
    else if (an_grab_real_chunk(addr, (char *)&ncallout, sizeof(ncallout))) {
	perror("io_init: *ncallout");
	if (!tolerate_error) exit(1);
    }
    timer_list = (struct timer_map *)CALLOC(ncallout, sizeof(struct timer_map));
    (void)an_grab_extern("timer_list", &addr);
    ktimer_list = (struct timer_map *)addr;
    if (an_grab_virt_chunk(0, addr, (char *)timer_list,
	(int)(ncallout * sizeof(struct timer_map)))) {
	perror("io_init: timer_list read");
	if (!tolerate_error) exit(1);
    }

    /* Grab the eirr_switch table */
    (void)an_grab_extern("eirr_switch", &addr);
    if (an_grab_virt_chunk(0, addr, (char *)eirr_switch, sizeof(eirr_switch))) {
	perror("io_init: eirr_switch read");
	if (!tolerate_error) exit(1);
    }

    /* Grab the int_dispatch table */
    (void)an_grab_extern("int_dispatch", &addr);
    if (an_grab_virt_chunk(0, addr, (char *)int_dispatch,
		sizeof(int_dispatch))) {
	perror("io_init: int_dispatch read");
	if (!tolerate_error) exit(1);
    }

    /* Grab some known int completion routine addresses */
    (void)an_grab_extern("int_compl",  &int_compl);
    (void)an_grab_extern("int_direct", &int_direct);
    (void)an_grab_extern("int_always", &int_always);

    /* Now sort the ktol mapping */
    qsort((void *)ktol_map, (size_t)ktol_size, sizeof(*ktol_map), ktol_compare);

    /* And fix all the local versions of kernel pointers */
    for (i = 2; i < Msg_control.max_pseudo; i++)
	pseudo_port_index[i] = (aio_tree_entry *)an_ktol(vpseudo_port_index[i]);

    for (i = 0; i < ioheader.num_io_class_recs; i++) {
	io_class_ptr[i].name	   = an_ktol(io_class_ptr[i].kname);
	io_class_ptr[i].class_link =
		(aio_tree_entry *)an_ktol(io_class_ptr[i].kclass_link);
    }
    for (i = 0; i < ioheader.num_io_mgr_recs; i++) {
	io_mgr_ptr[i].name	= an_ktol(io_mgr_ptr[i].kname);
	io_mgr_ptr[i].mi_link	=
		(aio_tree_entry *)an_ktol(io_mgr_ptr[i].kmi_link);
    }
    for (i = 0; i < ioheader.num_io_sw_mod_recs; i++) {
	io_mod_ptr[i].name	= an_ktol(io_mod_ptr[i].kname);
	io_mod_ptr[i].class	=
		(aio_class_entry *)an_ktol(io_mod_ptr[i].kclass);
    }
    for (i = 0; i < ioheader.num_io_tree_recs; i++) {
	io_tree_ptr[i].parent	  =
		(aio_tree_entry *)an_ktol(io_tree_ptr[i].kparent);
	io_tree_ptr[i].child	  =
		(aio_tree_entry *)an_ktol(io_tree_ptr[i].kchild);
	io_tree_ptr[i].sibling	  =
		(aio_tree_entry *)an_ktol(io_tree_ptr[i].ksibling);
	io_tree_ptr[i].mi_link	  =
		(aio_tree_entry *)an_ktol(io_tree_ptr[i].kmi_link);
	io_tree_ptr[i].class_link =
		(aio_tree_entry *)an_ktol(io_tree_ptr[i].kclass_link);
	io_tree_ptr[i].manager	  =
		(aio_mgr_entry *)an_ktol(io_tree_ptr[i].kmanager);
	io_tree_ptr[i].module	  =
		(aio_mod_entry *)an_ktol(io_tree_ptr[i].kmodule);
    }
    for (i = 0; i < ioheader.num_io_hw_mod_recs; i++) {
	native_mod_ptr[i].next	  = (aio_native_mod_entry *)
	    an_ktol(native_mod_ptr[i].entry.next);
	if (native_mod_ptr[i].entry.iodc_type.type == MOD_TYPE_BUSCONV) {
	    native_mod_ptr[i].next_bus	 = (aio_native_mod_entry *)
		an_ktol(native_mod_ptr[i].entry.type.bc.next_bus);
	    native_mod_ptr[i].other_port = (aio_native_mod_entry *)
		an_ktol(native_mod_ptr[i].entry.type.bc.other_port);
	}
    }
    for (i = 0; i < ioheader.num_kern_dev_recs; i++) {
	kern_dev_table[i].node
		= (aio_tree_entry *)an_ktol(kern_dev_table[i].knode);
	kern_dev_table[i].gen_node
		= (aio_tree_entry *)an_ktol(kern_dev_table[i].kgen_node);
    }

    /* Get the message pool */
    /* Get space for usage log tables */
    frameinfo = (struct frameinfo *)CALLOC(Msg_control.num_msgs,
		sizeof(struct frameinfo));
    first_frame = (msg_frame_ptr)CALLOC(Msg_control.num_msgs,
	(sizeof(pool_entry) + Msg_control.msg_length));
    vfirst_frame = Msg_control.first_frame;
    vlast_frame  = vfirst_frame + Msg_control.num_msgs;
    io_snapshot();

    /* figure out how many ports we really have */
    for (max_port = Msg_control.max_port_num-1; max_port > 0; max_port--)
	if (vport_index[max_port] != 0)
	    break;

#ifdef	notdef
    /* load the io_mem_map structures */
    (void)an_grab_extern("io_mem_buf",&io_mem_buf);
    io_mem_buf = (io_mem_buf + NBPG - 1) & ~(NBPG - 1);
    (void)an_grab_extern("p_io_mem_buf",&addr);
    if (an_grab_virt_chunk(0,addr,&io_mem_top,4)) {
	perror("io_init: p_io_mem_buf: read");
	if (!tolerate_error) exit(1);
    }
    io_mem_map = (int *)CALLOC((io_mem_top-io_mem_buf)/32,4);
    io_mem_map_dup = (int *)CALLOC((io_mem_top-io_mem_buf)/32,4);
    an_ioserv_set_up_mem();
#endif	/*notdef*/

#ifndef	NO_OLD_IOTREE
    /* grab the iotree */
    iotree = (struct iotree_type *)CALLOC(ioheader.num_iotree_recs,
					  sizeof(struct iotree_type));
    (void)an_grab_extern("iotree",&addr);
    if (an_grab_real_chunk(addr,(char *)iotree,
                 (int)(ioheader.num_iotree_recs*sizeof(struct iotree_type)))) {
	perror("io_init: iotree read");
	if (!tolerate_error) exit(1);
    }

    /* grab the mgr_table */
    mgr_table = (struct mgr_table_type *)
          CALLOC(ioheader.num_mgr_table_recs,sizeof(struct mgr_table_type));
    (void)an_grab_extern("mgr_table",&addr);
    if (an_grab_real_chunk(addr,(char *)mgr_table,
          (int)(ioheader.num_mgr_table_recs * sizeof(struct mgr_table_type)))) {
	perror("io_init: mgr_table read");
	if (!tolerate_error) exit(1);
    }
#endif	/*NO_OLD_IOTREE*/
}

an_get_io_tree_entry(addr, entry)
    unsigned addr;
    aio_tree_entry **entry;
{
    io_tree_entry kentry;
    io_conn_entry conn;
    unsigned start = addr;

    do {
	(*entry)++;
	if (an_grab_virt_chunk(0, addr, (char *)&kentry, sizeof(kentry))) {
	    printf("Can't get io_tree entry!\n");
	    return;
	}
	insert_ktol(addr, *entry);
	(*entry)->kparent	= kentry.parent;
	(*entry)->ksibling	= kentry.sibling;
	(*entry)->kchild	= kentry.child;
	(*entry)->conn		= kentry.conn;
	(*entry)->ent_id	= kentry.ent_id;
	(*entry)->mgr_index	= kentry.mgr_index;
	(*entry)->lu		= kentry.lu;
	(*entry)->port		= kentry.port;
	(*entry)->state		= kentry.state;
	(*entry)->hdw_address	= kentry.hdw_address;
	(*entry)->kmi_link	= kentry.mi_link;
	(*entry)->kclass_link	= kentry.class_link;

	/* Convert the `conn' entry to manager & module pointers */
	if (an_grab_virt_chunk(0, kentry.conn, (char *)&conn, sizeof(conn)))
	    printf("Can't get io connectivity entry!\n");
	else {
	    (*entry)->kmanager = conn.manager;
	    (*entry)->kmodule  = conn.module;
	}
	if (kentry.child != NULL_IO_TREE_PTR)
	    an_get_io_tree_entry(kentry.child, entry);
    } while ((addr = (unsigned)kentry.sibling) != start);
}

an_get_native_mod_entry(addr, entry)
    unsigned		 addr;
    aio_native_mod_entry **entry;
{
    native_mod_entry kentry;

    while (addr != 0) {
	(*entry)++;
	if (an_grab_virt_chunk(0, addr, (char *)&kentry, sizeof(kentry))) {
	    printf("Can't get native_module entry!\n");
	    return;
	}
	insert_ktol(addr, *entry);
	(*entry)->entry = kentry;
	(*entry)->next	= (aio_native_mod_entry *)kentry.next;
	if (kentry.iodc_type.type == MOD_TYPE_BUSCONV)
	    an_get_native_mod_entry(kentry.type.bc.next_bus, entry);
	addr = (unsigned)kentry.next;
    }
}

io_snapshot() {
    unsigned addr; 

    /* get message pool */
    (void)an_vtor(0, (unsigned)vfirst_frame, (int *)&addr);
    if (an_grab_real_chunk(addr, (char *)first_frame, (int)(Msg_control.num_msgs
	* (sizeof(struct pool_entry) + Msg_control.msg_length)))) {
	perror("message_pool: read");
	if (!tolerate_error) exit(1);
    }
}

imc_port_ptr
port_to_imc_ptr(port_num)
    port_num_type port_num;
{
    if (port_num > 0 && port_num <= max_port)
	return(&port_index[port_num]);
    else
	return(0);
}

aio_tree_entry *
port_to_io_tree(port_num)
    port_num_type port_num;
{
    imc_port_ptr port_ptr;

    if ((port_ptr = port_to_imc_ptr(port_num)) != 0)
	return((aio_tree_entry *)an_ktol(port_ptr->io_tree_ptr));
    else if (port_num < 0 && -port_num <= Msg_control.max_pseudo)
	return((aio_tree_entry *)an_ktol(vpseudo_port_index[-port_num]));
    else
	return(0);
}

int
node_to_index(node)
    aio_tree_entry *node;
{
    int rec, start;

    if (node->state.from_gen == FALSE) return(NONE);

    if (node->parent == io_tree_ptr)    /* root of tree */
	start = 0;
    else {
	if ((start = node_to_index(node->parent)) == NONE) return(NONE);
	start = iotree[start].child;
    }
    rec = start;
    do {
	if (iotree[rec].parm == node->hdw_address) break;
    } while ((rec = iotree[rec].sibling) != start);
    return(rec);
}

#ifndef	NO_OLD_IOTREE
aio_tree_entry *
index_to_node(index)
    int index;
{
    aio_tree_entry *node, *start;

    if (index == 0)
	start = io_tree_ptr->child;
    else {
	if ((start = index_to_node(iotree[index].parent)) == 0) return(0);
	start = start->child;
    }
    node = start;
    do {
	if (node->hdw_address == iotree[index].parm) break;
    } while ((node = node->sibling) != start);
    return(node);
}
#endif	/*NO_OLD_IOTREE*/

cleariotables() {
    bzero(frameinfo, (int)(Msg_control.num_msgs * sizeof(struct frameinfo)));
}

display_frame(param2, opt, redir, path, outf)
    int 	param2;
    char 	opt;
    int 	redir;
    char 	*path;
    FILE	*outf;
{
    msg_frame_ptr 	addr;

    an_mgr_do_init(outf);
    fprintf(outf, "\n");

    if (redir) {
	if ((outf = fopen(path, ((redir == 2) ? "a+" : "w+"))) == NULL) {
	    fprintf(stderr, "Can't open file "); perror(path);
	    goto out;
	}
    }

    switch(opt) {
	case 'n': /* index into swbuf table */
	    if (param2 < 0 || param2 >= Msg_control.num_msgs) {
		fprintf(outf," Frame index out of range\n");
		goto reset;
	    }
	    addr = &first_frame[param2];
	    param2 = (int)vfirst_frame + (param2 * sizeof(msg_frame));
	    break;
	
	case '\0' : /* Check validity of proc address */
	    if (param2 < (int)vfirst_frame || param2 > (int)vlast_frame) {
		fprintf(outf, " frame address out of range\n");
		goto reset;
	    }
	    addr = &first_frame[(param2-(int)vfirst_frame)/sizeof(msg_frame)];
	    break;

	default:
	    /* bad option */
	    fprintf(outf, " bad option <%c>\n", opt);
	    goto reset;

    }
    dumpframe(addr, (msg_frame_ptr)param2, outf);

reset: /* close file if we redirected output */
    if (redir) (void)fclose(outf);

out:
    outf = stdout;
}

display_imcport(param2, opt, redir, path, outf)
    int 	param2;
    char 	opt;
    int 	redir;
    char 	*path;
    FILE	*outf;
{
    imc_port_ptr 	addr;
    int			port_num;

    an_mgr_do_init(outf);

    if (redir) {
	if ((outf = fopen(path, ((redir == 2) ? "a+" : "w+"))) == NULL) {
	    fprintf(stderr, "Can't open file "); perror(path);
	    goto out;
	}
    }

    switch (opt) {
	case 'n': /* index into port table */
	    port_num = param2;
	    if (port_num < 0 || port_num > max_port) {
		fprintf(outf," port number out of range\n");
		goto out;
	    }
	    addr = vport_index[port_num];
	    break;
	
	case '\0': /* Check validity of port address */
 	    addr = (imc_port_ptr)param2;
	    for (port_num = 1; port_num <= max_port; port_num++)
		if (vport_index[port_num] == addr) break;
	    if (port_num >= max_port) {
		fprintf(outf," Port directory address out of range\n");
		goto out;
	    }
	    break;

	default: /* bad option */
	    fprintf(outf," bad option \n");
	    goto out;
    }
    dumpimcport(&port_index[port_num], addr, port_num, outf);

out: /* close file if we redirected output */
    if (redir) (void)fclose(outf);
    outf = stdout;
}

/* dump contents of imcport */
dumpimcport(pt, vpt, port_num, outf)
    imc_port_ptr 	pt, vpt;
    port_num_type	port_num;
    FILE		*outf;
{
    int 		 i, first = 0;
    struct mgr_info_type mgr;

    fprintf(outf," Port number %d (0x%x)     ", port_num, port_num);
    if (aio_get_mgr_info(AIO_PORT_NUM_TYPE, port_num, &mgr) != AIO_OK)
	fprintf(outf, "%s\n", mgr.mgr_name);
    else
	fprintf(outf, "%s    lu %d    h/w addr %s\n", mgr.mgr_name,
		mgr.io_tree_entry->lu, mgr.hw_address);

    fprintf(outf, "  imcport address:        0x%08x   ", vpt);
    fprintf(outf, "  entrypoint: 0x%08x (", pt->server_entry);
    an_display_sym((unsigned)pt->server_entry, outf, AIO_SYMBOL_AND_OFFSET);
    fprintf(outf, ")\n");
    fprintf(outf, "  enabled subqueues:      0x%08x\n", pt->enabled_subqueues);
    fprintf(outf, "  active subqueues:       0x%08x\n", pt->active_subqueues);
    fprintf(outf, "  pda address:            0x%08x     length: %d (0x%x) bytes\n",
	pt->pda, pt->pda_length, pt->pda_length);

    if (pt->io_tree_ptr != NULL_IO_TREE_PTR)
	fprintf(outf, "  io_tree_ptr:            0x%x [%2d]\n", pt->io_tree_ptr,
		(aio_tree_entry *)an_ktol(pt->io_tree_ptr) - io_tree_ptr);

    for (i = 0; i < 32; i++) {
	fb_list_ptr subq_end = (fb_list_ptr)((int)vpt +
		(int)&pt->subqueue[i].f - (int)pt);

	if (pt->subqueue[i].f != subq_end) {
	    if (first++ == 0) fprintf(outf,"  active subqueues:\n");
	    fprintf(outf, "          [%2d]  0x%08x\n", i, pt->subqueue[i].f);
	}
    }
    fprintf(outf,"\n");
}

/* dump contents of frame */
dumpframe(f, vf, outf)
    msg_frame_ptr 	f, vf;
    FILE		*outf;
{
    fprintf(outf, "    Frame address: 0x%08x \n", vf);
    fprintf(outf, "    forward link 0x%08x  backward link 0x%08x\n",
			f->link.f,		f->link.b);
    fprintf(outf, "    data         0x%08x  length        0x%08x\n",
			f->data, 		f->length);
}

/* dump contents of message*/
dumpmessage(m, vm, length, outf)
    llio_std_header_type *m, *vm;
    unsigned int	length;
    FILE		*outf;
{
    int i, j, *data;

    fprintf(outf, "        Message address: 0x%08x\n", vm);

    if (aio_decode_message((unsigned)vm, "        ", outf) == AIO_NO_DECODING) {

	if (length != 0) {
	    fprintf(outf, "\n        Message Data\n       ");
	    /* get start of data in message */
	    data = (int *)((int)m + sizeof(llio_std_header_type));


	    for (i = j = 0; i < length; i += 4) {
		fprintf(outf, " 0x%08x ", *data++);
		if (j++ == 5) {
		    j = 0;
		    fprintf(outf,"\n       ");
		}
	    }
	}
    }
    fprintf(outf,"\n");
}

/* Logswbuf, logs an entry and sees if it has been used before */
logframe(fp, vfp, type, imcport, imcpdt, outf)
    msg_frame_ptr 	fp, vfp;
    int 		type;
    int 		imcport;
    int 		imcpdt;
    FILE		*outf;
{
    int 		indx = fp - first_frame;

    if (frameinfo[indx].f_type != FLOST) {

	fprintf(outf,"ERROR:  duplicate message frame\n");

	fprintf(outf, "index %x fp %x msg_control.num_msgs %x\n", indx, fp,
	        Msg_control.num_msgs);

	dumpframe(fp, vfp, outf);
	fprintf(outf, "   Previous instance:");
	fprintf(outf, "   imcpdt 0x%08x  imcport 0x%08x  type  %d\n",
	        frameinfo[indx].f_imcpdt,frameinfo[indx].f_imcport,
	        frameinfo[indx].f_type);
	return(1);

    }

    if ((int)fp->link.f == 0) {
	fprintf(outf,"ERROR: frame has forward pointer of NIL !!\n");
	fprintf(outf," Might be lost!!\n");
	dumpframe(fp, vfp, outf);
    }

    frameinfo[indx].f_type  = type;
    frameinfo[indx].f_frame = (int)fp;
    frameinfo[indx].f_imcport = imcport;
    frameinfo[indx].f_imcpdt  = imcpdt;
    return(0);
}

iocheck(detail, outf)
    int		detail;
    FILE	*outf;
{
    int ipdt;

    an_mgr_do_init(outf);

    /* print header */
    fprintf(outf, "\n*************************************************\n");
    if (detail)
	fprintf(outf, "I/O Services Port and Message Structures\n");
    else
	fprintf(outf, "I/O Services Sanity Check\n");
    fprintf(outf, "*************************************************\n\n");

    /* scan ports */
    if (detail)
	fprintf(outf, "Scanning all ports:\n\n");
    else
	fprintf(outf, "Scanning active ports:\n\n");

    for (ipdt = 0; ipdt <= max_port;  ipdt++)
	an_ioserv_check_port(ipdt, detail + 2, outf);

    /* scan down standard free list */
    fprintf(outf, "Scanning message frame freelist:\n\n");
    an_ioserv_scan_free_list(detail, outf);

    /* scan for lost frames in standard pool */
    fprintf(outf, "Scanning for lost message frames:\n\n");
    an_ioserv_scan_lost_frames(outf);
}

an_ioserv_scan_lost_frames(outf)
    FILE *outf;
{
    int i;

    for (i = 0; i < Msg_control.num_msgs; i++) {
	if (frameinfo[i].f_type == FLOST) {
	    llio_std_header_type *vmsg_headr, *lmsg_headr;

	    fprintf(outf,"ERROR:  message frame (standard pool) lost\n");
	    vmsg_headr = (llio_std_header_type *)first_frame[i].data;
	    lmsg_headr = (llio_std_header_type *)
		((int)first_frame + (int)&vfirst_frame[i] - (int)vfirst_frame);
	    dumpframe(&first_frame[i], &vfirst_frame[i], outf);
	    dumpmessage(lmsg_headr, vmsg_headr, first_frame[i].length, outf);
	}
    }
}

an_ioserv_scan_free_list(detail, outf)
    int		detail;
    FILE	*outf;
{
    msg_frame_ptr vmsg_frame, lmsg_frame;

    /* get head of free list */
    vmsg_frame = (msg_frame_ptr)Msg_control.pool;

    /* if no messages on free list, print a warning */
    if ((int)vmsg_frame == EOC) fprintf(outf," WARNING: out of messages!!\n");

    /* walk through free list and keep track of what we've seen */
    while ((int)vmsg_frame != EOC)  {

	/* log the fact that we've seen this message frame */
	lmsg_frame = (msg_frame_ptr)
		((int)first_frame + (int)vmsg_frame - (int)vfirst_frame);
	if (logframe(lmsg_frame, vmsg_frame, FFREE, 0, 0, outf))  {
	    fprintf(outf,"\nERROR: terminating scan to prevent looping\n\n");
	    break;
	}

	/* print the frame and message if details are wanted */
	if (detail) {
	    llio_std_header_type 	*vmsg_headr, *lmsg_headr;

	    vmsg_headr = (llio_std_header_type *)lmsg_frame->data;
	    lmsg_headr = (llio_std_header_type *)
		((int)first_frame + (int)vmsg_headr - (int)vfirst_frame);
	    dumpframe(lmsg_frame, vmsg_frame, outf);
	    dumpmessage(lmsg_headr, vmsg_headr, lmsg_frame->length, outf);
	}

	/* advance to the next element of the free list */
	vmsg_frame = (msg_frame_ptr)lmsg_frame->link.f;
    }
}

#define	print_header	\
    if (printed == 0) {	\
	dumpimcport(port_ptr, vport_index[port_num], port_num, outf);	\
	printed = 1;	\
    }
	
an_ioserv_check_port(port_num, mode, outf)
    int		port_num, mode;
		   /*  1 = just log (unless error detected) */
		   /*  2 = print busy ports only + errors   */
		   /*  3 = print everything                 */
    FILE	*outf;
{
    int			printed = 0, subq, count;
    imc_port_ptr	port_ptr;

    /* make sure this is a valid port number */
    if ((port_ptr = port_to_imc_ptr(port_num)) == 0) return;

    /* print port if always printing */
    if (mode == 3) print_header

    /* walk through subqueues looking for messages */
     for (subq = 0; subq < 32; subq++) {
	msg_frame_ptr 	vmsg_frame, lmsg_frame, subq_end;

	/* grab the subq head */
	vmsg_frame = (msg_frame_ptr)port_ptr->subqueue[subq].f;
	subq_end = (msg_frame_ptr)((int)vport_index[port_num] +
  		(int)&port_ptr->subqueue[subq].f - (int)port_ptr);
	count = 0;

	/* do a sanity check on the first frame on this subq */
	if ((vmsg_frame < vfirst_frame || vmsg_frame > vlast_frame) &&
	    vmsg_frame != subq_end) {
	    print_header
	    fprintf(outf,"ERROR:  bogus subq pointer (subq %d)!\n", subq);
	    return;
	}

	/* walk through each msg frame on this subq */
	while (vmsg_frame != subq_end) {

	    /* print imcport if not already done (and it is wanted) */
	    if (mode != 1) print_header

	    /* print frame if it is wanted */
	    lmsg_frame = (msg_frame_ptr)
		((int)first_frame + ((int)vmsg_frame - (int)vfirst_frame));
	    if (mode != 1) {
		if (count++ == 0) fprintf(outf,"\n    Subqueue[%d]:\n", subq);
		dumpframe(lmsg_frame, vmsg_frame, outf);
	    }

	    /* log the frame and print a warning if there is a problem */
	    if (logframe(lmsg_frame, vmsg_frame, FQUEUED, vport_index[port_num],
			port_num, outf))  {
		print_header
		fprintf(outf, "ERROR: scan of subq %d terminated to prevent looping\n", subq);
		break;
	    }

	    /* print message if it is wanted */
	    if (mode != 1) {
		llio_std_header_type 	*vmsg_hdr, *lmsg_hdr;

		vmsg_hdr = (llio_std_header_type *)lmsg_frame->data;
		lmsg_hdr = (llio_std_header_type *)
			((int)first_frame + (int)vmsg_hdr - (int)vfirst_frame);
		dumpmessage(lmsg_hdr, vmsg_hdr, lmsg_frame->length, outf);
	    }

	    /* grab the next frame and check for errors */
	    vmsg_frame = (msg_frame_ptr)lmsg_frame->link.f;
	    if ((vmsg_frame < vfirst_frame || vmsg_frame > vlast_frame) &&
		 vmsg_frame != subq_end) {
		print_header
		fprintf(outf,"ERROR:  bad forward pointer on subq %d\n", subq);
		break;
	    }
	}
    }
}

an_ioserv_set_up_mem() {
    int		i, j;
    unsigned	addr, next_addr;

    /* run through equivalently mapped free lists */
    for (i = 1; i < 12; i++)  {

	/* grab appropriate list head */
        if (an_grab_extern("io_eqmem_heads", &next_addr)) {
	    fprintf(stderr,"couldn't get io_eqmem_heads!!\n");
	    break;
	}
        if (an_grab_virt_chunk(0, next_addr+(4*i), (char *)&addr, 4)) {
	    fprintf(stderr,"couldn't get io_eqmem_heads!!\n");
	    break;
	}

	/* if list is empty, move on */
	if (addr == 0) continue;

	/* walk through list until EOC is found or we detect a loop */
	for (j = 0; j < 5000; j++) {

	    /* add this entry to the map */
	    (void)an_ioserv_set_mem_map_bits(addr, 1 << i);

	    /* try to grab the entry */
	    if (an_grab_virt_chunk(0, addr, (char *)&next_addr, 4)) {
		fprintf(stderr, "read failed on entry 0.0x%08x!!\n", addr);
		break;
	    }

	    /* check for eoc -- stop if found, otherwise try again */
	    if (next_addr == 0)
		break;
	    else
		addr = next_addr;
	}
    }

    /* run through non-equivalently mapped free lists */
    for (i = 1; i < 17; i++) {

	/* grab appropriate list head */
        if (an_grab_extern("io_mem_heads", &next_addr)) {
	    fprintf(stderr, "couldn't get io_mem_heads!!\n");
	    break;
	}
        if (an_grab_virt_chunk(0, next_addr+(4*i), (char *)&addr, 4))  {
	    fprintf(stderr, "couldn't get io_mem_heads!!\n");
	    break;
	}

	/* if list is empty, move on */
	if (addr == 0)
	    continue;

	/* walk through list until EOC is found or we detect a loop */
	for (j = 0; j < 5000; j++) {

	    /* add this entry to the map */
	    (void)an_ioserv_set_mem_map_bits(addr, 1 << i);

	    /* try to grab the entry */
	    if (an_grab_virt_chunk(0, addr, (char *)&next_addr, 4)) {
		fprintf(stderr, "read failed on entry 0.0x%08x!!\n", addr);
		break;
	    }

	    /* check for eoc -- stop if found, otherwise try again */
	    if (next_addr == 0)
		break;
	    else
		addr = next_addr;
	}
    }
}
an_ioserv_mem_free_lists(outf)
    FILE *outf;
{
    int		i, j;
    unsigned	addr, next_addr, first;

    /* print first address */
    fprintf(outf,"\nFirst address io_get_mem used:\t\t");
    if (an_grab_extern("io_mem_buf",&next_addr)) {
	fprintf(outf,"couldn't get io_mem_buf!!\n");
	return;
    }
    first = (next_addr+2047) & ~2047;
    fprintf(outf,"0x%08x\n",first);

    /* print last address */
    fprintf(outf,"Last address io_get_mem used:\t\t");
    if (an_grab_extern("p_io_mem_buf",&next_addr)) {
	fprintf(outf,"couldn't get p_io_mem_buf!!\n");
	return;
    }
    if (an_grab_virt_chunk(0,next_addr,(char *)&addr,4)) {
	fprintf(outf,"couldn't get p_io_mem_buf!!\n");
	return;
    }
    addr--;
    fprintf(outf,"0x%08x\n",addr);

    /* print number of pages used */
    fprintf(outf,"Number of pages io_get_mem has used:\t%d\n",
	    (addr+1-first)/2048);

    /* print number of pages left */
    fprintf(outf,"Number of pages io_get_mem has left:\t");
    if (an_grab_extern("num_io_mem_pages",&next_addr)) {
	fprintf(outf,"couldn't get num_io_mem_pages!!\n");
	return;
    }
    if (an_grab_virt_chunk(0,next_addr,(char *)&addr,4)) {
	fprintf(outf,"couldn't get num_io_mem_pages!!\n");
	return;
    }
    fprintf(outf,"%d\n",addr);

    /* print equiv-mapped line */
    fprintf(outf,"\nEquivalently mapped free lists:\n");

    /* run through equivalently mapped free lists */
    for (i=1 ; i<12 ; i++) {

	/* print header line */
	fprintf(outf,"  entry %2d for %5d-byte sized buffers: ",i,1<<i);

	/* grab appropriate list head */
        if (an_grab_extern("io_eqmem_heads",&next_addr)) {
	    fprintf(outf,"couldn't get io_eqmem_heads!!\n");
	    break;
	}
        if (an_grab_virt_chunk(0,next_addr+(4*i),(char *)&addr,4)) {
	    fprintf(outf,"couldn't get io_eqmem_heads!!\n");
	    break;
	}

	/* if list is empty, move on */
	if (addr == 0) {
	    fprintf(outf,"empty\n");
	    continue;
	}
	else
	    fprintf(outf,"\n");

	/* walk through list until EOC is found or we detect a loop */
	for (j=0 ; j<5000 ; j++) {

	    /* print this address */
	    fprintf(outf,"     buffer  0x%08x\n",addr);

	    /* try to grab the entry */
	    if (an_grab_virt_chunk(0,addr,(char *)&next_addr,4)) {
		fprintf(outf,"read failed on entry 0.0x%08x!!\n",addr);
		break;
	    }

	    /* check for eoc -- stop if found, otherwise try again */
	    if (next_addr == 0)
		break;
	    else
		addr = next_addr;

	}
    }

    /* print equiv-mapped line */
    fprintf(outf,"\nNon-equivalently mapped free lists:\n");

    /* run through non-equivalently mapped free lists */
    for (i=1 ; i<17 ; i++) {

	/* print header line */
	fprintf(outf,"  entry %2d for %5d-byte sized buffers: ",i,1<<i);

	/* grab appropriate list head */
        if (an_grab_extern("io_mem_heads",&next_addr)) {
	    fprintf(outf,"couldn't get io_mem_heads!!\n");
	    break;
	}
        if (an_grab_virt_chunk(0,next_addr+(4*i),(char *)&addr,4)) {
	    fprintf(outf,"couldn't get io_mem_heads!!\n");
	    break;
	}

	/* if list is empty, move on */
	if (addr == 0) {
	    fprintf(outf,"empty\n");
	    continue;
	}
	else
	    fprintf(outf,"\n");

	/* walk through list until EOC is found or we detect a loop */
	for (j=0 ; j<5000 ; j++) {

	    /* print this address */
	    fprintf(outf,"     buffer  0x%08x\n",addr);

	    /* try to grab the entry */
	    if (an_grab_virt_chunk(0,addr,(char *)&next_addr,4)) {
		fprintf(outf,"read failed on entry 0.0x%08x!!\n",addr);
		break;
	    }

	    /* check for eoc -- stop if found, otherwise try again */
	    if (next_addr == 0)
		break;
	    else
		addr = next_addr;
	}
    }
}

an_ioserv_set_mem_map_bits (addr, num_bytes)
    unsigned	addr;		/* address of buffer to set bits for */
    int		num_bytes;	/* size of buffer to set bits for    */
{
    unsigned	bit_mask;
    int		word, first_bit, i;

    /* verify parameters */
    if ( (num_bytes < 1) || (num_bytes > 65536) ) {
	fprintf(stderr,"num_bytes out of range: 0x%x\n",num_bytes);
	return(1);
    }
    if ( (addr < io_mem_buf) || ((addr + num_bytes) > io_mem_top) ) {
	fprintf(stderr,"addr out of range: 0x%08x\n",addr);
	return(1);
    }

    /* calculate starting position */
    word = (addr - io_mem_buf) / 32;
    first_bit = (addr - io_mem_buf) % 32;
    bit_mask = 0x80000000 >> first_bit;

    /* set bits */
    for (i=0 ; i<num_bytes ; i++) {

	/* check for bit already set and then set bit in appropriate map */
	if ((io_mem_map[word] & bit_mask) != 0)
	    io_mem_map_dup[word] |= bit_mask;
	else
	    io_mem_map[word] |= bit_mask;

	/* move on */
	if (bit_mask == 1) {
	    word++;
	    bit_mask = 0x80000000;
	}
	else
	    bit_mask >>= 1;
    }
    return(0);

}

/*ARGSUSED*/
an_ioserv(call_type, port_num, outf, option_string)
    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*outf;		/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */
{
    int			an_ioserv_decode_message();

    /* perform the correct action depending on what call_type is */
    switch (call_type) {
	case AN_MGR_INIT: /* put my message decoding routine into system list */
	    (void)aio_init_message_decode(ABORT_EVENT_MSG, RESERVED_HP_19,
	                            an_ioserv_decode_message);
	    break;

	case AN_MGR_BASIC:
	    an_ioserv_basic(outf);
	    break;

	case AN_MGR_DETAIL:
	    an_ioserv_all(outf);
	    break;

	case AN_MGR_OPTIONAL:
	    an_ioserv_optional(option_string, outf);
	    break;

	case AN_MGR_HELP:
	    an_ioserv_help(outf);
	    break;
    }
}

#define	START		0x00000
#define DO_HEADER	0x00001
#define DO_PORT		0x00002
#define DO_FRAME	0x00004
#define DO_EM_FE	0x00008
#define DO_DETAIL	0x00010
#define DO_SANITY	0x00020
#define DO_OLDTREE	0x00040
#define DO_MEM1		0x00080
#define DO_MEM2		0x00100
#define DO_CLASS	0x00200
#define DO_EIRR		0x00400
#define DO_TREE		0x00800
#define DO_KERNDEV	0x01000
#define DO_NATIVE	0x02000
#define DO_MGR		0x04000
#define DO_MOD		0x08000
#define DO_BASIC	0x10000
#define DO_TIMER	0x20000
#define DO_DS		DO_CLASS|DO_TREE|DO_MOD|DO_MGR|\
			DO_KERNDEV|DO_NATIVE|DO_HEADER
#define DO_ALL		0xfffff

an_ioserv_optional(option_string, outf)
    char	*option_string;
    FILE	*outf;
{
    int do_flag		= 0;
    int	state		= START;
    int	imcport_num	= NONE;
    int	frame_num	= NONE;
    int	iotree_num	= NONE;
    int	io_tree_num	= NONE;
    int class_num	= NONE;
    int	mgr_num		= NONE;
    int	mod_num		= NONE;
    int	eirr_num	= NONE;
    int	mem_num1	= NONE;
    int	mem_num2	= NONE;
    int	timer_num	= NONE;
    int	temp;

    /* read through option string and set appropriate flags */
    char *tok = strtok(option_string," ");

    do {
	if      (eq(tok, "all"))       state = START,     do_flag  = DO_ALL;
	else if (eq(tok, "sum"))       state = START,     do_flag |= DO_HEADER;
	else if (eq(tok, "imcport"))   state = DO_PORT,   do_flag |= DO_PORT;
	else if (eq(tok, "frame"))     state = DO_FRAME,  do_flag |= DO_FRAME;
	else if (eq(tok, "em_fe"))     state = START,     do_flag |= DO_EM_FE;
	else if (eq(tok, "detail"))    state = START,     do_flag |= DO_DETAIL;
	else if (eq(tok, "sanity"))    state = START,     do_flag |= DO_SANITY;
	else if (eq(tok, "oldtree"))   state = DO_OLDTREE,do_flag |= DO_OLDTREE;
	else if (eq(tok, "mem"))       state = DO_MEM1,   do_flag |= DO_MEM1;
	else if (eq(tok, "class"))     state = DO_CLASS,  do_flag |= DO_CLASS;
	else if (eq(tok, "eirr"))      state = DO_EIRR,	  do_flag |= DO_EIRR;
	else if (eq(tok, "tree"))      state = DO_TREE,	  do_flag |= DO_TREE;
	else if (eq(tok, "kerndev"))   state = START,	  do_flag |= DO_KERNDEV;
	else if (eq(tok, "native"))    state = START,	  do_flag |= DO_NATIVE;
	else if (eq(tok, "mgr"))       state = DO_MGR,	  do_flag |= DO_MGR;
	else if (eq(tok, "mod"))       state = DO_MOD,	  do_flag |= DO_MOD;
	else if (eq(tok, "ds"))        state = START,     do_flag |= DO_DS;
	else if (eq(tok, "basic"))     state = START,     do_flag |= DO_BASIC;
	else if (eq(tok, "timer"))     state = DO_TIMER,  do_flag |= DO_TIMER;
	else {
	    /* no keyword match -- either a numeric parameter or a mistake */
	    if (state != START) {
		/* convert the token into a number if possible */
		if (stoi(tok, &temp, outf) == 0) {
		    if (temp < 0) {
			fprintf(stderr, "invalid parameter <%s>\n", tok);
			an_ioserv_help(outf);
			return;
		    }

		    /* stuff the correct kind of number */
		    if      (state == DO_PORT)	imcport_num	= temp;
		    else if (state == DO_FRAME)	frame_num	= temp;
		    else if (state == DO_OLDTREE)iotree_num	= temp;
		    else if (state == DO_TREE)	io_tree_num	= temp;
		    else if (state == DO_CLASS) class_num	= temp;
		    else if (state == DO_MGR)   mgr_num		= temp;
		    else if (state == DO_MOD)   mod_num		= temp;
		    else if (state == DO_TIMER) timer_num	= temp;
		    else if (state == DO_EIRR)	eirr_num	= temp;
		    else if (state == DO_MEM1)	mem_num1	= temp;
		    else if (state == DO_MEM2)	mem_num2	= temp;
		    else {		/* blow off -- invalid option */
			fprintf(stderr, "invalid option <%s>\n", tok);
			an_ioserv_help(outf);
			return;
		    }

		    state = (state == DO_MEM1) ? DO_MEM2 : START;
		}
	    }
	    else {
		fprintf(stderr, "invalid option <%s>\n", tok);
		an_ioserv_help(outf);
		return;
	    }
	}
    } while ((tok = strtok(NULL, " ")) != NULL);

    /* now process the flags that were set */
    if (do_flag == DO_ALL)    { an_ioserv_all(outf); return; }
    if (do_flag == DO_BASIC)  { an_ioserv_basic(outf); return; }
    if (do_flag & DO_HEADER)	dumpio(outf);
    if (do_flag & DO_PORT)	an_ioserv_imcport(imcport_num, outf);
    if (do_flag & DO_OLDTREE)	an_ioserv_iotree(iotree_num, outf);
    if (do_flag & DO_CLASS)	an_ioserv_class_table(class_num, outf);
    if (do_flag & DO_MGR)	an_ioserv_mgr_table(mgr_num, outf);
    if (do_flag & DO_MOD)	an_ioserv_mod_table(mod_num, outf);
    if (do_flag & DO_KERNDEV)	an_ioserv_kern_dev_table(NONE, outf);
    if (do_flag & DO_NATIVE)	an_ioserv_native(native_mod_ptr, 0, 1, outf);
    if (do_flag & DO_EM_FE)	an_ioserv_em_fe(outf);
    if (do_flag & DO_EIRR)	an_ioserv_eirr(eirr_num, outf);
    if (do_flag & DO_DETAIL)  { cleariotables(); iocheck(1, outf); }
    if (do_flag & DO_SANITY)  { cleariotables(); iocheck(0, outf); }
    if (do_flag & DO_FRAME)	an_ioserv_frame(frame_num, outf);
    if (do_flag & DO_TREE)	an_ioserv_io_tree(io_tree_num, 1, outf);
    if (do_flag & DO_TIMER)	an_ioserv_timer(timer_num, (timer_num != NONE),
					outf);
    if ((do_flag & DO_MEM1) && mem_num1 == NONE) {
	an_ioserv_mem(outf);
	an_ioserv_mem_free_lists(outf);
    }

    if ((do_flag & DO_MEM1) && mem_num1 != NONE) {
	if (mem_num2 == NONE) mem_num2 = 32;
	if (aio_mem_on_free_list((unsigned)mem_num1, mem_num2, "  ", outf) == 0)
	    fprintf(outf, "  didn't find buffer 0x%08x on any free list\n",
		    mem_num1);
    }
}

an_ioserv_basic(outf)
    FILE *outf;
{
    dumpio(outf);
    an_ioserv_io_tree(NONE, 0, outf);
    an_ioserv_native(native_mod_ptr, 0, 0, outf);
    cleariotables();
    iocheck(0, outf);
    an_ioserv_em_fe(outf);
}

an_ioserv_all(outf)
    FILE *outf;
{
    dumpio(outf);
    an_ioserv_class_table(NONE, outf);
    an_ioserv_mod_table(NONE, outf);
    an_ioserv_mgr_table(NONE, outf);
    an_ioserv_io_tree(NONE, 1, outf);
    an_ioserv_kern_dev_table(NONE, outf);
    an_ioserv_native(native_mod_ptr, 0, 1, outf);
    an_ioserv_imcport(NONE, outf);
    an_ioserv_frame(NONE, outf);
    cleariotables();
    iocheck(1, outf);
    an_ioserv_mem(outf);
    an_ioserv_em_fe(outf);
    an_ioserv_eirr(NONE, outf);
}

dumpio(o)
    FILE *o;
{
    fprintf(o, "\n==========\nI/O stats:\n==========\n");
    fprintf(o, " Size of message frame:                     %3d (0x%x) bytes\n",
            sizeof(msg_frame), sizeof(msg_frame));
    fprintf(o, " Size of message:                           %3d (0x%x) bytes\n",
	    Msg_control.msg_length, Msg_control.msg_length);
    fprintf(o, " Number of messages in system:              %3d (0x%x)\n",
	    Msg_control.num_msgs, Msg_control.num_msgs);
    fprintf(o, " Number of class records:                   %3d (0x%x)\n",
            ioheader.num_io_class_recs, ioheader.num_io_class_recs);
    fprintf(o, " Number of manager records:                 %3d (0x%x)\n",
            ioheader.num_io_mgr_recs, ioheader.num_io_mgr_recs);
    fprintf(o, " Number of software module records:         %3d (0x%x)\n",
            ioheader.num_io_sw_mod_recs, ioheader.num_io_sw_mod_recs);
    fprintf(o, " Number of hardware module records:         %3d (0x%x)\n",
            ioheader.num_io_hw_mod_recs, ioheader.num_io_hw_mod_recs);
    fprintf(o, " Number of io_tree records:                 %3d (0x%x)\n",
            ioheader.num_io_tree_recs, ioheader.num_io_tree_recs);
    fprintf(o, " Number of kernel device records:           %3d (0x%x)\n",
            ioheader.num_kern_dev_recs, ioheader.num_kern_dev_recs);
    fprintf(o, " Number of ports:                           %3d (0x%x)\n",
            max_port, max_port);
    fprintf(o, " Number of pseudo ports:                    %3d (0x%x)\n",
            Msg_control.max_pseudo, Msg_control.max_pseudo);
    fprintf(o, " Number of timers:                          %3d (0x%x)\n\n",
            ncallout, ncallout);
}

an_ioserv_class_table(index, outf)
    int		index;
    FILE	*outf;
{
    aio_class_entry *class;
    aio_tree_entry  *node;

    if (index == NONE) {
	fprintf(outf, "\n============\nCLASS TABLE:\n============\n");
	for (index = 0; index < ioheader.num_io_class_recs; index++)
	    an_ioserv_class_table(index, outf);
	return;
    }
    class = &io_class_ptr[index];

    fprintf(outf, "io_class_table[%d]: %s\n", index, class->name);

    fprintf(outf, "    class_link  = 0x%08x ", class->kclass_link);
    fprintf(outf, "options      = <");
    if (class->options.nonexistent)	fprintf(outf, "nonexistent");
    fprintf(outf, ">\n");
    if (class->class_link != 0) {
	fprintf(outf, "    nodes on class_link list:");
	for (node = class->class_link; node != 0; node = node->class_link)
	    fprintf(outf, " [%d]", node - io_tree_ptr);
	fprintf(outf, "\n");
    }
}

an_ioserv_imcport(port_num, outf)
    int		port_num;
    FILE	*outf;
{
    if (port_num == NONE) {
	fprintf(outf, "\n===============\nIMC PORT INDEX:\n===============\n");
	for (port_num = 1; port_num <= max_port; port_num++) {
	    if (vport_index[port_num] != 0)
		display_imcport(port_num, 'n', 0, " ", outf);
	}
	return;
    }
    if (port_to_imc_ptr(port_num) == 0)
	fprintf(stderr, "invalid port number <%d>\n", port_num);
    else
	display_imcport(port_num, 'n', 0, " ", outf);
}

an_ioserv_mgr_table(index, outf)
    int		index;
    FILE	*outf;
{
    aio_mgr_entry  *mgr;
    aio_tree_entry *node;

    if (index == NONE) {
	fprintf(outf, "\n==============\nI/O MGR TABLE:\n==============\n");
	for (index = 0; index < ioheader.num_io_mgr_recs; index++)
	    an_ioserv_mgr_table(index, outf);
	return;
    }
    mgr = &io_mgr_ptr[index];

    fprintf(outf, "io_mgr_table[%d]: %s\n", index, mgr->name);

    fprintf(outf, "    mi_link     = 0x%08x", mgr->kmi_link);
    if (mgr->entry) {
	fprintf(outf, "    port server = 0x%08x (", mgr->entry);
	an_display_sym((unsigned)mgr->entry, outf, AIO_SYMBOL_AND_OFFSET);
	fprintf(outf, ")\n");
    }
    else
	fprintf(outf, "    no port server\n");

    fprintf(outf, "    b_major     = %-10d", mgr->b_major);
    if (mgr->attach) {
	fprintf(outf, "    attach      = 0x%08x (", mgr->attach);
	an_display_sym((unsigned)mgr->attach, outf, AIO_SYMBOL_AND_OFFSET);
	fprintf(outf, ")\n");
    }
    else
	fprintf(outf, "    no attach routine\n");

    fprintf(outf, "    c_major     = %-10d", mgr->c_major);
    if (mgr->init) {
	fprintf(outf, "    init        = 0x%08x (", mgr->init);
	an_display_sym((unsigned)mgr->init, outf, AIO_SYMBOL_ONLY);
	fprintf(outf, ")\n");
    }
    else
	fprintf(outf, "    no init routine\n");

    fprintf(outf, "    options      = < ");
    if (mgr->options.pseudo)		fprintf(outf, "pseudo ");
    if (mgr->options.ldm)		fprintf(outf, "ldm ");
    if (mgr->options.dynamic_ds)	fprintf(outf, "dynamic_ds ");
    if (mgr->options.probe)		fprintf(outf, "probe ");
    if (mgr->options.reconfig)		fprintf(outf, "reconfig ");
    if (mgr->options.nonexistent)	fprintf(outf, "nonexistent ");
    if (mgr->options.bound)		fprintf(outf, "bound ");
    if (mgr->options.shared_port)	fprintf(outf, "shared_port ");
    fprintf(outf, ">\n");
    if (mgr->mi_link != 0) {
	fprintf(outf, "    nodes on mi_link list:");
	for (node = mgr->mi_link; node != 0; node = node->mi_link)
	    fprintf(outf, " [%d]", node - io_tree_ptr);
	fprintf(outf, "\n");
    }
}

an_ioserv_mod_table(index, outf)
    int		index;
    FILE	*outf;
{
    aio_mod_entry  *mod;

    if (index == NONE) {
	fprintf(outf, "\n==============\nI/O MOD TABLE:\n==============\n");
	for (index = 0; index < ioheader.num_io_sw_mod_recs; index++)
	    an_ioserv_mod_table(index, outf);
	return;
    }
    mod = &io_mod_ptr[index];

    fprintf(outf, "io_mod_table[%d]: %s\n", index, mod->name);

    fprintf(outf, "    class = [%2d] %-16s", mod->class - io_class_ptr,
	mod->class->name);
    if (mod->idy) {
	fprintf(outf, "  idy = 0x%08x (", mod->idy);
	an_display_sym((unsigned)mod->idy, outf, AIO_SYMBOL_AND_OFFSET);
	fprintf(outf, ")\n");
    }
    else
	fprintf(outf, "  (no idy function)\n");

    fprintf(outf, "    options      = < ");
    if (mod->options.overlay)		fprintf(outf, "overlay ");
    if (mod->options.add_child)		fprintf(outf, "add_child ");
    if (mod->options.nonexistent)	fprintf(outf, "nonexistent ");
    fprintf(outf, ">\n");
}

an_ioserv_kern_dev_table(index, outf)
    int		index;
    FILE	*outf;
{
    aio_kern_dev_entry	*kdp;
    struct mgr_info_type mgr;
    int			 s;

    if (index == NONE) {
	fprintf(outf, "\n===============\nKERN_DEV TABLE:\n===============\n");
	for (index = 0; index < ioheader.num_kern_dev_recs; index++)
	    an_ioserv_kern_dev_table(index, outf);
	return;
    }
    kdp = &kern_dev_table[index];

    fprintf(outf, "kern_dev_table[%d]: ", index);
    switch (index) {
	case KDT_CONSOLE:	fprintf(outf, "KDT_CONSOLE\n");		break;
	case KDT_ROOT:		fprintf(outf, "KDT_ROOT\n");		break;
	case KDT_ROOT_MIRROR:	fprintf(outf, "KDT_ROOT_MIRROR\n");	break;
	case KDT_DUMP:		fprintf(outf, "KDT_DUMP\n");		break;
	case KDT_SWAP:		fprintf(outf, "KDT_SWAP\n");		break;
	case KDT_SWAP_MIRROR:	fprintf(outf, "KDT_SWAP_MIRROR\n");	break;
	case KDT_ADDED_SWAP:	fprintf(outf, "KDT_ADDED_SWAP\n");	break;
	default:		fprintf(outf, "KDT_ADDED_SWAP+%d\n",
					index-KDT_ADDED_SWAP);		break;
    }

    fprintf(outf, "    node        = 0x%08x ", kdp->node);
    if (kdp->node != 0) {
	if ((s = aio_get_mgr_info(AIO_IO_NODE_TYPE, kdp->node, &mgr)) == AIO_OK)
	    fprintf(outf, "(%s port %d at %s)", mgr.mgr_name, mgr.port_num,
		mgr.hw_address);
	else if (s == AIO_INCOMPLETE)
	    fprintf(outf, "(%s)", mgr.mgr_name);
	else 
	    fprintf(outf, "(unknown node?)");
	fprintf(outf, " io_tree[%d]", kdp->node - io_tree_ptr);
    }
    fprintf(outf, "\n");

    fprintf(outf, "    gen_node    = 0x%08x ", kdp->gen_node);
    if (kdp->gen_node != 0) {
	if ((s = aio_get_mgr_info(AIO_IO_NODE_TYPE, kdp->gen_node, &mgr)) == AIO_OK)
	    fprintf(outf, "(%s port %d at %s)", mgr.mgr_name, mgr.port_num,
		mgr.hw_address);
	else if (s == AIO_INCOMPLETE)
	    fprintf(outf, "(%s)", mgr.mgr_name);
	else 
	    fprintf(outf, "(unknown node?)");
	fprintf(outf, " io_tree[%d]", kdp->node - io_tree_ptr);
    }
    fprintf(outf, "\n");

    fprintf(outf, "    dev_num     = 0x%08x", kdp->dev_num);
    fprintf(outf, " gen_dev_num = 0x%08x\n", kdp->gen_dev_num);

    fprintf(outf, "    flags       = < ");
    if (kdp->flags.gen_nonexistent)	fprintf(outf, "gen_nonexistent ");
    if (kdp->flags.gen_defaulted)	fprintf(outf, "gen_defaulted ");
    if (kdp->flags.gen_pseudo)		fprintf(outf, "gen_pseudo ");
    if (kdp->flags.boot_defaulted)	fprintf(outf, "boot_defaulted ");
    if (kdp->flags.hdw_path_overridden)	fprintf(outf, "hdw_path_overridden ");
    if (kdp->flags.mod_path_overridden)	fprintf(outf, "mod_path_overridden ");
    fprintf(outf, ">\n");
}

an_ioserv_em_fe(outf)
    FILE	*outf;
{
    unsigned	addr;
    int		in_em_fe, found = 0;
    extern int	niopdir;

    fprintf(outf, "\nChecking i/o pages for emulated FE mode:\n\n");
    for (addr = 0xfffff800-((niopdir-1)*0x800); addr != 0 ; addr += 0x800)
	if (aio_em_fe(addr, &in_em_fe) == AIO_OK && in_em_fe) {
	    fprintf(outf, "  page starting at 0x%08x is in emulated FE mode\n",
		    addr);
	    found = 1;
	}
    if (found == 0)
	fprintf(outf, "  no pages found in emulated FE mode\n");
}

an_ioserv_eirr(num, outf)
    int		num;
    FILE	*outf;
{
    unsigned		addr;
    struct compl_head	head;

    if (num == NONE) {
	fprintf(outf, "\n==================\nEIRR SWITCH TABLE:\n==================\n");
	for (num = 0; num < 32; num++) an_ioserv_eirr(num, outf);
	return;
    }
    if (num >= 32) {
	fprintf(stderr, "invalid eirr number <%d>\n", num);
	return;
    }
    fprintf(outf, " eirr bit %2d:", num);
    fprintf(outf, " eiem value = 0x%08x", eirr_switch[num].eiem);
    fprintf(outf, " rtn to call = ");
    an_display_sym((unsigned)eirr_switch[num].int_action, outf,
	AIO_SYMBOL_AND_OFFSET);
    fprintf(outf, "\n");

    if ((unsigned)eirr_switch[num].int_action == int_compl) {
	/* grab the completion list header */
	addr = (unsigned)int_dispatch[num].head;
	if (an_grab_virt_chunk(0, addr, &head, sizeof(head))) {
	    fprintf(outf, "problem with completion header!!\n");
	    return;
	}

	/* process completion entries until eoc */
	addr = (unsigned)head.link;
	fprintf(outf, "     completion entries:\n");
	while ((addr & 1) == 0)
	    addr = an_ioserv_print_compl_entry(addr, "       ", outf);
    }
    else if ((unsigned)eirr_switch[num].int_action == int_direct ||
	     (unsigned)eirr_switch[num].int_action == int_always) {
	struct poll_entry   *pl, entry;
	    
	for (pl = int_dispatch[num].poll; pl; pl = entry.link) {
	    char name[16];

	    if (an_grab_virt_chunk(0, (unsigned)pl, &entry, sizeof(entry))){
		fprintf(outf, "can't grab poll entry!!\n");
		break;
	    }
	    else if (aio_get_mgr_name(entry.port, name) != AIO_OK) {
		fprintf(outf, "no manager name for poll entry port %d\n",
		    entry.port);
		break;
	    }
	    fprintf(outf,"       manager    = %s (port %d)   hpa 0x%08x  iireg 0x%08x\n",
		        name, entry.port, entry.hpa, entry.ii_reg);
	}
    }
}

an_ioserv_print_compl_entry(entry_addr, str, o)
    unsigned	entry_addr;
    char	*str;
    FILE	*o;
{
    struct compl_entry	entry;
    char		name[16];

    /* grab the completion list entry */
    if (an_grab_virt_chunk(0, entry_addr, &entry, sizeof(entry))) {
	fprintf(o, "can't grab completion entry!!\n");
	return(1);
    }
    if (aio_get_mgr_name(entry.port, name) != AIO_OK)  name[0] = '\0';

    /* print the contents */
    fprintf(o, "%slink to next element = 0x%08x\n", str, (unsigned)entry.link);
    fprintf(o, "%sstatus               = 0x%08x\n", str, entry.status);
    fprintf(o, "%ssave_link            = 0x%08x\n", str, entry.save_link);
    fprintf(o, "%ssave_count           = 0x%08x\n", str, entry.save_count);
    fprintf(o, "%sport                 = %d (%s)\n",str, entry.port, name);
    fprintf(o, "%sfiller1              = 0x%08x\n", str, entry.filler[0]);
    fprintf(o, "%sfiller2              = 0x%08x\n", str, entry.filler[1]);
    fprintf(o, "%sfiller3              = 0x%08x\n\n", str, entry.filler[2]);

    return((unsigned)entry.link);	/* return the link field */
}

an_ioserv_frame(msg_num, outf)
    int		msg_num;
    FILE	*outf;
{
    if (msg_num == NONE) {
	fprintf(outf, "\n===========\nI/O FRAMES:\n===========\n");
	for (msg_num = 0; msg_num < Msg_control.num_msgs; msg_num++)
	    display_frame(msg_num, 'n', 0, " ", outf);
    }
    else if (msg_num >= Msg_control.num_msgs)
	fprintf(stderr, "invalid msg frame number <%d>\n", msg_num);
    else
	display_frame(msg_num, 'n', 0, " ", outf);
}

an_ioserv_io_tree(index, verbose, outf)
    int	 index, verbose;
    FILE *outf;
{
    aio_tree_entry *node;

    if (index == NONE) {
	fprintf(outf, "\n=========\nI/O TREE:\n=========\n");
	if (verbose) {
	    for (index = 0; index < ioheader.num_io_tree_recs; index++)
		an_ioserv_io_tree(index, verbose, outf);
	}
	else {
	    dump_io_tree(io_tree_ptr->child, 0, outf);
	}
	return;
    }
    if (index < 0 || index > ioheader.num_io_tree_recs) {
	fprintf(stderr, "invalid io_tree index <%d>\n", index);
	return;
    }
    node = &io_tree_ptr[index];

    fprintf(outf, "io_tree_ptr[%d]:\n", index);
    fprintf(outf, "   port         = %2d", node->port);
    fprintf(outf, "   parent       = 0x%08x [%2d] (%s/%s)\n", node->kparent,
	node->parent - io_tree_ptr,
	(node->parent->manager) ? node->parent->manager->name : no_mgr,
	(node->parent->module ) ? node->parent->module->name  : no_mod);
    fprintf(outf, "   mgr_index    = %2d",   node->mgr_index);
    fprintf(outf, "   sibling      = 0x%08x [%2d] (%s/%s)\n", node->ksibling,
	node->sibling - io_tree_ptr,
	(node->sibling->manager) ? node->sibling->manager->name : no_mgr,
	(node->sibling->module)  ? node->sibling->module->name  : no_mod);
    fprintf(outf, "   lu           = %2d", node->lu);
    fprintf(outf, "   child        = 0x%08x [%2d] (%s/%s)\n", node->kchild,
	(node->child) ? (node->child - io_tree_ptr) : -1,
	(node->child && node->child->manager)?node->child->manager->name:no_mgr,
	(node->child && node->child->module) ?node->child->module->name:no_mod);
    fprintf(outf, "   hdw_address  = %2d", node->hdw_address);
    fprintf(outf, "   ent_id       = 0x%x (%d)\n", node->ent_id,
	node->ent_id);
    fprintf(outf, "   mi_link      = 0x%08x [%2d]\n", node->kmi_link,
	(node->mi_link) ? node->mi_link - io_tree_ptr : -1);
    fprintf(outf, "   class_link   = 0x%08x [%2d]\n", node->kclass_link,
	(node->class_link) ? node->class_link - io_tree_ptr : -1);
    fprintf(outf, "   conn.mgr     = 0x%08x [%2d] (%s)\n", node->kmanager,
	(node->manager) ? node->manager - io_mgr_ptr : -1,
	(node->manager) ? node->manager->name	: no_name);
    fprintf(outf, "   conn.mod     = 0x%08x [%2d] (%s)\n", node->kmodule,
	(node->module) ? node->module - io_mod_ptr : -1,
	(node->module) ? node->module->name	: no_name);
    fprintf(outf, "   state        = < ");
    switch (node->state.bound_state) {
	case BS_NEVER_ACCESSED:	fprintf(outf, "BS_NEVER_ACCESSED ");	break;
	case BS_BOUND:		fprintf(outf, "BS_BOUND ");		break;
	case BS_UNBOUND:	fprintf(outf, "BS_UNBOUND ");		break;
	case BS_TOO_MANY:	fprintf(outf, "BS_TOO_MANY ");		break;
	case BS_NO_RESOURCES:	fprintf(outf, "BS_NO_RESOURCES ");	break;
	case BS_MGR_IN_WAY:	fprintf(outf, "BS_MGR_IN_WAY ");	break;
	case BS_NO_MODULE:	fprintf(outf, "BS_NO_MODULE ");		break;
	case BS_BIND_FAILED:	fprintf(outf, "BS_BIND_FAILED ");	break;
	default:		fprintf(outf, "BS_??? ");		break;
    }
    switch (node->state.ent_id_state) {
	case ES_NEVER_PROBED:	fprintf(outf, "ES_NEVER_PROBED ");	break;
	case ES_VALID:		fprintf(outf, "ES_VALID ");		break;
	case ES_NO_HARDWARE:	fprintf(outf, "ES_NO_HARDWARE ");	break;
	case ES_UNRECOGNIZED:	fprintf(outf, "ES_UNRECOGNIZED ");	break;
	case ES_PARENT_WONT_PROBE:
			fprintf(outf, "ES_PARENT_WONT_PROBE ");		break;
	case ES_PROBE_FAILED:	fprintf(outf, "ES_PROBE_FAILED ");	break;
	default:		fprintf(outf, "ES_??? ");		break;
    }
    if (node->state.from_gen)		fprintf(outf, "from_gen ");
    if (node->state.from_probe)		fprintf(outf, "from_probe ");
    if (node->state.from_file)		fprintf(outf, "from_file ");
    if (node->state.forced)		fprintf(outf, "forced ");
    if (node->state.cannot_unbind)	fprintf(outf, "cannot_unbind ");
    if (node->state.native_hdw)		fprintf(outf, "native_hdw ");
    fprintf(outf, ">\n");
}

an_ioserv_iotree(index, outf)
    int	 index;
    FILE *outf;
{
    aio_tree_entry *node;

    if (index == NONE) {
	fprintf(outf, "\n=============\nOLD I/O TREE:\n=============\n");
	for (index = 0; index < ioheader.num_iotree_recs; index++)
	    an_ioserv_iotree(index, outf);
	return;
    }
    else if (index < 0 || index >= ioheader.num_iotree_recs) {
	fprintf(stderr, "invalid iotree index <%d>\n", index);
	return;
    }
    node = index_to_node(index);

    fprintf(outf, "OLD iotree entry %d (%s):\n", index,
	(node->manager) ? node->manager->name : no_name);

    fprintf(outf, "   port             = %2d",   iotree[index].port);
    fprintf(outf, "   parent           = %2d (%s)\n", iotree[index].parent,
	(node->parent->manager) ? node->parent->manager->name : no_name);
    fprintf(outf, "   parm             = %2d",   iotree[index].parm);
    fprintf(outf, "   sibling          = %2d (%s)\n", iotree[index].sibling,
	(node->sibling->manager) ? node->sibling->manager->name : no_name);
    fprintf(outf, "   lu               = %2d", iotree[index].lu);
    fprintf(outf, "   child            = %2d (%s)\n", iotree[index].child,
	(node->child->manager) ? node->child->manager->name : no_name);

    fprintf(outf, "   mgr_table index  = %2d", iotree[index].mgr);
    fprintf(outf, "   state            = %2d ", iotree[index].state);
    switch (iotree[index].state) {
	case IOTREE_SPARE:	fprintf(outf, "(IOTREE_SPARE)\n");	break;
	case IOTREE_ASSIGNED:	fprintf(outf, "(IOTREE_ASSIGNED)\n");	break;
	case IOTREE_BOUND:	fprintf(outf, "(IOTREE_BOUND)\n");	break;
	default:		fprintf(outf, "(unknown state)\n");	break;
    }
    fprintf(outf, "   module index     = %2d (%s)\n", iotree[index].module,
	(node->module) ? node->module->name : no_name);
}

an_ioserv_timer(index, verbose, outf)
    int  index, verbose;
    FILE *outf;
{
    struct timer_map *tp;
    int s;

    if (index == NONE) {
	fprintf(outf, "\n================\nI/O TIMER TABLE:\n================\n");
	for (index = 0; index < ncallout; index++)
	    an_ioserv_timer(index, verbose, outf);
	return;
    }
    else if (index < 0 || index >= ncallout) {
	fprintf(stderr, "invalid timer index <%d>\n", index);
	return;
    }
    tp = &timer_list[index];

    if (verbose || tp->t_next == 0) {
	struct mgr_info_type mgr;

	fprintf(outf, "timer_list[%2d]: ", tp - timer_list);
	if ((s = aio_get_mgr_info(AIO_PORT_NUM_TYPE, tp->port_id,
		&mgr)) == AIO_INCOMPLETE)
	    fprintf(outf, "(%s)\n", mgr.mgr_name);
	else if (s == AIO_OK)
	    fprintf(outf, "(%s at %s)\n", mgr.mgr_name, mgr.hw_address);
	else 
	    fprintf(outf, "(unknown port?)\n");
	fprintf(outf, "    port_id             = %-6d",  tp->port_id);
	fprintf(outf, "    subqueue_id         = %2d\n", tp->subqueue_id);
	fprintf(outf, "    tag.message_id      = 0x%04x",
		tp->timer_tag.message_id);
	fprintf(outf, "    tag.transaction_num = 0x%08x\n",
		tp->timer_tag.transaction_num);
	if (verbose)
	    fprintf(outf, "    t_next              = 0x%08x [%2d]\n",
		tp->t_next, tp->t_next ? tp->t_next - ktimer_list : -1);
    }
}

an_ioserv_mem(outf)
    FILE	*outf;
{
    int		i, word, found_problem, bit;
    unsigned	first, bit_mask;

    fprintf(outf,"\nChecking memory freelists used by io_get_mem/io_rel_mem:\n\n");

    word = found_problem = bit = 0;
    bit_mask = 0x80000000;
    first = 0;

    /* check dup bits */
    for (i=0 ; i<(io_mem_top-io_mem_buf) ; i++) {

	/* no dups yet, seeing a dup now */
	if ( (first == 0) && ( (io_mem_map_dup[word] & bit_mask) != 0) ) {
	    first = io_mem_buf + (word*32) + bit;
	    found_problem = 1;
	}

	/* have been doing a dup, but this one isn't */
	else if ( (first != 0) && ((io_mem_map_dup[word] & bit_mask) == 0) ) {
	    aio_mem_problem(first,(int)(io_mem_buf+(word*32)+bit-first),
		"    ",outf);
	    first = 0;
	}

	/* move on */
	if (bit_mask == 1) {
	    word++;
	    bit = 0;
	    bit_mask = 0x80000000;
	}
	else {
	    bit++;
	    bit_mask >>= 1;
	}
    }

    /* got to end while handling a problem */
    if (first != 0)
	aio_mem_problem(first,(int)(io_mem_top-first),"    ",outf);

    if (found_problem == 0)
	fprintf(outf,"  all free lists ok\n");
}

static struct {
    char *name;
    sbit8 value;
} status_table[] = {
	{ "INVALID_SEARCH_KEY",		INVALID_SEARCH_KEY	},
	{ "LLIO_ABORTED",		LLIO_ABORTED		},
	{ "LLIO_BAD_FUNC_CODE",		LLIO_BAD_FUNC_CODE	},
	{ "LLIO_BAD_HM_PORT_NUM",	LLIO_BAD_HM_PORT_NUM	},
	{ "LLIO_BAD_LM_PORT_NUM",	LLIO_BAD_LM_PORT_NUM	},
	{ "LLIO_BEGINNING_OF_MEDIA",	LLIO_BEGINNING_OF_MEDIA	},
	{ "LLIO_BIND_BAD_HW_ADDR",	LLIO_BIND_BAD_HW_ADDR	},
	{ "LLIO_BIND_BAD_META",		LLIO_BIND_BAD_META	},
	{ "LLIO_BIND_BAD_REV_CODE",	LLIO_BIND_BAD_REV_CODE	},
	{ "LLIO_BIND_BAD_SUBSYS",	LLIO_BIND_BAD_SUBSYS	},
	{ "LLIO_BIND_HW_ERR",		LLIO_BIND_HW_ERR	},
	{ "LLIO_BIND_HW_TO",		LLIO_BIND_HW_TO		},
	{ "LLIO_BUSY",			LLIO_BUSY		},
	{ "LLIO_CANNOT_BIND_LOWER",	LLIO_CANNOT_BIND_LOWER	},
	{ "LLIO_CANNOT_DO_UNBIND",	LLIO_CANNOT_DO_UNBIND	},
	{ "LLIO_CANNOT_GET_MEM",	LLIO_CANNOT_GET_MEM	},
	{ "LLIO_CANNOT_IDY",		LLIO_CANNOT_IDY		},
	{ "LLIO_CANNOT_RELEASE_MEM",	LLIO_CANNOT_RELEASE_MEM	},
	{ "LLIO_CANNOT_RELEASE_RESOURCE",LLIO_CANNOT_RELEASE_RESOURCE	},
	{ "LLIO_CHANNEL_TIMEOUT",	LLIO_CHANNEL_TIMEOUT	},
	{ "LLIO_CLEAR_FAILURE",		LLIO_CLEAR_FAILURE	},
	{ "LLIO_DATA_ERROR",		LLIO_DATA_ERROR		},
	{ "LLIO_DATA_OVERRUN",		LLIO_DATA_OVERRUN	},
	{ "LLIO_DEVICE_OFFLINE",	LLIO_DEVICE_OFFLINE	},
	{ "LLIO_DEVICE_RESET",		LLIO_DEVICE_RESET	},
	{ "LLIO_DEV_POWER_ON",		LLIO_DEV_POWER_ON	},
	{ "LLIO_DOWNLOAD_FAIL",		LLIO_DOWNLOAD_FAIL	},
	{ "LLIO_END_MEDIA",		LLIO_END_MEDIA		},
	{ "LLIO_END_OF_MEDIA",		LLIO_END_OF_MEDIA	},
	{ "LLIO_EOF_FOUND",		LLIO_EOF_FOUND		},
	{ "LLIO_ERROR_RD_STATUS",	LLIO_ERROR_RD_STATUS	},
	{ "LLIO_HW_EOF",		LLIO_HW_EOF		},
	{ "LLIO_HW_PROBLEM",		LLIO_HW_PROBLEM		},
	{ "LLIO_INTERNAL_ERROR",	LLIO_INTERNAL_ERROR	},
	{ "LLIO_INV_ADDR",		LLIO_INV_ADDR		},
	{ "LLIO_INV_REQ_TRANS_LEN",	LLIO_INV_REQ_TRANS_LEN	},
	{ "LLIO_LOCKED_BY_OTHER",	LLIO_LOCKED_BY_OTHER	},
	{ "LLIO_LOCK_GRANTED_BUSY",	LLIO_LOCK_GRANTED_BUSY	},
	{ "LLIO_LOCK_GRANTED_SAFE",	LLIO_LOCK_GRANTED_SAFE	},
	{ "LLIO_LOCK_STAT_ALREADY",	LLIO_LOCK_STAT_ALREADY	},
	{ "LLIO_LOCK_STAT_ALREADY_BUSY",LLIO_LOCK_STAT_ALREADY_BUSY	},
	{ "LLIO_LOCK_STAT_ALREADY_SAFE",LLIO_LOCK_STAT_ALREADY_SAFE	},
	{ "LLIO_LOWER_MGR_ERROR",	LLIO_LOWER_MGR_ERROR	},
	{ "LLIO_MARGINAL_DATA",		LLIO_MARGINAL_DATA	},
	{ "LLIO_MEMORY_ERROR",		LLIO_MEMORY_ERROR	},
	{ "LLIO_MUST_BE_LOCKED",	LLIO_MUST_BE_LOCKED	},
	{ "LLIO_NORMAL_STATUS",		LLIO_NORMAL_STATUS	},
	{ "LLIO_NOT_BOUND",		LLIO_NOT_BOUND		},
	{ "LLIO_NOT_IMPLEMENTED",	LLIO_NOT_IMPLEMENTED	},
	{ "LLIO_NOT_LM_BOUND",		LLIO_NOT_LM_BOUND	},
	{ "LLIO_NOT_PRESENT",		LLIO_NOT_PRESENT	},
	{ "LLIO_NOT_READY",		LLIO_NOT_READY		},
	{ "LLIO_NOT_UNDERSTOOD",	LLIO_NOT_UNDERSTOOD	},
	{ "LLIO_NO_CTRL_Y_PIN",		LLIO_NO_CTRL_Y_PIN	},
	{ "LLIO_NO_MIRRORLOG",		LLIO_NO_MIRRORLOG	},
	{ "LLIO_NO_STATUS_YET",		LLIO_NO_STATUS_YET	},
	{ "LLIO_NO_UNBD_ACTIVE_IO",	LLIO_NO_UNBD_ACTIVE_IO	},
	{ "LLIO_NO_WRITE_ACCESS",	LLIO_NO_WRITE_ACCESS	},
	{ "LLIO_OK",			LLIO_OK			},
	{ "LLIO_ONLINE_DISK_FAILED",	LLIO_ONLINE_DISK_FAILED	},
	{ "LLIO_OVER_HUMIDITY",		LLIO_OVER_HUMIDITY	},
	{ "LLIO_PARITY_ERROR",		LLIO_PARITY_ERROR	},
	{ "LLIO_POWERFAIL_ABORTED",	LLIO_POWERFAIL_ABORTED	},
	{ "LLIO_PROTOCOL_ERROR",	LLIO_PROTOCOL_ERROR	},
	{ "LLIO_READ_TERM_BY_AEOR",	LLIO_READ_TERM_BY_AEOR	},
	{ "LLIO_READ_TERM_BY_BREAK",	LLIO_READ_TERM_BY_BREAK	},
	{ "LLIO_READ_TIMEOUT",		LLIO_READ_TIMEOUT	},
	{ "LLIO_READ_TIME_RETURNED",	LLIO_READ_TIME_RETURNED	},
	{ "LLIO_RECEIVER_OVERRUN",	LLIO_RECEIVER_OVERRUN	},
	{ "LLIO_REIMAGE_DONE",		LLIO_REIMAGE_DONE	},
	{ "LLIO_REIMAGE_FAILED",	LLIO_REIMAGE_FAILED	},
	{ "LLIO_REQ_IN_INV_STATE",	LLIO_REQ_IN_INV_STATE	},
	{ "LLIO_REQ_NOT_STARTED",	LLIO_REQ_NOT_STARTED	},
	{ "LLIO_RETRANSMIT_BLOCK",	LLIO_RETRANSMIT_BLOCK	},
	{ "LLIO_RETRY_INCURRED",	LLIO_RETRY_INCURRED	},
	{ "LLIO_SETMARK_FOUND",		LLIO_SETMARK_FOUND	},
	{ "LLIO_STILL_BOUND",		LLIO_STILL_BOUND	},
	{ "LLIO_SUSPECT_DATA",		LLIO_SUSPECT_DATA	},
	{ "LLIO_SW_PROBLEM",		LLIO_SW_PROBLEM		},
	{ "LLIO_TIMEOUT",		LLIO_TIMEOUT		},
	{ "LLIO_TIMEOUT_ABORT",		LLIO_TIMEOUT_ABORT	},
	{ "LLIO_TIMEOUT_ABORT_FAILED",	LLIO_TIMEOUT_ABORT_FAILED},
	{ "LLIO_TOO_MANY",		LLIO_TOO_MANY		},
	{ "LLIO_TRANSMISSION_ERR",	LLIO_TRANSMISSION_ERR	},
	{ "LLIO_UNINIT_MEDIA",		LLIO_UNINIT_MEDIA	},
	{ "LLIO_WRONG_ID_CODE",		LLIO_WRONG_ID_CODE	},
	{ "LPMC_ERR_NUM",		LPMC_ERR_NUM		},
	{ "MESSAGE_NOT_FOUND",		MESSAGE_NOT_FOUND	},
	{ "UNSUPPORTED_OPTION",		UNSUPPORTED_OPTION	},
};

an_ioserv_decode_status(llio_status, outf)
    llio_status_type	llio_status;
    FILE		*outf;
{
    int i;

    fprintf(outf, "generic llio_status = <0x%08x> means ", llio_status);
    for (i = 0; i < sizeof(status_table)/sizeof(status_table[0]); i++) {
	if (status_table[i].value == llio_status.u.error_num) {
	    fprintf(outf, "%s\n", status_table[i].name);
	    return;
	}
    }
    fprintf(outf, "NOTHING! -- can't decode!\n");
}

an_ioserv_decode_message(msg, prefix, o)
    io_message_type	*msg;
    char		*prefix;
    FILE		*o;
{
    unsigned addr;
    int      i;

    /* for each msg type, decode it */
    switch (msg->msg_header.msg_descriptor) {

	case ABORT_EVENT_MSG:
	    fprintf(o, "ABORT_EVENT_MSG\n");
	    break;

	case CREATION_MSG:
	    fprintf(o, "CREATION_MSG\n");
	    fprintf(o, "%screation options   = 0x%02x\n",prefix,
	            msg->union_name.creation_info.create_options);
	    fprintf(o, "%sserver data length = 0x%08x\n",prefix,
	            msg->union_name.creation_info.server_data_len);
	    fprintf(o, "%smessage size       = 0x%08x\n",prefix,
	            msg->union_name.creation_info.max_msg_size);
	    fprintf(o, "%snumber of messages = 0x%08x\n",prefix,
	            msg->union_name.creation_info.num_msgs);
	    fprintf(o, "%snumber of subqs    = 0x%02x\n",prefix,
	            msg->union_name.creation_info.num_subqueues);
	    fprintf(o, "%sport number        = 0x%02x\n",prefix,
	            msg->union_name.creation_info.port_num);
	    break;

	case DO_BIND_REQ_MSG:
	    fprintf(o, "DO_BIND_REQ_MSG\n");
	    fprintf(o, "%sreply subq            = 0x%02x\n",prefix,
	            msg->union_name.do_bind_req.reply_subq);
	    fprintf(o, "%smgr's port number     = 0x%04x\n",prefix,
	            msg->union_name.do_bind_req.mgr_port_num);
	    fprintf(o, "%sconfig addr 3         = 0x%08x\n",prefix,
	            msg->union_name.do_bind_req.config_addr_3);
	    fprintf(o, "%sconfig addr 2         = 0x%08x\n",prefix,
	            msg->union_name.do_bind_req.config_addr_2);
	    fprintf(o, "%sconfig addr 1         = 0x%08x\n",prefix,
	            msg->union_name.do_bind_req.config_addr_1);
	    fprintf(o, "%slower mgr port number = 0x%04x\n",prefix,
	            msg->union_name.do_bind_req.lm_port_num);
	    break;

	case DO_BIND_REPLY_MSG:
	    fprintf(o, "DO_BIND_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.do_bind_reply.reply_status,o);
	    break;

	case BIND_REQ_MSG:
	    fprintf(o, "BIND_REQ_MSG\n");
	    fprintf(o, "%sreply subq              = 0x%02x\n",prefix,
	            msg->union_name.bind_req.reply_subq);
	    fprintf(o, "%shigher mgr event subq   = 0x%02x\n",prefix,
	            msg->union_name.bind_req.hm_event_subq);
	    fprintf(o, "%shigher mgr subsystem    = 0x%04x\n",prefix,
	            msg->union_name.bind_req.hm_subsys_num);
	    fprintf(o, "%shigher mgr metalanguage = 0x%04x\n",prefix,
	            msg->union_name.bind_req.hm_meta_lang);
	    fprintf(o, "%shigher mgr rev code     = 0x%08x\n",prefix,
	            msg->union_name.bind_req.hm_rev_code);
	    fprintf(o, "%shigher mgr config 3     = 0x%08x\n",prefix,
	            msg->union_name.bind_req.hm_config_addr_3);
	    fprintf(o, "%shigher mgr config 2     = 0x%08x\n",prefix,
	            msg->union_name.bind_req.hm_config_addr_2);
	    fprintf(o, "%shigher mgr config 1     = 0x%08x\n",prefix,
	            msg->union_name.bind_req.hm_config_addr_1);
	    break;

	case BIND_REPLY_MSG:
	    fprintf(o, "BIND_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.bind_reply.reply_status,o);
	    fprintf(o, "%slower mgr rev code      = 0x%08x\n",prefix,
	            msg->union_name.bind_reply.lm_rev_code);
	    fprintf(o, "%slower mgr queue depth   = 0x%04x\n",prefix,
	            msg->union_name.bind_reply.lm_queue_depth);
	    fprintf(o, "%slower mgr low req subq  = 0x%02x\n",prefix,
	            msg->union_name.bind_reply.lm_low_req_subq);
	    fprintf(o, "%slower mgr high req subq = 0x%02x\n",prefix,
	            msg->union_name.bind_reply.lm_hi_req_subq);
	    fprintf(o, "%slower mgr freeze data   = 0x%02x\n",prefix,
	            msg->union_name.bind_reply.lm_freeze_data);
	    fprintf(o, "%slower mgr alignment     = 0x%02x\n",prefix,
	            msg->union_name.bind_reply.lm_alignment);
	    break;

	case DO_UNBIND_REQ_MSG:
	    fprintf(o, "DO_UNBIND_REQ_MSG\n");
	    fprintf(o, "%sreply subq             = 0x%02x\n",prefix,
	            msg->union_name.do_unbind_req.reply_subq);
	    fprintf(o, "%slower mgr port number  = 0x%08x\n",prefix,
	            msg->union_name.do_unbind_req.lm_port_num);
	    break;

	case DO_UNBIND_REPLY_MSG:
	    fprintf(o, "DO_UNBIND_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.do_unbind_reply.reply_status,o);
	    break;

	case UNBIND_REQ_MSG:
	    fprintf(o, "UNBIND_REQ_MSG\n");
	    fprintf(o, "%sreply subq             = 0x%02x\n",prefix,
	            msg->union_name.unbind_req.reply_subq);
	    fprintf(o, "%shigher mgr port number = 0x%08x\n",prefix,
	            msg->union_name.unbind_req.hm_port_num);
	    break;

	case UNBIND_REPLY_MSG:
	    fprintf(o, "UNBIND_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.unbind_reply.reply_status,o);
	    break;

	case DIE_REQ_MSG:
	    fprintf(o, "DIE_REQ_MSG\n");
	    fprintf(o, "%sreply subq  = 0x%02x\n",prefix,
	            msg->union_name.die_req.reply_subq);
	    break;

	case DIE_REPLY_MSG:
	    fprintf(o, "DIE_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.die_reply.reply_status,o);
	    break;

	case INT_COMPLETION_MSG:
	    fprintf(o, "INT_COMPLETION_MSG\n");
	    fprintf(o, "%scompletion entry  = 0x%08x\n",prefix,
	            (int)msg->union_name.int_completion.comp_entry);
	    addr = an_ioserv_print_compl_entry(
	                 (unsigned)msg->union_name.int_completion.comp_entry,
	                 prefix, o);
	    break;

	case INT_DIRECT_MSG:
	    fprintf(o, "INT_DIRECT_MSG\n");
	    fprintf(o, "%shpa address  = 0x%08x\n",prefix,
	            (int)msg->union_name.int_direct.hpa);
	    break;

	case PROBE_REQ_MSG:
	    fprintf(o, "PROBE_REQ_MSG\n");
	    fprintf(o, "%sreply subq = 0x%02x\n", prefix,
		msg->union_name.probe_req.reply_subq);
	    fprintf(o, "%sprobe option = 0x%02x  ", prefix,
		msg->union_name.probe_req.probe_option);
	    switch (msg->union_name.probe_req.probe_option) {
		case PROBE_FIRST:   fprintf(o, "(PROBE_FIRST)\n");	break;
		case PROBE_NEXT:    fprintf(o, "(PROBE_NEXT)\n");	break;
		case PROBE_ADDRESS: fprintf(o, "(PROBE_ADDRESS)\n");	break;
		default:	    fprintf(o, "(unknown!)\n");		break;
	    }
	    fprintf(o, "%shardware address = 0x%08x\n", prefix,
		msg->union_name.probe_req.hdw_address);
	    fprintf(o, "%snumber of layers = 0x%02x\n", prefix,
		msg->union_name.probe_req.num_layers);
	    fprintf(o, "%shardware layers  = 0x", prefix);
	    for (addr = 0; addr < MAX_PROBE_LAYERS; addr++)
		fprintf(o, "%08x.", msg->union_name.probe_req.hdw_layers[addr]);
	    fprintf(o, "\n");
	    break;

	case PROBE_REPLY_MSG:
	    fprintf(o, "PROBE_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.probe_reply.reply_status, o);
	    fprintf(o, "%shardware address = 0x%08x\n", prefix,
		msg->union_name.probe_reply.hdw_address);
	    fprintf(o, "%sentity id        = 0x%08x\n", prefix,
		msg->union_name.probe_reply.ent_id);
	    fprintf(o, "%snumber of layers = 0x%02x\n", prefix,
		msg->union_name.probe_reply.num_layers);
	    fprintf(o, "%shardware layers  = 0x", prefix);
	    for (addr = 0; addr < MAX_PROBE_LAYERS; addr++) {
		fprintf(o, "%08x.",
		    msg->union_name.probe_reply.hdw_layers[addr]);
	    }
	    fprintf(o, "\n");
	    break;

	case POWER_ON_REQ_MSG:
	    fprintf(o, "POWER_ON_REQ_MSG\n");
	    break;

	case POWER_ON_REPLY_MSG:
	    fprintf(o, "POWER_ON_REPLY_MSG\n");
	    break;

	case TIMER_EVENT_MSG:
	    fprintf(o, "TIMER_EVENT_MSG\n");
	    break;

	case DIAG_REQ_MSG:
	    fprintf(o, "DIAG_REQ_MSG\n");
	    break;

	case DIAG_REPLY_MSG:
	    fprintf(o, "DIAG_REPLY_MSG\n");
	    break;

	case DIAG_EVENT_MSG:
	    fprintf(o, "DIAG_EVENT_MSG\n");
	    fprintf(o, "%s", prefix);
	    aio_decode_llio_status(msg->union_name.diag_event.llio_status, o);
	    fprintf(o, "%sdiag_class      = 0x%02x  ", prefix,
		msg->union_name.diag_event.diag_class);
	    switch (msg->union_name.diag_event.diag_class) {
		case DIAG_HW_EVENT:   fprintf(o, "(DIAG_HW_EVENT)\n");	 break;
		case DIAG_SW_EVENT:   fprintf(o, "(DIAG_SW_EVENT)\n");	 break;
		case DIAG_OTHER_EVENT:fprintf(o, "(DIAG_OTHER_EVENT)\n");break;
		default:	      fprintf(o, "(unknown!)\n");	 break;
	    }
	    fprintf(o, "%shw_status_len   = 0x%02x\n", prefix,
		msg->union_name.diag_event.hw_status_len);
	    fprintf(o, "%smgr_info_len    = 0x%02x\n", prefix,
		msg->union_name.diag_event.mgr_info_len);
	    fprintf(o, "%sretry_count     = 0x%02x\n", prefix,
		msg->union_name.diag_event.retry_count);
	    fprintf(o, "%slog_all_retries = %s\n", prefix,
		BOOLEAN(msg->union_name.diag_event.log_all_retries));
	    fprintf(o, "%sretry_again     = %s\n", prefix,
		BOOLEAN(msg->union_name.diag_event.retry_again));
	    fprintf(o, "%sio_worked       = %s\n", prefix,
		BOOLEAN(msg->union_name.diag_event.io_worked));
	    fprintf(o, "%srun_autodiag    = %s\n", prefix,
		BOOLEAN(msg->union_name.diag_event.run_autodiag));
	    fprintf(o, "%smore_log_info   = %s\n", prefix,
		BOOLEAN(msg->union_name.diag_event.more_log_info));
	    fprintf(o, "%sbuffer          = \n", prefix);
	    addr = msg->union_name.diag_event.mgr_info_len
		 + msg->union_name.diag_event.hw_status_len;
	    if (addr > MAX_BUFF_BYTES) addr = MAX_BUFF_BYTES;
	    for (i = 0; i < addr; i++) {
		if ((i % 16) == 0) fprintf(o, "%s", prefix);
		fprintf(o, "%02x ",
			msg->union_name.diag_event.u.bit8_buff[i] & 0xff);
		if ((i % 16) == 15) fprintf(o, "\n");
	    }
	    if ((i % 16) != 0) fprintf(o, "\n");
	    break;

	case STATUS_REQ_MSG:
	    fprintf(o, "STATUS_REQ_MSG\n");
	    fprintf(o, "%sreply subq  = 0x%02x\n",prefix,
	            msg->union_name.status_req.reply_subq);
	    break;

	case STATUS_REPLY_MSG:
	    fprintf(o, "STATUS_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.status_reply.reply_status,o);
	    fprintf(o, "%shw status   = 0x%08x\n",prefix,
	            msg->union_name.status_reply.hw_status);
	    fprintf(o, "%ssw status   = 0x%08x\n",prefix,
	            msg->union_name.status_reply.sw_status);
	    break;

	case RESET_REQ_MSG:
	    fprintf(o, "RESET_REQ_MSG\n");
	    fprintf(o, "%sreply subq  = 0x%02x\n",prefix,
	            msg->union_name.reset_req.reply_subq);
	    break;

	case RESET_REPLY_MSG:
	    fprintf(o, "RESET_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.reset_reply.reply_status,o);
	    break;

	case LOCK_REQ_MSG:
	    fprintf(o, "LOCK_REQ_MSG\n");
	    fprintf(o, "%sreply subq  = 0x%02x\n",prefix,
	            msg->union_name.lock_req.reply_subq);
	    fprintf(o, "%slock_event  = 0x%02x  ",prefix,
	            msg->union_name.lock_req.lock_event);
	    switch (msg->union_name.lock_req.lock_event) {
		case LOCK_EVENT_IGNORE: fprintf(o, "(LOCK_EVENT_IGNORE)\n");
					break;
		case LOCK_EVENT_NORMAL: fprintf(o, "(LOCK_EVENT_NORMAL)\n");
					break;
		case LOCK_EVENT_COPY:   fprintf(o, "(LOCK_EVENT_COPY)\n");
					break;
		case LOCK_EVENT_DIVERT: fprintf(o, "(LOCK_EVENT_DIVERT)\n");
					break;
		default:		fprintf(o, "(unknown!)\n");
	    }
	    if ( (msg->union_name.lock_req.lock_event == LOCK_EVENT_COPY) ||
	         (msg->union_name.lock_req.lock_event == LOCK_EVENT_DIVERT) ) {
		fprintf(o, "%sevent subq  = 0x%02x\n",prefix,
			msg->union_name.lock_req.u.event_subq);
		fprintf(o, "%sevent port  = 0x%02x\n",prefix,
			msg->union_name.lock_req.u.event_port);
	    }
	    break;

	case LOCK_REPLY_MSG:
	    fprintf(o, "LOCK_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.lock_reply.reply_status,o);
	    fprintf(o, "%srequest subq  = 0x%02x\n",prefix,
	            msg->union_name.lock_reply.request_subq);
	    break;

	case UNLOCK_REQ_MSG:
	    fprintf(o, "UNLOCK_REQ_MSG\n");
	    fprintf(o, "%sreply subq  = 0x%02x\n",prefix,
	            msg->union_name.unlock_req.reply_subq);
	    break;

	case UNLOCK_REPLY_MSG:
	    fprintf(o, "UNLOCK_REPLY_MSG\n");
	    fprintf(o, "%s",prefix);
	    aio_decode_llio_status(msg->union_name.unlock_reply.reply_status,o);
	    break;

        default:
	    fprintf(o, "unknown message type!\n");
    }
}

dump_io_tree(entry, level, outf)
    aio_tree_entry *entry;
    int             level;
    FILE	   *outf;
{
    int i;
    aio_tree_entry *start = entry;

    do {
	fprintf(outf, "[%2d] ", entry - io_tree_ptr);
	for (i = 0; i < level; i++) fprintf(outf, "   ");
	fprintf(outf, "%-2d ", entry->hdw_address);
	fprintf(outf, "%s/%s ",  entry->module  ? entry->module->name  : no_mod,
			  entry->manager ? entry->manager->name : no_mgr);
	if (entry->lu != NONE && entry->manager && entry->manager->options.ldm)
	    fprintf(outf, "lu %2d ", entry->lu);
	if (entry->port != NONE) fprintf(outf, "port %2d ", entry->port);
	fprintf(outf, "idy ");
	switch (entry->state.ent_id_state) {
	    case ES_VALID:	 fprintf(outf, "0x%x ", entry->ent_id); break;
	    case ES_NEVER_PROBED:fprintf(outf, "Never_Probed ");	break;
	    case ES_NO_HARDWARE: fprintf(outf, "No_Hardware ");		break;
	    case ES_UNRECOGNIZED:fprintf(outf, "Unrecognized ");	break;
	    case ES_PARENT_WONT_PROBE:
		fprintf(outf, "Parent_Wont_Probe ");			break;
	    case ES_PROBE_FAILED:fprintf(outf, "Probe_Failed ");	break;
	    default:
		fprintf(outf, "ES_%d? ", entry->state.ent_id_state);	break;
	}
	switch (entry->state.bound_state) {
	    case BS_BOUND:
		fprintf(outf, "mi %2d ", entry->mgr_index);		break;
	    case BS_NEVER_ACCESSED:	fprintf(outf, "Never_Accessed ");break;
	    case BS_UNBOUND:		fprintf(outf, "Unbound ");	break;
	    case BS_TOO_MANY:		fprintf(outf, "Too_Many ");	break;
	    case BS_NO_RESOURCES:	fprintf(outf, "No_Resources ");	break;
	    case BS_MGR_IN_WAY:		fprintf(outf, "Mgr_In_Way ");	break;
	    case BS_NO_MODULE:		fprintf(outf, "No_Module ");	break;
	    case BS_BIND_FAILED:	fprintf(outf, "Bind_Failed ");	break;
	    default:
		fprintf(outf, "BS_%d? ", entry->state.bound_state);	break;
	}
	fprintf(outf, "from<");
	if (entry->state.from_gen)	(void)fputc('g', outf);
	if (entry->state.from_probe)	(void)fputc('p', outf);
	if (entry->state.from_file)	(void)fputc('i', outf);
	if (entry->state.forced)	(void)fputc('f', outf);
	fprintf(outf, ">\n");
	if (entry->child != 0) dump_io_tree(entry->child, level+1, outf);
    } while ((entry = entry->sibling) != start);
}
#endif
