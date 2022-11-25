static char *HPUX_ID = "@(#)$Revision: 66.3 $";

#include <stdio.h>
#include <fcntl.h>
#include <nlist.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <unistd.h>

/*
 * Translation table to map handles for drivers to dfile names
 */
struct xlate {
	char	*from;
	char	*to;
} xlate[] = {	"dragon",	"fpa",
		"lp",		"printer",
		"ptym",		"ptymas",
		"ptys",		"ptyslv",
		"ram",		"ramdisc",
		"rfai",		"rfa",
		"simon",	"98625",
		"sio626",	"98626",
		"sio628",	"98628",
		"sio642",	"98642",
		"srm629",	"srm",
		"stealth",	"vme2",
		"stp",		"stape",
		"ti9914",	"98624",
		"tp",		"tape",
		NULL,		NULL };

struct nlist nl[] = {
	{ "_amigo_open" },
	{ "_ciper_open" },
	{ "_cs80_open" },
	{ "_cdfs_link" },
	{ "_dos_open" },
	{ "_dskless_link" },
	{ "_dragon_isr" },
	{ "_gpio_link" },
	{ "_hpib_open" },
	{ "_lla_open" },
	{ "_nfs_link" },
	{ "_nsdiag0_open" },
	{ "_lp_open" },
	{ "_ptym_open" },
	{ "_ptys_open" },
	{ "_ram_open" },
	{ "_rdu_read" },
	{ "_rfai_link" },
	{ "_rje_open" },
	{ "_scsi_link" },
	{ "_snalink_open" },
	{ "_srm629_open" },
	{ "_stp_open" },
	{ "_tp_open" },
	{ "_vme_open" },
	{ "_stealth_open" },
	{ "_ti9914_link" },
	{ "_simon_link" },
	{ "_sio626_link" },
	{ "_sio628_link" },
	{ "_sio642_link" },
	{ "_acctresume" },
	{ "_acctsuspend" },
	{ "_argdevnblk" },
	{ "_check_alive_period" },
	{ "_dmmax" },
	{ "_dmmin" },
	{ "_dmshm" },
	{ "_dmtext" },
	{ "_dos_mem_byte" },
	{ "_dskless_cbufs" },
	{ "_dskless_fsbufs" },
	{ "_dskless_mbufs" },
	{ "_dskless_node" },
	{ "_filesizelimit" },
	{ "_maxdsiz" },
	{ "_maxssiz" },
	{ "_maxtsiz" },
	{ "_maxuprc" },
	{ "_minswapchunks" },
	{ "_nbuf" },
	{ "_ncallout" },
	{ "_ndilbuffers" },
	{ "_netmemmax" },
	{ "_netmemthresh" },
	{ "_nfile" },
	{ "_nflocks" },
	{ "_ngcsp" },
	{ "_ninode" },
	{ "_ncdnode" },
	{ "_nproc" },
	{ "_npty" },
	{ "_ntext" },
	{ "_num_cnodes" },
	{ "_num_lan_cards" },
	{ "_parity_option" },
	{ "_retry_alive_period" },
	{ "_scroll_lines" },
	{ "_selftest_period" },
	{ "_server_node" },
	{ "_serving_array_size" },
	{ "_shmmaxaddr" },
	{ "_timeslice" },
	{ "_unlockable_mem" },
	{ "_using_array_size" },
	{ "_messages_present" },
	{ "_semaphores_present" },
	{ "_shared_memory_present" },
	{ "_nswdev" },
	{ "_swdevt" },
	{ "_freemem" },
	{ "_maxmem" },
	{ "_physmem" },
	{ 0 },
};

