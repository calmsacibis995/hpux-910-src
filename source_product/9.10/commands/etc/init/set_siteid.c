/* HPUX_ID: @(#) $Revision: 64.4 $  */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cluster.h>

#ifndef TRUE
#   define TRUE  1
#   define FALSE 0
#endif

struct cct_entry *getccent();

set_site_id ()				/* this is an archaic name */
{
    struct cct_entry *cct = (struct cct_entry *)NULL;
    struct cct_entry *cct_me;
    int my_id;
    struct cct_entry cct_default;
    register char *cp;
    int diskless = 0;
    struct stat stbuf;

    diskless = stat("/etc/clusterconf", &stbuf) != -1 &&
	       stbuf.st_size != 0;

    /* 
     * This code is executed for both root server and client.
     * Clustering for non-root cluster members are done
     * automagically by the kernel.
     */
    if (diskless)
    {
	while ((cct = getccent()) != NULL)
	    if (cct->cnode_type == 'r')
	    {
		cct_default = *cct;
		cct = &cct_default;
		break;
	    }
	endccent();
    }

    if (cct == NULL)
    {
	if (diskless)
	    console("No rootserver in /etc/clusterconf, assuming standalone.\n");
	strcpy(cct_default.cnode_name, "standalone");
	cct_default.cnode_id = 1;
	cct = &cct_default;
	diskless = FALSE;
    }

    my_id = cct->cnode_id;
    cct_me = cct;

    if (cluster(cct->cnode_id, cct->cnode_name, cct->machine_id) == -1)
	switch (errno)
	{
	case EINVAL: 
	    console(
	      "Illegal root server cnode id value in /etc/clusterconf\n");
	    return;
	case EINPROGRESS: 
	    /*
	     * diskless node.  Figure out who we are.
	     */
	    if ((my_id = cnodeid()) != -1)
		cct_me = getcccid(my_id);
	    break;
	case ENXIO: 
	    console(
"WARNING !  LAN hardware is inconsistent with /etc/clusterconf.\n\
           Edit /etc/clusterconf and re-boot.\n");
	    break;

	default: 
	    do_perror("WARNING: Check /etc/clusterconf.  cluster error");
	    break;
	}

    if (diskless && my_id != cct->cnode_id)
	msg(FALSE, "server id:%4d, server name %s\ncnode id:%5d, cnode name  %s\n",
	    cct->cnode_id, cct->cnode_name,
	    my_id, cct_me ? cct_me->cnode_name : "unknown");
    else
	msg(FALSE, "cnode id:%5d, cnode name:  %s\n",
	    cct->cnode_id, cct->cnode_name);
}
