/*
 * $Header: uipc_mbuf.c,v 1.8.83.7 94/10/11 08:14:02 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/uipc_mbuf.c,v $
 * $Revision: 1.8.83.7 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 94/10/11 08:14:02 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) uipc_mbuf.c $Revision: 1.8.83.7 $ $Date: 94/10/11 08:14:02 $";
#endif

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)uipc_mbuf.c	7.10 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/mbuf.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/uio.h"

/*
** Define MBUF_QA2 when you are trying to debug
** a "freeing free mbuf" type of problem.  When #define'd, this will
** cause additional code to be compiled into the kernel which will help
** you catch the code which is freeing mbufs twice.  Specifically, when
** #define'd, the following will be compiled into the kernel (in 
** uipc_mbuf.o) :
**
** 1. an array is statically declared for holding mbuf addresses
**    (mbuf_freeq[]).  This array is used to see if an address being
**    returned has already been returned.
**
** 2. Stopping the kernel when a free mbuf is being returned isn't enough:
**    you really want to know who returned it the first time.  So, when
**    MBUF_QA2 is #define'd, another array (mbuf_freestk[]) is statically
**    declared for holding the address of the caller (stktrc, m_free and
**    m_freem excepted).  This is separate from the stack trace info which
**    is stored in the mbuf data area.  Perhaps this is overkill, but it
**    provides another opportunity to determine who originally freed the
**    mbuf;  the stack-trace in the panic will tell you who returned it the
**    second time.  The mbuf data area may be partially or completely
**    overwritten in between these times, hence the need for the redundant
**    information to be kept.
**
** 3. when m_free is called, a check is made to verify that the address
**    being returned is not already in mbuf_freeq[].  If found,
**    a panic is generated.  You can check the dump to find out more
**    info; the address of the mbuf being returned is stored in "bad_mbuf".
**    Otherwise, the address of the mbuf being returned is stored
**    in mbuf_freeq[], in FIFO order. 
**
** 4. when m_free is called, the stack trace showing the caller(s)
**    is stored in the mbuf data area.  You can use this information
**    to find out who returned the mbuf the first time (if it isn't
**    overwritten, that is).
**
** 5. when m_get is called, if the array holding mbufs is less than
**    at half capacity, then a new mbuf is obtained.  Otherwise, the
**    first mbuf in the array is returned;  the entries in this array
**    are moved up by one slot.  Thus, a large amount of time is
**    forced to occur between the time when an mbuf is returned and
**    the time when it is next allocated;  hopefully, this time will
**    be long enough to "catch the code in the act" of returning the
**    mbuf again.
*/
#ifdef MBUF_QA2
#define MBUF_FREEQ_SIZE 2048
struct mbuf *mbuf_freeq[MBUF_FREEQ_SIZE];
int mbuf_freestk[MBUF_FREEQ_SIZE][2];
int    check_for_dup_mbuf_flag = 0;
int mbufs_allocated_from_mbuf_freeq = 0;
void mbuf_free_qa2();
#endif /* MBUF_QA2 */


struct mbstat mbstat;
int	m_want;
struct mbuf *mfree;

int    netmem_factor = 10;
int    netmemmin = (200 * 1024);
extern int maxmem;
extern int netmemmax;

mbinit()
{
	static int	initialized = 0;
#ifdef MBUF_QA2
	int i;
#endif /* MBUF_QA2 */

	if (initialized)
		panic("mbinit: called twice");
	initialized = 1;
	INIT_Q(free_queue);

#ifdef MBUF_QA2
	for (i = 0; i <  MBUF_FREEQ_SIZE ; i++)
	    mbuf_freeq[i] = (struct mbuf *) 0;
#endif /* MBUF_QA2 */

	netmem_init();
	return; 	/* As no list need be maintained now */
}

netmem_init()
{

   /*
    * HP: The IP Reassembly problem wherein we could lock memory in reassembly
    *     queues.  The following allows the user to limit the maximum amount
    *     of memory in bytes that can be used for IP fragmentation reassembly
    *     queue memory.
    *
    *     if netmemmax <  0, there is no restriction on networking memory.
    *                  == 0, networking memory is default percentage of maxmem.
    *                  >  0, user specified dynamically malloc'ed mem in bytes
    *
    *     if netmemmax == 0, percentage is determined by netmem_factor, ie. we
    *                        allocate netmem_factor/100 * memmax.
    * 
    *     NOTE: If for any reason netmemmax happens to be less than netmemmin,
    *     we will initialize netmemmax to netmemmin to ensure a minimum amount
    *     of networking memory.
    */

   if (netmemmax == 0) {
      /* 
       *compute netmemmax as a percentage of maxmem and round it up 
       * to multiple of pages ie NBPG
       */
      if ((netmem_factor > 0) && (netmem_factor < 100)) {
         netmemmax = ((maxmem * netmem_factor + 99) / 100) * NBPG;
      }
      else {
         netmemmax = -1;
         }
    
      VASSERT(netmemmax != 0);
   }
   else if (netmemmax > 0) {
      /*
       * round up netmemmax to multiple of pages for efficiency of mem
       * utilization.  take care to avoid integer overflow.
       */
      if (netmemmax < (unsigned)maxmem * NBPG - NBPG + 1) 
         netmemmax = ((netmemmax + NBPG - 1) / NBPG) * NBPG;

      VASSERT(netmemmax != 0);
   }

   if ((netmemmax > 0) && (netmemmax < netmemmin)) {
      netmemmax = ((netmemmin + NBPG - 1) / NBPG) * NBPG;
   }

   if (netmemmax >= (unsigned)maxmem * NBPG - NBPG + 1) {
      netmemmax = -1;
   }

#ifndef QUIETBOOT
   if (netmemmax < 0)
      printf("Networking memory for fragment reassembly is unrestricted\n");
   else
      printf("Networking memory for fragment reassembly is restricted to %d bytes\n", netmemmax);
#endif /* ! QUIETBOOT */

}

