static char HPUX_ID[] = "@(#) $Revision: 72.5 $";
/*
 *  diskinfo: Give disk specific information about a device
 */

#include <stdio.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/cs80.h>
#include <errno.h>
#include <unistd.h>
#include <sys/scsi.h>
#include <sys/ioctl.h>
#include <sys/diskio.h>

/*
 * TEMPORARY kludge to work around an s800 header file bug.
 */
#if !defined(SIOC_CAPACITY)
#   define SIOC_CAPACITY	_IOR('S', 3, struct capacity)
#endif

/*
 * Device types recognized by diskinfo.
 */
#define CS80		0x000
#define SCSI		0x001
#define SCSI_TAPE	0x002
#define AUTOCH		0x003
#define ALINK		0x004
#define NIO		0x005
#define OPAL		0x006
#define UNSUPP_MAJOR	-1

/*
 * Tables used for mapping major numbers to device types.
 */
struct major_map
{
    short major_num;	/* device major number */
    short dev_type;	/* type of device */
};

struct major_map s300_major_map[] =
{
    {  4, CS80		},
    { 47, SCSI		},
    { 54, SCSI_TAPE	},
    { 55, AUTOCH	},
    { -1, UNSUPP_MAJOR	}
};

struct major_map s700_major_map[] =
{
    {  4, CS80		},
    { 47, SCSI		},
    { 54, SCSI_TAPE	},
    { 55, AUTOCH	},
    { 104, OPAL		},
    { 121, SCSI_TAPE	},
    { -1, UNSUPP_MAJOR	}
};

struct major_map s800_major_map[] =
{
    {  4, CS80		},
    { 13, SCSI		},
    { 19, AUTOCH	},
    { 12, ALINK		},
    {  7, NIO		},
    { -1, UNSUPP_MAJOR	}
};

void do_scsi_inquiry();

char *prog_name;

