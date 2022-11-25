#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 70.5 $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/vmmeter.h>
#include <sys/vmsystm.h>
#include <sys/dk.h>
#include <nlist.h>
#include <sys/buf.h>
#include <sys/pstat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef SecureWare
#include <sys/security.h>
#endif

/* forward declarations */
void usage();
void print_header();
void print_new_header();
void zerosum();
void doforkst();
void dosum();

/*
 * The following nlist structure is used only for finding the
 * beginning of the sum structure in kmem in order to zero
 * it out when the z option is specified.
 */
struct nlist nl[] = {
#define X_SUM 0
#ifdef __hp9000s300
   { "_sum" },
#else
#ifdef __hp9000s800
   { "sum" },
#endif /* __hp9000s800 */
#endif /* __hp9000s300 */
   { "" },
};

int      luonly;        /* flag to display only logical unit (800) */

#define DKBURST 8       /* # of pstat diskinfo entries to read at a time */

main(argc, argv)
     int argc;
     char **argv;
{

   int   dflag = 0,     /* d option flag */
         fflag = 0,     /* f option flag */
         sflag = 0,     /* s option flag */
         Sflag = 0,     /* S option flag */
         zflag = 0,     /* z option flag */
         nflag = 0,     /* n option flag */
         first = 1,     /* first time flag */
         dSflag = 0,    /* d or S option used */
         fszflag = 0,   /* f,s or z option used */
         lines = 0,     /* Num lines to display before reprinting header */
	 mult_stat = 1, /* multiple stat flag */
         c,
         *lumap,        /* array of ordered map from MI to LU */
	 *remap,        /* array of map to index in diskinfo */
         idx = 0,       /* index for diskinfo array */
         numents = 0;   /* # of valid diskinfo entries */

   time_t now;          /* current time - seconds since epoch */

   register int i,j;

   int interval = 0, /* seconds between output of stats */
       count    = 0, /* number of times stats are output */
       countset = 0; /* true if count is given */

   extern   int optind; /* index into argv[] for getopt */

   /*
    * variables to hold vm stats before printing
    * the names roughly correspond to the heading names in the output
    */
   unsigned long  r,         /* procs in run queue */
         b,          /* procs blocked for resources */
         w,          /* procs runnable or short sleeper but swapped */
         avm,        /* active virtual pages */
         free,       /* size of the free list */
         si,         /* procs swapped in */
         so,         /* procs swapped out */
         re,         /* page reclaims */
         at,         /* address translations faults */
         pi,         /* pages paged in */
         po,         /* pages paged out */
         fr,         /* pages freed per second */
         de,         /* anticipated short term memory shortfall */
         sr,         /* pages scanned by clock algorithm per second */
         in,         /* (non clock) device interrupts */
         sy,         /* system calls per second */
         cs;         /* cpu context switches per second */

   float usr_old,        /* %old format cpu in user (normal and niced) */
         sys_old,        /* %old format cpu in system modes */
         idle_old;       /* %old format cpu in idle modes */

   float usr[PST_MAX_PROCS],        /* %cpu in user (normal and niced) */
         sys[PST_MAX_PROCS],        /* %cpu in system modes */
         idle[PST_MAX_PROCS],       /* %cpu in idle modes */
         t;

   unsigned long  oldf_cpu_time[PST_MAX_CPUSTATES], 
				  /* old format clock ticks as of previous */
                                  /*     pass through main loop */
         old_cpu_time[PST_MAX_PROCS][PST_MAX_CPUSTATES], 
					  /* clock ticks as of previous */
                                          /*     pass through main loop */
         old_psv_sswpin = 0,              /* swapins as of prev pass */
         old_psv_sswpout = 0,             /* swapouts as of prev pass */
         cpticks[ PST_MAX_PROCS],                         /* Total ticks since last pass */
         cpticks_old;                     /* Total ticks since last pass */

   double etime;                          /* Total seconds since last pass */


   struct   pst_static     static_info;
   struct   pst_dynamic    dynamic_info;
   struct   pst_vminfo     vminfo;
   struct   pst_diskinfo   *diskinfo, *old_diskinfo;



   /*
    * PARSE OPTIONS
    */
   while ((c = getopt(argc, argv, "ndSfsz")) != EOF)
   {
      switch(c)
      {
         /*
          * Report disk transfer information
          */
         case 'd':
                  if (fszflag)
                  {
                     fprintf(stderr,"Option d cannot be specified along with f, s or z option.\n");
                     usage();
                     exit(1);
                  }
                  dflag++;
                  dSflag++;
                  break;
         /*
          * Report # processes swapped in/out instead of page reclaims
          * and address translation faults
          */
         case 'S':
                  if (fszflag)
                  {
                     fprintf(stderr,"Option S cannot be specified along with f, s or z option.\n");
                     usage();
                     exit(1);
                  }
                  Sflag++;
                  dSflag++;
                  break;
         /*
          * Report only [v]fork information
          */
         case 'f':
                  if (fszflag || dSflag)
                  {
                     fprintf(stderr,"Option f cannot be specified along with any other options.\n");
                     usage();
                     exit(1);
                  }
                  fflag++;
                  fszflag++;
                  break;
         /*
          * print out sum structure (actually only part of it)
          */
         case 's':
                  if (fszflag || dSflag)
                  {
                     fprintf(stderr,"Option s cannot be specified along with any other options.\n");
                     usage();
                     exit(1);
                  }
                  sflag++;
                  fszflag++;
                  break;
         /*
          * clear the sum structure
          */
         case 'z':
                  if (fszflag || dSflag)
                  {
                     fprintf(stderr,"Option z cannot be specified along with any other options.\n");
                     usage();
                     exit(1);
                  }
                  zflag++;
                  fszflag++;
                  break;
         case 'n':
                  nflag++;
                  break;
         case '?':
                  usage();
                  exit(1);
      }  
   }

   /*
    * Check whether there are any extraneous arguments if f, s, or z
    * options have been specified.
    */
   if ( fszflag && optind < argc )
   {
      if (fflag)
      {
         fprintf(stderr,"Option f cannot be specified along with any other options.\n");
         usage();
         exit(1);
      } else if (sflag)
      {
         fprintf(stderr,"Option s cannot be specified along with any other options.\n");
         usage();
         exit(1);
      } else
      {
         fprintf(stderr,"Option z cannot be specified along with any other options.\n");
         usage();
         exit(1);
      }
   }

   /*
    * Remaining arguments, if any, are the interval in seconds between
    * outputting stats and the count of how many times to output stats
    */
   if (optind < argc)
   {
      interval = atoi(argv[optind++]);
      if (interval < 0)
      {
         fprintf(stderr, "vmstat: interval (%d) must be > 0\n", interval);
         exit(1);
      }
      if (optind < argc)
      {
         count = atoi(argv[optind++]);
         if (count < 0)
         {
            fprintf(stderr, "vmstat: count (%d) must be > 0\n", count);
            exit(1);
         }
         countset++;
      }

   }
      
   /*
    * Remaining arguments, if any, are an error.
    */
   if (optind < argc)
   {
      fprintf(stderr,"Unknown option (%s).\n",argv[optind]);
      usage();
      exit(1);
   }

#if defined(SecureWare) && defined(B1)
	if (ISB1)
				/* force SEC_OWNER so pstat(2) will work */
		(void) forcepriv(SEC_OWNER);
#endif
                  
   if (zflag)
   {
#ifdef SecureWare
      zerosum(argc,argv);     /* zero out the sum structure in /dev/kmem */
#else
      zerosum();     /* zero out the sum structure in /dev/kmem */
#endif
      exit(0);
   }
   else if (fflag)
   {
      doforkst();    /* print out fork statistics */
      exit(0);
   }
   else if (sflag)
   {
      dosum();       /* print out the sum structure */
      exit(0);
   }

   /*
    * Get system information
    */
   pstat(PSTAT_STATIC, &static_info, sizeof(struct pst_static), 0, 0);
   pstat(PSTAT_DYNAMIC, &dynamic_info, sizeof(struct pst_dynamic), 0, 0);
   pstat(PSTAT_VMINFO, &vminfo, sizeof(struct pst_vminfo), 0, 0);

   /*
    * First time through main loop we use the sum structure stats and 
    * divide by seconds since boottime. Subsequent times through main
    * loop we use rate structure stats. To facilitate this we will
    * just dump sum struct stats divided by seconds since boottime into
    * the rate struct locations first time. The main loop will then merely
    * output rate structure stats each time including the first.
    */

   /* Get seconds since boottime */
   time(&now);
   etime = (float) (now - (time_t)static_info.boot_time);

   /* Copy sum struct stats/etime into rate struct stats */
   vminfo.psv_rxsfrec   = (unsigned long)vminfo.psv_sxsfrec / etime;
   vminfo.psv_rxifrec   = (unsigned long)vminfo.psv_sxifrec / etime;
   vminfo.psv_rpgrec    = (unsigned long)vminfo.psv_spgrec / etime;
   vminfo.psv_rpgpgin   = (unsigned long)vminfo.psv_spgpgin / etime;
   vminfo.psv_rpgpgout  = (unsigned long)vminfo.psv_spgpgout / etime;
   vminfo.psv_rdfree    = (unsigned long)vminfo.psv_sdfree / etime;
   vminfo.psv_rscan     = (unsigned long)vminfo.psv_sscan / etime;
   vminfo.psv_rintr     = (unsigned long)vminfo.psv_sintr / etime;
   vminfo.psv_rsyscall  = (unsigned long)vminfo.psv_ssyscall / etime;
   vminfo.psv_rswtch    = (unsigned long)vminfo.psv_sswtch / etime;

   /* clear structs/arrays that hold previous data */
   memset(old_cpu_time, 0, sizeof(old_cpu_time));

   /* Get disk information if dflag set */
   if (dflag)
   {
      /*
       * Determine whether only logical unit number (Series 800) should be
       * displayed in header or if it should be either select code/bus
       * address (Series 300) or select code/function number/bus address
       * (Series 700).  The latter two are the upper 16 bits of the 24-bit
       * minor number while the former is the middle eight bits.
       */
   
#ifdef __hp9000s300
      luonly = 0;
#endif
#ifdef __hp9000s800
      luonly = (sysconf(_SC_IO_TYPE) == IO_TYPE_SIO);
#endif

      /* calloc initial space for diskinfo array */
      if ((diskinfo = (struct pst_diskinfo *)
                           calloc(DKBURST, sizeof(struct pst_diskinfo))) == NULL)
      {
         perror("vmstat");
         exit(1);
      }

      /* Fill diskinfo array */
      numents = 0;
      while ((c = pstat(PSTAT_DISKINFO, diskinfo + numents,
                           sizeof(struct pst_diskinfo), DKBURST, idx)) > 0)
      {
         /* There may be more data so realloc more space - leaving */
         /* DKBURST available entries */
         numents += c;
         idx = diskinfo[numents - 1].psd_idx + 1; 
	 /*idx += c;*/
         if ((diskinfo = (struct pst_diskinfo *)
                   realloc((struct pst_diskinfo *) diskinfo,
                   (numents + DKBURST) * sizeof(struct pst_diskinfo))) == NULL)
         {
            perror("vmstat");
            exit(1);
         }
         /* clear the extra entries */
         memset(diskinfo + numents, 0, DKBURST * sizeof(struct pst_diskinfo));
   
      }
   
      if ((old_diskinfo = (struct pst_diskinfo *)
                        calloc(numents, sizeof(struct pst_diskinfo))) == NULL)
      {
         perror("vmstat");
         exit(1);
      }

      /*
       * Determine mapping from manager index (MI) to logical unit (LU).
       * Data from pstat is ordered by manager index.  It needs to be
       * displayed sorted by logical unit number.  Algorithm is based
       * upon Algorithm C (comparison counting) in Knuth Vol. 3.  Once the
       * order is obtained, change the order in order to access diskinfo
       * in the proper order.
       */
   
      /*
       * Zero out logical unit map (counts)
       */
      if ((lumap = (int *) calloc(numents, sizeof(int))) == NULL) {
         perror("iostat");
         exit(1);
      }

#define LU(x) ((diskinfo[x].psd_dev.psd_minor >> 8) & 255)
#define MINOR(x) (diskinfo[x].psd_dev.psd_minor)
#define SELBUS(x) ((diskinfo[x].psd_dev.psd_minor >> 8 ) & 0xffff)

      /* Compare all logical unit numbers with each other.  Whichever
       * one is higher gets incremented.
       */
      for (i = 0; i< numents - 1; i++)
         for (j= i + 1; j < numents; j++)
            if (MINOR(i) > MINOR(j))
               lumap[i]++;
            else
               lumap[j]++;

      /*
       * Zero out real map (indices)
       */
      if ((remap = (int *) calloc(numents, sizeof(int))) == NULL) {
         perror("iostat");
         exit(1);
      }

      /* The lumap contains the order of the diskinfo entries. Now
       * they have to be translated into indices into the diskinfo array.
       */
      for (i = 0; i < numents; i++)
         for (j = 0; j < numents; j++)
            if (i == lumap[j]) {
               remap[i] = j;
               break;
            };
   }

   if( nflag)
       print_new_header(dflag, Sflag, &lines, 1);
   else
       print_header(dflag, Sflag, &lines);

   /*
    ***** MAIN LOOP *****
    */
   do
   {
   
      /*
       * GATHER VM STATISTICS
       *
       * Many of the following assignments could be eliminated but
       * are left here for the sake of clarity and readability.
       */

#define XDIFF(var, ovar) t = var; var -= ovar; ovar = t;
      if ( nflag )
      {
          for( i = 0; i < dynamic_info.psd_proc_cnt; i++)
             cpticks[i] = 0;
          for ( j = 0; j < dynamic_info.psd_proc_cnt; j++)
             for (i = 0; i < static_info.cpu_states; i++)
             {
                XDIFF(dynamic_info.psd_mp_cpu_time[j][i],old_cpu_time[j][i]);
                cpticks[j] += dynamic_info.psd_mp_cpu_time[j][i];
             }
          for( i = 0; i < dynamic_info.psd_proc_cnt; i++)
          if (cpticks[i] == 0)
             cpticks[i] = 1;
          etime = cpticks[0] / (float) HZ;
       }
       else
       {
          cpticks_old = 0;
          for (i = 0; i < static_info.cpu_states; i++)
          {
             XDIFF(dynamic_info.psd_cpu_time[i],oldf_cpu_time[i]);
             cpticks_old += dynamic_info.psd_cpu_time[i];
          }
          if (cpticks_old == 0)
             cpticks_old = 1;
          etime = cpticks_old / (float) HZ;
       }
   
   

      r = dynamic_info.psd_rq;
      b = dynamic_info.psd_dw + dynamic_info.psd_pw;
      w = dynamic_info.psd_sw;
   
      avm = dynamic_info.psd_avm;
      free = vminfo.psv_cfree;
   
      si = (unsigned)vminfo.psv_sswpin - old_psv_sswpin;
      so = (unsigned)vminfo.psv_sswpout - old_psv_sswpout;
   
      at = vminfo.psv_rxsfrec + vminfo.psv_rxifrec;
      re = vminfo.psv_rpgrec - at;
      pi = vminfo.psv_rpgpgin;
      po = vminfo.psv_rpgpgout;
      fr = vminfo.psv_rdfree;
      de = vminfo.psv_deficit;
      sr = vminfo.psv_rscan;
   
#ifdef __hp9000s300
      if( vminfo.psv_rintr < HZ )
	  in = 0;
      else
          in = vminfo.psv_rintr - HZ;
#else
#ifdef __hp9000s800
      if( vminfo.psv_rintr < (2 * HZ))
	  in = 0;
      else
          in = vminfo.psv_rintr - (2 * HZ);
#endif /* __hp9000s800 */
#endif /* __hp9000s300 */
      sy = vminfo.psv_rsyscall;
      cs = vminfo.psv_rswtch;
      if(nflag)
      {
          for ( i = 0; i < dynamic_info.psd_proc_cnt; i++)
          {
              usr[i] = 100. * (dynamic_info.psd_mp_cpu_time[i][CP_USER] +
                       dynamic_info.psd_mp_cpu_time[i][CP_NICE]) / cpticks[i];
              sys[i] = dynamic_info.psd_mp_cpu_time[i][CP_SYS];
#ifdef CP_BLOCK
              sys[i] += dynamic_info.psd_mp_cpu_time[i][CP_BLOCK];
#endif
#ifdef CP_SWAIT
              sys[i] += dynamic_info.psd_mp_cpu_time[i][CP_SWAIT];
#endif
#ifdef CP_INTR
              sys[i] += dynamic_info.psd_mp_cpu_time[i][CP_INTR];
#endif
#ifdef CP_SSYS
              sys[i] += dynamic_info.psd_mp_cpu_time[i][CP_SSYS];
#endif
              sys[i] = 100. * sys[i] / cpticks[i];
              idle[i] = dynamic_info.psd_mp_cpu_time[i][CP_IDLE];
#ifdef CP_WAIT
             idle[i] +=  dynamic_info.psd_mp_cpu_time[i][CP_WAIT];
#endif /* CP_WAIT */
             idle[i] = 100. * idle[i] / cpticks[i];
        }
    }
   else
   {
          usr_old = 100. * (dynamic_info.psd_cpu_time[CP_USER] +
                       dynamic_info.psd_cpu_time[CP_NICE]) / cpticks_old;
          sys_old = dynamic_info.psd_cpu_time[CP_SYS];
#ifdef CP_BLOCK
          sys_old += dynamic_info.psd_cpu_time[CP_BLOCK];
#endif
#ifdef CP_SWAIT
          sys_old += dynamic_info.psd_cpu_time[CP_SWAIT];
#endif
#ifdef CP_INTR
          sys_old += dynamic_info.psd_cpu_time[CP_INTR];
#endif
#ifdef CP_SSYS
          sys_old += dynamic_info.psd_cpu_time[CP_SSYS];
#endif
          sys_old = 100. * sys_old / cpticks_old;
          idle_old = dynamic_info.psd_cpu_time[CP_IDLE];
#ifdef CP_WAIT
         idle_old +=  dynamic_info.psd_cpu_time[CP_WAIT];
#endif /* CP_WAIT */
         idle_old = 100. * idle_old / cpticks_old;
     }
   
      /*
       * OUTPUT STATISTICS
       */
       if(! nflag)
       {
	    printf("%5d %5d %5d ", r, b, w); 
	   
	    printf(" %7d  %6d ", avm, free);
	   
	    if (Sflag)
		 printf("%4d %4d ", si, so);
	    else
		 printf("%4d %4d ", re, at);
	   
	    printf(" %4d ", pi);
	    printf("%4d ", po);
	    printf(" %4d %4d ", fr, de);
	    printf(" %4d ", sr);
	    printf(" %5d %6d %5d ", in, sy, cs);
	    printf(" %2.0f %2.0f %2.0f\n", usr_old, sys_old, idle_old); 
		
       }
       else
       {
	    printf(" %7d  %6d ", avm, free);
	   
	    if (Sflag)
		 printf("%4d %4d ", si, so);
	    else
		 printf("%4d %4d ", re, at);
	   
	    printf(" %4d ", pi);
	    printf("%4d ", po);
	    printf(" %4d %4d ", fr, de);
	    printf(" %4d ", sr);
	    printf(" %5d %6d %5d\n", in, sy, cs);
	    if( first)
	    {
	        first = 0;
                print_new_header(dflag, Sflag,&lines,  0);
	    }	
	    if( mult_stat )
	    {
		mult_stat = 0;
	        printf(" %2.0f %2.0f %2.0f", usr[0], sys[0], idle[0]); 
	        printf("%5d %5d %5d\n", r, b, w); 
	    }
	    for( i = 1; i < dynamic_info.psd_proc_cnt; i++)
	   	printf(" %2.0f %2.0f %2.0f\n", usr[i], sys[i], idle[i]); 
	   
      } 
      if (dflag)     /* output disk transfer info */
      {
         /*
          * We assume that the diskinfo array is small (<= 24).
          * If it is large a better algorithm than just looping through
          * the array each time should be used.
          * The entries are no longer separated by disk type since now
          * there is a single pool of logical units for all disk types.
          * This greatly simplifies the code.
          */

	 /*
	  * For these calculations, don't worry about logical unit.
	  */
         for (i=0; i < numents; i++)
         {
            XDIFF(diskinfo[i].psd_dkxfer, old_diskinfo[i].psd_dkxfer);
         }

         printf("\n\t\tdisk transfers\n");

         /* Output headers for disks */
         for (i=0; i < numents; i++)
         {
	    j = remap[i];
            if (diskinfo[j].psd_dkmspw != 0.0)
               if (luonly)
                  printf("dk%1x\t", LU(j));
	       else
                  printf("dk%04x\t", SELBUS(j));
         }

         printf("\n");

         /* Output xfers for disks */
         for (i=0; i < numents; i++)
         {
	    j = remap[i];
            if (diskinfo[j].psd_dkmspw != 0.0)
	    {
               if (luonly)
                  printf("%2.0f\t", diskinfo[j].psd_dkxfer / etime);
	       else
                  printf("%4.0f\t", diskinfo[j].psd_dkxfer / etime);
	    }
         }

         printf("\n\n\n");
      }

      fflush(stdout);
   
      if (interval)
      {
         if (--count || !countset)
         {
            sleep(interval);
            if (--lines <= 0)
	       if( nflag)
	       {
                  print_new_header(dflag, Sflag, &lines, 1);
		  first++;
               }
	       else
                  print_header(dflag, Sflag, &lines);
	    mult_stat++;

            /* Save current values for next loop */
            old_psv_sswpin = vminfo.psv_sswpin;
            old_psv_sswpout = vminfo.psv_sswpout;

            /* Get new statistics */
            pstat(PSTAT_DYNAMIC, &dynamic_info,
                        sizeof(struct pst_dynamic), 0, 0);
            pstat(PSTAT_VMINFO, &vminfo,
                        sizeof(struct pst_vminfo), 0, 0);
            if (dflag)
               pstat(PSTAT_DISKINFO, diskinfo, sizeof(struct pst_diskinfo),
                     numents, 0);
            if (!countset)
               count = 2;
         }
      }
   } while (count);
}


