/* HPUX_ID: @(#) $Revision: 70.1 $  */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rbootd.h"
#include "rmp_proto.h"

#ifdef DTC
#include <unistd.h>	/* needed for lockf */
#include <fcntl.h>
#include <errno.h>
#endif /* DTC */

static char DEFAULTDIR[]       = "/usr/boot/"; /* default boot dir */
static char DEFAULTSRMDIR[]    = "/SYSTEMS/";  /* default srm boot dir*/
static char DEFAULTROOT[]      = "/";	       /* default root dir */
static char CLUSTERCONF[]      = "/etc/clusterconf";
static char BOOT_CONFIG_FILE[] = "/etc/boottab";
static char SRM_CONFIG_FILE[]  = "/etc/srmdconf";

static char BC_DEFAULT[]       = "default";
static char BC_CLUSTERDEFAULT[]= "cluster";
static char BC_PAWSDEFAULT[]   = "paws-srm";
static char BC_BASICDEFAULT[]  = "basic-srm";
static char BC_INSTALL[]       = "install";

static char SRM_KEYWORD_1[]    = "VOLUME-TABLE";
static char SRM_KEYWORD_2[]    = "LAN-CLIENTS";
#define VOLUME_NAME_LEN   16
#define BOOT_VOLUME_ADDR  8

static struct cinfo *bc_table[BF_TYPES];
static char *bc_names[BF_TYPES] = {
    (char *)0,		/* BF_SPECIFIC does not have a magic name */
    BC_CLUSTERDEFAULT,
    BC_PAWSDEFAULT,
    BC_BASICDEFAULT,
    BC_INSTALL,
    BC_DEFAULT,
    "DTC"		/* DTC nodes are not listed in boottab */
};

#define BBUFLEN 1024

/*
 * The following constants are used to delay responses to "install"
 * requests for s300 clients.  See the comments in get_client() for
 * more detail.
 */
#define WAITING_TIME	20	/* minimum in seconds */
#define WAITING_WINDOW	30	/* window ends here */

/* structure declaration for srm volume descriptor */

struct volinfo {
    struct volinfo *next;
    char *name;
    char *rootdir;
    int address;
};

#define MAXARGS 32	     /* Actual maximum number of arguments is */
static char *args[MAXARGS];  /* one less than MAXARGS.		      */
static int cline;

/* forward declarations -- externally visible procedures */

void config();
void freecinfocell();
struct cinfo *get_client();

/* forward declarations -- local procedures */

static int convert_aton(), parse(), getline();
static void processbootfiles(), addbootfiles(), expand_bootfiles();
static void config_finish();
static char *stringsave();
#ifdef DTC
static struct cinfo *process_map802record();
#endif /* DTC */
static struct cinfo *initcell(), *dupcinfocell();
static struct cinfo *findcname(), *findclient();
static struct volinfo *find_volume();

long config_time;

/* external declarations */

extern FILE *errorfile;

extern void log();
extern void errexit();
extern char *malloc();
extern char *net_ntoa();

/*
 * free_clients() --
 *    Free up space taken by previous configuration
 */
static void
free_clients()
{
    struct cinfo *tmpcinfoptr;
    struct cinfo *client;
    int i;

    /*
     * The specific client list (BF_SPECIFIC) may have active clients
     * on them, we must free these a little differently than the
     * other types.
     */
    client = bc_table[BF_SPECIFIC];
    while (client != (struct cinfo *)0)
    {
	/*
	 * If client state is C_ACTIVE, change state to C_REMOVE, so
	 * it will be removed by close_session(). Otherwise call
	 * freecinfocell() to free up all of the memory associated
	 * with this cell.
	 */
	tmpcinfoptr = client->nextclient;
	if (client->state == C_ACTIVE)
	{
	    client->state = C_REMOVE;
	    client->nextclient = (struct cinfo *)0;
	}
	else
	    freecinfocell(client);

	client = tmpcinfoptr;
    }
    bc_table[BF_SPECIFIC] = (struct cinfo *)0;

    /*
     * Now free up the rest of the clients.  None of these can be
     * C_ACTIVE, so we can simply free them.
     */
    for (i = BF_SPECIFIC+1; i < BF_TYPES; i++)
    {
	client = bc_table[i];
	while (client != (struct cinfo *)0)
	{
	    tmpcinfoptr = client->nextclient;
	    freecinfocell(client);
	    client = tmpcinfoptr;
	}
	bc_table[i] = (struct cinfo *)0;
    }
}

/*
 * add_cell() --
 *    Add cell to correct client list.
 *    First check to make sure entry does not already exist.
 *    If it does, print error message and ignore this entry.
 *    We use both the client name and client type for this match,
 *    so multiple entries are allowed, e.g.:
 *
 *      cluster:HPS300::listable for 300:more for 300
 *      cluster:HPS700::listable for 700:more for 700
 *      cluster:HPS800::listable for 800:more for 800
 *      cluster:::listable for others:more for 300
 *      node1:HPS300:080009001234:file for node1 as a 300:file2
 *      node1:HPS700:080009001235:file for node1 as a 700:file2
 *      node1:HPS800:080009001236:file for node1 as an 800:file2
 */
static void
add_cell(cinfoptr)
struct cinfo *cinfoptr;
{
    int i;

    for (i = 0; i < BF_TYPES; i++)
    {
	if (findcname(bc_table[i], cinfoptr->name,
		cinfoptr->machine_type, FALSE))
	{
	    log(EL1,
	"Ignored duplicate entry for %s in boot configuration file.\n",
		cinfoptr->name);
	    free(cinfoptr);
	    return;
	}
    }

    /*
     * Look for "magic" entries.
     */
    for (i = BF_SPECIFIC+1; i < BF_TYPES; i++)
    {
	if (strcmp(cinfoptr->name, bc_names[i]) == 0)
	{
	    cinfoptr->nextclient = bc_table[i];
	    bc_table[i] = cinfoptr;
	    cinfoptr->cl_type = i;
	    return;
	}
    }

    /*
     * No match on a "magic" entry, add this cell at front of
     * the specific client list.
     */
    cinfoptr->nextclient = bc_table[BF_SPECIFIC];
    bc_table[BF_SPECIFIC] = cinfoptr;
    cinfoptr->cl_type = BF_SPECIFIC;
}

/*
 * cluster_merge() --
 *    merge information from the cluster configuration file into the
 *    client information from /etc/boottab.
 *
 *    Fills in link address and cnode_id for entries that already
 *    exist from the boot configuration file.
 */
