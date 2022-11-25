/*	@(#)nlm_prot.h	$Revision: 1.8.109.1 $	$Date: 91/11/19 14:42:40 $  */
/*      (c) Copyright 1988 Hewlett-Packard Company  */
/*      (c) Copyright 1986 Sun Microsystems, Inc.   */

/*
 *
 * nlm_prot.h
 * generated from rpcgen -h nlm_prot.x
 */
 
#ifndef _RPCSVC_NLM_PROT_INCLUDED
#define _RPCSVC_NLM_PROT_INCLUDED

#include <rpcsvc/klm_prot.h>

#define NLM_PROG 100021
#define NLM_VERS 1
#define NLM_TEST 1
#define NLM_LOCK 2
#define NLM_CANCEL 3
#define NLM_UNLOCK 4
#define NLM_GRANTED 5
#define NLM_TEST_MSG 6
#define NLM_LOCK_MSG 7
#define NLM_CANCEL_MSG 8
#define NLM_UNLOCK_MSG 9
#define NLM_GRANTED_MSG 10
#define NLM_TEST_RES 11
#define NLM_LOCK_RES 12
#define NLM_CANCEL_RES 13
#define NLM_UNLOCK_RES 14
#define NLM_GRANTED_RES 15

#define LM_MAXSTRLEN	1024

#define NLM_VERSX 3
#define NLM_SHARE 20
#define NLM_UNSHARE 21
#define NLM_NM_LOCK 22
#define NLM_FREE_ALL 23
#define MAXNAMELEN 	(LM_MAXSTRLEN + 1)

enum nlm_stats {
	nlm_granted = 0,
	nlm_denied = 1,
	nlm_denied_nolocks = 2,
	nlm_blocked = 3,
	nlm_denied_grace_period = 4,
};
typedef enum nlm_stats nlm_stats;


struct nlm_holder {
	bool_t exclusive;
	int svid;
	netobj oh;
	u_int l_offset;
	u_int l_len;
};
typedef struct nlm_holder nlm_holder;


struct nlm_testrply {
	nlm_stats stat;
	union {
		struct nlm_holder holder;
	} nlm_testrply;
};
typedef struct nlm_testrply nlm_testrply;


struct nlm_stat {
	nlm_stats stat;
};
typedef struct nlm_stat nlm_stat;


struct nlm_res {
	netobj cookie;
	nlm_stat stat;
};
typedef struct nlm_res nlm_res;


struct nlm_testres {
	netobj cookie;
	nlm_testrply stat;
};
typedef struct nlm_testres nlm_testres;


struct nlm_lock {
	char *caller_name;
	netobj fh;
	netobj oh;
	int svid;
	u_int l_offset;
	u_int l_len;
};
typedef struct nlm_lock nlm_lock;


struct nlm_lockargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm_lock lock;
	bool_t reclaim;
	int state;
};
typedef struct nlm_lockargs nlm_lockargs;


struct nlm_cancargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm_lock lock;
};
typedef struct nlm_cancargs nlm_cancargs;


struct nlm_testargs {
	netobj cookie;
	bool_t exclusive;
	struct nlm_lock lock;
};
typedef struct nlm_testargs nlm_testargs;


struct nlm_unlockargs {
	netobj cookie;
	struct nlm_lock lock;
};
typedef struct nlm_unlockargs nlm_unlockargs;

bool_t xdr_nlm_stats();
bool_t xdr_nlm_holder();
bool_t xdr_nlm_testrply();
bool_t xdr_nlm_stat();
bool_t xdr_nlm_res();
bool_t xdr_nlm_testres();
bool_t xdr_nlm_lock();
bool_t xdr_nlm_lockargs();
bool_t xdr_nlm_cancargs();
bool_t xdr_nlm_testargs();
bool_t xdr_nlm_unlockargs();

/*
 * The following enums are actually bit encoded for efficient
 * boolean algebra.... DON'T change them.....
 */

enum fsh_mode {
	fsm_DN = 0,
	fsm_DR = 1,
	fsm_DW = 2,
	fsm_DRW = 3,
};
typedef enum fsh_mode fsh_mode;
bool_t xdr_fsh_mode();


enum fsh_access {
	fsa_NONE = 0,
	fsa_R = 1,
	fsa_W = 2,
	fsa_RW = 3,
};
typedef enum fsh_access fsh_access;
bool_t xdr_fsh_access();


struct nlm_share {
	char *caller_name;
	netobj fh;
	netobj oh;
	fsh_mode mode;
	fsh_access access;
};
typedef struct nlm_share nlm_share;
bool_t xdr_nlm_share();


struct nlm_shareargs {
	netobj cookie;
	nlm_share share;
	bool_t reclaim;
};
typedef struct nlm_shareargs nlm_shareargs;
bool_t xdr_nlm_shareargs();


struct nlm_shareres {
	netobj cookie;
	nlm_stats stat;
	int sequence;
};
typedef struct nlm_shareres nlm_shareres;
bool_t xdr_nlm_shareres();


struct nlm_notify {
	char *name;
	long state;
};
typedef struct nlm_notify nlm_notify;
bool_t xdr_nlm_notify();

#endif /* _RPCSVC_NLM_PROT_INCLUDED */
