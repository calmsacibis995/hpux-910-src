/*
 * @(#)float.h: $Revision: 64.2 $ $Date: 89/02/15 09:26:17 $
 * $Locker:  $
 * 
 */
#include "fpbits.h"
#include "spectrum.h"

/*
 * Declare the basic structures for the 3 different
 * floating-point precisions.
 *        
 * Single number  
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |s|       exp     |               mantissa                      |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */
#define	Sall(object) (object)
#define	Ssign(object) Bitfield_extract( 0,  1,object)
#define	Ssignedsign(object) Bitfield_signed_extract( 0,  1,object)
#define	Sexponent(object) Bitfield_extract( 1,  8,object)
#define	Smantissa(object) Bitfield_mask( 9, 23,object)
#define	Ssignaling(object) Bitfield_extract( 9,  1,object)
#define	Ssignalingnan(object) Bitfield_extract( 1,  9,object)
#define	Shigh2mantissa(object) Bitfield_extract( 9,  2,object)
#define	Sexponentmantissa(object) Bitfield_mask( 1, 31,object)
#define	Ssignexponent(object) Bitfield_extract( 0,  9,object)
#define	Shidden(object) Bitfield_extract( 8,  1,object)
#define	Shiddenoverflow(object) Bitfield_extract( 7,  1,object)
#define	Shiddenhigh7mantissa(object) Bitfield_extract( 8,  8,object)
#define	Shiddenhigh3mantissa(object) Bitfield_extract( 8,  4,object)
#define	Slow(object) Bitfield_mask( 31,  1,object)
#define	Slow4(object) Bitfield_mask( 28,  4,object)
#define	Slow31(object) Bitfield_mask( 1, 31,object)
#define	Shigh31(object) Bitfield_extract( 0, 31,object)
#define	Ssignedhigh31(object) Bitfield_signed_extract( 0, 31,object)
#define	Shigh4(object) Bitfield_extract( 0,  4,object)
#define	Sbit24(object) Bitfield_extract( 24,  1,object)
#define	Sbit28(object) Bitfield_extract( 28,  1,object)
#define	Sbit29(object) Bitfield_extract( 29,  1,object)
#define	Sbit30(object) Bitfield_extract( 30,  1,object)
#define	Sbit31(object) Bitfield_mask( 31,  1,object)

#define Deposit_ssign(object,value) Bitfield_deposit(value,0,1,object)
#define Deposit_sexponent(object,value) Bitfield_deposit(value,1,8,object)
#define Deposit_smantissa(object,value) Bitfield_deposit(value,9,23,object)
#define Deposit_shigh2mantissa(object,value) Bitfield_deposit(value,9,2,object)
#define Deposit_sexponentmantissa(object,value) \
    Bitfield_deposit(value,1,31,object)
#define Deposit_ssignexponent(object,value) Bitfield_deposit(value,0,9,object)
#define Deposit_slow(object,value) Bitfield_deposit(value,31,1,object)
#define Deposit_shigh4(object,value) Bitfield_deposit(value,0,4,object)

#define	Is_ssign(object) Bitfield_mask( 0,  1,object)
#define	Is_ssignaling(object) Bitfield_mask( 9,  1,object)
#define	Is_shidden(object) Bitfield_mask( 8,  1,object)
#define	Is_shiddenoverflow(object) Bitfield_mask( 7,  1,object)
#define	Is_slow(object) Bitfield_mask( 31,  1,object)
#define	Is_sbit24(object) Bitfield_mask( 24,  1,object)
#define	Is_sbit28(object) Bitfield_mask( 28,  1,object)
#define	Is_sbit29(object) Bitfield_mask( 29,  1,object)
#define	Is_sbit30(object) Bitfield_mask( 30,  1,object)
#define	Is_sbit31(object) Bitfield_mask( 31,  1,object)