static void
cluster_merge()
{
    extern struct cct_entry *getccent();
    struct cct_entry *cctptr;

    while ((cctptr = getccent()) != (struct cct_entry *)0)
    {
	struct cinfo *tmpcinfoptr;

	/*
	 * Check to see if we have already seen an entry in
	 * /etc/clusterconf with the same LLA as this entry.
	 * If we have, print an error and ignore this entry.
	 */
	for (tmpcinfoptr = bc_table[BF_SPECIFIC];
		 tmpcinfoptr != (struct cinfo *)0 &&
		 memcmp(tmpcinfoptr->linkaddr,
			cctptr->machine_id, ADDRSIZE) != 0;
		     tmpcinfoptr = tmpcinfoptr->nextclient)
	    continue;

	/*
	 * If we get out of the previous loop early, we have already
	 * seen this entry in /etc/clusterconf
	 */
	if (tmpcinfoptr != (struct cinfo *)0)
	{
	    char lla[LADDRSIZE];

	    if (net_ntoa(lla, tmpcinfoptr->linkaddr, ADDRSIZE) == NULL)
		strcpy(lla, "  <unknown>");

	    /* we use lla+2 to skip the leading "0x" */
	    log(EL1, "Duplicate entry for LLA %s in %s\n",
		lla+2, CLUSTERCONF);
	    log(EL1, "    Entry \"%s:%d:%s...\" ignored\n",
		lla+2, cctptr->cnode_id, cctptr->cnode_name);
	    continue;
	}

	/*
	 * See if this cnode has an explicit line in /etc/boottab.
	 * If it does, we add the information from /etc/clusterconf
	 * to the matching entries.  If not, we duplicate the
	 * "cluster" entries (if any) from /etc/boottab.
	 */
	for (tmpcinfoptr = bc_table[BF_SPECIFIC];
		 tmpcinfoptr != (struct cinfo *)0 &&
		 strcmp(tmpcinfoptr->name, cctptr->cnode_name) != 0;
		     tmpcinfoptr = tmpcinfoptr->nextclient)
	    continue;

	/*
	 * If we get out of the previous loop early, we must have an
	 * explicit entry for this cnode, use it instead of any default
	 * "cluster" entries.
	 */
	if (tmpcinfoptr != (struct cinfo *)0)
	{
	    /*
	     * For each client in the client list that matches our
	     * cnode name, we fill in the lla and cnode id fields.
	     * This is because we may have multiple entries, one per
	     * possible machine_type (and possibly one for a default
	     * machine type).
	     */
	    for (; tmpcinfoptr != (struct cinfo *)0;
		       tmpcinfoptr = tmpcinfoptr->nextclient)
		if (strcmp(tmpcinfoptr->name, cctptr->cnode_name) == 0)
		{
		    struct cinfo *default_client;

		    memcpy(tmpcinfoptr->linkaddr,
			    cctptr->machine_id, ADDRSIZE);
		    tmpcinfoptr->cnode_id = cctptr->cnode_id;
#ifdef LOCAL_DISK
		    tmpcinfoptr->swap_site = cctptr->swap_serving_cnode;
#endif /* LOCAL_DISK */
		    default_client = findcname(bc_table[BF_CLUSTER],
			    BC_CLUSTERDEFAULT,
			    tmpcinfoptr->machine_type,
			    FALSE);
		    if (default_client != (struct cinfo *)0)
			addbootfiles(tmpcinfoptr, default_client,
			DEFAULTROOT, BF_CLUSTER);
		    tmpcinfoptr->state = C_INACTIVE;
		}
	}
	else
	{
	    struct cinfo *default_client = bc_table[BF_CLUSTER];

	    /*
	     * For each default cluster in the BF_CLUSTER list
	     * we duplicate an entry for our cnode name and
	     * fill in the lla and cnode id fields.
	     * This is because we may have multiple entries, one per
	     * possible machine_type (and possibly one for a default
	     * machine type).
	     */
	    while (default_client != (struct cinfo *)0)
	    {
		/*
		 * Copy BF_CLUSTER cell
		 */
		tmpcinfoptr = dupcinfocell(default_client, DEFAULTROOT,
			BF_CLUSTER);

		/*
		 * Fill in cnode specific information
		 */
		strcpy(tmpcinfoptr->name, cctptr->cnode_name);
		memcpy(tmpcinfoptr->linkaddr, cctptr->machine_id, ADDRSIZE);
		tmpcinfoptr->cnode_id = cctptr->cnode_id;
#ifdef LOCAL_DISK
		tmpcinfoptr->swap_site = cctptr->swap_serving_cnode;
#endif /* LOCAL_DISK */
		tmpcinfoptr->state = C_INACTIVE;

		/*
		 * Add cell at front of client list
		 */
		tmpcinfoptr->nextclient = bc_table[BF_SPECIFIC];
		bc_table[BF_SPECIFIC] = tmpcinfoptr;

		default_client = default_client->nextclient;
	    }
	}
    }
    endccent();
}

/*
 * process_bootvolume() --
 *    Fill in client information for srm clients.
 */
static void
process_bootvolume(addr, rootdir)
char *addr;
char *rootdir;
{
    int found_a_match = FALSE;
    struct cinfo *tmpcinfoptr;
    struct cinfo *new_clients = NULL;
    struct cinfo *default_entry;
    char name[LADDRSIZE];

    tmpcinfoptr = bc_table[BF_SPECIFIC];
    while (tmpcinfoptr != (struct cinfo *)0)
    {
	struct cinfo *default_client;

	if (memcmp(tmpcinfoptr->linkaddr, addr, ADDRSIZE) == 0)
	{
	    found_a_match = TRUE;
	    tmpcinfoptr->state = C_INACTIVE;

	    default_client = findcname(bc_table[BF_PAWS],
				   BC_PAWSDEFAULT,
				   tmpcinfoptr->machine_type, FALSE);
	    if (default_client != (struct cinfo *)0)
		addbootfiles(tmpcinfoptr, default_client,
			rootdir, BF_PAWS);

	    default_client = findcname(bc_table[BF_BASICWS],
				   BC_BASICDEFAULT,
				   tmpcinfoptr->machine_type, FALSE);
	    if (default_client != (struct cinfo *)0)
		addbootfiles(tmpcinfoptr, default_client,
			rootdir, BF_BASICWS);
	}
	tmpcinfoptr = tmpcinfoptr->nextclient;
    }

    if (found_a_match)
	return;

    net_ntoa(name, addr, ADDRSIZE);

    default_entry = bc_table[BF_PAWS];
    while (default_entry != (struct cinfo *)0)
    {
	tmpcinfoptr = dupcinfocell(default_entry, rootdir, BF_PAWS);
	strcpy(tmpcinfoptr->name, name);
	memcpy(tmpcinfoptr->linkaddr, addr, ADDRSIZE);
	tmpcinfoptr->state = C_INACTIVE;
	tmpcinfoptr->nextclient = new_clients;
	new_clients = tmpcinfoptr;
	default_entry = default_entry->nextclient;
    }

    default_entry = bc_table[BF_BASICWS];
    while (default_entry != (struct cinfo *)0)
    {
	struct cinfo *existing;

	existing = findcname(new_clients, name,
			     default_entry->machine_type,
			     FALSE);
	if (existing != (struct cinfo *)0)
	    addbootfiles(existing, default_entry, rootdir, BF_BASICWS);
	else
	{
	    tmpcinfoptr = dupcinfocell(default_entry,
			      rootdir, BF_BASICWS);
	    strcpy(tmpcinfoptr->name, name);
	    memcpy(tmpcinfoptr->linkaddr, addr, ADDRSIZE);
	    tmpcinfoptr->state = C_INACTIVE;
	    tmpcinfoptr->nextclient = new_clients;
	    new_clients = tmpcinfoptr;
	}
	default_entry = default_entry->nextclient;
    }

    /*
     * Add list of new cells at front of client list
     */
    if (new_clients)
    {
	tmpcinfoptr = new_clients;
	while (tmpcinfoptr->nextclient)
	    tmpcinfoptr = tmpcinfoptr->nextclient;

	tmpcinfoptr->nextclient = bc_table[BF_SPECIFIC];
	bc_table[BF_SPECIFIC] = new_clients;
    }
}

/*
 * srm_merge() --
 *    Add entries from srmconf.  Fill in link address for entries
 *    that already exist from the boot configuration file.
 */
