/* @(#) $Revision: 37.4 $ */    
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	"../../hdr/defines.h"

static char br[]  = "bad range";	/*nl_msg 151*/
   

dolist(pkt,list,ch)
struct packet *pkt;
register char *list;
char ch;
{
	char *getasid();
	struct sid lowsid, highsid, sid;
	int n;

	while (*list) {
		list = getasid(list,&lowsid);
		if (*list == '-') {
			++list;
			list = getasid(list,&highsid);
			if (lowsid.s_br == 0) {
				if ((highsid.s_br || highsid.s_seq ||
					highsid.s_rel < lowsid.s_rel ||
					(highsid.s_rel == lowsid.s_rel &&
					highsid.s_lev < lowsid.s_lev)))
					{
						sprintf(Error,nl_msg(151,br));
						fatal(strcat(Error," (co12)"));
					}
				sid.s_br = sid.s_seq = 0;
				for (sid.s_rel = lowsid.s_rel; sid.s_rel <= highsid.s_rel; sid.s_rel++) {
					sid.s_lev = (sid.s_rel == lowsid.s_rel ? lowsid.s_lev : 1);
					for ( ;((sid.s_rel != highsid.s_rel) ||
						(sid.s_lev <= highsid.s_lev))
						&& (n = sidtoser(&sid,pkt));
						 sid.s_lev++)
						enter(pkt,ch,n,&sid);
				}
			}
			else {
				if (!(highsid.s_rel == lowsid.s_rel &&
					highsid.s_lev == lowsid.s_lev &&
					highsid.s_br == lowsid.s_br &&
					highsid.s_seq >= lowsid.s_seq))
				{
					sprintf(Error,nl_msg(151,br));
					fatal(strcat(Error," (co12)"));
				}
				for (; lowsid.s_seq <= highsid.s_seq &&
					(n = sidtoser(&lowsid,pkt)); lowsid.s_seq++)
						enter(pkt,ch,n,&lowsid);
			}
		}
		else {
			if (n = sidtoser(&lowsid,pkt))
				enter(pkt,ch,n,&lowsid);
		}
		if (*list == ',')
			++list;
	}
}


static char dls[]  =  "delta list syntax";	/*nl_msg 152*/

char *
getasid(p,sp)
register char *p;
register struct sid *sp;
{
	register char *old;
	char *sid_ab();

	p = sid_ab(old = p,sp);
	if (old == p || sp->s_rel == 0)
	{
		sprintf(Error,nl_msg(152,dls));
		fatal(strcat(Error," (co13)"));
	}
	if (sp->s_lev == 0) {
		sp->s_lev = MAX;
		if (sp->s_br || sp->s_seq)
		{
			sprintf(Error,nl_msg(152,dls));
			fatal(strcat(Error," (co13)"));
		}
	}
	else if (sp->s_br) {
		if (sp->s_seq == 0)
			sp->s_seq = MAX;
	}
	else if (sp->s_seq)
		{
			sprintf(Error,nl_msg(152,dls));
			fatal(strcat(Error," (co13)"));
		}
	return(p);
}