/* 
 * Double number.
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |s|       exponent      |          mantissa part 1              |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                    mantissa part 2                            |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */
#define Dallp1(object) (object)
#define Dsign(object) Bitfield_extract( 0,  1,object)
#define Dsignedsign(object) Bitfield_signed_extract( 0,  1,object)
#define Dexponent(object) Bitfield_extract( 1,  11,object)
#define Dmantissap1(object) Bitfield_mask( 12, 20,object)
#define Dsignaling(object) Bitfield_extract( 12,  1,object)
#define Dsignalingnan(object) Bitfield_extract( 1,  12,object)
#define Dhigh2mantissa(object) Bitfield_extract( 12,  2,object)
#define Dexponentmantissap1(object) Bitfield_mask( 1, 31,object)
#define Dsignexponent(object) Bitfield_extract( 0, 12,object)
#define Dhidden(object) Bitfield_extract( 11,  1,object)
#define Dhiddenoverflow(object) Bitfield_extract( 10,  1,object)
#define Dhiddenhigh7mantissa(object) Bitfield_extract( 11,  8,object)
#define Dhiddenhigh3mantissa(object) Bitfield_extract( 11,  4,object)
#define Dlowp1(object) Bitfield_mask( 31,  1,object)
#define Dlow31p1(object) Bitfield_mask( 1, 31,object)
#define Dhighp1(object) Bitfield_extract( 0,  1,object)
#define Dhigh4p1(object) Bitfield_extract( 0,  4,object)
#define Dhigh31p1(object) Bitfield_extract( 0, 31,object)
#define Dsignedhigh31p1(object) Bitfield_signed_extract( 0, 31,object)
#define Dbit3p1(object) Bitfield_extract( 3,  1,object)

#define Deposit_dsign(object,value) Bitfield_deposit(value,0,1,object)
#define Deposit_dexponent(object,value) Bitfield_deposit(value,1,11,object)
#define Deposit_dmantissap1(object,value) Bitfield_deposit(value,12,20,object)
#define Deposit_dhigh2mantissa(object,value) Bitfield_deposit(value,12,2,object)
#define Deposit_dexponentmantissap1(object,value) \
    Bitfield_deposit(value,1,31,object)
#define Deposit_dsignexponent(object,value) Bitfield_deposit(value,0,12,object)
#define Deposit_dlowp1(object,value) Bitfield_deposit(value,31,1,object)
#define Deposit_dhigh4p1(object,value) Bitfield_deposit(value,0,4,object)

#define Is_dsign(object) Bitfield_mask( 0,  1,object)
#define Is_dsignaling(object) Bitfield_mask( 12,  1,object)
#define Is_dhidden(object) Bitfield_mask( 11,  1,object)
#define Is_dhiddenoverflow(object) Bitfield_mask( 10,  1,object)
#define Is_dlowp1(object) Bitfield_mask( 31,  1,object)
#define Is_dhighp1(object) Bitfield_mask( 0,  1,object)
#define Is_dbit3p1(object) Bitfield_mask( 3,  1,object)

#define Dallp2(object) (object)
#define Dmantissap2(object) (object)
#define Dlowp2(object) Bitfield_mask( 31,  1,object)
#define Dlow4p2(object) Bitfield_mask( 28,  4,object)
#define Dlow31p2(object) Bitfield_mask( 1, 31,object)
#define Dhighp2(object) Bitfield_extract( 0,  1,object)
#define Dhigh31p2(object) Bitfield_extract( 0, 31,object)
#define Dbit2p2(object) Bitfield_extract( 2,  1,object)
#define Dbit3p2(object) Bitfield_extract( 3,  1,object)
#define Dbit21p2(object) Bitfield_extract( 21,  1,object)
#define Dbit28p2(object) Bitfield_extract( 28,  1,object)
#define Dbit29p2(object) Bitfield_extract( 29,  1,object)
#define Dbit30p2(object) Bitfield_extract( 30,  1,object)
#define Dbit31p2(object) Bitfield_mask( 31,  1,object)

#define Deposit_dlowp2(object,value) Bitfield_deposit(value,31,1,object)

#define Is_dlowp2(object) Bitfield_mask( 31,  1,object)
#define Is_dhighp2(object) Bitfield_mask( 0,  1,object)
#define Is_dbit2p2(object) Bitfield_mask( 2,  1,object)
#define Is_dbit3p2(object) Bitfield_mask( 3,  1,object)
#define Is_dbit21p2(object) Bitfield_mask( 21,  1,object)
#define Is_dbit28p2(object) Bitfield_mask( 28,  1,object)
#define Is_dbit29p2(object) Bitfield_mask( 29,  1,object)
#define Is_dbit30p2(object) Bitfield_mask( 30,  1,object)
#define Is_dbit31p2(object) Bitfield_mask( 31,  1,object)

/* 
 * Quad number.
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |s|          exponent           |      mantissa part 1          |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                    mantissa part 2                            |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                    mantissa part 3                            |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                    mantissa part 4                            |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */

