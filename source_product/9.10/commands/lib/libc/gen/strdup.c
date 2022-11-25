/*	@(#) $Revision: 64.7 $	*/

/****************************** BEWARE **********************************
************************************************************************
***	You are looking at AT&T System V.3 source. DO NOT            ***
***	incorporate any of this code or the algorithms contained     ***
***	here without fully understanding the contractual obligations ***
***	between AT&T and HP regarding this source and obtaining	     ***
***	permission from your functional manager.		     ***
************************************************************************
************************************************************************/


/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/* string duplication
   returns pointer to a new string which is the duplicate of string
   pointed to by s1
   NULL is returned if new string can't be created
*/

#ifdef _NAMESPACE_CLEAN
#define strcpy _strcpy
#define strlen _strlen
#define strdup _strdup
#       ifdef   _ANSIC_CLEAN
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif

#include <stdlib.h>
#include <string.h>
#ifndef NULL
#define NULL	0
#endif

extern size_t strlen();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef strdup
#pragma _HP_SECONDARY_DEF _strdup strdup
#define strdup _strdup
#endif

char *
strdup(s1) 

   char * s1;

{  
   char * s2;

   s2 = (char *) malloc((unsigned) strlen(s1)+1) ;
   return(s2==NULL ? NULL : strcpy(s2,s1) );
}
