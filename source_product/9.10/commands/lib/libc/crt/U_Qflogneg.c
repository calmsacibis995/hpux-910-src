#include "quadfp.h"

#define LDEQ 4
#define LDLT 8
#define LDGT 16

#pragma OPT_LEVEL 1

_U_Qflogneg(opnd2)
QUAD opnd2;
{
	QUAD opnd1;
	unsigned status, cond, excp;

	/* status is hardcoded in macros used by emulation routine */
	status = 0;
	opnd1.d[0] = 0;
	opnd1.d[1] = 0;

        /* do a a test != 0 */

	cond = LDEQ;

	/* call emulation routine */
	if (excp = _U_qfcmp(&opnd1, &opnd2, cond, &status)) {

		/* Force a Floating point exception */
		asm("	trap &8");
	}
	return(status);
}

#pragma OPT_LEVEL 2
