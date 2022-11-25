/* HPUX_ID: %W%     %E%  */

/******************************************************************************/
/* Routine to get the status of the ram_disc volumes, reset the access        */
/* counters on individual volumes and do a deallocate of a ram volume.        */
/*                                                                            */
/*                                   - by -                                   */
/*                             Douglas L. Baskins                             */
/*                          System Software Operation                         */
/*                            Fort Collins, Co 80526                          */
/*                                 Dec 10, 1986                               */
/* Note:                                                                      */
/*    This is presently a very good breadboard. (dlb)                         */
/*                                                                            */
/* Revision History:                                                          */
/*                                                                            */
/******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "ram.h"

/* the default name for the ram status dev (must be char dev with minor = 0) */
#define RAM_STATDEV "/dev/ram"

char *stat_dev = RAM_STATDEV;

struct stat statbuf;

int  total_accesses = 0;

/* reset read & write access counters */
int  rflag = 0;
int  dflag = 0;
int  Rflag = 0;
int  Dflag = 0;

extern char *optarg;
extern int optind;

main(argc, argv) 
int argc;
char **argv;
{
	register struct ram_descriptor *rp;
	int fd, dvolume, rvolume;
	int i, rsum, wsum;
	int hflag = 0;
	int errflg = 0;

	while ((i = getopt(argc, argv, "RDr:d:")) != EOF) {
		switch (i) {
		case 'r': rflag++;
			rvolume = atoi(optarg);
			if ((rvolume % RAM_MAXVOLS) != rvolume)
				errflg++;
			break;
		case 'd': dflag++;
			dvolume = atoi(optarg);
			if ((dvolume % RAM_MAXVOLS) != dvolume)
				errflg++;
			break;
		/* dealloc all ram discs */
		case 'D': Dflag++; break;

		/* reset count on all ram discs */
		case 'R': Rflag++; break;

		case '?': errflg++;
		}
	}
	if (optind < argc)
		stat_dev = argv[optind];

	if (stat(stat_dev, &statbuf) < 0) {
		fprintf(stderr, "%s: open of %s failed, Error -- ", 
			argv[0], stat_dev);
		perror("");
		errflg++;
	}
	if (errflg) {
		fprintf(stderr,
"Usage: %s [-R -D -r vol# -d vol# /dev/ram_dev]\n\n", argv[0]);
		fprintf(stderr,
"-R       > Reset access counters on all ram volumes\n");
		fprintf(stderr,
"-D       > Dealloc all ram volumes\n");
		fprintf(stderr,
"-r vol#  > Reset access counters on ram volume\n");
		fprintf(stderr,
"-d vol#  > Dealloc ram volume\n");
		exit(2);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
fprintf(stderr, "%s: Error --  %s  must be a char_special /dev file\n", 
		argv[0], stat_dev);
		exit(2);
	}
	fd = open(stat_dev, O_RDWR);

	/* dealloc all ram volumes */
	if (Dflag) {
		for (dvolume = 0; dvolume < RAM_MAXVOLS; dvolume++) 
			ioctl(fd, RAM_DEALLOCATE, &dvolume);
		exit(0);
	}
	/* reset all ram volumes access counters */
	if (Rflag) {
		for (rvolume = 0; rvolume < RAM_MAXVOLS; rvolume++) 
			ioctl(fd, RAM_RESETCOUNTS, &rvolume);
		exit(0);
	}
	if (rflag) {
		if (ioctl(fd, RAM_RESETCOUNTS, &rvolume) < 0) {
fprintf(stderr, "%s: resetcounts %s of volume %d failed, Error -- ", 
			argv[0], stat_dev, rvolume);
			perror("");
			exit(2);
		}
	}
	if (dflag) {
		if (ioctl(fd, RAM_DEALLOCATE, &dvolume) < 0) {
fprintf(stderr, "%s: deallocate %s of volume %d failed, Error -- ", 
			argv[0], stat_dev, dvolume);
			perror("");
			exit(2);
		}
		exit(0);
	}
	if (read(fd,&ram_device[0], sizeof(ram_device)) != sizeof(ram_device)) {
fprintf(stderr, "%s: read of %s failed, Error -- ", 
		argv[0], stat_dev);
		perror("");
		exit(2);
	}
	for (i = 0, rp = &ram_device[0]; i < RAM_MAXVOLS; i++, rp++) {
		if (rp->size == 0)
			continue;

		/* print header only once if when some data comes */
		if (hflag++ == 0)
			printf(
"Dk# Sz(Kb) Ac Opens 1K    2K    3K    4K    5K    6K    7K    8K  other     sum\n");

		/* output the access information */
		printf("%2d %6d  Rd %2d ", i, rp->size/4, rp->opencount);
		printf("%5d+", rp->rd1k);
		printf("%5d+", rp->rd2k);
		printf("%5d+", rp->rd3k);
		printf("%5d+", rp->rd4k);
		printf("%5d+", rp->rd5k);
		printf("%5d+", rp->rd6k);
		printf("%5d+", rp->rd7k);
		printf("%5d+", rp->rd8k);
		printf("%5d=", rp->rdother);

		rsum = rp->rd1k + rp->rd2k + rp->rd3k + rp->rd4k + rp->rd5k + 
			rp->rd6k + rp->rd7k + rp->rd8k + rp->rdother;

		printf("%8d\n", rsum);

		total_accesses += rsum;

		printf("%2d %6d  Wt %2d ", i, rp->size/4, rp->opencount);
		printf("%5d+", rp->wt1k);
		printf("%5d+", rp->wt2k);
		printf("%5d+", rp->wt3k);
		printf("%5d+", rp->wt4k);
		printf("%5d+", rp->wt5k);
		printf("%5d+", rp->wt6k);
		printf("%5d+", rp->wt7k);
		printf("%5d+", rp->wt8k);
		printf("%5d=", rp->wtother);

		wsum = rp->wt1k + rp->wt2k + rp->wt3k + rp->wt4k + rp->wt5k + 
			rp->wt6k + rp->wt7k + rp->wt8k + rp->wtother;

		printf("%8d\n", wsum);

		total_accesses += wsum;

		if (rp->flag & RAM_RETURN) 
printf("Volume %d scheduled for deallocation when %d opens reach zero\n",
			i, rp->opencount);
	}
	if (total_accesses) {
		printf(
"       TOTAL number of Reads and Writes to the ram discs  =           %9d\n",
	 		total_accesses);
	}
	exit(0);
}