#ifdef MBUF_QA2
pack_freeq(m)
struct mbuf *m;
{
    int freeq_size;

    for (freeq_size = 0; freeq_size <  MBUF_FREEQ_SIZE-1; freeq_size++) {
	mbuf_freeq[freeq_size] = mbuf_freeq[freeq_size+1];
	if (mbuf_freeq[freeq_size] == (struct mbuf *) 0) {
	    mbuf_freeq[freeq_size] = m;
	    return;
	}
    }
    mbuf_freeq[freeq_size] = m;
    return;
}
#endif /* MBUF_QA2 */

struct mbuf *
m_get(canwait, type)
int	canwait, type;
{
	register struct mbuf *m;
#ifdef MBUF_QA2
    int i, s;

    if (check_for_dup_mbuf_flag && (mbuf_freeq[MBUF_FREEQ_SIZE>>1] != 0)) {
	s = splimp();
	if ((m = mbuf_freeq[0]) != (struct mbuf *) 0) {
	    /* The mbuf_freeq is at least half full.  Allocate the new
	    ** mbuf from the first element in the array (that's the oldest
	    ** one, i.e., the one that's been free the longest), and move
	    ** the others up one notch.
	    */
	    ++mbufs_allocated_from_mbuf_freeq;
	    pack_freeq(0);

	    /* Make sure this mbuf isn't still in there (that is,
	    ** that the packing worked correctly).
	    */
	    for (i = 0; i < MBUF_FREEQ_SIZE; i++) {
		if (mbuf_freeq[i] == (struct mbuf *) 0) break;
		if (mbuf_freeq[i] == m) {
		    printf("m_get: mbuf %x in freeq, slot %d address %x",
			m, i, &mbuf_freeq[i]);
		    panic("m_get: mbuf in freeq (3)");
		}
	    }
	    (m)->m_type = type;
	    (m)->m_act  = 0;
	    (m)->m_next = 0;
	    (m)->m_flags = 0;
	    (m)->m_off  = MMINOFF;
	    splx(s);
	return(m);
	}
	splx(s);
    }
#endif /* MBUF_QA2 */

    MGET(m, canwait, type);
    return(m);
}


#ifdef MBUF_QA2
struct mbuf *
mclget(m)
struct mbuf *m;
{
    MCLGET(m);
}


struct mbuf *
mclgetvar_wait(m,size)
struct mbuf *m;
{
    MCLGETVAR_WAIT(m,size);
}


struct mbuf *
mclgetvar_dontwait(m,size)
struct mbuf *m;
{
    MCLGETVAR_DONTWAIT(m,size);
}
#endif /* MBUF_QA2 */


struct mbuf *
m_getclr(canwait, type)
int	canwait, type;
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	if (m)
		bzero(mtod(m, caddr_t), MLEN);
	return (m);
}


/* Allocate a LARGEBUF, that is, one whose data is owned by someone else */
struct mbuf *
mclgetx(fun, arg, addr, len, wait)
int	(*fun)(), arg, len, wait;
caddr_t addr;
{
	register struct mbuf *m;


	MGET(m, wait, MT_DATA);
	if (m == 0)
		return (0);
	m->m_off = (int)addr - (int)m;
	m->m_len = len;
	m->m_clsize = len;
	m->m_head = m;
	m->m_refcnt = 1;
	m->m_cltype = MCL_NFS;
	m->m_clfun = fun;
	m->m_clarg = arg;
	return (m);
}

#ifdef  __hp9000s800
struct mbuf *
mclget_dux(fun, arg, addr, len, wait)
int	(*fun)(), arg, len, wait;
caddr_t addr;
{
	register struct mbuf *m;


	MGET(m, wait, MT_DATA);
	if (m == 0)
		return (0);
	m->m_off = (int)addr - (int)m;
	m->m_len = len;
	m->m_cltype = MCL_DUX;
	m->m_clsize = len;
	m->m_sid = 0;
	m->m_dux_clfun = fun;
	m->m_dux_clarg = arg;
	return (m);
}
#endif  /* hp9000s800 */

/* Check to see if m points to a cluster.
   If m doesn't point to a cluster, simply call FREE to free the mbuf.
   If m points to a type 1 cluster, decrement the reference count and
   return the cluster. If m is the first mbuf pointing to this cluster
   then we free the mbuf only if the reference count has become zero.
   If this is not the first mbuf and the reference count becomes zero,
   then both m and the header mbuf are freed.
   If m points to a type 2 cluster (NFS), then the corresponding free
   routine is called.

   The MBUF_QA2 code is for debugging "freeing free mbuf" type problems.
   It is normally turned off in production kernels.
*/

