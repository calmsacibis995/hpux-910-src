/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/analyze.c,v $
 * $Revision: 1.77.83.3 $       $Author: root $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 16:30:01 $
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


/*
 *  Analyze origionally base on analyze.c 	4.7 (Berkeley) 5/24/83
 *  Although it has evolved into a much more complete and useful tool.
 *
 */

/*
 * Analyze - analyze a core (and optional paging area) saved from
 * a virtual Unix system crash. Analyze also has the cabability to
 * read dev/mem on an active system.
 */

#include "inc.h"
#include "defs.h"
#include "types.h"
#include "an_rpb.h"

#ifdef MP
#undef setjmp
#undef longjmp
#endif /* MP */

int	Qflg, Dflg, dflg, Iflg, iflg, Bflg, bflg, vflg, Cflg, Jflg;
int	Fflg, sflg, Mflg, Uflg, Sflg, Rflg, Hflg, qflg;
int	Aflg, Vflg, Pflg, Eflg, Xflg;
int	Tflg = 1; /* we will always dump the text table and addresses now */
int	Nflg;
int 	translate_enable, proc0;
int	Lflg;
int	suppress_default;
# define DUXFLAG Lflg
# define DUXCHAR 'L'

#ifdef VNODE
/* Vflg is unused, let's take it for dnlc */
# define DNLC_FLAG Vflg
# define DNLC_CHAR 'V'
#endif

int	NBUFHASH, BUFHSZ, INOHSZ, UIDHSZ;

/*
 * Sometimes we have only a partial core dump, and want analyze to
 * continue, if so poke this to one. A non-zero value will cause analyze
 * to continue if getting an error while reading an io table. It will
 * also turn off reading of all of networking structure. Thus it should
 * only be used sparingly. For that reason it is not an option. You should
 * never need to poke this if you have a complete core dump.
 */
int	tolerate_error = 0;

#ifdef DEBREC
/* Enable xdb like stack trace */
int	xdbenable = 1;
#endif

#ifdef iostuff
int	Oflg, oflg;
int	Zflg, zflg;
#endif
int	lflg;	/* MP spinlocks and semaphores */

int	semaflg;
int	activeflg = 0;
int	loadoffset = 0;
int	maxdepth;

/* end of bss */
int endbss = 0;

struct	proc sproc, *proc, *aproc, *vproc, *currentp;
int	nproc;
int 	oldpri;

struct	file *file, *afile, *vfile;
int	nfile;

struct	prochd qs[NQS], *aqs, *vqs;

struct	pte *Sysmap, *aSysmap, *vSysmap;
int 	Sysmapsize;

#ifdef MP
struct mpinfo *mp, *amp, *vmp;
struct mp_iva *mpiva, *ampiva, *vmpiva;
#endif

/* !!!! REGION STUFF !!!! */

/* pfdat structure */
struct pfdat phead, *aphead, *vphead;
struct pfdat pbad, *apbad, *vpbad;
struct pfdat *pfdat, *apfdat, *vpfdat;
struct pfdat **phash, *aphash, *vphash;
int	phashmask;


/* pregion structure */
struct pregion *prp, *aprp, *vprp, *vpregion, *pregion, *apregion;
struct pregion *prpfree;

/* region structure */
struct region *region, *aregion, *vregion;
struct region regactive, *aregactive, *vregactive;
struct region regfree, *aregfree, *vregfree;

/* swap stuff */
swpt_t swaptab[MAXSWAPCHUNKS], *aswaptab, *vswaptab;
swdev_t swdevt[NSWAPDEV], *aswdevt, *vswdevt;
struct vnode        *swapdev_vp;
use_t *usetable[MAXSWAPCHUNKS];
struct dblks *dusagetable[MAXSWAPCHUNKS];
int nextswap;
int swapwant;

#ifdef  hp9000s300
/* Syssegtab information */
struct pte *ptetable;
struct ste *segtable, *vsysseg;
#endif

/* system allocation map */
struct map *sysmap, *asysmap, *vsysmap;

/* quad4map */

/* sys info map */
struct sysinfo sysinfo, *asysinfo, *vsysinfo;

/* var structure with sizes of tables, etc... */
struct var v, *av;

/* temp for reading dbd */
dbd_t dbd_temp;

int	nswapmap;
dev_t	swapdev;
#ifdef hp9000s800
short	*uidhash, *auidhash, *vuidhash;
#endif

unsigned schtackaddr;

int	firstfree;
int	maxfree;
int	freemem;
/* starting value for doio.c, before we get the real value */
int 	physmem = 0x800;

/* We will use current pcoq for current pc on s300 */
int currentpcsq, currentsr5;
int currentsp, currentpcoq;

#ifdef DEBREC
int currentdp, currentpcoq2, currentrp, currentpsw;
int currentsr4, currentsr6, currentsr7;
#endif

char	*aetext, *vetext, *aend, *vend;

int 	ecamx;
struct	vfd *camap, *acamap, *aecamap;
struct  vfd tempvfd;

struct  buf bswlist, *abswlist, *vbswlist;
struct  buf *vbufhdr, *abufhdr;
struct	buf *swbuf, *aswbuf, *vswbuf;
struct	bufhd *bufhash, *abufhash, *vbufhash;
struct  bufqhead bfreelist[BQUEUES], *abfreelist, *vbfreelist;
int	nbuf, nswbuf;

/* MEMSTATS */
struct  cblock *cfree, *acfree, *vcfree;
int	nclist;
#ifndef hp9000s300
int	shmmni;
#endif /* not hp9000s300 */
int	bufpages;

