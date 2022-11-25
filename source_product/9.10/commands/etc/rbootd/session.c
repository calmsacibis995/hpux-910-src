/* HPUX_ID: @(#) $Revision: 70.1 $  */
#include <stdio.h>
#ifdef DTC
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif /* DTC */
#include "rbootd.h"
#include "rmp_proto.h"

extern FILE *errorfile;

extern struct cinfo *freecinfocell();
extern int openbootfile();

void   close_session();
static void setup_session();
#ifdef DTC
static int create_dnldfile();
static int getdtc_dnld_filename();
#endif /* DTC */

/*
 * Session Block:
 *    Used to contain list of open sessions
 *
 *    The session block is  an array of "session" structures containing
 *    state information for each session.  The index (zero not used)
 *    into this array will be the session ID assigned to this session.
 *    When it reaches MAXSESSIONS, it will recycle assigning sessions
 *    with entry one again, always checking for a closed session before
 *    reassigning a session ID.	 An open session is indicated by a
 *    non-NULL value in the client field.  A NULL client field indicates
 *    a closed session.
 *
 *    declared as:  struct session session_blk[MAXSESSIONS+1];
 */
extern int MAXSESSIONS;
extern struct session *session_blk;

/*
 * open_session --
 *    First we check the state flag in the client structure to see if
 *    this client is active. If he is, then the client structure and
 *    session entry will be reset to boot request state.
 *    If the client is inactive, we attempt to allocate the next
 *    available session number indexing the session block array and
 *    marking that entry as in use.  Only looks thru MAXSESSIONS
 *    session ids before giving up.  If it gives up, it returns
 *    SERVER BUSY.  If it gets a session block entry, it fills in the
 *    appropriate session block data, changes the clients state to
 *    active and returns NO ERROR.
 */
int
open_session(client, brequest)
struct cinfo *client;
boot_request *brequest;
{
    int error;
    int bootfd;
    int flength;
    char *cdfpathname;
    int i;
    static short nextsid = 1;
    short timedoutsid = -1;
    long now;
    long max_interval;
    long interval;

    /*
     * Get file name length
     */
    flength = brequest->flength;

    /*
     * check for name length errors.  Actually this is just robust
     * checking, since currently it should be impossible for
     * flength to be bad.  This is due to the fact that the flength
     * field is declared to be unsigned char, so it can't be < 0
     * or greater than 256 (current value of MAXPATHBYTES).
     */
    if (flength > MAXPATHBYTES || flength < 0)
    {
	/*
	 * bad length field; send bad packet reply
	 */
	log(EL1,
	    "Received bad packet (length field trashed).  Client: %s\n",
	    client->name);
	return (BADPACKET);
    }

    brequest->filename[flength] = '\0';

#ifdef DTC
    /*
     * If brequest is from DTC then do a mapping of FCODE to filename
     * (by making a call to getdtc_dnld_filename) before calling
     * openbootfile.  getdtc_dnld_filename sets "buf" to the actual
     * path name of file which will be passed into openbootfile.
     */

    if (client->cl_type == BF_DTC)
    {
	char buf[DTCPATHNAMESIZE];
	int status_get;

	status_get = getdtc_dnld_filename(brequest, client, buf);

#ifdef DEBUG
	log(EL5, "getdtc_dnld_filename returns %d, pathname = %s\n",
	    status_get, buf);
#endif /* DEBUG */

	/*
	 * Try to open boot file to be downloaded for the DTC
	 */
	if ((bootfd = openbootfile(buf, client,
				   brequest->mach_type,
				   &cdfpathname, &error)) < 0) {
	    return error; /* return error code from openbootfile */
	}

	/* For the benefit of RESET_SIC do the following */
	client->dtcsid = brequest->sid;
    }
    else
    {
	if ((bootfd = openbootfile(brequest->filename, client,
				   brequest->mach_type,
				   &cdfpathname, &error)) < 0)
	    return error; /* return error code from openbootfile */
    }
#else
    /*
     * Try to open boot file
     */
    if ((bootfd = openbootfile(brequest->filename, client,
			       brequest->mach_type,
			       &cdfpathname, &error)) < 0)
	return error; /* return error code from openbootfile */
#endif /* DTC */

    if (client->state == C_ACTIVE)
    {
	setup_session(client, client->sid, cdfpathname, bootfd);
	return NOERROR;
    }

    /*
     * Client is inactive, open up a new session
     */
#ifdef DEBUG
    log(EL5, "Looking for an open session\n");
#endif DEBUG

    now = time(0);
    max_interval = SESSION_TIMEOUT;
    for (i = 0; i < MAXSESSIONS; i++)
    {
	if (nextsid > MAXSESSIONS)
	    nextsid = 1;

	/*
	 * see if sid # nextsid is available
	 */
	if (session_blk[nextsid].client == (struct cinfo *)0)
	{
	    setup_session(client, nextsid++, cdfpathname, bootfd);
	    return NOERROR;
	}

	/*
	 * Check to see if we have a session we can time out. We won't
	 * use it unless there are no other available slots and it is
	 * oldest session.
	 */
	interval = now - session_blk[nextsid].client->timestamp;
	if (interval > max_interval)
	{
	    timedoutsid = nextsid;
	    max_interval = interval;
	}
	++nextsid;
    }

    /*
     * Use timed out session if one is available
     */
    if (timedoutsid != -1)
    {
	setup_session(client, timedoutsid, cdfpathname, bootfd);
	return NOERROR;
    }

    log(EL1, "Out of session space.\n");
    close(bootfd);
    return BUSY; /* no available sessions to allocate */
}

