/* @(#) $Revision: 64.5 $ */     
static char *HPUX_ID = "@(#) $Revision: 64.5 $";
#ifdef NLS
#define NL_SETN 3
#include	<msgbuf.h>
#include	<locale.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	"../hdr/defines.h"
# include	"../hdr/had.h"

struct stat Statbuf;
char Null[1];
char Error[128];

#define DIFF   "/bin/diff"
#define BDIFF  "/usr/bin/bdiff"
char	Diffpgm[80];
FILE	*Diffin;
FILE	*Gin;
int	Debug = 0;
struct packet gpkt;
struct sid sid;
int	num_files;
char	had[26];
char	*ilist, *elist, *glist;
char	*Comments,Cmrs[300],*Mrs,*Nsid;
char	*auxf(), *logname(), *sid_ba();
int	Domrs;
int verbosity;
int	Did_id;
long	Szqfile;
char	Pfilename[FILESIZE];
FILE	 *fdfopen();
char *Cassin;
extern FILE	*Xiop;
extern int	Xcreate;

main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	char c;
	char *sid_ab();
	int testmore;
	extern delta();
	extern	char	*savecmt();
	extern int Fcnt;

#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("delta"), stderr);
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

			case 'r':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				chksid(sid_ab(p,&sid),&sid);
				break;
			case 'g':
				glist = p;
				break;
			case 'y':
				Comments = p;
				break;
			case 'm':
				Mrs = p;
				repl(Mrs,'\n',' ');
				break;
			case 'p':
			case 'n':
			case 's':
				testmore++;
				break;
			case 'z':
				Cassin = p;
				break;
			default:
				sprintf(Error,"%s (cm1)", nl_msg(1,"unknown key letter"));
				fatal(Error);
			}

			if (testmore) {
				testmore = 0;
				if (*p) {
					sprintf(Error,
						nl_msg(2,"value after %c arg"),c);
					fatal(strcat(Error," (cm7)"));
				}
			}
			if (had[c - 'a']++)
			{
				sprintf(Error,"%s (cm2)", nl_msg(3,"key letter twice"));
				fatal(Error);
			}
			argv[i] = 0;
		}
		else num_files++;

	if(num_files == 0)
	{
		sprintf(Error,"%s (cm3)", nl_msg(4,"missing file arg"));
		fatal(Error);
	}
	if (!HADS)
		verbosity = -1;
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if (p=argv[i])
			do_file(p,delta);
	exit(Fcnt ? 1 : 0);

}


