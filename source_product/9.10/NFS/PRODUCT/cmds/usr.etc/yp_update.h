#ifndef YP_UPDATE_INCLUDED
#define YP_UPDATE_INCLUDED

#ifndef YP_CACHE
#undef  YP_UPDATE
#endif	/* YP_CACHE */

#ifdef  YP_UPDATE
/*
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <sys/time.h>
*/
/* define errors , detailed error messages are in logfile */

#define	YPUPDATE_NOMEM		1	/* no memory to cache entries */
#define	YPUPDATE_RENAME		2	/* rename failed */
#define	YPUPDATE_OPENTMP	3	/* coulnt open temp file */
#define	YPUPDATE_ASCII		4	/* couldnt open ascii file */
#define	YPUPDATE_TRUNCATE	5	/* ftruncate failed */
#define	YPUPDATE_FSTATDEL	6	/* fstat on delta file failed */
#define	YPUPDATE_NULLDFP	7	/* delta file ptr is NULL */
#define	YPUPDATE_BADMAP		8	/* bad map */
#define	YPUPDATE_DELTAERR	9	/* delta file not empty, cache map bad*/
#define	YPUPDATE_NULLMAPPTR	10	/* null mapptr */
#define	YPUPDATE_YPBIND		11	/* yp_bind on domain failed */
#define	YPUPDATE_YPFIRST	12	/* yp_first failed */
#define	YPUPDATE_YPNEXT		13	/* yp_next failed */
#define	YPUPDATE_DBSTAT		14	/* stat on db_file failed */
#define	YPUPDATE_NULLCL		15	/* cant make client handle */
#define	YPUPDATE_NULLSLIST	16	/* NULL server list */
#define	YPUPDATE_SENDERR	17	/* errors in send */
#define	YPUPDATE_UPDERR		18	/* failure from update_db */
#define	YPUPDATE_TIME		19	/* error getting time */
#define	YPUPDATE_SENDENTRY	20	/* list given as arg to send_entry */
#define	YPUPDATE_ASCIIFILE	21	/* ascii file name incorrect */
#define	YPUPDATE_DELTAFILE	22	/* delta file name incorrect */
#define	YPUPDATE_UNDEFOP	23	/* undefined operation */
/*
 * the ascii-deltafile-struct contains the name of the ascii-file in which 
 * change is to be made (for example, "/etc/passwd"). delta_file is the file
 * in which updates are written until merged into the ascii-file.
 *
 * The lines of the ascii_file are expected to contain the ascii lines, with
 * the key first, and the whole line as the value. This is consistent with the
 * data from which the NIS data bases are often built. /etc/passwd, /etc/hosts
 * etc are already in such a format. The separator is the charactar that
 * separates the key from the rest of the line. For example, in the /etc/passwd
 * file, the separator char is ':' and for the /etc/host file, the separator
 * char is a space or a tab. NOTE THAT THE separator char MUST BE USED TO
 * SEPARATE THE KEY FROM THE REST OF THE LINE ON ALL LINES. OTHERWISE, THE MATCH
 * operation will fail.
 *
 * There are no default values for any of these.
 */
struct ad_file_struct {	/* ascii-delta-file struct */
	char	ascii_file[YPMAXMAP+1];
	char	delta_file[YPMAXMAP+1];
	char	separator;
};
typedef struct ad_file_struct ad_file_struct;
bool_t xdr_ad_file_struct();
/*
 * The merge message contains info that is used to send a message to the
 * authenticator requesting that the delta file be merged into the ascii-file.
 */
struct merge_req {
	ad_file_struct adf;
	char domain [YPMAXDOMAIN+1];
	char map    [YPMAXMAP+1];
	bool_t opt_pull;
};
typedef struct merge_req merge_req;
bool_t xdr_merge_req();
/*
 * The server entry is used to save info about each server, specifically it's
 * name, binding info (ip-addr, domain name, port etc). The default domain name
 * is used. The list of servers is obtained from a nis database db_name in the
 * server_list structure below.
 */
struct server_entry {
	struct	server_entry	*next;
	char	name[YPMAXPEER+1];
	int	namelen;
	struct	dom_binding	domb;
	u_long			status;
};
typedef struct server_entry	server_entry;
/*
 * The server_list structure keeps the list of servers and all common info
 * about the servers. The default is "/usr/etc/yp". 'domain' is the domain
 * name. If NULL, the default domain name will be used. 'db_name' is the NIS data
 * base containing the names of servers as the key, lbuilt like the 'ypservers'
 * data base (which is the default). 'prognum' is the program number (default
 * YPPROG), 'procnum' is the procedure number (default YPPROC_UPDATE_MAPENTRY)
 * and 'version' is the version number (default YPVERS) of the server program.
 */
struct server_list {
	long	mtime;			/* last stat-time of servers.pag */
	char	*domain;		/* domain name */
	char	*db_name;		/* ptr to full db path name */
	CLIENT	*client;		/* place holder for client handle */
	u_long	prognum;		/* program number to talk to */
	u_long	version;		/* version number to talk to */
	u_long	procnum;		/* procedure number to talk to */
	server_entry *se_list;		/* list of all server_ebtries */
};
typedef struct server_list	server_list;
/*
 * Now define a struct that has all the essentials above, mostly for
 * use in the lib calls. Not all fields are of interest to all routines.
 * The lock_file is opened in the mandatory exclusive write access mode, to
 * prevent simiultaneous access by other writers.e
 */
struct yp_update_struct	{
	FILE	*dfp;			/* delta file pointer */
	char	lock_file [YPMAXMAP+1]; /* lock file name */
	merge_req	*mrg_msgp;	/* merge message struct ptr */
	server_list	*slp;		/* server list ptr */
};
typedef struct yp_update_struct yp_update_struct;

struct cu_data {
	int		cu_sock;
	bool_t		cu_closeit;
	struct sockaddr_in cu_raddr;
	int		cu_rlen;
	struct timeval	cu_wait;
	struct timeval	cu_total;
	struct rpc_err	cu_error;
	XDR		cu_outxdrs;
	u_int		cu_xdrpos;
	u_int		cu_sendsz;
	char		*cu_outbuf;
	u_int		cu_recvsz;
	char		cu_inbuf[1];
};
#endif /* YP_UPDATE */
#endif /* YP_UPDATE_INCLUDED */
