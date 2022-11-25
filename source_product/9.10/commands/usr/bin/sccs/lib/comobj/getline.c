/* @(#) $Revision: 37.2 $ */   
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
#include	"../../hdr/defines.h"

/*
	Routine to read a line into the packet.  The main reason for
	it is to make sure that pkt->p_wrttn gets turned off,
	and to increment pkt->p_slnno.
*/

char *
getline(pkt)
register struct packet *pkt;
{
	char *n, *fgets();
	register char *p;

	if(pkt->p_wrttn==0)
		putline(pkt,(char *) 0);
	if ((n = fgets(pkt->p_line,sizeof(pkt->p_line),pkt->p_iop)) != NULL) {
		pkt->p_slnno++;
		pkt->p_wrttn = 0;
		for (p = pkt->p_line; (p != NULL) && (*p); )
			pkt->p_chash += *p++;
	}
	else {
		if (!pkt->p_reopen) {
			fclose(pkt->p_iop);
			pkt->p_iop = 0;
		}
		if (!pkt->p_chkeof)
		{
			sprintf(Error,"%s (co5)",(nl_msg(171,"premature eof")));
			fatal(Error);
		}
		if (pkt->do_chksum && (pkt->p_chash ^ pkt->p_ihash)&0xFFFF)
		{
			sprintf(Error,"%s (co6)",(nl_msg(172,"corrupted file")));
			fatal(Error);
		}
		if (pkt->p_reopen) {
			fseek(pkt->p_iop,0L,0);
			pkt->p_reopen = 0;
			pkt->p_slnno = 0;
			pkt->p_ihash = 0;
			pkt->p_chash = 0;
			pkt->p_nhash = 0;
			pkt->p_keep = 0;
			pkt->do_chksum = 0;
		}
	}
	return(n);
}