static void
srm_merge()
{
    char bbuf[BBUFLEN];
    FILE *fptr;
    struct volinfo *vptr;
    struct volinfo *master_list;
    int nparams;
    int linelen;
    int i;
    char tmpaddr[ADDRSIZE];

    /*
     * Open srm configuration file
     */
    if ((fptr = fopen(SRM_CONFIG_FILE, "r")) == (FILE *)0)
	errexit("Could not open srm configuration file (%s)\n",
		SRM_CONFIG_FILE);

    /* Find Volume Table Keyword */

    cline = 0;
    while ((linelen = getline(bbuf, BBUFLEN, fptr, SRM_CONFIG_FILE)) != 0)
    {
	if (parse(bbuf, ':') == 1
	  && strcmp(args[0],SRM_KEYWORD_1) == 0)
	    break;
    }

    if (linelen == 0)
	errexit("%s Keyword not found in %s.\n",SRM_KEYWORD_1,SRM_CONFIG_FILE);


    /*
     * First read volume descriptions and create master volume
     * list
     */
    master_list = (struct volinfo *)0;
    while ((linelen = getline(bbuf, BBUFLEN, fptr, SRM_CONFIG_FILE)) != 0)
    {
	if ((nparams = parse(bbuf, ':')) == 1
	  && strcmp(args[0],SRM_KEYWORD_2) == 0)
	    break;

	if (nparams < 3)
	{
	    log(EL1, "Illegal format on line %d of %s. Line ignored.\n",
		    cline, SRM_CONFIG_FILE);
	    continue;
	}

	vptr = (struct volinfo *)malloc(sizeof (struct volinfo));
	if (vptr == (struct volinfo *)0)
	    errexit(MALLOC_FAILURE);

	if ((i = strlen(args[0])) > VOLUME_NAME_LEN)
	{
	    log(EL1, "Volume name(%s) too long on line %d of %s. Line ignored.\n",
		    args[0], cline, SRM_CONFIG_FILE);
	    (void)free((char *)vptr);
	    continue;
	}

	if (i == 0)
	{
	    log(EL1, "Missing Volume name on line %d of %s. Line ignored.\n",
		    cline, SRM_CONFIG_FILE);
	    (void)free((char *)vptr);
	    continue;
	}

	if (*args[1] < '0' || *args[1] > '9')
	    vptr->address = -1;
	else
	    vptr->address = atoi(args[1]);

	if (vptr->address < 0 || vptr->address > 127)
	{
	    log(EL1, "Illegal Volume address on line %d of %s. Line ignored.\n",
		    cline, SRM_CONFIG_FILE);
	    (void)free((char *)vptr);
	    continue;
	}

	vptr->name = stringsave(args[0]);
	vptr->rootdir = stringsave(args[nparams - 1]);

	/* Link Cell to front */

	vptr->next = master_list;
	master_list = vptr;
    }

    if (master_list == (struct volinfo *)0)
	errexit("No volume descriptions found in %s.\n", SRM_CONFIG_FILE);

    if (linelen == 0)
	errexit("Premature EOF encountered in %s.\n", SRM_CONFIG_FILE);

    /* Parse Client Descriptions */

    while (getline(bbuf, BBUFLEN, fptr, SRM_CONFIG_FILE) != 0)
    {
	if ((nparams = parse(bbuf, ':')) == 1)
	    break;

	if (convert_aton(tmpaddr, args[0]) != 0)
	{
	    log(EL1, "Illegal link address (%s) on line %d of %s. Line ignored.\n",
		    args[0], cline, SRM_CONFIG_FILE);
	    continue;
	}

	if ((nparams = parse(args[nparams - 1], ',')) == -1)
	{
	    log(EL1, "Too many  volumes on line %d of %s. Line ignored.\n",
		    cline, SRM_CONFIG_FILE);
	    continue;
	}

	while (nparams-- > 0)
	{
	    vptr = find_volume(master_list, args[nparams]);
	    if (vptr == (struct volinfo *)0)
	    {
		log(EL1, "No volume description for volume %s on line %d of %s. Volume ignored.\n",
			args[nparams], cline, SRM_CONFIG_FILE);
		continue;
	    }

	    if (vptr->address == BOOT_VOLUME_ADDR)
	    {
		process_bootvolume(tmpaddr, vptr->rootdir);
		break;
	    }
	}
    }

    /*
     * Free up volume descriptions from master list
     */
    vptr = master_list;
    while (vptr != (struct volinfo *)0)
    {
	master_list = master_list->next;
	(void)free(vptr);
	vptr = master_list;
    }

    fclose(fptr);
    return;
}

/*
 * expand_bootfiles() --
 *   Go through a list of boot files, expanding cmpname, scanname and
 *   fullname to their appropriate values.
 */
static void
expand_bootfiles(client)
struct cinfo *client;
{
    extern char *strrchr();

    register char *s1, *s2;
    register int bf_type;
    struct bootinfo *bi1ptr;
    struct bootinfo *bi2ptr;
    char tmpname[1024];

    /*
     * Expand boot file names according to type
     */
    bi1ptr = client->bootlist;
    while (bi1ptr != (struct bootinfo *)0)
    {
	bf_type = bi1ptr->type;
	switch (bf_type)
	{
	case BF_DEFAULT:
	case BF_CLUSTER:
	    s1 = bi1ptr->scanname;
	    if (*s1 != '/')
	    {
		s1 = tmpname;
		strcpy(s1, DEFAULTDIR);
		strcat(s1, bi1ptr->scanname);
	    }
	    else
	    {
		s2 = strrchr(s1, '/');
		s2 = stringsave(s2 + 1);
		(void)free(bi1ptr->scanname);
		bi1ptr->scanname = s2;
	    }
	    bi1ptr->cmpname = stringsave(s1);
	    break;
	case BF_PAWS:
	case BF_BASICWS:
	    s1 = bi1ptr->scanname;
	    if (*s1 != '/')
	    {
		if (bi1ptr->scanflag == FALSE)
		{
		    char bfaddr[LADDRSIZE];

		    (void)net_ntoa(bfaddr, client->linkaddr, ADDRSIZE);
		    s2 = bfaddr + (strlen(bfaddr) - 6);
		    s1 = tmpname;
		    if (bf_type == BF_PAWS)
		    {
			sprintf(s1, "/WORKSTATIONS/SYSTEM%s/%s",
				s2, bi1ptr->scanname);
		    }
		    else
		    {
			sprintf(s1, "/SYSTEMS/%s%s",
				bi1ptr->scanname, s2);
		    }

		    bi1ptr->cmpname = stringsave(s1);

		    /*
		     * Allocate another bootinfo cell since we map
		     * each non absolute pathname to two different
		     * absolute pathnames for basic and pascal
		     */
		    bi2ptr = (struct bootinfo *)malloc(sizeof (struct bootinfo));
		    if (bi2ptr == (struct bootinfo *)0)
			errexit(MALLOC_FAILURE);

		    bi2ptr->fullname = stringsave(bi1ptr->fullname);

		    /*
		     * concatenate cmpname to root directory to
		     * create true full absolute pathname.
		     */
		    s1 = bi1ptr->fullname;
		    if (s1[1] == '\0')
			s2 = bi1ptr->cmpname;
		    else
		    {
			s2 = tmpname;
			strcpy(s2, s1);
			strcat(s2, bi1ptr->cmpname);
		    }
		    (void)free(s1);
		    bi1ptr->fullname = stringsave(s2);

		    bi2ptr->scanname = stringsave(bi1ptr->scanname);
		    bi2ptr->scanflag = FALSE;
		    bi2ptr->type = bf_type;

		    /* Link cell into list */

		    bi2ptr->next = bi1ptr->next;
		    bi1ptr->next = bi2ptr;
		    bi1ptr = bi2ptr;

		    s1 = tmpname;
		    if (bf_type == BF_PAWS)
		    {
			sprintf(s1, "/WORKSTATIONS/SYSTEM/%s",
				bi1ptr->scanname);
		    }
		    else
		    {
			sprintf(s1, "/%s",
				bi1ptr->scanname);
		    }

		}
		else
		{
		    s1 = tmpname;
		    strcpy(s1, DEFAULTSRMDIR);
		    strcat(s1, bi1ptr->scanname);
		}
	    }
	    else
	    {
		s2 = strrchr(s1, '/');
		s2 = stringsave(s2 + 1);
		(void)free(bi1ptr->scanname);
		bi1ptr->scanname = s2;
	    }

	    bi1ptr->cmpname = stringsave(s1);
	    break;
	}

	/*
	 * concatenate cmpname to root directory to create true
	 * full absolute pathname.
	 */
	s1 = bi1ptr->fullname;
	if (s1[1] == '\0')
	    s2 = bi1ptr->cmpname;
	else
	{
	    s2 = tmpname;
	    strcpy(s2, s1);
	    strcat(s2, bi1ptr->cmpname);
	}
	(void)free(s1);
	bi1ptr->fullname = stringsave(s2);
	bi1ptr = bi1ptr->next;
    }
}

