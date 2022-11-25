/* $Revision: 66.10 $ */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 7	/* message set number */
#define MAXLNAME 14	/* maximum length of a language name */
#include <nl_types.h>
#include <locale.h>
nl_catd nlmsg_fd;
#endif NLS

#include	"lp.h"
#include	<cluster.h>
#ifdef TRUX
#include <sys/security.h>
#endif

#define	TMEMBER "Tmember"
#define	TCLASS "Tclass"
#define	TPSTATUS "Tpstatus"
#define	TQSTATUS "Tqstatus"

char errmsg[FILEMAX];	/* error message */
char curdir[FILEMAX+1];	/* current directory at time of invocation */
short force = FALSE;

#ifndef NLS
char enotmem[] = "printer \"%s\" is not a member of class \"%s\"";
#else NLS
char enotmem[] = "printer \"%1$s\" is not a member of class \"%2$s\"";	/* catgets 1 */
#endif NLS
char ememopen[] = "can't open member file";				/* catgets 2 */
char eclopen[] = "can't open class file";				/* catgets 3 */
char ebadmem[] = "corrupted member file";				/* catgets 4 */
#ifndef NLS
char einclass[] = "printer \"%s\" already in class \"%s\"";
#else NLS
char einclass[] = "printer \"%1$s\" already in class \"%2$s\"";		/* catgets 5 */
#endif NLS
char ememcreat[] = "can't create new member file";			/* catgets 6 */
char eclcreat[] = "can't create new class file";			/* catgets 7 */
char epcreat[] = "can't create new printer status file";		/* catgets 8 */
char eqcreat[] = "can't create new acceptance status file";		/* catgets 9 */
char eintcreat[] = "can't create new interface program";		/* catgets 10 */
char erequest[] = "can't create new request directory";			/* catgets 11 */
char epgone[] = "printer status entry for \"%s\" has disappeared";	/* catgets 12 */
char enodest[] = "destination \"%s\" non-existent";			/* catgets 13 */
char enopr[] = "printer \"%s\" non-existent";				/* catgets 14 */
char eprcl[] = "can't create printer \"%s\" -- it is an existing class name";	/* catgets 15 */
char eclpr[] = "can't create class \"%s\" -- it is an existing printer name";	/* catgets 16 */
char ebaddest[] = "\"%s\" is an illegal destination name";		/* catgets 17 */
char emissing[] = "new printers require -v and either -e, -i or -m";	/* catgets 18 */
char econflict[] = "keyletters \"-%c\" and \"-%c\" are contradictory";	/* catgets 19 */
char eintconf[] = "keyletters -e, -i and -m are mutually exclusive";	/* catgets 20 */
char enoclass[] = "class \"%s\" non-existent";				/* catgets 21 */
char enomodel[] = "model \"%s\" non-existent";				/* catgets 22 */
char enoaccess[] = "can't access file \"%.50s\"";			/* catgets 23 */
char ermreq[] = "can't remove request directory";			/* catgets 24 */
char ermpr[] = "can't remove printer";					/* catgets 25 */
char emove[] = "requests still queued for \"%s\" -- use lpmove";	/* catgets 26 */
char eclrm[] = "can't remove class file";				/* catgets 27 */
char esame[] = "-\"%c\" and -\"%c\" keyletters have the same value";	/* catgets 28 */
char ebadcnode[] = "cnode \"%s\" non-existent";				/* catgets 56 */
char ecdfcreat[] = "can't create interface \"%s\" as CDF";		/* catgets 57 */
char emvinter[] = "can't move interafce to specified cluster";		/* catgets 58 */
char econflict2[] = "keyletters \"-%s\" and \"-%s\" are contradictory";	/* catgets 59 */

static struct cct_entry *attached_cnode;
static char cnode_name[15];

char *a = NULL;		/* cnode name */
char *c = NULL;		/* class name */
char *d = NULL;		/* default destination */
char *e = NULL;		/* existing printer -- copy its interface for p */
short g = -1;		/* default priority */
short gopt = FALSE;	/* goption is set ? */
short h = FALSE;	/* hardwired terminal */
char *i = NULL;		/* interface pathname */
short l = FALSE;	/* login terminal */
char *m = NULL;		/* model name */
char *p = NULL;		/* printer name */
#ifdef REMOTE
char *ob3 = NULL;	/* use BSD 3 digit sequence numbers.  The */
			/* sequence file is in the request directory */
char *oce = NULL;	/* use existing cancel model */
char *oci = NULL;	/* remote cancel pathname */
char *ocm = NULL;	/* remote cancel model */
char *oma = NULL;	/* remote machine name */
char *opr = NULL;	/* remote printer name */
char *orc = NULL;	/* restrict local cancel command to owner */
char *ose = NULL;	/* use existing status model */
char *osi = NULL;	/* remote status pathname */
char *osm = NULL;	/* remote status model */
#endif REMOTE
char *r = NULL;		/* class name to remove printer p from */
char *v = NULL;		/* device pathname */
char *x = NULL;		/* destination to be deleted */

short newp = FALSE;	/* true if creating a new printer */
short newc = FALSE;	/* true if creating a new class */
short newa = FALSE;	/* true if creating a new CDF'd interface */

main(argc, argv)
int argc;
char *argv[];
{

#ifdef SecureWare
	if(ISSECURE){
            set_auth_parameters(argc, argv);
#ifdef B1
	    if(ISB1){
		initprivs();
        	(void) forcepriv(SEC_ALLOWMACACCESS);
	    }
#endif
	}
#endif
#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("lp"), stderr);
		putenv("LANG=");
	}
	nlmsg_fd = catopen("lp");
