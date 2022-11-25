/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/cdfs/RCS/cdfs_hooks.c,v $
 * $Revision: 1.4.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:36:43 $
 */

/*
** For CDFS configurability, this array of indirect functions is
** included.  Its entries are initially set to point to the cdfs_nop
** routine.  The table is filled in at boot time by the cdfs_link
** routine if CDFS is configured into the kernel.
** They are called with the CDFSCALL macro in cdfs_hooks.h.
*/

int cdfs_nop()
{
return(0);
}

int cdfs_initialized = 0;

/* Initialize all entries to the cdfs_nop routine. */

int (*cdfsproc[])() = {
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,
/* two more added for a test */
	cdfs_nop,
	cdfs_nop,
/* strategy; for recognizing demand paging on s800 */
	cdfs_nop,
	cdfs_nop,
	cdfs_nop,	/*fsctl*/
};

