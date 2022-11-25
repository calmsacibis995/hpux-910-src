/* @(#) $Revision: 66.2 $ */     

#include <stdio.h>
#include <sys/types.h>
#include <nlist.h>
#include <machine/param.h>

struct nlist nl[] = {
	{ "_swdevt" },
#define X_SWDEVT	00
	{ 0 },
};

struct nmlist   {
	long	value;
	char	type;
	char	name[16];
};
struct nmlist nml;

struct swdt {
	dev_t	sw_dev;
	int	sw_freed;
	int	sw_start;
	int	sw_nblks;
};
struct swdt swdt;

main(argc, argv)
int argc;
char **argv;
{
	int  kmem;
	char *kmemf, *nlistf;

	/* open the kernel memory device */
	kmemf = "/dev/kmem";
	kmem = open (kmemf,0);
	if (kmem < 0)
		exit (1);

	/* fetch kernel variable addresses */
	nlistf = "/hp-ux";
	nlist (nlistf,nl);
	if (nl[0].n_type == 0) 
		exit (-1);

        if (lseek (kmem, (long)nl[0].n_value, 0) == -1) 
		exit (-1);
	if (read (kmem, (char *)&swdt, sizeof(struct swdt)) != sizeof(struct swdt)) 
		exit (-1);
	if (strcmp("start", argv[1]) == 0)
		printf("%d", swdt.sw_start);
	else
		if (strcmp("dev",argv[1]) == 0)
			printf("%X", minor(swdt.sw_dev));
		else
			printf("%d", swdt.sw_nblks);
}
