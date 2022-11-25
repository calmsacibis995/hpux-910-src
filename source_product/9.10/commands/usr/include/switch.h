/*
 * Copyright (c) 1993 Hewlett-Packard Company. All rights reserved.
 *
 *	@(#)switch.h	$Header: switch.h,v 70.1 93/11/10 12:10:49 ssa Exp $
 *
 * This include file defines the constants, structures and function prototypes
 * used by the switch and required by a Switch based module. It reflects the
 * generic nature of the switch and its potential use in a wide variety of
 * contexts.
 * The names of constants, structures and function prototypes are derived from
 * the name service switch ERS document.
 */
#ifndef  _NSSWITCH_INCLUDED
#define  _NSSWITCH_INCLUDED

#ifdef  __cplusplus
extern  "C" {
#endif

/*
 * Info-classes / databases
 */
#define	__NSW_MXINFCLASS	14
#define	__NSW_ALIASES_DB	"aliases"
#define	__NSW_AUTOMOUNT_DB	"automount"
#define	__NSW_BOOTPARAMS_DB	"bootparams"
#define	__NSW_ETHERS_DB		"ethers"
#define	__NSW_GROUP_DB		"group"
#define	__NSW_HOSTS_DB		"hosts"
#define	__NSW_NETGROUP_DB	"netgroup"
#define	__NSW_NETMASKS_DB	"netmasks"
#define	__NSW_NETWORKS_DB	"networks"
#define	__NSW_PASSWD_DB		"passwd"
#define	__NSW_PROTOCOLS_DB	"protocols"
#define	__NSW_PUBLICKEY_DB	"publickey"
#define	__NSW_RPC_DB		"rpc"
#define	__NSW_SERVICES_DB	"services"

/*
 * Services
 *
 * 00-08: Reserved
 * 09	: Unknown
 * 10-XX: Supported Sources
 */
#define	  SRC_UNKNOWN	9
#define	  SRC_DNS	11
#define	  SRC_NIS	12
#define	  SRC_FILES	13

/*
 *-----------------------------------------
 * 	INFO-CLASS/DATABASE (e.g. NAME) SERVICE INTERFACE
 * Status - of a source lookup operation
 * Action - 
 *-----------------------------------------
 */
/*
 * Statuses 
 */
#define	__NSW_LOOKUP_ERRS	4 /* # of statuses */
#define	__NSW_STD_ERRS		__NSW_LOOKUP_ERRS
#define	__NSW_SUCCESS		0 /* name lookup succeeded */
#define	__NSW_NOTFOUND		1 /* name not found, service avl. */
#define	__NSW_UNAVAIL		2 /* service unavailable */
#define	__NSW_TRYAGAIN		3 /* service busy - try again */
#define	__NSW_STR_SUCCESS	"success"
#define	__NSW_STR_NOTFOUND	"notfound"
#define	__NSW_STR_UNAVAIL	"unavail"
#define	__NSW_STR_TRYAGAIN	"tryagain"
/*
 * Actions
 */
typedef unsigned char	action_t;
#define	__NSW_CONTINUE	0
#define	__NSW_RETURN	1
#define	__NSW_DEFUALT	__NSW_CONTINUE
#define	__NSW_STR_RETURN	"return"
#define	__NSW_STR_CONTINUE	"continue"

/*
 *-----------------------------------------
 *		SWITCH INTERFACE
 * Status - of open+read operation on the config file
 * Types
 * Routines
 *-----------------------------------------
 */
#define  __NSW_CONF_FILE	"/etc/nsswitch.conf"
/* 
 * Status
 */
enum	__nsw_parse_err {
  __NSW_PARSE_SUCCESS	=0, /* policy entry found */
  __NSW_PARSE_NOFILE	=1, /* config file not present */
  __NSW_PARSE_NOPOLICY	=2, /* policy entry not found */
  __NSW_PARSE_SYSERR	=3 /* system error during parse */
} ;
#define	__NSW_PARSE_ERRS	4 /* # of statuses */
#define	T_NSW_PARSEERR		enum  __nsw_parse_err
/*
 * Types
 */
struct	__nsw_long_err {
	int		errno;
	action_t	action;
	struct  __nsw_long_err	*next;
} ;
struct	__nsw_lookup  {
	char *		service_name;
	action_t	action [__NSW_STD_ERRS];
	struct  __nsw_long_err	*long_errs;
	struct  __nsw_lookup	*next;
} ;
#define	T_NSW_LOOKUP		struct  __nsw_lookup
#define	__NSW_ACTION(a,b) (a->action[b])
struct	__nsw_switchconfig  {
	int		vers;
	char *		dbase;
	int		num_lookups;
	T_NSW_LOOKUP	*lookups;
} ;
#define  T_NSW_SWCONFIG		struct  __nsw_switchconfig
/*
 * Interface Routine Prototypes
 */
#ifdef	NO_PROTOTYPES
#undef  USE_PROTOTYPES
#endif

#ifdef	USE_PROTOTYPES
/* Systems where function prototypes work */
T_NSW_SWCONFIG *	__nsw_getconfig (char *, T_NSW_PARSEERR *);
int	__nsw_freeconfig (T_NSW_SWCONFIG *);
T_NSW_SWCONFIG *	__nsw_getdefault (char *);
int	__nsw_dumpconfig (T_NSW_SWCONFIG *);
int	__nsw_inteqv (char *);

#define	__PF0(void)	(void)
#define	__PF1(arg1, type1) (type1 arg1)
#define	__PF2(arg1, type1, arg2, type2) \
    (type1 arg1, \
     type2 arg2)
#define	__PF3(arg1, type1, arg2, type2, arg3, type3) \
    (type1 arg1, \
     type2 arg2, \
     type3 arg3)
#define	__PF4(arg1, type1, arg2, type2, arg3, type3, arg4, type4) \
    (type1 arg1, \
     type2 arg2, \
     type3 arg3, \
     type4 arg4)
#else
T_NSW_SWCONFIG *	__nsw_getconfig ();
int	__nsw_freeconfig ();
T_NSW_SWCONFIG *	__nsw_getdefault ();
int	__nsw_dumpconfig ();
int	__nsw_inteqv ();

#define	__PF0(void) ()
#define	__PF1(arg1, type1) (arg1) \
    type1 arg1;
#define	__PF2(arg1, type1, arg2, type2) \
    (arg1, arg2) \
    type1 arg1; \
    type2 arg2;
#define	__PF3(arg1, type1, arg2, type2, arg3, type3) \
    (arg1, arg2, arg3) \
    type1 arg1; \
    type2 arg2; \
    type3 arg3;
#define	__PF4(arg1, type1, arg2, type2, arg3, type3, arg4, type4) \
    (arg1, arg2, arg3, arg4) \
    type1 arg1; \
    type2 arg2; \
    type3 arg3; \
    type4 arg4;
#endif	/* USE_PROTOTYPES */

#ifdef  __cplusplus
}
#endif

#endif  /*  _NSSWITCH_INCLUDED */