#ifdef MBUF_QA2
struct mbuf *bad_mbuf;
#endif /* MBUF_QA2*/

struct mbuf *
m_free(m)
struct mbuf *m;
{
	register struct mbuf *n;
	int	ms;
	int	flag;

#ifdef	MBUF_QA2
	bad_mbuf=m;	/* Make it easier for folks to find this addr */
#endif	/* MBUF_QA2*/

	if (m->m_type == MT_FREE)
		panic("m_free: freeing free mbuf");

#ifndef	VAGUE_STATS
	ms = splimp();
#endif	/* !VAGUE_STATS */
	if (M_HASCL(m)) {
#ifdef VAGUE_STATS
		ms = splimp();
#endif	/* VAGUE_STATS */
		switch (m->m_cltype) {

		case MCL_NORMAL:
			n = m->m_head;
			if (--(n->m_refcnt) == 0) {
				MCLFREE(MTOCL(n)); 
				mbstat.m_clusters--;
				mbstat.m_clbytes -= n->m_clsize;
				if (n != m) {
#ifdef	MBUF_QA2
			    		(void) mbuf_free_qa2(n);
#else
					/* free head mbuf (no longer needed) */
					n->m_type = MT_FREE;
					MBUF_FREE(n);
#endif	/* MBUF_QA2 */
				}
			}
			else /* refcnt != 0 */ {
				if (n == m) {
					/* mark head mbuf as free, but
					 * retain until last ref is freed
					 */
					mbstat.m_mtypes[m->m_type]--;
					m->m_type = MT_FREE;
#if 0
/*
 *	Somebody doesn't like it when I do this so until we can
 *	fix that problem, don't do this.
 */
					m->m_off  = MMINOFF;
#endif
					n = m->m_next;
					m->m_next = 0;
					splx(ms);
					return (n);
				}
			}
			break ;

		case MCL_STREAMS:
			n = m->m_head;
			if (--(n->m_refcnt) == 0) {
				int (*fun)() = n->m_osx_clfun;
				if (fun)
					(*fun)(n->m_osx_clarg);
				if (n != m) {
					/* free head mbuf (no longer needed) */
					n->m_type = MT_FREE;
					MBUF_FREE(n);
				}
		 	} else {
				if (n == m) {
					/* mark head mbuf as free, but
					 * retain until last ref is freed
					 */
					mbstat.m_mtypes[m->m_type]--;
					m->m_type = MT_FREE;
					m->m_off = MMINOFF;
					n = m->m_next;
					m->m_next = 0;
					splx(ms);
					return (n);
				}
			}
			break ;

		case MCL_EXT:
			n = m->m_ext_head;
			if (--(n->m_ext_refcnt) == 0) {
				void (*fun)() = n->m_ext_clfun;
				if (fun) {
				   (*fun)(n->m_ext_clarg);
				}
				if (n != m) {
				   /* free head mbuf (no longer needed) */
				   n->m_type = MT_FREE;
				   MBUF_FREE(n);
				}
		 	}
			else if (n == m) {
				/* mark head mbuf as free, but
				 * retain until last ref is freed
				 */
				mbstat.m_mtypes[m->m_type]--;
				m->m_type = MT_FREE;
#if 0
				/* precaution:  someone doesn't like this
				 * for MCL_NORMAL, so we won't do it here.
				 */
				m->m_off = MMINOFF;
#endif
				n = m->m_next;
				m->m_next = 0;
				splx(ms);
				return (n);
			}
			break ;

		case MCL_NFS:
			n = m->m_nfs_head;
			if (--(n->m_nfs_refcnt) == 0) {
				(*n->m_clfun)(n->m_clarg);
				if (n != m) {
					/* free head mbuf (no longer needed) */
					n->m_type = MT_FREE;
					MBUF_FREE(n);
				}
			} else {
				if (n == m) {
					/* mark head mbuf as free, but
					 * retain until last ref is freed
					 */
					mbstat.m_mtypes[m->m_type]--;
					m->m_type = MT_FREE;
					m->m_off  = MMINOFF;
					n = m->m_next;
					m->m_next = 0;
					splx(ms);
					return (n);
				}
			}
			break;

#ifdef  __hp9000s800
		case MCL_DUX:
			(*m->m_dux_clfun)(m->m_dux_clarg);
			break;

		case MCL_OPS:
			n = m->m_head;
			if (--(n->m_refcnt) == 0) {
				MCLFREEBUF(n,0);
				mbstat.m_clusters--;
				mbstat.m_clbytes -= n->m_clsize;
				if (n != m) {
#ifdef	MBUF_QA2
			    		(void) mbuf_free_qa2(n);
#else
					/* free head mbuf (no longer needed) */
					n->m_type = MT_FREE;
					MBUF_FREE(n);
#endif	/* MBUF_QA2 */
				}
			}
			else /* refcnt != 0 */ {
				if (n == m) {
					/* mark head mbuf as free, but
					 * retain until last ref is freed
					 */
					mbstat.m_mtypes[m->m_type]--;
					m->m_type = MT_FREE;
#if 0
/*
 *	Somebody doesn't like it when I do this so until we can
 *	fix that problem, don't do this.
 */
  					m->m_off  = MMINOFF;
#endif
					n = m->m_next;
					m->m_next = 0;
					splx(ms);
					return (n);
				}
			}
			break ;
#endif  /* hp9000s800 */

		default:
			panic("mclfree");
		}

#ifdef VAGUE_STATS
	splx(ms);
#endif /* VAGUE_STATS */
	}

