/* HPUX_ID: @(#) $Revision: 56.1 $  */

/* structures for user NSP servers */

struct unsp_routine
{
	int op;		/*operation*/
	char *func;	/*function to be called*/
};

extern struct unsp_routine unsp_routines[];