static void
config_finish()
{
    register struct cinfo *tmpcinfoptr;
    register struct cinfo **cinfoptrptr;
    int i;

    /*
     * Check to make sure that there are not any clients left that we
     * don't have a link address for (state == C_SETUP). If so, issue
     * warning and unlink it from client list.
     *
     * Since we might have multiple entries with the same name (because
     * of the machine_type field), we scan the list and mark all
     * entries with the same name as C_DELETE.  These are deleted but
     * no error message is issued (to avoid multiple error messages for
     * the same node).
     *
     * cinfoptrptr is used to delete entries from the linked list
     * without treating deletion from the head of the list as a special
     * case.
     */
    cinfoptrptr = &bc_table[BF_SPECIFIC];
    while ((tmpcinfoptr = *cinfoptrptr) != (struct cinfo *)0)
    {
	if (tmpcinfoptr->state == C_SETUP ||
	    tmpcinfoptr->state == C_DELETE)
	{
	    if (tmpcinfoptr->state == C_SETUP)
	    {
		struct cinfo *curr = tmpcinfoptr->nextclient;

		log(EL1,
		  "No link address found for node %s. Entry ignored.\n",
		   tmpcinfoptr->name);
		while (curr != (struct cinfo *)0)
		{
		    if (strcmp(tmpcinfoptr->name, curr->name) == 0)
			curr->state = C_DELETE;
		    curr = curr->nextclient;
		}
	    }
	    *cinfoptrptr = tmpcinfoptr->nextclient;
	    freecinfocell(tmpcinfoptr);
	    continue;
	}

	expand_bootfiles(tmpcinfoptr);
	cinfoptrptr = &(tmpcinfoptr->nextclient);
    }

    /*
     * Now expand boot files for the remaining client types.
     */
    for (i = BF_SPECIFIC+1; i < BF_TYPES; i++)
    {
	tmpcinfoptr = bc_table[i];
	while (tmpcinfoptr != (struct cinfo *)0)
	{
	    expand_bootfiles(tmpcinfoptr);
	    tmpcinfoptr = tmpcinfoptr->nextclient;
	}
    }
}

#ifdef DEBUG
static void
bootinfo_print(bootinfoptr)
struct bootinfo *bootinfoptr;
{
    log(EL0, "        scanname \"%s\"\n", bootinfoptr->scanname);
    log(EL0, "        cmpname  \"%s\"\n", bootinfoptr->cmpname);
    log(EL0, "        fullname \"%s\"\n", bootinfoptr->fullname);
    log(EL0, "        scanflag %d\n", bootinfoptr->scanflag);
    log(EL0, "        type     %d\n", bootinfoptr->type);
}

static void
cinfo_print(cinfoptr)
struct cinfo *cinfoptr;
{
    char laddr[LADDRSIZE];
    struct bootinfo *bootinfoptr;

    if (net_ntoa(laddr, cinfoptr->linkaddr, ADDRSIZE) == NULL)
	strcpy(laddr, "<not set>");

    log(EL0, "    name      \"%s\"\n", cinfoptr->name);
    log(EL0, "    addr      %s\n", laddr);
    log(EL0, "    type      \"%s\"\n", cinfoptr->machine_type);
    log(EL0, "    cnode     %d\n", cinfoptr->cnode_id);
#ifdef LOCAL_DISK
    log(EL0, "    swap_site %d\n", cinfoptr->swap_site);
#endif /* LOCAL_DISK */
    log(EL0, "    state     %d\n", cinfoptr->state);
    log(EL0, "    sid       %d\n", cinfoptr->sid);
    log(EL0, "    cl_type   %d\n", cinfoptr->cl_type);
    log(EL0, "    bootlist:\n");

    bootinfoptr = cinfoptr->bootlist;
    while (bootinfoptr != (struct bootinfo *)0)
    {
	bootinfo_print(bootinfoptr);
	bootinfoptr = bootinfoptr->next;
    }
}

static void
config_print()
{
    struct cinfo *tmpcinfoptr;
    int i;

    for (i = 0; i < BF_TYPES; i++)
    {
	log(EL0, "\"%s\" list:\n",
	    bc_names[i] ? bc_names[i] : "specific");

	tmpcinfoptr = bc_table[i];
	while (tmpcinfoptr != (struct cinfo *)0)
	{
	    cinfo_print(tmpcinfoptr);
	    tmpcinfoptr = tmpcinfoptr->nextclient;
	}
    }
}
#endif /* DEBUG */

