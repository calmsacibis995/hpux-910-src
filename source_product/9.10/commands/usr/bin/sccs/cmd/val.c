static char *HPUX_ID = "@(#) $Revision: 66.2 $";
#ifdef NLS
#define NL_SETN  9
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
/************************************************************************/
/*									*/
/*  val -                                                               */
/*  val [-mname] [-rSID] [-s] [-ytype] file ...                         */
/*                                                                      */
/************************************************************************/

# include	"../hdr/defines.h"
# include	"../hdr/had.h"


# define	FILARG_ERR	0200	/* no file name given */
# define	UNKDUP_ERR	0100	/* unknown or duplicate keyletter */
# define	CORRUPT_ERR	040	/* corrupt file error code */
# define	FILENAM_ERR	020	/* file name error code */
# define	INVALSID_ERR	010	/* invalid or ambiguous SID error  */
# define	NONEXSID_ERR	04	/* non-existent SID error code */
# define	TYPE_ERR	02	/* type arg value error code */
# define	NAME_ERR	01	/* name arg value error code */
# define	TRUE		1
# define	FALSE		0
# define	BLANK(l)	while (!(*l == ' ' || *l == '\t')) l++;

# define	CORRUPT_HEAD		0x001	/* @h line missing */
# define	CORRUPT_BDELTAB		0x002	/* @d line missing */
# define	CORRUPT_DATESEQ		0x004	/* dates out of order */
# define	CORRUPT_CTLLINE		0x008	/* unexpected ctl line */
# define	CORRUPT_BUSERNAM	0x010	/* @u line missing */
# define	CORRUPT_EOF		0x020	/* unexpected eof */
# define	CORRUPT_HASH		0x040	/* checksum error */
# define	CORRUPT_BUSERTXT	0x080	/* @t line missing */
# define	CORRUPT_MOD		0x100	/* @I, @D or @E line missing */
# define	CORRUPT_END		0x200	/* @E line missing */

int	ret_code;	/* prime return code from 'main' program */
int	inline_err;	/* input line error code (from 'process') */
int	corrupt_code;	/* details on CORRUPT_ERR condition */
int	infile_err;	/* file error code (from 'validate') */
int	inpstd;		/* TRUE = args from standard input */
int	verbose=FALSE;	/* print details of corruption if TRUE */

struct packet gpkt;

char	had[26];	/* had flag used in 'process' function */
char	path[FILESIZE];	/* storage for file name value */
char	sid[50];	/* storage for sid (-r) value */
char	type[50];	/* storage for type (-y) value */
char	name[50];	/* storage for name (-m) value */
char	line[BUFSIZ];
char	*get_line();	/* function returning ptr to line read */
char	*getval();	/* function returning adjusted ptr to line */
char	*fmalloc();	/* function returning ptr */
char	*fgets();	/* function returning i/o ptr */

struct delent {		/* structure for delta table entry */
	char type;
	char *osid;
	char *datetime;
	char *pgmr;
	char *serial;
	char *pred;
} del;

char	Error[128];

/* This is the main program that determines whether the command line
 * comes from the standard input or read off the original command
 * line.  See VAL(I) for more information.
*/
main(argc,argv)
int argc;
char	*argv[];
{
	FILE	*iop;
	register int j;

#ifdef NLS
	nl_catopen("sccs");
#endif


	ret_code = 0;
	if (argc == 2 && argv[1][0] == '-' && !(argv[1][1])) {
		inpstd = TRUE;
		iop = stdin;		/* read from standard input */
		while (fgets(line,BUFSIZ,iop) != NULL) {
			if (line[0] != '\n') {
				repl (line,'\n','\0');
				process(line);
				ret_code |= inline_err;
			}
		}
	}
	else {
		inpstd = FALSE;
		for (j = 1; j < argc; j++)
			sprintf(&(line[strlen(line)]),"%s ",argv[j]);
		j = strlen(line) - 1;
		line[j > 0 ? j : 0] = NULL;
		process(line);
		ret_code = inline_err;
	}
	exit(ret_code);

}


