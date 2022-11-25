
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS
#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 72.12 $";
#endif