void
config()
{
    FILE *fptr;
    static char bbuf[BBUFLEN];
    struct cinfo *tmpcinfoptr;
    static time_t bc_time;
    static time_t cc_time = (time_t)0;
    static time_t sc_time = (time_t)0;
#ifdef DTC
    static time_t dtc_time = (time_t)0;
#endif /* DTC */
    static int configflag = FALSE;  /* Have we configured once?    */
    static int srm_present = FALSE; /* Does srm config file exist? */
    static int cluster_present = FALSE; /* Does clusterconf exist? */
#ifdef DTC
    static int dtc_present = FALSE; /* Does dtc config file exist? */
    struct stat dtcbuf;
#endif /* DTC */
    struct stat bcbuf, ccbuf, scbuf;

    /*
     * Check to see if it is necessary to change configuration
     */
    if (stat(BOOT_CONFIG_FILE, &bcbuf) != 0)
    {
	log(EL1, "Could not stat boot configuration file (%s).\n",
		BOOT_CONFIG_FILE);
	return;
    }

    if (stat(CLUSTERCONF, &ccbuf) != 0)
    {
	if (cluster_present)
	{
	    log(EL1,
		"Could not stat cluster configuration file (%s).\n",
		CLUSTERCONF);
	    return;
	}
    }
    else
	cluster_present = TRUE;

    if (stat(SRM_CONFIG_FILE, &scbuf) != 0)
    {
	if (srm_present)
	{
	    log(EL1, "Could not stat srm configuration file (%s).\n",
		    SRM_CONFIG_FILE);
	    return;
	}
    }
    else
	srm_present = TRUE;

#ifdef DTC
    /*
     * if stat fails and if dtc_present is true we log an error and
     * continue instead of returning as is done in the case of srm
     * config file.
     */
    if (stat(DTC_CONFIG_FILE, &dtcbuf) != 0)
    {
	if (dtc_present)
	{
	    log(EL1, "Could not stat dtc configuration file (%s).\n",
		DTC_CONFIG_FILE);
	}
    }
    else
	dtc_present = TRUE;
#endif /* DTC */

    if (configflag)
    {
#ifdef DTC
	if (bcbuf.st_mtime == bc_time &&
	    (cluster_present == FALSE || ccbuf.st_mtime == cc_time) &&
	    (srm_present == FALSE || scbuf.st_mtime == sc_time) &&
	    (dtc_present == FALSE || dtcbuf.st_mtime == dtc_time))
#else
	if (bcbuf.st_mtime == bc_time &&
	    (cluster_present == FALSE || ccbuf.st_mtime == cc_time) &&
	    (srm_present == FALSE || scbuf.st_mtime == sc_time))
#endif /* DTC */

	    return;  /* We don't need to reconfigure */
	log(EL1, "RECONFIGURING\n");
    }

    configflag = TRUE;
    config_time = time(0);
    bc_time = bcbuf.st_mtime;
    if (cluster_present)
	cc_time = ccbuf.st_mtime;
    if (srm_present)
	sc_time = scbuf.st_mtime;
#ifdef DTC
    if (dtc_present)
	dtc_time = dtcbuf.st_mtime;
#endif /* DTC */

    free_clients();

    /*
     * Open and process boot configuration file
     */
    if ((fptr = fopen(BOOT_CONFIG_FILE, "r")) == (FILE *)0)
	errexit("Could not open boot configuration file (%s)\n",
		BOOT_CONFIG_FILE);

    cline = 0;
    while (getline(bbuf, BBUFLEN, fptr, BOOT_CONFIG_FILE) != 0)
    {
	/*
	 * Initialize cell from information in bbuf and add it to
	 * the correct client list.
	 */
	if ((tmpcinfoptr = initcell(bbuf)) != (struct cinfo *)0)
	    add_cell(tmpcinfoptr);
    }
    fclose(fptr);


#ifdef DTC
    /*
     * Open and process DTC configuration file.
     */
    if (dtc_present)
    {
	struct cinfo *tmpdtcinfoptr;
	struct map802record tmpmap802record;
	int map_fd;
	int status;

	/*
	 * open DTC_CONFIG_FILE for reading, if it fails log error
	 * and get out of this if statement.
	 * lock DTC_CONFIG_FILE, if lock fails then do not process
	 * anything, log the fact that lock failed and close
	 * DTC_CONFIG_FILE.  read sizeof(struct map802record) until
	 * EOF into tmpmap802record which is a struct of type
	 * map802record.
	 *   create struct cinfo by using information from the
	 *      record just read by using function process_map802record.
	 *   add the struct cinfo to the global client list by
	 *      using function addcell.
	 * close DTC_CONFIG_FILE on reaching EOF which releases lock.
	 */
	if ((map_fd = open(DTC_CONFIG_FILE, O_RDWR)) < 0)
	{
	    log(EL1, "open failed on %s. errno=%d.\n",
		DTC_CONFIG_FILE, errno);
	}
	else
	{
	    if (lockf(map_fd, F_TLOCK, 0) == 0)
	    {
		while ((status = read(map_fd, (char *)&tmpmap802record,
				      sizeof(struct map802record))) ==
		       sizeof(struct map802record))
		{

		    /* process struct read and make a call to addcell */

		    if ((tmpdtcinfoptr =
			 process_map802record(&tmpmap802record))
						!= (struct cinfo *)0)
			add_cell(tmpdtcinfoptr);
			tmpdtcinfoptr->cl_type = BF_DTC;
		}

		if (status == 0)
		{
#ifdef DEBUG
		    log(EL4, "EOF reached on reading %s.\n",
			DTC_CONFIG_FILE);
#endif /* DEBUG */
		    close(map_fd);
		}
		else
		{
		    log(EL1, "read failed on %s. errno=%d.\n",
			DTC_CONFIG_FILE, errno);
		    close(map_fd);
		}
	    }
	    else
	    {
		log(EL1, "lock failed on %s. errno=%d.\n",
		    DTC_CONFIG_FILE, errno);
		close(map_fd);
	    }
	}
    } /* end if dtc_present */
#endif /* DTC */

    if (cluster_present)
	cluster_merge();

    if (srm_present)
	srm_merge();

    config_finish();

#ifdef DEBUG
    {
	extern int loglevel;
	if (loglevel >= EL7)
	    config_print();
    }
#endif /* DEBUG */
}

#ifdef DTC
static struct cinfo *
process_map802record(map802data)
struct map802record *map802data;
{
    register struct cinfo *tmpdtcinfoptr;

    /*
     * Allocate new cell
     */
    tmpdtcinfoptr = (struct cinfo *)malloc(sizeof (struct cinfo));
    if (tmpdtcinfoptr == (struct cinfo *)0)
	errexit("Could not allocate space for dtc client structure.\n");

    tmpdtcinfoptr->cnode_id = 0;

    /*
     * Get nodename
     */
    memcpy(tmpdtcinfoptr->name, map802data->dtcname, DTCNAMESIZE);

    /*
     * Set machine_type. For DTC it is HP234
     */
    strcpy(tmpdtcinfoptr->machine_type, DTCMACHINETYPE);

    /*
     * Get link address (if not present then set state to C_SETUP,
     * and config_finish will log an error.
     */
    if ((memcmp(map802data->curaddr, "\0\0\0\0\0\0", ADDRSIZE)) == 0)
    {
	tmpdtcinfoptr->state = C_SETUP;
	memset(tmpdtcinfoptr->linkaddr, '\0', ADDRSIZE);
    }
    else
    {
	memcpy(tmpdtcinfoptr->linkaddr, map802data->curaddr, ADDRSIZE);
	tmpdtcinfoptr->state = C_INACTIVE;
    }

    /*
     * Set bootlist. As this field does not apply for the DTC
     * it is set to null.
     */
    tmpdtcinfoptr->bootlist = (struct bootinfo *)0;

    tmpdtcinfoptr->timestamp = config_time - 1;
    return tmpdtcinfoptr;
} /* process_map802record() */
#endif /* DTC */

/*
 * initcell() --
 *    Initialize a single boot entry cell, from information in a line
 *    from /etc/boottab
 */
static struct cinfo *
initcell(bcline)
char *bcline;
{
    static char bootf_err[] =
	"boot configuration file /etc/boottab. Entry ignored.\n";
    register struct cinfo *tmpcinfoptr;
    int nfields;

    /*
     * Allocate new cell
     */
    tmpcinfoptr = (struct cinfo *)malloc(sizeof (struct cinfo));
    if (tmpcinfoptr == (struct cinfo *)0)
	errexit("Could not allocate space for client structure.\n");

    tmpcinfoptr->cnode_id = 0;
#ifdef LOCAL_DISK
    tmpcinfoptr->swap_site = 0;
#endif /* LOCAL_DISK */

    if ((nfields = parse(bcline, ':')) != 5 && nfields != 4)
    {
	log(EL1, "Illegal format on line %d of %s", cline, bootf_err);
	return (struct cinfo *)0;
    }

    /*
     * Get nodename (could be "cluster")
     */
    if (strlen(args[0]) >= CNODENAMESIZE)
    {
	log(EL1, "Nodename (%s) too long in %s", args[0], bootf_err);
	return (struct cinfo *)0;
    }
    strcpy(tmpcinfoptr->name, args[0]);

    /*
     * Get machine_type field (if not present, then set to NULL)
     */
    if (strlen(args[1]) >= MACHINE_TYPE_MAX)
    {
	log(EL1, "Machine type (%s) too long in %s",
	    args[1], bootf_err);
	return (struct cinfo *)0;
    }
    strcpy(tmpcinfoptr->machine_type, args[1]);

    /*
     * Get link address (if not present then set state to C_SETUP.)
     * If link address is not found in clusterconf then we will log
     * an error [later in config_finish()].
     */
    if (*args[2] == '\0')
    {
	tmpcinfoptr->state = C_SETUP;
	memset(tmpcinfoptr->linkaddr, '\0', ADDRSIZE);
    }
    else
    {
	if (convert_aton(tmpcinfoptr->linkaddr, args[2]) != 0)
	{
	    log(EL1, "Illegal link address (%s) in %s",
		    args[2], bootf_err);
	    return (struct cinfo *)0;
	}
	tmpcinfoptr->state = C_INACTIVE;
    }

    /*
     * Get boot configuration files.
     * We first process the 'hidden' ones (Ones we will allow to be
     * booted if requested directly).  We do these first because
     * processbootfiles() inserts the new fields at the head of the
     * list.
     * Then we get the ones that we will return on a scan request.
     */
    tmpcinfoptr->bootlist = (struct bootinfo *)0;
    if (nfields == 5 && *(args[4]) != '\0')
    {				/* we have 'hidden' boot files */
	/*
	 * processbootfiles() may trash args[3] when it parses
	 * args[4].
	 */
	char *savestr = stringsave(args[3]);

	/*
	 * Add the 'hidden' boot files
	 */
	processbootfiles(&tmpcinfoptr->bootlist, args[4], FALSE);

	/*
	 * Now add the visible boot files
	 */
	if (*savestr != '\0')
	    processbootfiles(&tmpcinfoptr->bootlist, savestr, TRUE);

	free(savestr);
    }
    else
    {
	/*
	 * We don't have any 'hidden' boot files, just add the
	 * visible ones.
	 */
	if (*(args[3]) != '\0')
	    processbootfiles(&tmpcinfoptr->bootlist, args[3], TRUE);
    }

    tmpcinfoptr->timestamp = config_time - 1;
    return tmpcinfoptr;
}