enum {	X_AMIGO_OPEN,
#define	FIRST_DRIVER	 X_AMIGO_OPEN
	X_CIPER_OPEN,
	X_CS80_OPEN,
	X_CDFS_LINK,
	X_DOS_OPEN,
	X_DSKLESS_LINK,
	X_DRAGON_ISR,
	X_GPIO_LINK,
	X_HPIB_OPEN,
	X_LLA_OPEN,
	X_NFS_LINK,
	X_NSDIAG0_OPEN,
	X_LP_OPEN,
	X_PTYM_OPEN,
	X_PTYS_OPEN,
	X_RAM_OPEN,
	X_RDU_READ,
	X_RFAI_LINK,
	X_RJE_OPEN,
	X_SCSI_link,
	X_SNALINK_OPEN,
	X_SRM629_OPEN,
	X_STP_OPEN,
	X_TP_OPEN,
	X_VME_OPEN,
	X_STEALTH_OPEN,
	X_TI9914_LINK,
	X_SIMON_LINK,
	X_SIO626_LINK,
	X_SIO628_LINK,
	X_SIO642_LINK,
#define	LAST_DRIVER	 X_SIO642_LINK
	X_ACCTRESUME,
#define	FIRST_TUNABLE	 X_ACCTRESUME
	X_ACCTSUSPEND,
	X_ARGDEVNBLK,
	X_CHECK_ALIVE_PERIOD,
	X_DMMAX,
	X_DMMIN,
	X_DMSHM,
	X_DMTEXT,
	X_DOS_MEM_BYTE,
	X_DSKLESS_CBUFS,
	X_DSKLESS_FSBUFS,
	X_DSKLESS_MBUFS,
	X_DSKLESS_NODE,
	X_FILESIZELIMIT,
	X_MAXDSIZ,
	X_MAXSSIZ,
	X_MAXTSIZ,
	X_MAXUPRC,
	X_MINSWAPCHUNKS,
	X_NBUF,
	X_NCALLOUT,
	X_NDILBUFFERS,
	X_NETMEMMAX,
	X_NETMEMTHRESH,
	X_NFILE,
	X_NFLOCKS,
	X_NGCSP,
	X_NINODE,
	X_NCDNODE,
	X_NPROC,
	X_NPTY,
	X_NTEXT,
	X_NUM_CNODES,
	X_NUM_LAN_CARDS,
	X_PARITY_OPTION,
	X_RETRY_ALIVE_PERIOD,
	X_SCROLL_LINES,
	X_SELFTEST_PERIOD,
	X_SERVER_NODE,
	X_SERVING_ARRAY_SIZE,
	X_SHMMAXADDR,
	X_TIMESLICE,
	X_UNLOCKABLE_MEM,
	X_USING_ARRAY_SIZE,
#define	LAST_TUNABLE	 X_USING_ARRAY_SIZE
	X_MESSAGES_PRESENT,
	X_SEMAPHORES_PRESENT,
	X_SHARED_MEMORY_PRESENT,
	X_NSWDEV,
	X_SWDEVT,
	X_FREEMEM,
	X_MAXMEM,
	X_PHYSMEM,
};

/* Swap device information */
struct swdev
{
	dev_t	sw_dev;
	int	sw_freed;
	int     sw_start;
	int	sw_nblks;
};
struct  swdev swdv;
extern	char *optarg;
char	*hpux_file = "/hp-ux";
static char BADSEEK[] = "error lseeking kmem at %x\n";
static char BADREAD[] = "error reading kmem at %x\n";

#define GETVAR(loc, var) 					\
{								\
    if (lseek(kmem, (long)(loc), SEEK_SET) == -1)		\
    {								\
	fprintf(stderr, BADSEEK, (loc));			\
	exit(1);						\
    }								\
    if (read(kmem, (char *)&(var), sizeof (var)) != sizeof (var)) \
    {								\
	fprintf(stderr, BADREAD, (loc));			\
	exit(1);						\
    }								\
}

