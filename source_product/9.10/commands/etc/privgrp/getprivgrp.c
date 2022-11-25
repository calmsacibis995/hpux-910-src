static char *HPUX_ID = "@(#) $Revision: 37.1 $";

/*
 * Getprivgrp -- find out privileges associated with groups
 *		 with special kernel access.
 * usage:
 *      getprivgrp [-g | group-name]
 *
 * If group-name is -g, the global privilege mask is printed.
 * If group-name is not specified, all privileged group accesses
 * are listed.
 */

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <grp.h>
#include <signal.h>
#include <sys/privgrp.h>
#include "privnames.h"

#define NPRIVS (sizeof(privnames)/sizeof(privnames[0]))
#define register /**/

int null_mask[PRIV_MASKSIZ];
struct privgrp_map privgrp_map[PRIV_MAXGRPS];

main(argc, argv)
    int argc;
    char * argv[];
{
    register int i, group_id;
    int sigsys();
    struct group * grent, *getgrnam();

    signal(SIGSYS, sigsys);

    if (argc >= 3) {
	usage();
	exit(0);
    }

    /*
     * call getprivgrp to get system tables
     */
    if (getprivgrp(privgrp_map)) {
	perror("getprivgrp");
	exit(1);
    }

    if (argc == 2) {
	if (!strcmp(argv[1], "-g"))
	    group_id = PRIV_GLOBAL;
	else {

	    /*
	     * If the user specified a group name, look it up
	     * and print out privileges associated with the specified
	     * group
	     */
	    grent = getgrnam(argv[1]);

	    if (grent == (struct group *)0) {
		/*
		 * No entry for this in the group file
		 */
		usage();
		exit(1);
	    } else
		group_id = grent->gr_gid;
	}
	
	/*
	 * Find the entry associated with this group id
	 */
	for (i = 0; i < PRIV_MAXGRPS; i++)
	    if (privgrp_map[i].priv_groupno == group_id) {
		print_mask(group_id, (unsigned *)privgrp_map[i].priv_mask);
		break;
	    }

	/*
	 * Print a mask of zero if not found
	 */
	if (i == PRIV_MAXGRPS)
	    print_mask(group_id, null_mask);
    } else {
	/*
	 * Print out the privileged group list
	 */
	for (i = 0; i < PRIV_MAXGRPS; i++)
	    print_mask(privgrp_map[i].priv_groupno,
		       (unsigned *)privgrp_map[i].priv_mask);
    }
    exit(0);
}

usage()
{
    register int i;

    fprintf(stderr, "usage: getprivgrp [-g | group-name]\n");
}

/*
 * Print out a privileged group entry
 */
print_mask(group_id, mask)
    int group_id;
    unsigned *mask;
{
    register int i, j, k;
    struct group * grent, *getgrgid();

    switch (group_id) {
    case PRIV_GLOBAL:
	printf("global privileges: ");
	break;
    case PRIV_NONE:
	return;		/* no entry here */
    default:
	grent = getgrgid(group_id);

	if (grent == (struct group *)0) {
	    fprintf(stderr, "getprivgrp: bad groupid in group list (%d)\n", group_id);
	    if (setprivgrp(group_id, null_mask))
		perror("getprivgrp:error occured re-setting group_id");
	    exit(1);
	}
	printf("%s: ", grent->gr_name);
    }

    for (i = 0; i < PRIV_MASKSIZ; i++) {
	for (j = sizeof(int)*8*i + 1; mask[i]; mask[i] >>= 1, j++)
	    if (mask[i]&1)
		for (k = 0; k < NPRIVS; k++)
		    if (privnames[k].number == j)
			printf("%s ", privnames[k].name);
    }
    printf("\n");
}

sigsys()
{
    fprintf(stderr, "getprivgrp: no system call for getprivgrp\n");
    exit(1);
}
