/*
 * @(#)types.h: $Revision: 1.17.83.3 $ $Date: 93/09/17 16:32:21 $
 * $Locker:  $
 */


/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
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
*/


struct ihead{
	struct inode *ih_chain[2];
};


/* Page usage log table */
struct	paginfo {
	char	z_type;
	char	z_count;
	short	z_pid;
	vfd_t   z_vfd;

	unsigned int    z_virtual;
	unsigned int	z_space;
	unsigned int	z_pdirtype;
	unsigned int	z_hashndx;
	unsigned int	z_status;
	unsigned int    z_protid;
	};

#define BUFBLKSZ 0x100
#define BUFBLKMASK 0xff

#define RND     (MAXBSIZE/DEV_BSIZE)
#define BUFBLKHASH(dvp, dblkno)    \
        ((struct buf *)&bufblk[((u_int)(dvp)+\
		((int)(dblkno))+\
		((u_int)dvp>>5))&(BUFBLKMASK)])

/* Buffer usage log table */
struct	bufblk {
	int   freetype;
	int   hashtype;
	struct buf *vbp;
	struct bufblk *next;
	} ;

/* Indexes to swaptab and swapmap tables */
struct  dbdndx {
	u_int	type:4;
	u_int   stab:14;
	u_int   smap:14;
	};

/* Swap buffer headr log table */
struct	swbufblk {
	int   type;
	struct buf *bp;
	};


/* inode usage log table */
struct	inodeblk {
	int   list;
	struct inode *ip;
	};



/* used for mapping file */
struct sominfo {
	int	b1;
	int	e1;
	int	f1;
	int	b2;
	int	e2;
	int	f2;
};

struct prochd{
	struct proc *ph_link;
	struct proc *ph_rlink;
};


/* disc block usage table */
struct dblks {
	short	d_type;
	short	d_count;
};

struct pregblk{
	struct proc 	*p_proc;
	short		p_type;
	short		p_count;
};

struct regblk{
	struct pregion	*r_preg;
	short		r_type;
	short		r_count;
};

struct	sysblks {
	long		s_first;
	int     	s_size;
	char	s_type;
	int	s_index;
};

/* quad4map entry */
