/*
 *  $Header: hello.c,v 1.1.109.6 92/03/04 08:36:12 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


/*
 *	Hello output routines were taken from Mike Petry (petry@trantor.umd.edu)
 *	Also, hello input routines were written by Bill Nesheim, Cornell
 *	CS Dept,  Currently at nesheim@think.com
 */

#include "include.h"
#include "hello.h"
#if	defined(_IBMR2)
#include <time.h>
#endif				/* defined(_IBMR2) */
#include <sys/time.h>

#ifdef	PROTO_HELLO

#define	HELLO_TIMER_UPDATE	0
#define	HELLO_TIMER_FLASH	1

static task *hello_task = (task *) 0;
static time_t hello_next_flash = (time_t) 0;

int hello_pointopoint = FALSE;		/* Are we ONLY doing pointopoint HELLO? */
int hello_supplier = -1;		/* Are we broadcasting HELLO protocols? */
int doing_hello = FALSE;		/* Are we running HELLO protocols? */
metric_t hello_default_metric;	/* Default metric to use when propogating */
pref_t hello_preference;	/* Preference for HELLO routes */

int hello_n_trusted = 0;		/* Number of trusted gateways */
int hello_n_source = 0;			/* Number of source gateways */
gw_entry *hello_gw_list = NULL;		/* List of HELLO gateways */
adv_entry *hello_accept_list = NULL;	/* List of nets to accept from HELLO */
adv_entry *hello_propagate_list = NULL;	/* LIst of sources to propagates routes to HELLO */
adv_entry **hello_int_accept = NULL;	/* List of accept lists per interface */
adv_entry **hello_int_propagate = NULL;	/* List of propagate lists per interface */


metric_t hop_to_hello[] =
{					/* Translate interface metric to hello metric */
    0,					/* 0 */
    100,				/* 1 */
    148,				/* 2 */
    219,				/* 3 */
    325,				/* 4 */
    481,				/* 5 */
    713,				/* 6 */
    1057,				/* 7 */
    1567,				/* 8 */
    2322,				/* 9 */
    3440,				/* 10 */
    5097,				/* 11 */
    7552,				/* 12 */
    11190,				/* 13 */
    16579,				/* 14 */
    24564,				/* 15 */
    30000				/* 16 */
};


/* Routines for support of the hello window system */

/*
 * initialize the sliding HELLO history window.
 */
static void
hello_win_init(hwp, tdelay)
struct hello_win *hwp;
int tdelay;
{
    int msf = 0;

    while (msf < HWINSIZE)
	hwp->h_win[msf++] = DELAY_INFINITY;
    hwp->h_index = 0;
    hwp->h_min = tdelay;
    hwp->h_min_ttl = 0;
    hwp->h_win[0] = tdelay;
}


/*
 * add a HELLO derived time delay to the route entries HELLO window.
 */
static void
hello_win_add(hwp, tdelay)
struct hello_win *hwp;
int tdelay;
{
    int msf, t_index = 0;

    hwp->h_index++;
    if (hwp->h_index >= HWINSIZE)
	hwp->h_index = 0;
    hwp->h_win[hwp->h_index] = tdelay;
    if (tdelay > hwp->h_min)
	hwp->h_min_ttl++;
    else {
	hwp->h_min = tdelay;
	hwp->h_min_ttl = 0;
    }
    if (hwp->h_min_ttl >= HWINSIZE) {
	hwp->h_min = DELAY_INFINITY;
	for (msf = 0; msf < HWINSIZE; msf++)
	    if (hwp->h_win[msf] <= hwp->h_min) {
		hwp->h_min = hwp->h_win[msf];
		t_index = msf;
	    }
	hwp->h_min_ttl = 0;
	if (t_index < hwp->h_index)
	    hwp->h_min_ttl = hwp->h_index - t_index;
	else if (t_index > hwp->h_index)
	    hwp->h_min_ttl = HWINSIZE - (t_index - hwp->h_index);
    }
}


/*
 *	Dump info about a HELLO route
 */
