/* @(#) $Revision: 64.1 $ */     
#ifndef _MISC_INCLUDED
#define _MISC_INCLUDED

#ifdef __hp9000s300
/*
 * structure to access an
 * integer in bytes
 */
struct
{
	char	hibyte;
	char	lobyte;
};

/*
 * structure to access an integer
 */
struct
{
	int	integ;
};

/*
 * structure to access a long as integers
 */
struct {
	int	hiword;
	int	loword;
};

/*
 *	structure to access an unsigned
 */
struct {
	unsigned	unsignd;
};
#endif /* __hp9000s300 */
#endif /* _MISC_INCLUDED */
