/* $Header: useyellow.c,v 64.3 89/01/18 18:25:48 rsh Exp $ */

/*
**	usingyellow default initialization
*/

#ifdef HP_NFS

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _usingyellow usingyellow
#define usingyellow _usingyellow
#endif

int usingyellow = 0;	/* assume we aren't */

#endif HP_NFS
