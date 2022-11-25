/* @(#) $Revision: 70.26 $ */
/*
 * sdsadmin(1m) --
 *   command to administer Software Disk Striping.  See sdsadmin(1M)
 *   for usage information.
 *
 * CHANGES FOR 9.03 68K HP-UX --
 *   Several changes have been made to support SDS on the Series 300/400
 *   for the 68K 9.03 release of HP-UX.  They can be broken into 2 
 *   categories, changes to support booting and rooting on an SDS volume
 *   and changes requested by the SAM team to make SAM support easier.
 *   Changes for boot and root support are under '#ifdef SDS_BOOT' and
 *   changes for SAM support are under '#ifdef SDS_NEW'.  SDS_NEW was
 *   chosen instead of SDS_SAM because, in general, these changes are
 *   useful enhancements for all users of SDS and we may wish to include
 *   these on the S700 some day.
 *
 *   Essentially 3 changes were required for boot and root support.
 *   First, a new '-b' option was added which tells sdsadmin to copy
 *   the SDS boot block (secondary loader) into the lif volume of a
 *   newly created SDS array.  The second change was to create /etc/disktab
 *   entries with swap space so the user can autoconfigure to the root
 *   device.  This allows the user too boot and root off an SDS array
 *   without having to configure a special kernel that specifies an
 *   alternate primary swap device.  Finally, a '-e' option was added
 *   to allow an SDS array to be exported to different select codes and
 *   bus addresses.  This is much like the SDS import functionality only
 *   it lets you change the configuration before you hook the array up to
 *   different hardware.  This can be necessary for boot and root devices
 *   where sds_import won't work since you can't even boot the system.
 *
 *   Three changes were made for ease of use as requested by the SAM
 *   team.  First, the '-d' option for destroying an SDS array was modified
 *   to allow specifying just the lead device of an SDS array.  Previous
 *   versions required the user to supply the '-d' option with the same
 *   list of devices used to create the array.  In the new version, if
 *   any device of an array is specified then the destroy command
 *   determines all of the devices in the array and then procedes with 
 *   the destroy as before.  The second change was to make device file names 
 *   created by sdsadmin more visible and consistant.  In previous versions
 *   select code 14 was special case'd and would different have device file
 *   names than the same disks on a different select code.  This was changed
 *   to always give the same name regardless of select code.  Also, the
 *   string 'sds_' is prepended to all device file names for clarity.  
 *   Finally, on array creation, the super block on the lead device is
 *   clobbered just in case this device was the lead device with a FS on
 *   it in a previous array wich could fool tools such as SAM into thinking
 *   that there is a FS on this newly created array when there really isn't.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <disktab.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/cs80.h>
#include <sys/scsi.h>
#include <sys/ioctl.h>
#include <sys/diskio.h>
#include <sys/dirent.h>		/* for MAXNAMLEN */
#include <sys/fs.h>
#include "sds.h"
#include "sds_user.h"

#ifdef SDS_BOOT
#include <a.out.h>
#include <volhdr.h>

#define FILENAME      	10
#define DEPB           	8
#define LIF_END_DIR     -1
#define HPUX_BOOT	-5822
#define MAX_DISKTAB_PROTOTYPES 1024
#endif /* SDS_BOOT */

/*
 * Currently, the maximum partition size is (2^32) - (3 * DEV_BSIZE).
 * We must leave slightly more than 2 DEV_BSIZE blocks at the end so
 * that the buffer cache code works properly.
 *
 * The value END_SECTION_SIZE means to use all remaining
 * space for a partition.
 */
#define MAX_SECTION_SIZE ((4.0 * 1024 * 1024 * 1024) - (3 * DEV_BSIZE))
#define END_SECTION_SIZE 0xffffffff

/*
 * We always reserve a minimum of 16k at the front of each disk
 * (single-disk arrays only, multi-disk arrays reserve much more).
 */
#define SDS_MINRESERVE  (16 * 1024)

/*
 * The space for each partition of a multi-disk array must occupy an
 * amount of space on each disk that is a multiple of this size.  This
 * is so that the array may be converted to LVM format in the future.
 */
#define SDS_PARTITION_MULT	(1024 * 1024)

/*
 * The minimum stripesize is 4k.
 * The maximum stripesize is MAXBSIZE (64k).
 */
#define SDS_MINSTRIPE   ( 4 * 1024)
#define SDS_MAXSTRIPE   MAXBSIZE

/*
 * Max size of input description file.
 */
#define SDS_MAXDESC	( 8 * 1024)  /* max size of SDS description */

/*
 * For LVM migration, we must reserve at least 8192 bytes at the end
 * of the disk for the Bad Block Relocation Area (BBRA)
 */
#define BBRA_RESERVE	(8 * 1024)

static char __warn_msg[] = "%s: line %d, Warning: ";
static char __err_msg[] = "%s: line %d, Error: ";

#define warn(prog, x, y) {			\
    fprintf(stderr, __warn_msg, prog, x);	\
    fputs(y, stderr);				\
}

#define error(prog, x, y) {			\
    fprintf(stderr, __err_msg, prog, x);	\
    fputs(y, stderr);				\
}

void usage();
void sds_create();
void sds_import();
void sds_check();
void sds_list();
#ifdef SDS_BOOT
void sds_export();
#endif /* SDS_BOOT */
void sds_destroy();
void sds_undestroy();

void check_str();
int power_of_two();
void open_dev();
dev_t block_to_raw();
void identify_dev();

#ifdef SDS_BOOT
struct disktab_entries {
	char *d_name;
	int mbytes;
};

int sds_destroy_lead = 0;
unsigned long new_minor[SDS_MAXDEVS] = { 0 } ;
int num_new_minors = 0 ;
int use_new_minor = 0 ;
#endif /* SDS_BOOT */
/*
 * A convient way to group information about our open files.
 * We use files[SDS_MAXDEVS] as a temporary file.
 */
struct file_stuff files[SDS_MAXDEVS+1];
int n_files;

/*
 * The 'bus' array is used to keep track of which busses are being
 * used.  This information is used to assign a logical bus number
 * (0..n-1) to each device.  Using the bus number, we can choose an
 * optimal ordering of the devices (for sds_create()) or warn the
 * user that the devices are not connected in an optimal order
 * (sds_import()).
 */
dev_t busses[SDS_MAXDEVS];
int nbusses = 0;

/*
 * Command line parameters.
 */
char *prog;
int force = 0;
#ifdef SDS_BOOT
int boot = 0;
#endif /* SDS_BOOT */
void (*action)() = usage;

u_long partition_size = 0;	/* -P size,	     "max" default */
u_long stripesize = 0;  	/* -S stripesize,      16K default */
char *type = (char *)0;		/* -T type,	 does have default */
char *label = (char *)0;	/* -L label,		no default */
FILE *cfg = (FILE *)0;		/* -C cfg_file,		no default */

/*
 * We set had_create_parms to 1 if -P, -S, -T, -L or -C were used.
 * These options are only valid with -m.
 */
int had_create_parms = 0;

main(argc, argv)
int argc;
char *argv[];
{
    extern int optind;
    extern char *optarg;
    extern int opterr;
    int c;
    char *device;
    int i;

    if ((prog = strrchr(argv[0], '/') + 1) == (char *)1)
	prog = argv[0];

    opterr = 0;
#ifdef SDS_BOOT
    while ((c = getopt(argc, argv, "befmic:dul:C:P:S:T:L:")) != EOF)
#else /* not SDS_BOOT */
    while ((c = getopt(argc, argv, "fmic:dul:C:P:S:T:L:")) != EOF)
#endif /* else not SDS_BOOT */
    {
	switch (c)
	{
#ifdef SDS_BOOT
	case 'b':
	    boot = 1;
	    break;

	case 'e':
	    if (action != usage)
		usage();
	    action = sds_export;
            use_new_minor = 1 ;
	    break;

#endif /* SDS_BOOT */
	case 'f':
	    force = 1;
	    break;

	case 'm':
	    if (action != usage)
		usage();
	    action = sds_create;
	    break;

	case 'i':
	    if (action != usage)
		usage();
	    action = sds_import;
	    break;

	case 'c':
	    if (action != usage)
		usage();
	    action = sds_check;
	    device = optarg;
	    break;

	case 'd':
	    if (action != usage)
		usage();
	    action = sds_destroy;
	    break;

	case 'u':
	    if (action != usage)
		usage();
	    action = sds_undestroy;
	    break;

	case 'l':
	    if (action != usage)
		usage();
	    action = sds_list;
	    device = optarg;
	    break;

	case 'C':
	    had_create_parms = 1;
	    if (cfg != (FILE *)0)
	    {
		fprintf(stderr, "%s: only one -C option allowed\n",
		    prog);
		usage();
	    }

	    if (strcmp(optarg, "-") == 0)
		cfg = stdin;
	    else
	        cfg = fopen(optarg, "r");

	    if (cfg == (FILE *)0)
	    {
		fprintf(stderr, "%s: can't open %s\n", prog, optarg);
		exit(1);
	    }
	    break;

	case 'P':
	    had_create_parms = 1;

	    if (partition_size != 0)
	    {
		fprintf(stderr, "%s: only one -P option allowed\n",
		    prog);
		usage();
	    }

	    if (strcmp(optarg, "max") == 0)
		partition_size = END_SECTION_SIZE;
	    else
	    {
		if (sds_cvt_num(optarg, &partition_size) == -1 ||
		    partition_size == 0)
		{
		    fprintf(stderr, "%s: invalid partition size: %s\n",
			prog, optarg);
		    exit(1);
		}
	    }
	    break;

	case 'S':
	    had_create_parms = 1;

	    if (stripesize != 0)
	    {
		fprintf(stderr, "%s: only one -S option allowed\n",
		    prog);
		usage();
	    }

	    if (sds_cvt_num(optarg, &stripesize) == -1 ||
		stripesize < SDS_MINSTRIPE ||
		stripesize > SDS_MAXSTRIPE ||
		!power_of_two(stripesize))
	    {
		fprintf(stderr, "%s: invalid stripesize: %s\n",
		    prog, optarg);
		exit(1);
	    }
	    break;

	case 'T':
	    had_create_parms = 1;

	    if (type != (char *)0)
	    {
		fprintf(stderr, "%s: only one -T option allowed\n",
		    prog);
		usage();
	    }

	    type = optarg;
	    check_str(type, 0, "type");
	    break;

	case 'L':
	    had_create_parms = 1;

	    if (label != (char *)0)
	    {
		fprintf(stderr, "%s: only one -L option allowed\n",
		    prog);
		usage();
	    }

	    label = optarg;
	    check_str(label, 0, "label");
	    break;

	default:
	    usage();
	}
    }

    /*
     * Do not allow the C, P, S, L and T options to be used with
     * anything other than -m.
     */
    if (had_create_parms && action != sds_create)
    {
	fprintf(stderr, "%s: -C, -P, -S, -L and -T options ", prog);
	fprintf(stderr, "may only be used with -m\n");
	usage();
    }

    /*
     * Do not allow the P, S, and T options to be used with -C.
     */
    if (cfg != (FILE *)0 &&
	(partition_size != 0 || stripesize != 0 || type != (char *)0))
    {
	fprintf(stderr, "%s: -P, -S and -T options ", prog);
	fprintf(stderr, "may not be used with -C\n");
	usage();
    }

    /*
     * Default partition size is "max"
     */
    if (partition_size == 0)
	partition_size = END_SECTION_SIZE;

    argc -= optind;
    argv += optind;

    if (argc == 1 && stripesize != 0)
    {
	fprintf(stderr, "%s: you cannot specify a stripesize ", prog);
	fprintf(stderr, "for single-disk arrays\n");
	exit(1);
    }

    /*
     * Default stripesize is 16K
     */
    if (stripesize == 0)
	stripesize = 16 * 1024;

#ifdef SDS_BOOT
    if (action == sds_export) {
	sds_export(argc, argv);
	return 0;
    }
#endif /* SDS_BOOT */

    if (action == sds_create	||
	action == sds_import	||
	action == sds_destroy	||
	action == sds_undestroy)
    {
	if (argc < 1 || argc > SDS_MAXDEVS)
	{
	    fprintf(stderr, "%s: invalid number of devices: %d\n",
		prog, argc);
	    exit(2);
	}
    }
    else
    {
	/*
	 * The default action, if none was specified is "-l".
	 */
	if (action == usage)
	{
	    if (argc != 1)
		usage();
	    action = sds_list;
	    device = argv[0];
	}
	else
	{
	    if (argc != 0)
		usage();

	    argc = 1;
	    argv[0] = device;
	}
    }

    /*
     * Open each file, we do this up front so that we know that
     * we can at least access all of the devices before we start
     * any other processing.
     */
    for (i = 0; i < argc; i++)
    {
	dev_t bus;
	int j;

	/*
	 * First open the device.  The device must be a block device,
	 * but we really open the raw device.
	 */
	files[i].name = argv[i];
#ifndef WHITE_BOX
	open_dev(i);

	/*
	 * Compute the size and disktab name of each device.
	 * We do not always require all of this information, but
	 * it is handy to have around at times, and it makes the
	 * command more testable and more maintainable if we get
	 * all the information that we might need up front.
	 */
	identify_dev(i);
#else
	/*
	 * For white box testing, we will call a routine that simulates
	 * the data for the given devices.  The 'fd' will point to
	 * a file, rather than a real device, and we will get the
	 * device parameters (dev_t, name, blksz, size) from a
	 * statically initialized table, indexed by the device file
	 * name (i.e. argv[i]).
	 */
	white_box_open(&files[i], argv[i]);
#endif /* WHITE_BOX */

	/*
	 * Find the logical bus that this device is connected to.
	 */
	bus = files[i].dev & 0xfff000;
	for (j = 0; j < nbusses; j++)
	{
	    if (bus == busses[j])
	    {
		files[i].bus = j;
		break;
	    }
	}

	/*
	 * If this was a bus that we have not seen yet, add it to
	 * our list of busses.
	 */
	if (j == nbusses)
	{
	    busses[j] = bus;
	    files[i].bus = j;
	    nbusses++;
	}
    }
    n_files = argc;

    /*
     * Verify that all of the devices are unique.  sds_create() will
     * perform extra checks to ensure that the devices all have the
     * same block size and are close to the same size.
     */
    for (i = 0; i < argc-1; i++)
    {
	int j;

	for (j = i+1; j < argc; j++)
	{
	    if (files[i].dev == files[j].dev)
	    {
		fprintf(stderr, "%s: %s and %s are the same device!\n",
		    prog, files[i].name, files[j].name);
		exit(1);
	    }
	}
    }

    /*
     * All actions except sds_list and sds_check the devices that
     * were specified on the command line must be the physical device.
     * They may not be a partition of an SDS array.
     *
     * open_dev() mapped any partitioned devices to the physical
     * devices and saved the original dev in the files[] array.  All
     * we need to do is compare the two to see if they are different.
     * If so, we were given partition devices.
     */
    if (action != sds_list && action != sds_check)
    {
	for (i = 0; i < n_files; i++)
	{
	    if (files[i].orig_dev != files[i].dev)
	    {
#ifdef SDS_NEW
		if (action == sds_destroy && n_files == 1) {
		    sds_destroy_lead = 1;
		    break;
		}
#endif /* SDS_NEW */
		fprintf(stderr,
		    "%s: %s must refer to a physical disk\n",
		    prog, files[i].name);
		exit(1);
	    }
	}
    }

    (*action)();
    return 0;
}

/*
 * usage() --
 *    Print a usage message and perform an exit(2).
 */
