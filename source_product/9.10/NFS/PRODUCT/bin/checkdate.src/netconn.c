/*	   @(#)1.2	87/02/02	*/
/* HPUX_ID: @(#)netconn.c	1.2		87/01/07 */

#include <stdio.h>
#include <errno.h>
#include <errnet.h>
#include <sys/types.h>
#include <sys/stat.h>

extern int	errno;
extern int	errnet;
char	naccess[] = "/users/nerfs/nfsmgr/naccess";
#define	UP_LEN	64
#define MACHLEN	52
struct netstat 
{
/*I hope no one will use names longer than 51 characters */
	char	machine[MACHLEN];
	char	userandpasswd[UP_LEN]; /*UP_LEN is posted by netunam(2)*/
	int	status; /*-1: connection attempt failed,
			  >0: number of times this connection used*/
} nst_array[20];
struct netstat *head = (struct netstat *) NULL;

/*current limit is posted by netunam(2)*/
#define MAX_NET_CON	20

char nerrmsg1[]="netunam_connect: netunam to %s failed, futher access to this\nmachine might be ignored.  errno = %d, errnet = %d\n";
char nerrmsg2[]="netunam_connect: no valid entry for %s in naccess\n";
char nerrmsg3[]="netunam_connect: malloc() failed.  errno = 0d%d\n";
char nerrmsg4[]="netunam_connect: Can't open %s. errno = 0d%d\n";
char nerrmsg5[]="netunam_connect: Can't disconnect %s, errno = 0d%d, errnet = 0d%d\n";
char nerrmsg6[]="netunam_connect: Can't disconnect %s, errno = 0d%d\n";
char nerrmsg7[]="netunam_connect: Can't connect to %s, errno = 0d%d, errnet = 0d%d, retry %d\n";
char nerrmsg8[]="netunam_connect: Can't connect to %s, errno = 0d%d, retry %d\n";
int	overflow = 0;	/*if number of entry in /usr/sca/naccess is larger
			  than MAX_NET_CON, set overflow*/

int	count = 0;  /*number of valid entry in array*/
FILE	*fp;

extern int debuglevel;

netconnect(netfile)
char	*netfile;  /*for example: netfile point to "/net/hpfclj"*/
{
	struct netstat *current, *lessentry;
	char	linebuf[128];
	char	mach[MACHLEN];
	char	mach1[MACHLEN+2];
	char	user1[32];
	char	passwd1[32];
	register char	*cp1, *cp2;
	register int	i, j;
	int	leastuse;
	int	retry;
	struct stat statbuf;

	if (debuglevel >=2) {
		fprintf(stderr, "try to netunam to %s\n", netfile);
	}
	strcpy(mach1, netfile);
	strcat(mach1, "/.");
	if (stat (mach1, &statbuf) == 0) {
		if (debuglevel >=2) {
			fprintf(stderr, "%s is connected already\n", netfile);
		}
		return(1);
	}
/* read naccess and fill in nst_arry (only the first time) */
	if(head == (struct netstat *) NULL) {
		head = nst_array;
		if ((fp = fopen(naccess, "r")) == NULL) {
			fprintf(stderr, nerrmsg4, naccess, errno);
			exit(errno);
		}
		for (current = head; fgets(linebuf, sizeof(linebuf), fp);) {
			if(linebuf[0] == '#') continue; /*comments*/
			strncpy(current -> machine, "/net/", 5);
			i=sscanf(linebuf, "%s %s %s", 
			       &current -> machine[5], user1, passwd1);
			if (i != 3) continue; /*ignore illegal format*/
			strcpy(current -> userandpasswd, user1);
			strcat(current -> userandpasswd, ":");
			strcat(current -> userandpasswd, passwd1);
			if (debuglevel >=3) {
				fprintf(stderr, "new naccess entry: %s\n",
					current -> userandpasswd);
			}
			current -> status = 0;
			current++;
			count++;
			if (count >= MAX_NET_CON) {
				if (debuglevel >=3) {
					fprintf(stderr, "naccess array is now full(OK)\n");
				}
				overflow = 1;
				break;
			}
		}
	}

	for (current=head, leastuse=0x7fffffff, i=0; i<=count; i++, current++){
		if (!strncmp(netfile, current -> machine, MACHLEN)){
			break;
		}
		if (!overflow) continue; /*if no overflow we don't need to
					   search for least used entry */
	/*now keep track of which entry is used least*/
		if (current -> status < 0) continue; /*skip bad connection*/
		if (leastuse > current -> status) {
			leastuse = current -> status;
			lessentry = current;
		}
	}
	if(i > count && !overflow) {
		fprintf(stderr, nerrmsg2, current -> machine);
		return(0);
	}
	else if (i > count) { /*if more than MAX_LAN_CON required,
				we disconnect the least used one
				and connect the new one*/
		if (leastuse == 0x7fffffff) {
			fprintf(stderr,"netconn: network is in trouble, abort!\n");
			exit(1);
		}
		current = lessentry;
		if(current -> status > 0) {
			if(j=netunam(current -> machine, "")) {
				if (errno == ENET) {
					fprintf(stderr, nerrmsg5, 
					current -> machine, errno, errnet);
				}
				else {
					fprintf(stderr, nerrmsg6, 
					current -> machine, errno);
				}
				return(0);
			}
			current -> status = 0;
		}
		rewind(fp);
		while(i = (int)fgets(linebuf, sizeof(linebuf), fp)) {
			if(linebuf[0] == '#') continue;
			j = sscanf(linebuf, "%s %s %s",mach,user1,passwd1);
			if (j != 3) continue;
			if (!strcmp(netfile + 5, mach)) {
				strcpy(current->machine,netfile);
				strcpy(current->userandpasswd,user1);
				strcat(current->userandpasswd,":");
				strcat(current->userandpasswd,passwd1);
				current -> status = 0;
				break;
			}
		}
		if (i == NULL) {
			fprintf(stderr, nerrmsg2, netfile);
			return(0);
		}
	}
			
	if (current -> status == 0) {
		for (retry = 0; retry++ < 5; ) {
			if(j = netunam(current->machine, current->userandpasswd)){
				if (errno == ENET) {
					fprintf(stderr, nerrmsg7, 
					current -> machine, errno, errnet,retry);
				}
				else {
					fprintf(stderr, nerrmsg8, 
					current -> machine, errno,retry);
				}
				continue;
			}
			break;
		}
		if (retry >= 5) {
			fprintf(stderr, nerrmsg1, current -> machine, 
				errno, errnet);
			current -> status = -1;
			return(0);
		}
/*		fprintf(stderr, "netunam to %s is successful\n", current -> machine);
*/
		current -> status = 1;
	}
	else if (current -> status == 0x7ffffffe); 
	else current -> status++;
#ifdef DEBUG
	fprintf(stderr, "==============machineid = %s,status = %d\n", current -> machine, current -> status);
#endif
	return(1);
}
