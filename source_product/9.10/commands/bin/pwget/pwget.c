static char *HPUX_ID = "@(#) $Revision: 56.3 $";
#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>

int atoi(), getopt();
char *arg0;

#define GRGET	1
#define PWGET	2

int mode;			/* Mode of operation, either GRGET or PWGET. */

main(argc, argv)
int argc;
char **argv;
{
    int printgr(), printpw();
    int c;
    extern char *optarg;
    extern int optind;
    struct group *grp;
    struct passwd *pwd;
    int anyflag = 0,
	gflag = 0,
	nflag = 0,
	uflag = 0;
    int gid, uid;
    char *name, *opts;

    mode = 0;

    if ((arg0 = strrchr(argv[0], '/')) == NULL)
	arg0 = argv[0];
    else
	arg0++;			/* Start after the '/' */

    if (strcmp(arg0, "grget") == 0)
	mode = GRGET;
    else if (strcmp(arg0, "pwget") == 0)
	mode = PWGET;
    else
	usage();

    switch(mode)
    {
case GRGET:
	setgrent();
	opts = "g:n:";
	break;
case PWGET:
	setpwent();
	opts = "u:n:";
	break;
    }

    while ((c = getopt(argc, argv, opts)) != EOF)
    {
	switch (c)
	{
    case 'g':
	    if (anyflag != 0)
		usage();

	    gflag++;
	    anyflag++;
	    gid = atoi(optarg);
	    break;
    case 'n':
	    if (anyflag != 0)
		usage();

	    nflag++;
	    anyflag++;
	    name = optarg;
	    break;
    case 'u':
	    if (anyflag != 0)
		usage();

	    uflag++;
	    anyflag++;
	    uid = atoi(optarg);
	    break;
    case '?':
	    usage();
	    break;
	}
    }

    if (*argv[optind] != '\0')
	usage();

    if (gflag)
    {
	if ((grp = getgrgid(gid)) != NULL)
	    printgr(grp);
	else
	    exit(1);
    }
    else if (nflag)
    {
	if (mode == GRGET)
	{
	    if ((grp = getgrnam(name)) != NULL)
		printgr(grp);
	    else
		exit(1);
	}
	else if (mode == PWGET)
	{
	    if ((pwd = getpwnam(name)) != NULL)
		printpw(pwd);
	    else
		exit(1);
	}
    }
    else if (uflag)
    {
	if ((pwd = getpwuid(uid)) != NULL)
	    printpw(pwd);
	else
	    exit(1);
    }
    else
    {
	if (mode == GRGET)
	{
	    while ((grp = getgrent()) != NULL)
		printgr(grp);
	}
	else if (mode == PWGET)
	{
	    while ((pwd = getpwent()) != NULL)
		printpw(pwd);
	}
    }

    exit(0);
}


usage()
{
    switch(mode)
    {
case GRGET:
	fprintf(stderr, "usage: %s [ -g gid | -n name ]\n", arg0);
	break;
case PWGET:
	fprintf(stderr, "usage: %s [ -n name | -u uid ]\n", arg0);
	break;
default:
	fprintf(stderr, "Call with either grget or pwget\n");
	break;
    }

    exit(2);
}


printgr(g)
struct group *g;
{
    char **chr;
    int comma;

    if (g != NULL)
    {
	printf("%s:%s:%d:", g->gr_name, g->gr_passwd, g->gr_gid);

	for (comma = 0, chr = g->gr_mem; *chr != NULL; chr++)
	    printf("%s%s", ((comma==0)?comma++,"":","), *chr);

	printf("\n");
    }
}


printpw(p)
struct passwd *p;
{
    if (p != NULL)
    {
	printf("%s:%s", p->pw_name, p->pw_passwd);

	if (strcmp(p->pw_age, "") != 0)
	    printf(",%s", p->pw_age);

	printf(":%d:%d:%s:%s:%s\n", p->pw_uid, p->pw_gid,
		p->pw_gecos, p->pw_dir, p->pw_shell);
    }
}