#endif NLS || NLS16

	startup(argv[0]);

	options(argc, argv);	/* process command line options */

	chkopts(argc, argv);	/* check for legality of options */

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	if(d)
		newdflt(d);
	else {
		if((!p || !v || argc > 3) && (enqueue(F_NOOP, "") == 0))
			fatal((catgets(nlmsg_fd,NL_SETN,29, "can't proceed - scheduler running")), 1);
		if(x)
			rmdest();
		else
			printer();
	}
	summary();

	exit(0);
/* NOTREACHED */
}

/* addtoclass() -- add printer p to class c */

addtoclass()
{
	char class[2*DESTMAX+2];
	FILE *cl;

	sprintf(class, "%s/%s", CLASS, c);
	if((cl = fopen(class, "a")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,3, eclopen)), 1);
	fprintf(cl, "%s\n", p);
	fclose(cl);
	if(newc)
		newq(c);
}

/* catch -- catch signals */

catch()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cleanup();
	exit(1);
}

/* iscdf(name) */

char *iscdf(name)
char *name;
{
    struct stat stbuf;
    char pathname[FILEMAX];
    char cmd[BUFSIZ];
    FILE *fp;

    sprintf(pathname, "%s/%s/%s+", SPOOL, INTERFACE, name);
    if (stat(pathname, &stbuf))
        return (NULL);		/* interface file not found */
    else
        if (S_ISCDF(stbuf.st_mode)) {
            sprintf(cmd, "/bin/ls %s | /bin/grep -v default", pathname);
            fp = popen(cmd, "r");
            fread(cnode_name, 15, 1, fp);
            pclose(fp);
            /* remove the CR put on the end of string */
            sscanf(cnode_name, "%s\n", cnode_name);
            return(cnode_name);
        } else
            return (NULL);	/* not a CDF */
}


/* change_attached_cluster */

change_attached_cluster()
{
    char path1[FILEMAX], path2[FILEMAX];

    /* copy from former target cnode to current cnode*/
    sprintf(path1, "%s/%s/%s+/%s", SPOOL, INTERFACE, p, iscdf(p));
    sprintf(path2, "%s/%s/%s+/%s", SPOOL, INTERFACE, p, a);

    if( copy(path1, path2) != 0 )
	fatal((catgets(nlmsg_fd,NL_SETN,58, emvinter)), 1);

    unlink(path1);
}


/* chkopts -- check legality of command line options */

chkopts(argc, argv)
int argc;
char *argv[];
{
	char *tmp, *fullpath();

	if(d) {
		if(argc > 2)
			usage();
		if(*d != '\0' && !isdest(d)) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,13, enodest)), d);
			fatal(errmsg, 1);
		}
	}
	else if(x) {
		if(argc > 2)
			usage();
		if(!isdest(x)) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,13, enodest)), x);
			fatal(errmsg, 1);
		}
	}
	if(d || x)
		return;
	if(!p)
		usage();
	if(!isprinter(p)) {
		if(isclass(p)) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,15, eprcl)), p);
			fatal(errmsg, 1);
		}
		if(! legaldest(p)) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,17, ebaddest)), p);
			fatal(errmsg, 1);
		}
		if(!v || !(e || i || m))
			fatal((catgets(nlmsg_fd,NL_SETN,18, emissing)), 1);
		if(r) {
#ifndef NLS
			sprintf(errmsg, enotmem, p, r);
#else NLS
			sprintmsg(errmsg, (catgets(nlmsg_fd,NL_SETN,1, enotmem)), p, r);
#endif NLS
			fatal(errmsg, 1);
		}

		if(gopt){
			if( g < MINPRI || g > MAXPRI){
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,55, "bad priority %d")),g);
				fatal(errmsg,1);
			    }
		}

		newp = TRUE;
	}

	if(a) {
	    if(! iscnode(a)) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,56, ebadcnode)), a);
		fatal(errmsg, 1);
	    }
	    if(newp || (! iscdf(p)))
		newa = TRUE;
	}

	if(c) {
		if(! isclass(c)) {
			if(isprinter(c)) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,16, eclpr)), c);
				fatal(errmsg, 1);
			}
			if(! legaldest(c)) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,17, ebaddest)), c);
				fatal(errmsg, 1);
			}
			newc = TRUE;
		}
	}
	if((i && m) || (i && e) || (m && e))
		fatal((catgets(nlmsg_fd,NL_SETN,20, eintconf)), 1);

	if(e) {
		if(!isprinter(e)) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,14, enopr)), e);
			fatal(errmsg, 1);
		}
		if(strcmp(e, p) == 0) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,28, esame)), 'e', 'p');
			fatal(errmsg, 1);
		}
	}

	if(i) {
		if((tmp = fullpath(i, curdir)) == NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,30, "can't read current directory")), 1);
		if(GET_ACCESS(tmp, ACC_R) != 0) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,23, enoaccess)), tmp);
			fatal(errmsg, 1);
		}
	}

	if(m && !ismodel(m)) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,22, enomodel)), m);
		fatal(errmsg, 1);
	}

	if(h && l) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,19, econflict)), 'h', 'l');
		fatal(errmsg, 1);
	}

	if(a && l) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,19, econflict)), 'a', 'l');
		fatal(errmsg, 1);
	}

	if(a && oci) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,59, econflict2)), "a", "oci");
		fatal(errmsg, 1);
	}
	if(a && ocm) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,59, econflict2)), "a", "ocm");
		fatal(errmsg, 1);
	}
	if(a && oma) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,59, econflict2)), "a", "orm");
		fatal(errmsg, 1);
	}
	if(a && opr) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,59, econflict2)), "a", "orp");
		fatal(errmsg, 1);
	}
	if(a && osi) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,59, econflict2)), "a", "osi");
		fatal(errmsg, 1);
	}
	if(a && osm) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,59, econflict2)), "a", "osm");
		fatal(errmsg, 1);
	}

	if(r && !isclass(r)) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,21, enoclass)), r);
		fatal(errmsg, 1);
	}

	if(c && r && strcmp(c, r) == 0) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,28, esame)), 'c', 'r');
		fatal(errmsg, 1);
	}

	if(v) {
		if((tmp = fullpath(v, curdir)) == NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,31, "can't read current directory")), 1);
                if((GET_ACCESS(tmp,ACC_W) != 0) || (GET_ACCESS(tmp,ACC_R) != 0))
		{
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,23, enoaccess)), tmp);
			fatal(errmsg, 1);
		}
	}

	if(gopt){
		if( g < MINPRI || g > MAXPRI){
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,55, "bad priority %d")),g);
			fatal(errmsg,1);
		    }
	}

