/* @(#) $Revision: 64.2 $ */     
/*
 * date(t) -- returns a pointer to the current date and time in local language
 *	t is a time as returned by time(2)
 */
#include	"lp.h"

#ifndef NLS
#define	DAYSIZE		4
#define	DATESIZE	13
#else NLS
#define NL_SETN 3					/* set number */
#include <nl_types.h>
#include <limits.h>
extern nl_catd nlmsg_tfd;
#endif NLS

char *
date(t)
time_t t;
{

#ifndef NLS
	char *ctime();
#else NLS
	char *nl_cxtime();
	static char loc_buf[NL_TEXTMAX];
#endif NLS

	char *dp;
	char *lp;

#ifndef NLS
	dp = ctime(&t) + DAYSIZE;
	*(dp + DATESIZE -1 ) = '\0';
#else NLS
	if (*(lp = catgetmsg(nlmsg_tfd, NL_SETN, 1, loc_buf, NL_TEXTMAX)) == NULL){
		lp = "%b %2d %H:%M";		/* catgets 1 */
	}
	dp = nl_cxtime(&t, lp);
	strncpy(loc_buf, dp, NL_TEXTMAX);
	dp = loc_buf;
	*(dp + NL_TEXTMAX - 1) = '0';
	if(*(dp + strlen(dp) - 1) == '\n')
		*(dp + strlen(dp) - 1) = '\0';
#endif NLS
	return(dp);
}
