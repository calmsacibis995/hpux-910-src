/*
 * @@(#)var.h: $Revision: 1.6.83.6 $ $Date: 93/11/03 15:15:33 $
 * $Locker:  $
 */


/*
 * System Configuration Information
 */

struct var {
	int	v_buf;		/* Nbr of I/O buffers.			*/
	int	v_call;		/* Nbr of callout (timeout) entries.	*/
	int	v_inode;	/* Size of incore inode table.		*/
	char *	ve_inode;	/* Ptr to end of incore inodes.		*/
	int	v_file;		/* Size of file table.			*/
	char *	ve_file;	/* Ptr to end of file table.		*/
	int	v_mount;	/* Size of mount table.			*/
	char *	ve_mount;	/* Ptr to end of mount table.		*/
	int	v_proc;		/* Size of proc table.			*/
	char *	ve_proc;	/* Ptr to next available proc entry	*/
				/* following the last one used.		*/
#ifdef LATER
	int	v_clist;	/* Nbr of clists allocated.		*/
	int	v_maxup;	/* Max number of processes per user.	*/
	int	v_hbuf;		/* Nbr of hash buffers to allocate.	*/
	int	v_hmask;	/* Hash mask for buffers.		*/
	int	v_pbuf;		/* Nbr of physical I/O buffers.		*/
	int	v_sptmap;	/* Size of system virtual space		*/
				/* allocation map.			*/
#endif	
	int	v_vhndfrac;	/* Fraction of maxmem to set as limit	*/
				/* for running vhand.  See getpages.c	*/
				/* and clock.c				*/
#ifdef	LATER
	int	v_maxpmem;	/* The maximum physical memory to use.	*/
				/* If v_maxpmem == 0, then use all	*/
				/* available physical memory.		*/
				/* Otherwise, value is amount of mem to	*/
				/* use specified in pages.		*/
#endif
};

extern struct var v;

