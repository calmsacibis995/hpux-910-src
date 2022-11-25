#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <ctype.h>
#include <arpa/nameser.h>
#include "ns.h"
#include "db.h"

struct	netinfo *fnettab = NULL;
struct	timeval tt;
FILE *ddt = NULL;
struct hashbuf *hashtab;	/* root hash table */
struct hashbuf *fcachetab;	/* hash table of cache read from file */
int needmaint = 0;
int max_cache_ttl = 300;
int reloading = 0;
int debug = 0;
struct fwdinfo *fwdtab = NULL;
struct sockaddr_in nsaddr;

main(argc, argv)
int argc;
char *argv[];
{
	extern char *optarg;            /* for getopt */
	extern int optind;              /* for getopt */
	char c;			        /* for getopt */
	char *boot_file;
	int size = 0;
	char *s;

	boot_file = "/etc/named.boot";
	while ((c = getopt(argc, argv, "b:S:")) != EOF)
		switch(c){
			case 'b':
					boot_file = optarg;
					break;
			case 'S':
					size = atoi(optarg) * 1024;
					s = (char *)malloc(size);
					free(s);
					break;
		}
	if(optind != argc)
		boot_file = argv[optind];
	buildservicelist();
	buildprotolist();
	ns_init(boot_file);
}

net_mask()
{
	return(0);
}

db_update()
{
	return(1);
}

gettime(ttp)
struct timeval *ttp;
{
	if (gettimeofday(ttp, (struct timezone *)0) < 0)
		syslog(LOG_ERR, "gettimeofday failed: %m");
	return;
}

