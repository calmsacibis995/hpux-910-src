/*
 * @(#)gpu_data.h: $Revision: 1.6.83.3 $ $Date: 93/09/17 16:53:33 $
 * $Locker:  $
 */

#ifndef __GPU_DATA_H_INCLUDED
#    define __GPU_DATA_H_INCLUDED

#ifdef _KERNEL
#    include "../h/ioctl.h"
#    include "../h/types.h"
#    include "../h/buf.h"
#    include "../h/framebuf.h"
#    include "../h/region.h"
#    include "../machine/param.h"
#    include "../sio/llio.h"
#    include "../graf.800/graph02.h"
#    include "../graf.800/stirom.h"
#else
#    include <sys/ioctl.h>
#    include <sys/types.h>
#    include <sys/buf.h>
#    include <sys/framebuf.h>
#    include <machine/param.h>
#endif

/* Diagnostic request values for use by the kernel */
#define GRAPH_DIAG_OPEN  -1
#define GRAPH_DIAG_CLOSE -2
 
/*
 * Setting the diagnostic bit in the minor number for the framebuffer
 * device denotes that one is requesting a diagnostic open().
 */
#ifdef _WSIO
#   define FRAMEBUF_DIAG_REQ(xxx)	(xxx & 0x00000800)
#else
#   define FRAMEBUF_DIAG_REQ(xxx)	(xxx & 0x00800000)
#endif

/*
 * Maximum number of different minor numbers for the same lu that
 * are allowed to be open simultaneously.  Starbase ocassionally
 * likes to use bits in the minor number as hints to its own drivers.
 */
#define MAX_FRAMEDEVS 4

/*
 * Define maximum number of graphics devices supported.
 * Note that this should be increased when multi-function SGC cards
 * are supported.
 */
#ifdef _WSIO
#define MAX_DISPLAY0 2
#endif

/*
 * Maximum DMA transfer size in bytes (crt_dma_ctrl.length).
 * This number is the maximum amount that it makes sense to
 * transfer to a HP98730.  This should probably be changed to
 * some function of the particular display's dimensions.
 */
#ifdef INSTALL
#   define MAX_DMA_SIZE		0
#   define MAX_QUAD_HEADERS	1
#else
#   define MAX_DMA_SIZE		(1024*2048)
#   define MAX_QUAD_HEADERS	(MAX_DMA_PAGES/sizeof(graph_quad_type))
#endif
#define MAX_DMA_PAGES	((MAX_DMA_SIZE/NBPG)+1)

#define PROTID_BOGUS		-1

/* Driver/module states (gr_data->state values) */
typedef enum {
    GPU_UNINITIALIZED, GPU_NORMAL, GPU_POWER_DOWN, GPU_SHADOW, GPU_DEAD
} gpu_state;

typedef enum {
    GR_DISPLAY_BOGUS, GR_DISPLAY_A, GR_DISPLAY_B
} graph_display;

struct btlb_info {
    caddr_t start, end;
    int broken, index;
};

/*
 * Per-GPU data structure.  In the general case, it is possible
 * to have more than one display connected to an interface card
 * so this information does not belong in a card's PDA.
 */
#ifdef _KERNEL
#   define REG_T	reg_t
#   define QUAD_T	graph_quad_type
#   define IOBLKP_T	struct ioblk_pages_t *
#else
#   define REG_T	int
#   define QUAD_T	int
#   define IOBLKP_T	int *
#endif

struct gr_data {
#   ifdef _WSIO
	int		disp_sema;
#   else
	int    		*pdap;			   /* port data area pointer */
#   endif
    int			*frame_buffer, *ctl_space;
    int			mgr_index;
    REG_T		*reg_p;				   /* region pointer */
    int			size_frame_buffer, size_ctl_space;
    gpu_state		state;		       /* with respect to power, etc */
    int			fe_bank;		       /* 98550 bank address */
    int			supports_itehil;
    int			g_locking_blocksig_set;
    int			g_locking_caught_sigs;
    int			static_colormap;
    struct proc		*sel_proc;
    int			sel_flag, sel_qnum;
    int			(*gpu_reset)();		    /* card GCRESET function */
    int			(*graph_service)();/* mandatory top->bottom intrface */
    int			(*dma_restrictions)();
    int			(*dma_func)();		   /* DMA execution function */
    int			(*diag_func)();
    int			curr_max_slot;
    struct proc		*locking_proc;
    struct crt_fastlock_t	*lockp;
    int			num_ioblks;
    IOBLKP_T		ioblks;
    int			protid;
#ifdef _WSIO
    int			num_btlb;
    struct btlb_info	btlb_list[4];		  /* FIXME - 4 is hard-coded */
    int			*sc;			      /* select code pointer */
    char		*card_base;
    struct sti_data	*sti_data;
    int			g_state;
    int			*diag_proc;
    int			inqconf_size;	/* size of STI INQ_CONF() save-state */
    int                 sti_grp_saverest_start;/* used for framebuf timeouts */
#else
    int			*interlocked_fb, *interlocked_cs; /* != 0 for graph2 */
    int			probed;
    graph_display	which_display;
    char		*test_vector;
    struct crt_dma_ctrl	dma_params;		      /* copy of user params */
    QUAD_T		*quad_head[MAX_QUAD_HEADERS];
#endif /* _WSIO */
    dev_t		devs[MAX_FRAMEDEVS];
    crt_frame_buffer_t	desc;
    short		slot_pid[MAX_SLOT_ENTRIES];
};

#ifndef _WSIO
#   define FRAMEBUF_EVENT_SUBQ	3
#   define FRAMEBUF_REPLY_SUBQ	4
#   define NUM_FRAMEBUF_SUBQS	5

    /* Diagnostic messages issued from the ITE */
    #define FRAMEBUF_FROM_FRAMEBUF 0

    typedef enum {
	FRAME_PORT_UNINITIALIZED, FRAME_PORT_BOUND, FRAME_PORT_NORMAL
    } framebuf_state;

    typedef struct {
	int		 *gpu;	     /* cast to "struct iterminal" if needed */
	port_num_type	  my_port, parent_port;
	int		  mgr_index;
	framebuf_state	  state;
    } framebuf_pda_type;
#endif /* not _WSIO */

#endif /* not __GPU_DATA_H_INCLUDED  */