#define Qallp1(object) (object)
#define Qsign(object) Bitfield_extract( 0,  1,object)
#define Qsignedsign(object) Bitfield_signed_extract( 0,  1,object)
#define Qexponent(object) Bitfield_extract( 1, 15,object)
#define Qmantissap1(object) Bitfield_mask(16, 16,object)
#define Qsignaling(object) Bitfield_extract(16,  1,object)
#define Qsignalingnan(object) Bitfield_extract(1,  16,object)
#define Qhigh2mantissa(object) Bitfield_extract(16,  2,object)
#define Qexponentmantissap1(object) Bitfield_mask( 1, 31,object)
#define Qsignexponent(object) Bitfield_extract( 0, 16,object)
#define Qhidden(object) Bitfield_extract(15,  1,object)
#define Qhiddenoverflow(object) Bitfield_extract(14,  1,object)
#define Qhiddenhigh7mantissa(object) Bitfield_extract(15,  8,object)
#define Qhiddenhigh3mantissa(object) Bitfield_extract(15,  4,object)
#define Qbit7p1(object) Bitfield_extract( 7, 1,object)
#define Qlowp1(object) Bitfield_mask(31,  1,object)
#define Qlow31p1(object) Bitfield_mask( 1, 31,object)
#define Qhighp1(object) Bitfield_extract( 0,  1,object)
#define Qhigh4p1(object) Bitfield_extract( 0,  4,object)
#define Qhigh31p1(object) Bitfield_extract( 0, 31,object)
#define Qsignedhigh31p1(object) Bitfield_extract( 3,  1,object)

#define Deposit_qsign(object,value) Bitfield_deposit(value,0,1,object)
#define Deposit_qexponent(object,value) Bitfield_deposit(value,1,15,object)
#define Deposit_qmantissap1(object,value) Bitfield_deposit(value,16,16,object)
#define Deposit_qhigh2mantissa(object,value) Bitfield_deposit(value,16,2,object)
#define Deposit_qexponentmantissap1(object,value) \
    Bitfield_deposit(value,1,31,object)
#define Deposit_qsignexponent(object,value) Bitfield_deposit(value,0,16,object)
#define Deposit_qlowp1(object,value) Bitfield_deposit(value,31,1,object)
#define Deposit_qhigh4p1(object,value) Bitfield_deposit(value,0,4,object)

#define Is_qsign(object) Bitfield_mask( 0,  1,object)
#define Is_qsignaling(object) Bitfield_mask(16,  1,object)
#define Is_qhidden(object) Bitfield_mask(15,  1,object)
#define Is_qhiddenoverflow(object) Bitfield_mask(14,  1,object)
#define Is_qlowp1(object) Bitfield_mask(31,  1,object)
#define Is_qhighp1(object) Bitfield_mask( 3,  1,object)
#define Is_qbit3p1(object) Bitfield_mask( 0,  1,object)

#define Qallp2(object) (object)
#define Qmantissap2(object) (object)
#define Qbit6p2(object) Bitfield_extract( 6,  1,object)
#define Qbit7p2(object) Bitfield_extract( 7,  1,object)
#define Qlowp2(object) Bitfield_mask(31,  1,object)
#define Qlow4p2(object) Bitfield_mask(18,  4,object)
#define Qlow31p2(object) Bitfield_mask( 1, 31,object)
#define Qhighp2(object) Bitfield_extract( 0,  1,object)
#define Qhigh31p2(object) Bitfield_extract( 0, 31,object)

#define Qallp3(object) (object)
#define Qmantissap3(object) (object)
#define Qbit3p3(object) Bitfield_extract( 3, 1,object)
#define Qbit4p3(object) Bitfield_extract( 4, 1,object)
#define Qlowp3(object) Bitfield_extract(31,  1,object)
#define Qlow31p3(object) Bitfield_extract( 1, 31,object)
#define Qhighp3(object) Bitfield_extract( 0,  1,object)
#define Qhigh31p3(object) Bitfield_extract( 0, 31,object)

#define Qallp4(object) (object)
#define Qmantissap4(object) (object)
#define Qlowp4(object) Bitfield_extract(31,  1,object)
#define Qlow4p4(object) Bitfield_extract(28,  4,object)
#define Qlow31p4(object) Bitfield_extract( 1, 31,object)
#define Qhighp4(object) Bitfield_extract( 0,  1,object)
#define Qhigh31p4(object) Bitfield_extract( 0, 31,object)
#define Qbit28p4(object) Bitfield_extract(28, 1,object)
#define Qbit29p4(object) Bitfield_extract(29, 1,object)
#define Qbit30p4(object) Bitfield_extract(30, 1,object)
#define Qbit31p4(object) Bitfield_extract(31, 1,object)

