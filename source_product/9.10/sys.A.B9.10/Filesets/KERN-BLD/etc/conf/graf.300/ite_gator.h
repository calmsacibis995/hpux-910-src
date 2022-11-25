/*
 * @(#)ite_gator.h: $Revision: 1.3.84.3 $ $Date: 93/09/17 20:59:14 $
 * $Locker:  $
 */

#ifndef __ITE_GATOR_H_INCLUDED
#define __ITE_GATOR_H_INCLUDED

#ifdef _KERNEL
#   define GATORAID_UCODE_LEN 29
    extern short gatoraid_microcode[];

    extern gatorbox_write(), gatorbox_cursor(), gatorbox_clear(),
	   gatorbox_scroll(), gatorbox_init();
#endif

#endif /* __ITE_GATOR_H_INCLUDED */