/* This function processes the line sent by the main routine.  It
 * determines which keyletter values are present on the command
 * line and assigns the values to the correct storage place.  It
 * then calls validate for each file name on the command line
 * It will return to main if the input line contains an error,
 * otherwise it returns any error code found by validate.
*/
process(p_line)
char	*p_line;
{
	register int	j;
	register int	testklt;
	register int	line_sw;

	int	silent;
	int	num_files;

	char	filelist[50][FILESIZE];
	char	*savelinep;
	char	c;

	silent = FALSE;
	path[0] = sid[0] = type[0] = name[0] = 0;
	num_files = inline_err = corrupt_code = 0;

	/*
	make copy of 'line' for use later
	*/
	savelinep = p_line;
	/*
	clear out had flags for each 'line' processed
	*/
	for (j = 0; j < 27; j++)
		had[j] = 0;
	/*
	execute loop until all characters in 'line' are checked.
	*/
	while (p_line && *p_line) {
		testklt = 1;
		NONBLANK(p_line);
		if (*p_line == '-') {
			p_line += 1;
			c = *p_line;
			p_line++;
			switch (c) {
				case 's':
					testklt = 0;
					/*
					turn on 'silent' flag.
					*/
					silent = TRUE;
					break;
				case 'r':
					p_line = getval(p_line,sid);
					break;
				case 'y':
					p_line = getval(p_line,type);
					break;
				case 'm':
					p_line = getval(p_line,name);
					break;
				case 'v':
					testklt = 0;
					/*
					turn on 'verbose' flag.
					*/
					verbose = TRUE;
					break;
				default:
					inline_err |= UNKDUP_ERR;
			}
			/*
			use 'had' array and determine if the keyletter
			was given twice.
			*/
			if (had[c - 'a']++ && testklt++)
				inline_err |= UNKDUP_ERR;
		}
		else {
			/*
			assume file name if no '-' preceeded argument
			*/
			p_line = getval(p_line,filelist[num_files]);
			num_files++;
		}
	}
	/*
	check if any files were named as arguments
	*/
	if (num_files == 0)
		inline_err |= FILARG_ERR;
	/*
	check for error in command line.
	*/
	if (inline_err && !silent) {
		if (inpstd)
			report(inline_err,savelinep,"");
		else report(inline_err,"","");
		return;		/* return to 'main' routine */
	}
	line_sw = 1;		/* print command line flag */
	/*
	loop through 'validate' for each file on command line.
	*/
	for (j = 0; j < num_files; j++) {
		/*
		read a file from 'filelist' and place into 'path'.
		*/
		sprintf(path,"%s",filelist[j]);
		validate(path,sid,type,name);
		inline_err |= infile_err;
		/*
		check for error from 'validate' and call 'report'
		depending on 'silent' flag.
		*/
		if (infile_err && !silent) {
			if (line_sw && inpstd) {
				report(infile_err,savelinep,path);
				line_sw = 0;
			}
			else report(infile_err,"",path);
		}
	}
	return;		/* return to 'main' routine */
}


