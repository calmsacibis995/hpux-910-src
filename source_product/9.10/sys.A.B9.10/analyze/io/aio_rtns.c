#ifdef iostuff

#include "../standard/inc.h"
#include <string.h>
#include <ctype.h>
#include "machine/cpu.h"
#include <sio/llio.h>
#include <sio/iotree.h>
#include "aio.h"
#include <sioserv/ioserv.h>

/* externally defined (and documented) analyze functions */
int aio_decode_message();
int aio_em_fe();
int aio_get_mgr_info();
int aio_get_mgr_name();
int aio_init_llio_status_decode();
int aio_init_message_decode();
int aio_queued_message();
int aio_rev_mismatch();
int an_dump_hex_ascii();
int an_grab_extern();
int an_grab_real_chunk();
int an_grab_virt_chunk();
int an_vtor();
/*void aio_decode_llio_status();*/
/*void an_display_sym();*/

/* analyze globals this file needs */
extern struct pde			*pdir;
extern int				niopdir;
extern int				allow_sigint;
extern char				*cursym;

/* globals declared in an_ioserv.c */
extern imc_port_ptr	                *vport_index;
extern msg_frame_ptr 			vfirst_frame;
extern msg_frame_ptr 			first_frame;
extern msg_frame_ptr 			vlast_frame;
extern unsigned				io_mem_buf;
extern unsigned				io_mem_top;
extern int				*io_mem_map;

extern struct ioheader_type  		ioheader;
extern aio_tree_entry  			*io_tree_ptr;

extern struct iotree_type  		*iotree;

extern aio_tree_entry			*port_to_io_tree();
extern aio_tree_entry			*index_to_node();
extern imc_port_ptr			port_to_imc_ptr();
extern void				*calloc();

/* structure definitions for llio status and message decoding */
typedef struct llio_decode_blk {
    struct llio_decode_blk	*link;
    shortint			subsystem;
    int				(*decode_rtn)();
} llio_decode_blk; llio_decode_blk *llio_decode_ptr;

typedef struct msg_decode_blk {
    struct msg_decode_blk	*link;
    shortint			low_msg_descriptor;
    shortint			high_msg_descriptor;
    int				(*decode_rtn)();
} msg_decode_blk; msg_decode_blk *msg_decode_ptr;

/****************************************************************************/
/**                                                                        **/
/**  This routine grabs a lot of commonly required variables out of the    **/
/**  imcport and iotree and passes them back to the manager analysis       **/
/**  routine in a single structure. Two different input modes are          **/
/**  supported, port number and iotree index.                              **/
/**                                                                        **/
/****************************************************************************/
int
aio_get_mgr_info(call_type, parm, mgr)
    int			 call_type;	/* input is port or iotree index    */
    int			 parm;		/* port or iotree index		    */
    struct mgr_info_type *mgr;		/* returned info structure	    */
{
    port_num_type	 port_num;	/* port number for this manager	    */
    int			 iotree_index;	/* iotree index for this manager    */
    aio_tree_entry	 *node;		/* io_tree entry for this manager   */
    imc_port_ptr	 port_ptr;	/* imc port for this manager	    */

    /* if we start with a port number, grab the iotree index, the iotree */
    /*   entry and the imcport structure                                 */
    if (call_type == AIO_PORT_NUM_TYPE) {
	if (aio_get_mgr_name(port_num = parm, mgr->mgr_name) != AIO_OK)
	    return(AIO_NO_SUCH_PORT);
	if ((node = port_to_io_tree(port_num)) == 0)
	    return(AIO_INCOMPLETE);
	iotree_index = node_to_index(node);
    }

    /* if we start with an io_tree node, grab the port number and the	*/
    /*   iotree index 							*/
    else if (call_type == AIO_IO_NODE_TYPE) {
	node = (aio_tree_entry *)parm;
	if ((node - io_tree_ptr) < 0 ||
	    (node - io_tree_ptr) > ioheader.num_io_tree_recs)
	    return(AIO_NO_SUCH_PORT);
	port_num = node->port;
	iotree_index = node_to_index(node);
    }

    /* if we start with an iotree index, grab the port number, the iotree */
    /*   entry and the imcport structure                                  */
    else if (call_type == AIO_IOTREE_TYPE) {
	if ((node = index_to_node(iotree_index = parm)) == 0)
	    return(AIO_NO_SUCH_PORT);
	port_num = node->port;
    }

    /* otherwise, this is a bad call -- blow it off */
    else
	return(AIO_NO_SUCH_OPTION);

    port_ptr = port_to_imc_ptr(port_num);
    (void)aio_get_mgr_name(port_num, mgr->mgr_name);
    mgr->port_num	= port_num;
    mgr->enabled_subqs	= port_ptr->enabled_subqueues;
    mgr->active_subqs	= port_ptr->active_subqueues;
    mgr->blocked	= port_ptr->status.f.blocked;
    mgr->pda_address	= (unsigned)port_ptr->pda;
    mgr->io_tree_entry	= node;
    mgr->iotree_index	= iotree_index;
    mgr->mgr_table_index= iotree[iotree_index].mgr;
    mgr->eim		= 0;
    mgr->in_poll_list	= 0;

    /* fill in the manager's hw address */
    aio_get_hw_add(node, mgr->hw_address);