main(argc, argv)
int argc;
char *argv[];
{
	extern char *strrchr();
	char ch, *fname, VERBOSE, BLOCK;
	int fildes;
	int type;			/* interface type */
	u_long size;
	struct describe_type db;
	struct stat stats;
	extern int optind, opterr;
	extern char *optarg;

	opterr = 0;	/* Suppress error diagnostics from getopt() */
	VERBOSE = 0;
	BLOCK = 0;

	if ((prog_name = strrchr(argv[0], '/')) != NULL)
	    prog_name++;
	else
	    prog_name = argv[0];

	while ((ch = getopt(argc, argv, "bv")) != EOF)
		switch (ch) {
			case 'v':
				if (VERBOSE || BLOCK)
					usage();
				VERBOSE = 1;
				break;

			/* Echo Block */
			case 'b':
				if (BLOCK || VERBOSE)
					usage();
				BLOCK = 1;
				break;

			case '?':
			default:
				usage();
		}

        if ((fname = argv[optind++]) == NULL || argv[optind] != NULL)
		usage();

	if ((fildes = open(fname, O_RDONLY)) == -1) {
		fprintf(stderr, "%s: can't open ", prog_name);
		perror(fname);
		exit(1);
	}

	if (fstat(fildes, &stats)) {
		fprintf(stderr, "%s: can't stat ", prog_name);
		perror(fname);
		exit(1);
	}

	/*
	 * perform some file status processing
	 */
	if ((stats.st_mode & S_IFMT) == S_IFBLK) {
		fprintf(stderr, "%s: Character device required\n",
		    prog_name);
		exit(1);
	}

	if ((stats.st_mode & S_IFMT) != S_IFCHR) {
		fprintf(stderr, "%s: Device special file required\n",
		    prog_name);
		exit(1);
	}

	if ((type=getInterfaceType(fildes)) == UNSUPP_MAJOR)
		type=get_disk_type(major(stats.st_rdev));
	switch (type) {
	    case ALINK:
	    case NIO:
	    case CS80:
	        if (ioctl(fildes, CIOC_DESCRIBE, &db) == -1) {
		    fprintf(stderr, "%s: can't CIOC_DESCRIBE ",
			prog_name);
		    perror(fname);
		    exit(errno);
		}
		{
		/* since disk might be partitioned, use DIOC_CAPACITY first */
			capacity_type cap;

			if (ioctl(fildes, DIOC_CAPACITY, &cap) != -1) 
				size= (u_long) cap.lba;
			else {
#ifdef __hp9000s300
				size = db.volume_tag.volume.maxsvadd.lfb;
#else
				size = db.volume_tag.volume.maxsvadd_lfb;
#endif /* __hp9000s300 */
			
				if (size)
					size = ++size/1024*db.unit_tag.unit.nbpb;
			}
		}
#ifdef __hp9000s800
		{
		/* in a disk array which has several modes of operation  */
		/* (i.e. 2-way and 4-way) we need to use DIOC_DESCRIBE   */
		/* and change the model number to BCD format for display */
			disk_describe_type ddes;
			if (ioctl(fildes, DIOC_DESCRIBE, &ddes) != -1) {
				char *ptr;
				for (ptr=&ddes.model_num; !(isdigit(*ptr)); ptr++) ;
				db.unit_tag.unit.dn[0]=((ptr[0]-'0')<<4) | (ptr[1]-'0');
				db.unit_tag.unit.dn[1]=((ptr[2]-'0')<<4) | (ptr[3]-'0');
				db.unit_tag.unit.dn[2]=((ptr[4]-'0')<<4) | (ptr[5]-'0');
			}
		}
#endif
		if (BLOCK) {
			printf("%lu\n", size);
			break;
		}

		printf("CS80 describe of %s:\n", fname);
		/* TODO Include info about whether a tape, etc.
		 *	Include which units are present
		 */
		if (VERBOSE) {
		    printf("  controller:   0x%04x  installed unit word\n",
			                    db.controller_tag.controller.iv);
		    printf("              %8d  max instantaneous xfr rate (Kbytes/sec)\n",
				           db.controller_tag.controller.mitr);
		    printf("              %8d  ",
				           db.controller_tag.controller.ct);
		    switch (db.controller_tag.controller.ct)	{
		        case 0:
			    printf("CS/80 integrated single unit controller\n");
			    break;
			case 1:
			    printf("CS/80 integrated multi-unit controller\n");
			    break;
			case 2:
			    printf("CS/80 integrated multi-port controller\n");
			    break;
			case 4:
			    printf("SS/80 integrated single unit controller\n");
			    break;
			case 5:
			    printf("SS/80 integrated multi-unit controller\n");
			    break;
			case 6:
			    printf("SS/80 integrated multi-port controller\n");
			    break;
			default:
			    printf("\n");
		    }
		    printf("  unit:       %8d  generic device type",
				           db.unit_tag.unit.dt);
		    switch (db.unit_tag.unit.dt) {
		        case 0:
			    printf(" (fixed disc)\n");
			    break;
			case 1:
			    printf(" (flexible or removable media)\n");
			    break;
			case 2:
			    printf(" (tape)\n");
			    break;
			default:
			    printf("\n");
		    }
		    printf("                %02x%02x%02x  device number (6 bcd digits)\n",
				             db.unit_tag.unit.dn[0],
				             db.unit_tag.unit.dn[1],
				             db.unit_tag.unit.dn[2]);
		    printf("              %8d  # of bytes per block\n",
				           db.unit_tag.unit.nbpb);
		    printf("              %8d  # of blocks buffered\n",
				           db.unit_tag.unit.nbb);
		    printf("              %8d  recommended burst size\n",
				           db.unit_tag.unit.rbs);
		    printf("              %8d  blocktime (microseconds)\n",
				           db.unit_tag.unit.blocktime);
		    printf("              %8d  continuous average transfer rate (Kbytes/sec)\n",
				           db.unit_tag.unit.catr);
		    printf("              %8d  optimal retry time (centiseconds)\n",
				           db.unit_tag.unit.ort);
		    printf("              %8d  access time parameter (centiseconds)\n",
				           db.unit_tag.unit.atp);
		    printf("              %8d  maximum interleave factor\n",
				           db.unit_tag.unit.mif);
		    printf("                  0x%02x  fixed volume byte\n",
				              db.unit_tag.unit.fvb);
		    printf("                  0x%02x  removable volume byte\n",
				              db.unit_tag.unit.rvb);
		    printf("  volume:     %8d  maximum cylinder address\n",
				           db.volume_tag.volume.maxcadd);
		    printf("              %8d  maximum head address\n",
			                   db.volume_tag.volume.maxhadd);
		    printf("              %8d  maximum sector address\n",
				           db.volume_tag.volume.maxsadd);
#ifdef __hp9000s300
		    printf("              %8d  maximum single-vector address\n",
				           db.volume_tag.volume.maxsvadd.lfb);
#else
		    printf("              %8d  maximum single-vector address\n",
				           db.volume_tag.volume.maxsvadd_lfb);
#endif /* __hp9000s300 */
		    printf("              %8d  current interleave factor\n",
				           db.volume_tag.volume.currentif);
		}
		else {
		    printf("         product id: %x%02x%x",
 				            db.unit_tag.unit.dn[0],
				            db.unit_tag.unit.dn[1],
				            db.unit_tag.unit.dn[2]>>4);
		    if (db.unit_tag.unit.dn[2]&0xf) /* Printf option if non-zero */
		        printf(" (Option %x)\n", db.unit_tag.unit.dn[2]&0xf);
		    else printf("\n");
		    printf("               type: ");
		    switch (db.unit_tag.unit.dt) {
		        case 0:
			    printf("fixed disc\n");
			    break;
			case 1:
			    printf("flexible or removable media\n");
			    break;
			case 2:
			    printf("tape\n");
			    break;
			default:
			    printf("\n");
		    }
		    if (size)	{
			printf("               size: %lu Kbytes\n", size);
			printf("   bytes per sector: %d\n",
					db.unit_tag.unit.nbpb);
		    }
		    else
		        printf("               size: 0 (no media present)\n", size);
		}
		break;

	    case SCSI:
	    case SCSI_TAPE:
	    case AUTOCH:
	    case OPAL:
		do_scsi_inquiry(fildes, fname, BLOCK, VERBOSE);
		break;

	    case UNSUPP_MAJOR:
	    default:
		fprintf(stderr,
		    "%s: %s: unknown disk type (major %d)\n",
		    prog_name, fname, major(stats.st_rdev));
		exit(1);
	}
	exit(0);
}

