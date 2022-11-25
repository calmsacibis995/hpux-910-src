/* @(#) $Revision: 37.2 $ */      
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	"../../hdr/defines.h"


doie(pkt,ilist,elist,glist)
struct packet *pkt;
char *ilist, *elist, *glist;
{
	if (ilist) {
		if (pkt->p_verbose & DOLIST)
			fprintf(pkt->p_stdout,"%s\n",(nl_msg(141,"Included:")));
		dolist(pkt,ilist,INCLUDE);
	}
	if (elist) {
		if (pkt->p_verbose & DOLIST)
			fprintf(pkt->p_stdout,"%s\n",(nl_msg(142,"Excluded:")));
		dolist(pkt,elist,EXCLUDE);
	}
	if (glist)
		dolist(pkt,glist,IGNORE);
}
