 /* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/cdf.c,v $
 *
 * $Revision: 66.1 $
 *
 * cdf.c - cdf handling functions
 *
 * DESCRIPTION
 *
 *	These function handle cdf functionality for the pax archiving
 *	tool. Functions are provided to manipulate the pathnames of
 *	the archivable cdf files.
 *	  
 * AUTHOR
 *
 *	Wolfgang Kapellen , HP GmbH Boeblingen   (kawo@hpbbln)
 *
 * Copyright (c) 1990 Hewlett Packard GmbH
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such 
 * forms. 
 *
 * $Log:	cdf.c,v $
 * Revision 66.1  90/07/10  08:15:53  08:15:53  kawo
 * initial checkin
 * 
 *
 *
 */

#ifndef lint
static char *ident = "$Id: buffer.c,v 1.2 89/02/12 10:04:02 mark Exp $";
static char *copyright = "Copyright (c) 1990 Hewlett Packard GmbH .\nAll rights reserved.\n";
#endif /* ! lint */

/* Headers */

#include "pax.h"

/* Function Prototypes */

#ifdef _STDC_
#define P(x)	x
#else
#define P(x)	()
#endif

#undef P

#undef TRACE_OUT


/* string_overlapcpy - deletes one or more character from a string
 *
 * DESCRIPTION
 *
 *	A character pointed to is deleted from the zero
 *	terminated string.
 *
 * PARAMETERS
 *
 *	char	*s1;	The position to copy to.
 *	char	*s2;	The position to copy from.
 *
 * RETURNS
 *
 *	The manipulated string.
 */

void	string_overlapcpy(s1,s2)
    	char 	*s1;
    	char	*s2;
{
#ifdef TRACE_OUT
	(void)fprintf(stderr,"in string_overlapcpy\n");
#endif

    for (; *s2; s1++,s2++)
	*s1 = *s2;
    *s1 = '\0';
}


/* remove_extra_slashes - deletes additional slashes from a string
 *
 * DESCRIPTION
 *
 *	A path may consist of subdir names terminated by slashes.
 * 	The number of slashes may differ and so the pathname has to
 *	be "normalized" by deleting additional slashes.
 *
 * PARAMETERS
 *
 *	char	*path;	The pointer to the path to check.
 *
 * RETURNS
 *
 *	The manipulated string.
 */

void	remove_extra_slashes(path)
    char 	*path;
{
    char 	*cp, *cp2;
    struct stat t, dest;

    void	string_overlapcpy();

#ifdef TRACE_OUT
	(void)fprintf(stderr,"in remove_extra_slashes %s\n", path);
#endif

    for (cp=path; *cp; cp++) {
	if (*cp == '/' && *(cp+1) == '/') {
	    for (cp2=cp+2; *cp2 == '/'; cp2++)
		;
	    string_overlapcpy(cp+1,cp2);
	}
    }
    return;
}


/* if_cdf_stat - checks if the given path is an cdf
 *
 * DESCRIPTION
 *
 *	The information if a file or directory is a hidden one,
 * 	can be get by the stat() function. But only if your own
 *	context is included. So the path has to be expanded with
 *	a plus and it must be tried again.
 *	Example : /dev/.	no  cdf stat(only if own context)
 *		  /dev+/.	yes cdf stat
 *	In case you don't know in which way the path is given, if
 *	with or without an +, you have to try it several times.
 *	If the names come from stdin, normally a filter is used and
 *	so you can presume a + if hidden directories are to be stored.
 *
 * PARAMETERS
 *
 *	char	*path;	The pointer to the path to check.
 *
 * RETURNS
 *
 *	returns (0) if stat() was unlucky by finding a CDF
 *	returns (1) if stat() results CDF with the given path
 *	returns (2) if stat() results CDF with expanded path
 */

int	if_cdf_stat(path)
    	char 	*path;
{
	struct stat	status, *buf;
	char		tmp_path[PATH_MAX + 2];

#ifdef TRACE_OUT
	(void)fprintf(stderr,"in if_cdf_path\n");
#endif

	buf = &status;

	if (!names_from_stdin) {
	  (void)strcpy(tmp_path, path);
	  (void)strcat(tmp_path, "+/.");
	  if ((stat(tmp_path, buf) == 0) &&
	      (S_ISCDF(buf->st_mode))       ) {
	      return(2);
	  }
	}

	(void)strcpy(tmp_path, path);
	(void)strcat(tmp_path, "/.");
	if ((stat(tmp_path, buf) == 0) &&
    	   (S_ISCDF(buf->st_mode))       ) {
    	   return(1);
	}

	return(0);
}