void
usage()
{
   fprintf(stderr, "usage: vmstat [ [ -dnS ] [ interval [ count ] ] | -f | -s | -z ]\n");
}


void
print_header(dflag, Sflag, lines_ptr)
int   dflag,
      Sflag,
      *lines_ptr;
{
   if (dflag)
      *lines_ptr = 1;
   else
      *lines_ptr= 20;
   printf("\
         procs           memory                   page                              faults       cpu\n\
    r     b     w      avm    free   %7s    pi   po    fr   de    sr     in     sy    cs  us sy id\n", Sflag ? "si   so" : "re   at"); 
}


void
print_new_header(dflag, Sflag, lines_ptr, lflag)
int   dflag,
      Sflag,
      *lines_ptr,
      lflag;
{
   if (dflag)
      *lines_ptr = 1;
   else
      *lines_ptr= 10;
   if( lflag)
       printf("VM\n\
       memory                     page                          faults\n\
     avm    free   %7s    pi   po    fr   de    sr     in     sy    cs  \n", Sflag ? "si   so" : "re   at"); 
   else
       printf("CPU\n\
    cpu          procs \n\
 us sy id    r     b     w \n");
}


/*
 * zerosum() writes an empty sum structure to /dev/kmem
 * at the position of the current sum structure thus reseting
 * all sum structure values to zero.
 */
