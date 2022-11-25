#include "quadfp.h"

#pragma OPT_LEVEL 1

QUAD _U_Qfadd(opnd1,opnd2)
QUAD opnd1, opnd2;
{
	QUAD result;
	unsigned status, excp;

	/* status is hardcoded in macros used in emulation routine */
	status = 0;

	/* call emulation routine */
	if (excp = _U_qfadd(&opnd1, &opnd2, &result, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}
	return(result);
}

#pragma OPT_LEVEL 2
