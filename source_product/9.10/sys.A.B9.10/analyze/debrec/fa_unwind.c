/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/fa_unwind.c,v $
 * $Revision: 1.2.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:22:33 $
 */

/*
 * Original version based on:
 * UNISRC_ID: @(#)fa_unwind.c	X.00.04		1 Jan 87
 */

/* 

purpose : to fill in some var parms :

*entry_len = length of entry sequence.

entry_code [n] = 0, if after this instr, all registers are still accurate.

entry_code [n] = 1, if after this instr R3 has been set up for > 8k frame

entry_size [n] = number of bytes of the frame that have been allocated by
                   the entry code sofar, including this instruction.
                   (tell the number of bytes to pop off, to get to previous
                    routine)

*exit_len = length of exit sequence.

exit_code [n] = 0, if after this instruction, all memory is still valid 
                     (and SP not modified yet)

exit_code [n] = 1, this instruction is the special LDO 8(R3),SP
                     to get to code [n] = 2 from here :
                     simulate the execution of LDWM -8(SP),R3

exit_code [n] = 2, all the callee saves registers have been restored,
                     and SP is pointing to remaining flt pt regs (if any)


exit_size [n] = number of bytes of the frame that are still on the stack
                  (useful for telling how much flt pt callee restore has been
                   done, if code [n] == 2)

*/

#define TRUE 1
#define FALSE 0

#define FIRST_CALLEE_REG 3
#define FIRST_FP_REG 32

build_entry_exit_info (unwd1, unwd2, 
                       entry_code, entry_size, entry_len,
                       exit_code,  exit_size,  exit_len)

   unsigned unwd1, unwd2;
   int *entry_code, *entry_size, *entry_len;
   int *exit_code,  *exit_size, *exit_len;