usage()
{
	fprintf(stderr,
"Usage: %s [ -b | -v ] filename\n\
    <filename>  is a character special device file\n\
\t-b      to obtain disc size (in 1K blocks)\n\
\t-v      verbose mode\n", prog_name);
	exit(1);
}

int getInterfaceType(fd)
int fd;
{
	disk_describe_type disk_des;
	if (ioctl(fd, DIOC_DESCRIBE, &disk_des) != -1) {
		switch(disk_des.intf_type) {
		case CS80_SS_INTF:
		case CS80_MS_INTF:
		case CS80_SM_INTF:
		case CS80_MM_INTF:
		case FLEX_SS_INTF:
		case FLEX_MS_INTF:
		case FLEX_SM_INTF:
		case FLEX_PB_INTF:
		case FLEX_MM_INTF:
			return CS80;
		case SCSI_INTF:
			return SCSI;
		default:
			return UNSUPP_MAJOR;
		}
	}
	return UNSUPP_MAJOR;
}

int get_disk_type(major_num)
int major_num;
{
    struct major_map *table;

#ifdef __hp9000s300
    table = s300_major_map;
#else
    if (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO)
	table = s700_major_map;
    else
	table = s800_major_map;
#endif

    while (table->major_num != major_num && table->major_num != -1)
	table++;

    return table->dev_type;
}

static char *scsi_types[] =
{
    /*  0 */ "direct access",
    /*  1 */ "sequential access",
    /*  2 */ "printer",
    /*  3 */ "processor",
    /*  4 */ "write-once",
    /*  5 */ "CD-ROM",
    /*  6 */ "scanner",
    /*  7 */ "optical memory",
    /*  8 */ "medium changer",
    /*  9 */ "communications",
    /* 10 */ "Graphic Arts Pre-Press (10)",
    /* 11 */ "Graphic Arts Pre-Press (11)"
};

static char *noyes[] = { "no", "yes" };