#ifdef REMOTE
	if(osi) {
		if((tmp = fullpath(osi, curdir)) == NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,30, "can't read current directory")), 1);
		if(GET_ACCESS(tmp, ACC_R) != 0) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,23, enoaccess)), tmp);
			fatal(errmsg, 1);
		}
	}

	if(osm && !issmodel(osm)) {
		sprintf(errmsg, ((catgets(nlmsg_fd,NL_SETN,45, "status model \"%s\" non-existent")), osm));
		fatal(errmsg, 1);
	}

	if(oci) {
		if((tmp = fullpath(oci, curdir)) == NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,30, "can't read current directory")), 1);
		if(GET_ACCESS(tmp, ACC_R) != 0) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,23, enoaccess)), tmp);
			fatal(errmsg, 1);
		}
	}

	if(ocm && !iscmodel(ocm)) {
		sprintf(errmsg, ((catgets(nlmsg_fd,NL_SETN,46, "cancel model \"%s\" non-existent")), ocm));
		fatal(errmsg, 1);
	}

	if(ob3 && (*ob3 != 'y') && (*ob3 != 'n')){
/*
		sprintf(errmsg,"Arguement for ob3 must be 'y' or 'n', not %s",ob3);
*/
		sprintf(errmsg,(catgets(nlmsg_fd,NL_SETN,47, "No argument for ob3")));
		fatal(errmsg, 1);
	}

	if(orc && (*orc != 'y') && (*orc != 'n')){
		sprintf(errmsg,(catgets(nlmsg_fd,NL_SETN,48, "Arguement for orc must be 'y' or 'n', not %s")),orc);
		fatal(errmsg, 1);
	}

#endif REMOTE
}

/*
 *	chkreq() -- make sure that no requests are pending for destination x.
 *	Deleting printer x may imply the deletion of one or more
 *	classes.  Make sure that no requests are pending for these
 *	classes, also.
*/

chkreq()
{
	struct outq out;
	FILE *mf, *cf;
	char member[2*DESTMAX+2], class[2*DESTMAX+2];
	char mem[DESTMAX+2], cl[DESTMAX+2], *getline();
	int nmem;

	if(getodest(&out, x) != EOF) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,26, emove)), x);
		fatal(errmsg, 1);
	}

	sprintf(member, "%s/%s", MEMBER, x);
	if((mf = fopen(member, "r")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,2, ememopen)), 1);

	getline(errmsg, FILEMAX, mf);
	while(getline(cl, DESTMAX, mf) != NULL) {
		sprintf(class, "%s/%s", CLASS, cl);
		if((cf = fopen(class, "r")) == NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,3, eclopen)), 1);

		nmem = 0;
		while(getline(mem, DESTMAX, cf) != NULL)
			if(strcmp(mem, x) != 0)
				nmem++;
		if(nmem == 0) {
			setoent();
			if(getodest(&out, cl) != EOF) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,26, emove)), cl);
				fatal(errmsg, 1);
			}
		}
		fclose(cf);
	}
	fclose(mf);
	endoent();
}

/* cleanup -- called by catch() after interrupts or by fatal() after errors */

cleanup()
{
	endqent();
	endpent();
	endoent();
}

/* copy(name1, name2) -- copy file name1 to name2 */

copy(name1, name2)
char *name1, *name2;
{
	FILE *f1, *f2;
	int nch;
	char buf[BUFSIZ];

	if((f1 = fopen(name1, "r")) == NULL ||
	    (f2 = fopen(name2, "w")) == NULL)
		return(-1);
	while((nch = fread(buf, 1, BUFSIZ, f1)) > 0)
		fwrite(buf, 1, nch, f2);
	fclose(f1);
	fclose(f2);
	return(0);
}

/* create_cdf_interface */

create_cdf_interface()
{
    FILE *src_fp, *dst_fp;
    char buf[BUFSIZ];
    char path1[FILEMAX];
    char path2[FILEMAX];
    int number_bytes;

    sprintf(path1, "%s/%s/%s", SPOOL, INTERFACE, p);

    if ( ! newp ) {
	if((src_fp = fopen(path1, "r")) == NULL) {
	    sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, ecdfcreat)), path1);
	    fatal(errmsg, 1);
	}
	unlink(path1);
    }

    if ( mkdir(path1, 0755) ) {
	sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, ecdfcreat)), path1);
	fatal(errmsg, 1);
    }
    if ( chmod(path1, (0755|S_CDF)) ) {
	sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, ecdfcreat)), path1);
	fatal(errmsg, 1);
    }

    /* make interface+/cnode */

    sprintf(path2, "%s+/%s", path1, attached_cnode->cnode_name);
    if ( ! newp ) {
	/* copy interface */
	if((dst_fp = fopen(path2, "w")) == NULL) {
	    sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, ecdfcreat)), path1);
	    fatal(errmsg, 1);
	}
	while((number_bytes = fread(buf, 1, BUFSIZ, src_fp)) > 0)
		fwrite(buf, 1, number_bytes, dst_fp);
	fclose(src_fp);	fclose(dst_fp);
    } else {
	if((dst_fp = fopen(path2, "w")) == NULL) {	/* create new file */
	    sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, ecdfcreat)), path1);
	    fatal(errmsg, 1);
	}
	fclose(dst_fp);
    }
    chmod( path2, 0755 );

    /* make interface+/default */
    sprintf(path2, "%s+/default", path1);
    sprintf(path1, "%s/%s/%s", SPOOL, MODEL, CNODEMODEL);
    if(copy(path1,path2) != 0)
	fatal((catgets(nlmsg_fd,NL_SETN,10, eintcreat)), 1);
    chmod( path2, 0755 );
}