static struct cinfo *
copy_client(client)
struct cinfo *client;
{
    struct cinfo *tmpcinfoptr;
    struct bootinfo *cur;
    struct bootinfo *src;

    tmpcinfoptr = (struct cinfo *)malloc(sizeof (struct cinfo));
    if (tmpcinfoptr == (struct cinfo *)0)
	errexit(MALLOC_FAILURE);

    *tmpcinfoptr = *client; /* structure assignment */
    tmpcinfoptr->nextclient = (struct cinfo *)0;

    /*
     * Duplicate the list pointed to by client->bootlist and attach
     * it to tmpcinfoptr.
     */
    cur = (struct bootinfo *)0;
    src = client->bootlist;
    while (src != (struct bootinfo *)0)
    {
	struct bootinfo *new;

	new = (struct bootinfo *)malloc(sizeof (struct bootinfo));

	new->next     = (struct bootinfo *)0;
	new->scanname = stringsave(src->scanname);
	new->cmpname  = stringsave(src->cmpname);
	new->fullname = stringsave(src->fullname);
	new->scanflag = src->scanflag;
	new->type     = src->type;

	if (cur == (struct bootinfo *)0)
	    tmpcinfoptr->bootlist = new;
	else
	    cur->next = new;
	cur = new;
	src = src->next;
    }

    return tmpcinfoptr;
}

/*
 * dupcinfocell() --
 *    make a copy of a cinfo cell.
 */
static struct cinfo *
dupcinfocell(cellptr,rootdir,bf_type)
    struct cinfo *cellptr;
    char *rootdir;
    int bf_type;
{
    struct cinfo *tmpcinfoptr;

    tmpcinfoptr = (struct cinfo *)malloc(sizeof (struct cinfo));
    if (tmpcinfoptr == (struct cinfo *)0)
	errexit(MALLOC_FAILURE);

    *tmpcinfoptr = *cellptr; /* structure assignment */

    tmpcinfoptr->bootlist = (struct bootinfo *)0;
    addbootfiles(tmpcinfoptr,cellptr,rootdir,bf_type);

    return tmpcinfoptr;
}

static void
addbootfiles(dst_cellptr,src_cellptr,rootdir,bf_type)
    struct cinfo *dst_cellptr;
    struct cinfo *src_cellptr;
    char *rootdir;
    int bf_type;
{
    register struct bootinfo *bi1ptr,*bi2ptr,*bi3ptr;

    bi1ptr = src_cellptr->bootlist;

    if (dst_cellptr->bootlist == (struct bootinfo *)0)
	bi2ptr = (struct bootinfo *)0;
    else {
	bi2ptr = dst_cellptr->bootlist;
	while (bi2ptr->next != (struct bootinfo *)0)
	    bi2ptr = bi2ptr->next;
    }

    while (bi1ptr != (struct bootinfo *)0) {

	/* Allocate boot info structure */

	bi3ptr = (struct bootinfo *)malloc(sizeof (struct bootinfo));
	if (bi3ptr == (struct bootinfo *)0)
	    errexit("Could not allocate space for boot info structure.\n");

	bi3ptr->scanname = stringsave(bi1ptr->scanname);
	bi3ptr->cmpname  = (char *)0;
	bi3ptr->fullname = stringsave(rootdir);
	bi3ptr->scanflag = bi1ptr->scanflag;
	bi3ptr->type     = bf_type;
	bi3ptr->next     = (struct bootinfo *)0;

	/* Link at end of boot list */

	if (bi2ptr == (struct bootinfo *)0)
	    dst_cellptr->bootlist = bi3ptr;
	else
	    bi2ptr->next = bi3ptr;

	bi2ptr = bi3ptr;
	bi1ptr = bi1ptr->next;
    }
}

void
freecinfocell(cellptr)
    struct cinfo *cellptr;
{
    struct bootinfo *bi1ptr,*bi2ptr;


    /* Free up bootinfo cells */

    bi1ptr = cellptr->bootlist;

    while (bi1ptr != (struct bootinfo *)0) {
	bi2ptr = bi1ptr->next;
	free(bi1ptr->scanname);
	if (bi1ptr->cmpname != (char *)0)
	    free(bi1ptr->cmpname);
	free(bi1ptr->fullname);
	free(bi1ptr);

	bi1ptr = bi2ptr;
    }

    free(cellptr);
}

static void
processbootfiles(blptrptr,blist,scanflag)
    struct bootinfo **blptrptr;
    char *blist;
    int scanflag;
{
    register int nbootfiles;
    register char *s;
    struct bootinfo *tmpbinfoptr;
    extern char *strchr();

    nbootfiles = parse(blist,',');
    if (nbootfiles == -1) {
	log(EL1,"Too many boot files specified on line %d of %s.\n",
		cline,BOOT_CONFIG_FILE);
	return;
    }

    while (nbootfiles-- > 0) {
	s = args[nbootfiles];

	/*
	 * If pathname is relative it we don't allow it have
	 * subdirectories
	 */
	if (*s != '/' && strchr(s,'/') != (char *)0) {
	    log(EL1,"Illegal boot file name (%s) on line %d of %s. Entry ignored.\n",
		    s,cline,BOOT_CONFIG_FILE);
	    continue;
	}

	/*
	 * Allocate bootinfo cell
	 */
	tmpbinfoptr = (struct bootinfo *)malloc(sizeof (struct bootinfo));
	if (tmpbinfoptr == (struct bootinfo *)0)
	    errexit("Could not allocate space for boot info structure.\n");

	tmpbinfoptr->scanname = stringsave(args[nbootfiles]);
	tmpbinfoptr->cmpname  = (char *)0;
	tmpbinfoptr->fullname = stringsave(DEFAULTROOT);
	tmpbinfoptr->scanflag = scanflag;
	tmpbinfoptr->type     = BF_DEFAULT;

	/*
	 * Link cell at front of list
	 */
	tmpbinfoptr->next = *blptrptr;
	*blptrptr = tmpbinfoptr;
    }
}

static char *
stringsave(string)
char *string;
{
    char *savestring;

    if ((savestring = malloc(strlen(string) + 1)) == (char *)0)
	errexit("Couldn't get space for string in stringsave().\n");

    strcpy(savestring, string);

    return savestring;
}