void
usage()
{
    static char *usages[] =
    {
#ifdef SDS_BOOT
	"    %s [-f] -m -b [-T type] [-L label] [-P size] [-S stripesize] disk...\n",
	"    %s [-f] -m -b [-L label] -C description_file disk...\n",
	"    %s -e device1 new_minor_num1 device2 new_minor_num2...\n",
	"    %s -i disk...\n",
	"    %s -c lead_device\n",
	"    %s [-l] device\n",
	"    %s [-f] -d disk...\n",
	"    %s -u disk...\n",
#else /* not SDS_BOOT */
	"\t%s [-f] -m [-T type] [-L label] [-P size] [-S stripesize] disk...\n",
	"\t%s [-f] -m [-L label] -C description_file disk...\n",
	"\t%s -i disk...\n",
	"\t%s -c lead_device\n",
	"\t%s [-l] device\n",
	"\t%s [-f] -d disk...\n",
	"\t%s -u disk...\n",
#endif /* else not SDS_BOOT */
	(char *)0
    };
    int i;

    fputs("Usage:\n", stderr);
    for (i = 0; usages[i] != (char *)0; i++)
	fprintf(stderr, usages[i], prog);

    exit(2);
}

/*
 * yes_or_no() --
 *    Prompt for a yes/no response on stdout, read answer from stdin.
 *    Returns 1 if the answer was 'y' or 'Y', 0 otherwise.
 */
int
yes_or_no()
{
    int c;
    int answer;

    printf(" (y/n)? ");
    /*
     * Read the input character, then skip everything up to
     * the newline or EOF.
     */
    answer = getchar();
    while ((c = getchar()) != '\n' && c != EOF)
	continue;

    return answer == 'y' || answer == 'Y';
}

/*
 * power_of_two() --
 *    Return 1 if a number is a power of two, zero if not.
 */
int
power_of_two(n)
u_long n;
{
    int bitsum = 0;

    while (n != 0)
    {
	bitsum += n & 0x01;
	if (bitsum > 1)
	    return 0;
	n >>= 1;
    }
    return 1;
}

/*
 * check_str() --
 *     check that "str" is valid.  Prints an error message if not.
 *     A valid string consists of the letters from the set
 *     ".-_[a-z][A-Z][0-9]", and must not be longer than SDS_MAXNAME.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
check_str(str, line, errstr)
char *str;
int line;
char *errstr;
{
    if (*str == '\0')
    {
	fprintf(stderr, "%s: Error, ", prog);
	fprintf(stderr,
	    "value for \"%s\" must contain at least 1 character\n",
	    errstr);
	exit(1);
    }

    if (strlen(str) > SDS_MAXNAME)
    {
	if (line == 0)
	    fprintf(stderr, "%s: Error, ", prog);
	else
	    error(prog, line, "");
	fprintf(stderr, "value for \"%s\" too long\n", errstr);
	exit(1);
    }

    while (*str != '\0')
    {
	if (!(*str == '-' || *str == '_' || *str == '.' ||
	      (*str >= 'a' && *str <= 'z') ||
	      (*str >= 'A' && *str <= 'Z') ||
	      (*str >= '0' && *str <= '9')))
	{
	    if (line == 0)
		fprintf(stderr, "%s: Error, ", prog);
	    else
		error(prog, line, "");
	    fprintf(stderr, "value for \"%s\" may not contain '%c'\n",
		errstr, *str);
	    exit(1);
	}
	str++;
    }
}

/*
 * block_to_raw() --
 *    Given a "block" "dev_t", translate it to the "raw" one.
 *
 *    Performs an exit(1) if an error occurs (unknown major number).
 */
dev_t
block_to_raw(dev)
dev_t dev;
{
    switch (major(dev))
    {
    case CS80_MAJ_BLK:
        return makedev(CS80_MAJ_CHR, minor(dev));

    case SCSI_MAJ_BLK:
        return makedev(SCSI_MAJ_CHR, minor(dev));

    case AC_MAJ_BLK:
        return makedev(AC_MAJ_CHR, minor(dev));

    case OPAL_MAJ_BLK:
        return makedev(OPAL_MAJ_CHR, minor(dev));

    default:
	fprintf(stderr, "%s: can't map 0x%08x to a raw device\n",
	    prog, dev);
	exit(1);
    }
    /*NOTREACHED*/
}

/*
 * raw_to_block() --
 *    Given a "raw" "dev_t", translate it to the "block" one.
 *
 *    Performs an exit(1) if an error occurs (unknown major number).
 */
dev_t
raw_to_block(dev)
dev_t dev;
{
    switch (major(dev))
    {
    case CS80_MAJ_CHR:
        return makedev(CS80_MAJ_BLK, minor(dev));

    case SCSI_MAJ_CHR:
        return makedev(SCSI_MAJ_BLK, minor(dev));

    case AC_MAJ_CHR:
        return makedev(AC_MAJ_BLK, minor(dev));

    case OPAL_MAJ_CHR:
        return makedev(OPAL_MAJ_BLK, minor(dev));

    default:
	fprintf(stderr, "%s: can't map 0x%08x to a block device\n",
	    prog, dev);
	exit(1);
    }
    /*NOTREACHED*/
}

/*
 * driver_name() --
 *    Given a "dev_t", return the "name" of the driver for that device.
 *    Returns "unknown" for unknown major numbers.
 */
char *
driver_name(dev)
dev_t dev;
{
    switch (major(dev))
    {
    case CS80_MAJ_CHR:
    case CS80_MAJ_BLK:
        return "cs80";

    case SCSI_MAJ_CHR:
    case SCSI_MAJ_BLK:
        return "scsi";

    case AC_MAJ_CHR:
    case AC_MAJ_BLK:
        return "autoch";

    default:
	return "unknown";
    }
    /*NOTREACHED*/
}

#define raw_open(x, y, z)	dev_open(S_IFCHR, x, y, z)
#define blk_open(x, y, z)	dev_open(S_IFBLK, x, y, z)

/*
 * dev_open() --
 *    given a block dev_t and a mode (i.e O_RDONLY), return an open
 *    file descriptor to the raw or block version of the device.
 *
 *    Returns -1 if the device cannot be opened.
 */
int
dev_open(dtype, dev, mode, name)
u_long dtype;
dev_t dev;
mode_t mode;
char *name; /* only for printing error messages */
{
    int fd;
    dev_t rdev;
    char *tmp_name = tempnam("/tmp", "sdsT");

    if (dtype == S_IFCHR)
	rdev = block_to_raw(dev);
    else
	rdev = dev;

    if (tmp_name == (char *)0)
    {
	fprintf(stderr, "%s: can't create temporary name for %s\n",
	    prog, name);
	return -1;
    }

    if (mknod(tmp_name, dtype, rdev) == -1)
    {
	fprintf(stderr,
	    "%s: can't create temporary device for ", prog);
	perror(name);
	unlink(tmp_name); /* to be safe */
	free(tmp_name);
	return -1;
    }

    if ((fd = open(tmp_name, mode)) == -1)
    {
	fprintf(stderr,
	    "%s: can't open temporary device for ", prog);
	perror(name);
    }

    unlink(tmp_name); /* now that it is open, we can remove it */
    free(tmp_name);
    return fd;
}

/*
 * open_dev() --
 *    Given a path to a device, open both the "block" and "raw" device.
 *    Regardless of the device given, we create a temporary device with
 *    a section number of 0 and use that instead of the one given.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
open_dev(fdi)
int fdi;
{
    struct stat st;
    dev_t dev;

    if (stat(files[fdi].name, &st) == -1)
    {
	fprintf(stderr, "%s: can't stat ", prog);
	perror(files[fdi].name);
	exit(1);
    }

    if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode))
    {
	fprintf(stderr,
	    "%s: %s must be a block or character special device file\n",
	    prog, files[fdi].name);
	exit(1);
    }

    if (S_ISCHR(st.st_mode))
	dev = raw_to_block(st.st_rdev);
    else
	dev = st.st_rdev;
    files[fdi].orig_dev = dev;

    if (major(dev) == OPAL_MAJ_BLK || major(dev) == AC_MAJ_BLK)
        files[fdi].dev = makedev(AC_MAJ_BLK, minor(dev));
    else
        files[fdi].dev = dev & ~0x0f;

    /*
     * Open the block and raw version of the device.
     */
    files[fdi].fd  = blk_open(files[fdi].dev, O_RDWR, files[fdi].name);
    if (files[fdi].fd == -1)
	exit(1);

    files[fdi].rfd = raw_open(files[fdi].dev, O_RDWR, files[fdi].name);
    if (files[fdi].rfd == -1)
	exit(1);
}

/*
 * identify_dev() --
 *     Get information about a device (files[fdi]) that we might need
 *     to perform operations on the device.  We get the device size,
 *     device block size and a name that is suitable for using to
 *     lookup the device in /etc/disktab.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
identify_dev(fdi)
int fdi;
{
    int i;
    capacity_type capacity;
    disk_describe_type describe;
    struct describe_type db;
    union inquiry_data inq;

    /*
     * Now, get the size and block size of the device
     */
    if (ioctl(files[fdi].rfd, DIOC_CAPACITY, &capacity) == -1)
    {
	fprintf(stderr, "%s: can't get size of ", prog);
	perror(files[fdi].name);
	exit(1);
    }

    if (ioctl(files[fdi].rfd, DIOC_DESCRIBE, &describe) == -1)
    {
	fprintf(stderr, "%s: can't get describe ", prog);
	perror(files[fdi].name);
	exit(1);
    }

    files[fdi].size = (capacity.lba+1) * DEV_BSIZE;
    files[fdi].blksz = describe.lgblksz;

    /*
     * Now get the name and build a "disktab" name for the device.
     * Also store the 'product id' for the device.  We use this to
     * compute a default 'type' value for the array (if needed).
     */
    switch (major(files[fdi].dev))
    {
    case CS80_MAJ_BLK: /* cs80 */
	if (ioctl(files[fdi].rfd, CIOC_DESCRIBE, &db) == -1)
	{
	    fprintf(stderr, "%s: can't CIOC_DESCRIBE ", prog);
	    perror(files[fdi].name);
	    exit(1);
	}

	sprintf(files[fdi].dname, "hp%02x%02x%02x",
	    db.unit_tag.unit.dn[0],
	    db.unit_tag.unit.dn[1],
	    db.unit_tag.unit.dn[2]);

	/*
	 * The device name is stored as six hex digits.  A disk
	 * like a 7945, will thus have a string like "079450".
	 * So, if the first character (right after 'hp') is a 0,
	 * remove it.
	 */
	if (files[fdi].dname[2] == '0')
	    strcpy(files[fdi].dname+2, files[fdi].dname+3);

	/*
	 * The last character of the name is some sort of revision
	 * level, remove it.
	 */
	files[fdi].dname[strlen(files[fdi].dname)-1] = '\0';

	/*
	 * Put just the model number (without the 'hp') into the
	 * product id field.  We use this to compute the default
	 * 'type'.
	 */
	strcpy(files[fdi].prod_id, files[fdi].dname + 2);
	break;

    case SCSI_MAJ_BLK: /* scsi */
    case AC_MAJ_BLK: /* autoch */
	if (ioctl(files[fdi].rfd, SIOC_INQUIRY, &inq) < 0)
	{
	    fprintf(stderr, "%s: can't SIOC_INQUIRY ", prog);
	    perror(files[fdi].name);
	    exit(1);
	}

	/*
	 * Remove trailing blanks from the vendor_id.
	 */
	for (i = sizeof inq.inq1.vendor_id - 1; i >= 0; i--)
	{
	    if (inq.inq1.vendor_id[i] == ' ' ||
		inq.inq1.vendor_id[i] == '\0')
	    {
		inq.inq1.vendor_id[i] = '\0';
	    }
	    else
	    {
		break;
	    }
	}

	/*
	 * Remove trailing blanks from the product_id.
	 */
	for (i = sizeof inq.inq1.product_id - 1; i >= 0; i--)
	{
	    if (inq.inq1.product_id[i] == ' ' ||
		inq.inq1.product_id[i] == '\0')
	    {
		inq.inq1.product_id[i] = '\0';
	    }
	    else
	    {
		break;
	    }
	}

	/*
	 * Put the "vendor" in first, followed by '_' and then
	 * the "product_id".
	 */
	memset(files[fdi].dname, '\0', sizeof files[fdi].dname);
	strncpy(files[fdi].dname, inq.inq1.vendor_id,
	    sizeof inq.inq1.vendor_id);
	strcat(files[fdi].dname, "_");
	strncat(files[fdi].dname, inq.inq1.product_id,
	    sizeof inq.inq1.product_id);

	/*
	 * Save the product_id for computation of a default 'type'.
	 */
	strncpy(files[fdi].prod_id, inq.inq1.product_id,
	    sizeof inq.inq1.product_id);
	break;

    default:
	fprintf(stderr, "%s: don't understand disk type of %s\n",
	    prog, files[fdi].name);
	exit(1);
    }

#ifndef SDS_BOOT
    strcat(files[fdi].dname, "_noreserve");
#endif /* ! SDS_BOOT */
}

/*
 * sds_read_one() --
 *    Read the SDS_INFO file from the given device, placing the data
 *    in 'sds'.  Prints an error message and returns -1 if an error
 *    is detected.  Otherwise returns 0.
 */
int
sds_read_one(fdi, sds)
int fdi;
sds_info_t *sds;
{
    int err;
    char buf[SDS_MAXINFO];

    if ((err = sds_get_file(files[fdi].dev, buf)) != SDS_OK)
    {
	switch (err)
	{
	case SDS_OPENFAILED:
	case SDS_NOLIF:
	    fprintf(stderr, "%s: can't read LIF header from %s\n",
		prog, files[fdi].name);
	    break;

	case SDS_NOTARRAY:
	    fprintf(stderr, "%s: %s is not a member of an array\n",
		prog, files[fdi].name);
	    break;

	default:
	    fprintf(stderr,
		"%s: unknown status (%d) from sds_get_file()\n",
		prog, err);
	    break;
	}
	return -1;
    }

    if (sds_parse_raw(sds, buf) != SDS_OK)
    {
	fprintf(stderr, "%s: %s: data in SDS_INFO is corrupt\n",
	    prog, files[fdi].name);
	return -1;
    }

    return 0;
}

/*
 * LSEEK() --
 *    an lseek() with convenient error checking.
 *
 *    Performs an exit(1) if an error occurs.
 */
static int
LSEEK(fdi, offset, whence)
int fdi;
u_long offset;
u_long whence;
{
    u_long val = lseek(files[fdi].fd, offset, whence);

    if (val == (u_long)-1)
    {
	fprintf(stderr, "%s: can't seek on ", prog);
	perror(files[fdi].name);
	exit(1);
    }
    return val;
}

/*
 * WRITE() --
 *    an write() with convenient error checking.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
WRITE(fdi, buf, size)
int fdi;
char *buf;
int size;
{
    if (write(files[fdi].fd, buf, size) != size)
    {
	fprintf(stderr, "%s: can't write to ", prog);
	perror(files[fdi].name);
	exit(1);
    }
}

/*
 * make_lif_date() --
 *    Put the current date/time into a LIF date specification.
 */