	mbstat.m_mtypes[(m)->m_type]--;

#ifndef VAGUE_STATS
	splx(ms);
#endif /* !VAGUE_STATS */

	(n) = (m)->m_next;
	(m)->m_next = 0;

#ifdef	MBUF_QA2
	(void) mbuf_free_qa2(m);
#else
	(m)->m_type = MT_FREE;
	MBUF_FREE(m);
#endif	/* MBUF_QA2 */

	return(n);
}


#ifdef MBUF_QA2
void
mbuf_free_qa2(m)
struct mbuf *m;
{
    register struct mbuf *m0;
    int	ms;
    int i, j, freeq_size;
    uint *trace;		/* ptr to values returned by stktrc() */
    struct mbuf *m_free();
    void m_freem();
#ifdef __hp9000s800 
    uint *stktrc();
#endif /* __hp9000s800  */

    if (!m) panic("mbuf_qa2");
    bad_mbuf=m;
    for (i = 0; i < MBUF_FREEQ_SIZE ; i++) {
	if (mbuf_freeq[i] == m) {
	    printf("Found %x in mbuf_freeq at slot %d address %x\n",
		m, i, &mbuf_freeq[i]);
	    if (check_for_dup_mbuf_flag)
		panic("mbuf in free q");
	} else
	    if (mbuf_freeq[i] == (struct mbuf *) 0) break;
    }
    if (check_for_dup_mbuf_flag) {
	ms = splimp();
#ifdef __hp9000s800 
 	trace = stktrc(0, 0);	/* trace the stack */
#endif /* __hp9000s800  */
 	for (i = 0; i < MBUF_FREEQ_SIZE ; i++) {
 	    if (mbuf_freeq[i] == (struct mbuf *) 0) {
 		mbuf_freeq[i] = m;
#ifdef __hp9000s800 
		if (trace) {
		    if (check_for_dup_mbuf_flag & 2) 
			bcopy(&trace[3], &m->m_dat, MLEN);
		    mbuf_freestk[i][0] = (int) trace[4];
		    mbuf_freestk[i][1] = (int) trace[5];
		} else mbuf_freestk[i][0] = 0;
#endif /* __hp9000s800  */
 		splx(ms);
 		return;
 	    } else if (mbuf_freeq[i] == m) {
		printf ("m_free: mbuf %x in slot %d at address %x\n",
		    m, i, m);
		panic("m_free: freeing free mbuf(#2)"); 
	    }
 	}

	m0 = mbuf_freeq[0];
	pack_freeq(m);
#ifdef __hp9000s800 
	if (trace) {
	    if (check_for_dup_mbuf_flag & 2) 
		bcopy(&trace[3], &m->m_dat, MLEN);
	    mbuf_freestk[MBUF_FREEQ_SIZE][0] = (int) trace[4];
	    mbuf_freestk[MBUF_FREEQ_SIZE][1] = (int) trace[5];
	} else
	    mbuf_freestk[i][0] = 0;
#endif /* __hp9000s800  */
	for (i = 0; i < MBUF_FREEQ_SIZE ; i++) {
	    if (mbuf_freeq[i] == m0) {
		printf("m_free: logic error i %d m %x\n",i, m0);
		panic("m_free:logic error");
	    }
	}
	splx(ms);
	MBUF_FREE(m0);
    } else 
	MBUF_FREE(m);

    return;
}
#endif /* MBUF_QA2 */

m_freem(m)
register struct mbuf *m;
{
	register struct mbuf *n;

	if (m == NULL)
		return;
	do {
		n = m_free(m);
	} while (m = n);
}

m_freem_train(m)
        struct mbuf *m;
{
        struct mbuf *next_pkt;

	while (m) {
                next_pkt = m->m_act;
                m->m_act = 0;
		while (m = m_free(m));
                m = next_pkt;
        }
}


/*
 * Mbuffer utility routines.
 */

/*
 * mbufcopy_nfs - copies a single mbuf, standard cluster (<=MCLBYTES),
 * or non standard cluster (> MCLBYTES) to one or more mbufs possibly
 * with clusters of standard size (== MCLBYTES).
 * RETURNS:  a ptr to first of possibly several mbufs allocated.  (0 is
 *           returned when no mbufs were allocated (that, is space not
 *           available) or there is no data to be copied in the mbuf
 *           (that is, m->m_len == 0).
 */
struct mbuf *
mbufcopy_nfs(m)
register struct mbuf *m;
{
	struct mbuf *n, *last, *first = 0;
	int bytes_left;
	int off = 0;

	if (!m)
	    panic("mbufcopy_nfs: m == 0");
	if (m->m_len < 0)
	    panic("mbufcopy_nfs: m->m_len < 0");

	for (bytes_left = m->m_len;
             bytes_left;
             bytes_left -= n->m_len, off += n->m_len) {
	    MGET(n, M_DONTWAIT, m->m_type);
	    if (n == 0)
		goto nobufs;
	    if (bytes_left == m->m_len)
		first = n;	/* first mbuf allocated */
	    else
	    	last->m_next = n;
	    last = n;
	    if (bytes_left > (MLEN << 1)) {
	        MCLGET(n);
	        if (n->m_len == MLEN)    /* MALLOC failed */
		    goto nobufs;
	    } else
		n->m_len = MLEN;
	    n->m_len = MIN(bytes_left, n->m_len);
	    bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t), (unsigned)n->m_len);
	}
	return (first);