main(argc, argv)
int argc;
char *argv[];
{
    int int_parm, i, length, kmem, nswpdev;
    int mesg_present, sema_present, shmem_present;
    int nbpg, freemem, maxmem, physmem;
    char *kmemf;
    char c;
    struct utsname myname;

    while ((c = getopt(argc, argv, "n:")) != EOF)
	switch (c)
	{
	case 'n':
	    hpux_file = optarg;
	    break;

	default:
	    fprintf(stderr, "Usage: readmem [-n kern]\n");
	    fprintf(stderr,
		"  kern  - Name of kernel file (default: /hp-ux)\n");
	    return 1;
	}

    /*
     * open the kernel memory device
     */
    kmemf = "/dev/kmem";
    if ((kmem = open(kmemf, O_RDONLY)) < 0)
    {
	perror(kmemf);
	return 1;
    }

    /*
     * fetch kernel variable addresses
     */
    if (nlist(hpux_file, nl) != 0)
    {
	fprintf(stderr, "Can't do an nlist on %s\n", hpux_file);
	return 1;
    }
    printf("Using file: %s\n", hpux_file);

    /*
     * get the driver information
     */
    printf("\nDriver information:\n");
    for (i = FIRST_DRIVER; i <= LAST_DRIVER; i++)
    {
	GETVAR(nl[i].n_value, int_parm);
	if (int_parm != 0)
	{
	    char *print_name;
	    int k;

	    length = strcspn(&nl[i].n_name[1], "_");
	    nl[i].n_name[length + 1] = NULL;

	    /*
	     * change handles to dfile names if different
	     */
	    print_name = &nl[i].n_name[1];
	    for (k = 0; xlate[k].from != NULL; k++)
	    {
		if (strcmp(&nl[i].n_name[1], xlate[k].from) == 0)
		{
		    print_name = xlate[k].to;
		    break;
		}
	    }
	    printf("\t%s is present\n", print_name);
	}
    }

    /*
     * get the tunable parameter information
     */
    printf("\n\nTunable parameter information:\n");
    for (i = FIRST_TUNABLE; i <= LAST_TUNABLE; i++)
    {
	GETVAR(nl[i].n_value, int_parm);
	printf("\t%s is %d\n", &nl[i].n_name[1], int_parm);
    }

    /*
     * see if System V code is present
     */
    printf("\n\nSystem V code (messages, semaphores & shared memory):\n");
    GETVAR(nl[(int)X_MESSAGES_PRESENT].n_value, mesg_present);
    if (mesg_present)
	printf("\tMessage code is present\n");

    GETVAR(nl[(int)X_SEMAPHORES_PRESENT].n_value, sema_present);
    if (sema_present)
	printf("\tSemaphore code is present\n");

    GETVAR(nl[(int)X_SHARED_MEMORY_PRESENT].n_value, shmem_present);
    if (shmem_present)
	printf("\tShared memory code is present\n");

    /*
     * find the swap device information
     */
    printf("\n\nSwap information: (in 512 byte blocks)\n");
    GETVAR(nl[(int)X_NSWDEV].n_value, nswpdev);

    for (i = 0; i < nswpdev; i++)
    {
	GETVAR(nl[(int)X_SWDEVT].n_value + (i * sizeof (swdv)), swdv);
	printf("\tentry%d: major is %d, ", i, major(swdv.sw_dev));
	printf("minor is 0x%x; start = %d, size = %d\n",
	    minor(swdv.sw_dev), swdv.sw_start * 2, swdv.sw_nblks * 2);
    }

    nbpg = sysconf(_SC_PAGE_SIZE);	/* Number of bytes per page */

    printf("\n\nMemory statistics:\n");

    /*
     * Bootrom uses first 8K, so round up to nearest 64K
     */
    GETVAR(nl[(int)X_PHYSMEM].n_value, physmem);
    physmem *= nbpg;
    physmem = ((physmem + 65535) / 65536) * 65536;
    printf("\tphysical memory is %d bytes\n", physmem);

    GETVAR(nl[(int)X_MAXMEM].n_value, maxmem);
    printf("\tmax available memory is %d bytes\n", maxmem * nbpg);

    GETVAR(nl[(int)X_FREEMEM].n_value, freemem);
    printf("\tfree memory (# of %d pages) is %d or %d bytes\n",
	    nbpg, freemem, freemem * nbpg);
    return 0;
}
