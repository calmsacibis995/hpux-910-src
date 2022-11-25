/* @(#) $Revision: 70.1 $ */
/* HP-UX  cluster.h  3.2  4/11/86  08:56:57 */
#ifndef _CLUSTER_INCLUDED /* allows multiple inclusions */
#define _CLUSTER_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HFS

#ifndef MAX_CNODE
#  define MAX_CNODE	255
#endif /* MAX_CNODE */

/*
 * NOTE:  The MAXCNODE macro is left here only for compatibility with
 * previous source code.  Its use is discouraged and it may be
 * removed from a future release of HP-UX.  Please change all
 * necessary references from MAXCNODE to (MAX_CNODE + 1).
 */
#define MAXCNODE	(MAX_CNODE + 1)

#define M_IDLEN		6	/* machine_id length */

/*
 * values for cluster system calls's func paramter
 */
#define CLUSTER_SETID	0
#define CLUSTER_CHMOD	1


#ifndef _CNODE_T
#  define _CNODE_T
   typedef unsigned short int cnode_t;
#endif /* _CNODE_T

/*
 * cluster configuration table entry
 */

struct cct_entry {
	unsigned char machine_id[M_IDLEN];
	cnode_t cnode_id;
	char cnode_type;	/*r for root server, c -- for others */
	char cnode_name[15];
	cnode_t swap_serving_cnode;
	int kcsp;
};			

#if defined(__STDC__) || defined(__cplusplus)
#  ifndef _STDIO_INCLUDED
#    include <stdio.h> /* for FILE struct definition */
#  endif /* _STDIO_INCLUDED */
   extern struct cct_entry *getccent(void);
   extern struct cct_entry *getcccid(cnode_t);
   extern struct cct_entry *getccnam(const char *);
   extern struct cct_entry *fgetccent(FILE *);
   extern void setccent(void);
   extern void endccent(void);
   extern cnode_t cnodeid(void);
   extern int cnodes(cnode_t *);
#else /* not __STDC__ || __cplusplus */
   extern struct cct_entry *getccent();
   extern struct cct_entry *getcccid();
   extern struct cct_entry *getccnam();
   extern struct cct_entry *fgetccent();
   extern void setccent();
   extern void endccent();
   extern cnode_t cnodeid();
   extern int cnodes();
#endif /* else not __STDC__ || __cplusplus */

#endif /* HFS */

#ifdef __cplusplus
}
#endif

#endif /* _CLUSTER_INCLUDED */