void
#ifdef SecureWare
zerosum(argcount, vec)
int argcount;
char **vec;
#else
zerosum()
#endif
{
   struct vmmeter sum;
   int    mf;

#ifdef SecureWare
   if (ISSECURE)  {
	set_auth_parameters(argcount, vec);

#ifdef B1
	if (ISB1) {
		initprivs();
		(void) forcepriv(SEC_ALLOWDACACCESS);
		(void) forcepriv(SEC_ALLOWMACACCESS);

	} 
#endif

	if (!authorized_user("mem"))  {
		fprintf(stderr, "vmstat: no authorization to zero values\n");
		exit(1);
	}
   }
#endif
   nlist("/hp-ux", nl);
   if(nl[0].n_type == 0)
   {
      fprintf(stderr,
              "vmstat: no /hp-ux namelist, cannot clear sum structure\n");
      exit(1);
   }
   mf = open("/dev/kmem", O_RDWR);
   if (mf < 0)
   {
      fprintf(stderr, "vmstat: ");
      perror("/dev/kmem");
      exit(1);
   }
   if (lseek(mf, (long)nl[X_SUM].n_value, SEEK_SET) == -1)
   {
      fprintf(stderr, "vmstat: ");
      perror("/dev/kmem");
      exit(1);
   }
   memset((void *)&sum, 0, sizeof(sum));
   if (write(mf, &sum, sizeof sum) < 0)
   {
      fprintf(stderr, "vmstat: ");
      perror("/dev/kmem");
      exit(1);
   }
}
      