nobufs:
	m_freem_train(first);
	return ((struct mbuf *) 0);
}

struct mbuf *
m_copy_nfs_loopback(m)
struct mbuf *m;
{
	struct mbuf *m0, *n, *last;
	struct mbuf *retn = 0;  
        struct mbuf *head = 0; 
	int npckts;

	if (m == 0)
	   panic("m_copy_nfs_loopback");

        for (; m; m = m0->m_act) {    /* copy each chain in train */ 
          npckts = 0;
	  do {        /* copy a chain including clusters */
              if (!(n = mbufcopy_nfs(m))) {
		  m_freem_train(retn);
		  return((struct mbuf *) 0);
              }
              if (npckts++ == 0) {
		  if (head)
		      head->m_act = n;  /* link prior chain to new        */
                  last = head = n;      /* this is the head of each chain */
		  m0 = m;               /* retrieve the m_act field later */
		  if (!retn)
		      retn = n;         /* save start of new train */
              } else
                  last->m_next = n;
              while (last->m_next)      /* keep track of last mbuf in chain */
		  last = last->m_next;
              m = m->m_next;
	  } while (m);
        } /* for */
	retn->m_flags = m0->m_flags;                          /* copy flags and */
	retn->m_quad[MQ_CKO_OUT0] = m0->m_quad[MQ_CKO_OUT0];  /* checksum offload */
	retn->m_quad[MQ_CKO_OUT1] = m0->m_quad[MQ_CKO_OUT1];  /* for nfs */
        return (retn);
}

#ifdef	__hp9000s800
/*
 * Make a copy of an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * Should get M_WAIT/M_DONTWAIT from caller.
 */
int
mclcopy_dux(m, off, len, n)
register struct mbuf *m, *n;
int      off;
register int len;
{
	/* HP(8.05)
	 * copy dux cluster data into normal cluster.
	 *
	 * NB: copy is no longer exactly like original.
	 * alternative might be to copy mun_cldux; but
	 * this is probably wrong (!) since it might
	 * cause repercussions when cluster is freed.
	 * e.g., we probalby don't want to call
	 * mun_dux_clfun with mun_{sid,off}.
	 *
	 * CAVEAT:
	 * we panic if DUX cluster too large because
	 * otherwise data could never be transmitted
	 * and system might hang.  this is okay for
	 * now because we are called only from LAN
	 * driver, at which point DUX cluster <= 1526.
	 * 
	 * in future, might prefer to MALLOC(m_len) or
	 * page-size multiple, or do multiple MCLGETs
	 */

	MCLGET(n);
	if (n->m_len == MLEN)    /* MALLOC failed */
		return (1); 
	n->m_len = MIN(len, m->m_len - off);
	if ((unsigned) n->m_len > MCLBYTES)
		panic("mclcopy_dux: cluster too large");

	if (m->m_sid) { /* mbuf data not in kernel space */
		lbcopy(m->m_sid, (caddr_t)m + m->m_sid_off + off,
		  KERNELSPACE, mtod(n, caddr_t), (unsigned)n->m_len);
	}
	else { /* mbuf data in kernel space */
		bcopy(mtod(m, caddr_t) + off,
		  mtod(n, caddr_t), (unsigned)n->m_len);
	}
	return (0);
}
#endif  /* hp9000s800 */


/*
 * Make a copy of an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * Should get M_WAIT/M_DONTWAIT from caller.
 */
struct mbuf *
m_copy(m, off, len)
register struct mbuf *m;
int	off;
register int	len;
{
	register struct mbuf *n, **np;
	struct mbuf *top, *p;
	int s;

	if (len == 0)
		return (0);
	if (len < 0)
		panic("m_copy 0: len < 0");
	if (off < 0)
		panic("m_copy 1: off < 0");

	while (off > 0) {
		if (m == 0)
			panic("m_copy 2: off too big");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}

	np = &top;
	top = 0;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL)
				panic("m_copy 3: off too big");
			break;
		}

		MGET(n, M_DONTWAIT, m->m_type);
		*np = n;
		if (n == 0)
			goto nobufs;
		n->m_len = MIN(len, m->m_len - off);

		if (m->m_off > MMAXOFF) {
		    	n->m_clsize = m->m_clsize;
		    	if ((n->m_cltype = m->m_cltype) != MCL_DUX) {
				n->m_off = ((int)mtod(m, struct mbuf*) - (int)n)
				           + off;
				n->m_head = m->m_head;
				s = splimp();
				m->m_head->m_refcnt++;
				splx(s);
			}
			else {  /* MCL_DUX */
#ifdef	__hp9000s800
				if (mclcopy_dux(m, off, len, n))
					goto nobufs;
#else
				panic("m_copy 4: dux");
#endif	/* hp9000s800 */
			}
		} 
		else {
			bcopy(mtod(m, caddr_t) + off, mtod(n, caddr_t),
			      (unsigned)n->m_len);
		}

		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	return (top);

