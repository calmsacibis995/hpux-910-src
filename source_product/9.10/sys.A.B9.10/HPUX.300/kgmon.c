/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/REG.300/RCS/kgmon.c,v $
 * $Revision: 1.2.84.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/03/21 16:15:06 $
 */

#include "../h/param.h"
#include "../h/file.h"
#include "../h/vm.h"
#include <stdio.h>
#include <nlist.h>
#include <ctype.h>
#include "../h/gprof.h"

#define	PROFILING_ON	0
#define PROFILING_OFF	3

/*
 * froms is actually a bunch of unsigned shorts indexing tos
 */
u_short	*froms;
struct	tostruct *tos;
char	*s_lowpc;
u_long	s_textsize;
int	ssiz;
off_t	sbuf;

struct nlist nl[] = {
#define N_FROMS		0
	{ "_froms" },
#define	N_PROFILING	1
	{ "_profiling" },
#define	N_S_LOWPC	2
	{ "_s_lowpc" },
#define	N_S_TEXTSIZE	3
	{ "_s_textsize" },
#define	N_SBUF		4
	{ "_sbuf" },
#define N_SSIZ		5
	{ "_ssiz" },
#define	N_TOS		6
	{ "_tos" },
	0,
};

#if defined(vax)
#define	clear(x)	((x) &~ 0x80000000)
#endif

char	*system = "/hp-ux";
char	*kmemf = "/dev/kmem";
int	kmem;
int	bflag, hflag, kflag, rflag, pflag;
int	debug = 0;

main(argc, argv)
	int argc;
	char *argv[];
{
	int mode, disp, openmode = O_RDONLY;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'b':
			bflag++;
			openmode = O_RDWR;
			break;
		case 'h':
			hflag++;
			openmode = O_RDWR;
			break;
		case 'r':
			rflag++;
			openmode = O_RDWR;
			break;
		case 'p':
			pflag++;
			openmode = O_RDWR;
			break;
		case 'd':
			debug++;
			break;
		default:
			fprintf(stderr, "Usage: kgmon [-b -h -r -p a.out kmem]\n");
			fprintf(stderr, "       -b	begin profiling\n");
			fprintf(stderr, "       -h	halt profiling\n");
			fprintf(stderr, "       -r	reset profiling counters\n");
			fprintf(stderr, "       -p	print profiling data to gmon.out\n");									
			fprintf(stderr, "as in \n");	
			fprintf(stderr, "  kgmon -r; kgmon -b; benchmark; kgmon -h; kgmon -p\n"); 
			fprintf(stderr, "followed by \n  gprof /hp-ux gmon.out\n");
			exit(1);
		}
		argc--, argv++;
	}
	if (argc > 0) {
		system = *argv;
		argv++, argc--;
	}
	nlist(system, nl);
	if (nl[0].n_type == 0) {
		fprintf(stderr, "%s: no namelist\n", system);
		exit(2);
	}

#ifdef notdef
	if (argc > 0) {
		kmemf = *argv;
		kflag++;
	}
#endif

	kmem = open(kmemf, openmode);
	if (kmem < 0) {
		openmode = O_RDONLY;
		kmem = open(kmemf, openmode);
		if (kmem < 0) {
			fprintf(stderr, "cannot open ");
			perror(kmemf);
			exit(3);
		}
		fprintf(stderr, "%s opened read-only\n", kmemf);
		if (rflag)
			fprintf(stderr, "-r supressed\n");
		if (bflag)
			fprintf(stderr, "-b supressed\n");
		if (hflag)
			fprintf(stderr, "-h supressed\n");
		rflag = 0;
		bflag = 0;
		hflag = 0;
	}

#ifdef notdef
	if (kflag) {
		off_t off;

		off = clear(nl[N_SYSMAP].n_value);
		lseek(kmem, off, L_SET);
		nl[N_SYSSIZE].n_value *= 4;
		Sysmap = (struct pte *)malloc(nl[N_SYSSIZE].n_value);
		if (Sysmap == 0) {
			perror("Sysmap");
			exit(4);
		}
		read(kmem, Sysmap, nl[N_SYSSIZE].n_value);
	}
#endif

	mode = kfetch(N_PROFILING);
	if (hflag)
		disp = PROFILING_OFF;
	else if (bflag)
		disp = PROFILING_ON;
	else
		disp = mode;
	if (pflag) {
		if (openmode == O_RDONLY && mode == PROFILING_ON)
			fprintf(stderr, "data may be inconsistent\n");
		dumpstate();
	}
	if (rflag)
		resetstate();
	turnonoff(disp);
	fprintf(stdout, "kernel profiling is %s.\n", disp ? "off" : "running");
}

