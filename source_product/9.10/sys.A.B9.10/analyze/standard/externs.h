/*
 * @(#)externs.h: $Revision: 1.54.83.3 $ $Date: 93/09/17 16:31:01 $
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


extern int	Qflg;
extern int	Jflg;
extern int	Xflg;
extern int	Pflg;
extern int	Eflg;
extern int	Dflg;
extern int	dflg;
extern int	Iflg;
extern int	iflg;
extern int	Bflg;
extern int	bflg;
extern int	Tflg;
extern int	vflg;
extern int	Vflg;
extern int	Cflg;
extern int	Fflg;
extern int	sflg;
extern int	Mflg;
extern int	Uflg;
extern int	Aflg;
extern int	tolerate_error;
extern int      proc0;
#ifdef iostuff
extern int	Oflg;
extern int	oflg;
extern int	Zflg;
extern int	zflg;
#endif
extern int	Sflg;
extern int	Rflg;
extern int	Hflg;
extern int	Nflg;
extern int	Lflg;
#define DUXFLAG Lflg
#define DUXCHAR 'L'
#define DNLC_FLAG Vflg
#define DNLC_CHAR 'V'
extern int      qflg;
extern int	lflg;
extern int	activeflg;
extern int	maxdepth;
extern int	endbss;
extern int 	semaflg;
extern int      suppress_default;

extern struct mpinfo *mp, *amp, *vmp;
extern struct mp_iva *mpiva, *ampiva, *vmpiva;
/* extern struct lock spl_lock, spl_word; changed - get header def */

extern struct	proc sproc, *proc, *aproc, *vproc, *currentp;
extern int	currentsp, currentsr5, currentpcsq, currentpcoq;
extern int	nproc;
extern int 	oldpri;

extern struct	text stext, *text, *atext, *vtext;
extern int	ntext;

extern struct	file *file, *afile, *vfile;
extern int	nfile;

extern struct	prochd qs[], *aqs;


/* pfdat structure */
extern struct pfdat phead, *aphead, *vphead;
extern struct pfdat pbad, *apbad, *vpbad;
extern struct pfdat *pfdat, *apfdat, *vpfdat;
extern struct pfdat **phash, *aphash, *vphash;
extern int	phashmask;

/* vas structure */
extern struct vas *vastable, *avastable, *vvastable;

/* pregion structure */
extern struct pregion *prp, *aprp, *vprp, *apregion, *vpregion, *pregion;
extern struct pregion *prpfree;

/* region structure */
extern struct region *region, *aregion, *vregion;
extern struct region regactive, *aregactive, *vregactive;
extern struct region regfree, *aregfree, *vregfree;

/* swap stuff */
extern swpt_t swaptab[], *aswaptab, *vswaptab;
extern swdev_t swdevt[],*aswdevt, *vswdevt;
extern struct vnode        *swapdev_vp;
extern use_t *usetable[];
extern struct dblks *dusagetable[];
extern int nextswap;
extern int swapwant;

/* system allocation map */
extern struct map *sysmap, *asysmap, *vsysmap;

/* quad4map */

/* tune parameters */
extern struct tune tune, *atune, *vtune;

/* sys info map */
extern struct sysinfo sysinfo, *asysinfo, *vsysinfo;

/* var structure with sizes of tables, etc... */
extern struct var v, *av, *vv;

/* temp for reading dbd */
extern dbd_t dbd_temp;

extern int	nswapmap;
extern dev_t	swapdev;

#ifdef hp9000s800
extern short	*uidhash, *auidhash, *vuidhash;
#endif

extern int	firstfree;
extern int	maxfree;
extern int	freemem;
extern int	physmem;

extern int	lotsfree, minfree, desfree;

extern int 	minpagecpu, maxpagecpu;
extern int 	vhandrunrate, handlaps;
extern int 	maxpendpageouts, pageoutrate;
extern int 	gpgslim, memzerorate;
extern int 	prepage, maxpageout;
extern int	nicepaging, nicepageshift, nicepagelog;

extern	char	*aetext, *aend;

extern int 	ecamx;
extern struct	vfd *camap, *acamap, *aecamap;
extern struct	vfd tempvfd;

extern struct  buf bswlist, *abswlist, *vbswlist;
extern struct  buf *vbufhdr, abufhdr;
extern struct	buf *swbuf, *aswbuf, *vswbuf;
extern struct	bufhd *bufhash, *abufhash, *vbufhash;
extern struct  bufqhead bfreelist[], *abfreelist, *vbfreelist;
extern int	nbuf, nswbuf;
extern struct bufqhead net_bchain, *vnet_bchain, *anet_bchain;

/* MEMSTATS */
extern struct  cblock *cfree, *acfree, *vcfree;
extern int	nclist;
extern int	shmmni;
extern int	bufpages;

extern struct  shmid_ds sshmid, *shmem, *ashmem, *vshmem;
extern struct  shminfo shminfo, *ashminfo;
extern struct  shmid_ds **shm_shmem, **ashm_shmem, **vshm_shmem;

extern struct  vfd **shm_vfds, *ashm_vfds;

extern struct ihead *ihead;


extern struct  ihead *aihead, *vihead;
extern struct  inode *ifreeh, **ifreet;

extern struct  inode *inode, *ainode, *vinode;
extern int	ninode;

extern struct rnode *rtable[RTABLESIZE], *artable, *vrtable;
extern struct rnode *rpfreelist;