/* fromclass() -- remove printer p from class r */

fromclass()
{
	char class[2*DESTMAX + 2], member[DESTMAX + 2];
	char *getline(), *getdflt();
	FILE *ocl, *cl;
	int nmem = 0;

	sprintf(class, "%s/%s", CLASS, r);
	if((ocl = fopen(class, "r")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,3, eclopen)), 1);
	if((cl = fopen(TCLASS, "w")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,3, eclopen)), 1);

	
#ifdef SecureWare
	if(ISSECURE)
	        lp_change_mode(SPOOL, TCLASS, 0660, ADMIN,
                       "temporary printer class file");
#endif
	while(getline(member, DESTMAX, ocl) != NULL) {
		if(strcmp(member, p) != 0) {
			nmem++;
			fprintf(cl, "%s\n", member);
		}
	}

	if(nmem == 0) {
		if(rmreqdir(r) != 0) {
			unlink(TCLASS);
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,26, emove)), r);
			fatal(errmsg, 1);
		}
		if(unlink(class) < 0)
			fatal((catgets(nlmsg_fd,NL_SETN,27, eclrm)), 1);
		rmqent(r);
		if(strcmp(getdflt(), r) == 0)
			newdflt(NULL);
	}
	else {
		unlink(class);
		if(link(TCLASS, class) < 0)
			fatal((catgets(nlmsg_fd,NL_SETN,7, eclcreat)), 1);
	}
	unlink(TCLASS);
	fclose(ocl);
	fclose(cl);
}

/* getdflt() -- return system default destination */

char *
getdflt()
{
	static char dflt[DESTMAX + 2];
	char *getline();
	FILE *df;

	if((df = fopen(DEFAULT, "r")) == NULL)
		dflt[0] = '\0';
	else if(getline(dflt, DESTMAX, df) == NULL)
		dflt[0] = '\0';
	if(df)
		fclose(df);
	return(dflt);
}

/* getline(str, max, file) -- get string str of max length from file */

char *
getline(str, max, file)
char *str;
int max;
FILE *file;
{
	char *fgets();
	char *c_char;

	if(fgets(str, max, file) == NULL)
		return(NULL);
	if(*(c_char = str + strlen(str) - 1) == '\n')
		*c_char = '\0';
	return(str);
}

/* ismodel(name) -- predicate which returns TRUE iff name is a model,
		 FALSE, otherwise.		*/

ismodel(name)
char *name;
{
	char model[FILEMAX];

	if(*name == '\0' || strlen(name) > DESTMAX)
		return(FALSE);

	/* Check model directory */

	sprintf(model, "%s/%s/%s", SPOOL, MODEL, name);
	return(GET_ACCESS(model, ACC_R) != -1);
}


/* iscnode(name) */

iscnode(name)
char *name;
{
    if ( (attached_cnode = getccnam(name)) == (struct cct_entry *) 0 )
	return(FALSE);
    else
	return(TRUE);
}

#ifdef REMOTE
/* iscmodel(name) -- predicate which returns TRUE iff name is a model
		in the cmodel directory, FALSE, otherwise. */

iscmodel(name)
char *name;
{
	char cmodel[FILEMAX];

	if(*name == '\0' || strlen(name) > DESTMAX)
		return(FALSE);

	/* Check cmodel directory */

	sprintf(cmodel, "%s/%s/%s", SPOOL, CMODEL, name);
	return(GET_ACCESS(cmodel, ACC_R) != -1);
}

/* issmodel(name) -- predicate which returns TRUE iff name is a model
		in the smodel directory, FALSE, otherwise. */

issmodel(name)
char *name;
{
	char smodel[FILEMAX];

	if(*name == '\0' || strlen(name) > DESTMAX)
		return(FALSE);

	/* Check smodel directory */

	sprintf(smodel, "%s/%s/%s", SPOOL, SMODEL, name);
	return(GET_ACCESS(smodel, ACC_R) != -1);
}
#endif REMOTE

/* legaldest(d_str) -- returns TRUE if d is a syntactically correct destination
	name, FALSE otherwise.			*/

legaldest(d_str)
char *d_str;
{
	char c_chr;

	if(strlen(d_str) > DESTMAX)
		return(FALSE);
	while(c_chr = *d_str++)
		if(! isalnum(c_chr) && c_chr != '_')
			return(FALSE);
	return(TRUE);
}

/* memberfile() -- make new member file for printer p */

memberfile()
{
	char member[2*DESTMAX + 2], class[DESTMAX + 2], dev[FILEMAX];
	char *getline(), *fullpath(), *fp;
	short removed = FALSE;
	FILE *mf, *omf;

	sprintf(member, "%s/%s", MEMBER, p);
	if(! newp && (omf = fopen(member, "r")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,2, ememopen)), 1);
	if((mf = fopen(TMEMBER, "w")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,2, ememopen)), 1);