#define Deposit_qlowp4(object,value) Bitfield_deposit(value,31,1,object)

#define Is_qlowp4(object) Bitfield_mask(31,  1,object)

/* Extension - An additional structure to hold the guard, round and
 *             sticky bits during computations.
 */
#define Extall(object) (object)
#define Extsign(object) Bitfield_extract( 0,  1,object)
#define Exthigh31(object) Bitfield_extract( 0, 31,object)
#define Extlow31(object) Bitfield_extract( 1, 31,object)
#define Extlow(object) Bitfield_extract( 31,  1,object)

/*
 * Declare the basic structures for the 3 different
 * fixed-point precisions.
 *        
 * Single number  
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |s|                    integer                                  |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */
typedef int sgl_integer;

/* 
 * Double number.
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |s|                     high integer                            |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                       low integer                             |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */
struct dint {
        int  wd0;
        unsigned int wd1;
};

struct dblwd {
        unsigned int wd0;
        unsigned int wd1;
};

/* 
 * Quad number.
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |s|                  integer part1                              |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                    integer part 2                             |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                    integer part 3                             |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                    integer part 4                             |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */

struct qint {
        int  wd0;
        unsigned int wd1;
        unsigned int wd2;
        unsigned int wd3;
};

struct quadwd {
        unsigned int  wd0;
        unsigned int wd1;
        unsigned int wd2;
        unsigned int wd3;
};



/* useful typedefs */
typedef int sgl_floating_point;
typedef struct dblwd dbl_floating_point;
typedef struct dint dbl_integer;
typedef struct quadwd quad_floating_point;
typedef struct qint quad_integer;

/* 
 * Define the different precisions' parameters.
 */
#define SGL_BITLENGTH 32
#define SGL_EMAX 127
#define SGL_EMIN (-126)
#define SGL_BIAS 127
#define SGL_WRAP 192
#define SGL_INFINITY_EXPONENT (SGL_EMAX+SGL_BIAS+1)
#define SGL_THRESHOLD 32
#define SGL_EXP_LENGTH 8
#define SGL_P 24

#define DBL_BITLENGTH 64
#define DBL_EMAX 1023
#define DBL_EMIN (-1022)
#define DBL_BIAS 1023
#define DBL_WRAP 1536
#define DBL_INFINITY_EXPONENT (DBL_EMAX+DBL_BIAS+1)
#define DBL_THRESHOLD 64
#define DBL_EXP_LENGTH 11
#define DBL_P 53

#define QUAD_BITLENGTH 128
#define QUAD_EMAX 16383
#define QUAD_EMIN (-16382)
#define QUAD_BIAS 16383
#define QUAD_WRAP 24576
#define QUAD_INFINITY_EXPONENT (QUAD_EMAX+QUAD_BIAS+1)
#define QUAD_THRESHOLD 128
#define QUAD_EXP_LENGTH 15
#define QUAD_P 113

/* Boolean Values etc. */
#define FALSE 0
#define TRUE (!FALSE)
#define NOT !
#define XOR ^

/* other constants */
#undef NULL
#define NULL 0
#define NIL 0
#define SGL 0
#define DBL 1
#define BADFMT 2
#define QUAD 3


/* Types */
typedef int boolean;
typedef int FORMAT;
typedef int VOID;


/* Declare status register equivalent to FPUs architecture.
 *
 *  0 1 2 3 4 5 6 7 8 910 1 2 3 4 5 6 7 8 920 1 2 3 4 5 6 7 8 930 1
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |V|Z|O|U|I|C|  rsv  |  model    | version |RM |rsv|T|r|V|Z|O|U|I|
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */
#define Cbit(object) Bitfield_extract( 5, 1,object)
#define Tbit(object) Bitfield_extract( 25, 1,object)
#define Roundingmode(object) Bitfield_extract( 21, 2,object)
#define Invalidtrap(object) Bitfield_extract( 27, 1,object)
#define Divisionbyzerotrap(object) Bitfield_extract( 28, 1,object)
#define Overflowtrap(object) Bitfield_extract( 29, 1,object)
#define Underflowtrap(object) Bitfield_extract( 30, 1,object)
#define Inexacttrap(object) Bitfield_extract( 31, 1,object)
#define Invalidflag(object) Bitfield_extract( 0, 1,object)
#define Divisionbyzeroflag(object) Bitfield_extract( 1, 1,object)
#define Overflowflag(object) Bitfield_extract( 2, 1,object)
#define Underflowflag(object) Bitfield_extract( 3, 1,object)
#define Inexactflag(object) Bitfield_extract( 4, 1,object)
#define Allflags(object) Bitfield_extract( 0, 5,object)

