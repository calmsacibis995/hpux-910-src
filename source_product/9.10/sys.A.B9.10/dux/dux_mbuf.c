/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_mbuf.c,v $
 * $Revision: 1.6.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:41:42 $
 */

/* HPUX_ID: @(#)dux_mbuf.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/
#include 	"../h/mbuf.h"
#include 	"../h/malloc.h"
#include 	"../h/param.h"
#include 	"../dux/dm.h"

#define ETHERMTU	1500

#define 	MBUF_CHAIN 	0
#define 	CBUF_CHAIN 	1


struct mbuf *
m_clget(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;
	int ms;

	m = m_get(canwait, type);
	if (m == 0)
		return (m);

	if (canwait == M_DONTWAIT) {
		MCLGETVAR_DONTWAIT(m,ETHERMTU);
	} else {
		MCLGETVAR_WAIT(m,ETHERMTU);
	}
	if (m->m_len == MLEN) {
		(void) m_free(m);
		return(NULL);
	}

	ms = splimp();
	mbstat.m_clusters++;
	splx(ms);

	return (m);
}

struct mbuf *
dux_mem_get( canwait, type, numbytes )
register int canwait, type, numbytes;
{
register struct mbuf *m;
register int totalbytes, s;

/* remove the following two lines when we fix dm.h WAIT DONTWAIT flags 
   to be the same as M_WAITOK and M_NOWAIT */
	if (canwait == WAIT) canwait = M_WAITOK;
	else canwait = M_NOWAIT;

#ifdef __hp9000s300
        s = CRIT5();
#else
	s = spl6();
#endif
	/*
	** Must add in the size of the header for this request.
	*/
	totalbytes = numbytes + DM_HEADERLENGTH;
	if( totalbytes > DM_MAX_MESSAGE )
	{
	  panic("dskless_mem_get: Request greater than max. allowable message");
	}
	if (totalbytes <= MLEN) {
        	m = m_get(canwait, type);
		if (m != 0)
		init_sngl_ieee_hdr(m, MBUF_CHAIN);
	} else {
		m = m_clget(canwait, type);
	 	if (m != 0)
		init_sngl_ieee_hdr(mtod(m, char *), CBUF_CHAIN);
	}
	if (m != 0)
	(void) dux_init_mbuf( m, type, totalbytes );
#ifdef __hp9000s300
	UNCRIT(s);
#else
	splx(s);
#endif
	return (m);
}


void
dux_m_free( m )
register struct mbuf *m;
{
	m_freem(m);
}

/*
** This routine fills in the IEEE802.3
** header for the  supplied mbuf pointer. 
*/
init_sngl_ieee_hdr(mbuf, specified_chain)
register dm_message *mbuf;
register int specified_chain;
{
struct dm_header *dmp;
register struct proto_header *ph;

	if( mbuf == NULL )
	{
	   panic("init_sngl_ieee_hdr: NULL buf ptr.");
	}

	switch( specified_chain )
	{
	case MBUF_CHAIN:

		/*
		** In an mbuf the ieee header goes offset
		** into the m_data portion of the mbuf.
		*/
		dmp = (struct dm_header *)((int)(mbuf) + MMINOFF);
		ph = &(dmp->dm_ph);
		break;
		
	case CBUF_CHAIN:

		/*
		** In an cbuf the ieee header goes immediately
		** at the front of the cbuf.
		*/
		dmp = (struct dm_header *)((int)(mbuf));
		ph = &(dmp->dm_ph);
		break;

	default:
		panic("init_sngl_ieee_hdr: invalid type");

	}

	dux_ieee802_header(ph);
}



/*
** Initialize the mbuf and possible associated cluster.
** This routine is only called after the resources have
** been obtained. Simply fills in necessary fields, etc.
*/
dux_init_mbuf( mb, type, totalbytes )
register struct mbuf *mb;
register int type, totalbytes;
{
register struct dm_header *hp;

	/*
	** Consistency check for valid mbuf pointer.
	*/
	if( mb == NULL )
	{
		panic("dskless_init_mbuf: invalid mbuf pointer!");
	}

	mb->m_type = type;
	
	mb->m_len = totalbytes;


	/*
	** There is some other basic bookkeeping and standard 
	** initializations that must take place for every mbuf.
	** And here there they are.
	**
	** The size should include the size of the transmitted header
	** plus the size of the user data.  (This is just the
	** convention established between the DM and the network,
	** not some law of the universe)
	*/
	hp = DM_HEADER(mb);
	hp->dm_headerlen = (totalbytes - DM_HEADERLENGTH)
			 + sizeof(struct dm_transmit);

	hp->dm_flags = hp->dm_tflags = 0;
	hp->dm_bufp = NULL;
	hp->dm_datalen = 0;
	hp->dm_bufoffset = 0;
	hp->dm_mid = -1;	/*invalid mid*/
	hp->dm_srcsite = my_site;

}