delta(file)
char *file;
{
	static int first = 1;
	int n, linenum;
	char type;
	register int ser;
	extern char had_dir, had_standinp;
	extern char *Sflags[];
	char nsid[50];
	char dfilename[FILESIZE];
	char gfilename[FILESIZE];
	char line[BUFSIZ];
	char *getline();
	FILE  *dodiff();
	struct stats stats;
	struct pfile *pp, *rdpfile();
	struct stat sbuf;
	struct idel *dodelt();
	int inserted, deleted, orig;
	int newser;
	int status = 0, i;
	int diffloop;
	int difflim;
	int holduid,holdgid;
	if (setjmp(Fjmp))
		return;
	sinit(&gpkt,file,1);
	if (first) {
		first = 0;
		dohist(file);
	}
	if (lockit(auxf(gpkt.p_file,'z'),2,getpid()))
	{
		sprintf(Error,"%s (cm4)", nl_msg(5,"cannot create lock file"));
		fatal(Error);
	}
	gpkt.p_reopen = 1;
	gpkt.p_stdout = stdout;
	copy(auxf(gpkt.p_file,'g'),gfilename);
	Gin = xfopen(gfilename,0);
	pp = rdpfile(&gpkt,&sid);
	strcpy(Cmrs,pp->pf_cmrlist);
	if(!pp->pf_nsid.s_br)
		{
		 sprintf(nsid,"%d.%d",pp->pf_nsid.s_rel,pp->pf_nsid.s_lev);
		}
	else
		{
			sprintf(nsid,"%d.%d.%d.%d",pp->pf_nsid.s_rel,pp->pf_nsid.s_lev,pp->pf_nsid.s_br,pp->pf_nsid.s_seq);
		}
	Nsid=nsid;
	gpkt.p_cutoff = pp->pf_date;
	ilist = pp->pf_ilist;
	elist = pp->pf_elist;

	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);
	if ((ser = sidtoser(&pp->pf_gsid,&gpkt)) == 0 ||
		sidtoser(&pp->pf_nsid,&gpkt))
		{
			sprintf(Error,"%s (de3)", nl_msg(6,"invalid sid in p-file"));
			fatal(Error);
		}
	doie(&gpkt,ilist,elist,glist);
	setup(&gpkt,ser);
	finduser(&gpkt);
	doflags(&gpkt);
	gpkt.p_reqsid = pp->pf_nsid;
	permiss(&gpkt);
	flushto(&gpkt,EUSERTXT,1);
	gpkt.p_chkeof = 1;
	copy(auxf(gpkt.p_file,'d'),dfilename);
	gpkt.p_gout = xfcreat(dfilename,0444);
	while(readmod(&gpkt)) {
		chkid(gpkt.p_line,Sflags['i'-'a']);
		if(fputs(gpkt.p_line,gpkt.p_gout)==EOF)
			FAILPUT;
	}
	fclose(gpkt.p_gout);
	orig = gpkt.p_glnno;
	gpkt.p_glnno = 0;
	gpkt.p_verbose = verbosity;
	Did_id = 0;
	while (fgets(line,sizeof(line),Gin) != NULL &&
			 !chkid(line,Sflags['i'-'a']))
		;
	fclose(Gin);
	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	if (!Did_id)
		if (Sflags[IDFLAG - 'a'])
			if(!(*Sflags[IDFLAG - 'a']))
			{
				sprintf(Error,"%s (cm6)", nl_msg(7,"no id keywords"));
				fatal(Error);
			}
			else
			{
				sprintf(Error,"%s (cm10)", nl_msg(8,"invalid id keywords"));
				fatal(Error);
			}
		else if (gpkt.p_verbose)
			fprintf(stderr,"%s (cm7)\n",(nl_msg(9,"No id keywords")));

	/*
	The following while loop executes 'bdiff' on g-file and
	d-file. If 'bdiff' fails (usually because segmentation
	limit it is using is too large for 'diff'), it is
	invoked again, with a lower segmentation limit.
	*/
	difflim = 4000;
	diffloop = 0;
	while (1) {
		inserted = deleted = 0;
		gpkt.p_glnno = 0;
		gpkt.p_upd = 1;
		gpkt.p_wrttn = 1;
		getline(&gpkt);
		gpkt.p_wrttn = 1;
		newser = mkdelt(&gpkt,&pp->pf_nsid,&pp->pf_gsid,
						diffloop,orig);
		flushto(&gpkt,EUSERTXT,0);
		Diffin = dodiff(auxf(gpkt.p_file,'g'),
				dfilename, difflim, diffloop);

		while (n = getdiff(&type,&linenum)) {
			if (type == INS) {
				inserted += n;
				insert(&gpkt,linenum,n,newser);
			}
			else if (type == DEL) {
				deleted += n;
				delete(&gpkt,linenum,n,newser);
			}
		}
		fclose(Diffin);
		if (gpkt.p_iop)
			while (readmod(&gpkt))
				;
#ifdef debug
		/* Use fprintf because child uses the stdin/stdout pair */
		fprintf(stderr,"going to wait --- this is the parent\n");
#endif
		status = 0;
		wait(&status);
#ifdef debug
	fprintf(stderr,"return of wait = %d\n",status);
#endif
		/* First run through attempts to use diff & diff returns 1 */
		if ( !diffloop && (status == 256) ) status = 0;
		if (status) {		/* diff failed */
			/*
			Check top byte (exit code of child).
			*/
			if (((status >> 8) & 0377) == 32) { /* 'execl' failed */
				sprintf(Error,
					nl_msg(12,"cannot execute '%s'"),Diffpgm);
				fatal(strcat(Error," (de12)"));
			}
			/*
			Re-try.
			*/
			if (difflim -= 500) {	/* reduce segmentation */
#ifdef NLS 
				fprintmsg(stderr,
					nl_msg(13,"'%1$s' failed, re-trying, segmentation = %2$d"),
					Diffpgm,difflim);
				fprintf(stderr," (de13)\n");
#else
				fprintf(stderr,
			"'%s' failed, re-trying, segmentation = %d (de13)\n",
					Diffpgm,difflim);
#endif  
				/*    (set up)
				** If Xiop is being closed, make sure the return is
				**  0, or the final write may have failed.
				*/
				if ( fclose(Xiop) != 0 )
				  {
				    xmsg( gpkt.p_file, "delta" );
				    exit(1);
				  }
				Xiop = 0;	/* for new x-file */
				Xcreate = 0;
				/*
				Re-open s-file.
				*/
				gpkt.p_iop = xfopen(gpkt.p_file,0);
				setbuf(gpkt.p_iop,gpkt.p_buf);
				/*
				Reset counters.
				*/
				gpkt.p_slnno = 0;
				gpkt.p_ihash = 0;
				gpkt.p_chash = 0;
				gpkt.p_nhash = 0;
				gpkt.p_keep = 0;
			}
			else
			{
				/* tried up to 500 lines, can't go on */
				sprintf(Error,"%s (de4)", nl_msg(14,"diff failed"));
				fatal(Error);
			}
		}
		else {		/* diff succeeded */
			break;			/* exit while loop */
		}
	diffloop = 1;
	}
	fgetchk(gfilename,&gpkt);
	unlink(dfilename);
	stats.s_ins = inserted;
	stats.s_del = deleted;
	stats.s_unc = orig - deleted;
	if (gpkt.p_verbose) {
		fprintf(gpkt.p_stdout,nl_msg(15,"%u inserted"),stats.s_ins);
		fprintf(gpkt.p_stdout,"\n");
		fprintf(gpkt.p_stdout,nl_msg(16,"%u deleted"),stats.s_del);
		fprintf(gpkt.p_stdout,"\n");
		fprintf(gpkt.p_stdout,nl_msg(17,"%u unchanged"),stats.s_unc);
		fprintf(gpkt.p_stdout,"\n");
	}
	flushline(&gpkt,&stats);
	stat(gpkt.p_file,&sbuf);
	rename(auxf(gpkt.p_file,'x'),gpkt.p_file);
	chmod(gpkt.p_file,sbuf.st_mode);
	chown(gpkt.p_file,sbuf.st_uid,sbuf.st_gid);
	if (Szqfile)
		rename(auxf(gpkt.p_file,'q'),Pfilename);
	else {
		xunlink(Pfilename);
		xunlink(auxf(gpkt.p_file,'q'));
	}
	clean_up(0);
	if (!HADN) {
		fflush(gpkt.p_stdout);
		holduid=geteuid();
		holdgid=getegid();
		setuid(getuid());
		setgid(getgid());
		unlink(gfilename);
		setuid(holduid);
		setgid(holdgid);
	}
}