extern struct vfs *rootvfs;
extern struct ncache *ncache, *ancache, *vncache;
extern struct nc_hash nc_hash[], *anc_hash, *vnc_hash;
extern struct nc_lru nc_lru, *anc_lru, *vnc_lru;
extern struct ncstats ncstats, *ancstats;

extern struct dquot dqfreelist, *vdqfree, *adqfree;
extern struct dquot dqresvlist, *vdqresv, *adqresv;
extern struct dqhead *dqhead, *vdqhead, *adqhead;

/*
 * extern struct  mounthead mounthash[], *amounthash, *vmounthash;
 */

/* nsp table */
extern struct nsp *nsp, *ansp, *vnsp;
extern int ncsp, max_nsp;

/* mount */
extern int mountlock, mount_waiting;

/* selftest */
extern int selftest_passed;

/* cluster table and misc */
extern struct cct clustab[MAXSITE], *vclustab, *aclustab;
extern site_t my_site, swap_site, root_site;
extern u_int my_site_status;

/* dm and dux mbufs */
extern int dm_is_interrupt;
extern dm_message dm_int_dm_message;
/* mbuf stats */
extern struct dux_mbstat dux_mbstat;

/* protocol */
extern struct using_entry *using_array, *vusing_array, *ausing_array;
extern struct serving_entry *serving_array, *vserving_array, *aserving_array;
extern int using_array_size, serving_array_size;
/* DM op stats */
extern struct proto_opcode_stats inbound_opcode_stats;
extern struct proto_opcode_stats outbound_opcode_stats;
extern char *dm_opcode_descrp[];

/* mapping stuff for spectrum */

#ifdef hp9000s800
extern struct  hpde *htbl, *ahtbl, *vhtbl;
extern struct  hpde *pdir, *apdir, *vpdir;
extern struct  hpde **pgtopde_table, **apgtopde_table, **vpgtopde_table;
extern int pdirhash_type;
extern int newpageshift;
extern int scaled_npdir;
extern struct hpde tmppde;
extern int	nhtbl, npdir, niopdir, uptr, npids, niopids;

/* MEMSTATS */
extern int	nhtbl2, *htbl2, *htbl1;

extern struct  vfd *Umap;

#endif hp9000s800
extern struct  user *ubase;
extern int    uptr;

extern int rpb;
extern int rpbbuf[];
extern char *rpbdescriptor[];
extern char *savestatedescriptor[];


extern int trc[];
extern int sptrc[];

extern	int	allow_sigint;
extern	int	got_sigint;
extern unsigned int global_mask, global_value;
extern unsigned int stackaddr, stacksize;

#ifdef INTERACTIVE
extern	int	interactive;
extern int	cur_proc_addr;
extern int	cur_text_addr;
extern int	cur_buf_addr;
extern int	cur_swbuf_addr;
extern int	cur_inode_addr;
extern int	cur_file_addr;
extern int 	cur_vnode_addr;
extern int	cur_rnode_addr;
extern int	cur_cred_addr;
extern int	cur_pdir_addr;
extern int	cur_cmap_addr;
extern int	cur_mem_addr;
extern int	cur_phymem_addr;
extern int	cur_shmem_addr;
extern int	cur_mbuf_addr;
#endif

extern FILE *outf, *outf1;

extern int	pid;

/* Page usage log table */
extern struct paginfo *paginfo;
extern char	*typepg[] ;
#ifdef hp9000s800
extern char *typepdir[] ;
#endif


/* Buffer usage log table */
extern struct bufblk *bufblk;
extern char *bufnames[];

/* Swap buffer headr log table */
extern struct swbufblk *swbufblk;
extern long	nswbufblk;
extern char *swpnames[];

/* inode usage log table */
extern struct inodeblk *inodeblk;
extern long	ninodeblk;
extern char *inodenames[];

/* disc block log table */
extern struct dblks *dblks;
extern long	ndblks;



/* pregion  log table */
extern struct	pregblk *vasblks;
extern long	nvasblks;

/* pregion  log table */
extern struct	pregblk *pblks;
extern long	npblks;

/* pregion types */
extern char *pnames[];

/* region  log table */
extern struct	regblk *rblks;
extern long	nrblks;

/* sysmap log table */
extern struct sysblks *sblks;
extern int nsblks;

/* region types */
extern char *rnames[];

/* MEMSTATS */
extern int pfdat_types[];

/* kmemstats names */
extern char *kmemnames[];
extern struct kmemstats *kmemstats, *vkmemstats, *akmemstats;


/* swapmap type of pages */
extern char *dnames[];

/* core map type of pages */
extern char	*tynames[] ;

union	{
	char buf[UPAGES][NBPG];
	struct user U;
	} extern u_area;
#define	u	u_area.U /* put in external file as well */

extern char kstack[][NBPG];

extern char *cursym;

extern char     *kernel;
extern int	fcore ;
extern int	fsym ;
extern int	fswap ;
extern int 	frealcore;
extern int	loadbase;

#ifdef hp9000s800
extern struct header somhdr;
extern struct som_exec_auxhdr auxhdr;
extern struct symbol_dictionary_record *symdict, *esymdict;
struct	nlist *symtab, *esymtab;
extern  struct sominfo txtmap;
#endif


extern struct	nlist nl[] ;

#ifdef DEBREC
extern int xdbenable;
#endif


#ifdef hp9000s300

extern u_int usrstack;
extern u_int highpages;
extern struct ste *segtable;
extern struct pte *ptetable;

#endif

extern int physmembase;
extern int three_level_tables;

extern int UIDHSZ;
