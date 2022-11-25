#ifndef lint
static char	rcsid[] = "@(#)yp_update.c:	$Revision: 1.4.109.1 $	$Date: 91/11/19 14:12:24 $  ";
#endif
/* yp_update.c 90/08/01 */
/*static char sccsid[] = "yp_update.c 1.1 90/08/01 Copyright 1990 Hewlett Packard";*/
/*******************************************************************************
 *
 *	THE YP_UPDATE CODE NEEDS TO BE TESTED MORE as of 08/27/9 - prabha0
 *
 *	YP_UPDATE routines are defined here. They are to be used only with the
 *	YP_CACHE routines defined in yp_cache.c. All these work within the
 *	framework of YP.
 *
 *	YP_UPDATE defines a set of routines which one can use to write services
 *	to send a message to a number of servers. This is useful in maintaining
 *	a distributed data management system.
 *
 *	rpc.yppasswdd has been re-written to use this facility when built
 *	with -DYP_CACHE and -DYP_UPDATE. The usage is explained below with
 *	rpc.yppasswdd (the NIS passwd authenticator) as an example. 
 *
 *	rpc.yppasswdd receives messages from yppasswd for passwd changes. The
 *	changes are authenticated and written into the ascii-source file,
 *	/etc/passwd. Later the changes reach the nis servers (master & slaves)
 *	when a ypmake is done, during which the map is transferred to the
 *	servers. 
 *	
 *	The problem with this approach is that the changes get to ypservers only
 *	when a ypmake is done (usually much later) and that the passwd file has
 *	to be re-written for each change. With a large passwd file, this could
 *	result in a lot of I/O. The following discussion uses rpc.yppasswdd as
 *	an example.
 *
 *	With the YP_UPDATE scheme defined here rpc.yppasswdd (the authenticator)
 *	will write the change into a delta file every time a passwd change is
 *	accepted, and send the change to all ypserv programs. The authenticator
 *	(rpc.yppasswdd for instance) sends messages by calling the send_entry()
 *	routine that sends the message to the servers or the save_n_send_entry
 *	routine that writes the key and value to the delta file and then sends
 *	the change to the servers. send_entry() determines the set of servers to
 *	send to based on the information in the server_list structure defined in
 *	yp_update.h, the file 'db_name' containing the names of the servers,
 *	prognum, procnum and version of the server program (used in talking to
 *	the portmap on the server machine for the server's port) and a pointer
 *	to a client handle. The routine make_server_list makes a list of server
 *	addresses and one client handle to be used to send messages to all
 *	servers. This structure must be filled in by the authenticator and a
 *	pointer passed to the routines here.
 *
 *	The only message that can be sent, is a yp_key_value type message, with
 *	a domain name, mapname, an ascii key and an ascii value string. This
 *	fits the need for all nis messages today. All sorts of NIS like changes
 *	can be propagated like this now. The file yp_cache.c defines the
 *	xdr_ routines to handle the entry. A procnum, YPPROC_UPDATE_MAPENTRY,
 *	has been defined in yp_prot.h for this use. You	may define your own. 
 *
 *	ypservers cache the changes for the given domain and the map. It is
 *	looked up when a lookup request comes along. The cache is invalidated
 *	when a map transfer takes place.
 *	
 *	An entry point, YPUPDATEPROC_MERGE, has been defined in yp_prot.h using
 *	which a message can be sent to the authenticator to request that all the
 *	changes up to now be merged into the ascii file. A program
 *	'merge_updates' (in /usr/etc/yp) sends the message when ever ypmake
 *	is run, if a delta file of size > 0 is present for the authenticator.
 *	'merge_updates' reads the file /usr/etc/yp/map_to_prog to determine
 *	which program to send the merge message to. For instance, the line
 *	"passwd 100009 1 /etc/passwd /etc/pwd_delta.byname" in the map_to_prog
 *	file tells merge_updates that for the passwd changes the prognum to call
 *	is 100009, the version number is 1, the ascii-file is /etc/passwd and
 *	the delta file is /etc/pwd_delta.byname. When the message is received,
 *	the authenticator may verify the arguments and call update_db tp merge
 *	the changes in.
 *
 *	HOW TO ADD NEW SERVICES:
 *
 *	To add your own service, you must first write the authenticator to be
 *	run on the NIS master. The authenticator must know the server's prognum,
 *	version etc. The authenticator talks to the portmap to find the servers'
 *	port. You must add the a line to the map_to_prog file with the name of
 *	the map (as used in ypmake), the prognum, version number and the
 * 	delta_file. You may have to have a program that can send changes to the
 *	authenticator (like yppasswd to send passwd changes to rpc.yppasswdd).
 *	Then you have to make a data base (like ypservers, for the ypserv
 *	programs) with the names of the machines on which the servers are
 *	running and should receive the update messages and place it in the
 *	domain directory under /usr/etc/yp. When you send a message to the
 *	authenticator, that message may be sent to all the servers running and
 *	may be added to the delta_file. The changes may	be merged on request on
 *	based on command line arguments.
 *	(see rpc.ypasswd.c).
 *
 *	EXTERNALLY CALLABLE ROUTINES:
 *
 *	make_cache_entry	returns a ptr to a cache entry made from inputs
 *	get_default_domain_name	returns default domain name
 *	update_db		merges and cache's the changes
 *	send_entry		send an entry to the servers
 *	save_n_send_entry	saves entry in deltafile, and send it to servers
 *	get_serverlist		makes list of all servers
 *	free_serverlist		frees up the serverlist struct
 *	init_update		initialise update, make serverlist etc.
 *	xdr_merge_req	xdr'ises the merge_req struct for send/recv
 *
 ******************************************************************************/

