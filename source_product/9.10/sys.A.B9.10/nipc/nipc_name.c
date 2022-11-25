/*
 * $Header: nipc_name.c,v 1.5.83.5 93/11/11 12:52:57 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_name.c,v $
 * $Revision: 1.5.83.5 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/11 12:52:57 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_name.c $Revision: 1.5.83.5 $";
#endif

/* 
 * This file contains utility routines that operate on netipc node names and
 * socket names.  These routines are called primarily from ipcname(), 
 * ipcnamerase(), ipclookup(), and ipcdest() syscalls and the sockregd process.
 * The nm_downshift() and nm_islocal() utilities are also called by Probe.
 */
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/malloc.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ns_ipc.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_domain.h"
#include "../nipc/nipc_protosw.h"
#include "../nipc/nipc_err.h"
#include "../nipc/nipc_hpdsn.h"
#include "../nipc/nipc_name.h"
#include "../netinet/in.h"

extern struct mbuf * sou_sbdqmsg();
extern struct mbuf * path_buildcsite();
extern struct mbuf * prb_buildpr();
extern struct mbuf * prb_getcache();
extern void prb_init();

char	nipc_nodename[NS_MAX_NODE_NAME];
int	nipc_nodename_len = 0;
struct	socket	*nipc_pxpso;

#ifndef	TRUE
#define  TRUE  1
#define  FALSE 0
#endif

nm_downshift(name, nlen)

char *name;	/* name to be downshift */
int  nlen;	/* length of name       */

{
	for ( ; nlen--; name++)
		if (*name >= 'A' && *name <= 'Z')
			*name = (char)((int)*name + 0x20);

}  /* nm_downshift */


/* nm_getnpr() maps a node name into a nodal path report for */
/* the named node.  The work of obtaining the nodal path     */
/* is actually done by Probe, depending on whether the node  */
/* name is local or remote.                                  */

nm_getnpr(nodename, nlen, npr)

char	*nodename;	/* nodename to be mapped to a nodal path report */
int	nlen;		/* byte length of nodename			*/
u_short	**npr;		/* npr pointer to be returned  */

{
	struct mbuf	*m;  	/* mbuf chain containing npr   */
	u_short		*lnpr;	/* local ptr to npr in contiguous memory */
	caddr_t		cache;	/* probe cache address of npr  */
	int		local;  /* nodename is local initially */
	int		error;  /* local error		       */
	int		rlen;   /* path report length	       */

	/* determine if the nodename is local or remote node */
	local = nm_islocal(nodename, nlen);

	/* Interface to probe to get a nodal path report for nodename. */
	/* The interface used depends on whether nodename is local.    */
	if (local) {
		if ((m = prb_buildpr(PATH_LOOPOK)) == 0) 
			return ENOBUFS;
	} else {
		if ((m = prb_getcache(nodename, nlen, &cache, &error)) == 0) {
			if ((error == ENOBUFS) || (error == EINTR))
				return (error);
			else 
				return (E_NONODE);
		}
	}

	/* read the npr mbuf chain into contiguous memory and validate it */
	rlen = m_len(m);
	/*
	 *	Changed MALLOC call to kmalloc to save space. When
	 *	MALLOC is called with a variable size, the text is
	 *	large. When size is a constant, text is smaller due to
	 *	optimization by the compiler. (RPC, 11/11/93)
	 */
	lnpr = (u_short *) kmalloc(rlen, M_NIPCPR, M_WAITOK);
	m_copydata(m, 0, rlen, (caddr_t)lnpr);
	error = path_reportvalid(lnpr);

	/* free resources used to obtain npr, depending upon how we got it */
	if (local)
		m_freem(m);
	else
		prb_freecache(cache);

	*npr = lnpr;
	return (error);
}  /* nm_getnpr */


/* nm_soinit() is called by nipc_init() at system  */
/* boot time.  It creates the global pxp request */
/* socket used for remote ipclookups.            */

void
nm_soinit()