#ifdef SecureWare
	if(ISSECURE)
        	lp_change_mode(SPOOL, TMEMBER, 0660, ADMIN,
                       "temporary printer member file");
#endif	
	if(! newp && getline(dev, FILEMAX, omf) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,4, ebadmem)), 1);
	fprintf(mf, "%s\n", v ? (fp = fullpath(v, curdir)) : dev);
	if(v) {
		sprintf(errmsg, "%s %s", p, fp);
		enqueue(F_DEV, errmsg);
	}

	if(!newp) {
		removed = FALSE;
		while(getline(class, DESTMAX, omf) != NULL) {
			if(r && strcmp(class, r) == 0)
				removed = TRUE;
			else {
				if(c && strcmp(class, c) == 0) {
#ifndef NLS
					sprintf(errmsg, einclass, p, c);
#else NLS
					sprintmsg(errmsg, (catgets(nlmsg_fd,NL_SETN,5, einclass)), p, c);
#endif NLS
					fatal(errmsg, 0);
					c = NULL;
				}
				fprintf(mf, "%s\n", class);
			}
		}
	}
	if(c)
		fprintf(mf, "%s\n", c);
	if(r && !removed) {
#ifndef NLS
		sprintf(errmsg, enotmem, p, r);
#else NLS
		sprintmsg(errmsg, (catgets(nlmsg_fd,NL_SETN,1, enotmem)), p, r);
#endif NLS
		fatal(errmsg, 0);
		r = NULL;
	}
	if(!newp) {
		fclose(omf);
		unlink(member);
	}
	fclose(mf);
	if(link(TMEMBER, member) < 0)
		fatal((catgets(nlmsg_fd,NL_SETN,6, ememcreat)), 1);
	unlink(TMEMBER);
}

/* mkrequest(name) -- make new request directory for name */

mkrequest(name)
char *name;
{
	char request[2*DESTMAX + 2];

	sprintf(request, "%s/%s", REQUEST, name);
	sprintf(errmsg, "mkdir %s", request);
#ifdef SecureWare
	if(ISSECURE)
            lp_req_dir(SPOOL, 0770, ADMIN, request, erequest, errmsg);
	else{
	    if(system(errmsg) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,11, erequest)), 1);
	}
#else	
	if(system(errmsg) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,11, erequest)), 1);
#endif
}

/* newdflt(name) -- change system default destination to name */

newdflt(name)
char *name;
{
	FILE *df;

	if((df = fopen(DEFAULT, "w")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,32, "can't open system default destination file")), 1);
#ifdef SecureWare
	if(ISSECURE)
        	lp_change_mode(SPOOL, DEFAULT, 0660, ADMIN,
                       "default printer destination change");
#endif	
	if(name && *name != '\0')
		fprintf(df, "%s\n", name);
	fclose(df);
}

/* newinter() -- change interface for printer p */

newinter()
{
	char interface[2*DESTMAX+2], file[2*DESTMAX+2];
	char *new;
	char *cnode;

	if ( (cnode = iscdf(p)) )
	    sprintf(interface, "%s/%s+/%s", INTERFACE, p, cnode);
	else
	    sprintf(interface, "%s/%s", INTERFACE, p);

	if(m || e) {
		sprintf(file, "%s/%s", m ? MODEL : INTERFACE, m ? m : e);
		new = file;
	}
	else
		new = fullpath(i, curdir);

	if(! newp)
		unlink(interface);
	if(copy(new, interface) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,10, eintcreat)), 1);
#ifdef SecureWare
	if(ISSECURE)
            lp_change_mode(SPOOL, interface, 0755, ADMIN,
                       "change interface for printer");
	else
	    chmod(interface, 0755);
#else	
	chmod(interface, 0755);
#endif
}


#ifdef REMOTE
/* newcinter() -- change cancel interface for printer p */

newcinter()
{
	char cinterface[2*DESTMAX+2], file[2*DESTMAX+2];
	char *new;

	sprintf(cinterface, "%s/%s", CINTERFACE, p);
	if(ocm) {
		sprintf(file, "%s/%s", ocm ? CMODEL : CINTERFACE, ocm ? ocm : oce);
		new = file;
	}
	else
		new = fullpath(oci, curdir);
	if(! newp)
		unlink(cinterface);
	if(copy(new, cinterface) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,49, "can't create new cancel interface program")), 1);
#ifdef SecureWare
	if(ISSECURE)
		lp_change_mode(SPOOL, cinterface, 0755, ADMIN,
			"change cancel interface for printer");
	else
		chmod(cinterface, 0755);
#else
	chmod(cinterface, 0755);
#endif
}


/* newsinter() -- change status interface for printer p */

newsinter()
{
	char sinterface[2*DESTMAX+2], file[2*DESTMAX+2];
	char *new;

	sprintf(sinterface, "%s/%s", SINTERFACE, p);
	if(osm) {
		sprintf(file, "%s/%s", osm ? SMODEL : SINTERFACE, osm ? osm : ose);
		new = file;
	}
	else
		new = fullpath(osi, curdir);
	if(! newp)
		unlink(sinterface);
	if(copy(new, sinterface) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,50, "can't create new status interface program")), 1);
#ifdef SecureWare
	if(ISSECURE)
		lp_change_mode(SPOOL, sinterface, 0755, ADMIN,
			"create new status interface for printer");
	else
		chmod(sinterface, 0755);
#else
	chmod(sinterface, 0755);
#endif
}
#endif REMOTE

/* newmode() -- change mode of existing printer to hardwired or login */

