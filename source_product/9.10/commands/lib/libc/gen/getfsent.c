/* @(#) $Revision: 64.7 $ */      

#ifdef _NAMESPACE_CLEAN
#  ifdef __lint
#  define isdigit _isdigit
#  define isspace _isspace
#  endif /* __lint */
#define fgets _fgets
#define strncmp _strncmp
#define strcpy _strcpy
#define strcmp _strcmp
#define fopen _fopen
#define fclose _fclose
#define strlen _strlen
/* defined here */
#define getfsent _getfsent
#define setfsent _setfsent
#define endfsent _endfsent
#define getfsspec _getfsspec
#define getfstype _getfstype
#define getfsfile _getfsfile
#endif

#ifdef HP_NFS
#define COMMENT -2
#include <string.h>
static char *findopt(), *hasopt();
#endif HP_NFS

#include <checklist.h>
#include <stdio.h>
#include <ctype.h>

#ifdef hpux
#   ifdef BUFSIZ
#      undef BUFSIZ
#   endif
#   define BUFSIZ 1024
#endif

static FILE *fs_file = (FILE *)0;

/*
 * This routine is called by checklistscan:
 *     1. advances pointer till the end of the current character field
 *        and null terminate the current field
 *     2. skip blanks and tabs to next field and returns pointer at the 
 *	  beginning of the new field.  If the new field is the comment
 *	  field than fsskip return 0;
 */ 
static char *
fsskip(p)
	register char *p;
{
	while (*p && *p != ' ' && *p != '\n' && *p != '\t')
		++p;
	if (*p)
		*p++ = 0;  
	while (*p && ((*p == ' ') || (*p == '\t')) )
		++p;
	if ( *p == '#' || *p == '\n')
		*p = 0;
	return p;
}


/*
 * This routine is called by checklistscan to scan the last two fields
 * fs_passno and fs_freq where they can be specified as
 *      fs_passno   [fs_freq]    [#  .....]
 * 
 */
static char *
fsdigit(backp, string)
	int *backp;
	char *string;
{
	register int value = 0;
	register char *cp;

	for (cp = string; *cp && isdigit(*cp); cp++) {
		value *= 10;
		value += *cp - '0';
	}
	*backp = value;

	while ( *cp && ((*cp==' ') || (*cp=='\t')) )  /* fs_freq is specified */
		cp++;
	if (*cp == '#' || *cp == '\n')
		*cp = 0;
	return cp;
}

/*
 * checklistscan get information from a file system entry
 * and save it in the structure checklist.  This routine returns
 *	1:   if there is only fs_spec present 
 *    >=5:   if there are all fields + [fs_freq] present
 *	0:   if that file system entry is invalid. i.e. number of fields
 *	     which are present in one entry is neither 1 nor >=5
 */
static
checklistscan(fs)
	struct checklist *fs;
{
	static char line[BUFSIZ+1];
	register char *cp;
#ifdef HP_NFS
	static char cbuf[BUFSIZ];
	char *spec_file, *type, *options;
#endif HP_NFS

	/*
	 * Skip comments, even if not HP_NFS
	 */
	do
	{
	    if ((cp = fgets(line, BUFSIZ, fs_file)) == NULL)
		    return EOF;
	} while (*cp == '#'); /* skip comments */

#ifdef HP_NFS
	spec_file = cp;
	cp = fsskip(cp);

	if (strncmp(spec_file, "/dev/", 5) == 0)
	{
	    if ((strncmp(spec_file+5, "rdsk", 4) == 0) ||
		(strncmp(spec_file+5, "rhd", 3) == 0))
	    {		/* /dev/rdsk/? or /dev/rhd?, put in fs_spec. */
		fs->fs_spec = spec_file;
		(void)strcpy(cbuf, "/dev/");
		(void)strcpy(cbuf+5, spec_file + 6);
		fs->fs_bspec = cbuf;
	    }
	    else
	    {		/* /dev/dsk/? or /dev/hd?, put in fs_bspec. */
		fs->fs_bspec = spec_file;
		(void)strcpy(cbuf, "/dev/r");
		(void)strcpy(cbuf+6, spec_file + 5);
		fs->fs_spec = cbuf;
	    }
	}
	else
	{		/* Don't know what it is, null out fs_bspec. */
	    fs->fs_spec = spec_file;
	    fs->fs_bspec = "";
	}

	if (*cp == 0)
		return 1;     /* minimal entry: 1 field: fs_spec */
#else not HP_NFS
	fs->fs_spec = cp;
	if (*(cp = fsskip(cp)) == 0)
		return 1;     /* minimal entry: 1 field: fs_spec */

	fs->fs_bspec = cp;
	if (*(cp = fsskip(cp)) == 0)
	        return 0;     /* invalid entry */
#endif not HP_NFS

	fs->fs_dir = cp;
	if (*(cp = fsskip(cp)) == 0)
	        return 0;     /* invalid entry */

#ifdef HP_NFS
	type = cp;
	if (*(cp = fsskip(cp)) == 0)
	        return 0;     /* invalid entry */
	options = cp;
	cp = fsskip(cp);

