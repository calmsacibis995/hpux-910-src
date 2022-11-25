#ifndef lint
static  char rcsid[] = "@(#)ypmatch:	$Revision: 1.26.109.1 $	$Date: 91/11/19 14:07:37 $  ";
#endif
/* NFSSRC ypmatch.c	2.1 86/04/17 */
/*"ypmatch.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * This is a user command which looks up the value of a key in a map
 *
 * Usage is:
 *	ypmatch [-d domain] [-t] [-k] key [key ...] mname 
 *	ypmatch -x
 *
 * where:  the -d switch can be used to specify a domain other than the
 * default domain.  mname may be either a mapname, or a nickname which will
 * be translated into a mapname according to the translation table at
 * transtable.  The  -t switch inhibits this translation. The -k switch
 * prints keys as well as values.  The -x switch may be used alone to
 * dump the translation table.
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

void get_command_line_args();
void getdomain();
bool match_list();
bool match_one();
void print_one();
void dumptable();
void usage();

#define TIMEOUT 30			/* Total seconds for timeout */
#define INTER_TRY 10			/* Seconds between tries */

int translate = TRUE;
int printkeys = FALSE;
int dodump = FALSE;
char *domain = NULL;
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
char default_domain_name[YPMAXDOMAIN+1];
char *map = NULL;
char **keys = NULL;
int nkeys;
struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};
char *transtable[] = {
	"passwd", "passwd.byname",
	"group", "group.byname",
	"networks", "networks.byaddr",
	"hosts", "hosts.byname",
	"protocols","protocols.bynumber",
	"services","services.byname",
	"aliases", "mail.aliases",
	"ethers", "ethers.byname",
	NULL
};
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

/*
 * This is the main line for the ypmatch process.
 */
main(argc, argv)
	char **argv;
{
	int i;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("ypmatch",0);
#endif NLS

	get_command_line_args(argc, argv);

	if (dodump) {
		dumptable();
		exit(0);
	}

	if (!domain) {
		getdomain();
	}

	if (translate) {
						
		for (i = 0; transtable[i]; i += 2)

			if (strcmp(map, transtable[i]) == 0) {
				map = transtable[i + 1];
			}
	}

	if (match_list()) {
		exit(0);
	} else {
		exit(1);
	}
}

/*
 * This does the command line argument processing.
 */
void
get_command_line_args(argc, argv)
	int argc;
	char **argv;
	
{
		
	if (argc < 2) {
		usage();
		exit(1);
	}
	argv++;

	while (--argc > 0 && (*argv)[0] == '-') {

		switch ((*argv)[1]) {

		case 't':
			translate = FALSE;
			break;

		case 'k':
			printkeys = TRUE;
			break;

		case 'x':
			dodump = TRUE;
			break;

		case 'd':

			if (argc > 1) {
				argv++;
				argc--;
				domain = *argv;

				if (strlen(domain) > YPMAXDOMAIN) {
					fprintf(stderr,
						(catgets(nlmsg_fd,NL_SETN,1, "ypmatch:  NIS domain name exceeds %d characters\n")),
						YPMAXDOMAIN);
					exit(1);
				}
				
			} else {
				usage();
				exit(1);
			}
				
			break;
				
		default:
			usage();
			exit(1);
		}

		argv++;
	}

	if (!dodump) {
		
		if (argc < 2) {
			usage();
			exit(1);
		}

		keys = argv;
		nkeys = argc -1;
		map = argv[argc -1];

		if (strlen(map) > YPMAXMAP) {
			(void) fprintf(stderr,
				(catgets(nlmsg_fd,NL_SETN,2, "ypmatch:  mapname exceeds %d characters\n")),
				YPMAXMAP);
			exit(1);
		}
	}
}

/*
 * This gets the local default domainname, and makes sure that it's set
 * to something reasonable.  domain is set here.
 */
void
getdomain()		
{
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
	if (!getdomainname(default_domain_name, YPMAXDOMAIN+1) ) {
		domain = default_domain_name;
	} else {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "ypmatch:  getdomainname system call failed\n")));
		exit(1);
	}

	if (strlen(domain) == 0) {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "ypmatch:  the NIS domain name hasn't been set on this machine\n")));
		exit(1);
	}
}

/*
 * This traverses the list of argument keys.
 */
bool
match_list()
{
	bool error;
	bool errors = FALSE;
	char *val;
	int len;
	int n = 0;

	while (n < nkeys) {
		error = match_one(keys[n], &val, &len);

		if (!error) {
			print_one(keys[n], val, len);
			free(val);
		} else {
			errors = TRUE;
		}

		n++;
	}
	
	return (!errors);
}

/*
 * This fires off a "match" request to any old nis server, using the vanilla
 * nis client interface.  To cover the case in which trailing NULLs are included
 * in the keys, this retrys the match request including the NULL if the key
 * isn't in the map.
 */
bool
match_one(key, val, len)
	char *key;
	char **val;
	int *len;
{
	int err;
	bool error = FALSE;

	*val = NULL;
	*len = 0;
	err = yp_match(domain, map, key, strlen(key), val, len);
	

	if (err == YPERR_KEY) {
		err = yp_match(domain, map, key, (strlen(key) + 1),
		    val, len);
	}
		
	if (err) {
		(void) nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "ypmatch:  can't match %1$s.\n          Reason:  %2$s\n")), key,
			yperr_string(err) );
		error = TRUE;
	}
	
	return (error);
}

/*
 * This prints the value, (and optionally, the key) after first checking that
 * the last char in the value isn't a NULL.  If the last char is a NULL, the
 * \n\0 sequence which the nis client layer has given to us is shuffled back
 * one byte.
 */
void
print_one(key, val, len)
	char *key;
	char *val;
	int len;
{
	if (printkeys) {
		(void) printf((catgets(nlmsg_fd,NL_SETN,6, "%s: ")), key);
	}

	(void) printf((catgets(nlmsg_fd,NL_SETN,7, "%.*s\n")), len, val);
}

/*
 * This will print out the map nickname translation table.
 */
void
dumptable()
{
	int i;

	for (i = 0; transtable[i]; i += 2) {
		nl_printf((catgets(nlmsg_fd,NL_SETN,8, "Use \"%1$s\" for map \"%2$s\"\n")), transtable[i],
		    transtable[i + 1]);
	}
}

void
usage()
{
fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "Usage:  ypmatch [-k] [-t] [-d domain] key ...  mname\n        ypmatch -x\n\nwhere   mname may be either a mapname or a map nickname\n        -k prints keys as well as values\n        -t inhibits map nickname translation\n        -x prints a list of map nicknames and their corresponding mapnames\n")));
}
