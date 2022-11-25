#include "quadfp.h"

#pragma OPT_LEVEL 1

QUAD _U_Qfneg(opnd2)
QUAD opnd2;
{
	QUAD opnd1;
	QUAD result;
	unsigned status, excp;

	/* status is hardcoded in macros used by emulation routine */
	status = 0;
	opnd1.d[0] = 0;
	opnd1.d[1] = 0;

	/* call emulation routine */
	if (excp = _U_qfsub(&opnd1, &opnd2, &result, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}
	return(result);
}

#pragma OPT_LEVEL 2
