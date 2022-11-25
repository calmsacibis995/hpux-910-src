/* @(#) $Revision: 37.2 $ */    
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	"../../hdr/defines.h"


fmterr(pkt)
register struct packet *pkt;
{
	fclose(pkt->p_iop);
	sprintf(Error,(nl_msg(161,"format error at line %u")),pkt->p_slnno);
	fatal(strcat(Error," (co4)"));
}
