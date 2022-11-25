/* $Revision: 64.4 $ */
#include <sys/param.h>

/******************************************************************
 *   
 *    gethostname(2)
 *		returns the hostname for the system 
 *              with a max  of MAXHOSTNAMELEN (255) characters.
 *    sethostname(2)
 *		sets the hostname for the system
 *              with a max  of MAXHOSTNAMELEN (255) characters.
 *    
 *
 *****************************************************************/

#define	SETHOSTNAME	4
#define	GETHOSTNAME	5

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _gethostname gethostname
#   define gethostname _gethostname
#endif /* _NAMESPACE_CLEAN */
int
gethostname(name, len)
char	*name;
int	len;
{
	return(_utssys(name, len, GETHOSTNAME));
}

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _sethostname sethostname
#   define sethostname _sethostname
#endif /* _NAMESPACE_CLEAN */
int
sethostname(name, len)
char	*name;
int	len;
{
	return(_utssys(name, len, SETHOSTNAME));
}
