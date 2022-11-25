/* -*-C-*-
*********************************************************************
*
* File:         bootpquery.c
* RCS:          $Header: bootpquery.c,v 1.10.109.1 91/11/21 11:48:17 kcs Exp $
* Description:  BootP Client
* Author:       Jerry McCollom, HP/CND
* Created:      Mon Aug 28 09:12:43 1989
* Modified:     Mon Aug 28 09:39:00 1989 (Jerry McCollom) jmc@hpcndr
*
* (c) Copyright 1989, Hewlett-Packard Company, all rights reserved.
*
*********************************************************************
*/

#ifndef lint
static char rcsid[] = "@(#)$Header: bootpquery.c,v 1.10.109.1 91/11/21 11:48:17 kcs Exp $";
#endif lint

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <nlist.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <ctype.h>
#include <netdb.h>

#include "bootp.h"

#define BACKINT         1       /* How long to back off, default = 1 second */
#define MAXREQ          3       /* Maximum number of times to try a request */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 255
#endif

struct netinfo {
	struct netinfo *next;
	u_long net;
	u_long mask;
	int flags;
	struct in_addr my_addr;
	struct in_addr braddr;
};
struct	netinfo *nettab = NULL;

/*
 * Vendor magic cookies for CMU and RFC1048
 */

#ifdef VEND_CMU
u_char vm_cmu[4]     = VM_CMU;
#endif
u_char vm_rfc1048[4] = VM_RFC1048;
u_char vm_other[4]   = { 0, 0, 0, 0 };


/*
 * Hardware type names as specified by the Assigned Numbers RFC.
 */

char *htypenames[] = {
	"reserved",
	"ethernet",
	"ethernet3",
	"ax.25",
	"pronet",
	"chaos",
	"ieee802",
	"arcnet"
};
#define MAXHTYPES	sizeof(htypenames)

char bootfile[MAXPATHLEN];	/* Request specific bootfile */
char bootsname[MAXHOSTNAMELEN];	/* Name of BOOTP server */
struct in_addr *bootsrvr;	/* Address of BOOTP server */
struct in_addr *clientaddr;	/* Address of BOOTP client */
u_char *haddr;			/* Hardware address for BOOTREQUEST */
char *haddrbuf[MAXPATHLEN];
u_char *vm_cookie = vm_rfc1048;	/* Vendor information cookie */
int htype;			/* Hardware type */
long tid;                       /* Temp ID */
int broadcast = 1;		/* True if BOOTREQUEST should be broadcast */