#ifndef YP_CACHE
#undef  YP_UPDATE
#endif	/* YP_CACHE */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#ifdef  YP_UPDATE
#include <sys/stat.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <sys/time.h>
#include "yp_cache.h"
#include "yp_update.h"
#endif /* YP_UPDATE */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
#endif /* NLS */

extern int	errno;

#ifdef  YP_UPDATE
extern char		*strchr();
extern int		set_cache_ptrs();
extern void		detachentry();
extern entrylst 	*add_to_ypcachemap();
extern entrylst 	*search_ypcachemap();
extern void 		remove_ypcachemap();

static long		t_loc, seq_id	= 0;
static u_long		default_prognum = YPPROG;	/* ypserv prog */
static u_long		default_procnum = YPPROC_UPDATE_MAPENTRY;
static u_long		default_version	= YPVERS;
static char		*default_db_name= "ypservers";

static struct entrylst	entry;

/*The global struct entry is filled in with data & a ptr to entry is returned.*/

entrylst *
make_cache_entry(key, val, entry_type)
char	*key, *val;
int	entry_type;
{
	/* form an entry to be written to file and/or sent to servers */
	entry.seq_id			 = t_loc + seq_id++;
	entry.key_val.status		 = entry_type;	/* sticky */
	entry.key_val.keydat.dptr	 = key;
	entry.key_val.keydat.dsize	 = strlen(key);
	entry.key_val.valdat.dptr	 = val;
	entry.key_val.valdat.dsize	 = strlen(val);
	entry.next			 = NULL;
	return(&entry);
}

/*
 * It copies & caches the entry e. The entry e is left intact.
 */

static int	
cache_entry(domainname, mapname, e)
char	*domainname;
char	*mapname;
entrylst *e;
{
	entrylst * p;

	/*
         * if already in list, delete it. call detachentry with prev = 0
	 * because search_ypcachemap moves the entry to the front of the list.
         */

	if ((p = search_ypcachemap(domainname, mapname, e->key_val.keydat)) != NULL) {
		/* search has set the cache_ptrs. call detachentry */
		detachentry(0, p);
		free (p);
	}

	p = add_to_ypcachemap (domainname, mapname, e->key_val.keydat, e->key_val.valdat, 1);

	if (p == (entrylst * )0) { /* memory not available. */
#ifdef DEBUG
		fprintf(stderr,"cache_entry: out of memory!\n");
#endif
		set_map_state(domainname, mapname, BAD_STATE);
		return (YPUPDATE_NOMEM);	/* no memory */
	}
	return(0);	/* no error */
}

/* The following are defined in rpc_ypassd.c too. The following
 * version will be used when YP_UPDATE is defined - prabha */