static void
setup_session(client, sid, bootfilename, bootfd)
struct cinfo *client;
short sid;
char *bootfilename;
int bootfd;
{
    /*
     * close previous boot file if one is still open
     */
    if (session_blk[sid].bfd != -1)
    {
	log(EL3, "Reuse Session #%d\n", sid);
	(void)close(session_blk[sid].bfd);  /* try to close fd */
#if defined(DTC) && defined(DEBUG)
	log(EL5, "setup_session(): closed file descriptor %d\n",
	    session_blk[sid].bfd);
#endif /* DTC && DEBUG */
    }
    else
	log(EL3, "Open Session #%d\n", sid);

    log(EL3, "	  Client    = %s\n", client->name);
    log(EL3, "	  Boot File = %s\n", bootfilename);

    session_blk[sid].bfd = bootfd;  /* reset file descrip */
    session_blk[sid].curoffset = 0;
    session_blk[sid].client = client;
    client->state = C_ACTIVE;
    client->sid = sid;
    PUNCH_TIMESTAMP(&(client->timestamp));

#ifdef DEBUG
    log(EL5, "CLIENT BLOCK --\n");
    log(EL5, "	  Session id = %d\n", client->sid);
    log(EL6, "	  Client     = %s\n", client->name);
    log(EL5, "	  Client id  = %d\n", client->cnode_id);
    log(EL5, "	  Timestamp  = %d\n", client->timestamp);
    log(EL5, "	  bootfile # = %d\n", session_blk[client->sid].bfd);
#endif DEBUG
}

/*
 * update_session --
 *   check for a valid open session (sid).  If the sid is invalid or
 *   the session is not currently open the error code BADSID is
 *   returned.	BADSID is also returned if sid is valid, but is
 *   assigned to a client with a different address.  Otherwise, NOERROR
 *   is returned and the timestamp is punched.
 */
update_session(sid, from_addr)
int sid;
char *from_addr;
{
    register struct cinfo *client;

    if (sid < 1 || sid > MAXSESSIONS)
    {
	log(EL1, "Tried to update invalid session id, sid = %d\n", sid);
	return BADSID;
    }

    if ((client = session_blk[sid].client) == (struct cinfo*)0)
    {
	/*
	 * session has already been closed
	 */
	log(EL1, "Received request for closed session.\n");
	return BADSID;
    }

    if (memcmp(client->linkaddr, from_addr, ADDRSIZE) != 0)
    {
	/*
	 * session open, but assigned to different client
	 */
	log(EL1, "Received request for an open session from an inactive client.\n");
#ifdef DEBUG
	{
	    extern char *net_ntoa();
	    char buf[LADDRSIZE];

	    log(EL4, "Request for open session from inactive client\n");
	    log(EL4, "	  Session belongs to client %s (%s)\n",
		client->name,
		net_ntoa(buf, client->linkaddr, ADDRSIZE));
	    log(EL4, "	  Requesting client = %s\n",
		net_ntoa(buf, from_addr, ADDRSIZE));
	}
#endif
	return BADSID;
    }

#ifdef DEBUG
    log(EL6, "Updating a session:\n");
    log(EL6, "	  Session id = %d\n", sid);
    log(EL6, "	  Client     = %s\n", client->name);
    log(EL6, "	  offset     = %d\n", session_blk[sid].curoffset);
    log(EL6, "	  timestamp  = %d\n", client->timestamp);
    log(EL6, "	  bootfile # = %d\n", session_blk[sid].bfd);
#endif DEBUG

    PUNCH_TIMESTAMP(&(client->timestamp));
    return NOERROR;
}

/*
 * close_session --
 *    call update session to check for a valid open session, then it
 *    closes out the session.
 */