/* Definitions relevant to the status register */

/* Rounding Modes */
#ifdef __hp9000s300
#define ROUNDNEAREST 0
#define ROUNDZERO    1
#define ROUNDMINUS   2
#define ROUNDPLUS    3
#else
#define ROUNDNEAREST 0
#define ROUNDZERO    1
#define ROUNDPLUS    2
#define ROUNDMINUS   3
#endif

/* Exceptions */
#define NOEXCEPTION		0x0
#define INVALIDEXCEPTION	0x20
#define DIVISIONBYZEROEXCEPTION	0x10
#define OVERFLOWEXCEPTION	0x08
#define UNDERFLOWEXCEPTION	0x04
#define INEXACTEXCEPTION	0x02
#define UNIMPLEMENTEDEXCEPTION	0x01

/* Declare exception registers equivalent to FPUs architecture 
 *
 *  0 1 2 3 4 5 6 7 8 910 1 2 3 4 5 6 7 8 920 1 2 3 4 5 6 7 8 930 1
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |excepttype |  r1     | r2/ext  |  operation  |parm |n| t/cond  |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */
#define Allexception(object) (object)
#define Exceptiontype(object) Bitfield_extract( 0, 6,object)
#define Instructionfield(object) Bitfield_mask( 6,26,object)
#define Parmfield(object) Bitfield_extract( 23, 3,object)
#define Rabit(object) Bitfield_extract( 24, 1,object)
#define Ibit(object) Bitfield_extract( 25, 1,object)

#define Set_exceptiontype(object,value) Bitfield_deposit(value, 0, 6,object)
#define Set_parmfield(object,value) Bitfield_deposit(value, 23, 3,object)
#define Set_exceptiontype_and_instr_field(exception,instruction,object) \
    object = exception << 26 | instruction

/* Declare the condition field
 *
 *  0 1 2 3 4 5 6 7 8 910 1 2 3 4 5 6 7 8 920 1 2 3 4 5 6 7 8 930 1
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |                                                     |G|L|E|U|X|
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 */
#define Allexception(object) (object)
#define Greaterthanbit(object) Bitfield_extract( 27, 1,object)
#define Lessthanbit(object) Bitfield_extract( 28, 1,object)
#define Equalbit(object) Bitfield_extract( 29, 1,object)
#define Unorderedbit(object) Bitfield_extract( 30, 1,object)
#define Exceptionbit(object) Bitfield_extract( 31, 1,object)

/* An alias name for the status register */
#define Fpustatus_register (*status)

/**************************************************
 * Status register referencing and manipulation.  *
 **************************************************/
#ifdef __hp9000s300

/* HARDCODED DEFAULTS FOR THE S300 */

#define Rounding_mode() ROUNDNEAREST
#define Is_rounding_mode(rmode) (ROUNDNEAREST == rmode)
#define Is_invalidtrap_enabled() TRUE
#define Is_divisionbyzerotrap_enabled() TRUE
#define Is_overflowtrap_enabled() TRUE
#define Is_underflowtrap_enabled() FALSE
#define Is_inexacttrap_enabled() FALSE

/* Bitfield extracted values if needed, note 68k has 3 other   */
/* traps which can be set                                      */
/* NOTE, Bitfield extract values are in HP-PA numbering scheme */
/* 0 --- 31, not 31 --- 0                                      */

/* Rounding mode */
/* #define Rounding_mode() Bitfield_extract(26,2,*status)                    */
/* #define Is_rounding_mode(rmode) (Bitfield_extract(26,2,*status) == rmode) */
/* #define Set_rounding_mode(value)                                          */

