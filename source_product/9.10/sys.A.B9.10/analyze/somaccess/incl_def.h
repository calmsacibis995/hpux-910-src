/* UNISRC_ID: @(#)incl_def.h	4.00	85/10/01  */

/*===========================================================================
   PURPOSE:

   This file defines various constants and error numbers
   Date create : 20 June 1984
   Engineer:  Lam Tran

*/
/*===========================================================================

 This section is for general constants use in system call.

---------------------------------------------------------------------------*/

#define FILE_BEGIN         0  /* beginning of the file, uses for fseek */
#define FILE_CURRENT       1  /* from current file ptr,    ---         */
#define ONE                1  /* constant 1 */
#define TWO                2  /* constant 2 */
#define THREE              3  /* constant 3 */
#define FOUR               4  /* constant 4 */

/*---------------------------------------------------------------------------*/

#define BAD               -1  /* bad return */
#define OK                 0  /* good return */

/*---------------------------------------------------------------------------*/

#define LD1_DF1            0  /* l d bits: space loadable & defined */
#define LD0_DF0            2  /* l d bits: space not loadable nor defined */
#define LD1_DF0            1  /* l d bits: space loadable but not defined */
#define LD0_DF1            3  /* l d bits: space not loadable but defined */



/*=========================================================================*/

/*===========================================================================

 This section defines offsets in the SOM header

----------------------------------------------------------------------------*/

#define SPACE_DIC_LOC     44L  /* byte offset of space_dic_location in SOM */
#define SPACE_DIC_TOTAL   48L  /* -------------------------total-----------*/
#define SUBSP_DIC_LOC     52L  /* byte offset of subsp_dic_location in SOM */
#define SUBSP_DIC_TOTAL   56L  /* ----------------------------total    */
#define SPACE_STR_LOC     68L  /* byte offset of space_strings_location */
#define SPACE_STR_SIZE    72L  /* ----------------------------size     */

/*=========================================================================*/
/*
=============================================================================

 This section is for individual function error number

============================================================================*/
/*
-----------------------------------------------------------------------------

ERRORS for som access routines have a general format:

                    A_B_xyz

where A: General function name, i.e, find, get, modify...
      B: Specific field name, i.e, space, subspace, LST...
    xyz: An abreviation of error, i.e, FREAD_ERR, 0_SPREC, ...

----------------------------------------------------------------------------*/

#define MALLOC_ERR 400              /* temp malloc error for all */



#define FD_SP       500      /* Start sp_nmfdr errors */

#define FD_SP_FSEEK_ERR0      (FD_SP+1)     /* Fseek seeks space dic. loc */
#define FD_SP_FSEEK_ERR1      (FD_SP+2)     /* Fseek seeks space string loc */
#define FD_SP_FSEEK_ERR2      (FD_SP+3)     /* Fseek seeks space rec #0 */
#define FD_SP_FSEEK_ERR3      (FD_SP+4) /* Fseeks seeks string name of sp rec */

#define FD_SP_FREAD_ERR0  (FD_SP+5)  /* Fread reads space dic. loc in som header
 */
#define FD_SP_FREAD_ERR1  (FD_SP+6)  /* Fread reads space dic. total in som head
er */
#define FD_SP_FREAD_ERR2  (FD_SP+7)  /* Fread reads string dic. loc in som heade
r */
#define FD_SP_FREAD_ERR3  (FD_SP+8)  /* Fread reads string dic. size in som head
er */
#define FD_SP_FREAD_ERR4  (FD_SP+9)  /* Fread reads space record into space stru
cture */
#define FD_SP_FREAD_ERR5  (FD_SP+10) /* Fread reads length of string name */
#define FD_SP_FREAD_ERR6  (FD_SP+11) /* Fread reads string name */

#define FD_SP_0_SPREC0  (FD_SP+12)  /* Space record not defined: space dic. loc
<= 0 */
#define FD_SP_0_SPREC1  (FD_SP+13)  /* Space record not defined: space dic. tota
l <= 0 */
#define FD_SP_0_SPSTR0  (FD_SP+14)  /* Space string not defined: string dic. loc
 <= 0 */
#define FD_SP_0_SPSTR1  (FD_SP+15)  /* Space string not defined: string dic. siz
e <= 0 */
#define FD_SP_SPREC_COR (FD_SP+16)  /* Space record corrupted: invalid space nam
e poiter */
#define FD_SP_SPNAM_NIL (FD_SP+17)  /* Space record has a nil name pointer */

#define FD_SP_FSEEK_ERR4 (FD_SP+18) /* Fseeks seeks space string name */


/*---------------------------------------------------------------------------*/


#define FD_SUB  600                 /* Start subsp_nmfdr error */

#define FD_SUB_FSEEK_ERR0      (FD_SUB+1)  /* Fseek seeks space dic. loc */
#define FD_SUB_FSEEK_ERR1      (FD_SUB+2)  /* Fseek seeks subspece dic. loc */

#define FD_SUB_FSEEK_ERR2      (FD_SUB+3)     /* Fseek seeks space string loc. i
n som header */
#define FD_SUB_FSEEK_ERR3      (FD_SUB+4)     /* Fseeks seeks subspace record in
 the loop */

#define FD_SUB_FREAD_ERR0  (FD_SUB+5)  /* Fread reads space dic. loc in som head
er */
#define FD_SUB_FREAD_ERR1  (FD_SUB+6)  /* Fread reads space dic. total in som he
ader */
#define FD_SUB_FREAD_ERR2  (FD_SUB+7)  /* Fread reads subspace dic. loc in som h
eader */
#define FD_SUB_FREAD_ERR3  (FD_SUB+8)  /* Fread reads subspace dic. total in som
 header */
