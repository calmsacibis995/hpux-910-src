/*
 * $Header: nipc_path.c,v 1.3.83.5 93/11/11 12:53:03 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_path.c,v $
 * $Revision: 1.3.83.5 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/11 12:53:03 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_path.c $Revision: 1.3.83.5 $";
#endif

/* 
 * The routines contained in this file operate on netipc path reports.
 * Specifically, they provide an interface for creating, parsing, and
 * manipulating nodal and connect site path reports.  These routines rely on
 * domain specific path routines pointed to by the nipc_domain structure to do
 * the actual work for each domain report within a path report. 
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/malloc.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ns_ipc.h"
#include "../nipc/nipc_err.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_domain.h"
#include "../nipc/nipc_protosw.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_hpdsn.h"
#include "../nipc/nipc_name.h"
#include "../netinet/in.h"

extern struct nipc_domain *nipc_domain_list;

/*   +--------------------------------+<-------------------------  
 *   |         report length          |				 \ 
 *   +--------------------------------+<-----------------         |
 *   |      domain report length      |                  \        |
 *   +--------------------------------+                   |       |
 *   | domain version|   domain id    |                   |       |
 *   +--------------------------------+                   |       |
 *   |                                |                 domain    |
 *   |               .                |                 report    |
 *   |               .                |                   |       |
 *   |               .                |                   |       |
 *   |                                |                   |       |
 *   |                                |                  /       path
 *   +--------------------------------+<-----------------       report
 *   |      domain report length      |                  \        |
 *   +--------------------------------+                   |       |
 *   | domain version|   domain id    |                   |       |
 *   +--------------------------------+                   |       |
 *   |                                |                   |       |
 *   |               .                |                 domain    |
 *   |               .                |                 report    |
 *   |               .                |                   |       |
 *   |                                |                   |       |
 *   |                                |                  /       /
 *   +--------------------------------+<-------------------------
 */

#define PR_LEN		0	/* offset to path report length		      */
#define PR_DRPT		1	/* offset to 1st domain report of path report */
#define DR_LEN		0	/* offset to domain report length	      */
#define DR_ID		1	/* offset to domain report version & id       */


/* path_addreport() appends a domain report to a path */
/* report mbuf chain.  It allocates a new mbuf if     */
/* there is insufficient room for the domain report   */
/* at the end of input mbuf chain.                    */

struct mbuf *
path_addreport(dr, m)

 u_short     *dr;	/* domain report in contiguous memory         */
 struct mbuf *m;	/* mbuf which domain report is to be added to */

{
	/* if there's not enough space in the current mbuf for the */
	/* domain report then allocate a new mbuf		   */
	if ((m->m_off + m->m_len + dr[DR_LEN] + sizeof(dr[DR_LEN])) > MMAXOFF) {
		m->m_next = m_get(M_WAIT, MT_SONAME);
		m = m->m_next;
		m->m_len = 0;
	}

	/* copy the domain report */
	bcopy((caddr_t)dr, mtod(m, caddr_t) + m->m_len,
			   (int)dr[DR_LEN] + sizeof(dr[DR_LEN]));

	/* adjust the mbuf length */
	m->m_len += dr[DR_LEN] + sizeof(dr[DR_LEN]);

	return (m);

}  /* path_addreport */


/* path_buildcsite() builds a connect site path */
/* report for a specified socket.               */

struct mbuf *
path_buildcsite(so, loopback)

struct socket *so;	 /* socket to build the connect site path report for */
int	      loopback;  /* non-zero if loopback interface is to be included */