static void
hello_rt_dump(fd, rt)
FILE *fd;
rt_entry *rt;
{
    int cnt, ind;
    struct hello_win *hwp = (struct hello_win *) rt->rt_data->rtd_data;

    (void) fprintf(fd, "\t\t\tMinimum HELLO time delay in last %d updates: %d\n",
		   HWINSIZE,
		   hwp->h_min);
    (void) fprintf(fd, "\tLast %d HELLO time delays:\n\t\t",
		   HELLO_REPORT);
    ind = hwp->h_index;
    for (cnt = HELLO_REPORT; cnt; cnt--) {
	(void) fprintf(fd, "%d ",
		       hwp->h_win[ind]);
	if (++ind >= HWINSIZE) {
	    ind = 0;
	}
    }
    (void) fprintf(fd, "\n");
}

/*
 *	Trace a HELLO packet
 */
/* ARGSUSED */
static void
hello_trace(comment, src, dst, hello, length, nets)
char *comment;
struct sockaddr_in *src, *dst;
char *hello;
int length, nets;
{
    int i;
    const char *cp;
    char *end = hello + length;
    struct hm_hdr hm_hdr;
    struct hellohdr hellohdr;
    struct type0pair type0pair;
    struct type1pair type1pair;
    struct iovec iov;

    tracef("HELLO %s %A -> %A  %d bytes",
	   comment,
	   src,
	   dst,
	   length);
    if (nets >= 0) {
	tracef("  %d nets", nets);
    }

    /* Calculate the checksum of this packet */
    iov.iov_base = hello;
    iov.iov_len = length;
    if (gd_inet_cksum(&iov, 1, length)) {
	tracef(" *checksum bad*");
    }
    trace(TR_HELLO, 0, NULL);

    PickUp_hellohdr(hello, hellohdr);

    if (trace_flags & TR_UPDATE) {
	switch (hellohdr.h_date & H_DATE_BITS) {
	    case H_DATE_LEAPADD:
		cp = "add_leap_second ";
		break;
	    case H_DATE_LEAPDEL:
		cp = "del_leap_second ";
		break;
	    case H_DATE_UNSYNC:
		cp = "unsync ";
		break;
	    default:
		cp = "";
	}
	hellohdr.h_date &= ~H_DATE_BITS;
	trace(TR_NOSTAMP | TR_HELLO, 0, "%s %s%d/%d/%d %02d:%02d:%02d.%03d GMT  tstp %d",
	      comment,
	      cp,
	      (hellohdr.h_date >> H_DATE_MON_SHIFT) & H_DATE_MON_MASK,
	      (hellohdr.h_date >> H_DATE_DAY_SHIFT) & H_DATE_DAY_MASK,
	      ((hellohdr.h_date >> H_DATE_YEAR_SHIFT) & H_DATE_YEAR_MASK) + H_DATE_YEAR_BASE,
	      hellohdr.h_time / (60 * 60 * 1000),
	      (hellohdr.h_time / (60 * 1000)) % 60,
	      (hellohdr.h_time / (1000)) % 60,
	      hellohdr.h_time % 1000,
	      hellohdr.h_tstp);

	while (hello < end) {
	    PickUp_hm_hdr(hello, hm_hdr);
	    trace(TR_NOSTAMP | TR_HELLO, 0, "%s\ttype %d  count %d", comment, hm_hdr.hm_type, hm_hdr.hm_count);
	    for (i = 0; i < hm_hdr.hm_count; i++) {
		switch (hm_hdr.hm_type) {
		    case 0:
			PickUp_type0pair(hello, type0pair);
			trace(TR_NOSTAMP | TR_HELLO, 0, "%s\t\tdelay %d  offset %d",
			      comment,
			      type0pair.d0_delay,
			      type0pair.d0_offset);
			break;
		    case 1:
			PickUp_type1pair(hello, type1pair);
			trace(TR_NOSTAMP | TR_HELLO, 0, "%s\t\t%-15s  delay %5d  offset %d",
			      comment,
			      inet_ntoa(type1pair.d1_dst),
			      type1pair.d1_delay,
			      type1pair.d1_offset);
			break;
		    default:
			trace(TR_NOSTAMP | TR_HELLO, 0, "%s\t\tInvalid type - giving up!", comment);
			return;
		}
	    }
	}
	trace(TR_HELLO, 0, NULL);
    }
}


