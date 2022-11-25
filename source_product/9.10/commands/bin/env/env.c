static char *HPUX_ID = "@(#) $Revision: 70.2 $";
/*
 *	env [ -i ] [ - ] [ name=value ]... [command arg...]
 *	set environment, then execute command (or print environment)
 *	- says start fresh, otherwise merge with inherited environment
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <nl_types.h>
#include <errno.h>
#define	NL_SETN	1

char	**newenv;
char	*nullp = NULL;

extern	char **environ;
extern	errno;
extern	char *sys_errlist[];
char	*nvmatch(), *strchr();
void	exit();

main(argc, argv, envp)
register char **argv, **envp;
{
	unsigned nent=0,elsize=sizeof(newenv);
	char **p;
	char c;
	extern int optind;
	int i,err=0;
	nl_catd catd;

	catd = catopen("env",0);
	do {
		while ((c=getopt(argc,argv,"i"))!=EOF)
			switch(c) {
				case 'i': envp = &nullp; break;
				case '?': err++;
			}
		if (optind<argc && strcmp(argv[optind],"-")==0) {
			envp = &nullp;
			optind++;
			c='x';
		}
	} while (c != EOF);
	if (err) {
		fputs(catgets(catd,NL_SETN,1,"usage: env [-] [-i] [name=value] ... [utility [argument ...]]\n"),stderr);
		exit(1);
	}
/*
 *	Count the number of '=' characters in the arguments to estimate
 *	the maximum number of newenv entries to allocate due to
 *	arguments (hokey, but it works!)
 */
	for (i=optind;i<argc;i++)
		if (strchr(argv[i],'=') != NULL) nent += 1;
		
/*
 *	Add number of entries in inherited environment (if any).
 */
	for (p=envp; *p != NULL; p++) nent += 1;

	newenv = (char **)calloc(nent+(unsigned) 1,elsize);
		
	if (newenv == NULL) {
		(void) fputs(catgets(catd,NL_SETN,2,"insufficient memory\n"), stderr);
		exit(1);
	}

	newenv[nent] = NULL;	/* last pointer in newenv is NULL */

	for (; *envp != NULL; envp++)				/* add inherited values */
		if (strchr(*envp, '=') != NULL)
			addname(*envp);

	i=optind;
	while (argv[i] != NULL && strchr(argv[i], '=') != NULL)	/* add (or merge) arguments */
		addname(argv[i++]);

	if (argv[i] == NULL)
		print(0); /* doesn't return */
	else {
		int exitval;
		environ = newenv;
		(void) execvp(argv[i], &(argv[i]));
		switch(errno) {
		case EINVAL:
		case EACCES:
			exitval=126;
			break;
		case ENOTDIR:
		case ENOENT:
			exitval=127;
			break;
		default:
			exitval=1;
		}
		(void) fputs(sys_errlist[errno], stderr);
		(void) fputs(": ", stderr);
		(void) fputs(argv[i], stderr);
		(void) putc('\n', stderr);
		exit(exitval);
	}

	return(0);
}

addname(arg)
register char *arg;
{
	register char **p;

	for (p = newenv; *p != NULL; p++) {
		if (nvmatch(arg, *p) != NULL)
			break;
	}
	*p = arg;
}

print(code)
{
	register char **p = newenv;

	while (*p != NULL)
		(void) puts(*p++);
	exit(code);
}

/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of s2, else NULL
 */

char *
nvmatch(s1, s2)
register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++ == '=')
			return(s2);
	if (*s1 == '\0' && *(s2-1) == '=')
		return(s2);
	return(NULL);
}
