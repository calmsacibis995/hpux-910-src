#ifndef NULL
#include <stdio.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
/*	
	Trace flags from old gated
*/
#define	TR_NONE	0x0000
#define	TR_INT	0x0001
#define	TR_EXT	0x0002
#define	TR_RTE	0x0004
#define	TR_EGP	0x0008
#define	TR_UPD	0x0010
#define	TR_RIP	0x0020
#define	TR_HEL	0x0040
#define	TR_ICMP	0x0080
#define	TR_STMP	0x0100
#define	TR_GEN	0x000F
#define	TR_ALL	0x01FF

/*
	Protocol Defs
*/

#define	PR_RIP		0x00
#define	PR_HELLO	0x01
#define	PR_EGP		0x02

#define bool	int
#define TRUE	-1
#define FALSE	0

#define MAX_GROUP	40	/* number of egp neigbor groups */

typedef struct _str_list {
	char *str;
	struct _str_list *next;
} str_list;


/*
	Protocol structure used for holding protocol information
*/

typedef struct _proto 
{
	char mode;
/*
	Modes for a protocol (not all are supported with each protocol)
*/
#define	MODE_OFF	0x00 
#define	MODE_ON		0x01 
#define	MODE_QUIET	0x02 
#define	MODE_SUPPLIER	0x04 
#define	MODE_P2P	0x08 

	str_list *gw_value;
	str_list *maxup;	/* used by egp */
	str_list *trustedgw;
	str_list *srcgw;
	str_list *noout;
	str_list *noin;
	str_list *defaultmetric;

} proto;
	
typedef struct _intf 
{
	str_list *name;
	str_list * metric;
	struct _intf *next;
} intf;

typedef struct _neighbor 
{
	str_list *host;
	str_list *metricout;
	str_list *asin;
	str_list *asout;
	bool nogendefault;
	bool acceptdefault;
	bool defaultout;
	str_list *interface;
	str_list *srcnet;
	str_list *gateway;	
	struct _neighbor *next;
} neighbor;


typedef struct _cntl 
{
	bool use;
	str_list *addr;	/* intf for donot listen, gw for listen, AS for EGP */
	str_list *intf;	/* if proto is EGP, this the ASlist */
	str_list *proto;
	str_list *opps;	/* chain addrs, if the opposite use */
	str_list *metric;	
	struct _cntl *next;
} cntl;

typedef struct _rt {	/* default route parameters */
	str_list *addr;
	str_list *gw;
	str_list *proto;
	str_list  *metric;
	bool active;
	struct _rt *next;
} rt;

