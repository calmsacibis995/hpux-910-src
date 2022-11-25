/* $Header: privgrp.h,v 1.21.83.4 93/09/17 18:32:19 kcs Exp $ */  

#ifndef _SYS_PRIVGRP_INCLUDED
#define _SYS_PRIVGRP_INCLUDED

/*
 * privgrp.h: Parameters for control of privileged group facility
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/*
 * Privileged group definitions
 *
 * All definitions must have the privilege (in upper case) comment
 * so that getprivgrp and setprivgrp may find available privileged
 * groups and associated numbers.  (Don't put privilege (in upper case)
 * anywhere else in this file!)
 */
#define	PRIV_RTPRIO	1	/* PRIVILEGE */
#define	PRIV_MLOCK	2	/* PRIVILEGE */
#define	PRIV_CHOWN	3	/* PRIVILEGE */
#define PRIV_LOCKRDONLY 4	/* PRIVILEGE */
#define PRIV_SETRUGID	5	/* PRIVILEGE */
#define	PRIV_MPCTL	6	/* PRIVILEGE */

#define privmask(x) (1<<(x-1))

#define privmask_word1  ( 	privmask(PRIV_RTPRIO) | \
				privmask(PRIV_MLOCK)  | \
				privmask(PRIV_CHOWN)  | \
				privmask(PRIV_LOCKRDONLY)  | \
				privmask(PRIV_SETRUGID)  | \
				privmask(PRIV_MPCTL))

/* Multi-word logical or of all privileges.  */
#define PRIV_ALLPRIV(word)      (word == 0 ? privmask_word1 : 0x0) /* all privileges */

/* Maximum number of privileged groups in system */
#define PRIV_MAXGRPS	33

/* Size of the privilege mask, based on largest numbered privilege */
#define PRIV_MASKSIZ	2

/* Special group ids recognized by setprivgrp(2) */
#define PRIV_NONE	((gid_t) -1)	/* grant privilege to no one */
#define PRIV_GLOBAL	((gid_t) -2)	/* grant privilege to all */

/* PRIV_MAXGRPS includes the elements in privgrp_map plus */
/* one element for the global privileges in priv_global.  */
#define	EFFECTIVE_MAXGRPS	(PRIV_MAXGRPS-1)

/* Structure defining the privilege mask */
struct privgrp_map {
#ifdef _CLASSIC_ID_TYPES
    int   		priv_groupno;
#else
    gid_t   		priv_groupno;
#endif
    unsigned int	priv_mask[PRIV_MASKSIZ];
};


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
  extern int getprivgrp(struct privgrp_map *);
  extern int setprivgrp(gid_t, const int *);
#else /* not _PROTOTYPES */
  extern int getprivgrp();
  extern int setprivgrp();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_PRIVGRP_INCLUDED */