#define FD_SUB_FREAD_ERR4  (FD_SUB+9)  /* Fread reads string dic. loc in som hea
der */
#define FD_SUB_FREAD_ERR5  (FD_SUB+10) /* Fread reads string dic. size in som he
ader */
#define FD_SUB_FREAD_ERR6  (FD_SUB+11) /* Fread reads space record into space st
ructure */
#define FD_SUB_FREAD_ERR7  (FD_SUB+12) /* Fread reads subspace record into subsp
ace structure */
#define FD_SUB_FREAD_ERR8  (FD_SUB+13) /* Fread reads subspace string name lengt
h */
#define FD_SUB_FREAD_ERR9  (FD_SUB+14) /* Fread reads subspace string name */

#define FD_SUB_0_SPREC0  (FD_SUB+15)  /* Space record not defined: space dic. lo
c <= 0 */
#define FD_SUB_0_SPREC1  (FD_SUB+16)  /* Space record not defined: space dic. to
tal <= 0 */
#define FD_SUB_0_SPREC2  (FD_SUB+17)  /* Subspace record not defined: subspace d
ic. loc <= 0 */
#define FD_SUB_0_SPREC3  (FD_SUB+18)  /* Subspace record not defined: subspace d
ic. total <= 0 */
#define FD_SUB_0_SUBSTR0  (FD_SUB+19)  /* Subspace string not defined: string di
c. loc <= 0 */
#define FD_SUB_0_SUBSTR1  (FD_SUB+20)  /* Subspace string not defined: string di
c. size <= 0 */
#define FD_SUB_SUBREC_COR (FD_SUB+21)  /* Subspace record corrupted: invalid sub
space name poiter */
#define FD_SUB_SUBNAM_NIL (FD_SUB+22)  /* Subspace record has a nil name pointer
 */

#define FD_SUB_FSEEK_ERR4   (FD_SUB+23)     /* Fseek seeks subspace string name
with same ac_type */
#define FD_SUB_FSEEK_ERX1   (FD_SUB+24)     /* Fseek seeks to read space record into structure  */

/*---------------------------------------------------------------------------*/


#define GET_SP                 700            /* start get space error */

#define GET_SP_FSEEK_ERR0      (GET_SP+1)     /* Fseek seeks space dic. loc in s
om header */
#define GET_SP_FSEEK_ERR1      (GET_SP+2)     /* Fseek seeks requested space rec
ord */

#define GET_SP_FREAD_ERR0  (GET_SP+3)  /* Fread reads space dic. loc in som head
er */
#define GET_SP_FREAD_ERR1  (GET_SP+4)  /* Fread reads space dic. total in som he
ader */
#define GET_SP_FREAD_ERR2  (GET_SP+5)  /* Fread reads  requested space record in
to internal space structure */

#define GET_SP_0_SPREC0  (GET_SP+6)  /* Space record not defined: space dic. loc
 <= 0 */
#define GET_SP_0_SPREC1  (GET_SP+7)  /* Space record not defined: space dic. tot
al <= 0 */
#define GET_SP_SP_X_INV  (GET_SP+8)  /* Input sp_index is invalid */

#define GET_SP_FWRITE_ERR  (GET_SP+9)  /* Fwrite can't write space record back *
/


/*---------------------------------------------------------------------------*/



#define GET_SUB      750             /* Start get subspace routine errors */


#define GET_SUB_FSEEK_ERR0      (GET_SUB+1)     /* Fseeks seeks space dic. loc i
n som header */
#define GET_SUB_FSEEK_ERR1      (GET_SUB+2)     /* Fseek seeks the space record
encompasses the requested subspace */
#define GET_SUB_FSEEK_ERR2      (GET_SUB+3)     /* Fseek seeks subspace dic. loc
ation in SOM */
#define GET_SUB_FSEEK_ERR3      (GET_SUB+4)     /* Fseeks seeks the requested su
bspace record */

#define GET_SUB_FREAD_ERR0  (GET_SUB+5)  /* Fread reads space dic. loc in som he
ader */
#define GET_SUB_FREAD_ERR1  (GET_SUB+6)  /* Fread reads space dic. total in som
header */
#define GET_SUB_FREAD_ERR2  (GET_SUB+7)  /* Fread reads space record into intern
al space structure */
#define GET_SUB_FREAD_ERR3  (GET_SUB+8)  /* Fread reads subspace dic. loccation
in som header */
#define GET_SUB_FREAD_ERR4  (GET_SUB+9)  /* Fread reads string dic. total in som
 header */
#define GET_SUB_FREAD_ERR5  (GET_SUB+10) /* Fread reads the requested subspace r
ecord into internal structure */

#define GET_SUB_0_SPREC0  (GET_SUB+11)  /* Space record not defined: space dic.
loc <= 0 */
#define GET_SUB_0_SPREC1  (GET_SUB+12)  /* Space record not defined: space dic.
total <= 0 */
#define GET_SUB_SP_X_INV  (GET_SUB+13)  /* Input sp_index is invalid */
#define GET_SUB_0_SUBREC0  (GET_SUB+14) /* Subspace record not defined: subspace
 dic. loc <= 0 */
#define GET_SUB_0_SUBREC1  (GET_SUB+15)  /* Subspace record not defined: subspac
e dic. total <= 0 */
#define GET_SUB_SBSP_X_INV  (GET_SUB+16)  /* Subsp_index inputed invalid */


#define GET_SUB_FWRITE_ERR   (GET_SUB+17)   /* Can't write back subspace */