void
make_lif_date(buf)
u_char *buf;
{
    extern struct tm *localtime();
    extern long time();
    long now;
    struct tm *date;

    now = time((long *)0);
    date = localtime(&now);
    date->tm_mon++; /* 1..12, not 0..11 */

    buf[0] = (date->tm_year / 10 << 4) | (date->tm_year % 10);
    buf[1] = (date->tm_mon  / 10 << 4) | (date->tm_mon  % 10);
    buf[2] = (date->tm_mday / 10 << 4) | (date->tm_mday % 10);
    buf[3] = (date->tm_hour / 10 << 4) | (date->tm_hour % 10);
    buf[4] = (date->tm_min  / 10 << 4) | (date->tm_min  % 10);
    buf[5] = (date->tm_sec  / 10 << 4) | (date->tm_sec  % 10);
}

/*
 * create_lif_volume() --
 *    Ensure that a lif volume exists on a disk.  If a LIF volume of
 *    the correct size is already present, then do nothing.  Otherwise,
 *    we create a completey new LIF volume.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
create_lif_volume(fdi, name, size)
int fdi;
char *name;
u_long size;
{
    struct lvol lifvol;
    struct dentry lifdir[(2 * LIFSECTORSIZE) / sizeof (struct dentry)];

    (void)LSEEK(fdi, 0, SEEK_SET);
    if (read(files[fdi].fd, &lifvol, sizeof lifvol) != sizeof lifvol)
    {
	fprintf(stderr, "%s: can't read from ", prog);
	perror(files[fdi].name);
	exit(1);
    }

    /*
     * Round size down to an integral number of LIFSECTORSIZE chunks.
     */
    size &= ~(LIFSECTORSIZE-1);

    /*
     * If the existing LIF volume is already the right size, we can
     * use it as is (this is probably true because we are doing an
     * "import" of an existing array).
     */
    if (lifvol.discid == LIFID &&
	lifvol.tps * lifvol.spm * lifvol.spt * LIFSECTORSIZE == size)
	return;

    /*
     * Create a new lif volume.
     */
    lifvol.discid = LIFID;
    memset(lifvol.volname, ' ', MAXVOLNAME);
    strncpy(lifvol.volname, name, MAXVOLNAME);
    lifvol.dstart = 2;
    lifvol.dummy1 = 0x1000;	/* 1000 for system 3000 */
    lifvol.dummy2 = 0;
    lifvol.dsize = 2;		/* 2 blocks for directory */
    lifvol.version = 1;
    lifvol.dummy3 = 0;
    lifvol.tps = 1;
    lifvol.spm = 1;
    lifvol.spt = size / LIFSECTORSIZE;
    make_lif_date(lifvol.date);
    memset(lifvol.reserved1, '\0', sizeof lifvol.reserved1);
    lifvol.iplstart = 0;
    lifvol.ipllength = 0;
    lifvol.iplentry = 0;
    lifvol.reserved[0] = 0;
    lifvol.reserved[1] = 0;

    (void)LSEEK(fdi, 0, SEEK_SET);
    if (write(files[fdi].fd, &lifvol, sizeof lifvol) != sizeof lifvol)
    {
	fprintf(stderr, "%s: can't write LIF header to ", prog);
	perror(files[fdi].name);
	exit(1);
    }

    /*
     * Now write the directory entries.
     */
    memset(lifdir, '\0', sizeof lifdir);
    lifdir[0].ftype = EOD;

    (void)LSEEK(fdi, lifvol.dstart * LIFSECTORSIZE, SEEK_SET);
    if (write(files[fdi].fd, lifdir, sizeof lifdir) != sizeof lifdir)
    {
	fprintf(stderr, "%s: can't write LIF directory to ", prog);
	perror(files[fdi].name);
	exit(1);
    }
}

#ifdef SDS_BOOT
struct dentry *alloc_lifdir_entry(fdi, lifvol, lifdir, size, name)
    int fdi;
    struct lvol *lifvol;
    struct dentry *lifdir;
    u_long size;
    char *name;
{
    struct dentry *lifdirp, *lifdir_best, *lifdire;
    u_long high_water;

    /*
     * Search for an existing named file. As we go, we keep track
     * of a "high water mark" to determine where the data for a new
     * record will be stored.  We also will note if we found
     * a purged file of exactly the right size for our named file.
     * If such an entry exists, we will re-use it.
     */
    lifdirp = lifdir;
    high_water = lifvol->dstart + lifvol->dsize;
    lifdire = lifdirp + ((lifvol->dsize*LIFSECTORSIZE) / sizeof(struct dentry));
    lifdir_best = (struct dentry *)0;
    for (lifdirp = lifdir;
	    lifdirp < lifdire && lifdirp->ftype != EOD; lifdirp++)
    {
	if (lifdirp->ftype == PURGED)
	{
	    if (lifdir_best == (struct dentry *)0 &&
		lifdirp->size == size)
		lifdir_best = lifdirp;
	}
	else
	{
	    if (lifdirp->start + lifdirp->size > high_water)
		high_water = lifdirp->start + lifdirp->size;

	    if (strncmp(lifdirp->fname, name, MAXFILENAME) == 0)
	    {
		if (lifdirp->size == size)
		{
		    lifdir_best = lifdirp;
		    break;
		}

		/*
		 * If the named file exists, but it is not the right size.
		 * Purge it.
		 */
		lifdirp->ftype = PURGED;
	    }
	}
    }

    /*
     * If lifdir_best is not set, then we must allocate a new
     * directory entry.
     */
    if (lifdir_best == (struct dentry *)0)
    {
	/*
	 * We must have one more slot for this directory entry.
	 */
	if (lifdirp >= (lifdire - 1))
	{
	    fprintf(stderr, "%s: no room in LIF directory on %s\n",
		prog, files[fdi].name);
	    exit(1);
	}

	lifdirp->start = high_water;
	lifdirp->size  = size;

	if (lifdirp->start + lifdirp->size >
	    lifvol->tps * lifvol->spm * lifvol->spt)
	{
	    fprintf(stderr, "%s: no room in LIF volume on %s\n",
		prog, files[fdi].name);
	    exit(1);
	}

	/*
	 * Update the "end of directory" marker.
	 */
	(lifdirp+1)->ftype = EOD;
    }
    else
	lifdirp = lifdir_best;

    return lifdirp;
}

#endif /* SDS_BOOT */
/*
 * sds_write_one() --
 *    Write the SDS_INFO file to the given device.  The contents of
 *    the file to be written are contained in 'buf'.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
sds_write_one(fdi, buf)
int fdi;
char *buf;
{
    struct lvol lifvol;
    struct dentry *lifdir, *lifdire;
    struct dentry *lifdirp;
    struct dentry *lifdir_best;
#ifdef SDS_BOOT
    FILE *bootbfile;
    u_long high_water, nbytes;
    u_long size, start, filesize, bootstart;
    int i, dsize, loadsize;
    struct stat st;
    struct exec aout;
    time_t file_time;
    struct load lhdr;
    char buff[LIFSECTORSIZE];
#else /* ! SDS_BOOT */
    u_long high_water;
    u_long sds_info_size;
    int dsize;
#endif /* else ! SDS_BOOT */

    /*
     * Read the LIF directory from the device.
     */
    (void)LSEEK(fdi, 0, SEEK_SET);
    if (read(files[fdi].fd, &lifvol, sizeof lifvol) != sizeof lifvol)
    {
	fprintf(stderr, "%s: can't read LIF header from ", prog);
	perror(files[fdi].name);
	exit(1);
    }

    dsize = lifvol.dsize * LIFSECTORSIZE;
    lifdir = (struct dentry *)malloc(dsize);
    if (lifdir == (struct dentry *)0)
    {
	fprintf(stderr, "%s: ", prog);
	perror("can't allocate memory");
	exit(1);
    }
    lifdire = lifdir + (dsize / sizeof (struct dentry));

    (void)LSEEK(fdi, lifvol.dstart * LIFSECTORSIZE, SEEK_SET);
    if (read(files[fdi].fd, lifdir, dsize) != dsize)
    {
	fprintf(stderr, "%s: can't read LIF directory from ", prog);
	perror(files[fdi].name);
	exit(1);
    }

#ifdef SDS_BOOT
    size = SDS_MAXINFO / LIFSECTORSIZE;
    lifdirp = alloc_lifdir_entry(fdi, &lifvol, lifdir, size, SDS_INFO);
#else /* ! SDS_BOOT */
    /*
     * Search for an existing "SDS_INFO" file. As we go, we keep track
     * of a "high water mark" to determine where the data for a new
     * SDS_INFO record will be stored.  We also will note if we found
     * a purged file of exactly the right size for our SDS_INFO file.
     * If such an entry exists, we will re-use it.
     */
    sds_info_size = SDS_MAXINFO / LIFSECTORSIZE;
    high_water = lifvol.dstart + lifvol.dsize;
    lifdir_best = (struct dentry *)0;
    for (lifdirp = lifdir;
	    lifdirp < lifdire && lifdirp->ftype != EOD; lifdirp++)
    {
	if (lifdirp->ftype == PURGED)
	{
	    if (lifdir_best == (struct dentry *)0 &&
		lifdirp->size == sds_info_size)
		lifdir_best = lifdirp;
	}
	else
	{
	    if (lifdirp->start + lifdirp->size > high_water)
		high_water = lifdirp->start + lifdirp->size;

	    if (strncmp(lifdirp->fname, SDS_INFO, MAXFILENAME) == 0)
	    {
		if (lifdirp->size == sds_info_size)
		{
		    lifdir_best = lifdirp;
		    break;
		}

		/*
		 * There is an SDS_INFO, but it is not the right size.
		 * Purge it.
		 */
		lifdirp->ftype = PURGED;
	    }
	}
    }

    /*
     * If lifdir_best is not set, then we must allocate a new
     * directory entry.
     */
    if (lifdir_best == (struct dentry *)0)
    {
	/*
	 * We must have one more slot for this directory entry.
	 */
	if (lifdirp >= (lifdire - 1))
	{
	    fprintf(stderr, "%s: no room in LIF directory on %s\n",
		prog, files[fdi].name);
	    exit(1);
	}

	lifdirp->start = high_water;
	lifdirp->size  = SDS_MAXINFO / LIFSECTORSIZE;

	if (lifdirp->start + lifdirp->size >
	    lifvol.tps * lifvol.spm * lifvol.spt)
	{
	    fprintf(stderr, "%s: no room in LIF volume on %s\n",
		prog, files[fdi].name);
	    exit(1);
	}

	/*
	 * Update the "end of directory" marker.
	 */
	(lifdirp+1)->ftype = EOD;
    }
    else
	lifdirp = lifdir_best;
#endif /* else ! SDS_BOOT */

    strncpy(lifdirp->fname, SDS_INFO, MAXFILENAME);
    lifdirp->ftype = BIN;
    make_lif_date(lifdirp->date);
    lifdirp->lastvolnumber = 0x8001;
    lifdirp->extension = 0;

    /*
     * Now, write the SDS data to the disk.
     */
    LSEEK(fdi, lifdirp->start * LIFSECTORSIZE, SEEK_SET);
    if (write(files[fdi].fd, buf, SDS_MAXINFO) != SDS_MAXINFO)
    {
	fprintf(stderr, "%s: can't write SDS_INFO to ", prog);
	perror(files[fdi].name);
	exit(1);
    }
#ifdef SDS_BOOT
    if (boot && fdi == 0) {
        if ((bootbfile = fopen("/etc/sdsboot", "r")) == NULL) {
	    fprintf(stderr,"can't open /etc/sdsboot for input\n");
	    exit(1);
        }
    
        if (fstat(fileno(bootbfile), &st) == -1) {
	    fprintf(stderr,"can't stat /etc/sdsboot\n");
	    exit(1);
        }
        file_time = st.st_mtime;
    
        if (fread(&aout, sizeof(aout), 1, bootbfile) == 0) {
	    fprintf(stderr,"can't read a.out header for /etc/sdsboot\n");
	    exit(1);
        }

        if (aout.a_magic.file_type == SHARE_MAGIC) {
	    fprintf(stderr, "/etc/sdsboot cannot be shared text\n");
	    exit(2);
        }
    
        size = aout.a_text + aout.a_data;
        filesize = (size + sizeof(struct load) + LIFSECTORSIZE-1)/LIFSECTORSIZE;
    
        start = aout.a_entry;

/*	XXX  make this into a function called 3 times?   */
        lifdirp = alloc_lifdir_entry(0, &lifvol, lifdir, filesize, "SYSHPUX   ");
        bootstart = lifdirp->start;
        strncpy(lifdirp->fname, "SYSHPUX   ", MAXFILENAME);
        lifdirp->ftype = HPUX_BOOT;
        make_lif_date(lifdirp->date);
        lifdirp->lastvolnumber = 0x8001;
	lifdirp->extension = start;

        lifdirp = alloc_lifdir_entry(0, &lifvol, lifdir, filesize, "SYSBCKUP  ");
        lifdirp->start = bootstart;
        strncpy(lifdirp->fname, "SYSBCKUP  ", MAXFILENAME);
        lifdirp->ftype = HPUX_BOOT;
        make_lif_date(lifdirp->date);
        lifdirp->lastvolnumber = 0x8001;
	lifdirp->extension = start;

        lifdirp = alloc_lifdir_entry(0, &lifvol, lifdir, filesize, "SYSDEBUG  ");
        lifdirp->start = bootstart;
        strncpy(lifdirp->fname, "SYSDEBUG  ", MAXFILENAME);
        lifdirp->ftype = HPUX_BOOT;
        make_lif_date(lifdirp->date);
        lifdirp->lastvolnumber = 0x8001;
	lifdirp->extension = start;

	lhdr.address = start;
	lhdr.count = size;

        /*
         * Now, write the bootb data to the disk.
         */
        loadsize = sizeof(struct load);
        LSEEK(fdi, lifdirp->start * LIFSECTORSIZE, SEEK_SET);
        if (write(files[fdi].fd, &lhdr, loadsize) != loadsize)
        {
	    fprintf(stderr, "%s: can't write sdsboot header to ", prog);
	    perror(files[fdi].name);
	    exit(1);
        }

        fseek(bootbfile, sizeof(struct exec), SEEK_SET);

        for (i = 0 ; i < size; i += nbytes) {
	    nbytes = fread(buff, 1, LIFSECTORSIZE, bootbfile);
	    if (nbytes <= 0) {
	        fprintf(stderr,"Unexpected EOF on input\n");
	        exit(1);
	    }

	    /*
	     * If this is the last block and it is a partial block,
	     * zero pad it and write out an entire block.
	     */
	    if (i + nbytes >= size && nbytes < LIFSECTORSIZE)
	    {
	        memset(buff+nbytes, '\0', LIFSECTORSIZE - nbytes);
	        nbytes = LIFSECTORSIZE;
	    }
    
            if (write(files[fdi].fd, buff, nbytes) != nbytes)
            {
	        fprintf(stderr, "%s: can't write sdsboot to ", prog);
	        perror(files[fdi].name);
	        exit(1);
            }
        }
    }
#endif /* SDS_BOOT */

    /*
     * Now re-write the directory entries.
     */
    (void)LSEEK(fdi, lifvol.dstart * LIFSECTORSIZE, SEEK_SET);
    if (write(files[fdi].fd, lifdir, dsize) != dsize)
    {
	fprintf(stderr, "%s: can't write LIF directory to ", prog);
	perror(files[fdi].name);
	exit(1);
    }

    free(lifdir);
}

/*
 * write_sds() --
 *    Given an SDS descriptor, writes the SDS_INFO configuration data
 *    to each device of an array.
 *
 *    Performs an exit(1) if an error occurs.
 *
 * An example SDS_INFO file might look like this:
 *
 * disk       1
 * unique     0x2cec6d0c
 * device     0x000f0000
 * device     0x000e0500
 * type       7959x2-1
 * partition  1
 *    start          434176
 *    stripesize      16384
 *    size        606076928
 */
