/* @(#) $Revision: 70.1 $ */

/*
 * Math library error routines (for libm).
 */

#include	<math.h>
#include	<errno.h>
#include	"c_include.h"

#ifdef _NAMESPACE_CLEAN
#define abs _abs
#define sprintf _sprintf
#define strlen _strlen
#define write _write
#endif /* _NAMESPACE_CLEAN */

typedef union { unsigned int i[2]; double d; float f[2];} DOUBLE;
double _error_handler(name,opnd1,opnd2,op_type,return_code,matherr_type)
char *name;
DOUBLE *opnd1, *opnd2;
int op_type;
int return_code;
int matherr_type;
{
	int write();
	struct exception excpt;

	DOUBLE temp;
	char buffer[50];
	int length = 0;
	int matherr_ret = 0;

	if (matherr_type > 0) {
		if (op_type == DOUBLE_TYPE) {
			excpt.arg1 = opnd1->d;
			excpt.arg2 = opnd2->d;
			}
		else {
			excpt.arg1 = (double) opnd1->f[0];
			excpt.arg2 = (double) opnd2->f[0];
			}
	}

	switch (return_code) {
		case NAN_RET:
			/* returned value will be a double precision */
			/* quiet NaN.  				     */
			temp.i[0] = 0x7FF80000;
			temp.i[1] = 0x0;
			break;
		case ZERO_RET:
			/* returned value will be a double precision */
			/* ZERO.  				     */
			temp.d = 0.0;
			break;
		case INF_RET:
			/* returned value will be a double precision */
			/* INFINITY.  				     */
			temp.i[0] = 0x7FF00000;
			temp.i[1] = 0x0;
			break;
		case HUGE_RET:
			/* returned value will be a double precision */
			/* HUGE_VAL.  				     */
			if (op_type == DOUBLE_TYPE)
				temp.d = HUGE;
			else
				temp.d = (double) MAXFLOAT;
			break;
		case NHUGE_RET:
			/* returned value will be a double precision */
			/* -HUGE_VAL.  				     */
			if (op_type == DOUBLE_TYPE)
				temp.d = -HUGE;
			else
				temp.d = - (double) MAXFLOAT;
			break;
		case MAXFLOAT_RET:
			temp.d = (double) MAXFLOAT_RET;
			break;
		case NMAXFLOAT_RET:
			temp.d = - (double) MAXFLOAT_RET;
			break;
		case OP1_RET:
			/* returned value will be a *opnd1 */
			temp.d = excpt.arg1;
			break;
		case ONE_RET:
			/* returned value will be a double precision */
			/* ONE.  				     */
			temp.d = 1.0;
			break;
		default:
			/* returned value will be a double precision */
			/* quiet NaN.  				     */
			temp.i[0] = 0x7FF40000;
			temp.i[1] = 0x0;
			break;
	}

	if (matherr_type > 0) {
		excpt.retval = temp.d;

		/* set up struct excpt */
		excpt.type = matherr_type;	/* error type */
		excpt.name = name;		/* routine name */
#ifdef libM
		matherr_ret = _matherr(&excpt);
#else
		matherr_ret = matherr(&excpt);
#endif /* libM */
	
		if (op_type == DOUBLE_TYPE)
			temp.d = excpt.retval;
		else
			/* WHAT IF excpt.retval too big for type float ???? */
			temp.f[0] = (float) excpt.retval;
	}

	if (!matherr_ret) {
#ifndef libM
		if (matherr_type == DOMAIN)
			length = sprintf(buffer,"%s: DOMAIN error\n",name);
		else if (matherr_type == SING)
			length = sprintf(buffer,"%s: SING error\n",name);
		else if (matherr_type == TLOSS)
			length = sprintf(buffer,"%s: TLOSS error\n",name);

		if (length > 0)
			write(2,buffer,length);
#endif /* libM */
		if ((abs(matherr_type) == DOMAIN)||(abs(matherr_type) == SING))
			errno = EDOM;
		else
			errno = ERANGE;
	}
			
	return temp.d;			/* return value */
}