/* This function actually does the validation on the named file.
 * It determines whether the file is an SCCS-file or if the file
 * exists.  It also determines if the values given for type, SID,
 * and name match those in the named file.  An error code is returned
 * if any mismatch occurs.  See VAL(I) for more information.
*/
validate(c_path,c_sid,c_type,c_name)
char	*c_path;
char	*c_sid;
char	*c_type;
char	*c_name;
{
	char *auxf();
	register char	*l;
	int	goods,goodt,goodn,hadmflag;

	infile_err = goods = goodt = goodn = hadmflag = 0;
	sinit(&gpkt,c_path);
	if (!sccsfile(c_path) || (gpkt.p_iop = fopen(c_path,"r")) == NULL)
		infile_err |= FILENAM_ERR;
	else if (l = get_line(&gpkt)) {		/* read first line in file */
		/*
		check that it is header line.
		*/
		if (*l++ != CTLCHAR || *l++ != HEAD) {
			infile_err |= CORRUPT_ERR;
			corrupt_code |= CORRUPT_HEAD;
		}

		else {
			/*
			get old file checksum count
			*/
			satoi(l,&gpkt.p_ihash);
			gpkt.p_chash = 0;
			if (HADR)
				/*
				check for invalid or ambiguous SID.
				*/
				if (invalid(c_sid))
					infile_err |= INVALSID_ERR;
			/*
			read delta table checking for errors and/or
			SID.
			*/
			if (do_delt(&gpkt,goods,c_sid)) {
				fclose(gpkt.p_iop);
				infile_err |= CORRUPT_ERR;
				return;
			}

			read_to(EUSERNAM,&gpkt);

			if (HADY || HADM) {
				/*
				read flag section of delta table.
				*/
				while ((l = get_line(&gpkt)) &&
					*l++ == CTLCHAR &&
					*l++ == FLAG) {
					NONBLANK(l);
					repl(l,'\n','\0');
					if (*l == TYPEFLAG) {
						l += 2;
						if (equal(c_type,l))
							goodt++;
					}
					else if (*l == MODFLAG) {
						hadmflag++;
						l += 2;
						if (equal(c_name,l))
							goodn++;
					}
				}
				if (*(--l) != BUSERTXT) {
					fclose(gpkt.p_iop);
					infile_err |= CORRUPT_ERR;
					corrupt_code |= CORRUPT_BUSERTXT;
					return;
				}
				/*
				check if 'y' flag matched '-y' arg value.
				*/
				if (!goodt && HADY)
					infile_err |= TYPE_ERR;
				/*
				check if 'm' flag matched '-m' arg value.
				*/
				if (HADM && !hadmflag) {
					if (!equal(auxf(sname(c_path),'g'),c_name))
						infile_err |= NAME_ERR;
				}
				else if (HADM && hadmflag && !goodn)
						infile_err |= NAME_ERR;
			}
			else read_to(BUSERTXT,&gpkt);
			read_to(EUSERTXT,&gpkt);
			gpkt.p_chkeof = 1;
			/*
			read remainder of file so 'read_mod'
			can check for corruptness.
			*/
			while (read_mod(&gpkt))
				;
		}
	fclose(gpkt.p_iop);	/* close file pointer */
	}
	return;		/* return to 'process' function */
}


/* This function reads the 'delta' line from the named file and stores
 * the information into the structure 'del'.
*/
getdel(delp,lp)
register struct delent *delp;
register char *lp;
{
	NONBLANK(lp);
	delp->type = *lp++;
	NONBLANK(lp);
	delp->osid = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->datetime = lp;
	BLANK(lp);
	NONBLANK(lp);
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->pgmr = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->serial = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->pred = lp;
	repl(lp,'\n','\0');
}


/* This function does a read through the named file until it finds
 * the character sent over as an argument.
*/
read_to(ch,pkt)
register char ch;
register struct packet *pkt;
{
	register char *n;
	while ((n = get_line(pkt)) && n &&
			!(*n++ == CTLCHAR && *n == ch))
		;
	return;
}


/* This function places into a specified destination characters  which
 * are delimited by either a space, tab or 0.  It obtains the char-
 * acters from a line of characters.
*/
char	*getval(sourcep,destp)
register char	*sourcep;
register char	*destp;
{
	while (*sourcep != ' ' && *sourcep != '\t' && *sourcep != '\0')
		*destp++ = *sourcep++;
	*destp = 0;
	return(sourcep);
}