{
	struct nipc_domain	*domain;	/* domain of the socket     */
	struct mbuf		*first = NULL;	/* first mbuf for the cspr  */
	struct mbuf		*m = NULL; /* mbuf chain under construction */
	short			*report_len;	/* path report length	    */

	/* find the domain for this socket */
	domain = ((struct nipccb *)(so->so_ipccb))->n_protosw->np_domain;

	/* allocate the first mbuf for the cspr */
	first = m_get(M_WAIT, MT_SONAME);

	/* save space in the first mbuf to put the report length */
	m = first;
	m->m_len = sizeof(short);
	report_len = mtod(m, short *);

	/* call the domain specific routine to build the csite domain reports */
	m = domain->nd_buildcsite(so, m, loopback);
	if (!m) {
		m_freem(first);
		return (m);
	}

	/* calculate the report length and return the cspr to the caller */
	*report_len = m_len(first) - sizeof(*report_len);
	return (first);

}  /* path_buildcsite */


path_getcsiteaddr(m, proto, addr)

struct mbuf	    *m;     /* mbuf chain containing connect site path report */
struct nipc_protosw *proto; /* specified protocol the caller wishes to reach  */
struct sockaddr     *addr;  /* address space to be filled in from npr info    */

{
	struct nipc_domain	*domain; /* domain of specified protocol    */
	struct sockaddr		laddr;	 /* address info from domain report */
	int			score;   /* score of addr from domain report*/
	int			best_score; /* score of best addr found     */
	u_short			*cspr;      /* cspr in contigous memory     */
	u_short			*cspr_end;  /* end of path report	    */
	u_short			*dr;	    /* pointer to domain report     */
	short			dlen;	    /* length of domain report      */
	int			mlen;       /* length of cspr mbuf	    */
	int			malloc_called = 0;

	/* make sure the cspr is in contigous aligned memory */
	if (((mlen = m_len(m)) > MMAXOFF) || (m->m_off & BYTE_ALIGN)) {
		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		cspr = (u_short *) kmalloc(mlen, M_NIPCPR, M_WAITOK);
		malloc_called++;
		m_copydata(m, 0, mlen, (caddr_t) cspr);
	} else
		cspr = mtod(m, u_short *); 

	/* get the domain of the specified protocol */
	domain = proto->np_domain;

	/* initialize scores */
	score = best_score = PATH_NO_PATH;

	/* loop through the domain reports calling the domain specific  */
	/* routine to get an addr to the specified protocol, saving the */
	/* addr with the best score					*/ 
	cspr_end = cspr + cspr[PR_LEN]/2;	
	for (dr = &cspr[PR_DRPT]; dr < cspr_end; dr += dlen + 1) {
		dlen = dr[DR_LEN]/2;
		if (dr[DR_ID] == domain->nd_domain) {
			score = domain->nd_getcsiteaddr(dr, proto, &laddr);
			if (score > best_score) {
				bcopy((char *)&laddr, (char *)addr, sizeof(struct sockaddr));
				if (score >= PATH_LOCAL_SUBNET)
					break;  /* best case; stop searching! */
				best_score = score;
			}
		}
	}

	if (malloc_called)
		FREE(cspr, M_NIPCPR);

	if (score == PATH_NO_PATH)
		return (E_NOPATH);
	else 
		return (0);

}  /* path_getcsiteaddr */


path_getnodeaddr(npr, proto, addr)

u_short *npr;		/* pointer to nodal path report in contiguous memory */
struct nipc_protosw *proto; /* specified protocol the caller wishes to reach */
struct sockaddr     *addr;  /* address space to be filled in from npr info   */

{
	struct nipc_domain	*domain; /* domain of specified protocol    */
	struct sockaddr		laddr;	 /* address info from domain report */
	int			score;   /* score of addr from domain report*/
	int			best_score; /* score of best addr found     */
	u_short			*npr_end;   /* end of path report	    */
	u_short			*dr;	    /* pointer to domain report     */
	short			dlen;	    /* length of domain report      */

	/* get the domain of the specified protocol */
	domain = proto->np_domain;

	/* initialize scores */
	score = best_score = PATH_NO_PATH;