{
	struct nipc_protosw	*proto;	/* pxp protosw pointer		*/
	struct socket 		*sou_create();

	/* get the protosw pointer for pxp */
	if(mi_findproto(HPDSN_DOM, &proto, NSP_HPPXP, NS_REQUEST))
		panic("nm_soinit");

	/* allocate a socket structure and nipccb for the request socket */
	nipc_pxpso = sou_create(proto);

	/* attach the protocol to the socket */
	if((*proto->np_bsd_protosw->pr_usrreq)(nipc_pxpso, PRU_ATTACH,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0))
		panic("nm_soinit:2");

	/* bind a protocol address to the request socket */
	if((*proto->np_bsd_protosw->pr_usrreq)(nipc_pxpso, PRU_BIND,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0))
		panic("nm_soinit:3");
	
	/* initialize probe level 4 */
	(void) prb_init();

}  /* nm_soinit */


/* nm_isinvalid() downshifts a node name and */
/* verifies that it is valid syntax.         */

#define	ALPHA(c)		((c) >= 'a' && (c) <= 'z')
#define	ALPHA_NUM(c)		(ALPHA(c) || (c) >= '0' && (c) <= '9')
#define	ALPHA_NUM_DASH(c)	(ALPHA_NUM(c) || (c) == '-' || (c) == '_')

nm_isinvalid(name, nlen)

char 	*name;		/* nodename to be validated */
int 	nlen;		/* length of the nodename   */

{
	 int	parts = 1;	/* count of nodename parts           */
	 int	newpart;	/* flag indicating start of new part */
	 int	part_len = 0;	/* count of characters in each part  */

	/* verify that the nodename length is in bounds */
	if(nlen == 0)
		return (0);	
	if(nlen > NS_MAX_NODE_NAME) 
		return E_NLEN;
	
	/* put the name in lowercase */
	nm_downshift(name, nlen);

	/* parse the nodename ensuring each part begins with an alphabetic */
	/* character, followed by and alphanumeric character, a dash, or   */
	/* an underscore; ensure that each part has a valid length         */

	for (newpart = 1 ; nlen ; name++, nlen--, part_len++) {
		if(newpart) {
			if(!ALPHA(*name))
				return E_NODENAMESYNTAX;
			newpart = 0;
			continue;
		}
		if(*name == '.') {
			/* end of a part; verify the length */
			if (part_len > NS_MAX_NODE_PART)
				return E_NODENAMESYNTAX;
			newpart = 1;	/* next character begins a new part */
			part_len = -1;	/* reset part_len for the next part */
			parts++;	/* increment number of parts count  */
		} else if (!ALPHA_NUM_DASH(*name))
			return E_NODENAMESYNTAX;
	}

	/* verify the length of the last part */
	if ((part_len == 0) || (part_len > NS_MAX_NODE_PART))
		return E_NODENAMESYNTAX;

	/* verify that there were not more than 3 parts */
	if (parts > NM_MAXPARTS)
		return E_NODENAMESYNTAX;

	return (0);

}  /* nm_isinvalid */
	

/* nm_islocal() compares the input nodename */
/* to the local netipc nodename and returns */
/* true or false.                           */

nm_islocal(name, nlen)
	char *name;
	int  nlen;
{
	if(nlen == 0)
		return TRUE;
	if(nlen != nipc_nodename_len)
		return FALSE;
	if (bcmp(name, nipc_nodename, nlen) == 0)
		return TRUE;
	else
		return FALSE;

}  /* nm_islocal */


/* nm_locallookup() is called by ipclookup for a local   */
/* node.  It looks up the socket name in the local       */
/* socket registry and outputs the socket type, protocol */
/* and connect site path report for the named socket     */
/* or destination descriptor.                            */

nm_locallookup(soname, nlen, loop, protocol, sockkind, cspr)