newmode()
{
	struct pstat pr;

	if(getpdest(&pr, p) == EOF) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,12, epgone)), p);
		fatal(errmsg, 1);
	}
	if(a)
		pr.p_flags |= P_CNODE;

	if(h || a)
		pr.p_flags &= ~P_AUTO;
	else	/* l must be set */
		pr.p_flags |= P_AUTO;
	putpent(&pr);
	endpent();
}

/* pridefault() -- change default priority of existing printer */

pridefault()
{
	struct pstat pr;

	if(getpdest(&pr, p) == EOF) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,12, epgone)), p);
		fatal(errmsg, 1);
	}
	pr.p_default = g;
	putpent(&pr);
	endpent();
}

/* newprinter() -- add new printer p */

newprinter()
{
	struct pstat pr;

	strcpy(pr.p_dest, p);
	pr.p_rdest[0] = '\0';
#ifdef REMOTE
	pr.p_pid = 0;
	pr.p_seqno = -1;
#else
	pr.p_pid = pr.p_seqno = 0;
#endif REMOTE
	time(&pr.p_date);
	sprintf(pr.p_reason, (catgets(nlmsg_fd,NL_SETN,33, "new printer")));
	pr.p_fence = 0;
	pr.p_flags = 0;
	if(l)
		pr.p_flags |= P_AUTO;
	if(a)
		pr.p_flags |= P_CNODE;

	if(gopt){
		pr.p_default = g;
	}else{
		pr.p_default = 0;
	}
	   
#ifdef REMOTE
	pr.p_version = SPOOLING_VERSION;

	strncpy(pr.p_remotedest,oma,SP_MAXHOSTNAMELEN);
	strncpy(pr.p_remoteprinter,opr,DESTMAX);

	if ((ob3) && (*ob3 == 'y'))	/* use BSD 3 digit */
		pr.p_rflags |= P_OB3;	/* sequence numbers. */
	else 				/* else do not use BSD 3 */
		pr.p_rflags &= ~P_OB3;	/* digit sequence numbers. */

	pr.p_rflags &= ~(P_OCI|P_OCM);	/* cancel model/interface */
	if (oci){			/* remote interface defined? */
		pr.p_rflags |= P_OCI;	/* YES */
	}else{				/* NO */
		if (ocm){		/* remote model defined? */
			pr.p_rflags |= P_OCM;	/* YES */
		}				/* NO */
	}

	if ((orc) && (*orc == 'y'))	/* restrict the cancel */
		pr.p_rflags |= P_ORC;	/* command.  YES */
	else
		pr.p_rflags &= ~P_ORC;	/* NO */

	pr.p_rflags &= ~(P_OSI|P_OSM);	/* lpstat model/interface */
	if (osi){			/* remote interface defined? */
		pr.p_rflags |= P_OSI;	/* YES */
	}else{				/* NO */
		if (osm){		/* remote model defined? */
			pr.p_rflags |= P_OSM;	/* YES */
		}				/* NO */
	}
	if (oma) 			/* remote machine exist */
		pr.p_rflags |= P_OMA;	/* yes */
	else
		pr.p_rflags &= ~P_OMA;	/* no */

	if (opr) 			/* remote printer exist */
		pr.p_rflags |= P_OPR;	/* yes */
	else 
		pr.p_rflags &= ~P_OPR;	/* no */

#endif REMOTE
	addpent(&pr);
	newq(p);
	endpent();
}

/* newq(name) -- create new qstatus entry for name */

newq(name)
char *name;
{
	struct qstat acc;

	strcpy(acc.q_dest, name);
	acc.q_accept = FALSE;
	time(&acc.q_date);
	sprintf(acc.q_reason, (catgets(nlmsg_fd,NL_SETN,34, "new destination")));
#ifdef REMOTE
	if ((ob3) && (*ob3 == 'y'))	/* set p_ob3 TRUE to */
		acc.q_ob3 = TRUE;	/* use BSD 3 digit */
	else				/* sequence numbers */
		acc.q_ob3 = FALSE;
#endif REMOTE
	addqent(&acc);
	endqent();
}

/* options -- process command line options */

