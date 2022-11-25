/*	   @(#) util.h:	$Revision: 1.19.109.1 $	$Date: 91/11/19 14:22:49 $  
	util.h 1.1 86/02/05 (C) 1985 Sun Microsystems, Inc. 
	util.h	2.1 86/04/16 NFSSRC 
*/

#define EOS '\0'

#ifndef NULL 
#	define NULL ((char *) 0)
#endif


#define MALLOC(object_type) ((object_type *) malloc(sizeof(object_type)))

#define FREE(ptr)	free((char *) ptr) 

/*  HPNFS
 *
 *  alloca is a Sun function which automatically frees allocated space
 *  when the called routine returns.  In revnetgroup, it's used only
 *  by getgroup.c
 *
 *  Dave Erickson, 1-22-87.
 *
 *  HPNFS  */

#define STRCPY(dst,src) \
	(dst = malloc((unsigned)strlen(src)+1), (void) strcpy(dst,src))

#define STRNCPY(dst,src,num) \
	(dst = malloc((unsigned)(num) + 1),\
	(void)strncpy(dst,src,num),(dst)[num] = EOS) 

extern char *malloc();

char *getline();
void fatal();