/* expand_cdf_path - checks path if cdfs are in there
 *
 * DESCRIPTION
 *
 *	If the path includes cdfs to be archived, without archiving
 *	the cdf itself, you don't know if it is a normal directory
 *	with a plus in its name or a cdf. So the pathname is expanded
 *	with an additional slash to mark an cdf.
 *	Example : /dev+/context1/rdsk	
 *		  /dev+//context1/rdsk
 *	This function searches for a plus in the pathname and then
 *	test if it is an cdf or not. If yes, a slash is included.
 *
 * PARAMETERS
 *
 *	char	*path;	The pointer to the path to check.
 *
 * RETURNS
 *
 *	The manipulated string.
 */

void	expand_cdf_path(path)
    	char 	*path;
{
	char	tmp_string[PATH_MAX+1];
	char	*to_plus, *to_tmp;

	void	remove_extra_slashes();
	int	if_cdf_stat();

	remove_extra_slashes(path);

#ifdef TRACE_OUT
	(void)fprintf(stderr,"entry expand_cdf_path %s\n", path);
#endif

	for ( to_plus = path, to_tmp = tmp_string;
	      *to_plus				 ;
	      to_plus++, to_tmp++                 ) {
	    *to_tmp = *to_plus;
	    *(to_tmp+1) = '\0';
	    if ( (*(to_plus+1) == '/') && (*to_plus == '+') ) {
#ifdef TRACE_OUT
	(void)fprintf(stderr,"in expand_cdf_path tmp_string %s\n", tmp_string);
#endif
	       if ( (if_cdf_stat(tmp_string) == 1) &&
		  ((strlen(tmp_string)+1) < PATH_MAX)      ) {
#ifdef TRACE_OUT
	(void)fprintf(stderr,"condition is true\n");
#endif
		  to_tmp++;
		  *to_tmp = '/';
		  *(to_tmp+1) = '\0';
	       } /* else warning */
	    }
	}
#ifdef TRACE_OUT
	(void)fprintf(stderr,"exit expand_cdf_path %s\n", tmp_string);
#endif

	(void)strcpy(path, tmp_string);

#ifdef TRACE_OUT
	(void)fprintf(stderr,"exit expand_cdf_path %s\n", path);
#endif
}


/* modify_last_name_in_path - checks if last name in path is an cdf
 *
 * DESCRIPTION
 *
 *	If the last name in path ia an cdf, the path is modified in
 *	relation to the set or unset -H option.
 *
 * PARAMETERS
 *
 *	char	*path;	The pointer to the path to check.
 *
 * RETURNS
 *
 *	The manipulated string.
 *	returns (0) if it is an cdf.
 *	returns (1) if it is not an cdf.
 */

int	modify_last_name_in_path(path)
    	char 	*path;
{
	char	*to_end;
	int	cdfstat;
	int	found_hidden;

	int	if_cdf_stat();

#ifdef TRACE_OUT
	(void)fprintf(stderr,"entry modify_last_name_in_path %s\n", path);
#endif

	found_hidden = 0;
	to_end       = path - 1 + strlen(path);

	/* delete trailing slashes */
	while ((to_end > path) && (*to_end == '/')) {
	      *to_end = '\0';
	      to_end--;
	}

	cdfstat = if_cdf_stat(path);
#ifdef TRACE_OUT
	(void)fprintf(stderr,"in modify_last_name_in_path %s\n", path);
#endif
	
	/* store hidden files and directories */
	if (f_hidden)
	   switch (cdfstat) {
	     case 0: /* it is not a hidden directory */
		break;
	     case 1: /* it is a hidden directory given by <name>+ */
		(void)strcat(path, "/");
		found_hidden++;
		break;
	     case 2: /* it is a hidden directory given by <name> */
		(void)strcat(path, "+/");
		found_hidden++;
		break;
	     default:
		break;
	}
	else
	   switch (cdfstat) {
	     case 0: /* it is not a hidden directory */
		break;
	     case 1: /* it is a hidden directory given by <name>+ */
		if (names_from_stdin) {
		   (void)strcat(path, "/");
		   found_hidden++;
		}
		else {
		   /* reduce path by deleting the + */
	      	   *to_end = '\0';
	      	   to_end--;
		}
		break;
	     case 2: /* it is a hidden directory given by <name> */
		break;
	     default:
		break;
	}
#ifdef TRACE_OUT
	(void)fprintf(stderr,"exit modify_last_name_in_path %s\n", path);
#endif
	return(found_hidden);
}