nobufs:
        m_freem(top);
       	return (0);
}


/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
m_copydata(m, off, len, cp)
register struct mbuf *m;
register int	off;
register int	len;
caddr_t cp;
{
	register unsigned	count;

	if (off < 0 || len < 0)
		panic("m_copydata 1");
	while (off > 0) {
		if (m == 0)
			panic("m_copydata 2");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		if (m == 0)
			panic("m_copydata 3");
		count = MIN(m->m_len - off, len);

#ifdef __hp9000s800	
		/* need to use special routine if it is a COW buffer */	
		if (M_HASCL(m) && (m->m_cltype == MCL_OPS)) 
			MCLCOPYFROM (m,off,count,cp);
		else 
#endif
			bcopy(mtod(m, caddr_t) + off, cp, count);
		
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}
}

/* 
 * For Netipc m_copy needs to be able to step past message boundaries.
 * This routine does that and then calls m_copy
 */
struct mbuf *
nipc_m_copy(m, off, len)
struct mbuf	*m;
int		off;
int		len;
{
	struct mbuf *front = m;

	if (len == 0)
		return (0);
	if (off < 0 || len < 0)
		panic("nipc_m_copy 1");
	while (off > 0) {
		if (m == 0)
			panic("nipc_m_copy 2");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		if (m->m_next)
			m = m->m_next;
		else
			m = front = front->m_act;
	}
	return(m_copy(m, off, len));
}
/* 
 * For Netipc m_copydata needs to be able to step past message boundaries.
 * This routine does that and then calls m_copy
 */
nipc_m_copydata(m, off, len, cp)
register struct mbuf *m;
register int	off;
register int	len;
caddr_t cp;
{
	struct mbuf *front=m;

	if (off < 0 || len < 0)
		panic("nipc_m_copydata 1");
	while (off > 0) {
		if (m == 0)
			panic("nipc_m_copydata 2");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		if (m->m_next)
			m = m->m_next;
		else
			m = front = front->m_act;
	}
	m_copydata(m, off, len, cp);
}

/*  m_frag 
 *
 *  Cut an mbuf chain into two chains at a flexible boundary.
 *
 *  Parameters:
 *          m - on input m references a chain of mbufs of any positive
 *              length; if m is 0 on input, m_frag returns 0, which should
 *		not be construed as an error.  on output, m references an 
 *		mbuf chain no greater than the initial value of *residp 
 *		bytes long.
 *     residp - on input residp references the maximum length (in bytes) of 
 *		the mbuf chain referenced by m on output; this initial value 
 *		may be greater than the length of the chain referenced by m
 *		on input; and this initial value *must* equal 0 when anded 
 *		with mask.  on output, residp references the residual number
 *		of bytes not referenced by m on output; ie, the initial
 *		value of *residp less the length of the chain referenced by
 *		m on output; the output value of *residp anded with mask 
 *		will equal 0 if m_frag returns a pointer to an mbuf chain.
 *   maxresid - the maximum output value of *residp if m_frag returns a 
 *		pointer to an mbuf chain; maxresid must be less than the
 *		initial value of *residp - ie, the chain referenced by m on
 *		on output *must* reference at least 1 byte.  maxresid anded
 *		with mask *must* equal 0.  note, the output value of *residp 
 *		may exceed maxresid if m_frag returns 0.
 *       mask - some value 2^N-1, where N is a non-negative integer; eg, 0,
 *		1, 3, 7 and so on.  if m_frag returns a pointer to an mbuf
 *		chain, the output value of *residp when logically anded with
 *		mask must equal 0.
 *
 *  Returns:    if no error, m_frag returns a pointer to the mbuf following
 *              the chain referenced by m; m_frag may return 0, which should
 *		not be construed to be an error.  if an error occurs, m_frag
 *		returns -1; the caller's test for error should be for -1 and 
 *		not for any negative value, because some negative values are 
 *		valid mbuf addresses; -1 cannot be a valid mbuf pointer 
 *		because it is a byte aligned address.
 */
