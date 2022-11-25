/* @(#) $Revision: 70.2 $ */     
/*	compact.h	4.4	83/06/03	*/

#define VAX 11/780


#if defined(vax) || defined(sun)
typedef int longint;
#else
typedef long longint;
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ndir.h>
#include <stdio.h>

#ifndef	MAXPATHLEN
#define	MAXPATHLEN 1024
#endif
#ifndef	MAXNAMLEN
#define	MAXNAMLEN 255
#endif
#define LNAME (MAXPATHLEN+1)
#define NEW flist; flist = flist -> next
#define LLEAF 010
#define RLEAF 04
#define SEEN 02
#define FBIT 01
#define COMPACTED 017777
#define PACKED 017437
#define EF 0400
#define NC 0401
/*#define DIRSIZE 514 */
#define DIRSIZE 1028

struct charac {
#if defined(vax) || defined(pdp11)
	char lob, hib;
#else
	char hib, lob;
#endif
};

union cio {
	struct charac chars;
	short integ;
};

struct fpoint {
	struct node *fp;
	int flags;
} in [258];

struct index {
	struct node *pt;
	struct index *next;
} dir [DIRSIZE], *head, *flist, *dirp, *dirq;

union treep {
	struct node *p;
	int ch;
};

struct node {
	struct fpoint fath;
	union treep sp [2];
	struct index *top [2];
	longint count [2];
} dict [258], *bottom;

longint oc;

FILE *cfp, *uncfp;

struct stat status;
