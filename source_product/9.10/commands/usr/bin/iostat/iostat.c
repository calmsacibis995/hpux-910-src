#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 70.1 $";
#endif

/*
 * Note: At this time the 300 does not support scsi so there are
 * several hacks put in to handle the special case where no
 * valid disk statistics are recorded. When scsi support is
 * added these hacks could be removed. The comments associated
 * with the hacks have "scsi" in them so just search for "scsi".
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/vmsystm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <unistd.h>
#include <sys/pstat.h>

/* forward declarations */
void usage();
void stats();

int	    luonly;        /* flag to display only logical unit (800) */

#define DKBURST 50       /* # of pstat diskinfo entries to read at a time */

main(argc, argv)
     int argc;
     char **argv;
{

   int      tflag = 0,     /* t option flag */
            lines = 0,     /* # lines to display before reprinting header */
            numhead,       /* # subheaders to display (i.e. "bps ...") */
	    *lumap,        /* array of ordered map from MI to LU */
	    *remap,        /* array of map to index in diskinfo */
            c,
            idx = 0,       /* index for diskinfo array */
            numents = 0;   /* # of valid diskinfo entries */

   register int i,j;

   int interval = 0, /* seconds between output of stats */
       count    = 0, /* number of times stats are output */
       countset = 0; /* true if count is given */

   extern   int optind; /* index into argv[] for getopt */

   double   cpticks,
            etime;

   long     old_cpu_time[PST_MAX_CPUSTATES],
            old_tknin = 0,
            old_tknout = 0;

   float    usr,
            nice,
            sys,
            idle,
            t;

   struct   pst_static     static_info;
   struct   pst_dynamic    dynamic_info;
   struct   pst_vminfo     vminfo;
   struct   pst_diskinfo   *diskinfo, *old_diskinfo;


   while ((c = getopt(argc, argv, "t")) != EOF)
   {
      switch(c)
      {
         /*
          * Report char's read/written to terminals and cpu utilization
          */
         case 't':
                  tflag++;
                  break;
         case '?':
                  usage();
                  exit(1);
      }  
   }

   /*
    * Remaining arguments, if any, are the interval between outputting
    * stats and the count of how many times to output stats
    */
   if (optind < argc)
   {
      interval = atoi(argv[optind++]);
      if (interval < 0)
      {
         fprintf(stderr, "iostat: interval (%d) must be > 0\n", interval);
         exit(1);
      }
      if (optind < argc)
      {
         count = atoi(argv[optind++]);
         if (count < 0)
         {
            fprintf(stderr, "iostat: count (%d) must be > 0\n", count);
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

   /*
    * Get system information
    */

   pstat(PSTAT_STATIC, &static_info, sizeof(struct pst_static), 0, 0);
   pstat(PSTAT_DYNAMIC, &dynamic_info, sizeof(struct pst_dynamic), 0, 0);
   pstat(PSTAT_VMINFO, &vminfo, sizeof(struct pst_vminfo), 0, 0);

   /* clear structs/arrays that hold previous data */
   memset(old_cpu_time, 0, sizeof(old_cpu_time));

   /*
    * Get disk information
    */

   /* calloc initial space for diskinfo array */
   if ((diskinfo = (struct pst_diskinfo *)
                        calloc(DKBURST, sizeof(struct pst_diskinfo))) == NULL)
   {
      perror("iostat");
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
      if ((diskinfo = (struct pst_diskinfo *)
                realloc((struct pst_diskinfo *) diskinfo,
                  (numents + DKBURST) * sizeof(struct pst_diskinfo))) == NULL)
      {
         perror("iostat");
         exit(1);
      }
      /* clear the extra entries */
      memset(diskinfo + numents, 0, DKBURST * sizeof(struct pst_diskinfo));

      idx = diskinfo[numents - 1].psd_idx + 1;
      /* idx += c; */
   }

   if ((old_diskinfo = (struct pst_diskinfo *)
                     calloc(numents, sizeof(struct pst_diskinfo))) == NULL)
   {
      perror("iostat");
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
   /*
    ***** MAIN LOOP *****
    */
   do
   {
   
      /*
       * We assume that the diskinfo array is small (<= 24).
       * If it is large a better algorithm than just looping through
       * the array each time should be used.
       * The entries are no longer separated by disk type since
       * now there is a single pool of logical units for all disk
       * types. This greatly simplifies the code.
       */

#define XDIFF(var, ovar) t = var; var -= ovar; ovar = t;

      /*
       * For these calculations, don't worry about logical unit.
       */
      for (i=0; i < numents; i++)
      {
         XDIFF(diskinfo[i].psd_dktime, old_diskinfo[i].psd_dktime);
         XDIFF(diskinfo[i].psd_dkseek, old_diskinfo[i].psd_dkseek);
         XDIFF(diskinfo[i].psd_dkwds, old_diskinfo[i].psd_dkwds);
      }

      XDIFF(vminfo.psv_tknin, old_tknin);
      XDIFF(vminfo.psv_tknout, old_tknout);
      cpticks = 0;
      for (i = 0; i < static_info.cpu_states; i++)
      {
         XDIFF(dynamic_info.psd_cpu_time[i], old_cpu_time[i]);
         cpticks += dynamic_info.psd_cpu_time[i];
      }
      if (cpticks == 0)
         cpticks = 1;

      etime = cpticks / (float) HZ;

      if (tflag)
      {
         usr = 100. * dynamic_info.psd_cpu_time[CP_USER] / cpticks;
         nice = 100. * dynamic_info.psd_cpu_time[CP_NICE] / cpticks;
         sys = dynamic_info.psd_cpu_time[CP_SYS];
#ifdef CP_BLOCK
         sys += dynamic_info.psd_cpu_time[CP_BLOCK];
#endif /* CP_BLOCK */
#ifdef CP_SWAIT
         sys += dynamic_info.psd_cpu_time[CP_SWAIT];
#endif /* CP_SWAIT */
#ifdef CP_INTR
         sys += dynamic_info.psd_cpu_time[CP_INTR];
#endif /* CP_INTR */
#ifdef CP_SSYS
         sys += dynamic_info.psd_cpu_time[CP_SSYS];
#endif /* CP_SSYS */
         sys = 100. * sys / cpticks;
         idle = dynamic_info.psd_cpu_time[CP_IDLE];
#ifdef CP_WAIT
         idle +=  dynamic_info.psd_cpu_time[CP_WAIT];
#endif /* CP_WAIT */
         idle = 100. * idle / cpticks;
         
         printf("\t\t   tty\t\t   cpu\n");
         printf("\t\t tin tout\t us  ni  sy  id\n");
         printf("\t\t%4.0f%5.0f\t",vminfo.psv_tknin / etime,
                                   vminfo.psv_tknout / etime);
         printf("%3.0f %3.0f %3.0f %3.0f\n",usr, nice, sys, idle);
      }

      if (tflag || (--lines <= 0))
      {
         if (tflag)
            lines = 1;
         else
            lines = 20;

         /* Output headers for disks */
         numhead = 0;
         for (i=0; i < numents; i++)
         {
	    j = remap[i];
            if (diskinfo[j].psd_dkmspw != 0.0)
            {
	       if (luonly)
                  printf("/dev/*dsk/c%1xd*s* ", LU(j));
	       else
                  printf("/dev/*dsk/c%04xd*s* ", SELBUS(j));
               numhead++;
            }
         }

         if (numhead)   /* In case no supported disks were found */
         {              /* Currently a hack for lack of 300 scsi support */
            printf("\n");
            for (i=0; i < numhead; i++)
               printf("bps  sps  msps  ");
            printf("\n");
         }
      }

      /*
       * Don't output stats if no supported disks were found. This is
       * currently a hack for lack of 300 scsi support and could be
       * removed provided all types of disks are supported.
       */
      if (!numhead && !tflag)
      {
         fprintf(stderr,
            "iostat: No disk information was found for your configuration.\n");
         exit(1);
      }
      else if (numhead)
      {
         /* Output stats for disks */
         for (i=0; i < numents; i++)
         {
	    j = remap[i];
            if (diskinfo[j].psd_dkmspw != 0.0)
               stats(diskinfo[j].psd_dktime, diskinfo[j].psd_dkseek,
                     diskinfo[j].psd_dkwds,  diskinfo[j].psd_dkmspw, etime);
         }
         printf("%s", tflag ? "\n\n" : "\n");

      } /* if (numhead): scsi hack */

      fflush(stdout);

      if (interval)
      {
         if (count || !countset)
         {
            sleep(interval);

            /* Get new statistics */
            pstat(PSTAT_DYNAMIC, &dynamic_info,
                      sizeof(struct pst_dynamic), 0, 0);
            pstat(PSTAT_VMINFO, &vminfo, sizeof(struct pst_vminfo), 0, 0);
            pstat(PSTAT_DISKINFO, diskinfo,
                      sizeof(struct pst_diskinfo), numents, 0);

            if (!countset)
               count = 1;
            else
               count--;
         }
      }
   } while (count);

   exit(0);
}


void
usage()
{
   fprintf(stderr, "usage: iostat [ -t ] [ interval [ count ] ]\n");
}


void
stats(dk_time, dk_seek, dk_wds, dk_mspw, etime)
long     dk_time,
         dk_seek,
         dk_wds;
float    dk_mspw;
double   etime;
{
   double atime, words, xtime, itime;

   atime = dk_time / (float) HZ;
   words = dk_wds*32.0; /* number of words transferred */
   xtime = dk_mspw*words;  /* transfer time */
   itime = atime - xtime;     /* time not transferring */
   if (xtime < 0)
      itime += xtime, xtime = 0;
   if (itime < 0)
      xtime += itime, itime = 0;
   printf("%3.0f ", words/512/etime);
   printf("%4.1f ", dk_seek/etime);
   printf("%5.1f  ", dk_seek ? itime*1000./dk_seek : 0.0);
}

