


/*

 NAME
    General utils that are not nls specific.

 SYNOPSIS
    See individual functions.

 DESCRIPTION
    See individual functions.

 RETURN VALUE

*/

/*
 * get the beginning and end of a string 
 *	(i.e. bypass beginning and trailing blanks).
 */
begin_end(s, len, pBeg, pEnd)
char	*s;
int	len, *pBeg, *pEnd;
{
	char	*wrk;

	for (*pBeg = 0, wrk = s; *wrk == ' '  &&  wrk < s + len; wrk++)
		*pBeg += 1;

	if (wrk == s + len)
		return(-1);

	for (*pEnd = len-1, wrk = s + len - 1; *wrk == ' '  &&  wrk > s; wrk--)
		*pEnd -= 1;

	return(0);
}