void
close_session(sid, from_addr)
int sid;
char *from_addr;
{
    struct cinfo *client;

    if (update_session(sid, from_addr) != NOERROR)
	return; /* sid invalid or already closed */

    /*
     * it's a valid open session, so close it
     */
    log(EL3, "Close Session #%d\n", sid);

    /*
     * close file descriptor of requested file if open
     */
    if (session_blk[sid].bfd != -1)
	close(session_blk[sid].bfd);
    session_blk[sid].bfd = -1;

    client = session_blk[sid].client;
    session_blk[sid].client = (struct cinfo *)0;

    if (client->state == C_REMOVE)
	freecinfocell(client);
    else
	client->state = C_INACTIVE;
}

#ifdef DTC

static int
create_dnldfile(configfile, dnldfile)
char *configfile;
char *dnldfile;
{
    /*
     * This procedure is called by getdtc_dnld_filename when rbootd
     * receives a boot request packet from a DTC with an FCODE of
     * CONFIG or SICCONF[0-5] or SECP.
     */

    /*
     * 1. open a file "dnldfile" for create. If file already exists
     *	  truncate the file.
     * 2. get length of file "configfile".
     * 3. write it into a buffer "buf".
     * 4. open configuration file for read. read this file into the
     *	  buffer "buf" at position sizeof(long) and close
     *	  configuration file.
     * 5. write buf into dnldfile.
     * 6. close dnldfile. close configuration file.
     * 7. return 0.
     * (return non zero value if any of the system calls fail)
     */

    char *buf;
    int fd;
    int dnld_fd;
    struct stat sbuf;

    /*
     * Open dnld file. If it exists truncate the file.
     */
    if ((dnld_fd=open(dnldfile, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
    {
#ifdef DEBUG
	log(EL5, "create_dnldfile(): open failed on %s, errno = %d.\n",
	    dnldfile, errno);
#endif /* DEBUG */
	return SCANNOTOPEN;
    }

    /*
     * Obtain size of configuration file.
     */
    if (stat(configfile, &sbuf) == -1)
    {
#ifdef DEBUG
	log(EL5, "create_dnldfile(): stat failed on %s, errno = %d.\n",
	    configfile, errno);
#endif /* DEBUG */
	close(dnld_fd);
	unlink(dnldfile);
	return SFILENOTFOUND;
    }

    if ((buf = (char *)malloc(sbuf.st_size+sizeof (long))) == (char *)0)
	errexit(MALLOC_FAILURE);

    memcpy((char *)buf, (char *)&sbuf.st_size, sizeof (long));

    /*
     * open configuration file to read its contents into buffer.
     */
    if ((fd = open(configfile, O_RDONLY)) == -1)
    {
#ifdef DEBUG
	log(EL5, "create_dnldfile(): open failed on %s, errno = %d.\n",
	    configfile, errno);
#endif /* DEBUG */
	close(dnld_fd);
	unlink(dnldfile);
	return DTCCANNOTOPEN;
    }

    /*
     * read contents of configuration file into buffer.
     */
    if (read(fd, (char *)buf + sizeof (long), sbuf.st_size) !=
							   sbuf.st_size)
    {
#ifdef DEBUG
	log(EL5, "create_dnldfile(): read failed on %s, errno = %d.\n",
	    configfile, errno);
#endif /* DEBUG */
	close(dnld_fd);
	unlink(dnldfile);
	close(fd);
	return -1;
    }

    /*
     * After reading the contents into buffer, you do not need the file
     * descriptor, hence close the file.
     */
    close(fd);

    /*
     * write buffer into dnld file .
     */
    if (write(dnld_fd, buf, sizeof (long) + sbuf.st_size) !=
					   sizeof (long) + sbuf.st_size)
    {
#ifdef DEBUG
	log(EL5, "create_dnldfile(): write failed on %s, errno = %d.\n",
	    dnldfile, errno);
#endif /* DEBUG */
	close(dnld_fd);
	unlink(dnldfile);
	return -1;
    }

    /*
     * If you have reached here dnld file has been created
     * successfully.
     */
    close(dnld_fd);
    return 0;

} /* create_dnldfile() */

static int
getdtc_dnld_filename(packet, client, pathname)
boot_request *packet;
struct cinfo *client;
char *pathname;
{
    /*
     * This procedure is used by open_session on receiving a boot
     * request from a DTC. A mapping between FCODE and filename is
     * done and an absolute pathname is got. If the boot request
     * contains an illegal FCODE then pathname is set to NULL.
     */

    dtcboot_request *brequest = (dtcboot_request *)packet;
    char fcode[20];
    char configfile[DTCPATHNAMESIZE];
    int status;

    strcpy(fcode, brequest->filename);
    strcpy(pathname, DTCMGRDIR);

    if (strncmp(fcode, "CODE", 4) == 0)
    {
	strcat(pathname, "code/cpuconv.cod");
	return 0;
    }


    if (strncmp(fcode, "CONFIG1", 7) == 0)
    {
	strcat(pathname, client->name);
	strcat(pathname, ".dtc/conf/global1");
	strcpy(configfile, pathname);
	strcat(pathname, ".dnld");

	/*
	 * create downloadable file. The DTC expects the length of the
	 * file to be specified in the first four bytes. Hence we need
	 * to insert at the beginning of the file global its length in
	 * order to make it downloadable. We make a call to function
	 * create_dnldfile() to do this and pass back the pathname of
	 * the downloadable file: ...../global.dnld. This file is
	 * removed after an open is made in the function openbootfile().
	 */
	if ((status = create_dnldfile(configfile, pathname)) != 0)
	{
#ifdef DEBUG
	    log(EL5, "create_dnldfile returns %d\n", status);
#endif /* DEBUG */
	    *pathname = '\0';
	    return -1;
	}
	else
	    return 0;
    }

    if (strncmp(fcode, "CONFIG", 6) == 0)
    {
	strcat(pathname, client->name);
	strcat(pathname, ".dtc/conf/global");
	strcpy(configfile, pathname);
	strcat(pathname, ".dnld");

	/*
	 * create downloadable file. The DTC expects the length of the
	 * file to be specified in the first four bytes. Hence we need
	 * to insert at the beginning of the file global its length in
	 * order to make it downloadable. We make a call to function
	 * create_dnldfile() to do this and pass back the pathname of
	 * the downloadable file: ...../global.dnld. This file is
	 * removed after an open is made in the function openbootfile().
	 */

	if ((status = create_dnldfile(configfile, pathname)) != 0)
	{
#ifdef DEBUG
	    log(EL5, "create_dnldfile returns %d\n", status);
#endif /* DEBUG */
	    *pathname = '\0';
	    return -1;
	}
	else
	    return 0;
    }

    /*
     * FCODE = SECP maps to /usr/dtcmgr/acclist. Make a call to
     * create_dnldfile() passing in /usr/dtcmgr/acclist as a parameter.
     * The function create_dnldfile() would create a downloadable file
     * in /usr/dtcmgr/<dtcname>/acclist.dnld. This pathname is passed to
     * openbootfile().
     * openbootfile() opens this file for reading and then unlinks the
     * file to save disc space.
     * This is done as acclist is a common downloadable file and
     * creating a downloadable file in a unique location makes it
     * possible to go ahead and unlink the file after opening it and
     * there by permits rbootd to service more than one DTC at the same
     * time.
     */
    if (strncmp(fcode, "SECP", 4) == 0)
    {
	strcat(pathname, "acclist");
	strcpy(configfile, pathname);

	strcpy(pathname, DTCMGRDIR);
	strcat(pathname, client->name);
	strcat(pathname, ".dtc/acclist.dnld");

	if ((status = create_dnldfile(configfile, pathname)) != 0)
	{
#ifdef DEBUG
    log(EL5, "create_dnldfile returns %d\n", status);
#endif /* DEBUG */
	    *pathname = '\0';
	    return -1;
	}
	else
	    return 0;
    }

    /*
     * Check for SICCONF[0..5]
     */
    if (strncmp(fcode, "SICCONF", 7) == 0)
    {
	strcat(pathname, client->name);
	strcat(pathname, ".dtc/conf/slot");

	switch (fcode[7])
	{
	case '0':
	    strcat(pathname, "0");
	    break;
	case '1':
	    strcat(pathname, "1");
	    break;
	case '2':
	    strcat(pathname, "2");
	    break;
	case '3':
	    strcat(pathname, "3");
	    break;
	case '4':
	    strcat(pathname, "4");
	    break;
	case '5':
	    strcat(pathname, "5");
	    break;
	default:
	    goto Error;
	}
	strcat(pathname, "/tioconf");
	strcpy(configfile, pathname);
	strcat(pathname, ".dnld");

	/*
	 * create downloadable file.
	 */
	if ((status = create_dnldfile(configfile, pathname)) != 0)
	{
#ifdef DEBUG
    log(EL5, "create_dnldfile returns %d\n", errno);
#endif /* DEBUG */
	    *pathname = '\0';
	    return -1;
	}
	else
	    return 0;
    }

    if (strncmp(fcode, "SIC2", 4) == 0)
    {
	strcat(pathname, "code/sic2.cod");
	return 0;
    }

    if (strncmp(fcode, "SIC3", 4) == 0)
    {
	strcat(pathname, "code/sic3.cod");
	return 0;
    }

    if (strncmp(fcode, "SIC4", 4) == 0)
    {
	strcat(pathname, "code/sic4.cod");
	return 0;
    }

    if (strncmp(fcode, "SIC", 3) == 0)
    {
	strcat(pathname, "code/sic.cod");
	return 0;
    }

Error:
    /*
     * if you have reached here that means you have an illegal fcode.
     */
    *pathname = '\0';
    return -1;

} /* getdtc_dnld_filename() */
#endif /* DTC */
