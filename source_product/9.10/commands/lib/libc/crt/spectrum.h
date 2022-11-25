/*
 * @(#)spectrum.h: $Revision: 64.1 $ $Date: 89/02/09 11:22:23 $
 * $Locker:  $
 * 
 */
/* amount is assumed to be a constant between 0 and 32 (non-inclusive) */
#define Shiftdouble(left,right,amount,dest)			\
    /* int left, right, amount, dest; */			\
    dest = ((left) << (32-(amount))) | ((unsigned int)(right) >> (amount))

/* amount must be less than 32 */
#define Variableshiftdouble(left,right,amount,dest)		\
    /* unsigned int left, right;  int amount, dest; */		\
    if (amount == 0) dest = right;				\
    else dest = ((((unsigned) left)&0x7fffffff) << (32-(amount))) |	\
          ((unsigned) right >> (amount))

/* amount must be between 0 and 32 (non-inclusive) */
#define Variable_shift_double(left,right,amount,dest)		\
    /* unsigned int left, right;  int amount, dest; */		\
    dest = (left << (32-(amount))) | ((unsigned) right >> (amount))
