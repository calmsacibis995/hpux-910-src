/* $Header: fptoms.c,v 1.2.109.2 94/10/28 17:27:02 mike Exp $
 * fptoms - return an asciized s_fp number in milliseconds
 */
# include "ntp_fp.h"

char   *
fptoms (fpv, ndec)
s_fp    fpv;
int     ndec;
{
    u_fp    plusfp;
    int     neg;

    if (fpv < 0)
        {
	plusfp = (u_fp)(-fpv);
	neg = 1;
        }
    else
        {
	plusfp = (u_fp)fpv;
	neg = 0;
        }

    return dofptoa (plusfp, neg, ndec, 1);
}