main(argc, argv)
int argc;
char **argv;
{
	u_long myaddr, bootp();		/* My IP address */
	extern int opterr, optind;	/* Options processing */
	extern char *optarg;
	int c;
	struct hostent *hp;
	u_long ipaddr;
	u_char *ghaddr();

	/* We can only run as super user. */
	if (geteuid() != 0) {
		fprintf(stderr, "bootpquery: Must be super-user\n");
		exit(1);
	}

	/* Copy hardware address into bootp packet */
	if ((argc < 2) || (*argv[1] == '-'))
		usage();
	else {
		strncpy(haddrbuf, argv[1], sizeof(haddrbuf));
		if ((haddr = ghaddr(argv[1])) == NULL) {
			fprintf(stderr, "bootpquery: Invalid hardware address %s\n",
				argv[1]);
			usage();
		}
	}

	/* Initialize option defaults */
	bzero(bootfile, sizeof(bootfile));
	bzero(bootsname, sizeof(bootsname));
	bootsrvr = NULL;
	clientaddr = NULL;
	htype = 1;
	

	/* Process options */
	if (argc > 2) {
		optind = 2;
		opterr = 0;
		/* Get hardware type if any */
		if (*argv[2] != '-') {
			if (isdigit(*argv[2]))
				htype = atoi(argv[2]);
			else if (!strncmp(argv[2], "ether", 5))
				htype = 1;
			else if (!strncmp(argv[2], "ieee", 4))
				htype = 6;
			else
				htype = -1;
			if (htype != 1 && htype != 6) {
				fprintf(stderr, 
					"bootpquery: Invalid hardware type %s.\n",
					argv[2]);
				usage();
			}
			optind++;
		}
		while ((c = getopt(argc,argv,"s:b:i:v:")) != EOF) {
			switch (c) {
			    case 's':
				ipaddr = inet_addr(optarg);
				if (ipaddr == (u_long)-1)
					hp = gethostbyname(optarg);
				else
					hp = gethostbyaddr(&ipaddr, 
							   sizeof(struct in_addr), AF_INET);
				if (hp == NULL) {
					fprintf(stderr,
						"bootpquery: Unknown server %s\n",
						optarg);
					exit(1);
				}
				bootsrvr = (struct in_addr *)
					malloc(sizeof(struct in_addr));
				if (ipaddr == (u_long)-1)
					bootsrvr->s_addr = *(u_long *)hp->h_addr;
				else
					bootsrvr->s_addr = (u_long)ipaddr;
				strncpy(bootsname, hp->h_name, sizeof(bootsname));
				broadcast = 0;
				break;
			    case 'b':
				strncpy(bootfile, optarg, sizeof(bootfile));
				break;
			    case 'i':
				ipaddr = inet_addr(optarg);
				if (ipaddr == (u_long)-1) {
					fprintf(stderr,
						"bootpquery: Invalid IP address %s\n",
						optarg);
					exit(1);
				}
				clientaddr = (struct in_addr *)
					malloc(sizeof(struct in_addr));
				clientaddr->s_addr = (u_long)ipaddr;
				break;
			    case 'v':
#ifdef VEND_CMU
				if (strncasecmp(optarg, "cmu", 3) == 0)
					vm_cookie = vm_cmu;
				else 
#endif
				if (strncasecmp(optarg, "rfc10", 5) ==  0)
					vm_cookie = vm_rfc1048;
				else {
					vm_cookie = vm_other;
					memcpy(vm_other, optarg,	
					       strlen(optarg) >= 4 ?
					       4 : strlen(optarg));
				}
				break;
  			    case '?': 
				fprintf(stderr,
					"bootpquery: Unknown option: '%s'\n",
					argv[optind-1]);
				usage();
			}
		}
	}

#ifdef BOOTP_DEBUG
	printf("Hardware type: %d\n", htype);
	printf("Hardware address: %s\n", haddrbuf);
	printf("Server: %s (%s)\n", bootsname, inet_ntoa(bootsrvr));
	printf("Bootfile: %s\n", bootfile);
	printf("IP Address: %s\n", inet_ntoa(ipaddr));
#endif
	myaddr = bootp(0, MAXREQ);
	if(!myaddr)
	{
		fprintf(stderr, "bootpquery: Bootp servers not responding!\n");
		exit(1);
	}
	else 
	{
	  if(myaddr == -1)
	  {
		fprintf(stderr, "bootpquery: Internal error!\n");
		exit(1);
	  }
	}
	exit(0);
}


/*
 * Try to find our IP address using the bootstrap protocol.
 * Build a bootp request packet and broadcast it onto the net.
 * Then wait for a reply packet, or a timeout.
 */
u_long
bootp(nback, retries)
int nback,              /* No backoff flag */
    retries;            /* Number of retries flag */
{
    struct sockaddr_in sin, to;
    int s, nfound, i, on = 1;
    long time();
    struct bootp bpreq, *bp = &bpreq;	/* Boot packet pointers */
    unsigned int backoff = BACKINT;	/* Backoff interval */
    long rand();
    long starttime;			/* Start of session time */
    fd_set readfds;
    struct timeval timeout;
    int ret, nreq = 0;
    char *h;

    srand((unsigned) time(NULL));     /* Seed random timer for tid */

    if (retries == 0) {
        retries = MAXREQ;
    }
        
    /* Open datagram socket for dialog */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

#ifdef SO_BROADCAST
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) < 0) {
	perror("setsockopt SO_BROADCAST");
	exit(1);
    }
