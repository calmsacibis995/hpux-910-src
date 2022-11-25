/* @(#) $Revision: 56.1 $ */     
#include <stdio.h>
#include <varargs.h>

/*
 *	This routine implements vsscanf (nonportably) until such time
 *	as one is available in the system (if ever).
 */

vsscanf(buf, fmt, ap)
char	*buf;
char	*fmt;
va_list	ap;
{
	FILE *dummy;
	int rv;

	/* Open /dev/null to get entry in _iob array. */

	dummy = fopen("/dev/null","r");

	if ( dummy != (FILE *)0 ) {
	    dummy->_base = dummy->_ptr = (unsigned char *)buf;
	    dummy->_cnt = strlen(buf);
	    rv = _doscan(dummy, fmt, ap);
	    fclose(dummy);
	} else {
	    rv = 0;
	}

	return rv;
}
