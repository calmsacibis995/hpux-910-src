#include "quadfp.h"

#pragma OPT_LEVEL 1

QUAD _U_Qfcnvxf_sgl_to_quad(opnd)
int opnd;
{
	QUAD result;
	unsigned status, excp;

	/* status is hardcoded in macros used by emulation routines */
	status = 0;

	/* call emulation routine */
	if (excp = _U_sqfcnvxf(&opnd, 0, &result, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}
	return(result);
}

QUAD _U_Qfcnvxf_usgl_to_quad(opnd)
unsigned opnd;
{
	QUAD result;
        DBL tmp;
	unsigned status, excp;

	/* status is hardcoded in macros used by emulation routines */
	status = 0;

	/* Convert unsigned integer to 64-bit signed integer */
	tmp.u[0] = 0;
	tmp.u[1] = opnd;

	/* call emulation routine */
	if (excp = _U_dqfcnvxf(&tmp, 0, &result, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}
	return(result);
}

#pragma OPT_LEVEL 2