void
do_scsi_inquiry(fd, fname, block, verbose)
int fd;
char *fname;
int block;
int verbose;
{
    union inquiry_data inq;
    struct capacity capacity;
    int i;
    int tape_device = 0; /* Is set to 1 for sequential access devices. */
    unsigned char *ptr;
    u_long size=0;
    disk_describe_type describe;

    if (ioctl(fd, SIOC_INQUIRY, &inq) < 0)
    {
	fprintf(stderr, "%s: can't SIOC_INQUIRY ", prog_name);
	perror(fname);
	exit(1);
    }

    if (ioctl(fd, SIOC_CAPACITY, &capacity) < 0 ) {
	if ( inq.inq2.dev_type == 1 ){ /* sequential access device */
	    capacity.lba = 0;
	    capacity.blksz = 0;
	    tape_device = 1;
	}
	else {
		/*
		 * The Series 800 SCSI disk driver doesn't support
		 * the SIOC_CAPACITY ioctl() call.  This was done
		 * because someone thought that DIOC_DESCRIBE and
		 * DIOC_CAPACITY ioctl calls would suffice.
		 *
		 * The problem is that the DIOC_CAPACITY only works
		 * on *disks*.  It doesn't work for tape drives.
		 *
		 * To work around this, we try SIOC_CAPACITY first.
		 * If it fails, we then try DIOC_DESCRIBE.
		 * 
		 * We also want to use the DIOC_DESCRIBE to get the 
		 * product id.  On most devices the SIOC_INQUIRY and the
		 * DIOC_DESCRIBE return the same product id, but on
		 * floppy and optical drives, they are different and
		 * we want the DIOC_DESCRIBE product id for newfs.
		 * 
		 */
		 capacity_type cp;

		 if (ioctl(fd, DIOC_DESCRIBE, &describe) < 0)
		 {
			fprintf(stderr, "%s: can't DIOC_DESCRIBE", prog_name);
			perror(fname);
			exit(1);
		 }

		 if (ioctl(fd, DIOC_CAPACITY, &cp) < 0) {
			fprintf(stderr, "%s: can't DIOC_CAPACITY", prog_name);
			perror(fname);
			exit(1);

		 } 
		 size=cp.lba;	
		 if (describe.maxsva > 0)
		   capacity.lba=describe.maxsva+1;   /* must add one get 
							 block per disks */
		 else
		   capacity.lba = 0;
		 capacity.blksz=describe.lgblksz;
	}
    } 
    else { 
	if ( inq.inq2.dev_type == 1 ) /* sequential access device */
		tape_device = 1;
	else if (ioctl(fd, DIOC_DESCRIBE, &describe) < 0)
	{
	  fprintf(stderr, "%s: can't DIOC_DESCRIBE", prog_name);
	  perror(fname);
	  exit(1);
	}
    }

		
    if (!size) {
	if (capacity.blksz != 0 && capacity.blksz < 1024)
		size=capacity.lba/(1024/capacity.blksz);
	else
		size=capacity.lba*(capacity.blksz/1024);
    }	

    if (block)
    {
	printf("%lu\n",
		((u_long)size));
	return;
    }

    printf("SCSI describe of %s:\n", fname);
    if (inq.inq1.added_len < 31)
    {
	printf("   device type %d\n", inq.inq1.dev_type);
	printf("   iso ecma ansi rmb dtq resv rdf\n");
	printf("    %d    %d    %d   %d   %d   %d    %d\n",
		inq.inq1.iso, inq.inq1.ecma, inq.inq1.ansi,
		inq.inq1.rmb, inq.inq1.dtq, inq.inq1.resv,
		inq.inq1.rdf);
	printf("   added_len %d   \n", inq.inq1.added_len);
	ptr = (unsigned char *)(inq.inq1.dev_class);
	for (i = 0; i < inq.inq1.added_len; i++)
	    printf("%x ", *ptr++);
	printf("\n");
    }
    else
    {
	/* Device class currently unused */
	printf("             vendor: %.8s\n", inq.inq1.vendor_id);

     /* describe.model_num cannot be used as product id for
      * sequential access devices as ioctl, DIOC_DESCRIBE fails.
      */
	if (tape_device)
		printf("         product id: %.16s\n", inq.inq1.product_id);
	else
		printf("         product id: %.16s\n", describe.model_num);

	/*
	 * Print the device type.
	 */
	printf("%19s: ", "type");
	if (inq.inq2.dev_type <= 11)
	    printf("%s\n", scsi_types[inq.inq2.dev_type]);
	else
	    if (inq.inq2.dev_type == 0x1f)
		printf("no device type\n");
	    else
		printf("unknown type (%d)\n", inq.inq2.dev_type);

	printf("               size: %lu Kbytes\n",
		(u_long)size);
	printf("   bytes per sector: %d\n", capacity.blksz);
	if (verbose)
	{
	    printf("          rev level: ");
	    if (inq.inq1.added_len >= 31)
		printf("%.4s\n", inq.inq1.rev_num);
	    else
		printf("no revision level\n");
	    printf("    blocks per disk: %d\n", capacity.lba);

	    printf("        ISO version: %1x\n", inq.inq1.iso);
	    printf("       ECMA version: %1x\n", inq.inq1.ecma);
	    printf("       ANSI version: %1x\n", inq.inq1.ansi);

	    printf("%19s: %s\n", "removable media",
		noyes[inq.inq1.rmb]);

	    printf("    response format: %1x\n", inq.inq1.rdf);
	    if (inq.inq1.added_len > 31)
	    {
		ptr = (unsigned char *)&inq.inq1.rev_num[4];
		printf("   (Additional inquiry bytes: ");
		for (i = 32; i < inq.inq1.added_len; i++)
		    printf("(%d)%x ", i, *++ptr);
		printf(")\n");
	    }
	}
    }
}
