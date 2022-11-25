/* HPUX_ID: @(#) $Revision: 49.1 $  */
    /*
     *	opcode of the `calls' instruction
     */
#define	CALLS	0xfb

    /*
     *	offset (in bytes) of the code from the entry address of a routine.
     *	(see asgnsamples for use and explanation.)
     */
#define OFFSET_OF_CODE	0
#define	UNITS_TO_CODE	(OFFSET_OF_CODE / sizeof(UNIT))

#define JSR_MASK 0xffc0
#define JSR 0x4e80
#define BSR_MASK 0xff00
#define BSR 0x6100