/*
 * hello_send():
 * 	Fill in the hello header and checksum, then send the packet.
 */
static void
hello_send(tp, dst, src, flags, iov, length, n_nets)
task *tp;
struct sockaddr_in *dst, *src;
flag_t flags;
struct iovec *iov;
int length;				/* length in octets of hello packet */
int n_nets;				/* number of nets in this update for debugging */
{
    char *hello = iov->iov_base;
    struct hellohdr hellohdr;
    struct tm *gmt;
    struct timeval timep;
    int error = FALSE;

    (void) gettimeofday(&timep, (struct timezone *) 0);
    gmt = (struct tm *) gmtime(&timep.tv_sec);

    /*
     * set the date field in the HELLO header.  Be very careful here as
     * the last two bits (14&15) should be set so the Fuzzware doesn't use
     * this packet to synchronize its Master Clock.  Using bitwise OR's
     * instead of addition just to be safe when dealing with h_date which
     * is an unsigned short.
     */
    hellohdr.h_date = ((gmt->tm_year - H_DATE_YEAR_BASE) & H_DATE_YEAR_MASK) |
		 ((gmt->tm_mday & H_DATE_DAY_MASK) << H_DATE_DAY_SHIFT) |
	    (((gmt->tm_mon + 1) & H_DATE_MON_MASK) << H_DATE_MON_SHIFT) |
				       H_DATE_UNSYNC;
    /*
     * milliseconds since midnight UT of current day
     */
    hellohdr.h_time = (gmt->tm_sec + gmt->tm_min * 60 + gmt->tm_hour * 3600)
	* 1000 + timep.tv_usec / 1000;
    /*
     * 16 bit field used in rt calculation,  0 for ethernets
     */
    hellohdr.h_tstp = 0;
    hellohdr.h_cksum = 0;
    PutDown_hellohdr(hello, hellohdr);
    hellohdr.h_cksum = gd_inet_cksum(iov, 1, length);
    hello = iov->iov_base;
    PutDown(hello, hellohdr.h_cksum);
    if (task_send_packet(tp,
			 iov->iov_base,
			 length,
			 flags,
			 (sockaddr_un *) dst) < 0) {
	error = TRUE;
    }
    TRACE_HELLOPKT(error ? "*NOT* SENT" : "SENT",
		   src,
		   dst,
		   iov->iov_base,
		   length,
		   n_nets);
    return;
}


/*
 *	Process an incomming HELLO packet
 */