char	*soname;	/* socket name to lookup in local socket registry     */
int	nlen;		/* length of soname				      */
int	loop;		/* loopback flag for building local cspr              */
int	*protocol;	/* (output) nipc protocol the socket is bound to      */
int	*sockkind;	/* (output) nipc type of the named socket	      */
struct mbuf **cspr;	/* (output) connect site path report for named socket */

{
	struct file	*fp;	/* file table entry for the named socket    */
	struct nipccb	*ncb;	/* netipc control block of the named socket */
	struct mbuf	*m;	/* local mbuf pointer used in m_copy        */
	int		error;	/* local error				    */

	/* get the ftab pointer for the named socket from our socket registry */
	if (error = sr_lookup(soname, nlen, &fp))
		return (error);

	/* trace ftab pointer to the socket's nipccb */
	ncb = (struct nipccb *)(((struct socket *)(fp->f_data))->so_ipccb);

	/* get the caller's output protocol parm from the nipccb protosw */
	*protocol = ncb->n_protosw->np_protocol;

	/* get the sockkind and cspr output parms depending on whether */
	/* this is a named destination descriptor or a call socket     */
	if (ncb->n_type == NS_DEST) { 
		/* merely copy the info */
		*sockkind = ncb->n_dest_type;
		m = ncb->n_cspr;
		*cspr = m_copy(m, 0, M_COPYALL);
	} else {  /* must be a call socket */
		*sockkind = ncb->n_type;
		/* build a local cspr */
		*cspr = path_buildcsite((struct socket *)fp->f_data, loop);
	}

	/* return status to the caller */
	if (!*cspr)
		return ENOBUFS;
	else
		return (0);

}  /* nm_locallookup */


/* nm_parsereply() is called by nm_remotelookup(). */
/* It parses an ipclookup reply received from a    */
/* remote sockregd.  If the reply was successful   */
/* then the protocol, sockkind and connect site    */
/* path report are extracted for output.           */

nm_parsereply(m, proto, kind, cspr)

struct mbuf	*m;	/* mbuf chain containing the reply data 	     */
int		*proto; /* (output) nipc protocol extracted from the reply   */
int		*kind;  /* (output) socket kind extracted from the reply     */
struct mbuf	**cspr; /* (output) pointer to cspr mbuf chain from the reply*/

{
	struct nm_lookupreply	*reply;	/* pointer to reply message      */
	struct nm_lookuphdr	*hdr;	/* pointer to fixed reply header */
	struct mbuf		*lcspr;	/* local pointer to cspr mbuf	 */
	u_short			*pr;	/* pointer to contiguous path report */
	int			msglen;	/* message length variable	 */
	int			error=0;/* local error			 */
	int			malloc_called = 0; 

	/* ensure that the fixed reply hdr is contiguous and aligned */
	if (((m->m_off > MMAXOFF) || (m->m_len < NM_HDR_LEN) ||
	     (m->m_off & BYTE_ALIGN)) &&
	    (m = m_pullup(m, NM_HDR_LEN)) == 0) {
		error = E_BADREGMSG;
		goto ERROR_RETURN;
	}
	hdr = mtod(m, struct nm_lookuphdr *);

	/* validate the reply message type and length */
	msglen = m_len(m);
	if ((hdr->hdr_msgtype != NM_REPLY) ||	      /* bad type      */
	    (hdr->hdr_msglen > msglen) ||	      /* bad msg length*/
	    ((hdr->hdr_error == 0) &&
	   	(hdr->hdr_msglen <= NM_REPLY_LEN)) || /* msg too small */
	    ((hdr->hdr_error != 0) &&
	    	(hdr->hdr_msglen != NM_HDR_LEN))) {   /* bad hdr size  */
		error = E_BADREGMSG;
		goto ERROR_RETURN;
	}
	msglen = hdr->hdr_msglen;

	/* check for an error reply */
	if (hdr->hdr_error == NM_VERSION_RESULT)
		error = E_VERSION;
	else if (hdr->hdr_error == NM_NOTFOUND_RESULT)
		error = E_NAMENOTFOUND;
	else if (hdr->hdr_error != NM_SUCCESS_RESULT)
		error = E_BADREGMSG;
	if (error) 
		goto ERROR_RETURN;

