/*	@(#) getgroup.h:	$Revision: 1.16.109.1 $	$Date: 91/11/19 14:22:21 $  
	getgroup.h 1.1 86/02/05 (C) 1985 Sun Microsystems, Inc.	
	getgroup.h	2.1 86/04/16 NFSSRC 
*/

struct grouplist {		
	char *gl_machine;
	char *gl_name;
	char *gl_domain;
	struct grouplist *gl_nxt;
};

struct grouplist *my_getgroup();

			