    /* fill in the next iotree index (index of next instance with same port) */
    if (node->manager->options.shared_port && node->mi_link != 0) {
	mgr->next_io_tree_entry = node->mi_link;
	mgr->next_iotree_index  = node_to_index(node->mi_link);
    }
    else {
	mgr->next_io_tree_entry = 0;
	mgr->next_iotree_index  = NONE;
    }
    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine sets up the status decode chain which                    **/
/**  aio_decode_llio_status will follow to determine which routine should  **/
/**  decode a given llio_status. Duplicate susbsystems are checked for.    **/
/**                                                                        **/
/****************************************************************************/
int
aio_init_llio_status_decode(subsystem, function_name)
    shortint		subsystem;		/* subsys to respond to	     */
    int			(*function_name)();	/* decode rtn to call	     */
{
    llio_decode_blk	*item;			/* chain element walker	     */
    llio_decode_blk	*last;			/* chain element walker	     */
    llio_decode_blk	*new;			/* new chain element	     */

    /* if decode chain is empty allocate an entry and set up start of list */
    
    if ((item = llio_decode_ptr) == 0)
	llio_decode_ptr = new =
		(struct llio_decode_blk *)calloc(1, sizeof(*new));
    else {	/* else walk through chain and add an element to end of list */
	do {
	    if (item->subsystem == subsystem) return(AIO_SUBSYS_USED);
	    last = item;
	    item = item->link;
	} while (last->link != 0);

	last->link = new = (struct llio_decode_blk *) calloc(1, sizeof(*new));
    }

    /* fill in the new element */
    new->subsystem  = subsystem;
    new->decode_rtn = function_name;
    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine decodes all llio status values. It does this by calling  **/
/**  the correct manager status decoding routine (based on the subsystem   **/
/**  number of the llio status variable). These decoding routines are      **/
/**  set up during initialization by aio_init_llio_status_decode.          **/
/**                                                                        **/
/****************************************************************************/

aio_decode_llio_status(status, outf)
    llio_status_type	status;		/* input llio status value	  */
    FILE		*outf;		/* output file			  */
{
    llio_decode_blk	*item;		/* element of decode chain	  */

    /* if this is a global status, call i/o services decoding */
    if (status.u.proc_num >= 0) {
	an_ioserv_decode_status(status, outf);
	return;
    }

    /* non-global status -- scan the decode chain and process if we can */
    for (item = llio_decode_ptr; item != 0; item = item->link) {
	if (item->subsystem == status.u.subsystem) {
	    (*item->decode_rtn)(status, outf);
	    return;
	}
    }
	    
    /* couldn't decode this status -- print a warning */
    fprintf(outf, "No decoding available for llio status <0x%08x>\n",
            status.is_ok);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine sets up the message decode chain which		   **/
/**  aio_decode_message will use to call routines to decode messages.      **/
/**  Overlapping ranges are checked for.                                   **/
/**                                                                        **/
/****************************************************************************/
int
aio_init_message_decode(low_msg_descriptor, high_msg_descriptor, function_name)
    shortint	low_msg_descriptor;		/* first msg descr to decode */
    shortint	high_msg_descriptor;    	/* last msg descr to decode  */
    int		(*function_name)();     	/* decoder rtn		     */
{
    msg_decode_blk	*item;			/* decode chain walker	     */
    msg_decode_blk	*last;			/* decode chain walker	     */
    msg_decode_blk	*new;			/* new chain element	     */

    /* do basic error checking */
    if (low_msg_descriptor > high_msg_descriptor)
	return(AIO_INVALID_RANGE);

    /* if decode chain is empty allocate an entry and set up start of list */
    
    if ((item = msg_decode_ptr) == 0)
	msg_decode_ptr = new = (struct msg_decode_blk *)calloc(1 ,sizeof(*new));

    /* else walk through chain and add an element to end of list */
    else {
	do {
	    if (low_msg_descriptor >= item->low_msg_descriptor &&
	        low_msg_descriptor <= item->high_msg_descriptor)
		return(AIO_INVALID_RANGE);
	    if (high_msg_descriptor >= item->low_msg_descriptor &&
	        high_msg_descriptor <= item->high_msg_descriptor)
		return(AIO_INVALID_RANGE);
	    if (low_msg_descriptor  <= item->low_msg_descriptor &&
	        high_msg_descriptor >= item->high_msg_descriptor)
		return(AIO_INVALID_RANGE);
	    last = item;
	    item = item->link;
	} while ((int)last->link != 0);

	last->link = new = (struct msg_decode_blk *)calloc(1, sizeof(*new));
    }

    /* fill in the new element */
    new->low_msg_descriptor	= low_msg_descriptor;
    new->high_msg_descriptor	= high_msg_descriptor;
    new->decode_rtn		= function_name;
    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine decodes all messages. It does this by calling the        **/
/**  correct manager status decoding routine (based on the message         **/
/**  descriptor of the message). These decoding routines are set up        **/
/**  during initialization by aio_init_message_decode. 		           **/
/**                                                                        **/
/**  This routine prints out the header itself (except for the ascii       **/
/**  decoding of the message descriptor).                                  **/
/**                                                                        **/
/****************************************************************************/
int
aio_decode_message(msgptr, prefix, outf)
    unsigned	msgptr;			/* virt address of message to decode */
    char	*prefix;		/* string to print before each line  */
    FILE	*outf;			/* where to print to		     */
{
    io_message_type	msg;		/* message that we will decode	     */
    msg_decode_blk	*item;		/* element of decode chain	     */
    int			status;		/* for aio_get_mgr_info call	     */
    struct mgr_info_type mgr;         	/* manager info structure            */

    /* grab the message */
    if (an_grab_virt_chunk(0, msgptr, &msg, sizeof(msg))) {
	fprintf(stderr,"aio_decode_message: couldn't get message\n");
	return(AIO_NO_SUCH_MESSAGE);
    }

    fprintf(outf, "%sfrom port = %d (0x%x)   ", prefix,
	msg.msg_header.from_port, msg.msg_header.from_port);
    status = aio_get_mgr_info(AIO_PORT_NUM_TYPE, msg.msg_header.from_port,&mgr);
    if (status == AIO_OK)
	fprintf(outf, "(%s at %s)\n", mgr.mgr_name, mgr.hw_address);
    else if (status == AIO_INCOMPLETE)
	fprintf(outf, "(%s)\n", mgr.mgr_name);
    else
	fprintf(outf, "(unknown manager!)\n");

    fprintf(outf, "%stransaction number 0x%x     message id = 0x%x\n",
	    prefix, msg.msg_header.transaction_num, msg.msg_header.message_id);
    fprintf(outf, "%smessage descriptor = %d (0x%x) -- ",
	  prefix, msg.msg_header.msg_descriptor, msg.msg_header.msg_descriptor);

    /* walk through the decode chain until we find an entry that matches */
    /*   the message descriptor of the message to be decoded	         */
    for (item = msg_decode_ptr; item != 0; item = item->link) {
	if (msg.msg_header.msg_descriptor >= item->low_msg_descriptor &&
	    msg.msg_header.msg_descriptor <= item->high_msg_descriptor) {

	    /* call the manager-supplied routine to do the rest */
	    (*item->decode_rtn)(msg, prefix, outf);
	    return(AIO_OK);
	}
    }

    /* no match found -- print a warning */
    fprintf(outf, "Unknown Message!\n");
    return(AIO_NO_DECODING);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine returns the ascii manager name given a port number.      **/
/**                                                                        **/
/****************************************************************************/
int
aio_get_mgr_name(port_num, mgr_name)
    port_num_type	port_num;	/* input port number of mgr to return */
    char		*mgr_name;	/* returned manager name	      */
{
    aio_tree_entry	*node;		/* io_tree node for this port	      */

    if ((node = port_to_io_tree(port_num)) != 0) {
	(void)strcpy(mgr_name, node->manager->name);
	return(AIO_OK);
    }
    else if (port_num == IO_CONFIGURATOR_PORT+1) {
	(void)strcpy(mgr_name, "configurator");
	return(AIO_OK);
    }
    else if (port_num == IO_DIAGNOSTICS_PORT+1) {
	(void)strcpy(mgr_name, "diagnostics");
	return(AIO_OK);
    }
    else
	return(AIO_NO_SUCH_PORT);
}

#ifdef	notdef	/* should be #ifndef NO_OLD_IOTREE */
aio_bcopy(src, dest, count)
    char	*src;
    char	*dest;
    int		count;
{
    int		i;

    /* copy bytes from src to dest */
    for (i = 0; i < count; i++) *dest++ = *src++;
}

/****************************************************************************/
/**                                                                        **/
/**  This routine returns the first iotree_index associated with a port.   **/
/**                                                                        **/
/****************************************************************************/

aio_port_to_index(port_num, index)
    port_num_type	port_num;	/* input port number		*/
    int    		*index;		/* returned iotree index	*/
{
    imc_port_ptr	port_ptr;

    if ((port_ptr = port_to_imc_ptr(port_num)) == 0)
	return(AIO_NO_SUCH_PORT);

    /* grab the iotree index out of imcport and range check */
    *index = node_to_index(port_ptr->io_tree_ptr);
    if (*index == NONE)
	return(AIO_NO_SUCH_PORT);
    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine returns the iotree entry with the input index.           **/
/**                                                                        **/
/****************************************************************************/
int
aio_get_iotree_entry(index, iotree_entry)
    int			index;		/* iotree index			*/
    struct iotree_type	*iotree_entry;  /* returned iotree entry	*/

{
    /* do some parameter checking on index */
    if (index < 0 || index >= ioheader.num_iotree_recs-1)
	return(AIO_NO_SUCH_ENTRY);

    /* copy from analyze global to return parameter */
    aio_bcopy(iotree[index], iotree_entry, sizeof(*iotree_entry));
    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine returns the manager table entry with the input index.    **/
/**                                                                        **/
/****************************************************************************/
int
aio_get_mgr_table_entry(index, mgr_table_entry)
    int				index;			/* mgr_table index   */
    struct mgr_table_type	*mgr_table_entry;	/* returned entry    */
{
    /* do some parameter checking on index */
    if (index < 0 || index >= ioheader.num_mgr_table_recs-1)
	return(AIO_NO_SUCH_ENTRY);

    /* copy from analyze global to return parameter */
    aio_bcopy(mgr_table[index], mgr_table_entry, sizeof(*mgr_table_entry));
    return(AIO_OK);
}
#endif	/*notdef*/

/****************************************************************************/
/**                                                                        **/
/**  This routine returns the address of a message which is queued on      **/
/**  a given port and a given subq. 					   **/
/**                                                                        **/
/****************************************************************************/
int
aio_queued_message(port_num, subq, msg_num, msgptr)
    port_num_type	port_num;	/* input port number to check	*/
    io_subq_type	subq;		/* input subq to check		*/
    int			msg_num;	/* which msg to return		*/
    int			*msgptr;	/* returned address of message  */
{
    imc_port_ptr	port_ptr;
    msg_frame_ptr 	vmsg_frame;	/* frame pointer		*/
    msg_frame_ptr 	vframe_end;	/* end of message list		*/
    int			counter = 0;	/* number of frames read	*/

    /* parameter range checks */
    if ((port_ptr = port_to_imc_ptr(port_num)) == 0)
	return(AIO_NO_SUCH_PORT);
    if (subq > 31)
	return(AIO_INVALID_SUBQ);

    /* grab virtual head of linked message frames on this subq */
    vmsg_frame = (msg_frame_ptr)port_ptr->subqueue[subq].f;

    /* grab virtual tail of the same;  it's the address of the queue	*/
    /* head -- funny how those circularly linked lists work		*/
    /* compute it by adding the offset of the queue head to the virtual	*/
    /* port pointer							*/
    vframe_end = (msg_frame_ptr)((int)vport_index[port_num]
	+ ((int)&port_ptr->subqueue[subq].f - (int)port_ptr));

    /* check for a valid frame */
    if (vmsg_frame != vframe_end &&
       (vmsg_frame < vfirst_frame || vmsg_frame > vlast_frame))
	return(AIO_INVALID_SUBQ);

    /* read frames while there are still some to read on this subq */
    while (vmsg_frame != vframe_end) {
	msg_frame_ptr lmsg_frame = &first_frame[vmsg_frame - vfirst_frame];

	/* got the right message -- return it */
	if (counter++ == msg_num) {
	    *msgptr = (int)lmsg_frame->data;
	    return(AIO_OK);
	}

	/* try to get the next one */
	vmsg_frame = (msg_frame_ptr)lmsg_frame->link.f;

	/* no messages left on this subq -- return */
	if (vmsg_frame != vframe_end &&
	   (vmsg_frame < vfirst_frame || vmsg_frame > vlast_frame))
	    return(AIO_NO_SUCH_MESSAGE);
    }
    return(AIO_NO_SUCH_MESSAGE);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine is called when an analysis routine has noted that there  **/
/**  is a revision problem. This routine prints a warning message          **/
/**  explaining the nature of the problem.                                 **/
/**                                                                        **/
/****************************************************************************/
int
aio_rev_mismatch(mgr_name, what_kind, code_rev, analyze_rev, outf)
    char 	*mgr_name;		/* which manager has a problem	*/
    int		what_kind;		/* what type of problem is it	*/
    int		code_rev;		/* rev number of the manager    */
    int		analyze_rev;		/* rev number of analysis rtn	*/
    FILE	*outf;			/* where to print		*/
{
    /* no revision number could be obtained from core */
    if (what_kind == AIO_NO_REV_INFO)
	fprintf(outf,"WARNING -- No revision information on manager %s\n",
		mgr_name);
    /* different rev numbers */
    else if (what_kind == AIO_INCOMPAT_REVS) {
	if (code_rev > analyze_rev) {		/* code newer than analyze */
	    fprintf(outf,"WARNING -- Analyze older than code for manager %s\n",
		    mgr_name);
	    fprintf(outf,"      code rev = %d and analyze rev = %d\n",
		    code_rev,analyze_rev);
	}
	else if (analyze_rev > code_rev) {	/* code older than analyze */
	    fprintf(outf,"WARNING -- Code older than analyze for manager %s\n",
		    mgr_name);
	    fprintf(outf,"      code rev = %d and analyze rev = %d\n",
		    code_rev,analyze_rev);
	}
	else					/* same age -- bug! */
	    return(AIO_REVS_THE_SAME);
    }
    else					/* bad parm! */
	return(AIO_NO_SUCH_OPTION);
    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine determines whether a given address is in emulated fe     **/
/**  mode. If yes, it returns a 1 in in_em_fe. Otherwise, it returns       **/
/**  a 0 in in_em_fe.                                                      **/
/**                                                                        **/
/****************************************************************************/
int
aio_em_fe(addr, in_em_fe)
    unsigned	addr;			/* i/o address to check for fe  */
    int		*in_em_fe;		/* 1=fe mode ; 0=not fe mode    */
{
    int		index;			/* index of pdir entry		*/
    unsigned	page_addr;      	/* page addr corresponds to     */
    struct pde	*entry;			/* pdir entry			*/

    /* parameter checking */
    if (addr < (unsigned)0xf0000000) return(AIO_INVALID_ADDRESS);

    /* turn the address into a pdir index -- check for range violation */
    page_addr = addr & 0xfffff800;
    index = -(((0xffffffff - addr) / 0x800) + 1);
    if (index < -niopdir)
        return(AIO_INVALID_ADDRESS);

    /* grab the pdir entry and make sure it is for the right page */
    entry = pgtopde(index);
    if ((entry->pde_page << 11) != page_addr)
        return(AIO_INVALID_ADDRESS);

    /* now check this page for emulated fe-mode */
    *in_em_fe = entry->pde_rtrap;

    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine finds the closest external to this address and prints    **/
/**  out the address in symbolic form.                                     **/
/**                                                                        **/
/****************************************************************************/

an_display_sym(addr, outf, call_type)
    unsigned	addr;		/* address to display		     */
    FILE	*outf;		/* where to print		     */
    int		call_type;	/* what to print -- offset?	     */
{
    int		diff;		/* distance from nearest symbol	     */

    /* grab the symbol name and the distance from it */
    diff = findsym((int)addr);

    /* if difference is real large, print absolutely */
    if (diff > 0xfffff)
	fprintf(outf, "0x%08x", addr);

    /* else if both symbol and offset wanted, print both */
    else if (call_type == AIO_SYMBOL_AND_OFFSET && diff != 0)
	fprintf(outf, "%s+0x%05x", cursym, diff);

    /* otherwise, print symbolname only */
    else
	fprintf(outf, "%s", cursym);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine produces a dump of the specified virtual address         **/
/**  range. Each line is 16 hex bytes followed by the equivalent ascii.    **/
/**                                                                        **/
/****************************************************************************/
int
an_dump_hex_ascii(space, addr, length, outf)
    space_t		space;		/* space of virtual address	     */
    unsigned		addr; 		/* offset of virtual address         */
    int			length;		/* number of bytes to display	     */
    FILE		*outf;		/* where to print		     */
{
    int 	phyaddr, i, wbuf[4], offset;

    /* parameter checking */
    if (length > 40000) return(AIO_TOO_BIG);

#ifndef IO_DDB
    /* allow signals to get through */
    allow_sigint = 1;
#endif IO_DDB

    /* print the buffer */
    for (offset = i = 0; i < length; i++) {
	if ((i % 4) == 0) {
	    phyaddr = ltor(space, (caddr_t)addr);
	    addr += 4;
	    if (phyaddr == 0) {
		fprintf(outf," 0x%x address not mapped\n",addr - 1);
		break;
	    }
	    (void)an_grab_real_chunk((unsigned)phyaddr, (char *)&wbuf[(i%16)/4],
			4);
	}

	if ((i+1) % 16 == 0) {
	    an_hexdump((unsigned char *)wbuf, 16, offset, outf);
	    offset += 16;
	}
	else if (i+1 == length)
	    an_hexdump((unsigned char *)wbuf, i-offset+1, offset, outf);
    }

#ifndef IO_DDB
    /* turn signals off again */
    allow_sigint = 0;
#endif IO_DDB

    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine grabs the address of a given external.                   **/
/**                                                                        **/
/****************************************************************************/
int
an_grab_extern(symbol_name, address)
    char	*symbol_name;	/* ascii name of external to grab    */
    unsigned    *address;	/* returned real address of external */
{
    extern unsigned lookup();

    return(((*address = lookup(symbol_name)) == 0) ? -1 : AIO_OK);
}


/****************************************************************************/
/**                                                                        **/
/**  This routine does a virtual to real address conversion.               **/
/**                                                                        **/
/****************************************************************************/

an_vtor(space, offset, phys_address)
    int    		space;		/* space of address to convert	   */
    unsigned    	offset;		/* offset of address to convert	   */
    int    		*phys_address;	/* returned real address	   */

{
    return(((*phys_address = ltor((space_t)space, (caddr_t)offset)) == 0) ?
	-1 : AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine grabs a chunk of space from the corefile given the real  **/
/**  address of it.                                                        **/
/**                                                                        **/
/****************************************************************************/
int
an_grab_real_chunk(address, localbuf, count)
    unsigned   	address;	/* real starting address	    */
    char	*localbuf;	/* buffer to read into		    */
    int		count;		/* number of bytes to read	    */
{
#ifdef IO_DDB
    bcopy((caddr_t)address,(caddr_t)localbuf,count);
#else
    extern off_t lseek();
    extern int fcore;

    /* seek to the correct location in the core */
    (void)lseek(fcore, (off_t)address, 0);

    /* try to read from the corefile -- check for errors */
    if (read(fcore, localbuf, (unsigned)count) != count)
	return(-1);
#endif /*IO_DDB*/
    
    return(AIO_OK);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine grabs a chunk of space from the corefile given the       **/
/**  virtual address of it.                                                **/
/**                                                                        **/
/****************************************************************************/
int
an_grab_virt_chunk(space, offset, localbuf, count)
    int    	space;		/* space of starting address	   */
    unsigned   	offset;		/* offset of starting address	   */
    char	*localbuf;	/* buffer to read into		   */
    int		count;		/* number of bytes to read	   */
{
#ifdef IO_DDB
    priv_lbcopy(space,(caddr_t)address,0,(caddr_t)localbuf,count);
#else
    /* grab a chunk of virtual space -- check for errors */
    if (getchunk(space, (int)offset, localbuf, count,"an_grab_virt_chunk"))
	return(-1);
#endif /*IO_DDB*/

    return(AIO_OK);
}

char *
decode_sversion(smodel, hmodel)
    unsigned smodel, hmodel;
{
    switch(smodel) {
	case 0x04:
	    switch(hmodel) {
	    	case 0x004: return("Indigo processor");
	    	case 0x008: return("Firefox processor");
	    	case 0x00A: return("TopGun processor");
	    	case 0x00B: return("Technical Shogun processor");
	    	case 0x00F: return("Commercial Shogun processor");
	    	case 0x00C: /* Fall into */
	    	case 0x080: return("Cheetah processor");
	    	case 0x081: return("Jaguar processor");
	    	case 0x082: return("Cougar processor");
	    	case 0x083: return("Panther processor");
	    	case 0x100: return("Burgundy processor");
	    	case 0x101: return("Silverfox processor (low)");
	    	case 0x102: return("Silverfox processor (high)");
	    	case 0x103: return("Lego processor");
	    }
	case 0x08: return("architected memory");
	case 0x09: return("processor-dependent memory");
	case 0x0C: 
	    switch(hmodel) {
		case 0x004: return("Cheetah BC, SMB port");
		case 0x006: return("Cheetah BC, MID_BUS port");
		case 0x005: return("Condor BC, MID_BUS port");
		case 0x100: return("Condor BC, HP-PB port");
		default:    return("unrecognized BC");
	    }
	case 0x0D: return("Arrakis RS232");
	case 0x0E: return("RS232");
	case 0x0F: return("Peacock graphics");
	case 0x10: return("CIO channel");
	case 0x14: return("HIL");
	case 0x15: return("Leonardo");
	case 0x16: return("HP-PB HRM");
	case 0x17: return("HP-PB HRC");
	case 0x18: return("parallel card");
	case 0x19: return("RDB");
	case 0x1c: return("Cheetah console");
	case 0x20: return("MID_BUS PSI");
	case 0x2F: return("HP-PB Transit PSI");
	case 0x30: return("MID_BUS Verification Master");
	case 0x34: return("MID_BUS Verification Slave");
	case 0x38: return("MID_BUS Verification EDU");
	case 0x39: return("HP-PB SCSI");
	case 0x3A: return("HP-PB Centronics");
	case 0x40: return("HP-PB HP-IB");
	case 0x44: return("HP-PB GPIO");
	case 0x48: return("Spectrograph LGB Framebuffer");
	case 0x49: return("Spectrograph LGB Control");
	case 0x4A: return("Nimbus LGB Framebuffer");
	case 0x4B: return("Nimbus LGB Control");
	case 0x4C: return("Martian RTI");
	case 0x50: return("Lanbrusca 802.3");
	case 0x51: return("HP-PB Transit 802.3");
	case 0x58: return("HP-PB Transit 802.4");
	case 0x80002: return("old RDB");
	default:   return("unknown sversion");
    }
    /*NOTREACHED*/
}

an_ioserv_native(entry, level, verbose, outf)
    aio_native_mod_entry	*entry;
    int				level, verbose;
    FILE			*outf;
{
    int			i;
    char		*type;

    if (level == 0)
	fprintf(outf, "\n================\nNATIVE MOD TABLE:\n================\n");
    for (; entry != 0; entry = entry->next) {
	native_mod_entry *mod_ptr = &entry->entry;

	for (i = 0; i < level; i++) printf("   ");
	printf("%-2d ", mod_ptr->fixed);
	switch (mod_ptr->iodc_type.type) {
	    case MOD_TYPE_NPROC:	type = "processor  "; break;
	    case MOD_TYPE_MEMORY:	type = "memory     "; break;
	    case MOD_TYPE_B_DMA_IO:	type = "B-dma      "; break;
	    case MOD_TYPE_B_DIRECT_IO:	type = "B-direct   "; break;
	    case MOD_TYPE_A_DMA_IO:	type = "A-dma      "; break;
	    case MOD_TYPE_A_DIRECT_IO:	type = "A-direct   "; break;
	    case MOD_TYPE_OTHER:	type = "other      "; break;
	    case MOD_TYPE_BUSCONV:	type = "bc         "; break;
	    case MOD_TYPE_CIO_CHANNEL:	type = "cio channel"; break;
	    case MOD_TYPE_CONSOLE:	type = "console    "; break;
	    case MOD_TYPE_BUS_ADAPTER:	type = "bus adapter"; break;
	    default:			type = "???        "; break;
	}
	printf("%s hpa: 0x%08x ", type, mod_ptr->hpa);
	if (verbose) {
	    printf("[%s]\n", decode_sversion(mod_ptr->iodc_sversion.model,
		mod_ptr->iodc_hversion.model));
	    printf("\thversion (model 0x%03x rev 0x%x) ",
		mod_ptr->iodc_hversion.model, mod_ptr->iodc_hversion.rev);
	    printf("iodc_spa (%s, shift 0x%x)\n",
		mod_ptr->iodc_spa.io ? "io" : "mem", mod_ptr->iodc_spa.shift);
	    printf("\tsversion (rev 0x%x model 0x%02x opt 0x%02x) ",
		mod_ptr->iodc_sversion.rev,   mod_ptr->iodc_sversion.model,
		mod_ptr->iodc_sversion.opt);
	    printf("type (%s%stype 0x%x)\n",
		mod_ptr->iodc_type.mr ? "mr, " : "",
		mod_ptr->iodc_type.wd ? "wd, " : "", mod_ptr->iodc_type.type);
	    printf("\trsvd 0x%x, rev 0x%x, dep 0x%x, check 0x%x, length 0x%x\n",
		mod_ptr->iodc_reserved,	mod_ptr->iodc_rev, mod_ptr->iodc_dep,
		mod_ptr->iodc_check,	mod_ptr->iodc_length);
	    switch (mod_ptr->iodc_type.type) {
	    	case MOD_TYPE_MEMORY:	
		    printf("\tspa: 0x%08x ", mod_ptr->type.mem.spa);
		    printf("spa_size: 0x%x ", mod_ptr->type.mem.spa_size);
		    printf("mem_size: 0x%x\n", mod_ptr->type.mem.mem_size);
		    printf("\tstatus: 0x%x ", mod_ptr->type.mem.io_status);
		    printf("err_info: 0x%x ", mod_ptr->type.mem.io_err_info);
		    printf("err_resp: 0x%x ", mod_ptr->type.mem.io_err_resp);
		    printf("err_req: 0x%x\n", mod_ptr->type.mem.io_err_req);
		    break;

	    	case MOD_TYPE_B_DMA_IO: case MOD_TYPE_B_DIRECT_IO:
	    	case MOD_TYPE_A_DMA_IO: case MOD_TYPE_A_DIRECT_IO:
	    	case MOD_TYPE_CIO_CHANNEL: case MOD_TYPE_OTHER:
		    printf("\tspa: 0x%08x ", mod_ptr->type.io.spa);
		    printf("spa_size: 0x%x ", mod_ptr->type.io.spa_size);
		    printf("eim: 0x%x ", mod_ptr->type.io.eim);
		    printf("poll_entry: 0x%x\n", mod_ptr->type.io.poll_entry);
		    break;

	    	case MOD_TYPE_BUSCONV:
		    printf("\tio_low: 0x%08x ", mod_ptr->type.bc.io_low);
		    printf("\tio_high: 0x%08x ", mod_ptr->type.bc.io_high);
		    printf("%s port\n", mod_ptr->type.bc.is_lower_port ?
			"lower" : "upper");
		    printf("\tstatus: 0x%x ", mod_ptr->type.bc.io_status);
		    printf("err_info: 0x%x ", mod_ptr->type.bc.io_err_info);
		    printf("err_resp: 0x%x ", mod_ptr->type.bc.io_err_resp);
		    printf("err_req: 0x%x\n", mod_ptr->type.bc.io_err_req);
		    break;

	    	default:
		    break;
	    }
	}
	else {
	    printf("hv: <%03x.%1x>, sv: <%1x.%02x.%2x> [%s]\n",
		mod_ptr->iodc_hversion.model, mod_ptr->iodc_hversion.rev,
		mod_ptr->iodc_sversion.rev,   mod_ptr->iodc_sversion.model,
		mod_ptr->iodc_sversion.opt,
	    	decode_sversion(mod_ptr->iodc_sversion.model,
				mod_ptr->iodc_hversion.model));
	}
	if (mod_ptr->iodc_type.type == MOD_TYPE_BUSCONV)
	    an_ioserv_native(entry->next_bus, level+1, verbose, outf);
    }
}

int
stoi(s, value, outf)
    char *s;
    int  *value;
    FILE *outf;
{
    int		 base  = 10;
    int		 minus = 0;
    char        *l;
    static char *digits = "0123456789abcdef";
    *value   = 0;

    if (*s == '-') {
	minus = 1;
	s++;
     }

    if (*s == '0') {
	if (s[1] == 'x' || s[1] == 'X') {
	    base = 16;
	    s += 2;
	}
	else if (s[1] == 'b' || s[1] == 'B') {
	    base = 2;
	    s += 2;
	}
	else {
	    base = 8;
	    s += 1;
	}
    }

    while (*s) {
	if (isupper(*s)) *s = tolower(*s);
	if ((l = strchr(digits, *s)) != 0 && l < &digits[base])
	    *value = *value*base + l-digits;
	else {
	    (void)fprintf(outf, "bad character `%c' in number\n", *s);
	    return(1);
	}
	s++;
    }
    if (minus) *value = -*value;
    return(0);
}

#  ifdef IO_DDB
fprintf(fd,fmt, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10)
    FILE *fd;
    char *fmt;
    unsigned x1, x2, x3, x4, x5, x6, x7, x8, x9, x10;
{
    prf(fmt, &x1, 0, 0, 0);
}
#  endif /*IO_DDB*/

#endif

/****************************************************************************/
/**                                                                        **/
/**  This routine walks through the i/o services free memory list to       **/
/**  determine if a given buffer is on a free list. If so, 1 is returned.  **/
/**  If not on any list, 0 is returned.	A message is printed if the buffer **/
/**  is found on any free list.						   **/
/**  If a list cannot be examined, a warning is printed to stderr.         **/
/**                                                                        **/
/****************************************************************************/

aio_mem_on_free_list(addr, num_bytes, prefix, outf)
    unsigned	addr;		/* address of buffer to set bits for */
    int		num_bytes;	/* size of buffer to set bits for    */
    char	*prefix;	/* buffer to print before each line  */
    FILE	*outf;		/* output file descriptor	     */
{
    unsigned	bit_mask;	/* bit we are currently checking     */
    int		word;		/* map word we are checking	     */
    int		first_bit;	/* first bit to check		     */
    int		i;		/* loop index			     */
    int		found_on_list;	/* flag set when we find on list     */

    /* verify parameters */
    if (num_bytes < 1		||
        num_bytes > 65536	||
        addr      < io_mem_buf	||
        (addr + num_bytes) > io_mem_top) {
	fprintf(outf,"%sinvalid buffer specification 0x%x to 0x%x\n",
		prefix, addr, addr+num_bytes-1);
	fprintf(outf,"%svalid range is 0x%x to 0x%x\n",
		prefix, io_mem_buf, io_mem_top-1);
	return(1);
    }

    /* calculate starting position */
    word = (addr - io_mem_buf) / 32;
    first_bit = (addr - io_mem_buf) % 32;
    bit_mask = 0x80000000 >> first_bit;
    found_on_list = -1;

    /* check bits */
    for (i = 0; i < num_bytes; i++) {
	/* byte in free list, so set flag and leave */
	if ((io_mem_map[word] & bit_mask) == 1) {
	    found_on_list = 1;
	    break;
	}

	/* move on */
	if (bit_mask == 1) {
	    word++;
	    bit_mask = 0x80000000;
	}
	else
	    bit_mask >>= 1;
    }

    /* handle any found buffers */
    if (found_on_list == 1) aio_mem_problem(addr,num_bytes,prefix,outf);

    return(found_on_list);
}

/****************************************************************************/
/**                                                                        **/
/**  This routine is internal only. It is called by aio_mem_on_free_list   **/
/**  if the buffer is on any free list.                                    **/
/**                                                                        **/
/****************************************************************************/

aio_mem_problem(bufst, num_bytes, prefix, outf)
    unsigned		bufst;		/* starting buffer address	*/
    int			num_bytes;	/* size of buffer		*/
    char		*prefix;	/* string printed before each line */
    FILE		*outf;	/* output file descriptor	*/
{
    int			i,j, size;
    unsigned		bufend, addr, next_addr, addr_end;

    /* some initialization */
    bufend = bufst + num_bytes - 1;

    /* run through equivalently mapped free lists */
    for (i=1 ; i<12 ; i++) {

	/* grab appropriate list head */
        if (an_grab_extern("io_eqmem_heads",&next_addr)) {
	    fprintf(outf,"couldn't get io_eqmem_heads!!\n");
	    break;
	}
        if (an_grab_virt_chunk(0,next_addr+(4*i), (char*)&addr,4)) {
	    fprintf(outf,"couldn't get io_eqmem_heads!!\n");
	    break;
	}

	/* if list is empty, move on */
	if (addr == 0)
	    continue;

	/* walk through list until EOC is found or we detect a loop */
	size = 1 << i;
	for (j=0 ; j<1000 ; j++) {

	    addr_end = addr + size - 1;
	    if  ( (bufst <= addr_end) && (bufend >= addr) ) 
		fprintf(outf,"%sbuffer 0x%08x found on %d byte equiv-mapped list\n",
			 prefix,addr,size);

	    /* try to grab the next entry */
	    if (an_grab_virt_chunk(0,addr, (char *)&next_addr,4)) {
		fprintf(outf,"read failed on entry 0.0x%08x!!\n",addr);
		break;
	    }

	    /* check for eoc -- stop if found, otherwise try again */
	    if (next_addr == 0)
		break;
	    else
		addr = next_addr;

	}

	/* print a warning message if loop detected */
	if (j == 1000)
	    fprintf(outf,"%s  loop on %d byte equiv-mapped list!!\n",
		    prefix,size);
	
    }

    /* run through non-equivalently mapped free lists */
    for (i=1 ; i<17 ; i++) {

	/* grab appropriate list head */
        if (an_grab_extern("io_mem_heads",&next_addr)) {
	    fprintf(outf,"couldn't get io_mem_heads!!\n");
	    break;
	}
        if (an_grab_virt_chunk(0,next_addr+(4*i), (char *)&addr,4)) {
	    fprintf(outf,"couldn't get io_mem_heads!!\n");
	    break;
	}

	/* if list is empty, move on */
	if (addr == 0)
	    continue;

	/* walk through list until EOC is found or we detect a loop */
	size = 1 << i;
	for (j=0 ; j<1000 ; j++) {

	    addr_end = addr + size - 1;
	    if  ( (bufst <= addr_end) && (bufend >= addr) ) 
		fprintf(outf,"%sbuffer 0x%08x found on %d byte non-equiv-mapped list\n",
			 prefix,addr,size);

	    /* try to grab the next entry */
	    if (an_grab_virt_chunk(0,addr, (char *)&next_addr,4)) {
		fprintf(outf,"read failed on entry 0.0x%08x!!\n",addr);
		break;
	    }

	    /* check for eoc -- stop if found, otherwise try again */
	    if (next_addr == 0)
		break;
	    else
		addr = next_addr;
	}

	/* print a warning message if loop detected */
	if (j == 1000)
	    fprintf(outf,"%s  loop on %d byte non-equiv-mapped list!!\n",
		    prefix,size);
    }
}

/****************************************************************************/
/**                                                                        **/
/**  Auxilliary routine for an_dump_hex_ascii.                             **/
/**                                                                        **/
/****************************************************************************/
static
an_hexdump(buf, size, offset, outf)
    unsigned char 	*buf;
    int  		size, offset;
    FILE		*outf;
{
#define LINESIZE	16
    int 	i, pos;
    char 	charbuf[LINESIZE+1];

    charbuf[LINESIZE] = '\0';
    for (i = 0; i < size; i++) {
	if ((pos = i % LINESIZE) == 0) fprintf(outf, "  %4d:  ", offset+i);

	fprintf(outf, "%02x ", buf[i]);
	charbuf[pos] = (isprint(buf[i]) && !isspace(buf[i])) ? buf[i] : '.';
	
	if (pos == LINESIZE-1) fprintf(outf, "    %s\n", charbuf);
    }

    if (pos != LINESIZE-1) {
	for (pos = i % LINESIZE; pos < LINESIZE; pos++) {
	    fprintf(outf, "-- ");
	    charbuf[pos] = '.';
	}
	fprintf(outf, "    %s\n", charbuf);
    }
}

/****************************************************************************/
/**                                                                        **/
/**  This routine builds the ascii hw pathname for a given iotree index.   **/
/**  It calls itself recursively to do this.                               **/
/**                                                                        **/
/****************************************************************************/
static
aio_get_hw_add(node, str)
    aio_tree_entry *node;	/* node to print  */
    char	   *str;	/* string to add onto		  */
{
    /* if at end of chain (top-level) start string with this node number */
    if (node->parent == io_tree_ptr) {
	(void)sprintf(str, "%d", node->hdw_address);
    }

    /* otherwise, call rtn recursively with this node's parent, eventually */
    /*    adding "." and address onto string				   */
    else {
	char temp[10];

	(void)aio_get_hw_add(node->parent, str);
	(void)sprintf(temp, "%c%d", (node->state.native_hdw) ? '/' : '.',
		node->hdw_address);
	(void)strcat(str, temp);
    }
}