static FILE *
open_lockfile (lock_file)
char	*lock_file;
{
	int	tempfd;
	static FILE    *tempfp;

	(void) umask(0);

	tempfd = open(lock_file, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (tempfd < 0) {
		if (errno == EEXIST) {
#ifdef DEBUG
			fprintf(stderr,"open_lockfile: password file busy - try again\n");
#endif
		} else {
#ifdef DEBUG
			fprintf(stderr,"open_lockfile: error opening %d:  errno = %d\n", lock_file, errno);
#endif
		}
		return((FILE * )0);
	}

	if ((tempfp = fdopen(tempfd, "w")) == NULL) {
#ifdef DEBUG
		fprintf(stderr,"open_lockfile: fdopen of %s failed\n", lock_file);
#endif
		close (tempfd);
		return((FILE * )0);
	}
	return (tempfp);
}

static int	
replace_asciifile(lock_file, ascii_file)
char	*lock_file, *ascii_file;
{
	char	cmdbuf[BUFSIZ];
	void	(*f1)(), (*f2)(), (*f3)(), (*f4)();
	int	err;

	f1 = signal(SIGHUP,  SIG_IGN);
	f2 = signal(SIGINT,  SIG_IGN);
	f3 = signal(SIGQUIT, SIG_IGN);
	f4 = signal(SIGCLD,  SIG_IGN);


	if ((err = rename(lock_file, ascii_file)) < 0) {

		/* the rename may have failed if the two files are
                   on different systems; if so, try a move operation */

		sprintf(cmdbuf, "/bin/mv %s %s", lock_file, ascii_file);
		if (system(cmdbuf)) {
#ifdef DEBUG
			fprintf(stderr,"replace_asciifile: error renaming %1$s to %2$s:  errno = %d\n", lock_file, ascii_file, errno);
#endif
			unlink(lock_file);      /* since mv did not succeed */
			err = YPUPDATE_RENAME; /* /bin/mv system call failed */
		} else
			err = 0;

	}

	signal(SIGHUP,  f1);
	signal(SIGINT,  f2);
	signal(SIGQUIT, f3);
	signal(SIGCLD,  f4);

	return (err);
}

/*
 * Write an entire list of CHANGES, in the map pointed to by mp, into the
 * ascii_file specified. The key in each entry in the list is compared with the
 * beginning of each line in the ascii_file until the separator char 'c' is
 * seen and the matching entries are replaced with ones in the list. Note that
 * only CHANGES are incorporated; that is, an entry with that key must be there
 * already in the ascii_file, for it to be replaced. If an entry exists in the
 * maplist with no corresponding entry in the ascii_file, we do not add it to
 * the ascii_file.
 *
 * lock_file is opened for exclusive access, to prevent multiple writers, and
 * the ascii_file is copied over to it, with changes from the maplist pulled in.
 * Finally, the lock_file is renamed as the ascii_file.
 */
static int	
write_list_to_db (mp, ascii_file, lock_file, c)
maplst	*mp;
char	*lock_file, *ascii_file, c;
{
	char	*name, buf[BUFSIZ+1], *p;

	entrylst * e, *prev;
	int	len, found  = 0;
	FILE	 * dbfp = (FILE * )0, *tempfp = (FILE * )0;

	if (mp->firstentry) {
		(void) umask(0);
		tempfp = open_lockfile(lock_file);
		if (tempfp == NULL) {
#ifdef DEBUG
			fprintf(stderr,"write_list_to_db: couldn't open temp file\n");
#endif
			return (YPUPDATE_OPENTMP); /* coulnt open temp file */
		}
		/* open file to read from */
		if ((dbfp = fopen(ascii_file, "r")) == NULL) {
#ifdef DEBUG
			fprintf(stderr,"write_list_to_db: fopen of %s failed\n", ascii_file);
#endif
			fclose(tempfp);
			return(YPUPDATE_ASCII);	/* couldnt open ascii file */
		}
		while (fgets(buf, sizeof(buf), dbfp)) {

			/* look in the buf for the separator char 'c' */
			p = strchr(buf, c);

			/* look for a match with name in the
		 * list and replace the coresponding line */

			found	 = 0;	/* asume no match */
			prev	 = (entrylst * )0;
			for (e = mp->firstentry; e != NULL; e = e->next) {
				len	  = e->key_val.keydat.dsize;
				name	  = e->key_val.keydat.dptr;

				/* find & modify the line that has matching login name*/
				if (p && ((p - buf) == len) && 
				    (memcmp(name, buf, len) == 0)) {

#ifdef DEBUG
					fprintf(stderr,tempfp, "%s\n", e->key_val.valdat.dptr);
#endif

					/* detach & throw the entry away from the list*/
					detachentry(prev, e);
					free_entry(&e);
					found = 1;	/* indicate match */
					break;
				}
				prev = e;
			}

			if (found == 0)	/* no match - keep the line as is */
				fputs(buf, tempfp);
		}

		if (mp->firstentry != NULL) {
#ifdef DEBUG
			fprintf(stderr,"write_list_to_db: List not empty even after EOF on the ascii_file.\n");
			print_map(mp);
#endif /* DEBUG */
		}

		fclose(dbfp);
		fclose(tempfp);

		/*rename lock_file to ascii_file;
		 * on failure try /bin/mv lock_file ascii_file */

		return(replace_asciifile(lock_file, ascii_file));
	} else
		return (0);	/* No entries to write, ok */
}

static int	
truncate_deltafile(dfp)
FILE	*dfp;
{
	int	dfd, err = 1;

	if (dfp != NULL) {
		/* truncate the file to 0 size */
		dfd =  fileno(dfp);
		err =  ftruncate(dfd, 0);
		if (err != 0) {
#ifdef DEBUG
			fprintf(stderr,"truncate_deltafile: can't truncate delta file\n");
#endif
			err = YPUPDATE_TRUNCATE; /* ftruncate on delta file failed */
		}
	} else 
#ifdef DEBUG
		fprintf(stderr,"truncate_deltafile: NULL file pointer\n");
#endif

	return (err);
}


/* This builds cache map from the open stream dfp */

static bool
rebuild_map(domainname, mapname, dfp)
char	*mapname;
FILE	*dfp;
{
	char	*key, *value, *p, buf[BUFSIZ];
	entrylst * ep;
	struct stat stb;
	int	state;
	int	err, dfd;

	(void) remove_ypcachemap (domainname, mapname);
	if (dfp) { /* dfp != NULL */
		dfd = fileno(dfp);
		err = fstat(dfd, &stb);
		if (err) {
#ifdef DEBUG
			fprintf(stderr,"rebuild_map: errno %d from fstat on deltafile\n", errno);
#endif
			return (YPUPDATE_FSTATDEL);	/* fstat on delta file failed */
		} else {
			if (stb.st_size > 0) {
				err = lseek(dfd, 0, SEEK_SET);
				while (fgets(buf, BUFSIZ - 1, dfp) != NULL) {
					key = buf;
					p = strchr(buf, '\t');
					*p = '\0';
					value = ++p;
					ep  = make_cache_entry(key, value, STICKY);
					err = cache_entry(domainname, mapname, ep);
					if (err != 0) {
#ifdef DEBUG
						fprintf (stderr,"rebuild_map: cant cache mapname entry.\n");
#endif
						continue;
					}
				}
			} else {
				return (0);
			}
		}
	} else {
#ifdef DEBUG
		fprintf(stderr,"rebuild_map: dfp (delta_file ptr) is NULL\n");
#endif
		return(YPUPDATE_NULLDFP);	/* delta file ptr is NULL */
	}
	state = get_map_state(domainname, mapname);
	if (state == BAD_STATE) {
#ifdef DEBUG
		fprintf(stderr,"rebuild_map: mapname_state of %s: BAD_STATE\n");
#endif
		remove_ypcachemap (domainname, mapname);
		return (YPUPDATE_BADMAP);	/* bad mapname */
	}
	return (0);
}


/*
 * this routine called when a req to update (from ypmake) is recvd or when
 * the count of changes exceed a preset maximum. The write_list_to_db call
 * writes all entries in the cached map (rebuilt from the delta_file, if need
 * be), to the ascii_file.
 *
 * An open file handle to the delta file is expected.
 */

update_db(upd_ptr)
yp_update_struct *upd_ptr;
{
	char	*domainname	= (upd_ptr->mrg_msgp)->domain;
	char	*mapname	= (upd_ptr->mrg_msgp)->map;
	char	*ascii_file	= ((upd_ptr->mrg_msgp)->adf).ascii_file;
	char	sep_char	= ((upd_ptr->mrg_msgp)->adf).separator;
	char	*lock_file	=  upd_ptr->lock_file;
	FILE	*dfp		=  upd_ptr->dfp;
	int	err = 0, dfd;
	int	cache_ok;
	struct	stat stb;

	if (dfp == NULL) {
#ifdef DEBUG
		fprintf(stderr,"update_db: pointer to deltafile is NULL\n");
#endif
		return (YPUPDATE_NULLDFP);
	}

	dfd = fileno(dfp);
	err = fstat(dfd, &stb);

	if (err) {
#ifdef DEBUG
		fprintf(stderr,"update_db: errno %d from stat on dfd (=%d)\n", errno, dfd);
#endif
		return (YPUPDATE_FSTATDEL);
	} else {
		if (stb.st_size > 0) {
			cache_ok = get_map_state (domainname, mapname);
			if ((cache_ok == BAD_STATE) || (cache_ok < 0)) {
#ifdef DEBUG
				fprintf(stderr,"update_db: cache map is no good\n");
#endif
				cache_ok = rebuild_map(domainname, mapname, dfp);
				if ((cache_ok == BAD_STATE) || (cache_ok < 0)) {
#ifdef DEBUG
					fprintf(stderr,"update_db: nis cache map %s not ok\n", mapname);
#endif
					return (YPUPDATE_BADMAP);
				}
			}
			(void) set_cache_ptrs(domainname, mapname);
			(void) get_mapptr (mapname, &mapptr);
			if (mapptr) {
				if (mapptr->firstentry) {
					err = write_list_to_db(mapptr, ascii_file, lock_file, sep_char);
					if (err == 0)
						err =  truncate_deltafile(dfp);
				} else { /* non-null deltafile, null map */
#ifdef DEBUG
					fprintf(stderr,"update_db: empty ypcache map %s, deltafile not empty !\n", mapname);
#endif
					return (YPUPDATE_DELTAERR);	/* delta file not empty, cache map bad */
				}
			} else {	/* non-null delta file, null map ? */
#ifdef DEBUG
				fprintf(stderr,"update_db: NULL mapptr for %s\n", mapname);
#endif
				return (YPUPDATE_NULLMAPPTR); /* null mapptr */
			}
		}
	}
	return (err);	/* from truncate_deltafile */
}


/*
 *  This gets the local kernel domainname, and sets the global domain to it.
 */

char	*
get_default_domain_name()
{
	char	*domain;

	if (!getdomainname(default_domain_name, (YPMAXDOMAIN + 1))) {
		domain = default_domain_name;
	} else {
#ifdef DEBUG
		fprintf(stderr,"get_default_domain_name: Can't get domainname from system call.\n");
#endif
		return (NULL);
	}
	if (strlen(domain) == 0) {
#ifdef DEBUG
		fprintf(stderr,"get_default_domain_name: the domainname hasn't been set on this machine.\n");
#endif
		return (NULL);
	}

	return (default_domain_name);
}


/*
 *  This adds a single server to the server list.  The servers name is
 *  translated to an IP address by calling gethostbyname(3n), which will
 *  probably make use of nis services.
 */

static void
add_server(slp, name, namelen)
server_list *slp;
char	*name;
int	namelen;
{
	server_entry *ps;
	struct hostent *h;
	u_short port;
	char	*domainname = slp->domain;
	u_long prognum = slp->prognum, version = slp->version;

	if (domainname == NULL)
		domainname = get_default_domain_name();

	if (prognum == 0)
		prognum = default_prognum;	/* YPPROG */

	if (version == 0)
		version = default_version;

	ps = (server_entry *) malloc((unsigned) sizeof (server_entry));
	if (ps == 0) {
		perror("add_server: malloc failure");
		exit(1);
	}

	name[namelen] = '\0';
	(void) strncpy(ps->name, name, namelen);

	ps->namelen = namelen;
	if ((h = (struct hostent *) gethostbyname(name)) != NULL) {
		strcpy(ps->domb.dom_domain, domainname);
		(ps->domb).dom_server_addr.sin_addr = *(struct in_addr *) h->h_addr;
		ps->domb.dom_server_addr.sin_family = AF_INET;
		port = pmap_getport(&((ps->domb).dom_server_addr), prognum, version, IPPROTO_UDP);
		/* note that getport may return 0. We still make the entry & fix it later */
		(ps->domb).dom_server_port = ((ps->domb).dom_server_addr).sin_port = port;
		ps->domb.dom_socket = RPC_ANYSOCK;
		ps->domb.dom_client = (CLIENT * )0;
		ps->next = slp->se_list;
		slp->se_list = ps;
	} else {
#ifdef DEBUG
		fprintf(stderr,"can't get an address for server %s.\n", name);
#endif
		free(ps);
	}
}

void
free_serverlist(slp)
server_list *slp;	/* pointer to server-list  to be free'd */
{
	server_entry *p, *s;

	if (slp->client)
		clnt_destroy (slp->client);

	slp->client = (CLIENT * )0;

	for (s = slp->se_list; s != (server_entry *)0; ) {
		p = s;
		s = s->next;
		p->next = NULL;

		if (p->name)
			free(p->name);

		if (p->domb.dom_domain)
			free (p->domb.dom_domain);

		if (p->domb.dom_client)
			clnt_destroy (p->domb.dom_client);

		free(p);
	}
	slp->se_list = (server_entry *)0;
}

/*
 * This uses nis operations to retrieve each server name in the map
 * "db_name". add_server is called for each one to add it to the
 * list of servers.
 */

static int
make_serverlist(slp)
server_list *slp;
{
	char	*key, *outkey, *val, *domainname = slp->domain, *db_name = slp->db_name;
	int	keylen, 	outkeylen, vallen, err;

	if (domainname == NULL)
		domainname = get_default_domain_name();

	if (db_name == NULL)
		db_name = default_db_name;

	if (err = yp_bind(domainname) ) {
#ifdef DEBUG
		fprintf(stderr,"make_serverlist: can't find a nis server for domain %s.  Reason:  %s.\n", domainname, yperr_string(err));
#endif
		return(YPUPDATE_YPBIND); /* yp_bind on domain failed */
	}

	if (err = yp_first(domainname, db_name, &outkey, &outkeylen, &val, &vallen)) {
#ifdef DEBUG
		fprintf(stderr,"make_serverlist: can't build server list from map \"%s\".  reason:  %s.\n", db_name, yperr_string(err));
#endif
		return(YPUPDATE_YPFIRST);	/* yp_first failed */
	}
	while (TRUE) {
		add_server(slp, outkey, outkeylen);
		free(val);
		key = outkey;
		keylen = outkeylen;

		if (err = yp_next(domainname, db_name, key, keylen,
		    &outkey, &outkeylen, &val, &vallen) ) {
			if (err == YPERR_NOMORE) {
				break;
			} else {
#ifdef DEBUG
				fprintf(stderr,"make_serverlist: can't build server list from map \"%s\".  reason:  %s.\n",
					     db_name, yperr_string(err));
#endif
				free_serverlist(slp);
				return(YPUPDATE_YPNEXT); /* yp_next failed */
			}
		}
		free(key);
	}
	return (0);
}

int	
get_serverlist(slp)
server_list *slp;
{
	char	server_db[128], *domainname = slp->domain, *db_name = slp->db_name;
	int	err, len;
	long	mtime;
	struct stat stb;

	if (domainname == NULL)
		domainname = get_default_domain_name();

	if (db_name == NULL)
		db_name = default_db_name;
	else {
		len = strlen(db_name);
#ifdef DEBUG
		if (len > 9)
			fprintf(stderr,"strlen(%s) > 9; wont work on short file name systems\n", db_name);
#endif
	}

	strcpy(server_db, "/usr/etc/yp/");
	strcat(server_db, domainname);
	strcat(server_db, "/");
	strcat(server_db, db_name);
	strcat(server_db, ".pag");

	/* stat server_db.dir to see if it has changed */
	err = stat(server_db, &stb);
	if (err == 0) {
		mtime = stb.st_mtime;
		if (mtime != slp->mtime) {/* compare modify times */
			slp->mtime = mtime;
			if (slp->se_list != NULL) {/* we already have a list */
				free_serverlist(slp);
			}
			err = make_serverlist(slp);

		} else { /* modify times are the same, check if list is ok */
			if (slp->se_list == NULL) {
				return(make_serverlist(slp));
			} else {
				return (0);
			}
		}
	} else {
#ifdef DEBUG
		fprintf(stderr,"get_serverlist: stat on %s returned errno = %d\n", server_db, errno);
#endif
		return (YPUPDATE_DBSTAT); /* stat on db_file failed */
	}
	return (0);
}


static int	
sendto_servers(slp, inproc, in)
server_list *slp;
xdrproc_t inproc;
char	*in;
{
	u_long	prognum = slp->prognum, version = slp->version, procnum = slp->procnum;
	server_entry *s;
	struct timeval tot_timeout, timeout;
	int	err = 0, port, 	sock, result;
	char	err_msg[128];
	struct cu_data *cu;
	CLIENT	 * cl;


	if (prognum == 0)
		prognum = default_prognum;	/* YPPROG - ypserv prog */

	if (version == 0)
		version = default_version;

	if (procnum)
		procnum = default_procnum;

	err = get_serverlist(slp);
	if (err == 0) {
		s = slp->se_list;
		if (s == NULL) { /* se_list == NULL */
#ifdef DEBUG
			fprintf(stderr,"send_to_servers: NULL server list !\n");
#endif
			return (YPUPDATE_NULLSLIST);    /* NULL server list */
		}
		cl = slp->client;
		if (cl == NULL) {	/* try to get a CLIENT handle */
			while ((cl == NULL) && (s != NULL)) {
				sock = RPC_ANYSOCK;
				timeout.tv_usec = 0;
				timeout.tv_sec  = 5;
				if ((cl = slp->client = clntudp_create (&(s->domb.dom_server_addr),
				    prognum, version, timeout, &sock)) == NULL){

				    s = s->next;
				}
			}
			if (cl == NULL) {
#ifdef DEBUG
				fprintf(stderr,"send_to_servers: Cant get a client handle\n");
#endif
				return (YPUPDATE_NULLCL); /* NULL Client */
			}
		}

		cu = (struct cu_data *) cl->cl_private;

		/*
		 * The port number may be bad already, because a server could
		 * have died and come back to life with another port number by
		 * now. How do we handle that ?
		 */

		for (s = slp->se_list; s != 0; s = s->next) {
			port = (s->domb).dom_server_port;
			if (port == 0) { /* get it's port now */
				port = pmap_getport(&((s->domb).dom_server_addr), prognum, version, IPPROTO_UDP);
				if (port)
					((s->domb).dom_server_addr).sin_port = port;
				else {
#ifdef DEBUG
					fprintf(stderr,"ypserv port on %s = 0 ! Not registered ?\n");
#endif
					continue;
				}
			}

			/* copy the ip-addr over to cu; port # is already in it */

			cu->cu_raddr = (s->domb).dom_server_addr;
			cu->cu_rlen  = sizeof (cu->cu_raddr);

			cu->cu_total.tv_sec = -1;
			cu->cu_total.tv_usec = -1;

			tot_timeout.tv_sec  = 5;
			tot_timeout.tv_usec = 0;

			s->status = (enum clnt_stat) clnt_call (cl, procnum, inproc, in, xdr_int, &result, tot_timeout);
			if (s->status != RPC_SUCCESS) {
				sprintf(err_msg, "sendto_servers: sending to %s ", s->name);
				clnt_perror(cl, err_msg);
				err++;
			}

		}
	} else {
#ifdef DEBUG
		fprintf(stderr,"error from get_server_list\n");
#endif
		return (err);
	}

	if (err) {	/* from clnt_calls */
		return (YPUPDATE_SENDERR);	/* errors in send */
	}

	return (0);
}


/* send_entry sends eactly one entry to all servers. slp has
 * the name of the server db with names of servers in it. */

send_entry(upd_ptr, ep)
yp_update_struct *upd_ptr;
entrylst *ep;
{
	char	*mapname 	= (upd_ptr->mrg_msgp)->map;
	char	*domainname	= (upd_ptr->mrg_msgp)->domain;
	server_list *slp	=  upd_ptr->slp;
	update_entry upd_msg;
	struct ypreq_nokey d_m;
	int	err = 0;

	if (domainname == NULL)
		domainname = get_default_domain_name();

	/* ep must contain only one entry; detect error if not */

	if (ep->next != (entrylst * )0)
		err = YPUPDATE_SENDENTRY;
	else {
		d_m.domain	 = domainname;
		d_m.map		 = mapname;
		upd_msg.dm	 = &d_m;
		upd_msg.ep	 = ep;
		err = sendto_servers (slp, xdr_update_entry, &upd_msg);
		if (err != 0) {
#ifdef DEBUG
			fprintf(stderr,"send_entry: %d failures.\n",err);
#endif
		}
	}
	return(err);
}


/*
 * write the key/value pair to the stream (dfp),
 * cache the entry and send it to all servers.
 */

save_n_send_entry(upd_ptr, e)
yp_update_struct	*upd_ptr;
register entrylst *e;
{
	char	*mapname 	= (upd_ptr->mrg_msgp)->map;
	char	*domainname	= (upd_ptr->mrg_msgp)->domain;
	server_list *slp	=  upd_ptr->slp;
	FILE	*dfp		=  upd_ptr->dfp;
	int	res = 1, err, dfd;

	if (domainname == NULL)
		domainname = get_default_domain_name();

	if (dfp != NULL) {	/* Then delta file is open; write to it.
						 * Assume the write will succeed. */
		dfd = fileno(dfp);
		lseek(dfd, 0, SEEK_END);
#ifdef DEBUG
		fprintf(dfp, "%s\t%s\n", e->key_val.keydat.dptr, e->key_val.valdat.dptr);
#endif
		fflush(dfp);
		res = 0;
	} else {
#ifdef DEBUG
		fprintf(stderr,"save_n_send_entry: save_n_send_entry: file pointer passed in is NULL.\n");
#endif
		res = 7;
	}

	/* cache entry in map and send a message to all servers */
	err = cache_entry(domainname, mapname, e);
	if (err == 0) {		/* map entry is cached */
		err = send_entry(upd_ptr, e);
		if (err != 0) {	/* convert uid field to string */
#ifdef DEBUG
			fprintf(stderr,"save_n_send_entry: send_entry returned error(%d).\n", err);
#endif
			/* we will not return an error for this failure. */
		}

	} else { /* error in cacheing - mark map bad */
		set_map_state(domainname, mapname, BAD_STATE);
#ifdef DEBUG
		fprintf(stderr,"save_n_send_entry: cache_entry: could cache entry for %s/%s\n", domainname, mapname);
#endif
		/* we will not return an error for this failure. */
	}
	return (res);
}


/*
 * init_update is to be called at startup. If domain is not set, it gets the
 * default domainname, merges in any changes left in the delta file to the
 * ascii_file & makes a list of servers from the db file slp->db_name in
 * /usr/etc/yp/domain.
 */

init_update(upd_ptr)
yp_update_struct *upd_ptr;
{
	char	*domainname		= (upd_ptr->mrg_msgp)->domain;
	server_list *slp		= upd_ptr->slp;
	int	err;

	if (domainname == NULL) {
		domainname = get_default_domain_name();
	}

	err = update_db(upd_ptr);
	if (err != 0) {
#ifdef DEBUG
		fprintf(stderr,"init_update: error return from update_db.\n");
#endif
		return(YPUPDATE_UPDERR);	/* failure from update_db */
	}

	err = time(&t_loc);
	if (err == -1) {
#ifdef DEBUG
		fprintf(stderr,"init_update: can't get time, errno = %d\n", errno);
#endif
		return(YPUPDATE_TIME);	/* error getting time */
	}
	err = make_serverlist(slp);
	if (err) {
#ifdef DEBUG
		fprintf(stderr,"init_update: error return from get_serverlist.\n");
#endif
		return(err);	/* error from make server list */
	}
	return (0);
}


bool_t
xdr_ad_file_struct(xdrs, objp)
XDR *xdrs;
ad_file_struct *objp;
{
	if (!xdr_vector(xdrs, (char *)objp->ascii_file, YPMAXMAP + 1, sizeof(char), xdr_char)) {
		return (FALSE);
	}
	if (!xdr_vector(xdrs, (char *)objp->delta_file, YPMAXMAP + 1, sizeof(char), xdr_char)) {
		return (FALSE);
	}
	if (!xdr_char(xdrs, &objp->separator)) {
		return (FALSE);
	}
	return (TRUE);
}


bool_t
xdr_merge_req(xdrs, objp)
XDR *xdrs;
merge_req *objp;
{
	if (!xdr_ad_file_struct(xdrs, &objp->adf)) {
		return (FALSE);
	}
	if (!xdr_vector(xdrs, (char *)objp->domain, YPMAXDOMAIN + 1, sizeof(char), xdr_char)) {
		return (FALSE);
	}
	if (!xdr_vector(xdrs, (char *)objp->map, YPMAXMAP + 1, sizeof(char), xdr_char)) {
		return (FALSE);
	}
	if (!xdr_bool (xdrs, &objp->opt_pull)) {
		return (FALSE);
	}
	return (TRUE);
}

#endif /* YP_UPDATE */