/* Boolean testing of the trap enable bits */
/* #define Is_invalidtrap_enabled() Bitfield_extract(18,1,*status)           */
/* #define Is_divisionbyzerotrap_enabled() Bitfield_extract(21,1,*status)    */
/* #define Is_overflowtrap_enabled() Bitfield_extract(19,1,*status)          */
/* #define Is_underflowtrap_enabled() Bitfield_extract(20,1,*status)         */
/* #define Is_inexacttrap_enabled() Bitfield_extract(22,1,*status)           */

/* Set the indicated flags in the status register */
#define Set_invalidflag()
#define Set_divisionbyzeroflag()
#define Set_overflowflag()
#define Set_underflowflag()
#define Set_inexactflag()

#define Clear_all_flags()

/* Manipulate the trap and condition code bits (tbit and cbit) */
#define Set_tbit()
#define Clear_tbit()
#define Is_tbit_set()
#define Is_cbit_set()

#define Set_status_cbit(value) Bitfield_deposit(value,31,1,Fpustatus_register)

#else

/* Rounding mode */
#define Rounding_mode()  Roundingmode(Fpustatus_register)
#define Is_rounding_mode(rmode) \
    (Roundingmode(Fpustatus_register) == rmode)
#define Set_rounding_mode(value) \
    Bitfield_deposit(value,21,2,Fpustatus_register)

/* Boolean testing of the trap enable bits */
#define Is_invalidtrap_enabled() Invalidtrap(Fpustatus_register)
#define Is_divisionbyzerotrap_enabled() Divisionbyzerotrap(Fpustatus_register)
#define Is_overflowtrap_enabled() Overflowtrap(Fpustatus_register)
#define Is_underflowtrap_enabled() Underflowtrap(Fpustatus_register)
#define Is_inexacttrap_enabled() Inexacttrap(Fpustatus_register)

/* Set the indicated flags in the status register */
#define Set_invalidflag() Bitfield_deposit(1,0,1,Fpustatus_register)
#define Set_divisionbyzeroflag() Bitfield_deposit(1,1,1,Fpustatus_register)
#define Set_overflowflag() Bitfield_deposit(1,2,1,Fpustatus_register)
#define Set_underflowflag() Bitfield_deposit(1,3,1,Fpustatus_register)
#define Set_inexactflag() Bitfield_deposit(1,4,1,Fpustatus_register)

#define Clear_all_flags() Bitfield_deposit(0,0,5,Fpustatus_register)

/* Manipulate the trap and condition code bits (tbit and cbit) */
#define Set_tbit() Bitfield_deposit(1,25,1,Fpustatus_register)
#define Clear_tbit() Bitfield_deposit(0,25,1,Fpustatus_register)
#define Is_tbit_set() Tbit(Fpustatus_register)
#define Is_cbit_set() Cbit(Fpustatus_register)

#define Set_status_cbit(value) Bitfield_deposit(value,5,1,Fpustatus_register)
#endif

/*******************************
 * Condition field referencing *
 *******************************/
#define Unordered(cond) Unorderedbit(cond)
#define Equal(cond) Equalbit(cond)
#define Lessthan(cond) Lessthanbit(cond)
#define Greaterthan(cond) Greaterthanbit(cond)
#define Exception(cond) Exceptionbit(cond)


/* Defines for the extension */
#define Ext_isone_sign(extent) (Extsign(extent))
#define Ext_isnotzero(extent) \
    (Extall(extent))
#define Ext_isnotzero_lower(extent) \
    (Extlow31(extent))
#define Ext_leftshiftby1(extent) \
    Extall(extent) <<= 1
#define Ext_negate(extent) \
    (int )Extall(extent) = 0 - (int )Extall(extent)
#define Ext_setone_low(extent) Bitfield_deposit(1,31,1,extent)

typedef int operation;

/* error messages */

#define		NONE		0
#define		UNDEFFPINST	1

/* Function definitions: opcode, opclass */
#define FTEST	(1<<2) | 0
#define FCPY	(2<<2) | 0
#define FABS	(3<<2) | 0
#define FSQRT   (4<<2) | 0
#define FRND    (5<<2) | 0

#define FCNVFF	(0<<2) | 1
#define FCNVXF	(1<<2) | 1
#define FCNVFX	(2<<2) | 1
#define FCNVFXT	(3<<2) | 1

#define FCMP    (0<<2) | 2

#define FADD	(0<<2) | 3
#define FSUB	(1<<2) | 3
#define FMPY	(2<<2) | 3
#define FDIV	(3<<2) | 3
#define FREM	(4<<2) | 3