static void
hello_recv(tp)
task *tp;
{
    int count;
    int i;
    flag_t rte_table;
    char *hello, *end;
    struct hm_hdr hm_hdr;
    struct type1pair type1pair;
    rt_entry *rt;
    if_entry *ifp, *ifpc;
    gw_entry *gwp;
    pref_t preference;
    struct sockaddr_in src;
    struct sockaddr_in dst;

    if (task_receive_packet(tp, &count)) {
	return;
    }
    hello = (char *) recv_iovec[RECV_IOVEC_DATA].iov_base;
    end = hello + recv_ip.ip_len;

    sockclear_in(&src);
    sockclear_in(&dst);
    src.sin_addr = recv_ip.ip_src;	/* struct copy */
    dst.sin_addr = recv_ip.ip_dst;	/* struct copy */
    TRACE_HELLOPKT("RECV", &src, &dst, hello, recv_ip.ip_len, -1);

    /* Locate or create a gateway structure for this gateway */
    gwp = gw_timestamp(&hello_gw_list, hello_task->task_rtproto, (sockaddr_un *) & recv_addr);

    /* If we have a list of trusted gateways, verify that this gateway is trusted */
    if (hello_n_trusted && !(gwp->gw_flags & GWF_TRUSTED)) {
	return;
    }
    /* Do we share a net with the sender? */
    if ((ifp = if_withdst((sockaddr_un *) & recv_addr)) == NULL) {
	trace(TR_EXT, LOG_WARNING, "hello_recv: gw %A no shared net?",
	      &recv_addr);
	return;
    }
    if (ifp->int_state & IFS_NOHELLOIN) {
	return;
    }
    /*
     * Are we talking to ourselves?
     *
     * if_withaddr() handles PTP links also.  If packet came
     * from other end of a PTP, let it fall through for further
     * processing.  We shouldn't ever hear our own HELLOs on a PTP link.
     */
    ifpc = if_withaddr((sockaddr_un *) & recv_addr);
    if (ifpc) {
	if_rtupdate(ifpc);
	if ((ifpc->int_state & IFS_POINTOPOINT) == 0) {
	    return;
	}
    }
    /*
     * update the interface timer on interface the packet came in on.
     */
    if_rtupdate(ifp);

    gwp = gw_timestamp(&hello_gw_list, hello_task->task_rtproto, (sockaddr_un *) & recv_addr);
    /* check the hello checksum */

    if (gd_inet_cksum(&recv_iovec[RECV_IOVEC_DATA], 1, recv_ip.ip_len) != (u_short) 0) {
	trace(TR_HELLO, LOG_WARNING, "hello_recv: bad HELLO checksum from %A",
	      &recv_addr);
	return;
    }

    /* message is made up of one or more sub messages */
    rt_open(tp);

    hello += Size_hellohdr;
    while (hello < end) {
	PickUp_hm_hdr(hello, hm_hdr);
	switch (hm_hdr.hm_type) {
	    case 0:
		hello += Size_type0pair * hm_hdr.hm_count;
		/* not interested in type 0 messages */
		break;
	    case 1:
		for (i = 0; i < hm_hdr.hm_count; i++) {
		    preference = hello_preference;
		    PickUp_type1pair(hello, type1pair);
		    sockclear_in(&dst);
		    dst.sin_addr = type1pair.d1_dst;

		    /*  is this new net acceptable? */
		    if (!is_valid_in((sockaddr_un *) & dst,
				     hello_accept_list,
				     INT_CONTROL(hello_int_accept, ifp),
				     gwp->gw_accept,
				     &preference)) {
			continue;
		    }
		    /* Force delay to be valid */
		    if (type1pair.d1_delay > DELAY_INFINITY) {
			type1pair.d1_delay = DELAY_INFINITY;
		    }
		    /*
	             *	Add the interface metric converted to a HELLO delay.
	             */
		    type1pair.d1_delay += hop_to_hello[ifp->int_metric] + HELLO_DELAY;

		    /* Determine routing table based on host bits */
		    rte_table = gd_inet_ishost(&dst) ? RTS_HOSTROUTE : RTS_INTERIOR;

		    /* check for internal route */
		    rt = rt_locate(rte_table, (sockaddr_un *) & dst, tp->task_rtproto);
		    if (!rt) {		/* new route */
			if (type1pair.d1_delay >= DELAY_INFINITY) {
			    continue;
			}
			rt = rt_add((sockaddr_un *) & dst,
				    (sockaddr_un *) 0,
				    (sockaddr_un *) & recv_addr,
				    gwp,
				    (metric_t) type1pair.d1_delay,
				    rte_table,
				    hello_task->task_rtproto,
				    0,
				    (time_t) 0,
				    preference);
			if (rt) {
			    rt->rt_data = rtd_alloc(sizeof(struct hello_win));
			    rt->rt_data->rtd_dump = hello_rt_dump;
			    hello_win_init((struct hello_win *) rt->rt_data->rtd_data, (metric_t) type1pair.d1_delay);
			}
		    } else {		/* update existing route */
			if (type1pair.d1_delay >= DELAY_INFINITY) {
			    /* destination now unreachable */
			    if (equal(&rt->rt_router, &recv_addr)) {
				(void) rt_unreach(rt);
				hello_win_add((struct hello_win *) rt->rt_data->rtd_data, DELAY_INFINITY);
			    }
			    continue;
			}
			if (equal(&rt->rt_router, &recv_addr)) {
			    if ((METRIC_DIFF(rt->rt_metric, type1pair.d1_delay) >= HELLO_HYST(rt->rt_metric)) ||
				(rt->rt_state & RTS_HOLDDOWN)) {
				if (rt_change(rt,
					      (sockaddr_un *) & recv_addr,
					   (metric_t) type1pair.d1_delay,
					      (time_t) 0,
					      preference)) {
				    hello_win_add((struct hello_win *) rt->rt_data->rtd_data, (metric_t) type1pair.d1_delay);
				}
			    }
			    rt_refresh(rt);
			} else {
			    if (rt->rt_state & RTS_HOLDDOWN) {
				continue;
			    }
			    /*
	                     * use the new gateway.
	                     */
			    if (((type1pair.d1_delay < rt->rt_metric) &&
				 (METRIC_DIFF(type1pair.d1_delay, rt->rt_metric) >= HELLO_HYST(rt->rt_metric))) ||
				(rt->rt_timer > rt->rt_timer_max / 2 &&
				 !(rt->rt_state & (RTS_CHANGED | RTS_REFRESH)))) {
				if (rt_change(rt,
					      (sockaddr_un *) & recv_addr,
					   (metric_t) type1pair.d1_delay,
					      (time_t) 0,
					      hello_preference)) {
				    hello_win_init((struct hello_win *) rt->rt_data->rtd_data, (metric_t) type1pair.d1_delay);
				}
			    }
			}
		    }
		}			/* for each advertized net */
		break;
	    default:
		trace(TR_INT, LOG_ERR, "hello_recv: invalid type %d", hm_hdr.hm_type);
	}				/* switch (mh->hm_type) */
    }					/* while not end of packet */

    if (rt_close(tp, gwp, (int) hm_hdr.hm_count)) {
	task_flash(tp);
    }
}