void
write_sds(sds, buf)
sds_info_t *sds;
char *buf;
{
    char *s;
    int i;
    u_long reserve;

    /*
     * Just to be tidy, we will zero out the buffer so that we do not
     * have garbage after our valid data.
     */
    memset(buf, '\0', SDS_MAXINFO);

    /*
     * Create a "template" of the SDS data to be written to each
     * disk.  The first line of the data will contain:
     *
     *	   "disk      NN\n"
     *
     * Where "NN" is right justified with spaces. Thus, the first
     * line will always be 13 characters long (including the "\n").
     */
    if (sds->ndevs == 1)
	s = buf;
    else
    {
	s = buf + 13;
	s += sprintf(s, "unique     0x%08x\n", sds->unique);
	for (i = 0; i < sds->ndevs; i++)
	    s += sprintf(s, "device     0x%08x\n", sds->devs[i]);
    }

    /*
     * Print the label (if any) and type.
     */
    if (sds->label[0] != '\0')
	s += sprintf(s, "label      %s\n", sds->label);
    s += sprintf(s, "type       %s\n", sds->type);

    /*
     * Print the partition information.  Only print the stripesize
     * on multi-disk arrays.
     */
    reserve = 0xffffffff;
    for (i = 0; i < SDS_MAXSECTS; i++)
    {
	if (sds->section[i].size != 0)
	{
	    s += sprintf(s, "partition %2d\n", i + 1);
	    s += sprintf(s, "    start      %10lu\n",
		sds->section[i].start);
	    if (sds->ndevs > 1)
		s += sprintf(s, "    stripesize %10lu\n",
		    sds->section[i].stripesize);
	    s += sprintf(s, "    size       %10lu\n",
		sds->section[i].size);

	    /*
	     * Re-compute the amount of reserved space.  We need this
	     * to size the LIF volume.
	     */
	    if (sds->section[i].start < reserve)
		reserve = sds->section[i].start;
	}
    }

    /*
     * Create a LIF volume on each member of the array.  We do
     * this before writing *any* of the SDS_INFO files, so that
     * if anything fails here, we have not made bogus entries on
     * any disks.  The name of the LIF volume will be the same as
     * the label of the array (unless the array has no label, in
     * which case we use the type instead).
     *
     * If the disk already has a LIF volume on it, then this is
     * a no-op.
     */
    if (sds->label[0] != '\0')
	s = sds->label;
    else
	s = sds->type;

    for (i = 0; i < n_files; i++)
	create_lif_volume(i, s, reserve);

    /*
     * Now create the LIF file on each member of the array.
     * sds_write_one() will print error messages and exit if an
     * an error is encounted while trying to write the SDS_INFO file.
     *
     * We do not put the "disk NN" on the disk of a single-disk array.
     */
    if (sds->ndevs == 1)
    {
	sds_write_one(0, buf);
	fsync(files[0].fd);
    }
    else
    {
	for (i = 0; i < n_files; i++)
	{
	    sprintf(buf, "disk      %2d", i+1);
	    buf[12] = '\n';

	    sds_write_one(i, buf);
	    fsync(files[i].fd);
	}
    }
}

/*
 * optimal_busses() --
 *    Return 1 if the devices in the files[] array are configured in
 *    an 'optimal' configuration.  Return 0 if not.
 */
int
optimal_busses()
{
    int i, j;

    /*
     * Go through the devices, in groups of nbusses.  The only
     * optimal configuration is 1, 2, 3...n-1, 1, 2, 3...n-1.
     */
    for (i = 0; i < n_files; i += nbusses)
	for (j = 0; j < nbusses; j++)
	    if (files[i+j].bus != j)
		return 0;

    return 1;
}

/*
 * make_optimal_busses() --
 *     Re-order the files[] array in an attempt to create an optimal
 *     bus configuration.  We want the disks to be in the order:
 *     1, 2, 3...nbusses-1, 1, 2, 3...nbusses-1.  We may not be able
 *     to do this, the devices must be evenly distributed among the
 *     available busses for this to work.
 */
void
make_optimal_busses()
{
    int i, j, k;

    for (i = 0; i < n_files; i += nbusses)
    {
	for (j = 0; j < nbusses; j++)
	{
	    if (files[i+j].bus != j)
	    {
		/*
		 * Try to find a device with the right bus number.
		 * If one is found, swap that one with the current
		 * device.
		 */
		for (k = i+j+1; k < n_files; k++)
		{
		    if (files[k].bus == j)
		    {
			/*
			 * Found a device on the desired bus, swap
			 * it with the current device.
			 */
			files[TMP_FILE] = files[i+j];
			files[i+j] = files[k];
			files[k] = files[TMP_FILE];
			break;
		    }
		}
	    }
	}
    }
}
#ifdef SDS_BOOT
/*	XXX  need comment  */

update_disktab(sds)
sds_info_t *sds;
{
    struct disktab_entries *dt_entries[MAX_DISKTAB_PROTOTYPES];
    int disktab_noswap_done = 0;
    int n_parts = 0;
    int i, count;
    void sds_add_disktab();

    /*
     * Check to see if disktab entries already exist for 
     * this array and if so just print them out and return.  
     */
    if (count = get_disktab_entries(sds->type, dt_entries)) {
        printf("%s: disktab name(s) of this array:\n", prog);
	for (i = 0; i < count; i++)
            printf("\t%s\n", dt_entries[i]->d_name);
	return;
    }

    /*
     * No entries currently exist so find all of 
     * the entries that we can use as prototypes.
     * If none are found just return.
     */
    if ((count = get_disktab_entries(files[0].dname, dt_entries)) == 0)
	return;

    /*
     * Count up the number of partitions.
     */
    for (i = 0; i < SDS_MAXSECTS; i++)
        if (sds->section[i].size != 0)
            n_parts++;

    /*
     * For each prototype found create a new entry
     * in disktab that describes this array.
     */
    printf("%s: disktab name(s) of this array:\n", prog);
    for (i = 0; i < count; i++) {
	/*
	 * If we have already created a noswap entry then skip
	 * any subsequent noswap prototypes.  Also ignore any
	 * prototypes that have swap space associated with them
	 * if this is a multiple partition array.
	 */
        if (((dt_entries[i]->mbytes == 0) && disktab_noswap_done) ||
            ((n_parts > 1) && (dt_entries[i]->mbytes != 0)))
                continue;

	/* Add the disktab entry */
        strcpy(files[0].dname, dt_entries[i]->d_name);
        sds_add_disktab(sds, dt_entries[i]->mbytes * sds->ndevs);

	/* Let the user know about this entry */
        if (dt_entries[i]->mbytes)
            printf("\tHP_%s_%dMB\n", sds->type, dt_entries[i]->mbytes*sds->ndevs);
        else
            printf("\tHP_%s_noreserve\n", sds->type);

	/* Did we create a noswap entry? */
        if (dt_entries[i]->mbytes == 0)
                disktab_noswap_done = 1;
    }
}

get_disktab_entries(target, disktab_entries)
char *target;
struct disktab_entries *disktab_entries[];
{
    FILE *pp;
    char buf[1024];
    struct disktab *dp;
    int nfound = 0;

    sprintf(buf, "grep '%s[:_|]' /etc/disktab | 			\
	          awk -F'|' '						\
		  {							\
		      for (i = 1; i <= NF; i++) 			\
			  if ($i ~ \"%s\") {				\
			      x = index($i, \":\"); 			\
			      if (x > 0) 				\
				  x = length($i) - x + 1; 		\
			      print substr($i, 1, length($i)-x); 	\
			      break					\
			  }						\
		  }'", target, target);


    pp = popen(buf, "r");
    if (pp == NULL) 
        return(0);

    while (fgets(buf, 1024, pp) != NULL) {
        buf[strlen(buf)-1] = '\0';
        dp = getdiskbyname(buf);

        if (dp == NULL) 
            continue;

        disktab_entries[nfound] = (struct disktab_entries *)
					malloc(sizeof(struct disktab_entries)); 
        disktab_entries[nfound]->d_name = (char *)malloc(strlen(dp->d_name)+1); 

        strcpy(disktab_entries[nfound]->d_name, dp->d_name);
        disktab_entries[nfound]->mbytes = 
	    (files[0].size -  (dp->d_partitions[0].p_size * 1024)) / 0x100000;

        nfound++;
    }		

    pclose(pp);
    return nfound;
}

#endif /* SDS_BOOT */

/*
 * The disktab(4) data structure uses "short"s to hold block and
 * fragment sizes.  Thus, we cannot use any block or fragment sizes
 * larger than 16384 (the highest power of two that will fit in a
 * "short".
 */
#define MAX_DISKTAB_BLKSZ 16384

/*
 * generate_disktab() --
 *    Given an SDS descriptor, fill in and return a disktab structure
 *    that describes the SDS array.  The disktab structure is returned
 *    in statically allocated space.
 *
 *    Performs an exit(1) if an error occurs.
 */
struct disktab *
#ifdef SDS_BOOT
generate_disktab(sds, swap)
sds_info_t *sds;
int swap;
#else /* ! SDS_BOOT */
generate_disktab(sds)
sds_info_t *sds;
#endif /* else ! SDS_BOOT */
{
    extern struct disktab *getdiskbyname();
    static struct disktab dt;
    static char name_buf[3 * (SDS_MAXNAME + 4) +
			 sizeof "_noreserve" +
			 sizeof "_noswap" + 1];
    struct disktab *dtab;
    int i;

    if ((dtab = getdiskbyname(files[0].dname)) == (struct disktab *)0)
    {
	fprintf(stderr, "%s: can't find disktab entry for %s\n",
	    prog, files[0].dname);
	exit(1);
    }

#ifdef SDS_BOOT
    if (swap)
        sprintf(name_buf, "HP_%s_%dMB", sds->type, swap);
    else
        sprintf(name_buf, "HP_%s_noreserve|HP_%s_noswap|HP_%s",
            sds->type, sds->type, sds->type);
#else /* ! SDS_BOOT */
    sprintf(name_buf, "HP_%s|HP_%s_noreserve|HP_%s_noswap",
	sds->type, sds->type, sds->type);
#endif /* else ! SDS_BOOT */

    /*
     * Initialize dt.
     */
    dt.d_type = (char *)0;
    memset(dt.d_partitions, (char)-1, sizeof dt.d_partitions);

    dt.d_name	    = name_buf;
    dt.d_secsize    = dtab->d_secsize;
    dt.d_rpm	    = dtab->d_rpm;
    dt.d_ntracks    = dtab->d_ntracks;
    dt.d_nsectors   = dtab->d_nsectors * sds->ndevs;
    dt.d_ncylinders = dtab->d_ncylinders;

    for (i = 0; i < SDS_MAXSECTS; i++)
    {
	int bsize, fsize;

	if (sds->section[i].size == 0)
	    continue;

	if (sds->ndevs == 1)
	{
	    bsize = 8192;
	    fsize = 1024;
	}
	else
	{
	    int ratio;

	    /*
	     * Pick the primary block size.  Must be at least
	     * DEV_BSIZE and no greater than MAX_DISKTAB_BLKSZ.
	     */
	    bsize = sds->section[i].stripesize;
	    if (bsize < DEV_BSIZE)
		bsize = DEV_BSIZE;

	    if (bsize > MAX_DISKTAB_BLKSZ)
	    {
		fprintf(stderr, "%s: Warning, requested stripesize ",
		    prog);
		fprintf(stderr, "for partition %d is too large to be\n",
		    i+1);
		fprintf(stderr, "\t  represented in %s.\n",
		    DISKTAB);
		fprintf(stderr, "\t  Be sure to use the -b and -f ");
		fprintf(stderr, "options of newfs(1M) if you create\n");
		fprintf(stderr, "\t  a filesystem on partition %d.\n",
		    i+1);
		bsize = MAX_DISKTAB_BLKSZ;
	    }

	    /*
	     * Pick a fragment size.  Typically we want this to be
	     * 1/8th of the primary block size, but it cannot be
	     * any smaller than max(DEV_BSIZE, disk_sector_size).
	     */
	    fsize = bsize / 8;
	    if (fsize < dtab->d_secsize)
		fsize = dtab->d_secsize;
	    if (fsize < DEV_BSIZE)
		fsize = DEV_BSIZE;

	    /*
	     * Now make sure that the ratio between fragment and
	     * block size is 8,4,2 or 1:1.  If it is not already
	     * a valid ratio, pick the next highest valid ratio.
	     */
	    ratio = bsize / fsize;
	    if (ratio > 8)
		ratio = 8;
	    while (ratio != 8 && ratio != 4 && ratio != 2 && ratio != 1)
		ratio++;

	    /*
	     * Now that we have the fragment size and the ratio,
	     * recompute the primary block size.  If it is too big,
	     * reduce the ratio until the primary block size is
	     * small enough.
	     */
	    bsize = fsize * ratio;
	    while (bsize > MAX_DISKTAB_BLKSZ)
	    {
		ratio /= 2;
		bsize = fsize * ratio;
	    }
	}

	/*
	 * Initialize the disktab fields for this partition.  Remember,
	 * for disktab, the partitions are numbered 1..8, not 0..7
	 */
#ifdef SDS_BOOT
	if (swap)
	    dt.d_partitions[i+1].p_size = 
			dt.d_nsectors * dt.d_ntracks * dt.d_ncylinders;
	else
		dt.d_partitions[i+1].p_size = sds->section[i].size / DEV_BSIZE;
#else /* ! SDS_BOOT */
	dt.d_partitions[i+1].p_size = sds->section[i].size / DEV_BSIZE;
#endif /* else ! SDS_BOOT */
	dt.d_partitions[i+1].p_bsize = bsize;
	dt.d_partitions[i+1].p_fsize = fsize;
    }

    return &dt;
}

