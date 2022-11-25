#ifndef lint
static  char rcsid[] = "@(#)ypcat:	$Revision: 1.28.109.1 $	$Date: 91/11/19 14:07:33 $  ";
#endif
/* NFSSRC ypcat.c	2.1 86/04/17 */
/*"ypcat.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * This is a user command which dumps each entry in a nis data base.  It gets
 * the stuff using the normal ypclnt package; the user doesn't get to choose
 * which server gives him the input.  Usage is:
 * ypcat [-k] [-d domain] [-t] map
 * ypcat -x
 * where the -k switch will dump keys followed by a single blank space
 * before the value, and the -d switch can be used to specify a domain other
 * than the default domain.
 * 
 * Normally, passwd gets converted to passwd.byname, and similarly for the
 * other standard files.  -t inhibits this translation.
 * 
 * The -x switch will dump the map nickname translation table.  
 */
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#ifdef NULL
#undef NULL
#endif
#define NULL 0
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>

int dumpkeys = FALSE;
int translate = TRUE;
int dodump = FALSE;
char *domain = NULL;
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *
 *  HPNFS
 */
char default_domain_name[YPMAXDOMAIN+1];
char *map = NULL;
char nullstring[] = "";
char *transtable[] = {
	"passwd", "passwd.byname",
	"group", "group.byname",
	"networks", "networks.byaddr",
	"hosts", "hosts.byaddr",
	"protocols","protocols.bynumber",
	"services","services.byname",
	"aliases", "mail.aliases",
	"ethers", "ethers.byname",
	NULL
};

void get_command_line_args();
void dumptable();
int callback();
int callback_err = 0;	/*  This variable is now global so the error value
			    received by the callback() function could be more
			    easily examined by the main routine.  */
void one_by_one_all();
void usage();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

/*
 * This is the mainline for the ypcat process.  It pulls whatever arguments
 * have been passed from the command line, and uses defaults for the rest.
 */

void
main (argc, argv)
	int argc;
	char **argv;

{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;
	int i;
	struct ypall_callback cbinfo;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("ypcat",0);
#endif NLS

	get_command_line_args(argc, argv);

	if (dodump) {
		dumptable();
		exit(0);
	}

	if (!domain) {

/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *
 *  HPNFS
 */
		if (!getdomainname(default_domain_name, YPMAXDOMAIN+1) ) {
			domain = default_domain_name;
		} else {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "ypcat:  getdomainname system call failed\n")));
			exit(1);
		}

		if (strlen(domain) == 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "ypcat:  the NIS domain name hasn't been set on this machine\n")));
			exit(1);
		}
	}

	if (err = yp_bind(domain) ) {
		nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "ypcat:  can't bind to an NIS server for domain %1$s.\n        Reason:  %2$s\n")),
			domain, yperr_string(err) );
		exit(1);
	}

	key = nullstring;
	keylen = 0;
	val = nullstring;
	vallen = 0;

	if (translate) {
		for (i = 0; transtable[i]; i+=2)
			if (strcmp(map, transtable[i]) == 0) {
			    map = transtable[i+1];
			    break;
			}
	}

	cbinfo.foreach = callback;
	cbinfo.data = (char *) NULL;
	err = yp_all(domain, map, &cbinfo);

	if (err == YPERR_VERS) {
		one_by_one_all(domain, map);
	}

	exit(err ? err : callback_err);
}

/*
 * This does the command line argument processing.
 */
static void
get_command_line_args(argc, argv)
	int argc;
	char **argv;

{

	argv++;

	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 't':
				translate = FALSE;
				argv++;
				break;

			case 'k':
				dumpkeys = TRUE;
				argv++;
				break;

			case 'x':
				dodump = TRUE;
				argv++;
				break;

			case 'd': 

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if (strlen(domain) > YPMAXDOMAIN) {
						fprintf(stderr,
							(catgets(nlmsg_fd,NL_SETN,14, "ypcat:  NIS domain name exceeds %d characters\n")),
							YPMAXDOMAIN);
						exit(1);
					}

				} else
					usage();

				break;

			default:
				usage();
			}

		} else {

			if (!map) {
				map = *argv;
				argv++;
			} else
				usage();
		}
	}

	if (!map && !dodump)
		usage();
}
/*
 * This will print out the map nickname translation table.
 */
static void
dumptable()
{
	int i;

	for (i = 0; transtable[i]; i += 2) {
		nl_printf((catgets(nlmsg_fd,NL_SETN,5, "Use \"%1$s\" for map \"%2$s\"\n")), transtable[i],
		    transtable[i + 1]);
	}
}

/*
 * This dumps out the value, optionally the key, and perhaps an error message.
 */
static int
callback(status, key, kl, val, vl, notused)
	int status;
	char *key;
	int kl;
	char *val;
	int vl;
	char *notused;
{
	if (status == YP_TRUE) {

		if (dumpkeys)
			(void) printf((catgets(nlmsg_fd,NL_SETN,6, "%.*s ")), kl, key);

		(void) printf((catgets(nlmsg_fd,NL_SETN,7, "%.*s\n")), vl, val);
		return (FALSE);
	} else {

		callback_err = ypprot_err(status);

		if (callback_err != YPERR_NOMORE) {
			(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "ypcat:  %s\n")), yperr_string(callback_err));

			callback_err = 1;
		} else
			callback_err = 0;
		return (TRUE);
	}
}

/*
 * This cats the map out by using the old one-by-one enumeration interface.
 * As such, it is prey to the old-style problems of rebinding to different
 * servers during the enumeration.
 */
static void
one_by_one_all(domain, map)
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;

	key = nullstring;
	keylen = 0;
	val = nullstring;
	vallen = 0;

	if (err = yp_first(domain, map, &outkey, &outkeylen, &val, &vallen) ) {

		if (err == YPERR_NOMORE) {
			exit(0);
		} else {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "ypcat:  can't get the first record from the NIS map.\n        Reason:  %s\n")),
			    yperr_string(err) );
			exit(1);
		}
	}

	while (TRUE) {

		if (dumpkeys) {
			printf((catgets(nlmsg_fd,NL_SETN,10, "%.*s ")), outkeylen, outkey);
		}

		printf((catgets(nlmsg_fd,NL_SETN,11, "%.*s\n")), vallen, val);
		free(val);
		key = outkey;
		keylen = outkeylen;

		if (err = yp_next(domain, map, key, keylen, &outkey, &outkeylen,
		    &val, &vallen) ) {

			if (err == YPERR_NOMORE) {
				exit(0);
			} else {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "ypcat:  can't get the next record from the NIS map.\n        Reason:  %s\n")),
					yperr_string(err) );
				exit(1);
			}
		}

		free(key);
	}
}

void
usage()
{
fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "Usage:  ypcat [-k] [-t] [-d domain] mname\n        ypcat -x\n\nwhere   mname may be either a mapname or a map nickname\n        -k prints keys as well as values\n        -t inhibits map nickname translation\n        -x prints a list of map nicknames and their corresponding mapnames\n")));
exit(1);
}
