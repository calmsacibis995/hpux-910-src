#ifdef LINT
#define volatile
#endif LINT

#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>

main (argc, argv)
	int argc;
	char *argv [];
{
	int core_fd;
	char line [80];
	char *cptr;
	char *dptr;
	int  sid;
	int  offset;

	if (argc != 3) {
		fprintf (stderr, "Usage: %s <kernel-file> <kernel-core-file>\n", argv[0]);
		exit (1);
	}
	core_fd = open (argv [2], O_RDONLY);
	init_vm_structures (argv [1], 0, core_fd, 2);

while (1) {
	sid = 0;
	offset = 0;
	fprintf (stdout, "Virtual address (space.offset): ");
	fflush (stdout);

	cptr = gets (line);
	if (cptr == NULL) {
		abort ();
	}

	while ((*cptr == ' ') || (*cptr == '\t')) cptr++;
	dptr = cptr;
	if ((*cptr == '\n') || (*cptr == '\0')) {
		fprintf (stderr, "Invalid virtual address format.\n");
		continue;
	}
	while (isxdigit(*dptr) || (*dptr == 'x')) dptr++;
	*dptr = '\0';
	if ((*cptr == '0') && (*(cptr+1) == 'x')) cptr += 2;
	sscanf (cptr, "%x", &sid);
	cptr = dptr + 1;
	while ((*cptr == ' ') || (*cptr == '\t')) cptr++;
	dptr = cptr;
	if ((*cptr == '\n') || (*cptr == '\0')) {
		fprintf (stderr, "Invalid virtual address format.\n");
		continue;
	}
	while (isxdigit(*dptr) || (*dptr == 'x')) dptr++;
	*dptr = '\0';
	if ((*cptr == '0') && (*(cptr+1) == 'x')) cptr += 2;
	sscanf (cptr, "%x", &offset);
	fprintf (stdout, "Vaddr 0x%X.0x%X,  mapped to phys addr 0x%X.\n", sid, offset, get_phys_addr (sid, offset));
	fflush (stdout);
}
}
