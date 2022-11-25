/*
 * @(#)unwind.h: $Revision: 1.7.83.3 $ $Date: 93/09/17 16:32:26 $
 * $Locker:  $
 */


/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

/*
 * unwind descriptor
 */
struct unwindDesc {
	unsigned int	start_ofs, end_ofs;
	unsigned int	cannot_unwind : 1;
	unsigned int	millicode : 1;
	unsigned int	millisave_save_sr0 : 1;
	unsigned int	region_desc : 2;
	unsigned int	reserved1 : 1;
	unsigned int	entry_sr : 1;
	unsigned int	entry_fr : 4;
	unsigned int	entry_gr : 5;
	unsigned int	args_stored : 1;
	unsigned int	variable_frame : 1;
	unsigned int	separate_package_body : 1;
	unsigned int	frame_extension_millicode : 1;
	unsigned int	stack_overflow_check : 1;
	unsigned int	two_instruction_SP_increment : 1;
	unsigned int	reserved2 : 5;
	unsigned int	save_sp : 1;
	unsigned int	save_rp : 1;
	unsigned int	save_mrp_in_frame : 1;
	unsigned int	reserved3 : 1;
	unsigned int	cleanup : 1;
	unsigned int	hpe_int : 1;
	unsigned int	hpux_int : 1;
	unsigned int	large_frame_r3 : 1;
	unsigned int	reserved4 : 2;
	unsigned int	frame_size : 27;
};
