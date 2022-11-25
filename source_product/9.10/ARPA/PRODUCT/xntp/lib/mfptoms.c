/* $Header: mfptoms.c,v 1.2.109.2 94/10/28 17:40:48 mike Exp $
 * mfptoms - Return an asciized signed LONG fp number in milliseconds
 */
# include "ntp_fp.h"

char   *
mfptoms (fpi, fpf, ndec)
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

    return dolfptoa (fpi, fpf, isneg, ndec, 1);
}