/*
 * sds_add_disktab() --
 *    Given an SDS descriptor, ensure that there is a vaild disktab
 *    entry for the array in /etc/disktab.  If there is a conflicting
 *    entry, print an error message.  If a valid entry already exists,
 *    do nothing and return.  Otherwise, a new disktab entry is
 *    appended to /etc/disktab.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
#ifdef SDS_BOOT
sds_add_disktab(sds, swap)
sds_info_t *sds;
int swap;
#else /* ! SDS_BOOT */
sds_add_disktab(sds)
sds_info_t *sds;
#endif /* else ! SDS_BOOT */
{
    extern struct disktab *getdiskbyname();
    int i;
    struct disktab *dtab;
    struct disktab *dtab2;
    FILE *fp;
    char dname[SDS_MAXNAME + 4];

    /*
     * Call generate_disktab(0) to fill in a valid disktab structure
     * for the lead device.  This is used as the 'base' disktab entry.
     */
#ifdef SDS_BOOT
    dtab = generate_disktab(sds, swap);
#else /* ! SDS_BOOT */
    dtab = generate_disktab(sds);
#endif /* else ! SDS_BOOT */

    /*
     * Check to see if an entry for this array already exists in
     * /etc/disktab.
     */
#ifdef SDS_BOOT
    if (swap)
        sprintf(dname, "%s_%dMB", sds->type, swap);
    else
        sprintf(dname, "%s", sds->type);
#else /* ! SDS_BOOT */
    sprintf(dname, "HP_%s", sds->type);
#endif /* else ! SDS_BOOT */
    dtab2 = getdiskbyname(dname);
    if (dtab2 != (struct disktab *)0)
    {
	/*
	 * An entry already exists.  If it is the same, then we are
	 * fine.  Otherwise, generate an error message and exit.
	 *
	 * We compare all fields except d_name and d_type.  Since we
	 * were able to find this entry with the default name, we know
	 * that d_name is ok, we do not compare it in case the user
	 * has added extra names to d_name manually.  d_type is used
	 * as a comment in the disktab entry.  When we generate a
	 * disktab entry, we do not put anything in d_type.  However,
	 * the user may have added d_type manually, so again we do not
	 * compare this field.
	 */
	if (dtab->d_secsize	!= dtab2->d_secsize	||
	    dtab->d_ntracks	!= dtab2->d_ntracks	||
	    dtab->d_nsectors	!= dtab2->d_nsectors	||
	    dtab->d_ncylinders	!= dtab2->d_ncylinders	||
	    dtab->d_rpm		!= dtab2->d_rpm		||
	    memcmp(dtab->d_partitions, dtab2->d_partitions,
		   sizeof dtab->d_partitions) != 0)
	{
	    fprintf(stderr, "%s: Error, a disktab entry for ", prog);
	    fprintf(stderr, "%s already exists in %s\n",
		dname, DISKTAB);
	    fprintf(stderr, "\tthat does not match the configuration ");
	    fprintf(stderr, "you just specified.\n");
	    exit(1);
	}

	/*
	 * The entries were the same, there is no need to do
	 * anything to /etc/disktab.
	 */
	return;
    }

    /*
     * Append the new entry to /etc/disktab.  This is safe, since we
     * have already verified that there is no conflicting or
     * duplicate entry.
     */
    if ((fp = fopen(DISKTAB, "a")) == NULL)
    {
	fprintf(stderr, "%s: can't append to ", prog);
	perror(DISKTAB);
	exit(1);
    }

    /*
     * Now print the structure to the end of /etc/disktab.
     */
    fprintf(fp, "%s:\\\n", dtab->d_name);
#ifdef SDS_BOOT
    if (swap) {
        fprintf(fp, 
            "\t:%d MByte reserved for swap & boot:ns#%lu:nt#%lu:nc#%lu:\\\n",
            swap,
	    dtab->d_nsectors,
	    dtab->d_ntracks,
	    dtab->d_ncylinders);
    } else {
        fprintf(fp, "\t:No swap or boot:ns#%lu:nt#%lu:nc#%lu:\\\n",
	    dtab->d_nsectors,
	    dtab->d_ntracks,
	    dtab->d_ncylinders);
    }
#else /* ! SDS_BOOT */
    fprintf(fp, "\t:ns#%lu:nt#%lu:nc#%lu:\\\n",
	dtab->d_nsectors,
	dtab->d_ntracks,
	dtab->d_ncylinders);
#endif /* ! SDS_BOOT */

    for (i = 0; i < NSECTIONS; i++)
    {
	if (dtab->d_partitions[i].p_size != -1)
	{
	    fprintf(fp, "\t:s%d#%lu:b%d#%d:f%d#%d:\\\n",
		i, dtab->d_partitions[i].p_size,
		i, dtab->d_partitions[i].p_bsize,
		i, dtab->d_partitions[i].p_fsize);
	}
    }
    fprintf(fp, "\t:se#%d:rm#%d:\n", dtab->d_secsize, dtab->d_rpm);

    if (fclose(fp) != 0)
    {
	fprintf(stderr, "%s: can't close ", prog);
	perror(DISKTAB);
	exit(1);
    }
}

/*
 * device_name() --
 *    Given a dev_t for a device, create the name that we would
 *    use for that device file.  The name is returned in a static
 *    buffer, and does not include the "/dev/dsk" or "/dev/rdsk"
 *    portion.
 */
char *
device_name(dev, str)
dev_t dev;
char *str;
{
    static char name[MAXNAMLEN];
    int disk_addr = m_busaddr(dev);
    int partition = m_volume(dev);
    int maj_num   = major(dev);
    int surface   = (int)(dev & 0x000001ff);
    int len;

#if !defined(SDS_NEW) || defined(__hp9000s700)
    /*
     * First, we see if the string ends in "s<n>", where <n> is a digit
     * from 0..8.  If so, we simply replace the <n> with the correct
     * partition number (the partition number is 1..8, so it is always
     * a single character).
     */
    len = strlen(str);
    if (str[len-2] == 's' && str[len-1] >= '0' && str[len-1] <= '8')
    {
	char *s;

	/*
	 * Copy the basename of the string to 'name'.
	 */
	if ((s = strrchr(str, '/') + 1) == (char *)1)
	    s = str;
	strcpy(name, s);

	len = strlen(name);
        if (maj_num == OPAL_MAJ_BLK || maj_num == OPAL_MAJ_CHR)
	    name[len-1] = '1';
	else
	    name[len-1] = partition + '0';
	return name;
    }
#endif /* !defined(SDS_NEW) || defined(__hp9000s700) */

    /*
     * They have named their devices with some unknown naming
     * convention.  Instead of munging their name, we create our
     * own, using our naming convention.
     */
    dev &= 0x00fff000;
    switch (dev)
    {
#ifdef __hp9000s300
    /*
     * Just one convention for s300/s400
     */
    default:
#else
    case 0x201000:
#endif /* s300 vs. s700 */
	/*
	 * Internal bus, naming convention is <disk_addr>s<partition>
	 */
        if (maj_num == OPAL_MAJ_BLK || maj_num == OPAL_MAJ_CHR)
	    sprintf(name, "%d%cs%d", (surface + 1) >> 1,
                surface & 1 ? 'a' : 'b', 1);
        else
	{
#ifdef __hp9000s300
#ifdef SDS_NEW
	    sprintf(name, "sds_c%02dd%ds%d",
		(dev >> 16) & 0xff, disk_addr, partition);
#else /* ! SDS_NEW */
	    if (((dev >> 16) & 0xff) != 0x0e)
		sprintf(name, "c%02dd%ds%d",
		    (dev >> 16) & 0xff, disk_addr, partition);
	    else
#endif /* else ! SDS_NEW */
#endif /* s300 */
#ifndef SDS_NEW
	    sprintf(name, "%ds%d", disk_addr, partition);
#endif /* ! SDS_NEW */
	}
	break;

#ifdef __hp9000s700
    case 0x410000:
    case 0x420000:
    case 0x430000:
    case 0x440000:
	/*
	 * Single function EISA card in slots 1, 2, 3 or 4.  Naming
	 * convention is c4[1234]d<disk_addr>s<partition>.
	 */
        if (maj_num == OPAL_MAJ_BLK || maj_num == OPAL_MAJ_CHR)
	    sprintf(name, "c4%dd%d%cs%d",
	        m_slot(dev), (surface +1 ) >> 1, surface & 1 ? 'a' : 'b', 1);
        else
	    sprintf(name, "c4%dd%ds%d", m_slot(dev), disk_addr, partition);
	break;

    default:
	/*
	 * Future CORE I/O cards, multi-function EISA cards, etc.
	 * Naming convention is c<XXX>d<disk_addr>s<partition>.
	 * <XXX> is the high 3 nibbles of minor(dev), in hex.
	 */
	sprintf(name, "c%03xd%ds%d", (dev >> 12), disk_addr, partition);
	break;
#endif /* s700 */
    }

    return name;
}

/*
 * create_devices() --
 *    Given an SDS descriptor, create the necessary device files
 *    in /dev/dsk and /dev/rdsk for each partition of the array.
 *
 *    If any device files already exist and are wrong, a warning
 *    message is issued.
 *    If any device files already exists and are correct, they are
 *    left untouched.
 *    Otherwise, the new device file is created.
 */
void
create_devices(sds)
sds_info_t *sds;
{
    int i;
    int j;
    dev_t dev;
    u_long dtype;
    char *dir;
    char *str;
    char *name;
    char d_name[sizeof "/dev/rdsk/" + MAXNAMLEN];
    struct stat st;

    /*
     * Make sure that the umask is 0, so that we get the right
     * permissions on the device file.  It is safe to change the
     * umask, as we will not be creating any more files other than
     * the device files created here.
     */
    (void)umask(0);

    /*
     * If necessary, create the /dev/opal and /dev/ropal directories.
     */
    if (major(files[0].dev) == AC_MAJ_BLK)
    {
	static char *ac_dirs[] = { "/dev/opal", "/dev/ropal" };

	for (i = 0; i < 2; i++)
	{
	    if (stat(ac_dirs[i], &st) == -1)
	    {
		if (errno != ENOENT)
		{
		    fprintf(stderr, "%s: can't stat ");
		    perror(ac_dirs[i]);
		    exit(1);
		}
		if (mkdir(ac_dirs[i], 0750) == -1)
		{
		    fprintf(stderr, "%s: can't create ");
		    perror(ac_dirs[i]);
		    exit(1);
		}
		(void)chown(ac_dirs[i], 0, 3); /* root, sys */
	    }
        }
    }

    /*
     * Now create the block and character special device files.
     * We do the block devices first (i == 0), and then the
     * character devices (i == 1).
     */
    for (i = 0; i < 2; i++)
    {
	if (i == 0)
	{
	    dtype = S_IFBLK;
            if (major(files[0].dev) == AC_MAJ_BLK)
	        dir = "/dev/opal";
            else
	        dir = "/dev/dsk";
	    str = "block";
	}
	else
	{
	    dtype = S_IFCHR;
            if (major(files[0].dev) == AC_MAJ_BLK)
	        dir = "/dev/ropal";
            else
	        dir = "/dev/rdsk";
	    str = "character";
	}

	for (j = 0; j < SDS_MAXSECTS; j++)
	{
	    if (sds->section[j].size == 0)
		continue;

	    if (i == 0)     /* block devs when 0, char devs when 1 */
		dev = files[0].dev;
	    else
		dev = block_to_raw(files[0].dev);

            if (major(dev) == AC_MAJ_BLK)
                dev = makedev(OPAL_MAJ_BLK, minor(dev));
            else if (major(dev) == AC_MAJ_CHR)
                dev = makedev(OPAL_MAJ_CHR, minor(dev));
            else
	        dev = (dev & ~0x0f) | (j + 1);

	    name = device_name(dev, files[0].name);
	    sprintf(d_name, "%s/%s", dir, name);

	    /*
	     * If the file does not exist, create it.
	     */
	    if (stat(d_name, &st) == -1)
	    {
		if (errno != ENOENT)
		{
		    fprintf(stderr, "%s: can't stat ");
		    perror(d_name);
		    exit(1);
		}

		if (mknod(d_name, dtype | 0640, dev) == -1)
		{
		    fprintf(stderr, "%s: can't create ");
		    perror(d_name);
		    exit(1);
		}
		(void)chown(d_name, 0, 3); /* root, sys */
	    }
	    else
	    {
		/*
		 * The file does exist.  If it is not the right dev_t,
		 * generate a warning.
		 */
		if (dev != st.st_rdev || (st.st_mode & S_IFMT) != dtype)
		{
		    fprintf(stderr, "%s: Warning %s exists, but is ",
			prog, d_name);
		    fprintf(stderr, "not correct for partition %d\n",
			j + 1);
		    fprintf(stderr, "\t%s should have major number %d",
			d_name, major(dev));
		    fprintf(stderr, " and minor number 0x%06x\n",
			minor(dev));
		    continue;
		}
	    }

	    printf("%s: %s device file for partition %d is %s\n",
		prog, str, j + 1, d_name);
	}
    }
}

/*
 * sds_parse() --
 *     parses an SDS description file, filling in 'sds'.  If an
 *     error is encounted, we simply call exit(1).
 *
 *    Performs an exit(1) if an error occurs.
 */
void
sds_parse(sds, blksz, buf)
sds_info_t *sds;
u_long blksz;
char *buf;
{
    int line = 0;
    char *str;
    int partition = 0;
    int partition_cnt;
    int kw;
    u_long minstripe = MAX(blksz, SDS_MINSTRIPE);

    /*
     * Initialize some fields in the sds structure.  We do not touch
     * the devs[] or ndevs fields.
     */
    memset(sds->label, '\0', sizeof sds->label);
    memset(sds->type, '\0', sizeof sds->type);
    memset(sds->section, '\0', sizeof sds->section);

    /*
     * Now parse the file.  Everything in the description file is
     * in the form of a "keyword value" pair.
     */
    while ((kw = sds_keyword(&buf, &line, &str)) != SDS_KW_EOF)
    {
	char *word;

	if (kw == SDS_KW_ERROR)
	{
	    fprintf(stderr,
		"%s: line %d, Error: unknown keyword \"%s\"\n",
		prog, line, str);
	    exit(1);
	}

	word = sds_word(&buf, &line);
	if (*word == '\0')
	{
	    error(prog, line, "missing value for ");
	    fprintf(stderr, "\"%s\" specification\n", str);
	    exit(1);
	}

	switch (kw)
	{
	case SDS_KW_LABEL:
	    if (sds->label[0] != '\0')
	    {
		error(prog, line,
		    "too many \"label\" specifications\n");
		exit(1);
	    }

	    check_str(word, line, "label");
	    strcpy(sds->label, word);
	    break;

	case SDS_KW_TYPE:
	    if (sds->type[0] != '\0')
	    {
		error(prog, line, "too many \"type\" specifications\n");
		exit(1);
	    }

	    check_str(word, line, "type");
	    strcpy(sds->type, word);
	    break;

	case SDS_KW_SECTION:
	    if (sds_cvt_num(word, &partition) == -1 ||
		partition == 0 || partition > SDS_MAXSECTS)
	    {
		error(prog, line, "partition number must be from ");
		fprintf(stderr, "1 to %d\n", SDS_MAXSECTS);
		exit(1);
	    }
	    --partition; /* internally, partition is 0..7, not 1..8 */

	    if (sds->section[partition].size != 0)
	    {
		error(prog, line, "duplicate partition number");
		fprintf(stderr, " %d\n", partition + 1);
		exit(1);
	    }

	    /*
	     * So that we can detect if they said "partition N", but
	     * did not specify a size for that partition, we mark that
	     * we have seen a "partition N" by setting the size to
	     * an illegal, non-zero value.
	     */
	    sds->section[partition].size = 1;
	    break;

	case SDS_KW_STRIPE:
	    if (sds->section[partition].size == 0)
	    {
		error(prog, line, "\"stripesize\" must be preceded ");
		fprintf(stderr, "by a \"partition\" specification\n");
		exit(1);
	    }

	    if (sds->section[partition].stripesize != 0)
	    {
		error(prog, line,
		    "too many \"stripesize\" specifications\n");
		exit(1);
	    }

	    if (sds_cvt_num(word, &sds->section[partition].stripesize) == -1)
	    {
		error(prog, line, "invalid \"stripesize\": ");
		fprintf(stderr, "%s\n", word);
		exit(1);
	    }

	    if (sds->section[partition].stripesize < minstripe)
	    {
		error(prog, line, "\"stripesize\" must be >= ");
		fprintf(stderr, "%d\n", minstripe);
		exit(1);
	    }

	    if (sds->section[partition].stripesize > SDS_MAXSTRIPE)
	    {
		error(prog, line, "\"stripesize\" must be <= ");
		fprintf(stderr, "%d bytes\n", SDS_MAXSTRIPE);
		exit(1);
	    }

	    if (!power_of_two(sds->section[partition].stripesize))
	    {
		error(prog, line,
		    "\"stripesize\" must be a power of two\n");
		exit(1);
	    }

	    if (sds->ndevs == 1)
	    {
		error(prog, line, "you cannot specify a stripesize ");
		fprintf(stderr, "for single-disk arrays\n");
		exit(1);
	    }
	    break;

	case SDS_KW_SIZE:
	    if (sds->section[partition].size == 0)
	    {
		error(prog, line, "\"size\" must be preceded by a ");
		fprintf(stderr, "\"partition\" specification\n");
		exit(1);
	    }

	    if (sds->section[partition].size != 1)
	    {
		error(prog, line, "too many \"size\" specifications\n");
		exit(1);
	    }

	    if (strcmp(word, "max") == 0)
		sds->section[partition].size = (u_long)END_SECTION_SIZE;
	    else if (sds_cvt_num(word, &sds->section[partition].size) == -1 ||
		     sds->section[partition].size == END_SECTION_SIZE)
	    {
		error(prog, line, "invalid \"size\": ");
		fprintf(stderr, "%s\n", word);
		exit(1);
	    }

	    if (sds->section[partition].size < minstripe)
	    {
		error(prog, line, "\"size\" must be >= ");
		fprintf(stderr, "%d bytes\n", minstripe);
		exit(1);
	    }
	    break;

	case SDS_KW_RESERVE:
	    if (sds->reserve != 0)
	    {
		error(prog, line,
		    "too many \"reserve\" specifications\n");
		exit(1);
	    }

	    if (sds_cvt_num(word, &sds->reserve) == -1)
	    {
		error(prog, line, "invalid \"reserve\": ");
		fprintf(stderr, "%s\n", word);
		exit(1);
	    }
	    break;

	default:
	    fprintf(stderr,
		"%s: line %d, Error: unknown keyword #%d\n",
		prog, line, kw);
	    exit(1);
	}
    }

    /*
     * Count the number of partitions, verify that there is at least
     * one.  For each valid partition, check that a size and stripesize
     * (if necessary) was given.
     */
    partition_cnt = 0;
    for (partition = 0; partition < SDS_MAXSECTS; partition++)
    {
	if (sds->section[partition].size == 0)
	    continue;

	partition_cnt++;
	if (sds->section[partition].size == 1)
	{
	    fprintf(stderr, "%s: Error, no \"size\" ", prog);
	    fprintf(stderr, "specified for partition %d\n",
		partition + 1);
	    exit(1);
	}

	/*
	 * The stripesize defaults to the global 'stripesize'.
	 */
	if (sds->section[partition].stripesize == 0)
	    sds->section[partition].stripesize = stripesize;
    }

    if (partition_cnt == 0)
    {
	fprintf(stderr, "%s: Error, array must have ", prog);
	fprintf(stderr, "at least one partition\n");
	exit(1);
    }

    /*
     * The autochanger can only have one partition, and it must be
     * partition 1.
     */
    if (major(sds->devs[0]) == AC_MAJ_BLK)
    {
        if (partition_cnt > 1)
	{
	    fprintf(stderr,
		"%s: Error, OpAL can only have one partition\n", prog);
	    exit(1);
	}

	if (sds->section[0].size == 0)
	{
	    fprintf(stderr,
		"%s: Error, OpAL must use partition 1\n",
		prog);
	    exit(1);
	}
    }
}