#endif /* SO_BROADCAST */

    sin.sin_family = AF_INET;
    sin.sin_port = htons(IPPORT_BOOTPC);
    sin.sin_addr.s_addr = INADDR_ANY;
	    

    if (bind(s, (caddr_t)&sin, sizeof(sin), 0) < 0) {
        perror("bind");
        exit(1);
    }

    to.sin_family = AF_INET;
    to.sin_port = htons(IPPORT_BOOTPS);
        
        
    /* Fill in BOOTP data */
    bp -> bp_op = BOOTREQUEST;			/* Request boot */
    bcopy(haddr, bp->bp_chaddr,			/* Hardware address */
	  sizeof(bp->bp_chaddr));
    bp -> bp_htype = htype;			/* Hardware type */
    bp->bp_hlen = 6;				/* Length of hardware address */
    bp->bp_hops = 0;				/* Number of hops */
    tid = rand();				/* Tranasaction ID */
    bp->bp_xid = tid;
    bp->bp_secs = 0;				/* Seconds since start up */
    if (clientaddr != NULL)			/* Client address (us) */
	    bp->bp_ciaddr = *clientaddr;
    else
	    bp->bp_ciaddr.s_addr = 0;
    bp->bp_yiaddr.s_addr = 0;   		/* Your address (us, via server) */
    if (bootsrvr != NULL)			/* Server address */
	    bp->bp_siaddr = *bootsrvr;
    else
	    bp->bp_siaddr.s_addr = 0;
    bp->bp_giaddr.s_addr = 0;   		/* Gateway address */
    memcpy(bp->bp_vend, vm_cookie, 4);		/* Vendor information */
    strncpy(bp->bp_sname, bootsname, 		/* Server name */
	    sizeof(bp->bp_sname));
    strncpy(bp->bp_file, bootfile,		/* Boot file */
	    sizeof(bp->bp_file));

    /* If broadcasting, get interface information */
    if (broadcast)
	    getifconf(s);

    /* Remember what time we started */
    starttime = time(NULL);
        
    for (;;) {                          /* Loop 'til done */
        /* Send packet, broadcasting if necessary */
	if (broadcast) {
		struct netinfo *ntp;
		for (ntp = nettab; ntp != NULL; ntp = ntp->next) {
			to.sin_addr = ntp->braddr;
			if (sendto(s, bp, sizeof(*bp), 0, &to, sizeof(to)) < 0) {
				perror("bootpquery: sendto");
			}
		}
	}
	else {
		to.sin_addr.s_addr = bootsrvr->s_addr;
		if (sendto(s, bp, sizeof(*bp), 0, &to, sizeof(to)) < 0) {
			perror("sendto");
			exit(1);
		}
	}
	/* Wait for a response */
	FD_ZERO(&readfds);
        FD_SET(s, &readfds);
        timeout.tv_sec = backoff;
        timeout.tv_usec = 0;
        nfound = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
        if(nfound == -1) {
            perror("select");
            exit(1);
        } else if(nfound == 1) {
            recvpack(s);
            break;
        } else {
            /* If the backoff interval is not defined and the last interval
               was less than 60, double last interval and add random between
               0 and 3 to help avoid congestion */
            if (nback == 0) {
                if (backoff < 60) {
                    backoff = (backoff << 1) + (rand() % 3);
                }
                else
                    backoff = 60;
            }

            /* Update time we've been waiting */
            bp->bp_secs = time(NULL) - starttime;

            /* Check to see if we've sent out too many packets */
            if (++nreq > retries) {
                return(0);
            }
        }
    }
    return(1);
}


/*
 * Get the lan interface information including
 * broadcast address, subnet mask, and address.
 */