options(argc, argv)
int argc;
char *argv[];
{
	int j;
	char letter;		/* current keyletter */
	int  letter1, letter2;	/* current key for o options */
	char *value;		/* value of current keyletter (or NULL) */
	char *strchr();

	if(argc == 1)
		usage();

	for(j = 1; j < argc; j++) {
		if(argv[j][0] != '-' || (letter = argv[j][1]) == '\0')
			usage();
		if(! isalpha(letter)) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,35, "illegal keyletter \"%c\"")), letter);
			fatal(errmsg, 1);
		}
		letter = tolower(letter);
		value = &argv[j][2];

		switch(letter) {
		case 'a':	/* indicates printer p is attached to cnode */
		        a = value;
			break;

		case 'c':	/* class in which to insert printer p */
			c = value;
			break;
		case 'd':	/* system default destination */
			d = value;
			break;
		case 'e':	/* copy existing printer interface */
			e = value;
			break;
		case 'g':	/* default priority */
			g = (short)atoi(value);
			gopt = TRUE;
			break;
		case 'h':	/* hardwired terminal */
			h = TRUE;
			break;
		case 'i':	/* interface pathname */
			i = value;
			break;
		case 'l':	/* login terminal */
			l = TRUE;
			break;
		case 'm':	/* model interface */
			m = value;
			break;
#ifdef REMOTE
		case 'o':	/* remote options */
			if((letter1 = argv[j][2]) == '\0')
				usage();
			if((letter2 = argv[j][3]) == '\0')
				usage();
			letter1 = tolower(letter1);
			letter2 = tolower(letter2);
			letter1 = ((letter1 << 8) | letter2);
			value = &argv[j][4];

			switch(letter1) {
				case (('b' << 8)| '3'):	/* use BSD 3 digit */
							/* sequence numbers? */
/*
					if (*value == ""){
*/
						ob3 = "y";	/* yes */
						value = "y";
/*
					}else{
						ob3 = value;
					}
*/
					break;		/* sequence numbers. */
				case (('c' << 8)| 'i'):
					oci = value;	/* remote cancel */
					break;		/* pathname */
				case (('c' << 8)| 'm'):
					ocm = value;	/* remote cancel */
					break;		/* model */
				case (('r' << 8)| 'm'):
					oma = value;	/* remote machine */
					break;		/* name */
				case (('r' << 8)| 'p'):
					opr = value;	/* remote printer */
					break;		/* name */
				case (('r' << 8)| 'c'):
/*
					if (*value == ""){
*/
						orc = "y";
						value = "y";
/*
					}else{
						orc = value;	/* restrict local */
					break;		/* cancel command */
				case (('s' << 8)| 'i'):
					osi = value;	/* remote status */
					break;		/* pathname */
				case (('s' << 8)| 'm'):
					osm = value;	/* remote status */
					break;		/* model */
				default:
					sprintf(errmsg, ((catgets(nlmsg_fd,NL_SETN,51, "illegal key \"%s\""))), argv[j]);
					fatal(errmsg, 1);
					break;
			}
			break;
#endif REMOTE
		case 'p':	/* printer name */
			p = value;
			break;
		case 'r':	/* class name to remove printer p from */
			r = value;
			break;
		case 'v':	/* device pathname */
			v = value;
			break;
		case 'x':	/* destination to be deleted */
			x = value;
			break;
		default:
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,36, "unknown keyletter \"-%c\"")), letter);
			fatal(errmsg, 1);
			break;
		}

		if(*value == '\0' && strchr("dhl", letter) == NULL) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,37, "keyletter \"%c\" requires a value")),
			    letter);
			fatal(errmsg, 1);
		}

	}
}

/* printer -- add new printer p or modify printer p */

printer()
{
	if(r)
		fromclass();

	if(newp){
		newprinter();
	}else{
		if(h || l || a)
		    newmode();
		if(gopt)
		    pridefault();
	}

	if(c || r || v)
		memberfile();
	if(c)
		addtoclass();

	if( a ) {
	    if ( newa )
		create_cdf_interface();
	    else
		change_attached_cluster();
	}

	if(e || i || m)
	    newinter();

#ifdef REMOTE
	if(oci || ocm)		/* remote cancel interface processing */
		newcinter();
	if(osi || osm)		/* remote status interface processing */
		newsinter();
#endif REMOTE
	if(newp)
		mkrequest(p);
	if(newc)
		mkrequest(c);
}

/* rmclass() -- remove class file for class x */

rmclass()
{
	char member[2*DESTMAX+2], class[2*DESTMAX+2];
	char mem[DESTMAX+2], cl[DESTMAX+2];
	char *getline();
	FILE *cf, *mf, *omf;

	sprintf(class, "%s/%s", CLASS, x);
	if((cf = fopen(class, "r")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,2, ememopen)), 1);

	while(getline(mem, DESTMAX, cf) != NULL) {
		sprintf(member, "%s/%s", MEMBER, mem);
		if((omf = fopen(member, "r")) == NULL ||
		    (mf = fopen(TMEMBER, "w")) == NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,2, ememopen)), 1);
#ifdef SecureWare
		if(ISSECURE)
                	lp_change_mode(SPOOL, TMEMBER, 0660, ADMIN,
                               "temporary printer member file");
#endif
		fprintf(mf, "%s\n", getline(errmsg, FILEMAX, omf));
		while(getline(cl, DESTMAX, omf) != NULL)
			if(strcmp(cl, x) != 0)
				fprintf(mf, "%s\n", cl);
		fclose(mf);
		fclose(omf);
		unlink(member);
		if(link(TMEMBER, member) != 0)
			fatal((catgets(nlmsg_fd,NL_SETN,6, ememcreat)), 1);
		unlink(TMEMBER);
	}
	fclose(cf);
	if(unlink(class) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,27, eclrm)), 1);
}

/* rmdest -- remove destination x */

rmdest()
{
	char *getdflt();

	if(isclass(x)) {
		if(rmreqdir(x) != 0) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,26, emove)), x);
			fatal(errmsg, 1);
		}
		rmqent(x);
		rmclass();
	}
	else {
		chkreq();
		rmprinter();
	}

	if(strcmp(getdflt(), x) == 0)
		newdflt(NULL);
}

/* rmpent(name) -- remove name's pstatus entry */

rmpent(name)
char *name;
{
	FILE *pf;
	struct pstat ps;

	if((pf = fopen(TPSTATUS, "w")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,8, epcreat)), 1);
#ifdef SecureWare
	if(ISSECURE)
        	lp_change_mode(SPOOL, TPSTATUS, 0660, ADMIN,
                       "temporary printer status file");
#endif
	while(getpent(&ps) != EOF)
		if(strcmp(ps.p_dest, name) != 0)
			wrtpent(&ps, pf);
	fclose(pf);
	unlink(PSTATUS);
	if(link(TPSTATUS, PSTATUS) < 0)
		fatal((catgets(nlmsg_fd,NL_SETN,8, epcreat)), 1);
	unlink(TPSTATUS);
	endpent();
}

/* rmprinter() -- remove printer x */

