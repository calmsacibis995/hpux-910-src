/* @(#) $Revision: 37.2 $ */     
#ifdef NLS
#include	<msgbuf.h>
#endif
# include	"../../hdr/defines.h"

eqsid(s1, s2)
register struct sid *s1, *s2;
{
	if (s1->s_rel == s2->s_rel &&
		s1->s_lev == s2->s_lev &&
		s1->s_br == s2->s_br &&
		s1->s_seq == s2->s_seq)
			return(1);
	else
		return(0);
}