	/* we have a good reply - ensure the reply header is contiguous */
	if ((m->m_len < NM_REPLY_LEN) &&
	   (m = m_pullup(m, NM_REPLY_LEN)) == 0) {
		error = E_BADREGMSG;
		goto ERROR_RETURN;
	}
	reply = mtod(m, struct nm_lookupreply *);

	/* extract the caller's output parms from the reply */

	/* NOTE: In previous revisions (6.5, 7.0) it was thought that   */
	/* protocol was kept in the reply header.  This manifested      */
	/* itself as a bug in 8.0.  To fix this problem, instead of     */
	/* trying to extract the output proto parm from the reply, we   */
	/* merely set the proto to TCP (since TCP is the only protocol  */
	/* that Netipc is supported over).  -slf       		        */

	*proto = NSP_TCP;
	*kind  = reply->reply_sockkind;

	/* strip off the reply header to get the cspr mbuf chain */
	msglen = msglen - NM_REPLY_LEN;
	m_adj(m, NM_REPLY_LEN);
	if (m->m_len == 0)
		m = m_free(m);
	lcspr = m;

	/* make sure the cspr is in contiguous memory and   */
	/* call the validation routine to see if it's valid */
	if (msglen > MMAXOFF) {
		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		pr = (u_short *) kmalloc(msglen, M_NIPCPR, M_WAITOK);
		m_copydata(lcspr, 0, msglen, (caddr_t) pr);
		malloc_called++;
	} else {
		if (((lcspr->m_next) || (lcspr->m_off > MMAXOFF) ||
		     (m->m_off & BYTE_ALIGN)) &&
	   	   (lcspr = m_pullup(lcspr, msglen)) == 0) {
			m_freem(lcspr);
			return (E_BADREGMSG);
		}
		pr = mtod(lcspr, u_short *);
	}
	if (error = path_reportvalid(pr))
		m_freem(lcspr);
	else
		*cspr = lcspr;

	if (malloc_called)
		FREE(pr, M_NIPCPR);

	return (error);

ERROR_RETURN:
	m_freem(m);
	return (error);

}  /* nm_parsereply */


/* nm_qualify() fully qualifies a node name according */
/* to the local netipc nodename.                      */

nm_qualify(name, nlen)

char		*name;	/* nodename to be qualified */
int		*nlen;	/* length of name           */

{
	int		len;
	int		parts;
	char		*src, *dst;

	/* downshift the name and check for valid syntax */
	if (nm_isinvalid(name, *nlen))
		return E_NODENAMESYNTAX;

	/* count the number of parts in the input name */
	for (dst = name, parts = 1; dst < &name[*nlen]; dst++)
		if(*dst == '.')
			parts++;

	/* if there were 3 parts the name is already qualified */
	if(parts == 3)
		return (0);

	/* verify the nipc_nodename can be used to qualify the caller's name */
	if(nipc_nodename_len == 0)
		return E_NODENAMESYNTAX;

	/* determine the position in nipc_nodename where the copy must begin */
	for (src = nipc_nodename; parts; src++)
		if(*src == '.')
			parts--;
	src--;	/* go back and pick up the period */

	/* determine the number of characters to be copied */
	len = &nipc_nodename[nipc_nodename_len] - src;

	/* do the copy to qualify the caller's name and update the nlen */
	bcopy(src, dst, len);
	*nlen = *nlen + len;

	return (0) ;

}  /* nm_qualify */


/* nm_random() is called by ipcname() to create a */
/* random socket name for the caller.             */

nm_random(seed, name)
	caddr_t		seed;	/* 4-byte address to be used as name seed */
	char		*name;  /* location where random name is returned */

