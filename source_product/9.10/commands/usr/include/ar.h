/* @(#) $Revision: 64.1 $ */      
#ifndef _AR_INCLUDED /* allow multiple inclusions */
#define _AR_INCLUDED

#define ARMAG	"!<arch>\n"
#define SARMAG	8
#define ARFMAG	"`\n"

#define AR_NAME_LEN	16

struct ar_hdr	/* archive file member header - printable ascii */
{
	char	ar_name[16];	/* file member name - `/' terminated */
	char	ar_date[12];	/* file member date - decimal */
	char	ar_uid[6];	/* file member user id - decimal */
	char	ar_gid[6];	/* file member group id - decimal */
	char	ar_mode[8];	/* file member mode - octal */
	char	ar_size[10];	/* file member size - decimal */
	char	ar_fmag[2];	/* ARFMAG - string to end header */
};
#endif /* _AR_INCLUDED */