dumpstate()
{
	int i;
	int fd;
	off_t kfroms, ktos;
	int fromindex, endfrom, fromssize, tossize;
	u_long frompc;
	int toindex;
	struct rawarc rawarc;
	char buf[BUFSIZ];

	turnonoff(PROFILING_OFF);
	fd = creat("gmon.out", 0666);
	if (fd < 0) {
		perror("gmon.out");
		return;
	}
	ssiz = kfetch(N_SSIZ);
	sbuf = kfetch(N_SBUF);
	klseek(kmem, (off_t)sbuf, L_SET);
	for (i = ssiz; i > 0; i -= BUFSIZ) {
		read(kmem, buf, i < BUFSIZ ? i : BUFSIZ);
		write(fd, buf, i < BUFSIZ ? i : BUFSIZ);
	}
	s_textsize = kfetch(N_S_TEXTSIZE);
	fromssize = s_textsize / HASHFRACTION;
	froms = (u_short *)malloc(fromssize);
	kfroms = kfetch(N_FROMS);
	klseek(kmem, kfroms, L_SET);
	i = read(kmem, ((char *)(froms)), fromssize);
	if (i != fromssize) {
		fprintf(stderr, "read froms: request %d, got %d", fromssize, i);
		perror("");
		exit(5);
	}
	tossize = (s_textsize * ARCDENSITY / 100) * sizeof(struct tostruct);
	tos = (struct tostruct *)malloc(tossize);
	ktos = kfetch(N_TOS);
	klseek(kmem, ktos, L_SET);
	i = read(kmem, ((char *)(tos)), tossize);
	if (i != tossize) {
		fprintf(stderr, "read tos: request %d, got %d", tossize, i);
		perror("");
		exit(6);
	}
	s_lowpc = (char *)kfetch(N_S_LOWPC);
	if (debug)
		fprintf(stderr, "s_lowpc 0x%x, s_textsize 0x%x\n",
		    s_lowpc, s_textsize);
	endfrom = fromssize / sizeof(*froms);
	for (fromindex = 0; fromindex < endfrom; fromindex++) {
		if (froms[fromindex] == 0)
			continue;
		frompc = (u_long)s_lowpc +
		    (fromindex * HASHFRACTION * sizeof(*froms));
		for (toindex = froms[fromindex]; toindex != 0;
		   toindex = tos[toindex].link) {
			if (debug)
			    fprintf(stderr,
			    "[mcleanup] frompc 0x%x selfpc 0x%x count %d\n" ,
			    frompc, tos[toindex].selfpc, tos[toindex].count);
			rawarc.raw_frompc = frompc;
			rawarc.raw_selfpc = (u_long)tos[toindex].selfpc;
			rawarc.raw_count = tos[toindex].count;
			write(fd, &rawarc, sizeof (rawarc));
		}
	}
	close(fd);
}

resetstate()
{
	int i;
	off_t kfroms, ktos;
	int fromssize, tossize;
	char buf[BUFSIZ];

	turnonoff(PROFILING_OFF);
#ifdef hpux
	memset(buf, 0, BUFSIZ);
#else
	bzero(buf, BUFSIZ);
#endif
	ssiz = kfetch(N_SSIZ);
	sbuf = kfetch(N_SBUF);
	ssiz -= sizeof(struct phdr);
	sbuf += sizeof(struct phdr);
	klseek(kmem, (off_t)sbuf, L_SET);
	for (i = ssiz; i > 0; i -= BUFSIZ)
		if (write(kmem, buf, i < BUFSIZ ? i : BUFSIZ) < 0) {
			perror("sbuf write");
			exit(7);
		}
	s_textsize = kfetch(N_S_TEXTSIZE);
	fromssize = s_textsize / HASHFRACTION;
	kfroms = kfetch(N_FROMS);
	klseek(kmem, kfroms, L_SET);
	for (i = fromssize; i > 0; i -= BUFSIZ)
		if (write(kmem, buf, i < BUFSIZ ? i : BUFSIZ) < 0) {
			perror("kforms write");
			exit(8);
		}
	tossize = (s_textsize * ARCDENSITY / 100) * sizeof(struct tostruct);
	ktos = kfetch(N_TOS);
	klseek(kmem, ktos, L_SET);
	for (i = tossize; i > 0; i -= BUFSIZ)
		if (write(kmem, buf, i < BUFSIZ ? i : BUFSIZ) < 0) {
			perror("ktos write");
			exit(9);
		}
}

turnonoff(onoff)
	int onoff;
{
	off_t off;

	if ((off = nl[N_PROFILING].n_value) == 0) {
		printf("profiling: not defined in kernel\n");
		exit(10);
	}
	klseek(kmem, off, L_SET);
	write(kmem, (char *)&onoff, sizeof (onoff));
}

kfetch(index)
	int index;
{
	off_t off;
	int value;

	if ((off = nl[index].n_value) == 0) {
		printf("%s: not defined in kernel\n", nl[index].n_name);
		exit(11);
	}
	if (klseek(kmem, off, L_SET) == -1) {
		perror("lseek");
		exit(12);
	}
	if (read(kmem, (char *)&value, sizeof (value)) != sizeof (value)) {
		perror("read");
		exit(13);
	}
	return (value);
}

klseek(fd, base, off)
	int fd, base, off;
{

#ifdef notdef
	if (kflag) {
		/* get kernel pte */
		base = clear(base);
		base = ((int)ptob(Sysmap[btop(base)].pg_pfnum))+(base&(NBPG-1));
	}
#endif
	return (lseek(fd, base, off));
}
