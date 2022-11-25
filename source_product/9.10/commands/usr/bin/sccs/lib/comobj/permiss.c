/* @(#) $Revision: 37.2 $ */   
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	"../../hdr/defines.h"


finduser(pkt)
register struct packet *pkt;
{
	register char *p;
	char *user, *logname();
	char *strend(), *getline();
	char groupid[6];
	int none;
	int ok_user;

	none = 1;
	user = logname();
	sprintf(groupid,"%d",getgid());
	while ((p = getline(pkt)) != NULL && *p != CTLCHAR) {
		none = 0;
		ok_user = 1;
		repl(p,'\n','\0');	/* this is done for equal test below */
		if(*p == '!') {
			++p;
			ok_user = 0;
			}
		if (!pkt->p_user)
			if (equal(user,p) || equal(groupid,p))
				pkt->p_user = ok_user;
		*(strend(p)) = '\n';	/* repl \0 end of line w/ \n again */
	}
	if (none)
		pkt->p_user = 1;
	if (p == NULL || p[1] != EUSERNAM)
		fmterr(pkt);
}


char	*Sflags[NFLAGS] = {'\000'};

doflags(pkt)
struct packet *pkt;
{
	register char *p;
	register int k;
	char *getline(), *fmalloc();

	for (k = 0; k < NFLAGS; k++)
		Sflags[k] = 0;
	while ((p = getline(pkt)) != NULL && *p++ == CTLCHAR && *p++ == FLAG) {
		NONBLANK(p);
		k = *p++ - 'a';
		NONBLANK(p);
		Sflags[k] = fmalloc(size(p));
		copy(p,Sflags[k]);
		for (p = Sflags[k]; *p++ != '\n'; )
			;
		*--p = 0;
	}
}


permiss(pkt)
register struct packet *pkt;
{
/*	extern char *Sflags[];  */
	register char *p;
	register int n;

	if (!pkt->p_user)
	{
		sprintf(Error,"%s (co14)",(nl_msg(181,"not authorized to make deltas")));
		fatal(Error);
	}
	if (p = Sflags[FLORFLAG - 'a']) {
		if (((unsigned)pkt->p_reqsid.s_rel) < (n = patoi(p))) {
#ifdef NLS		
			sprintmsg(Error,(nl_msg(182,"release %1$u < %2$u (floor)")),
				pkt->p_reqsid.s_rel,n);
			strcat(Error, " (co15)");
#else
			sprintf(Error,"release %u < %u (floor) (co15)",
				pkt->p_reqsid.s_rel,n);
#endif  
			fatal(Error);
		}
	}
	if (p = Sflags[CEILFLAG - 'a']) {
		if (((unsigned)pkt->p_reqsid.s_rel) > (n = patoi(p))) {
#ifdef NLS 
			sprintmsg(Error,(nl_msg(183,"release %1$u > %2$u (ceiling)")),
				pkt->p_reqsid.s_rel,n);
			strcat(Error, " (co16)");
#else
			sprintf(Error,"release %u > %u (ceiling) (co16)",
				pkt->p_reqsid.s_rel,n);
#endif  
			fatal(Error);
		}
	}
	/*
	check to see if the file or any particular release is
	locked against editing. (Only if the `l' flag is set)
	*/
	if ((p = Sflags[LOCKFLAG - 'a']))
		ck_lock(p,pkt);
}



char  l_str[]	=	"SCCS file locked against editing (co23)";
ck_lock(p,pkt)
register char *p;
register struct packet *pkt;
{
	int l_rel;
	int locked;
#ifdef NLS
	sprintf(l_str,"%s (co23)",(nl_msg(184,"SCCS file locked against editing")));

#endif

	locked = 0;
	if (*p == 'a')
		locked++;
	else while(*p) {
		p = satoi(p,&l_rel);
		++p;
		if (l_rel == pkt->p_gotsid.s_rel || l_rel == pkt->p_reqsid.s_rel) {
			locked++;
			sprintf(l_str,nl_msg(185,"release `%d' locked against editing"),l_rel);
			strcat(l_str," (co23)");
			break;
		}
	}
	if (locked)
		fatal(l_str);
}
