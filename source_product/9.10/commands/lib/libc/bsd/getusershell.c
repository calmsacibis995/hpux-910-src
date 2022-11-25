/* @(#) $Revision: 66.5 $ */

#ifdef _NAMESPACE_CLEAN
#define getusershell _getusershell
#define setusershell _setusershell
#define endusershell _endusershell
#define fopen _fopen
#define fstat _fstat
#define fclose _fclose
#define fgets _fgets
#define fileno _fileno
#define calloc _calloc
#ifdef _ANSIC_CLEAN
#define malloc _malloc
#define free _free
#endif /*  _ANSIC_CLEAN */
#ifdef __lint
#define isspace _isspace
#endif  /* __lint */
#endif  /* _NAMESPACE_CLEAN */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>

#define SHELLS "/etc/shells"

/*
 * Do not add local shells here.  They should be added in /etc/shells
 */
static char *okshells[] = {
        "/bin/sh",
        "/bin/rsh",
        "/bin/ksh",
        "/bin/rksh",
        "/bin/csh",
        "/bin/pam",
        "/bin/posix/sh",
        "/usr/bin/keysh",
        0
};

static char **shells, *strings;
static char **curshell = NULL;
extern char **initshells();

/*
 * Get a list of shells from SHELLS, if it exists.
 */

#ifdef _NAMESPACE_CLEAN
#undef getusershell
#pragma _HP_SECONDARY_DEF _getusershell getusershell
#define getusershell _getusershell
#endif /* _NAMESPACE_CLEAN */

char *
getusershell()
{
	char *ret;

	if (curshell == NULL)
		curshell = initshells();
	ret = *curshell;
	if (ret != NULL)
		curshell++;
	return (ret);
}

#ifdef _NAMESPACE_CLEAN
#undef endusershell
#pragma _HP_SECONDARY_DEF _endusershell endusershell
#define endusershell _endusershell
#endif /* _NAMESPACE_CLEAN */

endusershell()
{
	
	if (shells != NULL)
		free((char *)shells);
	shells = NULL;
	if (strings != NULL)
		free(strings);
	strings = NULL;
	curshell = NULL;
}

#ifdef _NAMESPACE_CLEAN
#undef setusershell
#pragma _HP_SECONDARY_DEF _setusershell setusershell
#define setusershell _setusershell
#endif /* _NAMESPACE_CLEAN */

setusershell()
{

	curshell = initshells();
}

static char **
initshells()
{
	register char **sp, *cp;
	register FILE *fp;
	register unsigned int min_st_size;
	struct stat statb;
	extern char *malloc(), *calloc();

	if (shells != NULL)
		free((char *)shells);
	shells = NULL;
	if (strings != NULL)
		free(strings);
	strings = NULL;
	if ((fp = fopen(SHELLS, "r")) == (FILE *)0)
		return(okshells);
	if (fstat(fileno(fp), &statb) == -1) {
		(void)fclose(fp);
		return(okshells);
	}
	/* Fix for file size less than three bytes in length */
	min_st_size = (statb.st_size < 3) ? 3 : statb.st_size;
	if ((strings = malloc((unsigned)min_st_size)) == NULL) {
		(void)fclose(fp);
		return(okshells);
	}
	shells = (char **)calloc((unsigned)min_st_size / 3, sizeof (char *));
	if (shells == NULL) {
		(void)fclose(fp);
		free(strings);
		strings = NULL;
		return(okshells);
	}
	sp = shells;
	cp = strings;
	while (fgets(cp, MAXPATHLEN + 1, fp) != NULL) {
		while (*cp != '#' && *cp != '/' && *cp != '\0')
			cp++;
		if (*cp == '#' || *cp == '\0')
			continue;
		*sp++ = cp;
		while (!isspace(*cp) && *cp != '#' && *cp != '\0')
			cp++;
		*cp++ = '\0';
	}
	*sp = (char *)0;
	(void)fclose(fp);
	return (shells);
}