	/* loop through the domain reports calling the domain specific  */
	/* routine to get an addr to the specified protocol, saving the */
	/* addr with the best score					*/ 
	npr_end = npr + npr[PR_LEN]/2;	
	for (dr = &npr[PR_DRPT]; dr < npr_end; dr += dlen + 1) {
		dlen = dr[DR_LEN]/2;
		if (dr[DR_ID] == domain->nd_domain) {
			score = domain->nd_getnodeaddr(dr, proto, &laddr);
			if (score > best_score) {
				bcopy((char *)&laddr, (char *)addr, sizeof(struct sockaddr));
				if (score >= PATH_LOCAL_SUBNET)
					break;  /* best case; stop searching! */
				best_score = score;
			}
		}
	}

	if (score = PATH_NO_PATH)
		return (E_NOPATH);
	else
		return (0);

}  /* path_getnodeaddr */


path_nodetocsite(npr, proto, protoaddr, cspr)

u_short			*npr;	    /* nodal path report in contigous memory */
struct nipc_protosw	*proto;	    /* protosw for connect site protocol     */
char			*protoaddr; /* connect site protocol address	     */
struct mbuf		**cspr;	    /* output connect site path report       */

{
	struct nipc_domain	*domain;       /* domain of the protocol  */
	struct mbuf		*first = NULL; /* first mbuf for the cspr */
	struct mbuf		*m = NULL; /* mbuf chain under construction */
	short			*report_len;   /* path report length	    */
	u_short			*npr_end;      /* end of nodal path report  */
	u_short			*ndr;	       /* nodal domain report ptr   */
	int			dlen;	       /* length of nodal domain rpt*/

	/* get the domain for this protocol */
	domain = proto->np_domain;

	/* allocate the first mbuf for the cspr */
	first = m_get(M_WAIT, MT_SONAME);

	/* save space in the first mbuf to put the report length */
	m = first;
	m->m_len = sizeof(short);
	report_len = mtod(m, short *);

	/* now loop through the domain reports within the npr, calling the    */
	/* domain specific routine to build a connect site domain report from */
	/* each nodal domain report that matches the caller's protocol domain */
	npr_end = npr + npr[PR_LEN]/2;	
	for (ndr = &npr[PR_DRPT]; ndr < npr_end; ndr += dlen + 1) {
		dlen = ndr[DR_LEN]/2;
		if (ndr[DR_ID] == domain->nd_domain) {
			m = domain->nd_nodetocsite(ndr, m, proto, protoaddr);
			if (!m) {
				m_freem(first);
				return (ENOBUFS);
			}
		}
	}

	/* calculate the report length */
	*report_len = m_len(first) - sizeof(*report_len);

	/* return the cspr to the caller (if we have one) */
	if (*report_len) {
		*cspr = first;
		return (0);
	} else {
		m_free(first);
		return (E_NOPATH);
	}
} /* path_nodetocsite */
		

path_reportvalid(pr_buf)

u_short	*pr_buf;	/* pointer to path report in contiguous memory */

{
	u_short	  	   *dr;		/* pointer to a domain report      */
	short		   dlen;	/* length of a domain report       */
	u_short		   *pr_end;	/* pointer to end of path report   */
	struct nipc_domain *domain;	/* domain struct for domain report */
	int		   error;	/* local error			   */

	/* verify that the path report length is even */
	if (pr_buf[PR_LEN] & 1)
		return (E_PATHREPORT);

	/* loop through the domain reports checking the domain report */
	/* length against the path report length and call the domain  */
	/* specific validation routine.				      */
	pr_end = pr_buf + pr_buf[PR_LEN]/2;	
	for (dr = &pr_buf[PR_DRPT]; dr <= pr_end; dr += dlen+1) {
		dlen = dr[DR_LEN]/2;
		if (dlen < 3)   /* must have vers, domain id, and ip addr */
			return (E_PATHREPORT);
		if (dr + dlen > pr_end)
			return (E_PATHREPORT);
		for (domain = nipc_domain_list; (domain); domain = domain->nd_next_domain)
			if (domain->nd_domain == dr[DR_ID])
				break;
		if (domain) {
			error = domain->nd_reportvalid(dr);
			if (error)
				return (error);
		}
	}

	return (0);

}  /* path_validreport */


