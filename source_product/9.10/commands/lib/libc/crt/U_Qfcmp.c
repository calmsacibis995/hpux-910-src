#include "quadfp.h"

#pragma OPT_LEVEL 1

_U_Qfcmp(opnd1,opnd2,cond)
QUAD opnd1, opnd2;
unsigned cond;
{
	unsigned status, excp;

	/* status is hardcoded in macros used by emulation routine */
	status = 0;

	/* call emulation routine */
	if (excp = _U_qfcmp(&opnd1, &opnd2, cond, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}
	return(status);
}
#pragma OPT_LEVEL 2
