#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/sysmacros.h>
#include <nlist.h>
#include <ndir.h>
#include <string.h>
#include <errno.h>

struct nlist nl[] =
{
#define NL_SWDEVT	0
#ifdef hp9000s800
    { "swdevt" },
#else
    { "_swdevt" },
#endif
    0
};

#define NUMBER_SWAPS	256

extern char *malloc();
extern void exit(), perror();
extern long lseek();
extern int foundInSearchList();
extern int fquiet;			/* -F option for mkrs */


struct swdevt swap[NUMBER_SWAPS];
char *devnames;		/* devnames is the device name of swap[i].sw_dev. */

swapinfo(dev, info, force)
char *dev;
struct swdevt *info;
int force;
{
    int kmemfp;			/* File pointer for /dev/kmem. */
    int i, num_swap_areas;
    struct stat statbuf;
    static int swapstate = 0;
    static int called = 0;

    if (fquiet)
	called = 1;		/* disable all force messages */

    for (i = 0; i < NUMBER_SWAPS; i++) {
	swap[i].sw_dev = swap[i].sw_enable = swap[i].sw_nblks = 0;
	swap[i].sw_start = 0;
    }

    if (nlist("/hp-ux", nl) != 0) {
	if (!called) {
	    (void) fprintf(stderr, "Unable to use nlist on the kernel");
	    (void) fprintf(stderr, " (/hp-ux). There may be an error in\n");
	    (void) fprintf(stderr, " your kernel (or it may be missing).\n");
	    perror("swapinfo");
	}
        if (force) {
	    if (!called) {
		(void) fprintf(stderr, "Force option specified; proceeding\n");
	    }
	    called = 1;
	    return(swapstate);
	}
	exit(1);
    }

    if (nl[0].n_value == 0) {
	if (!called)
	    (void) fprintf(stderr, "Couldn't nlist /hp-ux.\n");
	if (force) {
	    if (!called) 
		(void) fprintf(stderr, "Force option specified; proceeding\n");
	    called = 1;
	    return(swapstate);
	}
	exit(1);
    }

    if ((kmemfp = open("/dev/kmem", O_RDONLY)) < 0) {
	if (!called) {
	    (void) fprintf(stderr, "Couldn't open /dev/kmem. This may happen");
	    (void) fprintf(stderr, " if you are not running as \nsuper-user, or");
	    (void) fprintf(stderr, " the permissions on /dev/kmem are ");
	    (void) fprintf(stderr, "incorrect\n");
	}
	if (force) {
	    if (!called)
		(void) fprintf(stderr, "Force option specified; proceeding\n");
	    called = 1;
	    return(swapstate);
	}
	exit(1);
    }

    (void) lseek(kmemfp, (long)nl[NL_SWDEVT].n_value, L_SET);
    i = 0;
    do {
	(void) read(kmemfp, (char *)&(swap[i]), sizeof(struct swdevt));
    } while ((swap[i].sw_dev != 0xffffffff) && (swap[i].sw_dev != 0) 
	     && (i++ < NUMBER_SWAPS));

    (void) close(kmemfp);
    num_swap_areas = i;
    if(stat(dev, &statbuf) != 0) {
	if (!called) {
	    (void) fprintf(stderr, "Unable to stat %s\n",dev);
	    perror("stat");
	}
	if (force) {
	    if (!called)
		(void) fprintf(stderr, "Force option specified; proceeding\n");
	    called = 1;
	    return(swapstate);
	}
	exit(1);
    }

    for (i = 0; i < num_swap_areas; i++) {
        if(statbuf.st_rdev == swap[i].sw_dev) {
	    info->sw_start = swap[i].sw_start;
	    info->sw_nblks = swap[i].sw_nblks;
	    info->sw_enable = swap[i].sw_enable;
	    swapstate = 1;
	    called = 1;
	    return(swapstate);
        }
    }

    called = 1;
    return(swapstate);
}