/*
 * round_it() --
 *    Round a number up to a 'round_val' boundary.  This routine is
 *    overflow safe.  If the result would cause overflow, we truncate
 *    to a round_val boundary instead.
 */
round_it(x, round_val)
u_long x;
u_long round_val;
{
    u_long rem = x % round_val;
    u_long result;

    if (rem == 0)
	return x;

    result = x + (round_val - rem);
    if (result < x)
	return x - rem; /* overflow, truncate to round_val boundary */

    return result;
}

/*
 * compute_reserve() --
 *    Compute the amount of space that we must reserve for an SDS
 *    array.  This space is small (SDS_MINRESERVE) for single-disk
 *    arrays.  However for multi-disk arrays, we must reserve enough
 *    space to represent the SDS data in LVM terms (for future in-situ
 *    migration to LVM).
 */

#define ROUNDBITS2WORD(x)	round_it(x, 32)
#define ROUNDBITS2BYTE(x)	round_it(x, 8)

u_long
compute_reserve(ndevs, blksz, disk_size)
int ndevs;
u_long blksz;
u_long disk_size;
{
    u_long reserve;
    u_long vgda_size;
    u_long vgsa_size;
    u_long pvra;
    u_long maxlvs = SDS_MAXSECTS;	/* max logical volumes */
    u_long maxpvs = SDS_MAXDEVS;	/* max phyiscal devices */
    u_long maxpxs = (disk_size / SDS_PARTITION_MULT) + 1;
					/* max physical extents */
    u_long pvstruct;
    u_long mwc;

    if (ndevs == 1)
	return SDS_MINRESERVE;

    /*
     * Round the block size up to a multiple of 1K.  If the block size
     * is already greater than 1K, it must already be a multiple of
     * 1K since the blksz is guaranteed to be a power of two.
     */
    blksz = round_it(blksz, 1024);

    /*
     * The size of the PV structure is
     *   pvstruct		round_it(16 + 4 * maxpxs, blksz)
     *
     * The LVM information requires (in bytes unless otherwise
     * indicated):
     *
     *   PVRA --
     *     round_it(128 * 1024, blksz)
     *   VGDA --
     *     header/trailer	2 * blksz
     *     LV list		round_it(16 * maxlvs, blksz)
     *     PV list		maxpvs * pvstruct
     *   VGSA --
     *     round_it(20 +
     *		    (4 * ROUNDBITS2WORD(maxpvs)) +
     *		    maxpvs * ROUNDBITS2BYTE(maxpxs), blksz)
     *
     *   MWC  -- 1 * blksz (Note -- the MWC must start an an 8k
     *                      boundary.  To be safe we simply add 8k
     *                      to the MWC).
     *
     * Total size needed is 2 * (PVRA + VGDA + VGSA + MWC).
     * The VGDA, VGSA and MWC must all start on an 8K boundary.
     *
     * We add 25% to allow for possible growth of data LVM structures.
     */
    pvra = round_it(128 * 1024, blksz);
    pvstruct = round_it(16 + 4 * maxpxs, blksz);
    vgda_size = 2 * blksz +
		round_it(16 * maxlvs, blksz) +
		pvstruct * maxpvs;
    vgda_size = round_it(vgda_size, 8192);
    vgsa_size = 28 +
		(3 * 4 * ROUNDBITS2WORD(maxpvs)) +
		maxpvs * ROUNDBITS2BYTE(maxpxs);
    vgsa_size = round_it(vgsa_size, blksz);
    vgsa_size = round_it(vgsa_size, 8192);
    mwc = 8192;
    reserve = 2 * (pvra + vgda_size + vgsa_size + mwc);

    /*
     * Increase the computed amount by 25%, to allow for expansion of
     * LVM data structures.
     */
    reserve = (reserve * 125) / 100;

    /*
     * Round the reserved amount up to a 8K boundary.
     */
    return round_it(reserve, 8 * 1024);
}

/*
 * allocate_partitions() --
 *    Given an initialized SDS, physical block size and disk size,
 *    fill in all partition information of the SDS.
 *
 *    Each partition is rounded up to a (ndevs * SDS_PARTITION_MULT)
 *    multiple so that we can migrate the SDS array to LVM in the
 *    future.
 *
 * Assumptions:
 *    .  the disk_size is <= 2^32-1.
 *    .  blksz is a power of two.
 *    .  the disk_size is a multiple of blksz.
 *
 * These are ensured by other portions of the code:
 *    .  the stripesize for any partition is >= the blksz.
 *    .  the stripesize for any partition is a power of two.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
allocate_partitions(sds, blksz, disk_size)
sds_info_t *sds;
u_long blksz;
u_long disk_size;
{
    int i;
    int ndevs = sds->ndevs;
    u_long current_offset;	/* in (device) blocks */
    u_long reserve;		/* in bytes */
    u_long round_val;		/* in bytes */
    u_long disk_size_blks;	/* in (device) blocks */
    double temp;

    /*
     * Compute the amount to reserve at the beginning of each disk.
     */
    reserve = compute_reserve(sds->ndevs, blksz, disk_size);

    /*
     * The user may override the reserve amount and make it bigger
     * than what we would have reserved (they cannot, however make
     * it smaller).  We round up any user-specified reserve value to
     * an 8K boundary (so LVM migration still works).
     */
    sds->reserve = round_it(sds->reserve, 8 * 1024);
    if (sds->reserve > reserve)
	reserve = sds->reserve;
    sds->reserve = reserve;

    if (reserve >= disk_size)
    {
	fprintf(stderr, "%s: Error, reserve amount of %lu is ",
	    prog, reserve);
	fprintf(stderr, "bigger than %s\n", files[0].name);
	exit(1);
    }

    /*
     * Even for single disk arrays, we round partitions up to a
     * (1 Mbyte * ndevs) boundary.  Since the calculations here are
     * mostly done on a per-disk basis, round_val will be 1 Mbyte.
     */
    round_val = SDS_PARTITION_MULT;

    /*
     * Convert disk_size to blksz units.
     */
    disk_size_blks = disk_size / blksz;

    /*
     * Process each partition.
     */
    current_offset = reserve / blksz;
    for (i = 0; i < SDS_MAXSECTS; i++)
    {
	u_long size;		/* in bytes */
	u_long size_blks;	/* in (device) blocks */
	u_long orig_size_blks;	/* in (device) blocks */

	if (sds->section[i].size == 0)
	    continue;

	/*
	 * If the last partition took up all remaining space, and
	 * they are still trying to allocate another partition,
	 * print an error message and exit.
	 */
	if (current_offset == disk_size_blks)
	{
	    fprintf(stderr, "%s: Error, no room for partition %d\n",
		prog, i+1);
	    exit(1);
	}

	sds->section[i].start = current_offset * blksz;
	size = sds->section[i].size / ndevs;

	/*
	 * Roundup the per-disk size to a round_val boundary.
	 * Then convert the size to blksz units.
	 */
	size = round_it(size, round_val);
	size_blks = size / blksz; /* convert to blksz units */
	orig_size_blks = size_blks;

	/*
	 * If the end of this partition will go past the end of the
	 * disk, trim it back to a multiple of round_val that will
	 * still fit on the disk.  However, if the trimed size is less
	 * than the original size request, this is an error.
	 */
	if (current_offset + size_blks > disk_size_blks)
	{
	    u_long round_blks = round_val / blksz;

	    size_blks = disk_size_blks - current_offset;
	    size_blks = (size_blks / round_blks) * round_blks;

	    /*
	     * See if the resulting size is smaller than what was
	     * requested.  A value of END_SECTION_SIZE means 'the
	     * rest of the disk', so we do not perform any checks
	     * in that case.
	     */
	    if (sds->section[i].size != END_SECTION_SIZE &&
		orig_size_blks > size_blks)
	    {
		fprintf(stderr,
		    "%s: Error, partition %d is %lu bytes too large\n",
		    prog, i+1, (orig_size_blks - size_blks) * blksz);
		exit(1);
	    }
	    current_offset = disk_size_blks;
	}
	else
	{
	    /*
	     * Update current_offset to its new starting point.
	     */
	    current_offset += size_blks;
	}

	/*
	 * For LVM migration, we must ensure that there is at least
	 * some space for the Bad Block Relocation Area.  If the
	 * unlikely event that the last partition has not left at
	 * least 8k for the BBRA, then we trim the size of the last
	 * partition one (1Mbyte) chunk worth.
	 */
	if (ndevs != 1)
	{
	    u_long rem = disk_size_blks -
			 (sds->section[i].start / blksz + size_blks);
	    u_long orig_size_blks;

	    /*
	     * Make sure that there is enough room in the BBRA, if
	     * not, we must reduce the size of the last partition by
	     * a full "round_val" chunk (1 Megabyte).  This is really
	     * a shame, but should not happen often, and is required
	     * for LVM migration.
	     */
	    orig_size_blks = size_blks;
	    if (rem * blksz < BBRA_RESERVE)
	    {
		size_blks -= (round_val / blksz);
		current_offset = disk_size_blks;
	    }

	    /*
	     * If the size is now 0 (or it underflowed below 0), then
	     * there is no room for this partition.
	     */
	    if (size_blks == 0 || orig_size_blks < size_blks)
	    {
		fprintf(stderr, "%s: Error, no room for partition %d\n",
		    prog, i+1);
		exit(1);
	    }

	    if (sds->section[i].size != END_SECTION_SIZE &&
		orig_size_blks > size_blks)
	    {
		fprintf(stderr,
		    "%s: Error, partition %d is %lu bytes too large\n",
		    prog, i+1, (orig_size_blks - size_blks) * blksz);
		exit(1);
	    }
	}

	/*
	 * Calculate the actual size (in bytes) of this partition.
	 * If it is more than MAX_SECTION_SIZE, trim it back by blksz
	 * chunks at a time.
	 */
	temp = (double)size_blks * blksz * ndevs;
	while (temp > MAX_SECTION_SIZE)
	    temp -= blksz;

	/*
	 * If the size of this partition is exactly 2 gigabytes, we
	 * reduce it by one device block.  This way, applications
	 * that have a 2G limit will not wrap around, since we will
	 * always be less than 2G.
	 */
	if (temp == (2.0 * 1024 * 1024 * 1024))
	    temp -= blksz;

	sds->section[i].size = temp;
    }

    /*
     * Warn the user that some space was wasted (only if we did waste
     * any space).
     */
    temp = (disk_size_blks - current_offset) * blksz;
    if (temp > SDS_PARTITION_MULT)
    {
	/*
	 * Truncate the remainder to a SDS_PARTITION_MULT boundary
	 * so that we do not confuse the user about the amount of
	 * space that they could have used (since partitions are
	 * always a SDS_PARTITION_MULT * ndevs multiple).
	 */
	temp -= ((u_long)temp % SDS_PARTITION_MULT);
	temp *= ndevs;

	fprintf(stderr, "%s: Warning, %1.0f bytes were unallocated\n",
	    prog, temp);
    }
}

#ifdef SDS_NEW
clobber_fs()
{
    struct fs fs;
    char *name;
    char d_name[sizeof "/dev/rdsk/" + MAXNAMLEN];
    dev_t dev;
    int fd;

    dev = (files[0].dev & ~0x0f) | 1;

    name = device_name(dev, files[0].name);
    sprintf(d_name, "%s/%s", "/dev/dsk", name);

    fd = open(d_name, O_WRONLY);
    if (fd < 0) 
    {
        fprintf(stderr, "%s: can't open ", prog);
        perror(d_name);
        exit(1);
    }

    if (lseek(fd, SBLOCK * DEV_BSIZE, SEEK_SET) < 0) 
    {
        fprintf(stderr, "%s: can't seek on ", prog);
        perror(d_name);
        exit(1);
    }

    fs.fs_magic = 0;

    if (write(fd, &fs, sizeof fs) != sizeof fs)
    {
        fprintf(stderr, "%s: can't write to ", prog);
        perror(d_name);
        exit(1);
    }
}