/* This function will report the error that occured on the command
 * line.  It will print one diagnostic message for each error that
 * was found in the named file.
*/
report(code,inp_line,file)
register int	code;
register char	*inp_line;
register char	*file;
{
	char	percent;
	percent = '%';		/* '%' for -m and/or -y messages */
	if (*inp_line)
		printf("%s\n\n",inp_line);
	if (code & NAME_ERR)
#ifdef NLS 
	{
	printmsg(nl_msg(1,"    %1$s: %2$cM%3$c, -m mismatch"),file,percent,percent);
		printf("\n");	}
#else
		printf("    %s: %cM%c, -m mismatch\n",file,percent,percent);
#endif  
	if (code & TYPE_ERR)
#ifdef NLS 
	{
		printmsg(nl_msg(2,"    %1$s: %2$cY%3$c, -y mismatch"),file,percent,percent);
		printf("\n");	}
#else
		printf("    %s: %cY%c, -y mismatch\n",file,percent,percent);
#endif  
	if (code & NONEXSID_ERR)
	{	printf(nl_msg(3,"    %s: SID nonexistent"),file);
		printf("\n");	}
	if (code & INVALSID_ERR)
	{	printf(nl_msg(4,"    %s: SID invalid or ambiguous"),file);
		printf("\n");	}
	if (code & FILENAM_ERR)
	{	printf(nl_msg(5,"    %s: can't open file or file not SCCS"),file);
		printf("\n");	}
	if (code & CORRUPT_ERR) {
	    printf(nl_msg(6,"    %s: corrupted SCCS file\n"),file);
	    if (verbose) {
		char *expected = nl_msg(11,"expected");
		char *or = nl_msg(12,"or");
		if (corrupt_code & CORRUPT_HEAD)
		    printf("\t%s \"@%c\" (val1)\n", expected, HEAD);
		if (corrupt_code & CORRUPT_BDELTAB)
		    printf("\t%s \"@%c\" (val1)\n", expected, BDELTAB);
		if (corrupt_code & CORRUPT_DATESEQ)
		    printf("\t%s (val2)\n",
			nl_msg(13,"delta creation date out of sequence or in the future"));
		if (corrupt_code & CORRUPT_BUSERNAM)
		    printf("\t%s \"@%c\" (val1)\n", expected, BUSERNAM);
		if (corrupt_code & CORRUPT_BUSERTXT)
		    printf("\t%s \"@%c\" (val1)\n", expected, BUSERTXT);
		if (corrupt_code & CORRUPT_END)
		    printf("\t%s \"@%c\" (val1)\n", expected, END);
		if (corrupt_code & CORRUPT_CTLLINE)
		    printf("\t%s (val3)\n", nl_msg(14,"illegal control line"));
		if (corrupt_code & CORRUPT_EOF)
		    printf("\t%s (val4)\n", nl_msg(15,"unexpected end of file"));
		if (corrupt_code & CORRUPT_HASH)
		    printf("\t%s (val5)\n", nl_msg(16,"checksum error"));
		if (corrupt_code & CORRUPT_MOD)
		    printf("\t%s \"@%c\" %s \"@%c\" %s \"@%c\" (val1)\n",
			    expected, INS, or, DEL, or, END);
	    }
	}
	if (code & UNKDUP_ERR)
		{  printf(nl_msg(7,"    %s: Unknown or duplicate keyletter argument"),file);
		printf("\n");	}
	if (code & FILARG_ERR)
		{  printf(nl_msg(8,"    %s: missing file argument"),file);
		printf("\n");	}
	return;
}


/* This function takes as it's argument the SID inputed and determines
 * whether or not it is valid (e. g. not ambiguous or illegal).
*/
invalid(i_sid)
register char	*i_sid;
{
	register int count;
	register int digits;
	count = digits = 0;
	if (*i_sid == '0' || *i_sid == '.')
		return (1);
	i_sid++;
	digits++;
	while (*i_sid != '\0') {
		if (*i_sid++ == '.') {
			digits = 0;
			count++;
			if (*i_sid == '0' || *i_sid == '.')
				return (1);
		}
		digits++;
		if (digits > 5)
			return (1);
	}
	if (*(--i_sid) == '.' )
		return (1);
	if (count == 1 || count == 3)
		return (0);
	return (1);
}


/*
	Routine to read a line into the packet.  The main reason for
	it is to make sure that pkt->p_wrttn gets turned off,
	and to increment pkt->p_slnno.
*/

char	*get_line(pkt)
register struct packet *pkt;
{
	register char *n;
	register char *p;

	if ((n = fgets(pkt->p_line,sizeof(pkt->p_line),pkt->p_iop)) != NULL) {
		pkt->p_slnno++;
		for (p = pkt->p_line; p && *p; )
			pkt->p_chash += *p++;
	}
	else {
		if (!pkt->p_chkeof)
			infile_err |= CORRUPT_ERR, corrupt_code |= CORRUPT_EOF;
		if (pkt->do_chksum && (pkt->p_chash ^ pkt->p_ihash)&0xFFFF)
			infile_err |= CORRUPT_ERR, corrupt_code |= CORRUPT_HASH;
	}
	return(n);
}


/*
	Does initialization for sccs files and packet.
*/

