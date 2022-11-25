/* $Header: mfptoa.c,v 1.2.109.2 94/10/28 17:39:20 mike Exp $
 * mfptoa - Return an asciized representation of a signed LONG fp number
 */
# include "ntp_fp.h"

char   *
mfptoa (fpi, fpf, ndec)
U_LONG  fpi;
U_LONG  fpf;
int     ndec;
{
    int     isneg;

    if (M_ISNEG (fpi, fpf))
        {
	isneg = 1;
	M_NEG (fpi, fpf);
        }
    else
	isneg = 0;

    return dolfptoa (fpi, fpf, isneg, ndec, 0);
}