rmprinter()
{
	char member[2*DESTMAX+2], file[2*DESTMAX+2];
	char mem[DESTMAX+2], cl[2*DESTMAX+2], *getline();
	FILE *mf, *cf, *ocf;
	int nmem;

	force = TRUE;
	rmreqdir(x);
	rmqent(x);
	rmpent(x);

	sprintf(member, "%s/%s", MEMBER, x);
	if((mf = fopen(member, "r")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,2, ememopen)), 1);

	getline(errmsg, FILEMAX, mf);
	while(getline(cl, DESTMAX, mf) != NULL) {
		sprintf(file, "%s/%s", CLASS, cl);
		if((ocf = fopen(file, "r")) == NULL ||
		    (cf = fopen(TCLASS, "w")) == NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,3, eclopen)), 1);
#ifdef SecureWare
		if(ISSECURE)
                	lp_change_mode(SPOOL, TCLASS, 0660, ADMIN,
                               "temporary printer class file");
#endif

		nmem = 0;
		while(getline(mem, DESTMAX, ocf) != NULL) {
			if(strcmp(mem, x) != 0) {
				fprintf(cf, "%s\n", mem);
				nmem++;
			}
		}
		unlink(file);
		if(nmem == 0) {
			rmreqdir(cl);
			rmqent(cl);
		}
		else
			if(link(TCLASS, file) != 0)
				fatal((catgets(nlmsg_fd,NL_SETN,7, eclcreat)), 1);
		unlink(TCLASS);
		fclose(ocf);
		fclose(cf);
	}
	fclose(mf);
	if(unlink(member) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,25, ermpr)), 1);
	if ( iscdf(x) ) {
	    sprintf(file, "/bin/rm -rf %s/%s+", INTERFACE, x);
	    system(file);
	} else {
	    sprintf(file, "%s/%s", INTERFACE, x);
	    unlink(file);
	}
#ifdef REMOTE
	sprintf(file, "%s/%s", CINTERFACE, x);	/* If these files do */
	unlink(file);				/* not exist, ignore */
	sprintf(file, "%s/%s", SINTERFACE, x);	/* it. */
	unlink(file);
#endif REMOTE
}

/* rmqent(name) -- remove name's qstatus entry */

rmqent(name)
char *name;
{
	FILE *qf;
	struct qstat qs;

	if((qf = fopen(TQSTATUS, "w")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,9, eqcreat)), 1);
#ifdef SecureWare
	if(ISSECURE)
        	lp_change_mode(SPOOL, TQSTATUS, 0660, ADMIN,
                       "temporary printer queue status file");
#endif	
	while(getqent(&qs) != EOF)
		if(strcmp(qs.q_dest, name) != 0)
			wrtqent(&qs, qf);
	fclose(qf);
	unlink(QSTATUS);
	if(link(TQSTATUS, QSTATUS) < 0)
		fatal((catgets(nlmsg_fd,NL_SETN,9, eqcreat)), 1);
	unlink(TQSTATUS);
	endqent();
}

/* rmreqdir(name) -- remove name's request directory */

rmreqdir(name)
char *name;
{
	struct outq oq;

	if(!force && getodest(&oq, name) != EOF) {
		endoent();
		return(-1);
	}
	sprintf(errmsg, "rm -fr %s/%s", REQUEST, name);
	if(system(errmsg) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,24, ermreq)), 1);
	endoent();
	return(0);
}

/* startup -- initialization routine */

startup(name)
char *name;
{
	int catch(), cleanup();
	struct passwd *adm, *getpwnam();
	extern char *f_name;
	extern int (*f_clean)();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);

#ifdef SecureWare
	if(ISSECURE)
        	lp_set_mask(0022);
	else
		umask(0022);
#else	
	umask(0022);
#endif
	f_name = name;
	f_clean = cleanup;

	if(! ISADMIN)
		fatal(ADMINMSG, 1);

	if((adm = getpwnam(ADMIN)) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,38, "LP Administrator not in password file\n")), 1);

#ifdef SecureWare
        if(((ISSECURE) && (lp_set_ids(adm->pw_uid,adm->pw_gid) == -1)) ||
	   ((!ISSECURE) && ( setresgid(adm->pw_gid, adm->pw_gid, -1) == -1
		|| setresuid(adm->pw_uid, adm->pw_uid, -1) == -1 )))
#else
	if( setresgid(adm->pw_gid, adm->pw_gid, -1) == -1
		|| setresuid(adm->pw_uid, adm->pw_uid, -1) == -1 )
#endif
		fatal((catgets(nlmsg_fd,NL_SETN,39, "can't set user id to LP Administrator's user id")), 1);

	gwd(curdir);			/* get current directory */

	if(chdir(SPOOL) == -1)
		fatal((catgets(nlmsg_fd,NL_SETN,40, "spool directory non-existent")), 1);

}

/* summary -- print summary of actions taken */

summary()
{
}

/* usage -- print command usage message and exit */

usage()
{
	printf((catgets(nlmsg_fd,NL_SETN,41, "usages:\tlpadmin -pprinter [-vdevice] [-cclass] [-rclass]\n")));
	printf((catgets(nlmsg_fd,NL_SETN,42, "       \t       [-eprinter|-iinterface|-mmodel] [-h|-l]\n")));
	printf(((catgets(nlmsg_fd,NL_SETN,52, "       \t       [-ociinterface|-ocmmodel] [-ob3]\n"))));
	printf(((catgets(nlmsg_fd,NL_SETN,53, "       \t       [-osiinterface|-osmmodel] [-orc]\n"))));
	printf(((catgets(nlmsg_fd,NL_SETN,54, "       \t       [-ormmachine|-orpprinter]\n"))));
	printf(((catgets(nlmsg_fd,NL_SETN,60, "       \t       [-acluster_client]\n"))));
	printf((catgets(nlmsg_fd,NL_SETN,43, "       \tlpadmin -d[destination]\n")));
	printf((catgets(nlmsg_fd,NL_SETN,44, "       \tlpadmin -xdestination\n")));
	exit(0);
}