/*ARGSUSED*/
static void
hello_supply(tp, dst, src, ifp, gwp, do_split_horizon, flash_update)
task *tp;
struct sockaddr_in *dst, *src;
if_entry *ifp;
gw_entry *gwp;
int do_split_horizon;
int flash_update;
{
    int length;
    char *hello;
    char *save_ptr, *hm_hdr_ptr;
    rt_entry *rt;
    struct hm_hdr hm_hdr;
    struct type1pair type1pair;
    metric_t delay = 0, split_horizon;
    struct iovec *iovec = &recv_iovec[RECV_IOVEC_DATA];	/* Share receive buffer */

    hello = iovec->iov_base + Size_hellohdr;
    hm_hdr_ptr = hello;
    hello += Size_hm_hdr;
    save_ptr = hello;
    hm_hdr.hm_type = 1;
    hm_hdr.hm_count = 0;

    RT_TABLE(rt) {
	if (rt->rt_state & RTS_NOADVISE) {
	    continue;
	}
	if (flash_update && (hello_task->task_rtrevision >= rt->rt_revision)) {
	    continue;
	}
	/* Do not send interface routes back to the same interface */
	if ((rt->rt_ifp == ifp) && (rt->rt_proto & RTPROTO_DIRECT)) {
	    continue;
	}
	/* Subnets and host routes do not go everywhere */
	if (rt->rt_state & RTS_HOSTROUTE) {
	    if (((ifp->int_net.in.sin_addr.s_addr ^ rt->rt_dest.in.sin_addr.s_addr) &
		 ifp->int_netmask.in.sin_addr.s_addr) &&
		!((rt->rt_ifp->int_net.in.sin_addr.s_addr ^ rt->rt_dest.in.sin_addr.s_addr) &
		  rt->rt_ifp->int_netmask.in.sin_addr.s_addr)) {
		/* Host is being sent to another network and we learned it via it's home network */
		/* XXX - This assumes we are announcing the network route */
		continue;
	    }
	} else if (rt->rt_state & RTS_SUBNET) {
	    if ((rt->rt_dest.in.sin_addr.s_addr & ifp->int_netmask.in.sin_addr.s_addr) != ifp->int_net.in.sin_addr.s_addr) {
		/* Only send subnets to interfaces of the same network */
		continue;
	    }
	} else {
	    if (rt->rt_dest.in.sin_addr.s_addr == ifp->int_net.in.sin_addr.s_addr) {
		/* Do not send the whole net to a subnet */
		continue;
	    }
	}

	/* XXX - is this adequate? */
	if ((rt->rt_proto & hello_task->task_rtproto) && (ifp == rt->rt_ifp)) {
	    split_horizon = DELAY_INFINITY;
	} else {
	    split_horizon = 0;
	}

	if (rt->rt_ifp->int_state & IFS_LOOPBACK) {
	    /* Routes via the loopback interface must have an explicit metric */
	    delay = DELAY_INFINITY;
	}

	delay = (rt->rt_proto & RTPROTO_DIRECT) ? HELLO_DELAY : hello_default_metric;

	if (!propagate(rt,
		       hello_task->task_rtproto,
		       hello_propagate_list,
		       INT_CONTROL(hello_int_propagate, ifp),
		       gwp ? gwp->gw_propagate : NULL,
		       &delay)) {
	    continue;
	}
	delay += hop_to_hello[ifp->int_metric];
	if (delay > DELAY_INFINITY) {
	    delay = DELAY_INFINITY;
	}
	if (split_horizon) {
	    delay = split_horizon;
	}
	if (!(rt->rt_ifp->int_state & IFS_UP) || (rt->rt_state & (RTS_HOLDDOWN | RTS_DELETE))) {
	    delay = DELAY_INFINITY;
	}
	if (flash_update && (delay == DELAY_INFINITY) && (rt->rt_ifp == ifp) && (rt->rt_proto & hello_task->task_rtproto)) {
	    /* Don't flash deleted or unreachable routes back to their source */
	    continue;
	}
	if ((hello + Size_type1pair - iovec->iov_base) >= HELLOMAXPACKETSIZE) {
	    /* XXX - fragments should be rate limited to one every 0.5-2.0 seconds */
	    /* XXX - Maybe HELLO should have a random timer running all the time */
	    length = hello - iovec->iov_base;
	    hello = hm_hdr_ptr;
	    PutDown_hm_hdr(hello, hm_hdr);
	    hello_send(tp, dst, src, 0, iovec, length, (int) hm_hdr.hm_count);
	    hello = save_ptr;
	    hm_hdr.hm_count = 0;
	}
	type1pair.d1_delay = delay;
	type1pair.d1_offset = 0;	/* should be signed clock offset */
	type1pair.d1_dst = rt->rt_dest.in.sin_addr;	/* struct copy */

	PutDown_type1pair(hello, type1pair);
	hm_hdr.hm_count++;
    } RT_TABLEEND;

    if (hm_hdr.hm_count) {
	length = hello - iovec->iov_base;
	PutDown_hm_hdr(hm_hdr_ptr, hm_hdr);
	hello_send(tp, dst, src, 0, iovec, length, (int) hm_hdr.hm_count);
    }
}


