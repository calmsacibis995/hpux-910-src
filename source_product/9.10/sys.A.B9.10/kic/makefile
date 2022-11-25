# $Source: /source/hpux_source/kernel/sys.SWT68K_800/kic/RCS/makefile,v $
# $Revision: 1.2.83.5 $	$Author: kcs $
# $State: Exp $   	$Locker:  $
# $Date: 94/03/07 09:47:35 $

CFLAGS= -Wl,-a,archive -O -DFSD_KI -DMP -D_KERNEL_BUILD -I. -I../Filesets/BUILDENV/usr/include

default: all

all:    kic 

kic:	kic.o 
	$(CC) $(CFLAGS) kic.o -o kic
	chown root kic
	chmod 4555 kic
	ln kic ktc

set_nams.h:	../h/syscall.h makefile
	awk 'BEGIN {for (i=0; i<500; i++) { \
	    lbl[i]=sprintf("SYSCALL") \
	  } \
	  printf("void\nset_nams(print_l)\nstruct print_list print_l[];\n{\n"); \
	} \
	/^#define[ 	]*SYS_[a-z]/ { \
	  lbl[$$3]=substr($$2,5,99) \
	} \
	END {for (i=0; i<500; i++) { \
	    printf "  print_l[%d].trace_name = \"%s_%d\";\n",i,lbl[i],i\
	  } \
  	    printf("  print_l[KI_SWAPPER].trace_name     = \"SSYS_swapper_498\";\n"); \
	    printf("  print_l[KI_VHAND].trace_name       = \"SSYS_vhand_497\";\n"); \
	    printf("  print_l[KI_STATDAEMON].trace_name  = \"SSYS_statdaemon_496\";\n"); \
	    printf("  print_l[KI_SOCKREGD].trace_name    = \"SSYS_sockregd_495\";\n"); \
	    printf("  print_l[KI_NETISR].trace_name      = \"SSYS_netisr_494\";\n"); \
	    printf("  print_l[KI_SYNCDAEMON].trace_name  = \"SSYS_syncdaemon_493\";\n"); \
	    printf("  print_l[KI_GCSP].trace_name        = \"SSYS_gcsp_492\";\n"); \
	    printf("  print_l[KI_LCSP].trace_name        = \"SSYS_lcsp_491\";\n"); \
	    printf("  print_l[KI_SPINUPD].trace_name     = \"SSYS_spinupd_490\";\n"); \
	    printf("  print_l[KI_XPORTD].trace_name      = \"SSYS_xportd_489\";\n"); \
	    printf("  print_l[KI_UNHASHDAEMON].trace_name= \"SSYS_unhashdaemon_488\";\n"); \
	    printf("  print_l[KI_VDMAD].trace_name	 = \"SSYS_vdmad_487\";\n"); \
	    printf("  print_l[KI_LVMKD].trace_name	 = \"SSYS_lvmkd_486\";\n"); \
  	    printf("}\n") \
	}' <../h/syscall.h >set_nams.h

kic.o:		kic.c set_nams.h ../h/ki_calls.h
	$(CC) $(CFLAGS) -c kic.c

clean:
	/bin/rm -f *.o set_nams.h kic ktc

clobber:  clean
	/bin/rm -f kic ktc