	if (strcmp(type, "hfs") == 0)
	{
	    if (hasopt(options, "ro") != NULL)
		fs->fs_type = "ro";
	    else if ((hasopt(options, "rw") != NULL) ||
		     (hasopt(options, "defaults") != NULL))
		fs->fs_type = "rw";
	    else
		fs->fs_type = "xx";
	}
	else if (strcmp(type, "swap") == 0)
	{
	    fs->fs_type = "sw";
	}
	else
	{
	    fs->fs_type = "xx";
	}
#else not HP_NFS
	fs->fs_type = cp;
	if (*(cp = fsskip(cp)) == 0)
	        return 0;     /* invalid entry */
#endif not HP_NFS

#ifdef HP_NFS
	if (*cp == 0)
	    fs->fs_freq = -1;
	else
	    cp = fsdigit(&fs->fs_freq, cp);

	if (*cp == 0)
	    fs->fs_passno = -1;
	else
	    cp = fsdigit(&fs->fs_passno, cp);

	return 6;	      /* Always return as valid entry. */
#else not HP_NFS
	if (*(cp = fsdigit(&fs->fs_passno, cp)) == 0)
		return 5;
	if (*(cp = fsdigit(&fs->fs_freq, cp)) == 0)
		return 6;
	return 0;	      /* invalid entry: has >6 fields */
#endif not HP_NFS
}

#ifdef HP_NFS

static char *
findopt(p)
	char **p;
{
	char *cp = *p;
	char *retstr;

	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && *cp != ',')
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = cp;
	return retstr;
}

static char *
hasopt(str, opt)
	register char *str;
	register char *opt;
{
	char tmpopts[256];

	char *f, *opts;
	int optlen = strlen(opt);

	strcpy(tmpopts, str);
	opts = tmpopts;
	for (f = findopt(&opts); *f; f = findopt(&opts)) {
		if ((strncmp(opt, f, optlen) == 0) &&
			    ((f[optlen] == '\0') || (f[optlen] == ',')))
		    return f - tmpopts + str;
	} 
	return NULL;
}
#endif HP_NFS

/*
 * This routines reset file pointer which points to /etc/checklist:
 *      It closes the checklist file and reopen it 
 */	

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setfsent
#pragma _HP_SECONDARY_DEF _setfsent setfsent
#define setfsent _setfsent
#endif

setfsent()
{
	if (fs_file)
		endfsent();
	if ((fs_file = fopen(CHECKLIST, "r")) == NULL) {
		fs_file = 0;
		return 0;
	}
	return 1;
}

/* This routine closes the /etc/checklist file */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef endfsent
#pragma _HP_SECONDARY_DEF _endfsent endfsent
#define endfsent _endfsent
#endif

endfsent()
{
	if (fs_file) {
		fclose(fs_file);
		fs_file = 0;
	}
	return 1;
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getfsent
#pragma _HP_SECONDARY_DEF _getfsent getfsent
#define getfsent _getfsent
#endif

struct checklist *
getfsent()
{
	int nfields;
	static struct checklist fs;

	if (fs_file == 0 && setfsent() == 0)
		return (struct checklist *)0;

	nfields = checklistscan(&fs);

	if (nfields == EOF ||  nfields == 0 || nfields >1 && nfields <5)
		return (struct checklist *)0;

	if (nfields == 1)		/* minimal entry */
	{
		fs.fs_bspec = 0;
		fs.fs_dir =  0;
		fs.fs_type = 0;
		fs.fs_passno = -1;
		fs.fs_freq = -1;
	}
	return &fs;
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getfsspec
#pragma _HP_SECONDARY_DEF _getfsspec getfsspec
#define getfsspec _getfsspec
#endif

struct checklist *
getfsspec(name)
	char *name;
{
	register struct checklist *fsp;

	if (setfsent() == 0)	/* start from the beginning */
		return (struct checklist *)0;
	while((fsp = getfsent()) != 0)
		if (strcmp(fsp->fs_spec, name) == 0)
			return fsp;
	return (struct checklist *)0;
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getfsfile
#pragma _HP_SECONDARY_DEF _getfsfile getfsfile
#define getfsfile _getfsfile
#endif

struct checklist *
getfsfile(name)
	char *name;
{
	register struct checklist *fsp;

	if (setfsent() == 0)	/* start from the beginning */
		return (struct checklist *)0;
	while ((fsp = getfsent()) != 0)
		if (strcmp(fsp->fs_dir, name) == 0)
			return fsp;
	return (struct checklist *)0;
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getfstype
#pragma _HP_SECONDARY_DEF _getfstype getfstype
#define getfstype _getfstype
#endif

struct checklist *
getfstype(type)
	char *type;
{
	register struct checklist *fsp;

	if (setfsent() == 0)
		return (struct checklist *)0;
	while ((fsp = getfsent()) != 0)
		if (strcmp(fsp->fs_type, type) == 0)
			return fsp;
	return (struct checklist *)0;
}