/*
 *	call hello_supply to supply hello packets to all our nets
 */
/*ARGSUSED*/
static void
hello_job(tip, interval)
timer *tip;
time_t interval;
{
    task_toall(tip->timer_task,
	       hello_supply,
	       hello_pointopoint,
	       IFS_NOHELLOOUT,
	       hello_n_source ? hello_gw_list : NULL,
	       FALSE);
}


/*
 *	send a flash update packet
 */
/*ARGSUSED*/
static void
hello_do_flash(tip, interval)
timer *tip;
time_t interval;
{
    trace(TR_TASK, 0, "hello_do_flash: Doing flash update for HELLO");
    task_toall(hello_task,
	       hello_supply,
	       hello_pointopoint,
	       IFS_NOHELLOOUT,
	       hello_n_source ? hello_gw_list : NULL,
	       TRUE);
    hello_next_flash = (time_t) 2 + time_sec;
    trace(TR_TASK, 0, "hello_do_flash: Flash update done, none before %T", hello_next_flash);
}


/*
 *	Check to see if a flash update packet is allowed and send or schedule it
 */
static void
hello_flash(tp)
task *tp;
{
    if (time_sec >= hello_next_flash) {
	/* A flash update can be sent now, do it */
	hello_do_flash(tp->task_timer[HELLO_TIMER_FLASH], (time_t) 0);
    } else if (!tp->task_timer[HELLO_TIMER_FLASH] && (tp->task_timer[HELLO_TIMER_UPDATE]->timer_next_time > hello_next_flash)) {
	/* A flash update can't be sent and one is not yet scheduled */
	(void) timer_create(tp,
			    HELLO_TIMER_FLASH,
			    "Flash",
			    TIMERF_DELETE | TIMERF_ABSOLUTE,
			    hello_next_flash - time_sec,
			    hello_do_flash);
    }
}



