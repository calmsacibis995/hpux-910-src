/* @(#) $Revision: 64.4 $ */
static char *HPUX_ID = "@(#) $Revision: 64.4 $";

#ifdef NLS
#define NL_SETN  4
#include	<msgbuf.h>
#include	<locale.h>
#else
#define nl_msg(i, s) (s)
#endif
#ifdef NLS16
#include <nl_ctype.h>
#endif NLS16

# include	"../hdr/defines.h"
# include	"../hdr/had.h"

struct stat Statbuf;
char Null[1];
char Error[128];

int	Debug = 0;
int	had_pfile;
struct packet gpkt;
struct sid sid;
unsigned	Ser;
int	num_files;
char	had[26];
char	Whatstr[BUFSIZ];
char	Pfilename[FILESIZE];
char	*ilist, *elist, *lfile;
char	*sid_ab(), *auxf(), *logname();
char	*sid_ba(), *date_ba();
long	cutoff = 0X7FFFFFFFL;	/* max positive long */
int verbosity;
char	Gfile[FILESIZE];
char	*Type;
int	Did_id;
FILE *fdfopen();

/* Beginning of modifications made for CMF system. */
#define CMRLIMIT 128
char	cmr[CMRLIMIT];
int		cmri = 0;
/* End of insertion */

main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	char c;
	int testmore;
	extern int Fcnt;
/*	extern get();	Modified by T.O on 85/7/5
			Beacause get() is duplicated with anything
			in /lib/libc.a. Maybe I think one of the
			NLS tool. So I change the name from get()
			to _get().
*/
	extern _get();

#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("get"), stderr);
		putenv("LANG=");
	}
	nl_catopen("sccs");
#endif NLS || NLS16

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
	for(i=1; i<argc; i++)
		if(argv[i][0] == '-' && (c=argv[i][1])) {
			p = &argv[i][2];
			testmore = 0;
			switch (c) {

    			case CMFFLAG:
				/* Concatenate the rest of this argument with the
				 * existing CMR list. */
				while (*p) {
					if (cmri == CMRLIMIT)
						fatal ((nl_msg(1,"CMR list is too long.")));
					cmr[cmri++] = *p++;
					}
				cmr[cmri] = NULL;
				break;

			case 'a':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				Ser = patoi(p);
				break;
			case 'r':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				chksid(sid_ab(p,&sid),&sid);
				if ((sid.s_rel < MINR) ||
					(sid.s_rel > MAXR))
					{
					sprintf(Error,"%s (ge22)",nl_msg(2,"r out of range"));
					fatal(Error);
					}
				break;
			case 'w':
				if(*p)
					strcpy(Whatstr,p);
				break;
			case 'c':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				if (date_ab(p,&cutoff))
				{
					sprintf(Error,"%s (cm5)",nl_msg(3,"bad date/time"));
					fatal(Error);
				}
				break;
			case 'l':
				lfile = p;
				break;
			case 'i':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				ilist = p;
				break;
			case 'x':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				elist = p;
				break;
			case 'b':
			case 'g':
			case 'e':
			case 'p':
			case 'k':
			case 'm':
			case 'n':
			case 's':
			case 't':
				testmore++;
				break;
			default:
				sprintf(Error,"%s (cm1)",nl_msg(4,"unknown key letter"));
				fatal(Error);
			}

			if (testmore) {
				testmore = 0;
				if (*p) {
					sprintf(Error,
						nl_msg(5,"value after %c arg"),c);
					fatal(strcat(Error," (cm8)"));
				}
			}
			if (had[c - 'a']++)
			{
				sprintf(Error,"%s (cm2)",nl_msg(6,"key letter twice"));
				fatal(Error);
			}
			argv[i] = 0;
		}
		else num_files++;

	if(num_files == 0)
	{
		sprintf(Error,"%s (cm3)",nl_msg(7,"missing file arg"));
		fatal(Error);
	}
	if (HADE && HADM)
	{
		sprintf(Error,"%s (ge3)",nl_msg(8,"e not allowed with m"));
		fatal(Error);
	}
	if (HADE)
		HADK = 1;
	if (!HADS)
		verbosity = -1;
	if (HADE && ! logname())
	{
		sprintf(Error,"%s (cm9)", nl_msg(9,"User ID not in password file"));
		fatal(Error);
	}
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if (p=argv[i])
/*			do_file(p,get);	Modified by T.O on 85/7/5	*/
			do_file(p,_get);
	exit(Fcnt ? 1 : 0);

}