sinit(pkt,file)
register struct packet *pkt;
register char *file;
{

	zero(pkt,sizeof(*pkt));
	copy(file,pkt->p_file);
	pkt->p_wrttn = 1;
	pkt->do_chksum = 1;	/* turn on checksum check for getline */
}


read_mod(pkt)
register struct packet *pkt;
{
	register char *p;
	int ser;
	int iord;
	register struct apply *ap;

	while (get_line(pkt) != NULL) {
		p = pkt->p_line;
		if (p && (*p++ != CTLCHAR))
			continue;
		else {
			if (!((iord = *p++) == INS || iord == DEL || iord == END)) {
				infile_err |= CORRUPT_ERR, corrupt_code |= CORRUPT_MOD;
				return(0);
			}
			NONBLANK(p);
			satoi(p,&ser);
			if (iord == END)
				remq(pkt,ser);
 /*  
     ??? If we are just checking for nesting then we can always put on
     queue, and don't really care about keep or user fields.
     Need to check: are they calling other routines that expect an apply[]
     array;  if so, need to allocate one and zero it out.

			else if ((ap = &pkt->p_apply[ser])->a_code == APPLY)
				addq(pkt,ser,iod == INS ? YES : NO,iod,ap->a_reason & USER);
			else
				addq(pkt,ser,iod == INS ? NO : NULL,iod,ap->a_reason & USER);

*/

		else
			addq(pkt,ser);
		}
	}
	if (pkt->p_q)
		infile_err |= CORRUPT_ERR, corrupt_code |= CORRUPT_END;
	return(0);
}


/********** replaced with simpler routines -- NULL pointer problem ******

addq(pkt,ser,keep,iord,user)
struct packet *pkt;
int ser;
int keep;
int iord;
{
	register struct queue *cur, *prev, *q;

	for (cur = (struct queue *) (&pkt->p_q); cur && (cur = (prev = cur)->q_next); )
		if (cur->q_sernum <= ser)
			break;
	if (cur && (cur->q_sernum == ser))
		infile_err |= CORRUPT_ERR;
	prev->q_next = q = (struct queue *) fmalloc(sizeof(*q));
	q->q_next = cur;
	q->q_sernum = ser;
	q->q_keep = keep;
	q->q_iord = iord;
	q->q_user = user;
	if (pkt->p_ixuser && (q->q_ixmsg = chkix(q,(struct queue *) (&pkt->p_q))))
		++(pkt->p_ixmsg);
	else
		q->q_ixmsg = 0;

	setkeep(pkt);
}


remq(pkt,ser)
register struct packet *pkt;
int ser;
{
	register struct queue *cur, *prev;

	for (cur = (struct queue *) (&pkt->p_q); cur && (cur = (prev = cur)->q_next); )
		if (cur->q_sernum == ser)
			break;
	if (cur) {
		if (cur->q_ixmsg)
			--(pkt->p_ixmsg);
		prev->q_next = cur->q_next;
		ffree((char *) cur);
		setkeep(pkt);
	}
	else
		infile_err |= CORRUPT_ERR;
}


setkeep(pkt)
register struct packet *pkt;
{
	register struct queue *q;
	register struct sid *sp;

	for (q = (struct queue *) (&pkt->p_q); q && (q = q->q_next); )
		if (q->q_keep != NULL) {
			if ((pkt->p_keep = q->q_keep) == YES) {
				sp = &pkt->p_idel[q->q_sernum].i_sid;
				pkt->p_inssid.s_rel = sp->s_rel;
				pkt->p_inssid.s_lev = sp->s_lev;
				pkt->p_inssid.s_br = sp->s_br;
				pkt->p_inssid.s_seq = sp->s_seq;
			}
			return;
		}
	pkt->p_keep = NO;
}


# define apply(qp)	((qp->q_iord == INS && qp->q_keep == YES) || \
				(qp->q_iord == DEL && qp->q_keep == (NO & 0377)))

chkix(new,head)
register struct queue *new;
struct queue *head;
{
	register int retval;
	register struct queue *cur;
	int firstins, lastdel;

	if (!apply(new))
		return(0);
	for (cur = head; cur && (cur = cur->q_next); )
		if (cur->q_user)
			break;
	if (!cur)
		return(0);
	retval = 0;
	firstins = 0;
	lastdel = 0;
	for (cur = head;  cur && (cur = cur->q_next); ) {
		if (apply(cur)) {
			if (cur->q_iord == DEL)
				lastdel = cur->q_sernum;
			else if (firstins == 0)
				firstins = cur->q_sernum;
		}
		else if (cur->q_iord == INS)
			retval++;
	}
	if (retval == 0) {
		if (lastdel && (new->q_sernum > lastdel))
			retval++;
		if (firstins && (new->q_sernum < firstins))
			retval++;
	}
	return(retval);
}


*********** end of delete ******************************/