void
doforkst()
{
   struct pst_vminfo vminfo;

   pstat(PSTAT_VMINFO, &vminfo, sizeof(vminfo), 0, 0);
   printf("%d forks, %d pages, average=  %2.2f\n",
            vminfo.psv_cntfork,
            vminfo.psv_sizfork,
            (float)vminfo.psv_sizfork/vminfo.psv_cntfork);
}

void
dosum()
{
   struct pst_vminfo vminfo;

   pstat(PSTAT_VMINFO, &vminfo, sizeof(vminfo), 0, 0);
   printf("%lu swap ins\n", vminfo.psv_sswpin);
   printf("%lu swap outs\n", vminfo.psv_sswpout);
   printf("%lu pages swapped in\n", vminfo.psv_spswpin);
   printf("%lu pages swapped out\n", vminfo.psv_spswpout);
   printf("%lu total address trans. faults taken\n", vminfo.psv_sfaults);
   printf("%lu page ins\n", vminfo.psv_spgin);
   printf("%lu page outs\n", vminfo.psv_spgout);
   printf("%lu pages paged in\n", vminfo.psv_spgpgin);
   printf("%lu pages paged out\n", vminfo.psv_spgpgout);
   printf("%lu reclaims from free list\n", vminfo.psv_spgfrec);
   printf("%lu total page reclaims\n", vminfo.psv_spgrec);
   printf("%lu intransit blocking page faults\n", vminfo.psv_sintrans);
   printf("%lu zero fill pages created\n", vminfo.psv_snzfod);
   printf("%lu zero fill page faults\n", vminfo.psv_szfod);
   printf("%lu executable fill pages created\n", vminfo.psv_snexfod);
   printf("%lu executable fill page faults\n", vminfo.psv_sexfod);
   printf("%lu swap text pages found in free list\n", vminfo.psv_sxsfrec);
   printf("%lu inode text pages found in free list\n", vminfo.psv_sxifrec);
   printf("%lu revolutions of the clock hand\n", vminfo.psv_srev);
   printf("%lu pages scanned for page out\n", vminfo.psv_sscan);
   printf("%lu pages freed by the clock daemon\n", vminfo.psv_sdfree);
   printf("%lu cpu context switches\n", vminfo.psv_sswtch);
   printf("%lu device interrupts\n", vminfo.psv_sintr);
   printf("%lu traps\n", vminfo.psv_strap);
   printf("%lu system calls\n", vminfo.psv_ssyscall);
}