{
   int *old_entry_code = entry_code;
   int *old_exit_code  = exit_code;

   int frame_size;
   int number_sp_callee, number_fp_callee, number_callee;
   int save_RP, ak;

   int last_fp_callee_reg, last_callee_reg;
   int sp_callee_spill, fp_callee_spill, callee_spill;
   int bump_size;

   int cur_code, cur_size;
   int last_code, last_size;

   int RR_num, spill_offset;

/*
      unwd  = fp_callee_spill << (21 - 3);

      unwd |= (last_callee_reg - (FIRST_CALLEE_REG - 1)) << 16;

      unwd |= fp_caller_spill << (10 - 3);
      unwd |=    caller_spill << (5 -  2);
      unwd |= save_RP << 3;
      if (sp_callee_spill) {
         if (sp_caller_spill) unwd |= 3 << (25);
         else                 unwd |= 1 << (25);
      }

      which_proc () -> pr_unwind_descriptor. int1 |= unwd;
      which_proc () -> pr_unwind_descriptor. int2 |= frame_size >> 3;
*/

      /* get stuff out of unwind descriptor */
      frame_size = (unwd2 & 0x07FFFFFF) << 3;
      number_sp_callee = (unwd1 >> 25) & 3;
      number_fp_callee = (unwd1 >> 21) & 15;
      number_callee    = (unwd1 >> 16) & 31;
      save_RP = unwd1 & 8;
      ak = unwd2 & 0x20000000;

      /* compute some locals from unwind descriptor, so that the
         code below closely resembles the code in fa_entex.c
      */
      last_fp_callee_reg = number_fp_callee + (FIRST_FP_REG + 11);
      last_callee_reg    = number_callee    + (FIRST_CALLEE_REG - 1);

      fp_callee_spill = number_fp_callee << 3;
      callee_spill    = number_callee    << 2;

      sp_callee_spill = number_sp_callee;

      bump_size       = frame_size - fp_callee_spill;

      /* initiailize */
      cur_code = 0;
      cur_size = 0;

      if (save_RP) {

         /* STW R2,-20(SP) */
         *entry_code++ = 0;
         *entry_size++ = 0;

      }

      if (fp_callee_spill) {
        for (RR_num = (FIRST_FP_REG + 11);
	     ++RR_num != last_fp_callee_reg + 1;) {

              /* FSTDS,MA fpr<callee>,8(SP)   : PUSH flt pt callee saves reg */
              cur_size += 8;
              *entry_code++ = 0;
              *entry_size++ = cur_size;

        } /* for each FP callee reg */
      } /* fp callee spill */

      spill_offset = 0;
      if (callee_spill) {
	 for (RR_num = FIRST_CALLEE_REG - 1;
	      ++RR_num != last_callee_reg + 1;) {

	    if (spill_offset == 0) {

               if (ak) cur_size += 8;
               else    cur_size += bump_size;

               *entry_code++ = 0;
               *entry_size++ = cur_size;

	       if (ak) {
		  /* if > 8k then set up r3 as pseudo AP right away */

                  /* LDO -8-fpspill(SP),R3 */
                  *entry_code++ = 1;
                  *entry_size++ = cur_size;

                  /* ADDIL L'framesize,R3 */
                  *entry_code++ = 1;
                  *entry_size++ = cur_size;

                  /* LDO   R'framesize,R1,SP */
                  *entry_code++ = 1;
                  *entry_size++ = frame_size;

                  /* 8k frame just built, r3 modified ... */
                  cur_code = 1;
                  cur_size = frame_size;
	       }
	    }
	    else {
               /* just another callee register */

               /* STW r<callee>,-offset(SP) */
               *entry_code++ = cur_code;
               *entry_size++ = cur_size;
	    }

            spill_offset += 4;
         } /* for each reg to be spilled */

      } /* callee spill */
      else if (bump_size) { 
         /* no callee spill, but bump_size != 0;
            also, bump_size < 8k because if it were > 8k there would
            be callee spill */

         /* LDO bump_size(SP),SP */
         *entry_code++ = 0;
         *entry_size++ = bump_size;

      } /* no callee spill, but bump size != 0 */

      if (sp_callee_spill) {

         /* MFSP sp3, r1 */
         *entry_code++ = cur_code;
         *entry_size++ = cur_size;

         /* STW r1,offset(SP) */
         *entry_code++ = cur_code;
         *entry_size++ = cur_size;

      }


      /* set var parameter entry_len */

      *entry_len = entry_code - old_entry_code;


/***************************************************************************/

      last_code = cur_code = 0;
      last_size = cur_size = frame_size;

      if (save_RP) {

         /* LDW -20-frame_size(SP),R2 */
         *exit_code++ = 0;
         *exit_size++ = cur_size;

      } /* LDW -20(AP), RP */

      if (sp_callee_spill) {

         /* LDW spcalleespillvalue, r1 */
         *exit_code++ = 0;
         *exit_size++ = cur_size;

         /* MTSP r1, spr3 */
         *exit_code++ = 0;
         *exit_size++ = cur_size;
      }

      if (callee_spill) {
         RR_num = last_callee_reg;
         spill_offset = (RR_num - 3) << 2;
         while (RR_num != 2) {

	    if (spill_offset == 0) {
	       if (! ak) {

		  last_size = cur_size;
                  /* LDWM  bumpsize(SP),R3 */
                  cur_size -= bump_size;
                  *exit_code++ = 2;
                  *exit_size++ = cur_size;
	       }
	       else {

                  /* LDO 8+fpcalleespill(R3),SP */
                  last_code = *exit_code++ = 1;
                  last_size = *exit_size++ = fp_callee_spill + 8;

                  /* LDWM -8(SP),R3 */
                  *exit_code++ = 2;
                  *exit_size++ = fp_callee_spill;
	       }
               cur_code = 2;
               cur_size = fp_callee_spill;
	    }
	    else {

               /* LDW nn(SP),r<callee> */
               *exit_code++ = 0;
               *exit_size++ = cur_size;
	    }

	    RR_num--;
	    spill_offset -= 4;

         } /* while */
      }
      else if (bump_size) {

         /* LDO -bumpsize(SP),SP */
         *exit_code++ = 2;
         *exit_size++ = fp_callee_spill;

         cur_code = 2;
         cur_size = fp_callee_spill;
      }

      if (fp_callee_spill) {
	 for (RR_num = last_fp_callee_reg + 1;
	      --RR_num != (FIRST_FP_REG + 11);) {

	    last_code = cur_code;
	    last_size = cur_size;
            /* FLDDS,MB -8(SP),fpr<calleee> */
	    cur_code = 2;
            cur_size -= 8;
            *exit_code++ = cur_code;
            *exit_size++ = cur_size;

	 } /* for */
      } /* fp callee spill */

      if (callee_spill || bump_size || fp_callee_spill) {

         /* insert BV 0(R2) inbetween last two */

         /* slide previous one down one */
         exit_code [0] = exit_code [-1];
         exit_size [0] = exit_size [-1];

         /* insert a copy of one before that into previous */
         exit_code [-1] = last_code;
         exit_size [-1] = last_size;

         /* one more element */
         exit_code++;
         exit_size++;
      }
      else {

         /* BV 0(R2) */
         *exit_code++ = 2;
         *exit_size++ = 0;

         /* note : this nop have been scheduled, 
                   but it must have been removed */

         /* NOP */
         *exit_code++ = 2;
         *exit_size++ = 0;
      }

      *exit_len = exit_code - old_exit_code;

} /* build_entry_exit_array () */