/* This function reads the delta table entries and checks for the format
 * as specifed in sccsfile(V).  If the format is incorrect, a corrupt
 * error will be issued by 'val'.  This function also checks
 * if the sid requested is in the file (depending if '-r' was specified).
*/
do_delt(pkt,goods,d_sid)
register struct packet *pkt;
register int goods;
register char *d_sid;
{
	char *l;
	char odatetime[20];
	static time_t timer = 0;

	/* set an odatetime reference date equal to 'now' */
	if (timer == 0) {
	    timer = time((time_t)0);
	}
	date_ba(&timer, odatetime);

	while(getstats(pkt)) {
		if (((l = get_line(pkt)) != NULL) && *l++ != CTLCHAR || *l++ != BDELTAB) {
			corrupt_code |= CORRUPT_BDELTAB;
			return(1);
		}

		/* dates should be in descending order */
		getdel(&del,l);
		if (*odatetime && (strcmp(odatetime, del.datetime) < 0)) {
			corrupt_code |= CORRUPT_DATESEQ;
			return(1);
		}
		strcpy(odatetime, del.datetime);

		if (HADR && !(infile_err & INVALSID_ERR)) {
			if (equal(d_sid,del.osid) && del.type == 'D')
				goods++;
		}
		while ((l = get_line(pkt)) != NULL)
			if (pkt->p_line[0] != CTLCHAR)
				break;
			else {
				switch(pkt->p_line[1]) {
				case EDELTAB:
					break;
				case COMMENTS:
				case MRNUM:
				case INCLUDE:
				case EXCLUDE:
				case IGNORE:
					continue;
				default:
					corrupt_code |= CORRUPT_CTLLINE;
					return(1);
				}
				break;
			}
		if (l == NULL) {
			corrupt_code |= CORRUPT_EOF;
			return(1);
		}
		if (pkt->p_line[0] != CTLCHAR) {
			corrupt_code |= CORRUPT_BUSERNAM;
			return(1);
		}
	}
	if (pkt->p_line[1] != BUSERNAM) {
		corrupt_code |= CORRUPT_BUSERNAM;
		return(1);
	}
	if (HADR && !goods && !(infile_err & INVALSID_ERR))
		infile_err |= NONEXSID_ERR;
	return(0);
}


/* This function reads the stats line from the sccsfile */
getstats(pkt)
register struct packet *pkt;
{
	register char *p;
	p = pkt->p_line;
	if (get_line(pkt) == NULL || *p++ != CTLCHAR || *p != STATS)
		return(0);
	return(1);
}

/* The following local procs for addq() and remq() added because
   those in SCCS Lib access data structures that don't exist for
   prs ==> nil pointer dereferencing.
*/

addq(pkt,ser)
struct packet *pkt;
int ser;
{
	struct queue *cur, *prev, *q;

	for (cur = (struct queue *)(&pkt->p_q); cur && (cur = (prev = cur)->q_next); )
		if (cur->q_sernum <= ser)
			break;
	if (cur && (cur->q_sernum == ser))
		fmterr(pkt);
	prev->q_next = q = (struct queue *) fmalloc(sizeof(*q));
/*****************
	prev->q_next = q = alloc(sizeof(*q));
	zero(q,sizeof(*q));
************************/
	q->q_next = cur;
	q->q_sernum = ser;
}

remq(pkt,ser)
struct packet *pkt;
int ser;
{
	struct queue *cur, *prev;

	for (cur = (struct queue *)(&pkt->p_q); cur && (cur = (prev = cur)->q_next); )
		if (cur->q_sernum == ser)
			break;
	if (cur) {
		prev->q_next = cur->q_next;
		free(cur);
	}
	else
		fmterr(pkt);
}
