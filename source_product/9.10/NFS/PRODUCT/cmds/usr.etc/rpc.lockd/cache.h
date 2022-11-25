/*
 * Used by the udp.c, tcp.c, and pmap.c for caching client connections.
 */
#include <sys/time.h>
#include <netdb.h>

struct cache {
	char *host;
	int prognum;
	int versnum;
	CLIENT *client;
	struct cache *nxt;
	struct hostent *hp;	/* Host information for contacting portmap */
	int h_index;		/* Index in hostent addr array */
	struct sockaddr_in server_addr;		/* IP addr/port of server */
	struct timeval paddrtime;	/* Time we started using current addr*/
	struct timeval psendtime;	/* Time we sent last pmap request */
	struct timeval addrtime;	/* Last time we got port from portmap*/
	int xid;		/* Transaction ID of pmap request */
	bool_t firsttime;	/* firsttime on this request */
	int port_state;		/* See below */
};

/*
 * Possible values for port_state
 */
#define PORT_INVALID	   0	/* Still looking for port number */
#define PORT_VALID_FIRST   1	/* Port is valid, but never tried */
#define PORT_VALID_TIMEOUT 2	/* Port is valid, has been tried at least once*/


/*
 * Some constants used with the portmap stuff 
 */
#define MAXWAITTIME 60	     /* Time to wait for a reply PER ADDRESS,
				matches timeout in pmap_getport */
#define MINWAITTIME 5   /* don't send out more often then every 5 secs */
