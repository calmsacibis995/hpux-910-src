static char *HPUX_ID = "@(#) $Revision: 70.2 $";
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/param.h>
#include <pwd.h>

#ifdef	FSS
#include <signal.h>
#include <sys/fss.h>
#include <fsg.h>
#endif

#include <locale.h>
#include <nl_types.h>

extern int  optind;   /* for getopt */
extern char *optarg;
char *prog;

#ifdef NL_SETN
#undef NL_SETN
#endif
#define NL_SETN NL_SETD

main(argc,argv)
int	argc;
char	*argv[];
{
    extern uid_t getuid(), geteuid();
    extern gid_t getgid(), getegid();
    char *pwname();
    char *grname();
    struct passwd *pw;

    int c;
    uid_t uid, euid, puid;
    gid_t gid, egid, pgid, gidset[NGROUPS];
    char t = '\0';
    char *user;
    int	 n = 0, uflag = 0;
    int	 r = 0;
    int i, ngroups = NGROUPS, ngrps, first = 1;

    nl_catd nls_catalog;

    if (prog = strrchr(argv[0], '/')) 
	prog++;
    else prog = argv[0];

    if (!setlocale(LC_ALL, "")) {
	fprintf(stderr,
		"%s: Cannot set locale, continuing with \"C\" locale.\n",
		prog);
	nls_catalog = (nl_catd) -1;
    } else {
	nls_catalog = catopen("id", 0);
    }

    while ((c = getopt(argc, argv, "Ggnru")) != EOF)
	switch (c)
	{
	case 'G':
	case 'g': 
	case 'u': 
	    t = c;
	    break;
	case 'n': 
	    n = 1;
	    break;
	case 'r': 
	    r = 1;
	    break;
	}

    /* check for "user" argument, and superuser privileges */
    if((user = argv[optind]) != NULL && geteuid() == 0) {
	if((pw = getpwnam(user)) == NULL) {
	    fprintf(stderr,
		    catgets(nls_catalog, NL_SETN, 1, "Can't find user %s\n"),
		    user);
	    exit(1);
	}

	/*  Set uid and gid to that of the user whose name was given 
	 *  initgroups is used to set the appropriate suppl. gids.
	 */
	initgroups(user,pw->pw_gid);
	uid = euid = pw->pw_uid;
	gid = egid = pw->pw_gid;
	r = 1;
	++uflag;
    }
    else {                /* otherwise, use id's of calling process */
        uid  = getuid();
        gid  = getgid();
        euid = geteuid();
        egid = getegid();
    }

    /* check for supplementary group membership */
    ngrps = getgroups(ngroups,gidset); 

    
    switch(t)
    {
    case 0:
	printf(catgets(nls_catalog, NL_SETN, 2, "uid=%u(%s) gid=%u(%s)"),
	       uid,pwname(uid),gid,grname(gid));

	if (uid != euid)
	    printf(catgets(nls_catalog, NL_SETN, 3, " euid=%u(%s)"),
		   euid,pwname(euid));

	if (gid != egid)
	    printf(catgets(nls_catalog, NL_SETN, 4, " egid=%u(%s)"),
		   egid,grname(egid));

        /* Output supplementary GIDs if any, in same format */
        for(i = 0; i < ngrps; i++) {
	    if (gidset[i] != gid && gidset[i] != egid) 
		if(first) {
		    printf(catgets(nls_catalog, NL_SETN, 5, " groups=%u(%s)"),
			   gidset[i], grname(gidset[i]));
		    first = 0;
		}
		else    
		    printf(catgets(nls_catalog, NL_SETN, 6, ",%u(%s)"),
			   gidset[i], grname(gidset[i]));
	}
	
#ifdef	FSS
	pfsid();
#endif
	putchar('\n');
	break;
    case 'G':
	if (n)
	    printf("%s", grname(gid));
	else
	    printf("%u", gid);
	if (gid != egid) {
	    if (n)
		printf(" %s", grname(egid));
	    else
		printf(" %u", egid);
	}

	/* output suppl. GIDs in appropriate format */
	for(i = 0; i < ngrps; i++) {
	    if (gidset[i] != gid && gidset[i] != egid) {
		if (n)
		    printf(" %s", grname(gidset[i]));
		else
		    printf(" %u", gidset[i]);
	    }
	}
	putchar('\n');
	break;
    case 'g':
	pgid = r ? gid : egid;
	if (n)
	    printf("%s\n", grname(pgid));
	else
	    printf("%u\n", pgid);

#ifdef OLDFMT
	/* output suppl. GIDs in appropriate format */
	for(i = 0; i < ngrps; i++) {
	    if (gidset[i] != (r ? gid : egid)) {
		fputs(" ",stdout);
		fputs(!n ? ultoa(gidset[i]) : grname(gidset[i]),stdout);
	    }
	}
#endif
	break;
    case 'u':
	puid = r ? uid : euid;
	if (n)
	    printf("%s\n", pwname(puid));
	else
	    printf("%u\n", puid);
	break;
			
    }
    exit(0);
}

char *
pwname(id)
int	id;
{
    struct passwd *pw, *getpwuid();

    setpwent();
    return (pw = getpwuid(id)) ? pw->pw_name : "";
}

char *
grname(id)
int	id;
{
    struct group *gr, *getgrgid();

    setgrent();
    return (gr = getgrgid(id)) ? gr->gr_name : "";
}

#ifdef	FSS

int nofss = 0;		/* assume there is a fair share scheduler */

handler()
{
    nofss = 1;			/* no fair share scheduler */
}

char *
fgname(fsid)
int fsid;
{
    struct fsg *gr;

    return (gr = getfsgid(fsid)) != NULL ? gr->fs_grp : "";
}

pfsid()
{
    void (*oldsig)();
    int fsid;

    oldsig = signal(SIGSYS, handler);
    fsid = fss(FS_ID, 0);
    signal(SIGSYS, oldsig);	/* restore the previous value */

    if (nofss || fsid == -1)
	return;			/* no fair share scheduler */

    fputs(" fsid=", stdout);
    fputs(ultoa(fsid), stdout);
    fputc('(', stdout);
    fputs(fgname(fsid), stdout);
    fputc(')', stdout);
}
#endif	/* FSS */