getifconf(s)
    int s;
{
    int n;
    struct ifconf ifc;
    struct ifreq ifreq, *ifr;
    struct ifreq buf[10];
    struct netinfo *ntp, *ntip;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_req = buf;
    if ((ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) ||
	(ifc.ifc_len <= 0)) {
	    perror("bootpquery: ioctl");
	    exit(1);
    }
    n = ifc.ifc_len / sizeof(struct ifreq);
    ntp = NULL;
    for (ifr = ifc.ifc_req; n > 0; n--, ifr++) {
	    int if_flags;

	    if (ifr->ifr_addr.sa_family != AF_INET)
		    continue;
	    ifreq = *ifr;
	    if (ioctl(s, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
		    perror("bootpquery: get interface flags");
		    continue;
	    }
	    if_flags = ifreq.ifr_flags;
	    if ((ifreq.ifr_flags & IFF_UP) == 0)
		    continue;
	    if (ioctl(s, SIOCGIFADDR, (char *)&ifreq) < 0) {
		    perror("bootpquery: get interface addr");
		    continue;
	    }
	    if (ntp == NULL)
		    ntp = (struct netinfo *)malloc(sizeof(struct netinfo));
	    ntp->flags = if_flags;
	    ntp->my_addr = 
		    ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr;
#ifdef IFF_LOOPBACK
	    if (ntp->flags & IFF_LOOPBACK)
#else
	    /* test against 127.0.0.1 (yuck!!) */
	    if (ntp->my_addr.s_addr == htonl(0x7F000001))
#endif IFF_LOOPBACK
		    continue;
#ifdef SIOCGIFNETMASK
	    if (ioctl(s, SIOCGIFNETMASK, (char *)&ifreq) < 0) {
		    perror("bootpquery: get netmask");
		    continue;
	    }
	    ntp->mask = ((struct sockaddr_in *)
			 &ifreq.ifr_addr)->sin_addr.s_addr;
#else
	    /* 4.2 does not support subnets */
	    ntp->mask = net_mask(ntp->my_addr);
#endif
	    if (ntp->flags & IFF_POINTOPOINT) {
		    if (ioctl(s, SIOCGIFDSTADDR, (char *)&ifreq) < 0) {
			    perror("bootpquery: get dst addr");
			    continue;
		    }
		    ntp->mask = 0xffffffff;
		    ntp->net = ((struct sockaddr_in *)
				&ifreq.ifr_addr)->sin_addr.s_addr;
	    } else {
		    ntp->net = ntp->mask & ntp->my_addr.s_addr;
	    }
	    if (ntp->flags & IFF_BROADCAST) {
		    if (ioctl(s, SIOCGIFBRDADDR, (char *)&ifreq) < 0) {
			    perror("bootpquery: get broadcast addr");
			    continue;
		    }
		    ntp->braddr = ((struct sockaddr_in *)
				   &ifreq.ifr_broadaddr)->sin_addr;
	    }
	    ntp->next = NULL;
	    if (nettab == NULL)
		    nettab = ntip = ntp;
	    else {
		    ntip->next = ntp;
		    ntip = ntp;
	    }
	    ntp = NULL;
    }
    if (ntp)
	    (void) free((char *)ntp);
    if (nettab == NULL) {
	    perror("bootpquery: cannot continue; no lan interfaces available");
	    exit(1);
    }
}


/*
 * Intercept any bootp packets for the host
 * we are "booting".
 */
recvpack(s)
int     s;
{
    u_char buf[1024];           /* receive packet buffer */
    struct sockaddr_in from;
    int fromlen;
    struct bootp *rbp;
    register int n;
    
    fromlen = sizeof(from);
    n = recvfrom(s, buf, sizeof(buf), 0, &from, &fromlen);

    if (n == -1) {             /* Error */
        perror("recvfrom");
        exit(1);
    } else if(n != sizeof(*rbp)) {
        printf("bootpquery: bad length packet received\n");
        return;
    }

    rbp = (struct bootp *) buf;
            
    /* Check hardware address and transaction ID to make sure it's for us */
    if ((bcmp(haddr, rbp->bp_chaddr, 6) == 0)
        && (tid == rbp->bp_xid)
        && (rbp->bp_op == BOOTREPLY))
		printpack(rbp);
}



/*
 * Print a comma separated list of IP addresses
 */

print_iplist(list, len)
    u_char *list;
    int len;
{
    struct bootp bpreq, *bp = &bpreq;	/* Boot packet pointers */
	while (len) {
		bp->bp_siaddr.s_addr = _getlong(list);
		printf(inet_ntoa(bp->bp_siaddr));
		list += sizeof(u_long);
		len--;
		if (len)
			printf(", ");
	}
}


/*
 * Print a generic list of bytes.
 */
print_bytelist(list, len)
    u_char *list;
    int len;
{
	putchar('[');
	while (len) {
		printf("%d", (int)*(u_char *)list);
		list++;
		len--;
		if (len)
			printf(", ");
	}
	putchar(']');
}


/*
 * Convert a hardware address to an ASCII string.
 */
char *
haddrtoa(cp)
    u_char *cp;
{
	static char hbuf[32];
	char vbuf[5];
	int i;

	bzero(hbuf, sizeof(hbuf));
	for(i = 0; i < 6; i++, cp++) {
		sprintf(vbuf, "%.2x", (int)*cp);
		strcat(hbuf, vbuf);
		if (i < 5)
			strcat(hbuf, ":");
	}
	return(hbuf);
}


/*
 *  Print the contents of the bootp packet.
 */
 
printpack(bp)
struct bootp *bp;
{
	char hostbuf[BUFSIZ];
	struct hostent *hp;
	int vendor = 0;

	hp = gethostbyaddr(&bp->bp_siaddr, sizeof(struct in_addr), AF_INET);
	if (hp != NULL)
		sprintf(hostbuf, "%s (%s)", hp->h_name, 
			inet_ntoa(bp->bp_siaddr));
	else
		sprintf(hostbuf, "%s", inet_ntoa(bp->bp_siaddr));
	
	printf("Received BOOTREPLY from %s\n\n", hostbuf);
	printf("Hardware Address:\t%s\n", haddrtoa(bp->bp_chaddr));
	printf("Hardware Type:\t\t%s\n",
	       bp->bp_htype < MAXHTYPES ? htypenames[bp->bp_htype]
	       : "unknown");
	printf("IP Address:\t\t%s\n",
	       bp->bp_yiaddr.s_addr ? 
	       inet_ntoa(bp->bp_yiaddr) :
	       inet_ntoa(bp->bp_ciaddr));

	printf("Boot file:\t\t%s\n",
	       strlen(bp->bp_file) ? (char *)(bp->bp_file) : "(None)");

#ifdef VEND_CMU
	if (memcmp(bp->bp_vend, vm_cmu, 4) == 0)
		print_cmu(bp);
#endif

	if (memcmp(bp->bp_vend, vm_rfc1048, 4) == 0)
		print_rfc1048(bp);

}



#ifdef VEND_CMU

/*
 * Print the CMU "vendor" data of the
 * bootp packet pointed to by "bp".
 */

print_cmu(bp)
    register struct bootp *bp;
{
    struct cmu_vend *vendp;
    int count;

    printf("\nCMU Vendor Information:\n");
    vendp = (struct cmu_vend *) bp->bp_vend;
    if (vendp->v_flags * VF_SMASK) {
	    printf("  Subnet mask:          %s\n",
		   inet_ntoa(vendp->v_smask));
	    if (vendp->v_dgate.s_addr)
		    printf("  Default Gateway:      %s\n", 
			   inet_ntoa(vendp->v_dgate));
    }
    if (vendp->v_dns1.s_addr) {
	    count = (vendp->v_dns2.s_addr) ? 2 : 1;
	    printf("  Domain Name Server%s  ",
		   count ? "s:" : ": ");
	    print_iplist(&(vendp->v_dns1), count);
	    putchar('\n');
    }
    if (vendp->v_ins1.s_addr) {
	    count = (vendp->v_ins2.s_addr) ? 2 : 1;
	    printf("  IEN-116 Name Server%s ",
		   count ? "s:" : ": ");
	    print_iplist(&(vendp->v_ins1), count);
	    putchar('\n');
    }
    if (vendp->v_ts1.s_addr) {
	    count = (vendp->v_ts2.s_addr) ? 2 : 1;
	    printf("  Time Server%s         ",
		   count ? "s:" : ": ");
	    print_iplist(&(vendp->v_ts1), count);
	    putchar('\n');
    }
}

#endif /* VEND_CMU */




/*
 * Print the RFC1048 vendor data of the
 * bootp packet pointed by "bp".
 */

print_rfc1048(bp)
    register struct bootp *bp;
{
    int bytesleft, len;
    u_char *vp;
    char *tmpstr;
    char hostbuf[BUFSIZ];

    printf("\nRFC 1048 Vendor Information:\n");
    vp = (u_char *)bp->bp_vend;
    bytesleft = sizeof(bp->bp_vend);	/* Initial vendor area size */
    vp += 4;
    bytesleft -= 4;

    while (bytesleft && *vp && (*vp != TAG_END)) {
	    int tag = (int)*vp++;
	    int len = (int)*vp++;
	    bytesleft -= 2 + len;
	    switch(tag) {
		case TAG_SUBNET_MASK:
		    printf("  Subnet Mask:          ");
		    if (len == sizeof(u_long))
			    print_iplist(vp, 1);
		    else
			    print_bytelist(vp, len);
		    break;
		case TAG_TIME_OFFSET:
		    printf("  Time Offset:          ");
		    if (len == sizeof(u_long))
			    printf("%d seconds", _getlong(vp));
		    else
			    print_bytelist(vp, len);
		    break;
		case TAG_GATEWAY:
		    printf("  Gateway%s:\t\t", (len / 4) > 1 ? "s" : "");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_TIME_SERVER:
		    printf("  Time Server%s         ",
			   (len / 4) > 1 ? "s:" : ": ");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_NAME_SERVER: 
		    printf("  IEN-116 Name Server%s ", 
			   (len / 4) > 1 ? "s:" : ": ");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_DOMAIN_SERVER:
		    printf("  Domain Name Server%s  ", 
			   (len / 4) > 1 ? "s:" : ": ");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_LOG_SERVER:
		    printf("  Log Server            ",
			   (len / 4) > 1 ? "s:" : ": ");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_COOKIE_SERVER:
		    printf("  Cookie Server%s       ",
			   (len / 4) > 1 ? "s:" : ": ");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_LPR_SERVER:
		    printf("  LPR Server%s          ",
			   (len / 4) > 1 ? "s:" : ": ");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_IMPRESS_SERVER:   
		    printf("  IMPRESS Server%s      ",
			   (len / 4) > 1 ? "s:" : ": ");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_RLP_SERVER:   
		    printf("  RLP Server%s          ",
			   (len / 4) > 1 ? "s:" : ": ");
		    print_iplist(vp, len / 4);
		    break;
		case TAG_HOSTNAME:
		    strncpy(hostbuf, (char *)vp, len);
		    hostbuf[len] = '\0';
		    printf("  Host Name:            %s", hostbuf);
		    break;
		case TAG_BOOTSIZE:
		    printf("  Boofile Size:         ");
		    if (len == sizeof(u_short))
			    printf("%d 512 byte blocks", *(u_short *)vp);
		    else
			    print_bytelist(vp, len);
		    break;
		default:
		    printf("  Tag #%.3d              ", (int)tag);
		    print_bytelist(vp, len);
		    break;
	    }
	    putchar('\n');
	    vp += len;
    }
}


/*
 * Hardware address interpretation routine.
 */
u_char *
ghaddr(cp)
	register char *cp;
{
	register char c;
	register u_short val, n;
	static u_char haddr[6];
	u_char *pp = haddr, parts;
	int dots;

	parts = 0;
	dots = 0;
	while (parts < 6) {
		val = 0; 
		n=0;
		while ((c = *cp) && (n < 2)) {
			if (isdigit(c)) {
				val = (val << 4) + (c - '0');
				cp++, n++;
				continue;
			} 
			if (isxdigit(c)) {
				val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
				cp++, n++;
				continue;
			}
			break;
		}
		*pp++ = (u_char)val;
		parts++;
		if (*cp == '.' || *cp == ':')
			dots++, cp++;
		if (*cp && !isxdigit(*cp))
			return(NULL);
		if (!*cp && (parts < 6))
			return (NULL);
		if (!dots && (n != 2))
			return(NULL);
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && !isspace(*cp))
		return (NULL);
	return (haddr);
}


/*
 *  Print the usage message and exit.
 */

#define USAGE "Usage: bootpquery haddr [htype] [-i ipaddr] [-s server]\n\
                        [-b bootfile] [-v vendor]\n"

usage()
{
	fprintf(stderr, USAGE);
	exit(1);
}