static struct volinfo *
find_volume(vptr,name)
    register struct volinfo *vptr;
    char *name;
{
    while (vptr != (struct volinfo *)0)
    {
	if (strcmp(vptr->name, name) == 0)
	    return vptr;
	vptr = vptr->next;
    }
    return (struct volinfo *)0;
}

/*
 * findcname() --
 *   find a matching client from the boot configuration information.
 *   Use both client name and machine type for match.  If allow_default
 *   is TRUE, a boot configuration entry with a null machine_type
 *   matches any machine type (but a specific match has priority).
 */
static struct cinfo *
findcname(clients, clientname, machine_type, allow_default)
struct cinfo *clients;
char *clientname;
char *machine_type;
int allow_default;
{
    struct cinfo *tmpcinfoptr = clients;
    struct cinfo *defaultmatch = (struct cinfo *)0;

    while (tmpcinfoptr != (struct cinfo *)0)
    {
	if (strcmp(tmpcinfoptr->name, clientname) == 0)
	{
	    if (strcmp(tmpcinfoptr->machine_type, machine_type) == 0)
		return tmpcinfoptr;
	    if (tmpcinfoptr->machine_type[0] == '\0')
		defaultmatch = tmpcinfoptr;
	}
	tmpcinfoptr = tmpcinfoptr->nextclient;
    }

    return allow_default ? defaultmatch : (struct cinfo *)0;
}

/*
 * findclient() --
 *    find the correct client entry given a lla and the machine type
 *    string.  If machine_type is NULL or "", we don't check it, just
 *    return the first match.
 */
static struct cinfo *
findclient(laddr, machine_type)
char *laddr;
char *machine_type;
{
    register struct cinfo *tmpcinfoptr;
    register struct cinfo *defaultmatch = (struct cinfo *)0;

    tmpcinfoptr = bc_table[BF_SPECIFIC];
    while (tmpcinfoptr != (struct cinfo *)0)
    {
	if (memcmp(tmpcinfoptr->linkaddr,laddr,ADDRSIZE) == 0)
	{
	    if (machine_type == NULL || *machine_type == '\0' ||
		strcmp(tmpcinfoptr->machine_type, machine_type) == 0)
		return tmpcinfoptr;

	    if (tmpcinfoptr->machine_type[0] == '\0')
		defaultmatch = tmpcinfoptr;
	}
	tmpcinfoptr = tmpcinfoptr->nextclient;
    }
    return defaultmatch;
}

struct cinfo *
get_client(laddr, mach_type)
char *laddr;
char *mach_type;
{
    register struct cinfo *tmpcinfoptr;
    register struct cinfo *default_client;
    char clientaddr[LADDRSIZE];
    char machine_type[MACH_TYPE_LEN+1];
    char *s;
    int broadcast_req;
    int is_hps300;

    /*
     * If this was a "broadcast" request, the multicast bit of
     * laddr will be set to 1.
     *
     * We then clear this bit (but are careful to set it back before
     * we return).
     */
    broadcast_req = laddr[0] & 0x01;
    laddr[0] &= ~0x01;

    /*
     * Remove trailing blanks from mach_type field
     */
    strncpy(machine_type, mach_type, MACH_TYPE_LEN);
    machine_type[MACH_TYPE_LEN] = '\0';

    if ((s = strchr(machine_type, ' ')) != NULL)
	*s = '\0';

    /*
     * For broadcast requests, we will need to know if this is an s300,
     * save this information for reference.
     */
    if (broadcast_req)
	is_hps300 = (strcmp(machine_type, "HPS300") == 0);

    /*
     * See if we have a match
     */

#ifdef DTC
    /*
     * A check is made to see if the Machine type begins with HP234.
     * If it does then machine_type is set to NULL so that findclient
     * would then return the first match.
     */
    if ((strncmp(machine_type, DTCMACHINETYPE, DTC_MACH_LEN) == 0) ||
	(strncmp(machine_type, DTC16TYPE, DTC_MACH_LEN) == 0))
	machine_type[0] = '\0';
#endif /* DTC */

    if ((tmpcinfoptr = findclient(laddr, machine_type)) !=
						      (struct cinfo *)0)
    {
	/*
	 * For HPS300 clients, we reject "install" boot requests until
	 * a sufficiently long enough period has elapsed.  If the client
	 * is persistent, we will eventually answer him.
	 *
	 * This is so that most diskless boot requests will work without
	 * going through the "install" path.  If the client is supposed
	 * to be a diskless node, his server will more than likely
	 * answer before our waiting period has elapsed.  Of course, his
	 * server may be down, in which case will will end up answering.
	 * However, the "install" code that we download takes care of
	 * this occurrance too.  The reason for the waiting period is to
	 * minimize the frequency of "incorrect" install responses, not
	 * to eliminate them (unfortunately that is the best that we can
	 * do).
	 *
	 * Note that we really want to answer within a window around
	 * WAITING_TIME.  For example:
	 *   1. a cnode may come up and be booted from his real server
	 *      We saw this request, but rejected it because it was not
	 *      within our window.
	 *   2. Some long time later, the cnode is turned off and then
	 *      turned back on.
	 *   3. We will see this boot request and find that we have
	 *      waited longer than WAITING_TIME and answer -- this
	 *      would not be good.
	 *
	 * To prevent this situation from occurring we only respond in
	 * a window around the WAITING_TIME.  If the request is too
	 * early, we ignore it.  If the request is too late, we reset
	 * the time to the current time (which means we will wait for
	 * another WAITING_TIME period.
	 *
	 * For all clients other than "HPS300":
	 *    If this client is not a specific request, pretend he
	 *    does not exist.  Non-s300 clients (i.e. s700 and s800
	 *    clients) can direct the boot request specifically to
	 *    this server (by interacting with PDC).
	 */
	if (broadcast_req && tmpcinfoptr->cl_type == BF_INSTALL)
	{
	    long waited;
	    long now;

	    /*
	     * Simply reject broadcast INSTALL requests from non-s300
	     * systems.
	     */
	    if (!is_hps300)
		goto reject;

	reject2:
	    now = time(0);
	    waited = now - tmpcinfoptr->timestamp;

	    if (waited < WAITING_TIME || waited > WAITING_WINDOW)
	    {
		extern int loglevel;

		/*
		 * If he did not make it in our window, reset his
		 * timer and make him wait all over again.
		 */
		if (waited > WAITING_WINDOW)
		{
		    tmpcinfoptr->timestamp = now;
		    waited = 0; /* only for printf() msg */
		}

		/*
		 * Reject this request, it is too early to answer him.
		 * If the logging level is detailed, print more info.
		 */
		if (loglevel >= 3)
		{
		    log(EL3,
	    "Rejected %s request from %s, waiting %d more seconds\n",
			BC_INSTALL,
			net_ntoa(clientaddr, laddr, ADDRSIZE),
			WAITING_TIME - waited);

		    /*
		     * Restore the "broadcast" bit.
		     */
		    laddr[0] |= broadcast_req;

		    return (struct cinfo *)0;
		}
		else
		{
		    /*
		     * Terse logging, just log the "standard" reject
		     * message, restore the broadcast bit and return
		     * a NULL pointer indicating we should reject this
		     * boot request.
		     */
		    goto reject;
		}
	    }
	}

	return tmpcinfoptr;
    }

    /*
     * If this was not a broadcast request (i.e. it was addressed
     * specifically to me), see if we have an "install" entry for
     * this machine type.
     *
     * Since the s300/s400 cannot do anything other than send to a
     * broadcast address, we attempt to find an install entry
     * for them no matter what.
     */
    if (!broadcast_req || is_hps300)
    {
	default_client = findcname(bc_table[BF_INSTALL], BC_INSTALL,
			     machine_type, TRUE);
    }
    else
	default_client = (struct cinfo *)0;

    /*
     * If we did not find an "install" cell, see if there is a
     * "default" cell.  If we end up finding either an "install"
     * or "default" cell, we duplicate the cell and add this guy
     * to the client list.
     */
    if (default_client == (struct cinfo *)0)
	default_client = findcname(bc_table[BF_DEFAULT], BC_DEFAULT,
			    machine_type, TRUE);

    if (default_client != (struct cinfo *)0)
    {
	tmpcinfoptr = copy_client(default_client);
	(void)net_ntoa(tmpcinfoptr->name, laddr, ADDRSIZE);
	memcpy(tmpcinfoptr->linkaddr, laddr, ADDRSIZE);
	tmpcinfoptr->state = C_INACTIVE;

	/*
	 * link cell into client list
	 */
	tmpcinfoptr->nextclient = bc_table[BF_SPECIFIC];
	bc_table[BF_SPECIFIC] = tmpcinfoptr;

	if (tmpcinfoptr->cl_type == BF_INSTALL)
	{
	    log(EL2, "Adding new cell for %s client %s.\n",
		BC_INSTALL, tmpcinfoptr->name);

	    /*
	     * If this is an HPS300, we really do not want to answer him
	     * quite yet.  Simply save the time that he sent his first
	     * request and then reject the request.  If he is persistent,
	     * we will answer him later.
	     */
	    if (broadcast_req && is_hps300)
	    {
		tmpcinfoptr->timestamp = time(0);
		goto reject2;
	    }
	}
#ifdef DEBUG
	else
	{
	    extern int loglevel;

	    log(EL4, "Adding new cell for anonymous user %s.\n",
		tmpcinfoptr->name);
	    if (loglevel >= EL7)
		cinfo_print(tmpcinfoptr);
	}
#endif /* DEBUG */

	/*
	 * Restore the "broadcast" bit.
	 */
	laddr[0] |= broadcast_req;

	return tmpcinfoptr;
    }

reject:
    log(EL2, "Rejected boot request from %s\n",
		net_ntoa(clientaddr, laddr, ADDRSIZE));

    /*
     * Restore the "broadcast" bit.
     */
    laddr[0] |= broadcast_req;

    return (struct cinfo *)0;
}

