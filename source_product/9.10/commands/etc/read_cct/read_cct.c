static char *HPUX_ID = "@(#) $Revision: 64.1 $";

/* read_cct.c
 * A user level program to retrieve site_id, site_name from the file
 * /etc/clusterconf and swap device configuration from the file 
 * /etc/swapconf. It runs on root server when a workstation requests
 * to join the cluster and when root server runs /etc/init during boot
 * up.  
 */   

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/unsp.h>
#include <sys/types.h>
#include <cluster.h>

/*
 * this is a temporary kludge because of the two different
 * struct cct_entry definitions in <cluster.h> and <sys/cct.h>
 */
struct kernal_cct {
    u_char machine_id[6];
    cnode_t cnode_id;
    char cnode_name[15];
    cnode_t swap_serving_cnode;
    int kcsp;
};

/*
 * Convert a 6 byte LLA to a 12 character string of hex digits.
 */
char *
hex6toa(s)
unsigned char *s;
{
    static char buf[] = "010203040506";
    char *p = buf;
    register int i;

    for (i = 0; i < 6; i++)
    {
	register int n;

	n = (*s >> 4) & 0x0f;
	*p++ = n < 10 ? n + '0' : n - 10 + 'a';
	n = *s & 0x0f;
	*p++ = n < 10 ? n + '0' : n - 10 + 'a';

	s++;
    }
    *p = '\0';
    return buf;
}

main()
{
	unsp_msg um_msg;
	struct kernal_cct *cctp = (struct kernal_cct *) um_msg;
	struct cct_entry *cct_entry;
	int found, err, i;
	
	if ((err = ioctl (0, UNSP_READ, um_msg))!=0) {
		fputs("read clusterconf: unsp failed\n", stdout);
		ioctl (0, UNSP_ERROR, -1);
		ioctl (0, UNSP_REPLY);
		exit(1);
	} 	

	setccent();
	found = 0;
	while ((cct_entry = getccent()) != NULL) {
		if (memcmp(cctp->machine_id, cct_entry->machine_id, M_IDLEN) == 0) {
			found++;
			break;
		}
	}
	if (!found) {
		fputs("Cannot find machine ", stdout);
		fputs(hex6toa(cctp->machine_id), stdout);
		fputs(" in /etc/clusterconf\n", stdout);
		ioctl(0, UNSP_ERROR, -1);
		ioctl(0, UNSP_REPLY);
		exit(1);
	}

	cctp->cnode_id = cct_entry->cnode_id;
	strcpy(cctp->cnode_name, cct_entry->cnode_name);

/*	cctp->cnode_type = cct_entry->cnode_type; */

	cctp->swap_serving_cnode = cct_entry->swap_serving_cnode;
	cctp->kcsp = cct_entry->kcsp;

	ioctl(0, UNSP_WRITE, um_msg);
	ioctl (0, UNSP_REPLY);
	exit(0);

}
