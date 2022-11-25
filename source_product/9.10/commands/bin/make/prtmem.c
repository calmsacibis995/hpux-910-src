/* @(#) $Revision: 66.1 $ */    

/************************************************************/
/*	This file is completely ifdef'd and GETU is not defined
	for make when it is built; therefore, this code is never
	used.  It is a very anachronistic vestige of the time
	when people were running make on machines with tight
	memory restrictions (64K machines).  What it does is
	print out d's for all data locations in the u-area,
	prints out s's for all stack data locations in the u-area,
	and .'s for the space left over between the two.  Evidently
	this is so that a user can see how much data and stack
	space make used when it executed any particular makefile.
	Sizes are hard coded here; obviously if one were to try
	to run this on today's HP-UX virtual memory systems,
	one would have a meaningless mess.

	Keep this code around for historical reasons; but if in
	the future our code needs to be compared to a more recent
	version of make without this code, it can be removed.   */
/************************************************************/
 
#ifdef GETU
#define udsize uu[0]
#define ussize uu[1]
#endif


prtmem()
{
#ifdef GETU
#include "stdio.h"
#include "sys/param.h"
#include "sys/dir.h"
#include "sys/user.h"
	unsigned uu[2];
	register int i;

	if(getu( &((struct user *)0)->u_dsize, &uu, sizeof uu) > 0)
	{
		udsize *= 64;
		ussize *= 64;
		printf("mem: data = %u(0%o) stack = %u(0%o)\n",
			udsize, udsize, ussize, ussize);
/*
 *	The following works only when `make' is compiled
 *	with I&D space separated (i.e. cc -i ...).
 *	(Notice the hard coded `65' below!)
 */
		udsize /= 1000;
		ussize /= 1000;
		printf("mem:");
		for(i=1; i<=udsize;i++)
		{
			if((i%10) == 0)
				printf("___");
			printf("d");
		}
		for(;i<=65-ussize;i++)
		{
			if((i%10) == 0)
				printf("___");
			printf(".");
		}
		for(;i<=65;i++)
		{
			if((i%10) == 0)
				printf("___");
			printf("s");
		}
		printf("\n");
		fflush(stdout);
	}
#endif
}