mkdelt(pkt,sp,osp,diffloop,orig_nlines)
struct packet *pkt;
struct sid *sp, *osp;
int diffloop;
int orig_nlines;
{
	extern long Timenow;
	struct deltab dt;
	char str[BUFSIZ];
	char *del_ba();
	int newser;
	extern char *Sflags[];
	register char *p;
	int ser_inc, opred, nulldel;

	if (!diffloop && pkt->p_verbose) {
		sid_ba(sp,str);
		fprintf(pkt->p_stdout,"%s\n",str);
		fflush(pkt->p_stdout);
	}
	sprintf(str,"%c%c00000\n",CTLCHAR,HEAD);
	putline(pkt,str);
	newstats(pkt,str,"0");
	dt.d_sid = *sp;

	/*
	Check if 'null' deltas should be inserted
	(only if 'null' flag is in file and
	releases are being skipped) and set
	'nulldel' indicator appropriately.
	*/
	if (Sflags[NULLFLAG - 'a'] && (sp->s_rel > osp->s_rel + 1) &&
			!sp->s_br && !sp->s_seq &&
			!osp->s_br && !osp->s_seq)
		nulldel = 1;
	else
		nulldel = 0;
	/*
	Calculate how many serial numbers are needed.
	*/
	if (nulldel)
		ser_inc = sp->s_rel - osp->s_rel;
	else
		ser_inc = 1;
	/*
	Find serial number of the new delta.
	*/
	newser = dt.d_serial = maxser(pkt) + ser_inc;
	/*
	Find old predecessor's serial number.
	*/
	opred = sidtoser(osp,pkt);
	if (nulldel)
		dt.d_pred = newser - 1;	/* set predecessor to 'null' delta */
	else
		dt.d_pred = opred;
	dt.d_datetime = Timenow;
	strncpy(dt.d_pgmr,logname(),LOGSIZE-1);
	dt.d_type = 'D';
	del_ba(&dt,str);
	putline(pkt,str);
	if (ilist)
		mkixg(pkt,INCLUSER,INCLUDE);
	if (elist)
		mkixg(pkt,EXCLUSER,EXCLUDE);
	if (glist)
		mkixg(pkt,IGNRUSER,IGNORE);
	if (Mrs) {
		if (!(p = Sflags[VALFLAG - 'a']))
		{
			sprintf(Error,"%s (de8)", nl_msg(18,"MRs not allowed"));
			fatal(Error);
		}
		if (*p && !diffloop && valmrs(pkt,p))
		{
			sprintf(Error,"%s (de9)", nl_msg(19,"invalid MRs"));
			fatal(Error);
		}
		putmrs(pkt);
	}
	else if (Sflags[VALFLAG - 'a'])
	{
		sprintf(Error,"%s (de10)", nl_msg(20,"MRs required"));
		fatal(Error);
	}
/*
*
* CMF enhancement
*
*/
	if(Sflags[CMFFLAG - 'a'])
		{
		 if(Mrs)
			{
			 error((nl_msg(21,"input CMR's ignored")));
			 Mrs = "";
			}
		if(!deltack(pkt->p_file,Cmrs,Nsid,Sflags[CMFFLAG - 'a']))
			{
			 fatal((nl_msg(22,"Delta denied due to CMR difficulties")));
			}
		 putcmrs(pkt); /* this routine puts cmrs on the out put file */
		}
	sprintf(str,"%c%c ",CTLCHAR,COMMENTS);
	putline(pkt,str);
	sprintf(str,"%s",savecmt(Comments));
	putline(pkt,str);
	putline(pkt,"\n");
	sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
	putline(pkt,str);
	if (nulldel)			/* insert 'null' deltas */
		while (--ser_inc) {
			sprintf(str,"%c%c %s/%s/%05u\n", CTLCHAR, STATS,
				"00000", "00000", orig_nlines);
			putline(pkt,str);
			dt.d_sid.s_rel -= 1;
			dt.d_serial -= 1;
			if (ser_inc != 1)
				dt.d_pred -= 1;
			else
				dt.d_pred = opred;	/* point to old pred */
			del_ba(&dt,str);
			putline(pkt,str);
			sprintf(str,"%c%c ",CTLCHAR,COMMENTS);
			putline(pkt,str);
			putline(pkt,nl_msg(23,"AUTO NULL DELTA"));
			putline(pkt,"\n");
			sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
			putline(pkt,str);
		}
	return(newser);
}