#endif /* SDS_NEW */
/*
 * sds_create() --
 *    Create an SDS array from the devices specified on the command
 *    line.  Unless the -f option was supplied, no devices may already
 *    have SDS_INFO data on them.  Adds an entry to disktab for the
 *    new array (if necessary) and creates the device files necessary
 *    to access each partition of the new SDS array.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
sds_create()
{
    int i;
    u_long size;
    u_long slop;
    u_long blksz;
    int warned;
    sds_info_t sds;
    sds_info_t sds2;
    struct timeval tv;
    char fbuf[SDS_MAXDESC + 1]; /* need room for '\0' terminator */
    struct fs fs;

    /*
     * Verify that all of the devices use the same major number.
     * We cannot mix cs80, scsi and autochanger devices in the same
     * array.
     */
    for (i = 0; i < n_files - 1; i++)
    {
	if (major(files[i].dev) == OPAL_MAJ_BLK)
	{
	    fprintf(stderr, "%s: %s device cannot ", prog,
		driver_name(files[i].dev));
	    fprintf(stderr, "be part of an array\n");
	    exit(1);
	}

	if (major(files[i].dev) != major(files[i+1].dev))
	{
	    fprintf(stderr, "%s: %s and %s devices cannot ", prog,
		driver_name(files[i].dev),
		driver_name(files[i+1].dev));
	    fprintf(stderr, "be part of the same array\n");
	    exit(1);
	}
    }

    /*
     * Initialize the sds structure to zero so that we do not have
     * to explicitly initialize any fields.
     */
    memset(&sds, '\0', sizeof sds);

    /*
     * Re-order the devices to try to create an optimal configuration.
     * This can fail, for example if there are 6 devices, and 2 are
     * on the one bus and 4 are on a second bus, there is no way to
     * create an optimal configuration.  If we failed to create an
     * optimal configuration, we warn the system administrator.
     */
    make_optimal_busses();
    if (!optimal_busses())
    {
	fprintf(stderr, "%s: Devices are not connected", prog);
	fprintf(stderr, " in an optimal configuration\n");
    }

    /*
     * Ensure that none of the disks are already members of
     * an array.
     */
    if (!force)
    {
	for (i = 0; i < n_files; i++)
	{
	    /*
	     * See if this disk is a member of an array.
	     */
	    memset(&sds2, '\0', sizeof sds2);
	    if (sds_get_file(files[i].dev, fbuf) == SDS_OK &&
		sds_parse_raw(&sds2, fbuf) == SDS_OK)
	    {
		char *array_name;

		if (sds2.label[0] != '\0')
		    array_name = sds2.label;
		else
		    array_name = sds2.type;

		fprintf(stderr,
		    "%s: %s is already disk %d of array \"%s\"!\n",
		    prog, files[i].name,
		    sds2.which_disk + 1, array_name);
		exit(1);
	    }
	}
    }

    /*
     * Make sure that none of the devices contain an HP-UX filesystem.
     * We must do this even if they did not use -f, since we want to
     * explicitly nuke the filesystem if the use -f or if they tell
     * us to proceed.
     */
    for (i = 0; i < n_files; i++)
    {
	/*
	 * Read the superblock.
	 */
	(void)LSEEK(i, SBLOCK * DEV_BSIZE, SEEK_SET);
	if (read(files[i].fd, &fs, sizeof fs) != sizeof fs)
	{
	    fprintf(stderr, "%s: can't read from ", prog);
	    perror(files[i].name);
	    exit(1);
	}

	if (fs.fs_magic == FS_MAGIC ||
	    fs.fs_magic == FS_MAGIC_LFN ||
	    fs.fs_magic == FD_FSMAGIC)
	{
	    if (!force)
	    {
		if (isatty(0))
		{
		    printf("%s: %s contains an HP-UX filesystem. ",
			prog, files[i].name);
		    printf("Continue");
		    if (!yes_or_no())
		    {
			fprintf(stderr, "%s: operation aborted\n",
			    prog);
			exit(1);
		    }
		}
		else
		{
		    fprintf(stderr,
			"%s: Error, %s contains an HP-UX filesystem.\n",
			prog, files[i].name);
		    exit(1);
		}
	    }

	    /*
	     * One way or the other, they have decided it is okay to
	     * nuke this filesystem.  Since this superblock is in our
	     * reserved area, we will never write to it.  Thus, they
	     * would always get this warning.  To prevent this, we
	     * nuke the magic number of the superblock.
	     */
	    fs.fs_magic = 0;
	    (void)LSEEK(i, SBLOCK * DEV_BSIZE, SEEK_SET);
	    WRITE(i, (char *)&fs, sizeof fs);
	}
    }

    /*
     * Make sure that all the disks have the same block size.  We
     * do not allow disks of different block sizes to be part of
     * the same array.
     */
    for (i = 1; i < n_files; i++)
    {
	if (files[i].blksz != files[0].blksz)
	{
	    fprintf(stderr, "%s: Error, all drives do not ", prog);
	    fprintf(stderr, "have an identical block size\n");
	    exit(1);
	}
    }

    /*
     * See if the disks are all the same size.  If they are not the
     * same size (within min(0.5%, 512K)), print a warning.
     *
     * First, calculate the smallest size.
     */
    size = 0xffffffff;
    for (i = 0; i < n_files; i++)
	if (files[i].size < size)
	    size = files[i].size;

    slop = size / 200;	/* 0.5 percent */
    if (slop > 512 * 1024)
	slop = 512 * 1024;

    warned = 0;
    for (i = 0; i < n_files; i++)
    {
	if (files[i].size - size > slop)
	{
	    if (!warned)
	    {
		fprintf(stderr,
		    "%s: Warning, all devices are not the same size\n",
		    prog);
		warned = 1;
	    }
	    fprintf(stderr, "%10d bytes of %s will be unused\n",
		files[i].size - size, files[i].name);
	}

	/*
	 * Fill in the devices[] part of the sds structure.
	 */
	sds.devs[i] = files[i].dev;

	/*
	 * Pretend that the size of each disk is the minimum size
	 * of all disks.
	 */
	files[i].size = size;
    }

    /*
     * The stripesize must be >= max(disk block size, DEV_BSIZE).
     * We have verified that all devices in the array have an
     * identical block size, so we can use any member (we use the
     * first one).
     */
    blksz = files[0].blksz;
    if (blksz < DEV_BSIZE)
	blksz = DEV_BSIZE;

    /*
     * Verify that the default stripesize is larger than the device
     * block size.
     */
    if (stripesize < blksz)
    {
	fprintf(stderr,
	    "%s: Error, stripesize must be at least %d bytes\n",
	    prog, blksz);
	exit(1);
    }

    sds.ndevs = n_files;

    /*
     * If no configuration file was supplied, fill in the SDS
     * descriptor from the command line arguments.
     */
    if (cfg == (FILE *)0)
    {
	u_long capacity;
	u_long reserve;
	u_long p_size;
	int n_partitions;

	/*
	 * Convert the partition size (bytes) to the partition size
	 * per disk (device blocks).
	 *
	 * Even for single-disk arrays, we always round the partitions
	 * to a ndevs*SDS_PARTITION_MULT boundary.
	 */
	p_size = partition_size / sds.ndevs;
	p_size = round_it(p_size, SDS_PARTITION_MULT);

	/*
	 * First, compute how many (whole) partitions we will have.
	 * We must subtract the amount that we reserve at the beginning
	 * of the disk plus that amount that we reserve for the BBRA.
	 */
	reserve = compute_reserve(sds.ndevs, blksz, files[0].size);
	capacity = (files[0].size - reserve) - BBRA_RESERVE;
	n_partitions = capacity / p_size;

	/*
	 * If we will not use all of the capacity with
	 * SDS_MAXSECTS * partition_size, simply adjust n_partitions
	 * to SDS_MAXSECTS.  allocate_partitions() will generate
	 * a warning about unused space on the array.
	 */
	if (n_partitions > SDS_MAXSECTS)
	    n_partitions = SDS_MAXSECTS;

	for (i = 0; i < n_partitions; i++)
	{
	    sds.section[i].size = partition_size;
	    sds.section[i].stripesize = stripesize;
	}

	/*
	 * We may have one more partition, unless the
	 * "capacity / p_size" calculation came out evenly.
	 */
	if (capacity != (n_partitions * p_size) &&
	    n_partitions < SDS_MAXSECTS)
	{
	    sds.section[n_partitions].size = END_SECTION_SIZE;
	    sds.section[n_partitions].stripesize = stripesize;
	}

	/*
	 * If a type was specified on the command line (-T), copy
	 * it into place.  If none was specified, we fill in a default
	 * value later.
	 */
	if (type != (char *)0)
	    strcpy(sds.type, type);
    }
    else
    {
	/*
	 * Read in the input description file.  We limit the size to
	 * SDS_MAXDESC for convenience.  We read the data up front
	 * so that we can share the parsing routines that are also
	 * used to parse the SDS_INFO file.
	 */
	i = fread(fbuf, sizeof (char), SDS_MAXDESC, cfg);
	if (i <= 0)
	{
	    fprintf(stderr, "%s: ", prog);
	    perror("can't read array description file");
	    exit(1);
	}

	if (i == SDS_MAXDESC)
	{
	    fprintf(stderr, "%s: array description too large\n", prog);
	    exit(1);
	}
	fbuf[i] = '\0'; /* null-terminate the data file */

	/*
	 * Now that we have the array description file, parse it.
	 */
	sds_parse(&sds, blksz, fbuf);
    }

    /*
     * If a label was supplied on the command line, this overrides
     * anything in the config file.
     */
    if (label != (char *)0)
	strcpy(sds.label, label);

    /*
     * If no type was specified (either in the config file or on the
     * command line), try to make one up.  We compute the type by
     * appending "x<ndevs>-<n_partitions>" to the lead device product
     * id.  We might fail, if the value of the product id is too long.
     * If so, we print a message telling the user to specify a type.
     *
     */
    if (sds.type[0] == '\0')
    {
	int n_parts = 0;
	char def_name[SDS_MAXNAME + 5];

	/*
	 * Count up the number of partitions.
	 */
	for (i = 0; i < SDS_MAXSECTS; i++)
	    if (sds.section[i].size != 0)
		n_parts++;

	/*
	 * Compute the default type name for this array.  Since an
	 * autochanger can never have more than 1 partition, we do
	 * not put the "-n" on the end of the name for autoch arrays.
	 */
	if (major(files[0].dev) == AC_MAJ_BLK)
	    sprintf(def_name, "%sx%d", files[0].prod_id, sds.ndevs);
	else
	    sprintf(def_name, "%sx%d-%d",
		files[0].prod_id, sds.ndevs, n_parts);

	/*
	 * Make sure that we have enough room for the default type
	 * name.
	 */
	if (strlen(def_name) > SDS_MAXNAME)
	{
	    fprintf(stderr,
		"%s: default type of \"%s\" is too long\n",
		prog, def_name);
	    fprintf(stderr, "\tplease specify a type ");
	    if (cfg != (FILE *)0)
		fprintf(stderr, "in the configuration file\n");
	    else
		fprintf(stderr, "with -T\n");
	    exit(1);
	}

	strcpy(sds.type, def_name);
    }

    /*
     * Fill in partition information, round each partition to start
     * and end on the appropriate boundries.
     */
    allocate_partitions(&sds, blksz, size);

    /*
     * Compute a "unique" value for this array.  We use the
     * time of day for this.
     */
    (void)gettimeofday(&tv, (struct timezone *)0);
    sds.unique = tv.tv_sec ^ (u_long)tv.tv_usec;

    /*
     * Make sure that there is a disktab entry in /etc/disktab for
     * this array.  Adds one if necessary.  After verifying the
     * entry is in /etc/disktab, tell them what it is called.
     */
#ifdef SDS_BOOT
    update_disktab(&sds);
#else /* ! SDS_BOOT */
    sds_add_disktab(&sds);
    if (sds.label[0] != '\0')
	printf("%s: disktab name for \"%s\" is HP_%s\n",
	    prog, sds.label, sds.type);
    else
	printf("%s: disktab name of this array is HP_%s\n",
	    prog, sds.type);
#endif /* else ! SDS_BOOT */

    /*
     * Create device files for all of the partitions of this array.
     */
    create_devices(&sds);

    /*
     * Write the new SDS_INFO data to each disk of the new
     * SDS array.
     */
    write_sds(&sds, fbuf);

    /*
     * Finally, print a message indicating which device is the
     * lead device.
     */
    printf("%s: lead device of \"%s\" is %s\n",
	prog, sds.label[0] != '\0' ? sds.label : sds.type,
	files[0].name);
#ifdef SDS_NEW
    clobber_fs();
#endif /* SDS_NEW */
}

#define SDS_CHECK   1
#define SDS_IMPORT  2

/*
 * sds_check_or_import() --
 *    Perform operations that are common to sds_import() and
 *    sds_check().  Where there are slight differences, runtime
 *    checks are made, based on the value of 'operation'.
 *
 *    General algorithm for check:
 *       Read the SDS information from the first device.
 *
 *       If this is not the lead device
 *           issue a warning
 *
 *       If this device is not listed in the SDS information
 *           Tell the user to import the array.
 *           exit
 *
 *       For each other device in the SDS array
 *           Read the SDS information from that device
 *           Verify that it is for the same array as the first device.
 *           If this device is the lead device
 *               Tell the user that this is the lead device
 *           Verify that we do not see a device twice.
 *
 *       Re-order the sds->devs[] array to match the order of the disks
 *
 *    General algorithm for import:
 *       Read the SDS information from the first device.
 *
 *       If they did not give the right # of devices for this array
 *           issue an error message
 *           exit
 *
 *       Change sds->ndevs[] to match the devs on the command line
 *
 *       For all remaining devices (from the command line)
 *           Read the SDS information from that device
 *           Verify that it is for the same array as the first device.
 *           Verify that we do not see a device twice.
 *
 *       Re-order the sds->devs[] array to match the order of the disks
 *
 *    Performs an exit(1) if a fatal error occurs.
 */
