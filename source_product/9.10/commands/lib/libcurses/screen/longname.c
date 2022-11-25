/* @(#) $Revision: 66.1 $ */    

/*
 *	This routine returns the long name of the terminal.
 */
char *
longname()
{
	extern char ttytype[];
	extern char *strrchr();
	char *cp;
	
	/*
	 * Find the last '|' in the ttytype string.
	 */
	cp = strrchr(ttytype, '|');

	/*
	 * Return a pointer to the long name (which is the first name
	 * if no '|' was found).
	 */
	return cp == (char *)0 ? ttytype : cp + 1;
}