struct mbuf *
m_frag(m, residp, maxresid, mask)
struct mbuf *m;
int *residp;
int maxresid;
int mask;
{
	struct mbuf *p;
	int resid;
	int s;

	p = 0;
	if ((resid = *residp) == 0)
		goto done;

	/*     The following checks are required for correct operation;
	 *  do not eliminate!
	 */
	if (resid < 0)
		panic("m_frag 0: resid < 0");
	if (maxresid < 0)
		panic("m_frag 1: maxresid < 0");
	if (maxresid >= resid)
		panic("m_frag 2: maxresid >= resid");

	/*     Run through the mbufs until the end of the chain is reached
	 *  or until an mbuf is found that meets or exceeds the maximum 
	 *  fragment length (ie, initial value of *residp).
	 */
	while (m) {
		if (resid < m->m_len)
			goto last;
		resid -= m->m_len;
		p = m;
		m = m->m_next;
	}
	goto done;

	/*     Attempt to fragment the chain at an mbuf boundary.  This is
	 *  possible if the maximum fragment length miraculously ends at an
	 *  mbuf boundary, or if the fragment length upto the current mbuf
	 *  is greater than the minimum fragment length and the fragment 
	 *  length is a multiple of mask+1 (where mask == (2^N)-1 for some 
	 *  non-negative integer N).
	 *     Note, ((resid <= maxresid) && (p == 0)) always == FALSE
	 *  because maxresid must be < initial value of *residp.
	 *  the (p != 0) test is a paranoia check and could be eliminated.
	 */
last:
     	if (resid == 0)
		goto exact; 
	if ((resid <= maxresid) && ((resid & mask) == 0) && p)
		goto sever;

	/*     Create copy of current mbuf: let the copy contain or reference 
	 *  the data in the current mbuf following the maximum fragment length.
	 *  The copy will be the first mbuf in the returned residual chain.
	 */
	p = m;
	MGET(m, M_DONTWAIT, p->m_type);
	if (m == 0)
		goto nobufs;
	m->m_len = p->m_len - resid;
	p->m_len = resid;
	if (M_HASCL(p)) {
		m->m_clsize = p->m_clsize;
		if ((m->m_cltype = p->m_cltype) != MCL_DUX) {
			m->m_off = mtod(p,caddr_t) - (caddr_t)m + resid;
			m->m_head = p->m_head;
			s = splimp();
			p->m_head->m_refcnt++;
			splx(s);
		}
		else { /* MCL_DUX */
#ifdef	__hp9000s800
			if (mclcopy_dux(p, resid, m->m_len, m))
				goto duxerr;
#else
			panic("m_frag 3: dux");
#endif	/* hp9000s800 */
		}
	} 
	else {
		bcopy(mtod(p,caddr_t) + resid, mtod(m,caddr_t), m->m_len);
	}
	m->m_next = p->m_next;
	
	/*     Okay Exit code: return address of mbuf following the fragment
	 *  referenced by m.
	 */
exact:
      	resid = 0;
sever:
      	p->m_next = 0;
done:
     	*residp = resid;
	return (m);

	/*     Error Exit code: return -1; caller must test for -1, not < 0.
	 */
duxerr:
        m_free(m);
nobufs:
       	return ((struct mbuf *)-1);
}

m_cat(m, n)
register struct mbuf *m, *n;
{
	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_off >= MMAXOFF || 
		    m->m_off + m->m_len + n->m_len > MMAXOFF) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}


m_adj(mp, len)
struct mbuf *mp;
register int	len;
{
	register struct mbuf *m;
	register count;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_off += len;
				break;
			}
		}
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (; ; ) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			return;
		}
		count -= len;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		for (m = mp; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		while (m = m->m_next)
			m->m_len = 0;
	}
}


/*
 * Rearange an mbuf chain so that len bytes are contiguous
 * and in the data area of an mbuf (so that mtod and dtom
 * will work for a structure of size len).  Returns the resulting
 * mbuf chain on success, frees it and returns null on failure.
 * If there is room, it will add up to MPULL_EXTRA bytes to the
 * contiguous region in an attempt to avoid being called next time.
 */
