/* $Revision: 66.2 $ */
/*
STR2MODE - convert an ll-type mode string
	"-rwSr-xr--"
	or an octal string representation
	"04754"
to its octal value
	04754
returns > -1 on error
result should be cast to ushort for use with chmod(2), etc.
*/

#include <errno.h>
#include <sys/stat.h>

extern int errno;

int
str2mode(mode_string)
char *mode_string;
{
    int mode = 0;
    switch (mode_string[0] >= '0' && mode_string[0] <= '7') {
    case 1:	/* numeric representation */
	if (sscanf(mode_string, "%o", &mode) != 1)
	    return -1;
	break;
    case 0:	/* "ll" style handling */
	if (mode_string[0] == 'H')	/* cover the HP CDF case */
	    mode |= S_ISUID;
    /* do the OWNER stuff */
	switch(mode_string[1]) {
	case '-':
	    break;
	case 'r':
	    mode |= S_IRUSR;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
	switch(mode_string[2]) {
	case '-':
	    break;
	case 'w':
	    mode |= S_IWUSR;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
	switch(mode_string[3]) {
	case '-':
	    break;
	case 'x':
	    mode |= S_IXUSR;
	    break;
	case 'S':
	    mode |= S_ISUID;
	    break;
	case 's':
	    mode |= S_ISUID | S_IXUSR;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
    /* Now do GROUP */
	switch(mode_string[4]) {
	case '-':
	    break;
	case 'r':
	    mode |= S_IRGRP;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
	switch(mode_string[5]) {
	case '-':
	    break;
	case 'w':
	    mode |= S_IWGRP;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
	switch(mode_string[6]) {
	case '-':
	    break;
	case 'x':
	    mode |= S_IXGRP;
	    break;
	case 'S':
	    mode |= S_ISGID;
	    break;
	case 's':
	    mode |= S_ISGID | S_IXGRP;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
    /* Now do OTHER */
	switch(mode_string[7]) {
	case '-':
	    break;
	case 'r':
	    mode |= S_IROTH;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
	switch(mode_string[8]) {
	case '-':
	    break;
	case 'w':
	    mode |= S_IWOTH;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
	switch(mode_string[9]) {
	case '-':
	    break;
	case 'x':
	    mode |= S_IXOTH;
	    break;
	case 'T':
	    mode |= S_ISVTX;
	    break;
	case 't':
	    mode |= S_ISVTX | S_IXOTH;
	    break;
	default:
	    errno = EINVAL;
	    return -1;
	}
    }
    return mode;
}