void
sds_check_or_import(operation, sds)
int operation;
sds_info_t *sds;
{
    int i;
    int err = 0;
    dev_t seen_devs[SDS_MAXDEVS];
    sds_info_t sds2;
    char *array_name;

    if (sds_read_one(0, sds) != 0)
	exit(1);

    if (sds->label[0] != '\0')
	array_name = sds->label;
    else
	array_name = sds->type;

    /*
     * If we are importing an array, we change the sds->devs[] array
     * to match the arguments passed on the command line.  Of course,
     * we must verify that they specified the right number of devices
     * for this array.
     */
    if (operation == SDS_IMPORT)
    {
	if (sds->ndevs != n_files)
	{
	    fprintf(stderr, "%s: Error, \"%s\" has %d ",
		prog, array_name, sds->ndevs);
	    fprintf(stderr, "device%s,\n\t   but ",
		sds->ndevs > 1 ? "s" : "");
	    fprintf(stderr, "%d %s specified on the command line\n",
		n_files, n_files == 1 ? "was" : "were");
	    exit(1);
	}

	for (i = 0; i < sds->ndevs; i++)
	    sds->devs[i] = files[i].dev;
    }
    else
    {
	/*
	 * We are checking an array.  Issue a warning if they did not
	 * give us the lead device.
	 */
	if (sds->which_disk != 0)
	{
	    fprintf(stderr,
		"%s: Warning, %s is not the lead device of \"%s\"\n",
		prog, files[0].name, array_name);
	}
    }

    if (sds->ndevs == 1)
	sds->devs[0] = files[0].dev;

    for (i = 0; i < sds->ndevs; i++)
	if (files[0].dev == sds->devs[i])
	    break;

    if (i >= sds->ndevs)
    {
	fprintf(stderr, "%s: %s is disk %d of \"%s\", but",
	    prog, files[0].name, sds->which_disk + 1, array_name);
	fprintf(stderr, " is not listed in\n\tthe SDS_INFO file\n");
	fprintf(stderr, "%s: run \"%s -i\" to correct\n",
	    prog, prog);
	exit(1);
    }

    /*
     * Make sure that we do not see a disk twice.
     */
    for (i = 0; i < sds->ndevs; i++)
	seen_devs[i] = NODEV;
    seen_devs[sds->which_disk] = files[0].dev;

    for (i = 0; i < sds->ndevs; i++)
    {
	int fdi;
	char dev_name[20];

	/*
	 * We have already done the first device.
	 */
	if (sds->devs[i] == files[0].dev)
	    continue;

	/*
	 * If we are performing a "check" operation, then we must
	 * open each device.  If we are performing an "import"
	 * operation, then all the devices are already open.
	 */
	if (operation == SDS_CHECK)
	{
	    fdi = TMP_FILE;
	    sprintf(dev_name, "device 0x%08x", sds->devs[i]);
	    files[fdi].name = dev_name;
	    files[fdi].dev = sds->devs[i];

	    files[fdi].fd = blk_open(sds->devs[i], O_RDONLY, dev_name);
	    if (files[fdi].fd == -1)
	    {
		err = 1;
		continue;
	    }
	    files[fdi].rfd = -1; /* we do not need the raw device */
	}
	else
	{
	    /*
	     * The right file to use is files[i].  This is true because
	     * we just initialized the sds->devs[] array to the devices
	     * in the files[] array.
	     */
	    fdi = i;
	}

	if (sds_read_one(fdi, &sds2) != 0)
	{
	    if (operation == SDS_IMPORT)
		exit(1);

	    close(files[fdi].fd);
	    files[fdi].fd = -1;
	    files[fdi].dev = NODEV;
	    err = 1;
	    continue;
	}

	if (operation == SDS_CHECK)
	{
	    /*
	     * If this is the lead device (only possible if they did
	     * not specify the lead device to begin with), tell the
	     * user that this is the lead device.  We cannot print
	     * the actual name of a device file, since we do not know
	     * what their naming convention is.
	     */
	    if (sds2.which_disk == 0)
	    {
		fprintf(stderr,
		    "%s: lead device of \"%s\" has major number %d, ",
		    prog, array_name, major(files[fdi].dev));
		fprintf(stderr, "minor number 0x%06x\n",
		    minor(files[fdi].dev));
	    }

	    /*
	     * Close our temporary file, we are finished with it.
	     */
	    close(files[fdi].fd);
	    files[fdi].fd = -1;
	    files[fdi].dev = NODEV;
	}

	if (sds->unique != sds2.unique)
	{
	    fprintf(stderr,
		"%s: %s and %s are members of different SDS arrays\n",
		prog, files[0].name, dev_name);
	    if (operation == SDS_IMPORT)
		exit(1);
	    err = 1;
	    continue;
	}

	if (seen_devs[sds2.which_disk] != NODEV)
	{
	    fprintf(stderr,
		"%s: %s and device 0x%08x are both\n\t   disk",
		prog, dev_name, seen_devs[sds2.which_disk]);
	    fprintf(stderr, " %d of \"%s\"\n",
		sds2.which_disk + 1, array_name);
	    if (operation == SDS_IMPORT)
		exit(1);
	    err = 1;
	    continue;
	}
	seen_devs[sds2.which_disk] = sds->devs[i];
    }

    /*
     * If we saw any errors along the way, exit now.
     */
    if (err)
	exit(1);

    /*
     * Re-order the devices in sds.devs[] to match the disk ordering.
     * This is used by sds_import()
     */
    for (i = 0; i < sds->ndevs; i++)
	sds->devs[i] = seen_devs[i];
}

#ifdef SDS_BOOT
void
sds_export(argc, argv)
int argc;
char *argv[];
{
    int i;
    int j;
    sds_info_t sds_buf;
    register sds_info_t *sds = &sds_buf;
    char buf[SDS_MAXINFO];
    char *array_name;


	/*   XXX i and j are none too symbolic....  i is really the
	     device name index, and j is really the files[] index  */
    n_files = 0;
    for (i = 0, j = 0; i < argc; i+=2, j++) {
	files[j].name = argv[i];
        open_dev(j);
        identify_dev(j);
        n_files++;

        new_minor[num_new_minors] = strtoul (argv[i+1], (char **) NULL, 0) ;
	if (new_minor[num_new_minors] == 0) {
	    fprintf(stderr, "%s Error, specified new minor number ", prog);
	    fprintf(stderr, "\"%s\" is not a number\n", argv[i+1]);
	    exit(1);
	}
	num_new_minors++;
    }

    if (sds_read_one(0, sds) != 0)
	exit(1);

    if (sds->label[0] != '\0')
        array_name = sds->label;
    else
        array_name = sds->type;

    /*
     * We are exporting an array.  The first arg must be the lead device.
     */
    if (sds->which_disk != 0)
    {
        fprintf(stderr,
            "%s: Error, %s is not the lead device of \"%s\"\n",
            prog, files[0].name, array_name);
	exit(1);
    }

    if (sds->ndevs != n_files)
    {
        fprintf(stderr, "%s: Error, \"%s\" has %d ",
            prog, array_name, sds->ndevs);
        fprintf(stderr, "device%s,\n\t   but ",
            sds->ndevs > 1 ? "s" : "");
        fprintf(stderr, "%d %s specified on the command line\n",
            n_files, n_files == 1 ? "was" : "were");
        exit(1);
    }

    for (i = 0; i < sds->ndevs; i++) {
        sds->devs[i] = new_minor[i] ;
#ifdef SDS_DEBUG
        printf ("Using 0x%08x as the minor number\n",new_minor[i]) ;
#endif /* SDS_DEBUG */
    }

    write_sds(sds, buf);
}

#endif /* SDS_BOOT */
/*
 * sds_import() --
 *    Import an existing SDS array to a new configuration.  Updates
 *    the SDS_INFO on each disk of the array, adds a disktab entry
 *    (if needed) and creates any necessary device files.
 *
 *    Performs an exit(1) [indirectly] if an error occurs.
 */
void
sds_import()
{
    int i;
    int j;
    sds_info_t sds;
    char buf[SDS_MAXINFO];

    /*
     * First we call sds_check_or_import() to verify that all of the
     * members of the array were specified on the command line.  One
     * (very important) side-effect is that we get the array descriptor
     * in "sds" with the sds.devs[] SORTED in proper disk order.
     * Using this sorted list, we can re-sort the command line
     * arguments so that they are also properly ordered.  Then simply
     * call write_sds() to write the updated SDS_INFO to each member
     * of the array.
     */
    sds_check_or_import(SDS_IMPORT, &sds);

    for (i = 0; i < n_files; i++)
    {
	for (j = i; j < n_files; j++)
	{
	    if (sds.devs[i] == files[j].dev)
	    {
		if (i != j)
		{
		    files[TMP_FILE] = files[i];
		    files[i] = files[j];
		    files[j] = files[TMP_FILE];
		}
		break;
	    }
	}
    }

    /*
     * Make sure that there is a disktab entry in /etc/disktab for
     * this array.  Adds one if necessary.
     */
#ifdef SDS_BOOT
    update_disktab(&sds);
#else /* ! SDS_BOOT */
    sds_add_disktab(&sds);
    if (sds.label[0] != '\0')
	printf("%s: disktab name for \"%s\" is HP_%s\n",
	    prog, sds.label, sds.type);
    else
	printf("%s: disktab name of this array is HP_%s\n",
	    prog, sds.type);
#endif /* else ! SDS_BOOT */

    /*
     * Create device files for all of the partitions of this array.
     */
    create_devices(&sds);

    /*
     * Now that we have re-sorted the files[] array to match the
     * order of the devices in sds.devs[], we just call write_sds()
     * to update all members of the array.
     */
    write_sds(&sds, buf);

    /*
     * Check to see if the devices are connected in an "optimal"
     * configuration.  Warn the system administrator if not.
     */
    if (!optimal_busses())
    {
	fprintf(stderr, "%s: Devices are not connected", prog);
	fprintf(stderr, " in an optimal configuration\n");
    }

    /*
     * Finally, print a message indicating which device is the
     * lead device.
     */
    printf("%s: lead device of \"%s\" is %s\n",
	prog, sds.label[0] != '\0' ? sds.label : sds.type,
	files[0].name);
}

/*
 * sds_check() --
 *    Check that all of the disks of an SDS array are present and
 *    accessible.
 *
 *    Performs an exit(1) [indirectly] if an error occurs.
 */
void
sds_check()
{
    sds_info_t sds;

    sds_check_or_import(SDS_CHECK, &sds);
}

/*
 * sds_list() --
 *    Print information about a disk of an SDS array.
 *
 *    Performs an exit(1) [indirectly] if an error occurs.
 */
void
sds_list()
{
    int i;
    sds_info_t sds;

    if (sds_read_one(0, &sds) != 0)
	exit(1);

    printf("%s:\n", files[0].name);
    if (sds.label[0] != '\0')
	printf("%-12s %s\n", "label", sds.label);
    printf("%-12s %s\n", "type", sds.type);
    printf("%-12s %d of %d\n", "disk", sds.which_disk + 1, sds.ndevs);

    if (sds.ndevs > 1)
    {
	printf("%-12s 0x%08x\n", "unique_id", sds.unique);

	for (i = 0; i < sds.ndevs; i++)
	    printf("%-12s 0x%08x\n", "device", sds.devs[i]);
    }

    for (i = 0; i < SDS_MAXSECTS; i++)
    {
	if (sds.section[i].size != 0)
	{
	    /*
	     * If this is the lead device, also print the names for
	     * the block and character special files.
	     */
	    if (sds.which_disk == 0)
	    {
		dev_t dev = files[0].dev;
		char *name;

                if (major(dev) == AC_MAJ_BLK)
                    dev = makedev(OPAL_MAJ_BLK, minor(dev));
                else
		    dev = (dev & ~0x0f) | (i + 1);

		name = device_name(dev, files[0].name);

                if (major(files[0].dev) == AC_MAJ_BLK)
		    printf("partition %2d (/dev/opal/%s /dev/ropal/%s)\n",
		        i+1, name, name);
                else
		    printf("partition %2d (/dev/dsk/%s /dev/rdsk/%s)\n",
		        i+1, name, name);
	    }
	    else
		printf("partition %2d\n", i+1);

	    if (sds.ndevs > 1)
		printf("    stripesize %10lu\n",
		    sds.section[i].stripesize);
	    printf("    size       %10lu\n", sds.section[i].size);

	}
    }
}

/*
 * sds_destroy() --
 *    Remove the SDS_INFO data from an SDS disk.  All we do is
 *    overwrite the LIF magic number with 0.  If the -f option was
 *    not given, and stdin is a tty, ask the user for confirmation.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
sds_destroy()
{
    int i;
    int err = 0;
    short magic = 0;
    int do_prompt = (!force && isatty(0));
    char *array_name;
    sds_info_t sds;

    for (i = 0; i < n_files; i++)
    {
	/*
	 * Read the SDS_INFO file from the device.  If there is no
	 * SDS_INFO file, then we print an error.
	 */
	if (sds_read_one(i, &sds) != SDS_OK)
	{
	    fprintf(stderr,
		"%s: Error, %s is not part of an SDS array\n",
		prog, files[i].name);
	    err = 1;
	    continue;
	}

	/*
	 * When printing messages, we use the label of the array if
	 * it is defined, otherwise we use the type.
	 */
	if (sds.label[0] != '\0')
	    array_name = sds.label;
	else
	    array_name = sds.type;

	/*
	 * If the did not use the -f option, and stdin is a TTY, ask
	 * the system administrator to confirm the operation.
	 */
	if (do_prompt)
	{
	    printf("%s: %s is disk %d of \"%s\"\n",
		prog, files[i].name, sds.which_disk + 1, array_name);
	    printf("Destroy SDS configuration data for %s",
		files[i].name);

	    if (!yes_or_no())
	    {
		printf("%s: %s unchanged\n", prog, files[i].name);
		continue;
	    }
	}

#ifdef SDS_NEW		/*  XXX would be nice to restructure  */
        if (sds_destroy_lead) {
	    int j;

            for (j = 0; j < sds.ndevs; j++) {
		int fd;
		u_long val;

		fd = blk_open(sds.devs[j], O_RDWR, "");
                /*
                 * Just clobber the magic number of the LIF volume on the
                 * device.  This is a 16 bit number, which we simply set to 0.
                 */
                val = lseek(fd, 0, SEEK_SET);
                if (val == (u_long)-1) {
                    fprintf(stderr, "%s: can't seek on  dev = 0x%x", 
			prog, sds.devs[j]);
		    perror("write failed");
                    exit(1);
                }
                if (write(fd, (char *)&magic, sizeof magic) != sizeof magic) {
	            fprintf(stderr, "%s: can't write to dev = 0x%x", 
			prog, sds.devs[j]);
		    perror("write failed");
		    exit(1);
		}
                fsync(fd);
	    }
	    return;
        } else {
	    /*
	     * Just clobber the magic number of the LIF volume on the
	     * device.  This is a 16 bit number, which we simply set to 0.
	     */
	    (void)LSEEK(i, 0, SEEK_SET);
	    WRITE(i, (char *)&magic, sizeof magic);
	    fsync(files[i].fd);
	}
#else /* ! SDS_NEW */
	/*
	 * Just clobber the magic number of the LIF volume on the
	 * device.  This is a 16 bit number, which we simply set to 0.
	 */
	(void)LSEEK(i, 0, SEEK_SET);
	WRITE(i, (char *)&magic, sizeof magic);
	fsync(files[i].fd);
#endif /* else ! SDS_NEW */
    }

    if (err)
	exit(1);
}

/*
 * sds_undestroy() --
 *    Unremove the SDS_INFO data from an SDS disk.  Since the destroy
 *    only overwrote the LIF magic number field with 0, all we have to
 *    do is rewrite the magic number and then verify that the SDS data
 *    is still valid.  If the data is not valid, we overwrite the LIF
 *    magic number field with a 0 again.
 *
 *    Performs an exit(1) if an error occurs.
 */
void
sds_undestroy()
{
    int i;
    short magic;
    int err = 0;
    sds_info_t sds;

    for (i = 0; i < n_files; i++)
    {
	/*
	 * First, read the magic number field and check that there
	 * is either a 0 there (from when we did the destroy) or the
	 * correct LIF magic number (we are undestroying something
	 * that is not destroyed).
	 */
	(void)LSEEK(i, 0, SEEK_SET);
	if (read(files[i].fd, &magic, sizeof magic) != sizeof magic)
	{
	    fprintf(stderr, "%s: can't read from ", prog);
	    perror(files[i].name);
	    exit(1);
	}

	if (magic != 0 && magic != LIFID)
	    goto corrupted;

	/*
	 * Now, rewrite the LIF magic number in preparation for
	 * reading the SDS_INFO file.  Only do this if the magic
	 * number is not correct already.
	 */
	if (magic == 0)
	{
	    short tmp_magic = LIFID;

	    (void)LSEEK(i, 0, SEEK_SET);
	    WRITE(i, (char *)&tmp_magic, sizeof tmp_magic);
	    fsync(files[i].fd); /* make the write really happen */
	}

	/*
	 * Read the SDS_INFO file from the device.  If there is no
	 * SDS_INFO file, then we print an error.
	 */
	if (sds_read_one(i, &sds) != SDS_OK)
	{
	    /*
	     * If the original magic was zero, restore
	     * Overwrite the LIFID, since this is not a valid SDS_INFO
	     * file.
	     */
	    if (magic == 0)
	    {
		(void)LSEEK(i, 0, SEEK_SET);
		WRITE(i, (char *)&magic, sizeof magic);
		fsync(files[i].fd); /* make the write really happen */
	    }

	corrupted:
	    fprintf(stderr,
		"%s: SDS_INFO data on %s has been corrupted\n",
		prog, files[i].name);
	    err = 1;
	}
    }

    if (err)
	exit(1);
}