{
	int		i;
	char		saved_name[NM_SOCKLEN];
	struct file	*fp;

	/* stretch the 4-byte address to 8 bytes of random name and save it */
	for (i = 0; i < NM_SOCKLEN; i = i+2) {
		name[i] = ((int)seed) >> ((i*4) & 0xff);
		name[i+1] = name[i];
	}
	nm_downshift(name, NM_SOCKLEN);
	bcopy(name, saved_name, NM_SOCKLEN);

	/* Loop until we find a name which isn't in the table.  Each time */
	/* through the loop (name not unique) we increment the name one   */
	/* byte at a time odometer style and try again.  If the index     */
	/* reaches 8, then all possible combination of (lower case)       */
	/* characters which make up an eight byte random name are already */
	/* in our table - an unlikely condition to say the least!         */
	while (sr_lookup(name, NM_SOCKLEN, &fp) == (0)) {
		i = 0;
		name[i]++;
		if (name[i] >= 'A' && name[i] <= 'Z')
			name[i] = (char)((int)name[i] + 0x20); /* lower case */
		while (name[i] == saved_name[i]) {
			i++;
			if (i == NM_SOCKLEN)
				return;  /* we've tried all combinations! */
			name[i]++;
		}
	}

} /* nm_random */


/* nm_recvreply() is called by nm_remotelookup().    */
/* It waits for a reply message on the PXP request   */
/* socket and returns the data mbuf if there is one. */

nm_recvreply(msgid, m0)

int	msgid;	/* PXP message id for the request/reply message */
struct mbuf	**m0;	/* mbuf data chain for received message */

{
	struct mbuf	*m;	/* reply mbuf data chain 	  	  */
	struct sockbuf  *sb;    /* receive socket buffer 		  */
	int		*reply;	/* reply pointer used for checking errors */
	int		s;	/* spl setting		 		  */
	int		error = 0;	/* local error */

	sb = &nipc_pxpso->so_rcv;

	/* loop until we get our reply message */
	for (;;) {
		/* lock the socket buffer */
		while ((sb->sb_flags & SB_LOCK) && !error) {
			sb->sb_flags |= SB_WANT;
			error = sleep((caddr_t)&(sb)->sb_flags, PZERO+1|PCATCH);
		}
		if (error) {
			return (EINTR);
		}
		sb->sb_flags |= SB_LOCK;

		/* see if we have data */
		if (sb->sb_cc)  {
			m = sou_sbdqmsg(sb, msgid);
			if (m) 
				break;
		}

		/* wait for data to arrive */
		sbunlock(sb);
		sb->sb_flags |= SB_WAIT;
		error = sleep((caddr_t)&sb->sb_cc, PZERO+1|PCATCH);
		if (error) {
			return(EINTR);
		}
	}	

	/* we got our reply - release the resources we're holding */
	sbunlock(sb);  

	/* check the error field at the front of the reply mbuf */
	/* to see if there is reply data (successful reply)     */
	reply = mtod(m, int *);
	reply++;   /* error code follows message id */
	if (*reply) {
		/* error reply - no data for the caller */
		m_free(m); /* free the error reply mbuf */
		return (E_NOREGRESPONSE);
	}

	/* remove the message id and error code from the reply mbuf chain;    */
	/* if this leaves an emtpy mbuf at the front of the chain then free it*/
	m_adj(m, 2*(sizeof(int)));
	if (m->m_len == 0)
		m = m_free(m);
	
	if (m) {
		*m0 = m;
		return (0);
	} else
		return (E_NOREGRESPONSE);

}  /* nm_recvreply */


/* nm_remotelookup() is called by ipclookup() when the user */
/* specifies a remote node.  This routine uses the global   */
/* PXP request socket to send an ipclookup request to the   */
/* remote sockregd process.  When the reply comes back, it  */
/* extracts the protocol, socket kind, and connect site path*/
/* report for the named remote socket.                      */

nm_remotelookup(nodename, nlen, sockname, slen, proto, kind, cspr)

