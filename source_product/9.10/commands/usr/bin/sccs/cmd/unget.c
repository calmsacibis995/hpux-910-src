static char *HPUX_ID = "@(#) $Revision: 37.2 $";
#ifdef NLS
#define NL_SETN  8
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	"../hdr/defines.h"
# include	"../hdr/had.h"


/*
		Program can be invoked as either "unget" or
		"sact".  Sact simply displays the p-file on the
		standard output.  Unget removes a specified entry
		from the p-file.
*/

struct stat Statbuf;
char Error[128];

int	verbosity;
int	num_files;
int	cmd;
long	Szqfile;
char	had[26];
char	Pfilename[FILESIZE];
char	*auxf();
struct	packet gpkt;
struct	sid sid;

main(argc,argv)
int argc;
char *argv[];
{
	int	i, testmore;
	char	c, *p;
	char	*sid_ab();
	extern	unget();
	extern	int Fcnt;

#ifdef NLS
	nl_catopen("sccs");
#endif


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
			case 'n':
			case 's':
				testmore++;
				break;
			default:
				sprintf(Error,"%s (cm1)",nl_msg(1,"unknown key letter"));
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
			{	sprintf(Error,"%s (cm2)",nl_msg(3,"key letter twice"));
				fatal(Error);  }
			argv[i] = 0;
		}
		else num_files++;

	if(num_files == 0)
		{ sprintf(Error,"%s (cm3)",nl_msg(4,"missing file arg"));
		fatal(Error);  }

	/*	If envoked as "sact", set flag
		otherwise executed as "unget".
	*/
	if (equal(sname(argv[0]),"sact")) {
		cmd = 1;
		HADS = 0;
	}

	if (!HADS)
		verbosity = -1;
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if (p=argv[i])
			do_file(p,unget);
	exit(Fcnt ? 1 : 0);

}


unget(file)
char *file;
{
	extern	char had_dir, had_standinp;
	extern	char *Sflags[];
	int	i, status;
	char	gfilename[FILESIZE];
	char	str[BUFSIZ];
	char	*sid_ba();
	struct	pfile *pp, *edpfile();

	if (setjmp(Fjmp))
		return;

	/*	Initialize packet, but do not open SCCS file.
	*/
	sinit(&gpkt,file,0);
	gpkt.p_stdout = stdout;
	gpkt.p_verbose = verbosity;

	copy(auxf(gpkt.p_file,'g'),gfilename);
	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	/*	If envoked as "sact", call catpfile() and return.
	*/
	if (cmd) {
		catpfile(&gpkt);
		return;
	}

	if (lockit(auxf(gpkt.p_file,'z'),2,getpid()))
		{ sprintf(Error,"%s (cm4)",nl_msg(5,"cannot create lock file"));
		fatal(Error);  }
	pp = edpfile(&gpkt,&sid);
	if (gpkt.p_verbose) {
		sid_ba(&pp->pf_nsid,str);
		fprintf(gpkt.p_stdout,"%s\n",str);
	}

	/*	If the size of the q-file is greater than zero,
		rename the q-file the p-file and remove the
		old p-file; else remove both the q-file and
		the p-file.
	*/
	if (Szqfile)
		rename(auxf(gpkt.p_file,'q'),Pfilename);
	else {
		xunlink(Pfilename);
		xunlink(auxf(gpkt.p_file,'q'));
	}
	ffreeall();
	unlockit(auxf(gpkt.p_file,'z'),getpid());

	/*	A child is spawned to remove the g-file so that
		the current ID will not be lost.
	*/
	if (!HADN) {
		if ((i = fork()) < 0)
			fatal((nl_msg(6,"cannot fork, try again")));
		if (i == 0) {
			setuid(getuid());
			unlink(gfilename);
			exit(0);
		}
		else {
			wait(&status);
		}
	}
}


struct pfile *
edpfile(pkt,sp)
struct packet *pkt;
struct sid *sp;
{
	static	struct pfile goodpf;
	char	*user, *logname();
	char	line[BUFSIZ];
	struct	pfile pf;
	int	cnt, name;
	FILE	*in, *out, *fdfopen();

	cnt = -1;
	name = 0;
	user = logname();
	zero(&goodpf,sizeof(goodpf));
	in = xfopen(auxf(pkt->p_file,'p'),0);
	out = xfcreat(auxf(pkt->p_file,'q'),0644);
	while (fgets(line,sizeof(line),in) != NULL) {
		pf_ab(line,&pf,1);
		if (equal(pf.pf_user,user)) {
			name++;
			if (sp->s_rel == 0) {
				if (++cnt) {
					fclose(out);
					fclose(in);
					sprintf(Error,"%s (un1)",nl_msg(7,"SID must be specified"));
					fatal(Error);
				}
				goodpf = pf;
				continue;
			}
			else if (sp->s_rel == pf.pf_nsid.s_rel &&
				sp->s_lev == pf.pf_nsid.s_lev &&
				sp->s_br == pf.pf_nsid.s_br &&
				sp->s_seq == pf.pf_nsid.s_seq) {
					goodpf = pf;
					continue;
			}
		}
		fputs(line,out);
	}
	fflush(out);
	fstat(fileno(out),&Statbuf);
	Szqfile = Statbuf.st_size;
	copy(auxf(pkt->p_file,'p'),Pfilename);
	fclose(out);
	fclose(in);
	if (!goodpf.pf_user[0])
		if (!name)
			{ sprintf(Error,"%s (un2)",nl_msg(8,"login name not in p-file"));
			fatal(Error);  }
		else { sprintf(Error,"%s (un3)",nl_msg(9,"specified SID not in p-file"));
		fatal(Error);  }
	return(&goodpf);
}


/* clean_up() only called from fatal().
*/
clean_up(n)
{
	/*	Lockfile and q-file only removed if lockfile
		was created by this process.
	*/
	if (mylock(auxf(gpkt.p_file,'z'),getpid())) {
		unlink(auxf(gpkt.p_file,'q'));
		ffreeall();
		unlockit(auxf(gpkt.p_file,'z'),getpid());
	}
}


catpfile(pkt)
struct packet *pkt;
{
	int c;
	FILE *in;

	if(!(in = fopen(auxf(pkt->p_file,'p'),"r")))
		{ fprintf(stderr,nl_msg(10,"No outstanding deltas for: %s"),pkt->p_file);
	 	  fprintf(stderr,"\n");	}
	else {
		while ((c = getc(in)) != EOF)
			putc(c,pkt->p_stdout);
		fclose(in);
	}
}
