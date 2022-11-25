/* $Revision: 64.4 $ */

/******************************************************************
 *   
 *    setuname(2)
 *		sets the nodname for the system
 *		for utsname structure with max
 *		of UTSLEN (9)
 *    
 *
 *****************************************************************/


#define SETUNAME 3
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _setuname setuname
#define setuname _setuname
#endif

int
setuname(name, len)
char	*name;
int	len;
{
	return(_utssys(name, len, SETUNAME));
}