struct  shmid_ds sshmid, *shmem, *ashmem, *vshmem;
struct  shminfo shminfo, *ashminfo, *vshminfo;
struct  shmid_ds **shm_shmem, **ashm_shmem, **vshm_shmem;

struct  kmemstats *kmemstats, *akmemstats, *vkmemstats;

struct  vfd **shm_vfds, *ashm_vfds;

struct ihead *ihead;

struct  ihead *aihead, *vihead;
struct  inode *ifreeh, **ifreet;

struct  inode *inode, *ainode, *vinode;
int	ninode;

struct rnode *rtable[RTABLESIZE], *artable, *vrtable;
struct rnode *rpfreelist;

struct	vfs *rootvfs;
struct  ncache *ncache, *ancache, *vncache;
struct	nc_hash nc_hash[NC_HASH_SIZE], *anc_hash, *vnc_hash;
struct	nc_lru	nc_lru, *anc_lru, *vnc_lru;
struct	ncstats ncstats, *ancstats;
/*
 * struct mounthead mounthash[MNTHASHSZ], *amounthash, *vmounthash;
 */

/* nsp table */
struct nsp *nsp, *ansp, *vnsp;
int ncsp, max_nsp;

/* mount */
int mountlock, mount_waiting;

/* selftest */
int selftest_passed;

/* cluster table and misc */
struct cct clustab[MAXSITE], *vclustab, *aclustab;
site_t my_site, swap_site, root_site;
u_int my_site_status;

/* dm and dux mbufs */
int dm_is_interrupt;
dm_message dm_int_dm_message;
/* mbuf stats */
struct dux_mbstat dux_mbstat;
struct bufqhead net_bchain, *vnet_bchain, *anet_bchain;

/* protocol */
struct using_entry *using_array, *vusing_array, *ausing_array;
struct serving_entry *serving_array, *vserving_array, *aserving_array;
int using_array_size, serving_array_size;
/* DM op stats */
struct proto_opcode_stats inbound_opcode_stats;
struct proto_opcode_stats outbound_opcode_stats;

/* mapping stuff for s800 */

#ifdef hp9000s800

struct  hpde *htbl, *ahtbl, *vhtbl;
struct  hpde *pdir, *apdir, *vpdir;
struct  hpde **pgtopde_table, **apgtopde_table, **vpgtopde_table;
struct	hpde *base_pdir;
struct	hpde *max_pdir;
int pdirhash_type = -1;
int newpageshift = -1;
int scaled_npdir = -1;

int	nhtbl, npdir, niopdir, npids, niopids;
#endif /* hp9000s800 */

int     uptr;
struct  user *ubase;

/* DISK QUOTA */

struct dquot dqfreelist, *vdqfree, *adqfree;
struct dquot dqresvlist, *vdqresv, *adqresv;
struct dqhead *dqhead, *vdqhead, *adqhead;


/* MEMSTATS */
int *htbl1;
int *htbl2;
int nhtbl2;

unsigned int  boottime;

#ifdef hp9000s800
int	 rpb;
int  rpbbuf[sizeof (struct rpb) / sizeof (int)];
int  crash_processor_table [CRASH_TABLE_SIZE/sizeof (int)];
int  crash_event_table [CRASH_EVENT_TABLE_SIZE/sizeof (int)];
int  cet_entries = CRASH_EVENT_TABLE_SIZE/sizeof (struct crash_event_table_struct);
int	 rpb_list [MAX_RPBS] = 0;
#endif /* hp9000s800 */

int lotsfree, minfree, desfree;

/* stack trace buffer */
int trc[MAXSTKTRC+OVRHD+2];
int sptrc[MAXSTKTRC+OVRHD+2];

int 	allow_sigint = 0;
int 	got_sigint  = 0;
void	sigint_handler();
void	sigquit_handler();
struct  sigvec vec;
jmp_buf jumpbuf;
int 	runstringopt;
int 	realstatus;

u_int global_mask, global_value;

u_int stackaddr, stacksize;

#ifdef INTERACTIVE
char	scanbuf[80];
int	scanlength;
int	eof;
int 	interactive;
int	cur_proc_addr;
int	cur_text_addr;
int	cur_buf_addr;
int	cur_swbuf_addr;
int	cur_inode_addr;
int	cur_file_addr;
int	cur_vnode_addr;
int	cur_rnode_addr;
int	cur_cred_addr;
int	cur_cmap_addr;
int	cur_pdir_addr;
int	cur_mem_addr;
int	cur_phymem_addr;
int	cur_shmem_addr;
extern char *version;

/* zero templates used to clear out a structure */
struct paginfo   zeropaginfo;
struct swbufblk  zeroswbufblk;
struct inodeblk  zeroinodeblk;
struct dblks      zerodblks;

#endif /* INTERACTIVE */

FILE    *outf = stdout;
FILE    *outf1 = stdout;
int	pid;

#ifdef DEBREC
extern char *vsbSymfile;
extern char *vsbCorefile;
#endif

#ifdef hp9000s800
char *typepdir[] = {
	"invalid",
	"prevalid",
	"valid"
};
#endif

/* Buffer usage log table */
struct	bufblk *bufblk;

char *bufnames[]= {
	"free",
	"locked",
	"lru",
	"age",
	"empty",
	"virt",
	"hash"
};

/* Swap buffer headr log table */
struct	swbufblk *swbufblk;
long	nswbufblk;

char *swpnames[]= {
	"free",
	"swap",
	"clean"
};

