#include "quadfp.h"

#pragma OPT_LEVEL 1

int _U_Qfcnvfxt_quad_to_sgl(opnd)
QUAD opnd;
{
	int result;
	unsigned status, excp;

        /* status is hardcoded in macros used by emulation routines */
	status = 0;

	/* call emulation routine */
	if (excp = _U_qsfcnvfxt(&opnd, 0, &result, &status)) {

		/* Force a floating point exception */
		asm("	trap &8");
	}
	return(result);
}

unsigned _U_Qfcnvfxt_quad_to_usgl(opnd)
QUAD opnd;
{
	unsigned result;
	unsigned status, excp;
        DBL tmp;

        /* status is hardcoded in macros used by emulation routines */
	status = 0;

	/* call emulation routine */
	if (excp = _U_qdfcnvfxt(&opnd, 0, &tmp, &status)) {

		/* Force a floating point exception */
		asm("	trap &8");
	}
        if ((tmp.u[0] != 0) && ((tmp.u[0] >> 31) == 0)) {

		/* Force a floating point exception */
		asm("	trap &8");
        }
        result = tmp.u[1];
	return(result);
}

#pragma OPT_LEVEL 2