mkixg(pkt,reason,ch)
struct packet *pkt;
int reason;
char ch;
{
	int n;
	char str[BUFSIZ];

	sprintf(str,"%c%c",CTLCHAR,ch);
	putline(pkt,str);
	for (n = maxser(pkt); n; n--) {
		if (pkt->p_apply[n].a_reason == reason) {
			sprintf(str," %u",n);
			putline(pkt,str);
		}
	}
	putline(pkt,"\n");
}


# define	LENMR	60

putmrs(pkt)
struct packet *pkt;
{
	register char **argv;
	char str[LENMR+6];
	extern char **Varg;

	for (argv = &Varg[VSTART]; *argv; argv++) {
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,*argv);
		putline(pkt,str);
	}
}



/*
*
*	putcmrs takes the cmrs list on the Mrs line built by deltack
* 	and puts them in the packet
*	
*/
	putcmrs(pkt)    
	struct packet *pkt;
	{
		char str[510];
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,Cmrs);
		putline(pkt,str);
	}


static char ambig[] = "ambiguous `r' keyletter value";	/*nl_msg 24*/

struct pfile *
rdpfile(pkt,sp)
register struct packet *pkt;
struct sid *sp;
{
	char *user;
	struct pfile pf;
	static struct pfile goodpf;
	char line[BUFSIZ];
	int cnt, uniq;
	FILE *in, *out;

