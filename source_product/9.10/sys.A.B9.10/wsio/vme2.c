/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/vme2.c,v $
 * $Revision: 1.2.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:35:24 $
 */

vme_bus_error_check() {}
vme_map_mem_to_host() {}
vme_unmap_mem_from_host() {}
vme_map_mem_to_bus() {}
vme_unmap_mem_from_bus() {}
vme_testr() {}
vme_testw() {}

vme_isrlink() {}

vme2_open() { }
vme2_close() { }
vme2_ioctl() { }
vme_isrunlink() {}
vme_dma_setup() {}
vme_dma_cleanup() {}

vme_present() {return(1);}

vme_init() 
{
	printf("\n===============================================================\n");
	printf("VME Hardware has been detected and vme2 was in the dfile for\n");
	printf("this kernel. This message is from the unsupported vme_init\n");
	printf("hook.\n");
	printf("===============================================================\n");
}