/* inode usage log table */
struct	inodeblk *inodeblk;
long	ninodeblk;

char *inodenames[]= {
	"empty",
	"chain",
	"free"
};

/* disc block log table */
struct	dblks *dblks;
long	ndblks;

/* disc map type of pages */
char *dnames[] = {
	"DFREE",
	"DDATA",
	"DSTACK",
	"DTEXT",
	"DUDOT",
	"DPAGET",
	"DSHMEM",
};

struct pregblk zpregblk;
struct regblk zregblk;

/* Page usage log table */
struct	paginfo *paginfo;

char	*typepg[] = {
	"lost",
	"free",
	"sys",
	"unused",
	"uarea",
	"stext",
	"data",
	"stack",
	"shmem",
	"nulldref",
	"libtxt",
	"libdat",
	"sigstack",
	"io",
	"mmap",
	"ISRfreepool",
	/* MEMSTATS */
	"overflow",
};

/* MEMSTATS */
int pfdat_types[ZMAX+1];

/* pregion types */
char *pnames[] = {
	"unused",
	"uarea",
	"text",
	"data",
	"stack",
	"shmem",
	"nulldref",
	"libtxt",
	"libdat",
	"sigstack",
	"i/o",
	"mmap",
};

/* sysmap log table */
struct sysblks *sblks;
int nsblks;

/* region types */
char *rnames[] = {
	"unused",
	"private",
	"shared",
};

/* Kernel memory allocator names */
char *kmemnames[] = {
	"FREE",
	"MBUF",
	"DATA (DYNAMIC)",
	"HEADER",
	"SOCKET",
	"PCB",
	"RTABLE",
	"HTABLE",
	"ATABLE",
	"SONAME",
	"SOOPTS",
	"FTABLE", /* 11 */
	"RIGHTS",
	"IFADDR",
	"NTIMO",
	"DEVBUF",
	"ZOMBIE",
	"NAMEI",
	"GPROF",
	"IOCTLOPS",
	"SUPERBLK",
	"CRED",
	"TEMP",
	"VAS",
	"PREG",
	"REG",
	"IOSYS",
	"NIPCCB",
	"NIPCSR",
	"NIPCPR",
	"DQUOTA",
	"DMA",
	"GRAF",
	"ITE",
	"LAST",
};

/* core map type pages */
char	*tynames[] = {
	"sys",
	"text",
	"data",
	"stack",
	"kstack",
	"shmem"
};


int three_level_tables=0;
int physmembase=0;
vas_t *kernvasp;
unsigned int tt_region_addr;

/* set aside space for our u_area */
/* of course, there are callers who treat it by the page, so.... */
union	{
	char buf[UPAGES][NBPG];
	struct user U;
} u_area;
#define	u	u_area.U /* put in external file as well */

/*
 * set aside space for kernel stack
 *
 * This structure is used to hold both the ICS and the kernel stack.
 * We are making a valid assumption that ICS_SIZE (5) is greater than
 * STACK_SIZE (4)
 */
char kstack[ICS_SIZE][NBPG];

char *cursym;

char    *kernel;
int	fcore = -1;     /* core file fd */
int	fsym = -1;      /* som  file fd */
int	fswap = -1;     /* swap file fd */

int	frealcore = -1; /* Holds fcore pointer */
int	fkmem = -1;    	/* Holds fkmem, s300 only */
char *  loadbase=0;


#ifdef hp9000s800
struct header somhdr;
struct som_exec_auxhdr auxhdr;
struct symbol_dictionary_record *symdict, *esymdict;
struct	nlist *symtab, *esymtab;
struct sominfo txtmap;
#endif

#include "./an_nlist.h"

