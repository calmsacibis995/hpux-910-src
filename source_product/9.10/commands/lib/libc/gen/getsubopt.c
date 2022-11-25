/* @(#) $Revision: 70.1 $ */
/*
 * getsubopt() parses suboptions in a flag argument that was initially
 * parsed by getopt().  These suboptions are separated by commas and may
 * consist of either a single token or a token/value pair spearated by an
 * equal sign.  Since commas delimit suboptions in the option string, they
 * are not allowed to be part of the suboption or the value of a suboption.
 *
 * A command that uses this syntax is mount(1M), which allows the user
 * to specify mount parameters with the -o option as follows:
 * 
 *    mount -o rw,hard,bg,wsize=1024 speed:/usr /usr
 */

#ifdef _NAMESPACE_CLEAN
#  define strchr	_strchr
#  define strcmp	_strcmp
#  define getsubopt	_getsubopt
#endif /*_NAMESPACE_CLEAN */

#include <string.h>
#include <stdlib.h>

#ifdef _NAMESPACE_CLEAN
#  undef getsubopt _getsubopt
#  pragma _HP_SECONDARY_DEF _getsubopt getsubopt
#  define getsubopt _getsubopt
#endif /* _NAMESPACE_CLEAN */

int getsubopt(optionp, tokens, valuep)
char **optionp;
char **tokens;
char **valuep;
{
    int token_num = 0;
    char *ptr;

    ptr = *optionp;
    while((*ptr != '\0') && (*ptr != ','))
	ptr++;

    if (*ptr != '\0')
        *ptr++ = '\0';

    /*
     * "*optionp" now points to a token/value pair.  Separate out the
     * token portion now.  Let "vp" point to where the '=' sign was.
     *
     * "ptr" points to the next set of options to be processed.
     */
    *valuep = strchr(*optionp, '=');
    if (*valuep != (char *)NULL)
        **valuep = '\0';
    
    while (tokens[token_num] != NULL)
    {
	if (strcmp(tokens[token_num], *optionp) == 0)
	{
	    if (*valuep != (char *)NULL)
	    {
		/* Replace the '=' sign.  Not sure if this is necessary */
		**valuep = '=';
	        *valuep = *valuep + 1;
	    }
	    *optionp = ptr;
	    return token_num;
	}
	token_num++;
    }

    /*
     * Not found
     */
    if (*valuep != (char *) NULL)
	**valuep = '=';
    *valuep = *optionp;
    *optionp = ptr;
    return -1;
}