	uniq = cnt = -1;
	user = logname();
	zero(&goodpf,sizeof(goodpf));
	in = xfopen(auxf(pkt->p_file,'p'),0);
	out = xfcreat(auxf(pkt->p_file,'q'),0644);
	while (fgets(line,sizeof(line),in) != NULL) {
		pf_ab(line,&pf,1);
		if (equal(pf.pf_user,user)) {
			if (sp->s_rel == 0) {
				if (++cnt) {
					fclose(out);
					fclose(in);
					sprintf(Error,"%s (de1)", nl_msg(25,"missing -r argument"));
					fatal(Error);
				}
				goodpf = pf;
				continue;
			}
			else if ((sp->s_rel == pf.pf_nsid.s_rel &&
				sp->s_lev == pf.pf_nsid.s_lev &&
				sp->s_br == pf.pf_nsid.s_br &&
				sp->s_seq == pf.pf_nsid.s_seq) ||
				(sp->s_rel == pf.pf_gsid.s_rel &&
				sp->s_lev == pf.pf_gsid.s_lev &&
				sp->s_br == pf.pf_gsid.s_br &&
				sp->s_seq == pf.pf_gsid.s_seq)) {
					if (++uniq) {
						fclose(in);
						fclose(out);
						sprintf(Error,"%s (de15)",
							nl_msg(24,ambig));
						fatal(Error);
					}
					goodpf = pf;
					continue;
			}
		}
		if(fputs(line,out)==EOF)
			FAILPUT;
	}
	fflush(out);
	fstat(fileno(out),&Statbuf);
	Szqfile = Statbuf.st_size;
	copy(auxf(pkt->p_file,'p'),Pfilename);
	fclose(out);
	fclose(in);
	if (!goodpf.pf_user[0])
	{
		sprintf(Error,"%s (de2)", nl_msg(26,"login name or SID specified not in p-file"));
		fatal(Error);
	}
	return(&goodpf);
}


FILE *
dodiff(newf,oldf,difflim,diffloop)
char *newf, *oldf;
int difflim,diffloop;
{
	register int i;
	int pfd[2];
	FILE *iop;
	extern char Diffpgm[];
	char num[10];

#ifdef debug
	fprintf(stderr,"in dodiff -- oldf = %s, newf = %s \n", oldf, newf);
	fprintf(stderr,"             fork \n");
#endif
	if ( !diffloop )
	  {  /* First attempt is to use diff */
	    strcpy(Diffpgm, DIFF);
	  }
	else
	  {  /* Use bdiff if diff fails */
	    strcpy(Diffpgm, BDIFF);
	  }
	xpipe(pfd);
	if ((i = fork()) < 0) {
		close(pfd[0]);
		close(pfd[1]);
		sprintf(Error,"%s (de11)", nl_msg(29,"cannot fork, try again"));
		fatal(Error);
	}
	else if (i == 0) {
		close(pfd[0]);
		close(1);
		dup(pfd[1]);
		close(pfd[1]);
		for (i = 5; i < 15; i++)
			close(i);
		sprintf(num,"%d",difflim);
#ifdef debug
		fprintf(stderr," CHILD --- oldf = %s, newf = %s \n", oldf, newf);
#endif
		if ( !diffloop )
		  {
		    execl(Diffpgm,Diffpgm,oldf,newf,0);
		  }
		else
		  {
		    execl(Diffpgm,Diffpgm,oldf,newf,num,"-s",0);
		  }
#ifdef debug
		fprintf(stderr,"******** error !! child exit code = 32\n");
#endif
		close(1);
		exit(32);	/* tell parent that 'execl' failed */
	}
	else {
		close(pfd[1]);
		iop = fdfopen(pfd[0],0);
		return(iop);
	}
}


getdiff(type,plinenum)
register char *type;
register int *plinenum;
{
	char line[BUFSIZ];
	register char *p;
	char *rddiff(), *linerange();
	int num_lines;
	static int chg_num, chg_ln;
	int lowline, highline;

	if ((p = rddiff(line,BUFSIZ)) == NULL)
		return(0);

	if (*p == '-') {
		*type = INS;
		*plinenum = chg_ln;
		num_lines = chg_num;
	}
	else {
		p = linerange(p,&lowline,&highline);
		*plinenum = lowline;

		switch(*p++) {
		case 'd':
			num_lines = highline - lowline + 1;
			*type = DEL;
			skipline(line,num_lines);
			break;

		case 'a':
			linerange(p,&lowline,&highline);
			num_lines = highline - lowline + 1;
			*type = INS;
			break;

		case 'c':
			chg_ln = lowline;
			num_lines = highline - lowline + 1;
			linerange(p,&lowline,&highline);
			chg_num = highline - lowline + 1;
			*type = DEL;
			skipline(line,num_lines);
			break;
		}
	}

	return(num_lines);
}