main(argc, argv)
	int argc;
	char **argv;
{
	register struct nlist *np;
	register int i;
	extern sproc_text(), dmcheck(), shmem_table(), vfs_list();
	extern file_table(), bufcheck(), icheck(), pdircheck();
	extern rcheck();
	extern freelist(), coresummary(), iocheck(), snapshot();
	extern account_alloc();
	extern int getimcport();
	struct crash_event_table_struct *cet_ptr;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {

		register char *cp = *argv++;
		argc--;
		runstringopt++;
		if (processoptions(cp))
			exit(1);

	}

	if (argc < 1) {
		printusage();
		exit(1);
	}

	/* Warn him if he turned on tolerate_error */
	if (tolerate_error) {
		fprintf(stderr, "\n\n Warning, you have enabled the tolerate_error flag.\n");
		fprintf(stderr, " This shuts off network capability, and ignores all\n");
		fprintf(stderr, " read errors on structures.  Use only on\n");
		fprintf(stderr, " incomplete crash dumps.\n\n");
	}


	/* Install signal handler */
	allow_sigint = 0;
	vec.sv_handler = sigint_handler;
	vec.sv_mask = (1L << (SIGINT -1));
	vec.sv_onstack = 0;
	sigvector(SIGINT, &vec, 0);

	vec.sv_handler = sigquit_handler;
	vec.sv_mask = (1L << (SIGQUIT -1));
	vec.sv_onstack = 0;
	sigvector(SIGQUIT, &vec, 0);


	/* Open our core file */
	if ((fcore = open(argv[0], 0)) < 0) {
		perror(argv[0]);
		exit(1);
	}

	/* Check if we are running against an active kernel */
	if (strcmp(argv[0], "/dev/mem") == 0) {
		activeflg++;
	}

	frealcore = fcore;
#ifdef hp9000s300
	if (activeflg) {
		/* Open /dev/kmem */
		if ((fkmem = open("/dev/kmem", 0)) < 0) {
			perror("/dev/kmem open");
			exit(1);
		}
		fcore = fkmem;
	}
#endif

	if (strcmp(argv[0], "/dev/indigo") == 0)
		activeflg++;

	/* If this guy wants it, make him realtime (he gets to run alot
	 * faster, and data structures are not changed as much underneath
	 * him. (This should be used when analyzing an active system
	 * ie core=/dev/mem, and in the FOREGROUND ONLY!!!).
	 */

	if (Rflg) {
		oldpri = rtprio(0, 127);
	}

	/* get kernel name, and open file to read symbols */
	kernel = (argc > 1 ? argv[1] : "/hp-ux");
	if ((fsym = open(kernel, 0, 0)) < 0) {
		perror(kernel);
		exit(1);
	}

#ifdef DEBREC
	vsbSymfile  = kernel;
	vsbCorefile = argv[0];
#endif

	if (activeflg) {
		if ((fswap = open("/dev/swap", 0, 0)) < 0) {
			fprintf(stderr, "Open of dev/swap failed\n");
			fprintf(stderr, "errno = %d\n", errno);
		}
	}

	/* scan kernel for address of symbols */
	fprintf(stderr, "\n HP-UX Analyze version: [%s]\n", version);
	fprintf(stderr, " ======================================\n");
	fprintf(stderr, "\n Reading symbol table...");
	read_symbols();
	fprintf(stderr, " done\n");

	/* since I already have the symbol table, I could just do the
	 * nlist operation myself. Why not.
	 */

	fprintf(stderr, " Scanning symbols....");
	for (np = nl; *np->n_name != '\0' ; np++) {
		np->n_value = lookup(np->n_name);
	}

#ifdef notdef
	nlist(kernel, nl);
#endif

	fprintf(stderr, "    done\n");

#ifdef NETWORK
	net_nlist();
#endif NETWORK

	/* See if nlist worked */
	if (nl[0].n_value == 0) {
		fprintf(stderr, "%s: bad namelist\n",
		    kernel);
		exit(1);
	}

	/* Make sure that we have the right kernels before continuing */
	check_kernel();

	/*
	 * Go get some needed values from the core file. These are
	 * equivalently mapped values!!!
	 */

#ifdef hp9000s300
	/*
	 * Get this first, so we will have the option of using /dev/mem
	 */
	physmembase = get(nl[X_PHYSMEMBASE].n_value);
	loadbase = (char *)(physmembase << PGSHIFT);
	if (vflg) {
		fprintf(outf, "physmembase        = 0x%08x\n",
		    physmembase);
		fprintf(outf, "loadbase           = 0x%08x\n",
		    loadbase);
	}

	kernvasp = (vas_t *)(nl[X_KERNVAS].n_value);
	tt_region_addr = (unsigned int)get(nl[X_TT_REGION_ADDR].n_value);
	three_level_tables = (unsigned int)get(nl[X_THREE_LEVEL_TABLES].n_value);

	if (vflg) {
		fprintf(outf, "kernvasp           = 0x%08x\n",
		    kernvasp);
		fprintf(outf, "tt_region_addr     = 0x%08x\n",
		    tt_region_addr);
		fprintf(outf, "three_level_tables = %d\n\n",
		    three_level_tables);
	}
#endif

#ifdef hp9000s300
	/* Read in Syssegtable */
	segtable = (struct ste *)calloc(SEGTBLENT, sizeof (struct ste));
	vsysseg = (struct ste *)(nl[X_SYSSEGTAB].n_value);
	lseek(fcore, (long)vsysseg, 0);
	if (read(fcore, (char *)segtable, SEGTBLENT * sizeof (struct ste))
		!= (SEGTBLENT * sizeof (struct ste)) ) {
			perror("syssegtab read");
	}
	/* create ptetable */
	ptetable = (struct pte *)calloc(PGTBLENT, sizeof (struct pte));
	translate_enable = 1;
#endif

	ubase = (struct user *)(UAREA);
	uptr = (int)ubase;
	vphead = (struct pfdat *)(nl[X_PHEAD].n_value);
	vpbad = (struct pfdat *)(nl[X_PBAD].n_value);
	vpfdat = (struct pfdat *)get(nl[X_PFDAT].n_value);
	vphash = (struct pfdat *)get(nl[X_PHASH].n_value);
	phashmask = (int)get(nl[X_PHASHMASK].n_value);
	vsysmap = (struct map *)get(nl[X_SYSMAP].n_value);

	/* quad4map */

	aregactive = vregactive = (struct region *)(nl[X_REGACTIVE].n_value);
	aregfree = vregfree = (struct region *)(nl[X_REGFREE].n_value);
	aswaptab = vswaptab = (swpt_t *)(nl[X_SWAPTAB].n_value);
	aswdevt = vswdevt = (swdev_t *)(nl[X_SWDEVT].n_value);
	swapdev_vp = (struct vnode *)get(nl[X_SWAPDEV_VP].n_value);
	asysinfo = vsysinfo = (struct sysinfo *)(nl[X_SYSINFO].n_value);
	physmem = get(nl[X_PHYSMEM].n_value);

	nproc = get(nl[X_NPROC].n_value);
	firstfree = get(nl[X_FIRSTFREE].n_value);
	maxfree = get(nl[X_MAXFREE].n_value);
	freemem = get(nl[X_FREEMEM].n_value);

	nswbuf = get(nl[X_NSWBUF].n_value);
	nbuf = get(nl[X_NBUF].n_value);
	ninode = get(nl[X_NINODE].n_value);

	/* MEMSTATS */
	nclist = get(nl[X_NCLIST].n_value);
	bufpages = get(nl[X_BUFPAGES].n_value);
#ifndef hp9000s300
	shmmni = get(nl[X_SHMMNI].n_value);
#endif /* not hp9000s300 */

#ifdef MP
	/*	spl_lock is now a regular spinlock
	spl_lock = get(nl[X_SPL_LOCK].n_value);
	spl_word = get(nl[X_SPL_WORD].n_value);
	*/
#endif

	/* get configurable table values */
	BUFHSZ	= get(nl[X_BUFHSZ].n_value);
	NBUFHASH = BUFHSZ + 2;
	bufhash	= (struct bufhd *)malloc(NBUFHASH * sizeof (struct bufhd));
	INOHSZ	= get(nl[X_INOHSZ].n_value);
	ihead	= (struct ihead *)malloc(INOHSZ * sizeof (struct ihead));
	/* UIDHSZ is a define in the kernel; UIDHMASK is UIDHSZ - 1 */
	UIDHSZ	= get(nl[X_UIDHMASK].n_value) + 1;

#ifdef hp9000s800
	uidhash	= (short *)malloc(UIDHSZ * sizeof (short));

	/* Exception to the atype = getphyaddr(vtype), only allowed in s800 */
	vpgtopde_table = apgtopde_table = (struct hpde **)get(nl[X_PGTOPDE_TABLE].n_value);
	vpdir = apdir = (struct hpde *)get(nl[X_PDIR].n_value);
	npdir = get(nl[X_NPDIR].n_value);

	/* Exception to the atype = getphyaddr(vtype), only allowed in s800 */
	scaled_npdir = get(nl[X_SCALED_NPDIR].n_value);
	pdirhash_type = get(nl[X_PDIRHASH_TYPE].n_value);
	vhtbl = ahtbl = (struct hte *)get(nl[X_HTBL].n_value);
	nhtbl = get(nl[X_NHTBL].n_value);
	rpb = get(nl[X_RPB].n_value);

	base_pdir = (struct  hpde *)get(lookup("base_pdir"));
	max_pdir = (struct  hpde *)get(lookup("max_pdir"));
#endif

	lotsfree = get(nl[X_LOTSFREE].n_value);
	minfree = get(nl[X_MINFREE].n_value);
	desfree = get(nl[X_DESFREE].n_value);

	vend = (char *)nl[X_END].n_value;
	vetext = (char *)nl[X_ETEXT].n_value;

	ifreeh = (struct inode *)get(nl[X_IFREEH].n_value);
	ifreet = (struct inode **)get(nl[X_IFREET].n_value);
	rootvfs = (struct vfs *)get(nl[X_ROOTVFS].n_value);
	rpfreelist = (struct rnode *)get(nl[X_RPFREELIST].n_value);
	/* I am assuming these variables are equivalently mapped */
	my_site = getshort(nl[X_MY_SITE].n_value);
	swap_site = getshort(nl[X_SWAP_SITE].n_value);
	root_site = getshort(nl[X_ROOT_SITE].n_value);
	my_site_status = get(nl[X_MY_SITE_STATUS].n_value);
	if (my_site_status & CCT_CLUSTERED) {
		ncsp = get(nl[X_NCSP].n_value);
		using_array_size = get(nl[X_USING_ARRAY_SIZE].n_value);
		serving_array_size = get(nl[X_SERVING_ARRAY_SIZE].n_value);
	}

#ifdef hp9000s800
	/* Get pdir and hash table space and addesses */
	pdir = (struct hpde *)malloc((scaled_npdir) * sizeof (struct hpde));

	/* pdir is an offset into iopdir */
	htbl = (struct hpde *)malloc(nhtbl * sizeof (struct hpde));

	/* MEMSTATS */
	htbl1 = (int *)calloc(nhtbl, sizeof (int));
	nhtbl2 = nhtbl/2;
	htbl2 = (int *)calloc(nhtbl2, sizeof (int));

	pgtopde_table =(struct hpde **)malloc(physmem * sizeof (struct hpde *));

	/* read in pdir */
	lseek(fcore, (long)(vpdir), 0);
	if (read(fcore, (char *)pdir, (scaled_npdir) * sizeof (struct hpde))
		!= (scaled_npdir) * sizeof (struct hpde)) {
		perror("pdir read");
		exit(1);
	}

	/* read in hash table */
	lseek(fcore, (long)(vhtbl), 0);
	if (read(fcore, (char *)htbl, nhtbl * sizeof (struct hpde))
		!= nhtbl * sizeof (struct hpde)) {
		perror("htbl read");
		exit(1);
	}

	/* read in pgtopde_table */
	lseek(fcore, (long)(vpgtopde_table), 0);
	if (read(fcore, (char *)pgtopde_table, physmem * sizeof (struct hpde *))
		!= physmem * sizeof (struct hpde *)) {
		perror("pgtopde_read read");
		exit(1);
	}

	/* The S800 can now map!!! */
	/* getphysaddr, getchuck, ltor can now be used */
	translate_enable = 1;

	get_crash_processor_table(nl[X_CRASH_P_TABLE].n_value);
	get_crash_event_table(nl[X_CRASH_E_TABLE].n_value);
	cet_ptr = (struct crash_event_table_struct *)crash_event_table;
	get_rpb_list();

	/*
	 * See if there are any known RPBs that are not represented
	 * in the crash event table.  (There are known kernel paths
	 * that will cause the crash event table not to record entries
	 * or record them incorrectly.
	 */
	find_missing_rpbs();

	getrpb(cet_ptr->cet_savestate);
#endif

	set_pstat_cmds(nl[X_PST_CMDS].n_value);

#ifdef hp9000s300

 /* Or something like this, it does not exist yet... */
/* #define RPB_SP 9
#define RPB_PCOQ 18
	currentsp = rpbbuf[RPB_SP];
	currentpcoq = rpbbuf[RPB_PCOQ];
*/
#else hp9000s800

/* Should have these declared */
#define RPB_SP 32
#define RPB_RP 4
#define RPB_PCOQ 52
#define RPB_SR5 71
#define RPB_PCSQ 51
#define RPB_PCOQ2 53
#define RPB_DP 29
#define RPB_PSW 56
#define RPB_SR6 72
#define RPB_SR7 73

	currentsp = rpbbuf[RPB_SP];
	currentpcoq = rpbbuf[RPB_PCOQ];
	currentsr5 = rpbbuf[RPB_SR5];
	currentpcsq = rpbbuf[RPB_PCSQ];

#ifdef DEBREC

	currentrp = rpbbuf[RPB_RP];
	currentpcoq2 = rpbbuf[RPB_PCOQ2];
	currentdp = rpbbuf[RPB_DP];
	currentpsw = rpbbuf[RPB_PSW];
	currentsr6 = rpbbuf[RPB_SR6];
	currentsr7 = rpbbuf[RPB_SR7];
#endif
	if (currentsp == 0) {
		fprintf(outf, " No current sp to use\n");
		currentsr5 = 0xb;
	}
#endif

#ifdef hp9000s800
	/* Get space for our page information table */
	/* Our page directory is larger then maxfree */
	paginfo = (struct paginfo *)calloc(physmem , sizeof (struct paginfo));
	if (paginfo == NULL) {
		fprintf(stderr, "maxfree %x?... out of mem!\n", maxfree);
		exit(1);
	}
#endif

#ifdef hp9000s800
	/* Get current proc pointer for later use */
	currentp = (struct proc *)get(
		ltor(currentsr5, &((struct user *)(uptr))->u_procp));
#else
	if (activeflg) {
		 ubase = (struct user *)(nl[X_UBASE].n_value);
		 currentp = (struct proc *)get(ubase);
	 } else {
		 currentp = (struct proc *)get(nl[X_CURPROC].n_value);
	 }
#endif

	/* We can do mapping now, since the pdir is setup */
	aphead = (struct pfdat *)getphyaddr(vphead);
	apbad = (struct pfdat *)getphyaddr(vpbad);
	apfdat = (struct pfdat *)(getphyaddr( (unsigned)vpfdat));
	aphash = (struct pfdat *)(getphyaddr( (unsigned)vphash));
	asysmap = (struct map *)(getphyaddr( (unsigned)vsysmap));
	aregactive = (struct region *)getphyaddr(vregactive);
	aregfree = (struct region *)getphyaddr(vregfree);
	aswaptab = (swpt_t *)getphyaddr(vswaptab);
	aswdevt = (swdev_t *)getphyaddr(vswdevt);
	asysinfo = (struct sysinfo *)getphyaddr(vsysinfo);
	aend = (char *)getphyaddr(vend);
	aetext = (char *)getphyaddr(vetext);

	/* don't forget quad4map */

	/* get space for our sysmap log table */
	sblks = (struct sysblks *)calloc(SYSMAPSIZE , sizeof (struct sysblks));

	/* read swaptab, initially now so that we can find out how much
	 * space we need for our use_t tables, and how many.
	 */
#ifdef notdef
	longlseek(fcore, (long)aswaptab, 0);
	if (longread(fcore, (char *)swaptab, MAXSWAPCHUNKS * sizeof (swpt_t))
		!= MAXSWAPCHUNKS * sizeof (swpt_t)) {
		perror("swaptab read");
		if (!tolerate_error)
		exit(1);
	}
	longlseek(fcore, (long)aswdevt, 0);
	if (longread(fcore, (char *)swdevt, NSWAPDEV * sizeof (swdev_t))
		!= NSWAPDEV * sizeof (swdev_t)) {
		perror("swdevt read");
		if (!tolerate_error)
		exit(1);
	}
#endif /* notdef */

#ifdef MP
	/* get handle on address */
	vmp = nl[X_MPPROCINFO].n_value;
	amp = (struct mpinfo *)getphyaddr(vmp);
	/* get space */
	mp = (struct mpinfo *)calloc(MAX_PROCS , sizeof (struct mpinfo));
	mpiva = (struct mp_iva *)calloc(MAX_PROCS , sizeof (struct mp_iva));
#endif

	/* Get space for our buffer information table */
	bufblk = (struct bufblk *)calloc(BUFBLKSZ , sizeof (struct bufblk));
	if (bufblk == NULL) {
		fprintf(stderr, "Couldn't get bufblk space!\n");
		exit(1);
	}


	/* Get space for our swap buffer information table */
	swbufblk =(struct swbufblk *)calloc(nswbuf , sizeof (struct swbufblk));
	if (swbufblk == NULL) {
		fprintf(stderr, "Couldn't get swbufblk space!\n");
		exit(1);
	}


	/* Get space for our inode information table */
	inodeblk =(struct inodeblk *)calloc(ninode , sizeof (struct inodeblk));
	if (inodeblk == NULL) {
		fprintf(stderr, "Couldn't get inodeblk space!\n");
		exit(1);
	}

#ifdef iostuff
	/* get i/o stuff */
	fprintf(stderr, " Scanning I/O....");
	io_init(outf);
	fprintf(stderr, "        done\n");
#endif

	/* Get swapdev value */
	lseek(fcore, (long)(nl[X_SWAPDEV].n_value), 0);
	read(fcore, (char *)&swapdev, sizeof (int));

	/*  Get proc table info and space */
	vproc = (struct proc *)get(nl[X_PROC].n_value);
	/* Convert this   address to its real physical address */
	aproc = (struct proc *)(getphyaddr( (unsigned)vproc));
	proc = (struct proc *)malloc(nproc * sizeof (struct proc));



	/* Get file table info and space */
	lseek(fcore, (long)(nl[X_FILE].n_value), 0);
	read(fcore, (char *)&vfile, sizeof vfile);
	/* Convert this   address to its real physical address */
	afile = (struct file *)(getphyaddr( (unsigned)vfile));
	lseek(fcore, (long)(nl[X_NFILE].n_value), 0);
	read(fcore, (char *)&nfile, sizeof nfile);
	file = (struct file *)calloc(nfile, sizeof (struct file));


#ifdef  hp9000s300
	/* Get space for our page information table */
	paginfo = (struct paginfo *)calloc((maxfree - firstfree +1) , sizeof (struct paginfo));
	if (paginfo == NULL) {
		fprintf(stderr, "maxfree %x?... out of mem!\n", maxfree);
		exit(1);
	}
#endif

	 /* get space for phash table */
	phash= (struct pfdat **)malloc((phashmask+1) * sizeof (struct pfdat **));
	if (phash == NULL) {
		fprintf(stderr, "not enough mem for %x bytes of phash\n",
			(phashmask+1)* sizeof (struct pfdat **));
		exit(1);
	}

	/* get space for pfdat table */
	/* This is garbage.... the region kernel overlaps the pfdat
	 * table with structures before it knowing that it will never
	 * index into that part. Use a macro to convert from page to
	 * pfdat please!!! But for now lets go along with it.
	 */

	pfdat = (struct pfdat *)calloc(physmem, sizeof (struct pfdat));
	if (pfdat == NULL) {
		fprintf(stderr, "not enough mem for %x bytes of pfdat\n",
			physmem*sizeof (struct pfdat));
		exit(1);
	}

	/* get space for sysmap table */
	sysmap = (struct map *)malloc(SYSMAPSIZE * sizeof (struct map));
	if (sysmap== NULL) {
		fprintf(stderr, "not enough mem for %x bytes of sysmap\n",
			SYSMAPSIZE * sizeof (struct map));
		exit(1);
	}

#ifdef NETWORK
	net_getaddrs();
#endif NETWORK

	/* get shared memory info */
	/* read shminfo */
	vshminfo = (struct shminfo *)(nl[X_NSHMINFO].n_value);
	ashminfo = (struct shminfo *)getphyaddr(vshminfo);
	lseek(fcore, (long)(vshminfo), 0);
	if (read(fcore, (char *)&shminfo,  sizeof (struct shminfo))
		!= sizeof (struct shminfo)) {
		perror("shminfo read");
		exit(1);
	}
	vprintf("shminfo.shmmax %d  shminfo.shmseg %d  shminfo.shmmni %d\n",
		shminfo.shmmax, shminfo.shmseg, shminfo.shmmni);


#ifdef hp9000s800
	/* get shm_shmem table info */
	vshm_shmem = (struct shmid_ds **)(nl[X_NSHM_SHMEM].n_value);
	/* Convert this   address to its real physical address */
	ashm_shmem = (struct shmid_ds **)(getphyaddr( (unsigned)vshm_shmem));
	shm_shmem = (struct shmid_ds **)malloc(nproc * shminfo.shmseg * sizeof (struct shmid_ds *));

#else
	/* Get shm_pte table */
#endif

	/* get shmem table info */
	vshmem = (struct shmid_ds *)(nl[X_NSHMEM].n_value);
	/* Convert this   address to its real physical address */
	ashmem = (struct shmid_ds *)(getphyaddr( (unsigned)vshmem));
	shmem = (struct shmid_ds *)malloc(shminfo.shmmni * sizeof (struct shmid_ds));

	/* get run queue info */
	vqs = (struct prochd *)(nl[X_QS].n_value);
	aqs = (struct prochd *)getphyaddr(vqs);

#ifdef hp9000s800
	/* get uidhash info */
	vuidhash = (short *)(nl[X_UIDHASH].n_value);
	auidhash = (short *)getphyaddr(vuidhash);
#endif

	/*  Get buffer info */
	/* bswlist */
	vbswlist = (struct buf *)(nl[X_BSWLIST].n_value);
	/* Convert this   address to its real physical address */
	abswlist = (struct buf *)(getphyaddr( (unsigned)vbswlist));

	/* bfreelist */
	vbfreelist =  (struct buf *)(nl[X_BFREELIST].n_value);
	/* Convert this   address to its real physical address */
	abfreelist = (struct buf *)(getphyaddr( (unsigned)vbfreelist));

	/* bufhash */
	lseek(fcore, (long)(nl[X_BUFHASH].n_value), 0);
	read(fcore, (char *)&vbufhash, sizeof vbufhash);
	/* Convert this address to its real physical address */
	abufhash= (struct bufhd *)(getphyaddr( (unsigned)vbufhash));

	/* Pointer to first buffer header */
	lseek(fcore, (long)(nl[X_BUF].n_value), 0);
	read(fcore, (char *)&vbufhdr, sizeof vbufhdr);
	abufhdr= (struct buf *)(getphyaddr( (unsigned)vbufhdr));

	dqhead = (struct dqhead *) malloc (NDQHASH * sizeof(struct dqhead));

	/* disk	quota (hash table) */
	vdqhead	= (struct dqhead *)(nl[X_DQHEAD].n_value);
	adqhead	= (struct dqhead *)(getphyaddr((unsigned)vdqhead));
	vdqfree	= (struct dquot *)(nl[X_DQFREE].n_value);
	adqfree	= (struct dquot *)(getphyaddr((unsigned)vdqfree));
	vdqresv	= (struct dquot *)(nl[X_DQRESV].n_value);
	adqresv	= (struct dquot *)(getphyaddr((unsigned)vdqresv));

	/* MEMSTATS */
	/* cfree itself */
	lseek(fcore, (long)(nl[X_CFREE].n_value), 0);
	read(fcore, (char *)&vcfree, sizeof vcfree);
	/* Convert this   address to its real physical address */
	acfree= (struct cblock *)(getphyaddr( (unsigned)vcfree));
	cfree = (struct cblock *)malloc(nclist * sizeof (struct cblock));

	/* swbuf itself */
	lseek(fcore, (long)(nl[X_SWBUF].n_value), 0);
	read(fcore, (char *)&vswbuf, sizeof vswbuf);
	/* Convert this   address to its real physical address */
	aswbuf= (struct buf *)(getphyaddr( (unsigned)vswbuf));
	swbuf = (struct buf *)malloc(nswbuf * sizeof (struct buf));

	vncache = (struct ncache *)(nl[X_NCACHE].n_value);
	ancache = (struct ncache *)(getphyaddr((unsigned)vncache));
	vnc_hash = (struct nc_hash *)(nl[X_NC_HASH].n_value);
	anc_hash = (struct nc_hash *)(getphyaddr((unsigned)vnc_hash));
	vnc_lru = (struct nc_lru *)(nl[X_NC_LRU].n_value);
	anc_lru = (struct nc_lru *)(getphyaddr((unsigned)vnc_lru));
	ancstats = (struct ncstats *)(getphyaddr((unsigned)clear(nl[X_NCSTATS].n_value)));
	ncache = (struct ncache *)malloc(ninode * sizeof (struct ncache));

	/* inode table itself */
	lseek(fcore, (long)(nl[X_INODE].n_value), 0);
	read(fcore, (char *)&vinode, sizeof vinode);
	/* Convert this   address to its real physical address */
	ainode = (struct inode *)(getphyaddr( (unsigned)vinode));
	inode = (struct inode *)malloc(ninode * sizeof (struct inode));


	/* ihead  (inode hash table ) */
	vihead = (struct ihead *)(nl[X_IHEAD].n_value);

	/* Convert this address to its real physical address */
	aihead = (struct ihead *)(getphyaddr( (unsigned)vihead));

	/* rnode hash table */
	vrtable = (struct rnode *)(nl[X_RTABLE].n_value);
	artable = (struct rnode *)(getphyaddr((unsigned)vrtable));

	/* kmemstats */
	vkmemstats = (struct kmemstats *)nl[X_KMEMSTATS].n_value;
	akmemstats = (struct kmemstats *)getphyaddr((unsigned)vkmemstats);
	/* Convert this address to its real physical address */
	kmemstats = (struct kmemstats *)malloc(M_LAST * sizeof (struct kmemstats));

	if (my_site_status & CCT_CLUSTERED) {
		/* nsp table */
		vnsp = (struct nsp *)(nl[X_NSP].n_value);
		ansp = (struct nsp *)(getphyaddr((unsigned)vnsp));
		nsp = (struct nsp *)malloc(ncsp * sizeof (struct nsp));
		/* cluster table */
		vclustab = (struct cct *)(nl[X_CLUSTAB].n_value);
		aclustab = (struct cct *)(getphyaddr((unsigned)vclustab));
		/* protocol tables - using and serving arrays */
		vusing_array = (struct using_entry *)
			(nl[X_USING_ARRAY].n_value);
		ausing_array = (struct using_entry *)
			(getphyaddr((unsigned)vusing_array));
		using_array =  (struct using_entry *)
			malloc(using_array_size * sizeof (struct using_entry));
		vserving_array = (struct serving_entry *)
			(nl[X_SERVING_ARRAY].n_value);
		aserving_array = (struct serving_entry *)
			(getphyaddr((unsigned)vserving_array));
		serving_array = (struct serving_entry *)
			malloc(serving_array_size*sizeof (struct serving_entry));
	}

#ifdef DEBREC
	/* initialize debug records */
	if (xdbenable)
		xdbglue();
#endif

	/* read in kernel data structures */
	snapshot();

#ifdef INTERACTIVE
	interactive = isatty(0);
	if (runstringopt) {
		scan(0, 0);
		quit();
	} else {
		while (!eof) {
			if (setjmp(jumpbuf)) {
				restoreoptions();
			}
			/* clear sigint flag, and ignore sigints */
			got_sigint = 0;
			allow_sigint = 0;
			printf("\n");
			if (interactive)
				printf("Command: ");
			yyparse();
		}
	}

#else

	/* Batch type environment */
	scan(0, 0);
	quit();
#endif

}

f_stackaddr()
{
#ifdef hp9000s300
	return KSTACKBASE;
#else
#ifdef hp9000s800
	if (schtackaddr)
		return schtackaddr;
	else
		return KSTACKADDR;
#else
    you lose.
#endif
#endif
}
