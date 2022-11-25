/* @(#) $Revision: 32.3 $ */    

/*
 * Structure for entries in directory stack.
 */
struct	directory	{
	struct	directory *di_next;	/* next in loop */
	struct	directory *di_prev;	/* prev in loop */
	unsigned short *di_count;	/* refcount of processes */
#ifndef NONLS
	unsigned short *di_name;	/* actual name NLS: used to be char*/
#else
	char *di_name;
#endif
};
struct directory *dcwd;		/* the one we are in now */