insert(pkt,linenum,n,ser)
register struct packet *pkt;
register int linenum;
register int n;
int ser;
{
	char str[BUFSIZ];

	after(pkt,linenum);
	sprintf(str,"%c%c %u\n",CTLCHAR,INS,ser);
	putline(pkt,str);
	for (++n; --n; ) {
		rddiff(str,sizeof(str));
		putline(pkt,&str[2]);
	}
	sprintf(str,"%c%c %u\n",CTLCHAR,END,ser);
	putline(pkt,str);
}


delete(pkt,linenum,n,ser)
register struct packet *pkt;
register int linenum;
int n;
register int ser;
{
	char str[BUFSIZ];

	before(pkt,linenum);
	sprintf(str,"%c%c %u\n",CTLCHAR,DEL,ser);
	putline(pkt,str);
	after(pkt,linenum + n - 1);
	sprintf(str,"%c%c %u\n",CTLCHAR,END,ser);
	putline(pkt,str);
}


after(pkt,n)
register struct packet *pkt;
register int n;
{
	before(pkt,n);
	if (pkt->p_glnno == n)
		putline(pkt,(char *) 0);
}


before(pkt,n)
register struct packet *pkt;
register int n;
{
	while (pkt->p_glnno < n) {
		if (!readmod(pkt))
			break;
	}
}


char *
linerange(cp,low,high)
register char *cp;
register int *low, *high;
{
	cp = satoi(cp,low);
	if (*cp == ',')
		cp = satoi(++cp,high);
	else
		*high = *low;

	return(cp);
}


skipline(lp,num)
register char *lp;
register int num;
{
	for (++num;--num;)
		rddiff(lp,BUFSIZ);
}


char *
rddiff(s,n)
register char *s;
register int n;
{
	register char *r;

	if ((r = fgets(s,n,Diffin)) != NULL && HADP)
		if(fputs(s,gpkt.p_stdout)==EOF)
			FAILPUT;
	return(r);
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
	ap = &pkt->p_apply[n];
	if (pkt->p_cutoff > pkt->p_idel[n].i_datetime)
		switch(ap->a_code) {
	
		case SX_EMPTY:
			switch (ch) {
			case INCLUDE:
				condset(ap,APPLY,INCLUSER);
				break;
			case EXCLUDE:
				condset(ap,NOAPPLY,EXCLUSER);
				break;
			case IGNORE:
				condset(ap,SX_EMPTY,IGNRUSER);
				break;
			}
			break;
		case APPLY:
			sprintf(Error,"%s (de5)", nl_msg(33,"internal error in delta/enter()"));
			fatal(Error);
			break;
		case NOAPPLY:
			sprintf(Error,"%s (de6)", nl_msg(34,"internal error in delta/enter()"));
			fatal(Error);
			break;
		default:
			sprintf(Error,"%s (de7)", nl_msg(35,"internal error in delta/enter()"));
			fatal(Error);
			break;
		}
}


escdodelt()	/* dummy routine for dodelt() */
{
}
fredck()	/*dummy routine for dodelt()*/
{
}

clean_up(n)
{
	if (mylock(auxf(gpkt.p_file,'z'),getpid())) {
		if (gpkt.p_iop)
			fclose(gpkt.p_iop);
		if (Xiop) {
			fclose(Xiop);
			unlink(auxf(gpkt.p_file,'x'));
		}
		if(Gin)
			fclose(Gin);
		unlink(auxf(gpkt.p_file,'d'));
		unlink(auxf(gpkt.p_file,'q'));
		xrm(&gpkt);
		ffreeall();
		unlockit(auxf(gpkt.p_file,'z'),getpid());
	}
}


#ifdef NLS
static char bd[] = "leading SOH char in line %1$d of file `%2$s' not allowed";
#else
static char bd[] = "leading SOH char in line %d of file `%s' not allowed (de14)";
#endif

fgetchk(file,pkt)
register char	*file;
register struct packet *pkt;
{
	FILE	*iptr;
	char	line[BUFSIZ];
	register int k;

	iptr = xfopen(file,0);
	for (k = 1; fgets(line,sizeof(line),iptr); k++)
		if (*line == CTLCHAR) {
			fclose(iptr);
#ifdef NLS 
			sprintmsg(Error,(nl_msg(36,bd)),k,auxf(pkt->p_file,'g'));
			strcat(Error, " (de14)");
#else
			sprintf(Error,bd,k,auxf(pkt->p_file,'g'));
#endif  
			fatal(Error);
		}
	fclose(iptr);
}