extern char *Sflags[];

/*	get(file)	Modified by T.O on 85/7/5	*/
_get(file)
char *file;
{
	register char *p;
	register unsigned ser;
	extern char had_dir, had_standinp;
	struct stats stats;
	struct idel *dodelt();
	char	str[32];
	char *idsubst(), *auxf();
	int i,status,holduid,holdgid;

	Fflags |= FTLMSG;
	if (setjmp(Fjmp))
		return;
	if (HADE) {
		had_pfile = 1;
		/*
		call `sinit' to check if file is an SCCS file
		but don't open the SCCS file.
		If it is, then create lock file.
		*/
		sinit(&gpkt,file,0);
		if (lockit(auxf(file,'z'),10,getpid()))
		{
			sprintf(Error,"%s (cm4)", nl_msg(10,"cannot create lock file"));
			fatal(Error);
		}
	}
	/*
	Open SCCS file and initialize packet
	*/
	sinit(&gpkt,file,1);
	gpkt.p_ixuser = (HADI | HADX);
	gpkt.p_reqsid.s_rel = sid.s_rel;
	gpkt.p_reqsid.s_lev = sid.s_lev;
	gpkt.p_reqsid.s_br = sid.s_br;
	gpkt.p_reqsid.s_seq = sid.s_seq;
	gpkt.p_verbose = verbosity;
	gpkt.p_stdout = (HADP ? stderr : stdout);
	gpkt.p_cutoff = cutoff;
	gpkt.p_lfile = lfile;
	copy(auxf(gpkt.p_file,'g'),Gfile);

	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);
	finduser(&gpkt);
	doflags(&gpkt);
	if (!HADA)
		ser = getser(&gpkt);
	else {
		if ((ser = Ser) > maxser(&gpkt))
		{
			sprintf(Error,"%s (ge19)",  nl_msg(11,"serial number too large"));
			fatal(Error);
		}
		gpkt.p_gotsid = gpkt.p_idel[ser].i_sid;
		if (HADR && sid.s_rel != gpkt.p_gotsid.s_rel) {
			zero(&gpkt.p_reqsid, sizeof(gpkt.p_reqsid));
			gpkt.p_reqsid.s_rel = sid.s_rel;
		}
		else
			gpkt.p_reqsid = gpkt.p_gotsid;
	}
	doie(&gpkt,ilist,elist,(char *) 0);
	setup(&gpkt,(int) ser);
	if (!(Type = Sflags[TYPEFLAG - 'a']))
		Type = Null;
	if (!(HADP || HADG))
		if (exists(Gfile) && (S_IWRITE & Statbuf.st_mode)) {
			sprintf(Error,nl_msg(12,"writable `%s' exists"),Gfile);
			fatal(strcat(Error," (ge4)"));
		}
	if (gpkt.p_verbose) {
		sid_ba(&gpkt.p_gotsid,str);
		fprintf(gpkt.p_stdout,"Retrieved:\n%s\n",str);
	}
	if (HADE) {
		if (HADC || gpkt.p_reqsid.s_rel == 0)
			gpkt.p_reqsid = gpkt.p_gotsid;
		newsid(&gpkt,Sflags[BRCHFLAG - 'a'] && HADB);
		permiss(&gpkt);
		if (exists(auxf(gpkt.p_file,'p')))
		    {
/* ATT local fix. Should be checked against effective user id.*/
		    if(Statbuf.st_uid != getuid() && Statbuf.st_uid !=geteuid())
		       { /* If user does not own P-file, avoid future errors */
		       sprintf( Error, nl_msg(26,"being edited") );
		       fatal( strcat( Error," (ge17)" ) );
		       }
		    mk_qfile(&gpkt);
		    }
		else 
		    had_pfile = 0;
		wrtpfile(&gpkt,ilist,elist);
	}
	if (!HADG || HADL) {
		if (gpkt.p_stdout)
			fflush(gpkt.p_stdout);
		holduid=geteuid();
		holdgid=getegid();
		setuid(getuid());
		setgid(getgid());
		if (HADL)
			gen_lfile(&gpkt);
		if (HADG)
			exit(0);
		flushto(&gpkt,EUSERTXT,1);
		idsetup(&gpkt);
		gpkt.p_chkeof = 1;
		Did_id = 0;
		/*
		call `xcreate' which deletes the old `g-file' and
		creates a new one except if the `p' keyletter is set in
		which case any old `g-file' is not disturbed.
		The mod of the file depends on whether or not the `k'
		keyletter has been set.
		*/

		if (gpkt.p_gout == 0) {
			if (HADP)
				gpkt.p_gout = stdout;
			else
				gpkt.p_gout = xfcreat(Gfile,HADK ? 0644 : 0444);
		}
		while(readmod(&gpkt)) {
			prfx(&gpkt);
			p = idsubst(&gpkt,gpkt.p_line);
			if(fputs(p,gpkt.p_gout)==EOF)
				FAILPUT;
		}
		if (gpkt.p_gout)
			fflush(gpkt.p_gout);
		if (gpkt.p_gout && gpkt.p_gout != stdout)
			fclose(gpkt.p_gout);
		if (gpkt.p_verbose)
		{
			fprintf(gpkt.p_stdout,nl_msg(13,"%u lines"),gpkt.p_glnno);
			fprintf(gpkt.p_stdout,"\n");
		}
		if (!Did_id && !HADK)
			if (Sflags[IDFLAG - 'a'])
				if(!(*Sflags[IDFLAG - 'a']))
				{
					sprintf(Error,"%s (cm6)",nl_msg(14,"no id keywords"));
					fatal(Error);
				}
				else
				{
					sprintf(Error,"%s (cm10)",nl_msg(15,"invalid id keywords"));
					fatal(Error);
				}
			else if (gpkt.p_verbose)
				fprintf(stderr,"%s (cm7)\n", (nl_msg(16,"No id keywords")));
		setuid(holduid);
		setgid(holdgid);
	}
	if (gpkt.p_iop)
		fclose(gpkt.p_iop);

	/*
	remove 'q'-file because it is no longer needed
	*/
	unlink(auxf(gpkt.p_file,'q'));
	ffreeall();
	unlockit(auxf(gpkt.p_file,'z'),getpid());
}