struct mbuf *
m_pullup(n, len)
register struct mbuf *n;
int	len;
{
	register struct mbuf *m;
	register int	count;
	int	space;

	if (len > MLEN)
		goto bad;
	if (M_HASCL(n)) {
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
	} else {
		if (M_UNALIGNED(n) || n->m_off + len > MMAXOFF) {
			bcopy(mtod(n, caddr_t), (caddr_t)((int)(n)+MMINOFF),
			    n->m_len);
			n->m_off = MMINOFF ;
		}
		m = n;
		if (!(n = n->m_next)) return(m);
		if ((len -= m->m_len) <= 0) return(m);
	}
	space = MMAXOFF - m->m_off;
	do {
		count = MIN(MIN(space - m->m_len, len + MPULL_EXTRA), n->m_len);
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		if (n->m_len)
			n->m_off += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	return (0);
}


m_len(m)
struct mbuf *m;
{
	int len;
	for (len = 0; m; m = m->m_next)
		len += m->m_len;
	return(len);
}


/* 
 * Allocate a LARGEBUF, that i one whose data is owned by someone else
 */
struct mbuf *
mclget_osx(fun, arg, addr, len)
int	(*fun)(), arg, len;
caddr_t addr;
{
	register struct mbuf *m;

	/* 
	 * Use the NO_WAIT flag since the STREAMS
	 * scheduler can never sleep
	 */
	MGET(m, M_NOWAIT, MT_DATA);
	if (m == 0)
		return (0);
	m->m_off = (int)addr - (int)m;
	m->m_len = len;
	m->m_cltype = MCL_STREAMS;
	m->m_clsize = len;
	m->m_head = m;
	m->m_refcnt = 1;
	m->m_osx_clfun = fun;
	m->m_osx_clarg = arg;
	return (m);
}
/*
 * From here to the end of the file is QA only code!
 */

#ifdef	MBUF_QA

struct mbuf	*mbuf_head, *mbuf_tail, *cl_head, *cl_tail;
int		mbuf_qa, m_fail;
int		m_seed = -1;
struct free_q	free_queue;


struct mbuf *
get_mbuf_l(head, tail, canwait)
struct mbuf **head, **tail;
{
	struct mbuf *m;
	if (*head) {
		m = *head;
		*head = m->m_next;
		if (!*head)
			*tail = 0;
	} else {
		MALLOC(m, struct mbuf *, MSIZE, M_MBUF, canwait);
	}
	return(m);
}

struct mbuf *
get_cl_l(head, tail)
struct mbuf **head, **tail;
{
	struct mbuf *m;
	if (*head) {
		m = *head;
		*head = m->m_next;
		if (!*head)
			*tail = 0;
	} else {
		MALLOC(m, struct mbuf *, MCLBYTES, M_MBUF, M_DONTWAIT);
		if (m) {
			mbstat.m_clusters++;
			mbstat.m_clbytes += MCLBYTES;
		}
	}
	return(m);
}

struct mbuf *
alloc_npg(canwait)
int canwait;
{
	u_int npg = btorp(MCLBYTES);
	if (canwait == M_WAIT)
		return((struct mbuf*) kdalloc(npg));
	else 
		return((struct mbuf*) kdalloc_nowait(npg));
}
			
struct mbuf *
get_clmbuf_q(q, canwait)
struct free_q *q;
int canwait;
{
	struct mbuf *m;
	if (q->len == 0) {
		m = alloc_npg(canwait);
		if (m) {
			mbstat.m_clusters++;
			mbstat.m_clbytes += MCLBYTES;
			return(m);
		} else 
			return(NULL);
	}
	if (mbuf_qa & MBQA_INVAL)
		hdl_addtrans(KERNPREG, KERNELSPACE, q->v_addr[q->head],
		    0, q->pfn[q->head]); 
	q->len--;
	m = (struct mbuf *) q->v_addr[q->head];
	INC_Q(q->head);
	return(m);
}

random_loss(canwait) 
int canwait;
{
	if (mbuf_qa & MBQA_RLOSS)
		if (canwait == M_DONTWAIT) {
			m_fail = GEN_RAN(m_seed, RND_MODULO);
			if (m_fail < M_FAIL_THRESH)
				return(1);
		}
	return(0);
}

struct mbuf *
m_get_qa(canwait, type) 
int canwait, type;
{
	struct mbuf *m;
	int ms = splimp();
	if (type <= MT_FREE || type >= MT_TOTAL)
		panic("MGET called with bad type "); 
	if (random_loss(canwait)) {
		mbstat.m_drops++;
		splx(ms);
		return (NULL);
	}
	if (mbuf_qa & MBQA_EXHAUST)
		m = get_clmbuf_q(&free_queue, canwait);
	else
		m = get_mbuf_l(&mbuf_head, &mbuf_tail, canwait);
	if (m) {
		mbstat.m_mtypes[type]++;
		m->m_type = type;
		m->m_act  = 0;
		m->m_next = 0;
		m->m_flags = 0;
		m->m_off  = MMINOFF;
	} 
	splx(ms);
	return(m);
}

mclget_qa(m)
struct mbuf *m;
{
	struct mbuf *p;
	int ms = splimp();
	if (random_loss(M_DONTWAIT)) {
		mbstat.m_drops++;
		m->m_len = MLEN;
		splx(ms);
		return;
	}			
	if (mbuf_qa & MBQA_EXHAUST)
		p = get_clmbuf_q(&free_queue, M_DONTWAIT);
	else
		p = get_cl_l(&cl_head, &cl_tail);
	if (p) {
		m->m_off = (int)p - (int)m;
		m->m_len = MCLBYTES;
		m->m_head = m;
		m->m_refcnt = 1;
		m->m_clsize = MCLBYTES;
		m->m_cltype = MCL_NORMAL;
	} else {
		mbstat.m_drops++;
		m->m_len = MLEN;
	}			
	splx(ms);
}

mclfree_qa(m)
struct mbuf *m;
{
	int ms = splimp();
	if (mbuf_qa & MBQA_EXHAUST)
		free_clmbuf_q(&free_queue, m);
	else {
		if (cl_tail)
			cl_tail->m_next = m;
		else
			cl_head = m;
		cl_tail = m;
	}
	m->m_next = 0;
	splx(ms);
}

mbuf_free_qa(m)
struct mbuf *m;
{
	int ms = splimp();
	if (mbuf_qa & MBQA_EXHAUST)
		free_clmbuf_q(&free_queue, m);
	else {
		if (mbuf_tail)
			mbuf_tail->m_next = m;
		else
			mbuf_head = m;
		mbuf_tail = m;
	}
	m->m_next = 0;
	splx(ms);
}

extern caddr_t hdl_vpfn();
free_clmbuf_q(q, m) 
struct free_q *q;
struct mbuf *m;
{
	if (q->len == FQ_LEN) {	
		if (mbuf_qa & MBQA_INVAL)
			hdl_addtrans(KERNPREG, KERNELSPACE, q->v_addr[q->head],
			    0, q->pfn[q->head]);
		kdfree(q->v_addr[q->head], btorp(MCLBYTES));
		INC_Q(q->head);
		q->len--;
		mbstat.m_clusters--;
		mbstat.m_clbytes -= MCLBYTES;
	}
	q->v_addr[q->tail] = (caddr_t) m;	
	if (mbuf_qa & MBQA_INVAL) {
		q->pfn[q->tail] = hdl_vpfn(KERNELSPACE, m);
		hdl_unvirtualize(q->pfn[q->tail]);
	}
	INC_Q(q->tail);
	q->len++;
}
#endif