/*
 *	Cleanup before re-init
 */
/*ARGSUSED*/
static void
hello_cleanup(tp)
task *tp;
{
    adv_cleanup(&hello_n_trusted, &hello_n_source, hello_gw_list,
		&hello_accept_list, &hello_propagate_list,
		&hello_int_accept, &hello_int_propagate);
}


/*
 *	Dump info about HELLO
 */
static void
hello_dump(fd)
FILE *fd;
{
    (void) fprintf(fd, "HELLO:\n");
    (void) fprintf(fd, "\tDefault metric: %d\t\tDefault preference: %d\n",
		   hello_default_metric,
		   hello_preference);
    if (hello_gw_list) {
	(void) fprintf(fd, "\tActive gateways:\n");
	gw_dump(fd, "\t\t", hello_gw_list);
    }
    control_accept_dump(fd, 1, hello_accept_list, hello_int_accept, hello_gw_list);
    control_propagate_dump(fd, 1, hello_propagate_list, hello_int_propagate, hello_gw_list);
    (void) fprintf(fd, "\n\n");
}


/*
 *	Initialize HELLO socket and task
 */
/*ARGSUSED*/
void
hello_init()
{
    int hello_socket = 1;
    if_entry *ifp;
    void (*flash) () = hello_flash;	/* Hack for UTX/32 and Ultrix */

    if (doing_hello) {
	if (!hello_task) {
	    if (hello_supplier < 0) {
		if (n_interfaces > 1) {
		    hello_supplier = TRUE;
		    trace(TR_ALL, LOG_NOTICE, "hello_init: Acting as HELLO supplier to our direct nets");
		}
		IF_LIST(ifp) {
		    if (ifp->int_state & IFS_POINTOPOINT) {
			hello_supplier = TRUE;
			trace(TR_INT, 0, "init_hello: PointoPoint HELLO supplier to: %s", ifp->int_name);
		    }
		} IF_LISTEND(ifp) ;
	    }
	    if (hello_supplier < 0) {
		hello_supplier = FALSE;
	    }
	    if ((hello_socket = task_get_socket(AF_INET, SOCK_RAW, IPPROTO_HELLO)) < 0) {
		quit(errno);
	    }
	    hello_task = task_alloc("HELLO");
	    hello_task->task_flags = TASKF_IPHEADER;
	    hello_task->task_proto = IPPROTO_HELLO;
	    hello_task->task_socket = hello_socket;
	    hello_task->task_rtproto = RTPROTO_HELLO;
	    hello_task->task_recv = hello_recv;
	    hello_task->task_cleanup = hello_cleanup;
	    hello_task->task_dump = hello_dump;

	    if (hello_supplier) {
		hello_task->task_flash = flash;
		(void) timer_create(hello_task,
				    HELLO_TIMER_UPDATE,
				    "Update",
				    0,
				    (time_t) HELLO_TIMERRATE,
				    hello_job);
		if (task_set_option(hello_task,
				    TASKOPTION_RECVBUF,
				    (caddr_t) (16 * 1024)) < 0) {
		    quit(errno);
		}
		if (task_set_option(hello_task,
				    TASKOPTION_DONTROUTE,
				    (caddr_t) TRUE) < 0) {
		    quit(errno);
		}
	    }
	    if (!task_create(hello_task, HELLOMAXPACKETSIZE)) {
		quit(EINVAL);
	    }
	    if (HELLO_TIMERRATE < rt_timer->timer_interval) {
		timer_interval(rt_timer, (time_t) HELLO_TIMERRATE);
	    }
	}
	if (hello_supplier) {
	    if_rtactive = TRUE;		/* Indicate we are broadcasting */
	    ignore_redirects = TRUE;	/* Gateways don't listen to redirects */
	}
    } else {
	hello_cleanup((task *) 0);
	if (hello_task) {
	    task_delete(hello_task);
	    hello_task = (task *) 0;
	}
    }
}


#endif				/* PROTO_HELLO */