static int
convert_aton(bin_addr,ascii_addr)
char *bin_addr;
char *ascii_addr;
{
    char tmpaddr[LADDRSIZE];
    extern char *net_aton();

    if (strlen(ascii_addr) < LADDRSIZE)
    {
	if (ascii_addr[0] == '0' &&
		(ascii_addr[1] == 'x' || ascii_addr[1] == 'X') &&
		net_aton(bin_addr, ascii_addr, ADDRSIZE) != (char *)0)
	    return 0;

	if (strlen(ascii_addr) < LADDRSIZE-2)
	{
	    tmpaddr[0] = '0';
	    tmpaddr[1] = 'x';
	    strcpy(&tmpaddr[2], ascii_addr);
	    if (net_aton(bin_addr, tmpaddr, ADDRSIZE) != (char *)0)
		return 0;
	}
    }
    return -1;
}

/*
 * parse() -- split a line into a global array (args[]) using "sepchar"
 *	      as a field seperator.
 */
static int
parse(pbuf,sepchar)
    register char *pbuf;
    char sepchar;
{
    register int argcount = 0;

    while (*pbuf != '\0' && argcount < MAXARGS)
    {
	args[argcount++] = pbuf;

	/*
	 * Skip until seperator or null
	 */
	while (*pbuf != sepchar && *pbuf != '\0')
	    pbuf++;

	if (*pbuf != '\0')
	    *pbuf++ = '\0';
    }

    if (argcount == MAXARGS)
	return -1;
    return argcount;
}

/*
 * getline() --
 *    read a line from the stream into buf.
 *    Strips off anything after and including a '#' (comment) or '\n',
 *    whichever comes first.  getline() also removes all white space
 *    and control characters.
 *    The n parameter tells getline() how large buf is.  If the line
 *    being read from the file is longer than n-1 characters, a warning
 *    is printed and the line is ignored.  getline() continues to read
 *    lines until a non-zero length line (after stripping) is read.
 *
 *    Returns the length of the string, or 0 on EOF.
 */
static int
getline(buf,n,stream,filename)
    char *buf;
    int n;
    FILE *stream;
    char *filename;
{
    char *s1, *s2, *strchr();
    int slen;
    int ch;

    do
    {
	if (fgets(buf, n, stream) == (char *)0)
	    return 0;

	cline++;

	/*
	 * strip off comments or trailing newline
	 */
	for (s1 = buf; *s1 && *s1 != '#' && *s1 != '\n'; s1++)
	    continue;

	if (s1 == '\0')
	{
	    *buf = '\0';
	    log(EL1, "Illegal format on line %d of %s. Line ignored.\n",
		    cline, filename);
	}
	else
	    *s1 = '\0';

	/*
	 * Remove all white space and control characters
	 */
	s1 = s2 = buf;
	while (*s2 != '\0')
	{
	    ch = *s2 & 0177;
	    if (ch > ' ' && ch != '\t' && ch != 0177)
		*s1++ = *s2++;
	    else
		s2++;
	}
	*s1 = '\0';

	slen = strlen(buf);
    } while (slen == 0);

    return slen;
}
#ifdef LOCAL_DISK

#include <sys/nsp.h>

/*
 * swap_server_ok(client) --
 *    This routine is used to verify that it is ok to respond to a
 *    boot request.  Its purpose is to determine if we are the rbootd
 *    that should respond to the boot request.
 *
 *    Rootservers respond if:
 *       1.  The client is not going to be a cnode.
 *       2.  The client is going to be a cnode and it swaps locally
 *	     or to us AND the number of csps enabled is >= 1.
 *
 *    Clients respond if:
 *       1.  The client is going to be a cnode and it will swap to
 *	     us AND the number of csps enabled is >= 1.
 */
int
swap_server_ok(client)
register struct cinfo *client;
{
    static char msg[] = "Rejected boot request from ";
    static char fmt[] = "%s%s %s\n";

    extern cnode_t cnodeid();
    static int me = -1;
    cnode_t them = client->cnode_id;
    cnode_t swap_site = client->swap_site;

    if (me == -1)
	me = cnodeid();

    /*
     * If the requesting client isn't going to be a cnode, we answer
     * only if we are the rootserver.
     */
    if (them == 0)
    {
	if (me <= 1)
	    return TRUE; /* not a client, answer the request */

	/*
	 * I'm not the rootserver, I don't answer non-cnode requests
	 */
	log(EL3, fmt, msg, client->name, "(rootserver will answer)");
	return FALSE; /* someone else will answer this request */
    }

    /*
     * If I'm a diskless node, I only answer boot requests from
     * machines that are allowed to join the cluster.  The rootserver
     * handles other types of boot requests.
     */
    if ((me > 1 && swap_site != me) ||
	(me <= 1 && swap_site != me && swap_site != them))
    {
	log(EL3, fmt, msg, client->name, "(I'm not its swap server)");
	return FALSE; /* someone else will answer this request */
    }

    /*
     * We are the one that is supposed to to boot this diskless client.
     * The booting cnode either swaps to our swap disk, or we are the
     * rootserver and the cnode is swapping locally.
     *
     * Either way, ensure that we have at least 1 csp enabled before
     * we answer (just in case someone started us before the csps were
     * enabled).
     */
    if (csp(NSP_CMD_DELTA, 0) < 1)
    {
	log(EL1, fmt, msg, client->name, "(no csps enabled)");
	return FALSE;
    }

    return TRUE; /* the cnode is allowed to boot from us */
}
#endif /* LOCAL_DISK */