char	*nodename;	/* qualified nodename where named socket resides     */
int	nlen;		/* length (in bytes) of nodename 		     */
char	*sockname;	/* socket name to lookup	 		     */
int	slen;		/* length (in bytes) of sockname 		     */
int	*proto;		/* (output) nipc protocol bound to named socket      */
int	*kind;		/* (output) nipc socket kind of named socket         */
struct mbuf	**cspr;	/* (output) connect site path report of named socket */

{
	struct mbuf	*nam;	/* mbuf pointer for remote address info	      */
	struct mbuf	*m;	/* mbuf pointer for lookup request/reply data */
	struct nm_lookupreq	*req;	/* request message pointer            */
	struct nm_lookuphdr	*hdr;	/* request message header pointer     */
	u_short		*npr;	/* pointer to nodal path report 	      */
	int		msgid;	/* request message identifier assigned by PXP */
	int		error;	/* local error				      */
	struct sockaddr	*addr;	/* sockaddr for remote PXP sockregd port      */
	struct sockaddr_in	*addr_in;	/* AFINET version of sockaddr */
	struct nipc_protosw	*pxpproto;	/* nipc protosw for PXP	      */

	/* initialize the pxp protosw pointer */
	pxpproto = ((struct nipccb *)(nipc_pxpso->so_ipccb))->n_protosw;

	/* get an mbuf for the remote address */
	nam = m_get(M_WAIT, MT_SONAME);
	nam->m_len = sizeof(struct sockaddr);
	bzero((caddr_t)nam, sizeof(struct sockaddr));
	addr = mtod(nam, struct sockaddr *);
	addr_in = (struct sockaddr_in *) addr;

	/* get a nodal path report for the remote nodename and extract	*/
	/* an internet address from it					*/
	error = nm_getnpr(nodename, nlen, &npr);
	if (!error)  {
		error = path_getnodeaddr(npr, pxpproto, addr);
		FREE(npr, M_NIPCPR);
	}
	if (error) {
		m_free(nam);
		return (error);
	}
		
	/* initialize the sockaddr port to the well-known PXP sockregd port */
	addr_in->sin_port = NM_SERVER;

	/* get an mbuf for the lookup request */
	m = m_get(M_WAIT, MT_DATA);
	m->m_len = NM_REQ_LEN + slen;
	req = mtod(m, struct nm_lookupreq *);
	hdr = &(req->req_hdr);

	/* fill in the lookup request message */
	hdr->hdr_msglen = NM_REQ_LEN + slen;
	hdr->hdr_pid	= NM_PID;
	hdr->hdr_msgtype= NM_REQUEST;
	hdr->hdr_seqnum	= 0;
	hdr->hdr_capmask= NM_CAPABILITY;
	hdr->hdr_error	= 0;
	hdr->hdr_version= NM_VERSION;
	hdr->hdr_unused = 0;
	req->req_nameptr= NM_REQ_LEN;
	req->req_endptr	= NM_REQ_LEN + slen;
	bcopy(sockname, (char *)hdr + req->req_nameptr, slen);

	/* send the request on our global pxp request socket */
	error = (*pxpproto->np_bsd_protosw->pr_usrreq)(nipc_pxpso, PRU_SEND, m,
						nam, (struct mbuf *) &msgid);
	m_free(nam);	/* free the sockaddr mbuf */
	if (error) 
		return (E_CANTCONTACTSERVER);

	/* wait for a reply to our request */
	error = nm_recvreply(msgid, &m);
	if (error) {
		/* abort the outstanding request if we got an interrupt */
		if (error == EINTR)
			(*pxpproto->np_bsd_protosw->pr_usrreq)(nipc_pxpso,
				PRU_REQABORT, (struct mbuf *) 0,
				(struct mbuf *) 0, (struct mbuf *) &msgid);
			
		return (error); 
	}

	/* parse the lookup reply, extracting the protocol, sockkind, and cspr*/
	error = nm_parsereply(m, proto, kind, cspr);

	return (error);

}  /* nm_remotelookup */
