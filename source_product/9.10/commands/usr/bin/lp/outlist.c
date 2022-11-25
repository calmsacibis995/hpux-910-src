/* $Revision: 64.1 $ */
/* routines to manipulate output request list */

#include	"lp.h"
#include	"lpsched.h"

/* newol() -- returns pointer to empty output list */

struct outlist *
newol()
{
	struct outlist *ol;

	if((ol = (struct outlist *) malloc(sizeof(struct outlist))) == NULL)
		fatal(CORMSG, 1);

#ifdef REMOTE
	ol->ol_ob3   = FALSE;
	ol->ol_host  = NULL;
	ol->ol_seqno = -1;
	ol->ol_time  = 0;
#else
	ol->ol_seqno = ol->ol_time = 0;
#endif REMOTE
	ol->ol_name  = NULL;
	ol->ol_print = ol->ol_dest = NULL;
	ol->ol_next  = ol->ol_prev = ol;
	ol->ol_priority = 0;

	return(ol);
}

/* inserto(d, priority, seqno, name) -- inserts output request with
        sequence number	'seqno' and logname 'name' at the tail of
	the output list for priority 'priority' of destination 'd' */
/* the remote spooling version has a host parameter that identifies the
	host name of the originating system */

#ifdef REMOTE
inserto(d, priority, seqno, name, host, ob3)
char	*host;
int	ob3;
#else
inserto(d, priority, seqno, name)
#endif REMOTE
struct dest *d;
short priority;
int seqno;
char *name;
{
	static int timer = 1;	/* counter to order entry of requests */
	struct outlist *new, *head;
	struct prilist	*pri, *getpri();
	char *s, *strcpy(), *strncpy();
#ifdef REMOTE
	char *h;		/* points to host name */
#endif REMOTE

	if((new = (struct outlist *) malloc(sizeof(struct outlist))) == NULL ||
#ifdef REMOTE
	   (s = malloc((unsigned)(strlen(name) + 1))) == NULL ||
	   (h = malloc((unsigned)(SP_MAXHOSTNAMELEN))) == NULL)
#else
	   (s = malloc((unsigned)(strlen(name) + 1))) == NULL)
#endif REMOTE
		fatal(CORMSG, 1);

	new->ol_seqno = seqno;
	new->ol_time = timer++;
	strcpy(s, name);
	new->ol_name = s;
	new->ol_priority =  priority;
#ifdef REMOTE
	strncpy(h, host, SP_MAXHOSTNAMELEN);
	new->ol_host = h;
	new->ol_ob3  = ob3;
#endif REMOTE
	new->ol_print = NULL;
	new->ol_dest = d;

        pri = getpri(d, priority);

	head = pri->pl_output;
	new->ol_next = head;
	new->ol_prev = head->ol_prev;
	head->ol_prev = new;
	(new->ol_prev)->ol_next = new;
}

/* deleteo(o) -- delete output entry o */

deleteo(o)
struct outlist *o;
{
	free(o->ol_name);
#ifdef REMOTE
	free(o->ol_host);
#endif REMOTE

	(o->ol_next)->ol_prev = o->ol_prev;
	(o->ol_prev)->ol_next = o->ol_next;

	free((char *) o);
}

/* geto(d, seqno) -- returns pointer to the output list structure for
	destination d with sequence number seqno.  If this doesn't exist,
	then geto returns NULL.
*/

struct outlist *
#ifdef REMOTE
geto(d, seqno, host)
char *host;
#else
geto(d, seqno)
#endif REMOTE
struct dest *d;
int seqno;
{
	struct outlist *head, *o ,*foundo;
	struct prilist *headpl, *p;
	short foundit;

	foundo = NULL;

	for(foundit = FALSE, headpl = d->d_priority, p = headpl->pl_next ;
	    !foundit && p != headpl ; p = p->pl_next){
		for(head = p->pl_output, o = head->ol_next;
		    !foundit && o != head; o= o->ol_next){
#ifdef REMOTE
			if(o->ol_seqno == seqno &&
			   strcmp(o->ol_host,host) == 0){
#else REMOTE
			if(o->ol_seqno == seqno){
#endif REMOTE
				foundit = TRUE;
				foundo = o;
			}
		}
	}
	return(foundo);
}
