/*
 * @(#) $Revision: 70.6 $"
 *
 * sum -- Sum bytes in file mod 2^16
 *
 * AW: 8 Jan 1991
 * POSIX.2 draft 11.2 implementation. NLS support.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#if defined(NLS) || defined (NLS16)
#include <locale.h>
#include <setlocale.h>
#endif

#define WDMSK 0177777L
#define ITG_BSIZE 512	       /* left at 512 for reporting purposes */
#define BUF_SIZE ((int) 8192)  /* size of the input buffer */

#ifndef NLS
#define	catgets(i, sn, mn, s)	(s)
#define	open_cat()
#else
#define	NL_SETN	1			/* set number */
nl_catd	nlmsg_fd;
#endif

struct part
{
    unsigned short hi;
    unsigned short lo;
};

/*
 * this only works right in case short is 1/2 of long
 */
union hilo
{
    struct part hl;
    long	lg;
};

void exit();			/* to make some lints happy	*/
void perror();			/* to make some lints happy	*/
static unsigned long memcrc();	/* CRC a collection of bytes	*/
unsigned long algsum();		/* algorithm for -r option	*/
int errno_save;			/* to save errno */

/* POSIX.2/D11.2 
 * crctab - a fast CRC lookup table
 *
 * The Generating Polynomial is as follows:
 * G(x) = x^32 + x^26 + x^23 + x^22 + x^16 +x^12 +x^11 +x^10 + x^8 +
 *        x^7 + x^5 + x^4 + x^2 + x^1 + x^0
 *
 */
   static unsigned long crctab [] = {
   0x00,
   0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
   0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e,
   0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
   0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d,
   0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0,
   0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63,
   0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
   0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa,
   0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75,
   0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180,
   0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
   0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87,
   0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
   0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5,
   0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
   0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4,
   0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b,
   0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea,
   0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
   0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541,
   0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc,
   0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f,
   0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
   0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e,
   0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
   0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c,
   0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
   0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b,
   0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2,
   0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671,
   0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
   0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8,
   0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767,
   0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6,
   0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
   0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795,
   0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
   0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b,
   0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
   0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82,
   0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d,
   0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8,
   0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
   0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff,
   0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee,
   0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d,
   0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
   0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c,
   0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
   0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02,
   0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*
 * Types of checksum algorithms.
 */
#define DFLT_SUM	0	/* default algorithm (running sum) */
#define POLY_SUM	1	/* CRC polynomial sum (-p)	   */
#define ALG_SUM		2	/* arithmetic shift left 16 bit ?? */

/*
 * buffer for reading data
 */
unsigned char buf[BUF_SIZE];
char *program_name;

#if defined(NLS) || defined(NLS16)
open_cat()
{
static int fflag = 0;

     if (!fflag)
     {
	if(!setlocale(LC_ALL, "")) {
		fputs(_errlocale("sum"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)(-1);
	} else
		nlmsg_fd = catopen("sum", 0);
	fflag = 1;
     }
}
#endif

main(argc, argv)
int argc;
char *argv[];
{
    extern int optind;		/* for getopt */
    extern int opterr;		/* for getopt */
    int c;			/* for getopt */
    int sum_type = DFLT_SUM;
    int errflg = 0;
    char *p;

    int fd;
    register long nbytes;
    register int len;
    register unsigned long sum;
    union hilo suma;

    program_name = argv[0];                 /* name or program (sum/cksum) */
    p = strrchr(program_name, '/');         /* find the last / */
    if (p!=NULL)                            /* if there is indeed one: */
	program_name = p+1;                 /* just use last component */
    if (strcmp(program_name, "cksum") == 0) {
	sum_type = POLY_SUM;		    /* same as sum -p */
	goto c_proc;			    /* continue processing */
    }

    opterr = 0; /* suppress getopt(3c) error messages */
    while ((c = getopt(argc, argv, "rp")) != EOF)
    {
	switch (c)
	{
	case 'r':
	    if (sum_type == POLY_SUM)
		goto Only_one;
	    sum_type = ALG_SUM;
	    break;
	case 'p':
	    if (sum_type == ALG_SUM)
	    {
	    Only_one:
		open_cat();
		fputs(catgets(nlmsg_fd, NL_SETN, 2, "sum: only one of -r or -p may be specified\n"), stderr);
		usage();
	    }
	    sum_type = POLY_SUM;
	    break;
	default:
	    open_cat();
	    usage();
	}
    }

c_proc:
    if (optind >= argc)
    {
	argv[1] = "";
	optind = 1;
	argc = 2;
    }

    do
    {
	if (argv[optind][0] == '\0' ||
	    (argv[optind][0] == '-' && argv[optind][1] == '\0'))
	    fd = 0;
	else
	    if ((fd = open(argv[optind], O_RDONLY)) == -1)
	    {
		errno_save = errno;	/* save errno */
		open_cat();
		fprintf(stderr,catgets(nlmsg_fd, NL_SETN, 3, "%s: can't open "),program_name);
		errno = errno_save;	/* restore errno */
		perror(argv[optind]);
		errflg += 10;
		continue;
	    }

	nbytes = 0;
	switch (sum_type)
	{
	case ALG_SUM:
	    sum = 0;
	    while ((len = read(fd, buf, sizeof buf)) > 0)
	    {
		sum = algsum(buf, len, sum);
		nbytes += len;
	    }
	    break;
	case POLY_SUM:
	    sum = 0;
	    while ((len = read(fd, buf, sizeof buf)) > 0)
	    {
		sum = memcrc(buf, len, sum);
		nbytes += len;
	    }
	    break;
	default:
	    suma.lg = 0;
	    while ((len = read(fd, buf, sizeof buf)) > 0)
	    {
		register unsigned char *b = buf;
		register unsigned char *e = buf + len;

		do
		{
		    suma.lg += *b++;
		} while (b < e);
		nbytes += len;
	    }
	}

	if (len < 0)
	{
	    errflg++;
	    errno_save = errno;		/* save errno */
	    open_cat();
	    fprintf(stderr,catgets(nlmsg_fd, NL_SETN, 4, "%s: read error on "), program_name);
	    errno = errno_save;		/* restore it */
	    perror(argv[optind] ? argv[optind] : "-");
	}
	close(fd);

	switch (sum_type)
	{
	case ALG_SUM:
	    (void)printf("%.5u%6ld",
		sum, (nbytes + ITG_BSIZE - 1) / ITG_BSIZE);
	    break;
	case POLY_SUM:
	    (void)printf("%u %d", sum, nbytes);
	    break;
	default:
	    {
		unsigned long lsavhi;
		unsigned long lsavlo;
		union hilo tempa;

		tempa.lg = (suma.hl.lo & WDMSK) + (suma.hl.hi & WDMSK);
		lsavhi = (unsigned)tempa.hl.hi;
		lsavlo = (unsigned)tempa.hl.lo;
		(void)printf("%u %ld", (unsigned)(lsavhi + lsavlo),
		    (nbytes + ITG_BSIZE - 1) / ITG_BSIZE);
	    }
	}

	if (argv[optind] && argv[optind][0] != '\0')
	{
	    putchar(' ');
	    fputs(argv[optind], stdout);
	}
	(void)putchar('\n');
    } while (++optind < argc);

    return errflg;
}

usage()
{
	fputs(catgets(nlmsg_fd, NL_SETN, 1, "usage: sum [ -r | -p ] [ file ... ]\n"), stderr);
	exit(1);
}

static unsigned long
memcrc(b, n, s)
register unsigned char *b;	/* pointer to data to CRC	*/
int n;				/* number of bytes to CRC	*/
register unsigned long s;	/* old CRC state		*/
{
register unsigned int i, c;

	for(i = n; i > 0; --i) {
		c = (unsigned int) (*b++);
		s = (s << 8) ^ crctab[(s >> 24) ^ c];
	}

	while( n != 0) {
		c = n & 0377;
		n >>= 8;
		s = (s << 8) ^ crctab[(s >> 24) ^ c];
	}

	return(~s);
}

#if !defined(__hp9000s300) && !defined(__hp9000s700) && !defined(__hp9000s800)
unsigned long
algsum(b, n, s)
register unsigned char *b;
int n;
register unsigned long s;
{
    register unsigned char *e = b + n;

    do
    {
	/*
	 * Rotate the sum one bit to the right.  We can
	 * completely ignore whatever is in the high order
	 * 16 bits, as it will not affect the calculations.
	 *
	 * The next byte of input is added *after* the
	 * rotate.
	 */
	if (s & 0x01)
	    s = ((s >> 1) | 0x8000) + *b++; /* set 15 */
	else
	    s = ((s >> 1) & 0x7fff) + *b++; /* clr 15 */
    } while (b < e);

    return s & 0xffff;  /* clear the high order bits */
}
#endif /* not 300, 700 or 800 */