newsid(pkt,branch)
register struct packet *pkt;
int branch;
{
	int chkbr;

	chkbr = 0;
	/* if branch value is 0 set newsid level to 1 */
	if (pkt->p_reqsid.s_br == 0) {
		pkt->p_reqsid.s_lev += 1;
		/*
		if the sid requested has been deltaed or the branch
		flag was set or the requested SID exists in the p-file
		then create a branch delta off of the gotten SID
		*/
		if (sidtoser(&pkt->p_reqsid,pkt) ||
			pkt->p_maxr > pkt->p_reqsid.s_rel || branch ||
			in_pfile(&pkt->p_reqsid,pkt)) {
				pkt->p_reqsid.s_rel = pkt->p_gotsid.s_rel;
				pkt->p_reqsid.s_lev = pkt->p_gotsid.s_lev;
				pkt->p_reqsid.s_br = pkt->p_gotsid.s_br + 1;
				pkt->p_reqsid.s_seq = 1;
				chkbr++;
		}
	}
	/*
	if a three component SID was given as the -r argument value
	and the b flag is not set then up the gotten SID sequence
	number by 1
	*/
	else if (pkt->p_reqsid.s_seq == 0 && !branch)
		pkt->p_reqsid.s_seq = pkt->p_gotsid.s_seq + 1;
	else {
		/*
		if sequence number is non-zero then increment the
		requested SID sequence number by 1
		*/
		pkt->p_reqsid.s_seq += 1;
		if (branch || sidtoser(&pkt->p_reqsid,pkt) ||
			in_pfile(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			pkt->p_reqsid.s_seq = 1;
			chkbr++;
		}
	}
	/*
	keep checking the requested SID until a good SID to be
	made is calculated or all possibilities have been tried
	*/
	while (chkbr) {
		--chkbr;
		while (in_pfile(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			++chkbr;
		}
		while (sidtoser(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			++chkbr;
		}
	}
	if (sidtoser(&pkt->p_reqsid,pkt) || in_pfile(&pkt->p_reqsid,pkt))
		fatal((nl_msg(17,"bad SID calculated in newsid()")));
}


enter(pkt,ch,n,sidp)
struct packet *pkt;
char ch;
int n;
struct sid *sidp;
{
	char str[32];
	register struct apply *ap;

	sid_ba(sidp,str);
	if (pkt->p_verbose)
		fprintf(pkt->p_stdout,"%s\n",str);
	ap = &pkt->p_apply[n];
	switch(ap->a_code) {

	case SX_EMPTY:
		if (ch == INCLUDE)
			condset(ap,APPLY,INCLUSER);
		else
			condset(ap,NOAPPLY,EXCLUSER);
		break;
	case APPLY:
		sid_ba(sidp,str);
		sprintf(Error,nl_msg(18,"%s already included"),str);
		fatal(strcat(Error," (ge9)"));
		break;
	case NOAPPLY:
		sid_ba(sidp,str);
		sprintf(Error,nl_msg(19,"%s already excluded"),str);
		fatal(strcat(Error," (ge10)"));
		break;
	default:
		sprintf(Error,"%s (ge11)",  nl_msg(20,"internal error in get/enter()"));
		fatal(Error);
		break;
	}
}


gen_lfile(pkt)
register struct packet *pkt;
{
	char *n;
	int reason;
	char str[32];
	char line[BUFSIZ];
	struct deltab dt;
	FILE *in;
	FILE *out;

	in = xfopen(pkt->p_file,0);
	if (*pkt->p_lfile)
		out = stdout;
	else
		out = xfcreat(auxf(pkt->p_file,'l'),0444);
	fgets(line,sizeof(line),in);
	while (fgets(line,sizeof(line),in) != NULL && line[0] == CTLCHAR && line[1] == STATS) {
		fgets(line,sizeof(line),in);
		del_ab(line,&dt,pkt);
		if (dt.d_type == 'D') {
			reason = pkt->p_apply[dt.d_serial].a_reason;
			if (pkt->p_apply[dt.d_serial].a_code == APPLY) {
				putc(' ',out);
				putc(' ',out);
			}
			else {
				putc('*',out);
				if (reason & IGNR)
					putc(' ',out);
				else
					putc('*',out);
			}
			switch (reason & (INCL | EXCL | CUTOFF)) {
	
			case INCL:
				putc('I',out);
				break;
			case EXCL:
				putc('X',out);
				break;
			case CUTOFF:
				putc('C',out);
				break;
			default:
				putc(' ',out);
				break;
			}
			putc(' ',out);
			sid_ba(&dt.d_sid,str);
			fprintf(out,"%s\t",str);
			date_ba(&dt.d_datetime,str);
			fprintf(out,"%s %s\n",str,dt.d_pgmr);
		}
		while ((n = fgets(line,sizeof(line),in)) != NULL)
			if (line[0] != CTLCHAR)
				break;
			else {
				switch (line[1]) {

				case EDELTAB:
					break;
				default:
					continue;
				case MRNUM:
				case COMMENTS:
					if (dt.d_type == 'D')
						fprintf(out,"\t%s",&line[3]);
					continue;
				}
				break;
			}
		if (n == NULL || line[0] != CTLCHAR)
			break;
		putc('\n',out);
	}
	fclose(in);
	if (out != stdout)
		fclose(out);
}

#ifdef NLS
char	Curdate[64];
char	Curtime[64];
char	Gdate[64];
char	Chgdate[64];
char	Chgtime[64];
char	Gchgdate[64];
#else
char	Curdate[18];
char	*Curtime;
char	Gdate[9];
char	Chgdate[18];
char	*Chgtime;
char	Gchgdate[9];
#endif
char	Sid[32];
char	Mod[16];
char	Olddir[BUFSIZ];
char	Pname[BUFSIZ];
char	Dir[BUFSIZ];
char	*Qsect;

idsetup(pkt)
register struct packet *pkt;
{
	extern long Timenow;
	register int n;
	register char *p;
#ifdef NLS
	strcpy(Curdate , repl((nl_cxtime(&Timenow,nl_msg(101,"%y/%m/%d"))),'\n','\0'));
	strcpy(Gdate , repl((nl_cxtime(&Timenow,nl_msg(102,"%m/%d/%y"))),'\n','\0'));
	strcpy(Curtime , repl((nl_cxtime(&Timenow,nl_msg(103,"%H:%M:%S"))),'\n','\0'));
#else
	date_ba(&Timenow,Curdate);
	Curtime = &Curdate[9];
	Curdate[8] = 0;
	makgdate(Curdate,Gdate);
#endif
	for (n = maxser(pkt); n; n--)
		if (pkt->p_apply[n].a_code == APPLY)
			break;
	if (n)
#ifdef NLS
	strcpy(Chgdate , repl((nl_cxtime(&pkt->p_idel[n].i_datetime,nl_msg(104,"%y/%m/%d"))),'\n','\0'));
	strcpy(Gchgdate , repl((nl_cxtime(&pkt->p_idel[n].i_datetime,nl_msg(105,"%m/%d/%y"))),'\n','\0'));
	strcpy(Chgtime , repl((nl_cxtime(&pkt->p_idel[n].i_datetime,nl_msg(106,"%H:%M:%S"))),'\n','\0'));
#else
		date_ba(&pkt->p_idel[n].i_datetime,Chgdate);
	Chgtime = &Chgdate[9];
	Chgdate[8] = 0;
	makgdate(Chgdate,Gchgdate);
#endif
	sid_ba(&pkt->p_gotsid,Sid);
	if (p = Sflags[MODFLAG - 'a'])
		copy(p,Mod);
	else
		copy(Gfile,Mod);
	if (!(Qsect = Sflags[QSECTFLAG - 'a']))
		Qsect = Null;
}


#ifndef NLS
makgdate(old,new)
register char *old, *new;
{
	if ((*new = old[3]) != '0')
		new++;
	*new++ = old[4];
	*new++ = '/';
	if ((*new = old[6]) != '0')
		new++;
	*new++ = old[7];
	*new++ = '/';
	*new++ = old[0];
	*new++ = old[1];
	*new = 0;
}
#endif NLS


static char Zkeywd[5] = "@(#)";

char *
idsubst(pkt,line)
register struct packet *pkt;
char line[];
{
	static char tline[BUFSIZ];
	char hold[BUFSIZ];
	static char str[32];
	register unsigned char *lp, *tp;
	char *trans();
	int recursive = 0;
	extern char *Type;
	extern char *Sflags[];

	if (HADK || !any('%',line))
		return(line);

	tp = tline;
	for(lp=line; *lp != 0; lp++) {
#ifdef NLS16
		if (FIRSTof2(*lp) && SECof2(*(lp+1))) {
			*tp++ = *lp++;
			goto skip;
		}
#endif NLS16
		if(lp[0] == '%' && lp[1] != 0 && lp[2] == '%') {
			if((!Did_id) && (Sflags['i'-'a']) &&
				(!(strncmp(Sflags['i'-'a'],lp,strlen(Sflags['i'-'a'])))))
					++Did_id;
			switch(*++lp) {

			case 'M':
				tp = trans(tp,Mod);
				break;
			case 'Q':
				tp = trans(tp,Qsect);
				break;
			case 'R':
				sprintf(str,"%u",pkt->p_gotsid.s_rel);
				tp = trans(tp,str);
				break;
			case 'L':
				sprintf(str,"%u",pkt->p_gotsid.s_lev);
				tp = trans(tp,str);
				break;
			case 'B':
				sprintf(str,"%u",pkt->p_gotsid.s_br);
				tp = trans(tp,str);
				break;
			case 'S':
				sprintf(str,"%u",pkt->p_gotsid.s_seq);
				tp = trans(tp,str);
				break;
			case 'D':
				tp = trans(tp,Curdate);
				break;
			case 'H':
				tp = trans(tp,Gdate);
				break;
			case 'T':
				tp = trans(tp,Curtime);
				break;
			case 'E':
				tp = trans(tp,Chgdate);
				break;
			case 'G':
				tp = trans(tp,Gchgdate);
				break;
			case 'U':
				tp = trans(tp,Chgtime);
				break;
			case 'Z':
				tp = trans(tp,Zkeywd);
				break;
			case 'Y':
				tp = trans(tp,Type);
				break;
			case 'W':
				if((Whatstr[0] != NULL) && (!recursive)) {
					recursive = 1;
					lp += 2;
					strcpy(hold,Whatstr);
					strcat(hold,lp);
					lp = hold;
					lp--;
					continue;
				}
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Mod);
				*tp++ = '\t';
			case 'I':
				tp = trans(tp,Sid);
				break;
			case 'P':
				copy(pkt->p_file,Dir);
				dname(Dir);
				if ( getcwd( Olddir, BUFSIZ ) == (char *) 0 )
				{
					sprintf(Error,"%s (ge20)", nl_msg(21,"getcwd failed"));
					fatal(Error);
				}
				if(chdir(Dir) != 0)
				{
					sprintf(Error,"%s (ge21)", nl_msg(22,"cannot change directory"));
					fatal(Error);
				}
				if ( getcwd( Pname, BUFSIZ ) == (char *) 0 )
				{
					sprintf(Error,"%s (ge20)", nl_msg(21,"getcwd failed"));
					fatal(Error);
				}
				if(chdir(Olddir) != 0)
				{
					sprintf(Error,"%s (ge21)", nl_msg(22,"cannot change directory"));
					fatal(Error);
				}
				tp = trans(tp,Pname);
				*tp++ = '/';
				tp = trans(tp,(sname(pkt->p_file)));
				break;
			case 'F':
				tp = trans(tp,pkt->p_file);
				break;
			case 'C':
				sprintf(str,"%u",pkt->p_glnno);
				tp = trans(tp,str);
				break;
			case 'A':
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Type);
				*tp++ = ' ';
				tp = trans(tp,Mod);
				*tp++ = ' ';
				tp = trans(tp,Sid);
				tp = trans(tp,Zkeywd);
				break;
			default:
				*tp++ = '%';
				*tp++ = *lp;
#ifdef NLS16			/* eat the closing '%' if this was a kanji
				   with its 2nd byte being a '%'           */
				if (FIRSTof2(*lp) && SECof2('%'))
					*tp++ = *++lp;
#endif NLS16
				continue;
			}
			if (!(Sflags['i'-'a']))
				++Did_id;
			lp++;
		}
		else
skip:			*tp++ = *lp;
	}

	*tp = 0;
	return(tline);
}


char *
trans(tp,str)
register char *tp, *str;
{
	while(*tp++ = *str++)
		;
	return(tp-1);
}


prfx(pkt)
register struct packet *pkt;
{
	char str[32];

	if (HADN)
		fprintf(pkt->p_gout,"%s\t",Mod);
	if (HADM) {
		sid_ba(&pkt->p_inssid,str);
		fprintf(pkt->p_gout,"%s\t",str);
	}
}


clean_up(n)
{
	/*
	clean_up is only called from fatal() upon bad termination.
	*/
	if (gpkt.p_iop)
		fclose(gpkt.p_iop);
	if (gpkt.p_gout)
		fflush(gpkt.p_gout);
	if (gpkt.p_gout && gpkt.p_gout != stdout) {
		fclose(gpkt.p_gout);
		unlink(Gfile);
	}
	if (HADE) {
		if (! had_pfile) {
			unlink(auxf(gpkt.p_file,'p'));
		}
		else if (exists(auxf(gpkt.p_file,'q'))) {
			copy(auxf(gpkt.p_file,'p'),Pfilename);
			rename(auxf(gpkt.p_file,'q'),Pfilename);
		     }
	}
	/*
	** Added in response to DTS bug DSDe600744
	** If there is a fault, remove the l. file if it
	**  was generated.
	*/
	if ( HADL )
	  {
	  if ( exists(auxf(gpkt.p_file,'l')) )
	    {
	      unlink( auxf(gpkt.p_file,'l') );
	    }
	  }
	ffreeall();
	unlockit(auxf(gpkt.p_file,'z'),getpid());
}


static	char	warn[] = "WARNING: being edited: `%s'";

wrtpfile(pkt,inc,exc)
register struct packet *pkt;
char *inc, *exc;
{
	char line[64], str1[32], str2[32];
	char *user;
	FILE *in, *out;
	struct pfile pf;
	register char *p;
	int fd;
	extern long Timenow;

	user = logname();
	if (exists(p = auxf(pkt->p_file,'p'))) {
		fd = xopen(p,2);
		in = fdfopen(fd,0);
		while (fgets(line,sizeof(line),in) != NULL) {
			p = line;
			p[length(p) - 1] = 0;
			pf_ab(p,&pf,0);
			if (!(Sflags[JOINTFLAG - 'a'])) {
				if ((pf.pf_gsid.s_rel == pkt->p_gotsid.s_rel &&
     				   pf.pf_gsid.s_lev == pkt->p_gotsid.s_lev &&
				   pf.pf_gsid.s_br == pkt->p_gotsid.s_br &&
				   pf.pf_gsid.s_seq == pkt->p_gotsid.s_seq) ||
				   (pf.pf_nsid.s_rel == pkt->p_reqsid.s_rel &&
				   pf.pf_nsid.s_lev == pkt->p_reqsid.s_lev &&
				   pf.pf_nsid.s_br == pkt->p_reqsid.s_br &&
				   pf.pf_nsid.s_seq == pkt->p_reqsid.s_seq)) {
					fclose(in);
					sprintf(Error,
					     nl_msg(26,"being edited: `%s'"),line);
					fatal(strcat(Error," (ge17)"));
				}
				if (!equal(pf.pf_user,user))
					{
					fprintf(stderr,(nl_msg(25,warn)),line);
					fprintf(stderr," (ge18)\n");
					}
			}
			else
			{
			fprintf(stderr,(nl_msg(25,warn)),line);
			fprintf(stderr," (ge18)\n");
			}
		}
		out = fdfopen(dup(fd),1);
		fclose(in);
	}
	else
		out = xfcreat(p,0644);
	fseek(out,0L,2);
	sid_ba(&pkt->p_gotsid,str1);
	sid_ba(&pkt->p_reqsid,str2);
	date_ba(&Timenow,line);
	fprintf(out,"%s %s %s %s",str1,str2,user,line);
	if (inc)
		fprintf(out," -i%s",inc);
	if (exc)
		fprintf(out," -x%s",exc);
	if (cmrinsert () > 0)	/* if there are CMRS and they are okay */
		fprintf (out, " -z%s", cmr);
	fprintf(out,"\n");
	fclose(out);
	if (pkt->p_verbose)
	{
		fprintf(pkt->p_stdout,nl_msg(27,"new delta"));
		fprintf(pkt->p_stdout," %s\n",str2);
	}
}


getser(pkt)
register struct packet *pkt;
{
	register struct idel *rdp;
	int n, ser, def;
	char *p;
	extern char *Sflags[];

	def = 0;
	if (pkt->p_reqsid.s_rel == 0) {
		if (p = Sflags[DEFTFLAG - 'a'])
			chksid(sid_ab(p, &pkt->p_reqsid), &pkt->p_reqsid);
		else {
			pkt->p_reqsid.s_rel = MAX;
			def = 1;
		}
	}
	ser = 0;
	if (pkt->p_reqsid.s_lev == 0) {
		for (n = maxser(pkt); n; n--) {
			rdp = &pkt->p_idel[n];
			if ((rdp->i_sid.s_br == 0 || HADT) &&
				pkt->p_reqsid.s_rel >= rdp->i_sid.s_rel &&
				rdp->i_sid.s_rel > pkt->p_gotsid.s_rel) {
					ser = n;
					pkt->p_gotsid.s_rel = rdp->i_sid.s_rel;
			}
		}
	}
	/*
	 * If had '-t' keyletter and R.L SID type, find
	 * the youngest SID
	*/
	else if ((pkt->p_reqsid.s_br == 0) && HADT) {
		for (n = maxser(pkt); n; n--) {
			rdp = &pkt->p_idel[n];
			if (rdp->i_sid.s_rel == pkt->p_reqsid.s_rel &&
			    rdp->i_sid.s_lev == pkt->p_reqsid.s_lev )
				break;
		}
		ser = n;
	}
	else if (pkt->p_reqsid.s_br && pkt->p_reqsid.s_seq == 0) {
		for (n = maxser(pkt); n; n--) {
			rdp = &pkt->p_idel[n];
			if (rdp->i_sid.s_rel == pkt->p_reqsid.s_rel &&
				rdp->i_sid.s_lev == pkt->p_reqsid.s_lev &&
				rdp->i_sid.s_br == pkt->p_reqsid.s_br)
					break;
		}
		ser = n;
	}
	else {
		ser = sidtoser(&pkt->p_reqsid,pkt);
	}
	if (ser == 0)
	{
		sprintf(Error,"%s (ge5)",nl_msg(28,"nonexistent sid"));
		fatal(Error);
	}
	rdp = &pkt->p_idel[ser];
	pkt->p_gotsid = rdp->i_sid;
	if (def || (pkt->p_reqsid.s_lev == 0 && pkt->p_reqsid.s_rel == pkt->p_gotsid.s_rel))
		pkt->p_reqsid = pkt->p_gotsid;
	return(ser);
}


/* Null routine to satisfy external reference from dodelt() */

escdodelt()
{
}
/* NULL routine to satisfy external reference from dodelt() */
fredck()
{
}

in_pfile(sp,pkt)
struct	sid	*sp;
struct	packet	*pkt;
{
	struct	pfile	pf;
	char	line[BUFSIZ];
	char	*p;
	FILE	*in;

	if (Sflags[JOINTFLAG - 'a']) {
		if (exists(auxf(pkt->p_file,'p'))) {
			in = xfopen(auxf(pkt->p_file,'p'),0);
			while ((p = fgets(line,sizeof(line),in)) != NULL) {
				p[length(p) - 1] = 0;
				pf_ab(p,&pf,0);
				if (pf.pf_nsid.s_rel == sp->s_rel &&
					pf.pf_nsid.s_lev == sp->s_lev &&
					pf.pf_nsid.s_br == sp->s_br &&
					pf.pf_nsid.s_seq == sp->s_seq) {
						fclose(in);
						return(1);
				}
			}
			fclose(in);
		}
		else return(0);
	}
	else return(0);
}


mk_qfile(pkt)
register struct	packet *pkt;
{
	FILE	*in, *qout;
	char	line[BUFSIZ];

	in = xfopen(auxf(pkt->p_file,'p'),0);
	qout = xfcreat(auxf(pkt->p_file,'q'),0644);

	while ((fgets(line,sizeof(line),in) != NULL))
		if(fputs(line,qout)==EOF)
			FAILPUT;
	fclose(in);
	fclose(qout);
}

/* cmrinsert -- insert CMR numbers in the p.file. */

cmrinsert ()
{
	extern char *strrchr (), *Sflags[];
	extern int	cmrcheck ();
	char holdcmr[CMRLIMIT];
	char tcmr[CMRLIMIT];
	char *p;
	int bad;
	int valid;


	if (Sflags[CMFFLAG - 'a'] == 0)		/* CMFFLAG was not set. */
		return (0);

	if ( HADP && ( ! HADZ))	/* no CMFFLAG and no place to prompt. */
	{
		sprintf(Error,"%s\n",nl_msg(29,"Background CASSI get with no CMRs"));
		fatal(Error);
	}

retry:
	if (cmr[0] == NULL) {					/* No CMR list.  Make one. */
		if(HADZ && ((!isatty(0)) || (!isatty(1))))
		{
			sprintf(Error,"%s\n",nl_msg(30,"Background CASSI get with invalid CMR"));
			fatal(Error);
		}
		fprintf (stdout,"%s ", (nl_msg(31,"Input Comma Separated List of CMRs:")));
		fgets (cmr, CMRLIMIT, stdin);
		p=strend(cmr);
		*(--p) = NULL;
		if ((int)(p - cmr) == CMRLIMIT) {
			fprintf (stdout,"%s\n",(nl_msg(32,"?Too many CMRs.")));
			cmr[0] = NULL;
			goto retry;					/* Entry was too long. */
			}
		}

	/* Now, check the comma seperated list of CMRs for accuracy. */

	bad = 0;
	valid = 0;
	strcpy(tcmr,cmr);
	while(p=strrchr(tcmr,',')) {
		++p;
		if(cmrcheck(p,Sflags[CMFFLAG - 'a']))
			++bad;
		else {
			++valid;
			strcat(holdcmr,",");
			strcat(holdcmr,p);
		}
		*(--p) = NULL;
	}
	if(*tcmr)
		if(cmrcheck(tcmr,Sflags[CMFFLAG - 'a']))
			++bad;
		else {
			++valid;
			strcat(holdcmr,",");
			strcat(holdcmr,tcmr);
		}

	if(!bad && holdcmr[1]) {
		strcpy(cmr,holdcmr+1);
		return(1);
	}
	else {
		if((isatty(0)) && (isatty(1)))
			if(!valid)
				fprintf(stdout,"%s\n", (nl_msg(33,"Must enter at least one valid CMR.")));
			else
				fprintf(stdout,"%s\n", (nl_msg(34,"Re-enter invalid CMRs, or press return.")));
		cmr[0] = NULL;
		goto retry;
	}
}
