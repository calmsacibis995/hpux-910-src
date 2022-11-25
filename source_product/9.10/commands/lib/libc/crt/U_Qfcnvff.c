#include "quadfp.h"

#pragma OPT_LEVEL 1

QUAD _U_Qfcnvff_sgl_to_quad(opnd)
int opnd;
{
	QUAD result;
	unsigned status, excp;

	/* staus is hardcoded in macros used by emulation routine */
        status = 0;

	/* call emulation routine */
	if (excp = _U_sqfcnvff(&opnd, 0, &result, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}
	return(result);
}

QUAD _U_Qfcnvff_dbl_to_quad(opnd)
double opnd;
{
	QUAD result;
	unsigned status, excp;

	/* staus is hardcoded in macros used by emulation routine */
        status = 0;

	/* call emulation routine */
	if (excp = _U_dqfcnvff(&opnd, 0, &result, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}

	return(result);
}

int _U_Qfcnvff_quad_to_sgl(opnd)
QUAD opnd;
{
	int result;
	unsigned status, excp;

	/* status is hardcoded in macros used by emulation routine */
        status = 0;

	/* call emulation routine */
	if (excp = _U_qsfcnvff(&opnd, 0, &result, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}

	return(result);
}

double _U_Qfcnvff_quad_to_dbl(opnd)
QUAD opnd;
{
	DBL result;
	unsigned status, excp;

	/* staus is hardcoded in macros used by emulation routine */
        status = 0;

	/* call emulation routine */
	if (excp = _U_qdfcnvff(&opnd, 0, &result, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}

	return(result.d);
}

#pragma OPT_LEVEL 2
