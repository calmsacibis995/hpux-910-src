/*
 * @(#)dux_rdX.h: $Revision: 1.5.83.3 $ $Date: 93/09/17 16:42:10 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */

#ifdef _KERNEL_BUILD
#include "../dux/dux_dev.h"
#else  /* ! _KERNEL_BUILD */
#include <dux/dux_dev.h>
#endif /* _KERNEL_BUILD */

/* Network message structure for remote driver requests */

struct rdu_message		/*DUX MESSAGE STRUCTURE*/
  { dev_t  rdu_dev;             /* The Device number                      */
    union
      { struct                  /* Parameters for open request            */
	  { u_int  flag;        /* Mode flag for device open              */
	    u_int  mode;        /* Mode of device (IFBLK or IFCHR)        */
	    site_t site;        /* Site where request should go           */
	  } open;

	struct                  /* Parameters for strategy request        */
	  { long    flags;
	    long    bcount;
	    daddr_t blkno;
	    daddr_t offset;
	    int	    nbpg;
	  } strat;

	struct                  /* Parameters for ioctl request           */
	  { int com;
	    int flag;
	  } ioc;

	struct
	  { int     uio_offset;
	    int     uio_resid;
	    int     uio_fpflags;
	  } uio;

	int     which;          /* Select parameter                       */

	struct			/* Close parameters			  */
	  { int flag;
	    site_t orig_site;	/* Site that initiated device close	  */
	  } close;
      } rdu_params;

    char rdu_data[sizeof(int)]; /* Actually longer */
  };

typedef struct rdu_message rdu_t;

#define RDU_SIZE(d)     ((d) + sizeof(rdu_t) - sizeof(int))
#define DMRD_IOSIZE     (DM_MAX_MESSAGE - sizeof(rdu_t) - \
				sizeof(struct dm_header))

#define rdu_open        rdu_params.open
#define rdu_flag        rdu_params.close.flag
#define rdu_origsite	rdu_params.close.orig_site
#define rdu_which       rdu_params.which
#define rdu_strat       rdu_params.strat
#define rdu_ioc         rdu_params.ioc
#define rdu_uio         rdu_params.uio
#define rdu_iodata      rdu_data

/*  Special message for a large  I/O,  used  to  overlay  the  unsp_message
 *  structure.
 */

struct rdX_bigio	/*DUX MESSAGE STRUCTURE*/
  { int   rdX_um_id;
    u_int rdX_size;     /*  Not sent after the first message on read    */
    dev_t rdX_dev;      /*  Not sent after the first message            */
    u_int rdX_offset;   /*  Not sent after the first message            */
    u_int rdX_fpflags;  /*  Not sent after the first message            */
    u_int rdX_sousig;   /*  Not sent after the first message            */
  };

/* Network message structure for remote driver responses                */

struct rds_message		/*DUX MESSAGE STRUCTURE*/
  { union
      { u_int index;            /*  Index of opened device              */
	u_int resid;            /*  I/O residual                        */
      } rds_value;

    char rds_data[sizeof(int)]; /*  Actually longer                     */
  };

typedef struct rds_message rds_t;

#define RDS_SIZE(d)     ((d) + sizeof(rds_t) - sizeof(int))

#define rds_index       rds_value.index
#define rds_resid       rds_value.resid
#define rds_iodata      rds_data

extern int KDBLVL;
