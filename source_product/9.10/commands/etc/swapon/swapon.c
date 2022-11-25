#include <stdio.h>
#include <mntent.h>        /* for getmntent() */
#include <sys/stat.h>      /* for stat() */
#include <string.h>        /* for perror */
#include <errno.h>         /* for errno, EALREADY def's */
#include <sys/types.h>     /* for statfs() */
#include <sys/vfs.h>       /* for statfs() */
#include <sys/fs.h>        /* for super block def */
#include <unistd.h>        /* for lseek() and sysconf() */
#include <fcntl.h>         /* for open() */
#include <sys/param.h>     /* for MAXBSIZE */
#include <sys/swap.h>      /* for NSWPRI */
#include <sys/sysmacros.h> /* for minor() */

#define ARG2	2
#define ARG5	5

#ifdef DEBUG1
#undef MNT_CHECKLIST
#define MNT_CHECKLIST "./checklist"
#endif /* DEBUG1 */

#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 70.4 $";
#endif

#define MAX_SWAP_PRI NSWPRI-1

/* forward declarations */
void usage();
void savecore_inuse_msg();

#ifdef DEBUG
int swapon();
#endif /* DEBUG */

/*
 * Parameters to swapon(). They are global so that bounds checking
 * and adjustments can be made without passing parameters.
 */

int   min = 0,                /* min # filesys blocks to allocate up front */
      limit = 0,              /* max # filesys blocks allowed for swapping */
      reserve = 0,            /* # filesys blocks reserved for filesys only*/
      priority = 1,           /* priority of swap resource */
      workstation_mode;       /* whether workstation (true => 300/700) or
				 multi-user (false => 800) mode */

short use_checklist = 0;      /* -a option flag: swapon checklist entries */

int   usage_given = 0;        /* flag to insure usage message only given once */

struct stat stat_buf;         /* for stat'ing device or directory arg */


/******************/
/****   main   ****/
/******************/
main(argc, argv)
   int   argc;
   char  **argv;
{
   short not_checklist_only = 0, /* option other than -a was specified */
         dev_swap = 0,           /* flag indicating swapon of block device */
         filesys_swap = 0,       /* flag indicating swapon of file system */
         swap_at_end = 0,        /* -e option flag: swapon at end of file */
                                 /* system - S300 only, always 0 on S800 */   
         unlock_savecore = 0,	 /* -u option flag: unlock devices being */
				 /*    used by savecore(1m)  */
         force_swap = 0;         /* -f option flag: force swapon of block */
                                 /*    device even if file system exists*/

   int   opt;                    /* option returned by getopt */
   extern int optind;            /* index into argv[] for getopt */
   extern char *optarg;          /* ptr to option argument for getopt */
   char  *getoptstring;          /* ptr to string used by getopt */

   int   errors = 0,             /* for exit() values */
         ret = 0;                /* for return values of functions */


   /*********************/
   /*   DETERMINE MODE  */
   /*********************/

#ifdef __hp9000s300
   workstation_mode = 1;        /* always true on Series 300 */
#else
#ifdef __hp9000s800
   workstation_mode = (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO); /* true on 700,
                                                                 false on 800 */
#endif /* __hp9000s800 */
#endif /* __hp9000s300 */

   /*********************/
   /*   PARSE OPTIONS   */
   /*********************/

   /* -e option only available on series 300 */
   if (workstation_mode) getoptstring = "aefm:l:r:p:";
   else getoptstring = "aufm:l:r:p:";
   while ((opt = getopt(argc, argv, getoptstring)) != EOF)
   {
      switch(opt)
      {
         case 'a':
                  use_checklist++;
                  break;
         case 'u':
		  unlock_savecore++;
		  break;
         case 'e':
		  if (workstation_mode) {
                     if (force_swap)
                     {
                        fprintf(stderr,
                             "swapon: -e option cannot be used with -f option\n");
                        usage();
                        exit(1);
                     }
                     dev_swap++;
                     swap_at_end++;
                     not_checklist_only++;
		  } else {
                     fprintf(stderr, "swapon: -e option not available on series 800\n");
                     usage();
                     exit(1);
		  }
                  break;
         case 'f':
                  if (workstation_mode && swap_at_end)
                  {
                     fprintf(stderr,
                          "swapon: -f option cannot be used with -e option\n");
                     usage();
                     exit(1);
                  }
                  dev_swap++;
                  force_swap++;
		  not_checklist_only++;
                  break;
         case 'm':
                  filesys_swap++;
                  min = atoi(optarg);
                  not_checklist_only++;
                  break;
         case 'l':
                  filesys_swap++;
                  limit = atoi(optarg);
                  not_checklist_only++;
                  break;
         case 'r':
                  filesys_swap++;
                  reserve = atoi(optarg);
                  not_checklist_only++;
                  break;
         case 'p':
                  priority = atoi(optarg);
                  not_checklist_only++;
                  break;
         case '?':
                  usage();
                  exit(1);
      }  
   }

#ifdef DEBUG1
   printf("%s%s%s%s%s%s\n", use_checklist ? "use_checklist\n" : "",
                        not_checklist_only ? "not_checklist_only\n" : "",
                        swap_at_end ? "swap_at_end\n" : "",
                        force_swap ? "force_swap\n" : "",
                        dev_swap ? "dev_swap\n" : "",
                        filesys_swap ? "filesys_swap\n" : "");
   if (min) printf("min = %d\n",min);
   if (limit) printf("limit = %d\n",limit);
   if (reserve) printf("reserve = %d\n",reserve);
   if (priority) printf("priority = %d\n",priority);
   printf("\n");
#endif /* DEBUG1 */

   /***************************************************************/
   /* We now have 4 possible cases to consider:                   */
   /* 1 - swapon using /etc/checklist (use_checklist flag set)    */
   /* 2 - swapon to a device (dev_swap flag set)                  */
   /* 3 - swapon to file system (filesys_swap flag set)           */
   /* 4 - other: above three flags not set, could be device swap  */
   /*            if only block device(s) given, file system swap  */
   /*            if only directories given or just plain garbage  */
   /*                                                             */
   /* Each case will exit() so that we don't have 3 added levels  */
   /* of indentation                                              */
   /***************************************************************/

   /******************************************************************/
   /* CASE 1  --  swapon all swap resources listed in /etc/checklist */
   /******************************************************************/
   if (use_checklist)
   {
      if (not_checklist_only || optind < argc)
      {
         fprintf(stderr,"swapon: illegal use of -a option\n"),
         usage();
         exit(1);
      }
      ret = process_checklist(unlock_savecore);
      exit(ret);
   }

   /******************************************************************/
   /* CASE 2  --  swapon a device (dev_swap flag set)                */
   /******************************************************************/
    if (dev_swap)
    {
      /* if file system swap also specified exit with usage msg */
      if (filesys_swap)
      {
         fprintf(stderr,"swapon: illegal usage - both device swap and file system swap were indicated\n");
         usage();
         exit(1);
      }

      /* if no device specified exit with usage msg */
      if (optind == argc)
      {
         fprintf(stderr,"swapon: no device specified\n");
         usage();
         exit(1);
      }

      /* if priority out of range exit with error message */
      if (priority < 0 || priority > MAX_SWAP_PRI)
      {
         fprintf(stderr,
               "swapon: priority (%d) must be >= 0 and <= %d\n", priority,MAX_SWAP_PRI);
         exit(1);
      }

      /* otherwise loop for each device specified */
      while (optind < argc)
      {
         /* if cannot stat, perror and continue */
         if (stat(argv[optind], &stat_buf) < 0)
         {
            fprintf(stderr,"swapon: ");
            perror(argv[optind]);
            errors++;
         }
         else /* could stat device */
         {
            /* if not a block device, output error msg and continue */
            if (! S_ISBLK(stat_buf.st_mode))
            {
               fprintf(stderr,"swapon: %s: not a block device\n", argv[optind]);
               errors++;
            }
            else  /* is a block device so try swapon */
               errors += swapon_dev(argv[optind], priority, swap_at_end,
				             force_swap, unlock_savecore);
         }
         optind++;
      }
   exit(errors);
   }


   /******************************************************************/
   /* CASE 3 - swapon to file system (filesys_swap flag set)         */
   /******************************************************************/
    if (filesys_swap)
    {
      /* if no directory specified exit with usage msg */
      if (optind == argc)
      {
         fprintf(stderr,"swapon: no directory specified\n");
         usage();
         exit(1);
      }

      /* loop for each directory specified */
      while (optind < argc)
      {
         /* if cannot stat, perror and continue */
         if (stat(argv[optind], &stat_buf) < 0)
         {
            fprintf(stderr,"swapon: ");
            perror(argv[optind]);
            errors++;
         }
         else /* else could stat */
         {
            /* if not a directory, output error msg and continue */
            if (! S_ISDIR(stat_buf.st_mode))
            {
               fprintf(stderr,"swapon: %s: not a directory\n",
                              argv[optind]);
               errors++;
            }
            else /* is a directory */
            {
               /* check params against given file system */
               ret = check_swapfs_params(argv[optind]);
               if (ret == -2)       /* fatal error in params */
               {
                  exit(1);
               }
               else if (ret == -1)  /* non fatal error in params */
               {
                  usage();
                  errors++;
               }
               else                 /* params OK */
                  errors += do_swapon(argv[optind], priority, min, limit,
                                       reserve, 0, ARG5);
            }
         }
         optind++;
      }
   exit(errors);
   }

   /******************************************************************/
   /* CASE 4 - other: use_checklist, dev_swap and filesys_swap flags */
   /*                 not set. Could be device swap if only block    */
   /*                 device(s) given, file system swap if only      */
   /*                 directories given or just plain garbage        */
   /******************************************************************/

   /* if no device or directory specified exit with usage msg */
   if (optind == argc)
   {
      fprintf(stderr,"swapon: No device or directory specified.\n");
      usage();
      exit(1);
   }  /* else if cannot stat device or directory */
   else if (stat(argv[optind], &stat_buf) < 0)
   {
      fprintf(stderr,"swapon: ");
      perror(argv[optind]);
      usage();
      exit(1);
   }  /* else if directory is specified... */
   else if (S_ISDIR(stat_buf.st_mode))
   {
      /*
       * There are 2 legal possibilities here: all 4 optional file system
       * parameters may have been specified or none of them may have been
       * specified. If the 4 optional params were specified record them.
       */
      if ((optind + 5) == argc)
      {
         min = atoi(argv[optind+1]);
         limit = atoi(argv[optind+2]);
         reserve = atoi(argv[optind+3]);
         priority = atoi(argv[optind+4]);

      }  /*else if illegal # of params specified - error */
      else if ((optind + 1) < argc)
      {
         fprintf(stderr,
            "swapon: Illegal number of parameters specified for swapping to file system\n");
         fprintf(stderr,
            "        (to directory %s).\n", argv[optind]);
         usage();
         exit(1);
      }

      /* check parameters and call swapon */
      ret = check_swapfs_params(argv[optind]);
      if (ret < 0)
         exit(1);
      else
         errors += do_swapon(argv[optind], priority, min, limit, reserve, 
			                                     0, ARG5);
   }
   else if (! S_ISBLK(stat_buf.st_mode))
   /* if device not specified either, exit with usage messgae */
   {
      fprintf(stderr, "swapon: %s: Not a directory or block device.\n",
            argv[optind]);
      /*
       * if additional arguments given, output meesage indicating that
       * we can't continue - e.g. if 2 devices were to be enabled and
       * the first is not a block device just exit with usage message
       */
      if (optind < (argc + 1))
      {
         fprintf(stderr,
               "        Could not determine whether device or file system\n");
         fprintf(stderr,
               "        was to be enabled: cannot continue.\n");
         usage();
      }
      exit(1);
   }
   else
   {
   /*
    * device was specified so check priority, try swapon then
    * loop for remaining devices
    */

      /* if priority out of range exit with error message */
      if (priority < 0 || priority > MAX_SWAP_PRI)
      {
         fprintf(stderr,
               "swapon: priority (%d) must be >= 0 and <= %d\n", priority,MAX_SWAP_PRI);
         errors++;
      }
      else
         while (optind < argc)
         {
            errors += swapon_dev(argv[optind], priority, swap_at_end,
				           force_swap, unlock_savecore);
            optind++;
         }
   }
   exit(errors);
}


/************************************************************************/
/* usage() - output usage message only if it has not been given already */
/************************************************************************/
void
usage()
{
   if (! usage_given)
   {
      if (workstation_mode)
	  fprintf(stderr,"\nusage: /etc/swapon -a\n");
      else
	  fprintf(stderr,"\nusage: /etc/swapon -a [-u]\n");
      if (workstation_mode)
         fprintf(stderr,  "       /etc/swapon [-p priority] [-e | -f] device ...\n");
      else
         fprintf(stderr,  "       /etc/swapon [-p priority] [-f] [-u] device ...\n");
      fprintf(stderr,  "       /etc/swapon [-m min] [-l limit] [-r reserve] [-p priority] directory\n");
      fprintf(stderr,  "       /etc/swapon directory [min limit reserve priority]\n\n");
   }
   usage_given++;
}


/************************************************************************/
/* process_checklist() - reads the /etc/checklist file and enables all  */
/* "swap" and "swapfs" entries. Returns 0 on success, >0 on error       */
/************************************************************************/
process_checklist(unlock)
  short unlock;
{
   FILE *chklst;
   struct mntent mnt, *mntp;
   int errors = 0,
       ret = 0,
       found = 0,          /* if option is found */
       swap_at_end = 0;    /* swap at end of file system - S300 only */
                           /* always 0 on S800 */  

   if ((chklst = setmntent(MNT_CHECKLIST, "r")) == 0)
   {
      perror(MNT_CHECKLIST);
      exit(1);
   }

   mntp = &mnt;

   while ((mntp = getmntent(chklst)) != 0)
   {

      if (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0)
      {  /* Swap device */

         /* get params - if not specified, set to default values */
         swap_at_end = ((hasmntopt(mntp, MNTOPT_END) == 0) ? 0 : 1);
         if (! workstation_mode && swap_at_end)
         {
            fprintf(stderr,
               "swapon: The 'end' option cannot be used on the series 800\n");
            fprintf(stderr,
               "        Correct the entry for '%s' in the file %s\n",
                           mntp->mnt_fsname, MNT_CHECKLIST);
            errors++;
            continue;
         }
         ret = nopt(mntp, MNTOPT_PRI, &found);
         priority = found ? ret : 1;

         /* if priority out of range output error message */
         if (priority < 0 || priority > MAX_SWAP_PRI)
         {
            fprintf(stderr,
               "swapon: priority (%d) must be >= 0 and <= %d\n", priority,MAX_SWAP_PRI);
            errors++;
            continue;
         }

	 /* stat device file for later error handling */
	 if (stat(mntp->mnt_fsname, &stat_buf) < 0) {
	    fprintf(stderr,"swapon: ");
	    perror(mntp->mnt_fsname);
	    errors++;
	    continue;
	 }

	 /* check if the device is in use by savecore(1m) */
	 if (savecore_dev (stat_buf.st_rdev))
	 {
	     if (unlock)
		 (void) override_savecore_lock (stat_buf.st_rdev);
	     else
	     {
		 savecore_inuse_msg (mntp->mnt_fsname);
		 errors++;
		 continue;
	     }
	 }
	 
	 printf("\nAdding %s as swap device\n", mntp->mnt_fsname);

	 /* Note - can't force swap using checklist so force_swap = 0 */
	 /* Note - savecore unlocking already done so last arg = 0 */
	 errors += swapon_dev(mntp->mnt_fsname, priority, swap_at_end, 0, 0);
	 
      }
      else if (strcmp(mntp->mnt_type, MNTTYPE_SWAPFS) == 0)
      {  /* Swap filesystem */

         /* get params - if not specified, set to default values */
         ret = nopt(mntp, MNTOPT_MIN, &found);
         min = found ? ret : 0;
         ret = nopt(mntp, MNTOPT_LIM, &found);
         limit = found ? ret : 0;
         ret = nopt(mntp, MNTOPT_RES, &found);
         reserve = found ? ret : 0;
         ret = nopt(mntp, MNTOPT_PRI, &found);
         priority = found ? ret : 1;

         if (check_swapfs_params(mntp->mnt_dir) == 0)
         {
	    /* stat directory for later error handling */
	    if (stat(mntp->mnt_dir, &stat_buf) < 0) {
	       fprintf(stderr,"swapon: ");
	       perror(mntp->mnt_dir);
	       errors++;
	       continue;
	    }

            printf("\nAdding %s as swap filesystem,\n        minimum=%d, limit=%d, reserve=%d and priority=%d\n",
                      mntp->mnt_dir, min, limit, reserve, priority);
            errors += do_swapon(mntp->mnt_dir, priority, min, limit,
                                    reserve, 0, ARG5);
         }
      }
   }
   endmntent(chklst);
   exit(errors);
}


/************************************************************************/
/* nopt() - Return the value of a numeric option of the form foo=x. If  */
/* option is found set found to 1, if option is not found or is         */
/* malformed set found to 0.                                            */
/************************************************************************/
nopt(mnt, opt, found)
   struct mntent *mnt;
   char *opt;
   int  *found;
{
   int val = -1;
   char *equal;
   char *str;

   *found = 0;
   if (str = hasmntopt(mnt, opt))
      if (equal = strchr(str, '='))
      {
         val = number(&equal[1]);
         *found = 1;
      }
      else
         fprintf(stderr, "swapon: bad numeric option '%s'\n", str);
   return(val);
}


/************************************************************************/
/* number() - Return integer conversion of string where string is a     */
/* number in octal, decimal or hexidecimal. This was added in release   */
/* 7.0 and is left here for backward compatability. Return -1 on failure*/
/************************************************************************/
number(arg)
   register char    *arg;
{
   int   base = 10;           /* selected base        */
   long  num  =  0;           /* function result      */
   int   digit;               /* current digit        */
   int   sign;                /* Sign +/-    */
   char  *arg_str = arg;      /* In case of errors */

   if ((arg == 0) || (*arg == 0))
      return(0);

   if (*arg == '-')
   {
      sign = -1;
      arg++;
   }
   else
   {
      sign = 1;
   }

   if (*arg == '0')                        /* determine base */
      if ((*(++arg) != 'x') && (*arg != 'X'))
         base = 8;
      else
      {
         base = 16;
         ++arg;
      }

   while ((digit = *arg++) && (digit != ','))
   {
      if (base == 16)
      {               /* convert hex a-f or A-F */
         if ((digit >= 'a') && (digit <= 'f'))
            digit += '0' + 10 - 'a';
         else
            if ((digit >= 'A') && (digit <= 'F'))
               digit += '0' + 10 - 'A';
      }
      digit -= '0';

      if ((digit < 0) || (digit >= base))
      {   /* out of range */
         fprintf(stderr, "Invalid numeric argument (%s) in base %d\n",
                           arg_str, base);
         return(-1);
      }
      num = num*base + digit;
   }
   return(num * sign);
}

/************************************************************************/
/* check_swapfs_params() checks to see of the parameters for file       */
/* system swap are valid. Returns 0 if OK, -1 on non-fatal error, -2 on */
/* fatal error. A fatal error is one whereby all calls to swapon(2)     */
/* with given params would fail. A non-fatal error is one whereby the   */
/* the call to swapon(2) would fail for the given file system but might */
/* succeed for another file system.                                     */
/************************************************************************/
check_swapfs_params(fs)
char *fs;
{
   struct statfs statfs_buf;
   int fatal = 0,
       non_fatal = 0;

   if (statfs(fs, &statfs_buf) == -1)
   {
      fprintf(stderr, "swapon: ");
      perror(fs);
      return(-2);
   }

   if (priority < 0 || priority > MAX_SWAP_PRI)
   {
      fprintf(stderr,
               "swapon: priority (%d) must be >= 0 and <= %d\n", priority,MAX_SWAP_PRI);
      fatal++;
   }

   if (reserve < 0)
   {
      fprintf(stderr, "swapon: %s reserve (%d) must be >= 0\n", fs, reserve);
      fatal++;
   }
   else if (reserve > statfs_buf.f_blocks)
   {
      fprintf(stderr,
               "swapon: %s reserve (%d) must be <= max blocks in filesystem (%d)\n",
               fs, reserve, statfs_buf.f_blocks);
      non_fatal++;
   }

   if (limit < 0)
   {
      fprintf(stderr, "swapon: %s limit (%d) must be >= 0\n", fs, limit);
      fatal++;
   }
   else if (limit > statfs_buf.f_blocks)
   {
      fprintf(stderr,
               "swapon: %s limit (%d) must be <= max blocks in filesystem (%d)\n",
               fs, limit,  statfs_buf.f_blocks);
      non_fatal++;
   }

   if (min < 0)
   {
      fprintf(stderr, "swapon: %s minimum blocks (%d) must be >= 0\n", fs, min);
      fatal++;
   }
   else if ((limit > 0) && (min > limit))
   {
      fprintf(stderr, "swapon: %s minimum blocks (%d) must be <= limit (%d)\n",
               fs, min, limit);
      fatal++;
   }
   else if (min > statfs_buf.f_bavail)
   {
      fprintf(stderr, "swapon: %s minimum blocks (%d) must be <= free blocks in filesystem (%d)\n",
               fs, min, statfs_buf.f_bavail);
      non_fatal++;
   }

   if (fatal)
      return(-2);
   else if (non_fatal)
      return(-1);
   else
      return(0);
}

/************************************************************************/
/* do_swapon() - call swapon() with given parameters. On error, output  */
/* suitable error message. Return 0 on success, 1 on failure.           */
/************************************************************************/
do_swapon(name, priority, min, limit, reserve, swap_at_end, pri_arg)
   char  *name;
   int   priority, min, limit, reserve, swap_at_end, pri_arg;
{
   int ret;

   if (pri_arg == ARG2)
       ret = swapon(name, priority, 0, 0, 0);
   else
       ret = swapon(name, min, limit, reserve, priority);

   if (ret == -1)
   {
      ret = 1;
      /* output better messages than perror would provide */
      if ( errno == EALREADY)
         fprintf(stderr,"swapon: %s already enabled for swap\n", name);
      else if (((errno == ENOSPC) || (errno == EEXIST) || (errno == ENOSYS)) && (S_ISBLK(stat_buf.st_mode)))
	 fprintf(stderr,"swapon: incompatible swap configuration between %s and address %lx\n",name,minor(stat_buf.st_rdev));
      else
      {
         fprintf(stderr,"swapon: ");
         perror(name);
      }
   }
   else
      ret = 0;

   return(ret);
}


/************************************************************************/
/* swapon_dev() - calls do_swapon() with appropriate parameters only if */
/* it is legal to swap to given device. An example of not being legal   */
/* would be if the device contained a file system and both force_swap   */
/* and swap_at_end were false. Returns 0 on success, 1 on failure.      */
/* Will output sutiable error messages for failures.                    */
/************************************************************************/
swapon_dev(dev, priority, swap_at_end, force_swap, unlock)
   char *dev;
   int   priority;
   short swap_at_end, force_swap, unlock;
{
    struct stat st;
    int   ret;

   /* check for file system on device - return on error */
   if ((ret = filesys_on_dev(dev)) < 0)
      return(1);

   /* if file system on device */
   if (ret)
   {
      /* if we can't swapon a device with a file sytem - return with error */
      /* if from command line and print warning if from /etc/checklist.    */
      if ( ! (force_swap || (swap_at_end && workstation_mode)))
      {
         fprintf(stderr, "swapon: Device %s contains a file system\n", dev);
	 if (use_checklist) {
		if (workstation_mode) {
         	    fprintf(stderr, "        Swap to end of file system assumed\n");
         	    fprintf(stderr, "        Add 'end' to options field in %s to stop message\n",MNT_CHECKLIST);
		} else {
         	    fprintf(stderr, "        Cannot swap to this device\n");
		    return(1);
		}
	 } else {
		if (workstation_mode)
         	    fprintf(stderr, "        Cannot swap to this device without -e option\n");
		else
         	    fprintf(stderr, "        Cannot swap to this device\n");
         	return(2);
	 }
      }
    
      /* Otherwise we can swap to the device -- */
      /* the following insures that a subsequent swapon */
      /* to this device will succeed without using -f */
      if (force_swap && ! destroy_filesys(dev))
         fprintf(stderr,"swapon: warning: could not overwrite file system on device %s\n", dev);

   }
   /* else filesystem not found on device */
   else if (swap_at_end && workstation_mode)
   /* if trying to swap to end of non-existent file system */
   {
      fprintf(stderr, "swapon: File system not found on device %s\n", dev);
      fprintf(stderr, "        Cannot swap to end of file system\n");
      return(1);
   }

    if (stat(dev, &st) < 0)
    {
	fprintf (stderr, "swapon: ");
	fflush (stderr);
	perror (dev);
	return (1);
    }
    
    /* check if the device is in use by savecore(1m) */
    if (savecore_dev (st.st_rdev))
    {
	if (unlock)
	    (void) override_savecore_lock (st.st_rdev);
	else
	{
	    savecore_inuse_msg (dev);
	    return(1);
	}
    }

   /* no errors caused us to skip this device so swapon */
   return(do_swapon(dev, priority, 0, 0, 0, swap_at_end, ARG2));
}


/************************************************************************/
/* filesys_on_dev() - returns 1 if a file system is found on the device,*/
/* 0 if not and -1 on error. Does this by checking for magic number in  */
/* second block on device.                                              */
/************************************************************************/
filesys_on_dev(dev)
   char *dev;
{
   int fd,
       errors = 0;
   long magic;
   char block[SBSIZE];
   struct fs *super;
   
   if ((fd = open(dev, O_RDWR)) < 0)
   {
      fprintf(stderr, "swapon: ");
      perror(dev);
      errors++;
   }
   else
   {  
      super = (struct fs *)block;
   
      if (lseek(fd, (long)dbtob(SBLOCK), SEEK_SET) == -1)
      {
         fprintf(stderr, "swapon: ");
         perror(dev);
         errors++;
      }
      else  
         if (read(fd, super, SBSIZE) != SBSIZE)
         {
            fprintf(stderr, "swapon: ");
            perror(dev);
            errors++;
         }
   }

   close(fd);

   if (errors)
      return(-1);
   else
   {
      magic = super->fs_magic;
      if (magic == FS_MAGIC || magic == FS_MAGIC_LFN || magic == FD_FSMAGIC)
         return(1);
      else
         return(0);
   }
}


/************************************************************************/
/* destroy_filesys() - destroys a file system on the device by          */
/* overwriting the first 2 blocks with 0's. This insures that           */
/* subsequent calls to filesys_on_dev() return 0. Returns 1 on success, */
/* 0 on failure.                                                        */
/************************************************************************/
destroy_filesys(dev)
   char  *dev;
{
   int fd,
       bufsize = SBSIZE > BBSIZE ? SBSIZE : BBSIZE,
       errors = 0;
   char *buf;

   if ((buf = (char *) calloc(bufsize, sizeof(char))) == NULL)
      return(0);
   
   if ((fd = open(dev, O_WRONLY)) < 0)
      errors++;
   else
      if (lseek(fd, (long)dbtob(BBLOCK), SEEK_SET) == -1)
         errors++;
      else
      {
         if (write(fd, buf, BBSIZE) != BBSIZE)
            errors++;
         else
            if (lseek(fd, (long)dbtob(SBLOCK), SEEK_SET) == -1)
               errors++;
            else
               if (write(fd, buf, SBSIZE) != SBSIZE)
                  errors++;
      }

   close(fd);
   free(buf);
   if (errors)
      return(0);
   else
      return(1);
}

/* ADDITIONS FOR SAVECORE INTERLOCKING				*/

/* NOTE:
 *	The following global data structure is used to contain the 
 *	list of devices which are being used by savecore. The contents
 *	of this table should only be accessed through the interfaces 
 * 	defined below
 */

#define	SC_LOCKFILE	"/etc/savecore.LCK"
#define DT_CHUNK	128
#define HEX		16

static struct dtab	/* table of devices currently in use by savecore */
{
    dev_t	*tab;
    int		ents;
    int		next;
} sc_devs;


/************************************************************************/
/* write_savecore_devs() - rewrites the savecore "lockfile" if any	*/
/* of the entries have been modified (which means we are overriding the	*/
/* savecore lock). This is the signal to savecore that swapping has	*/
/* been enabled on one of the dump devices. This function is registered */
/* with and called from atexit().					*/
/************************************************************************/
void write_savecore_devs ()
{
    int		i;
    FILE	*fp;

    /* Rewrite the contents of the lockfile skipping over zero'd out
     * entries.
     */
    if ((fp = fopen (SC_LOCKFILE, "w")) == NULL)
    {
	fprintf (stderr, "swapon: ");
	fflush (stderr);
	perror (SC_LOCKFILE);
	return;
    }
    
    for (i=0; i < sc_devs.next; i++)
    {
	if (sc_devs.tab[i] != 0)
	    (void) fprintf (fp, "%x\n", sc_devs.tab[i]);
    }
    
    (void) fclose (fp);
    return;
}


/************************************************************************/
/* read_savecore_devs() - Read the savecore(1m) lockfile and build an	*/
/* internal list of dev_t's which are currently being used by savecore.	*/
/* Return 0 for success or -1 if an error occurs.			*/
/************************************************************************/
read_savecore_devs()
{
    char	line[BUFSIZ];
    int		i;
    dev_t	dev;
    dev_t	*ptr;
    size_t	sz;
    FILE	*fp;


    /* If the lockfile does not exist, then we're done */

    if ((fp = fopen(SC_LOCKFILE, "r")) == NULL)
    {
	if (errno == ENOENT)
	    return 0;
        else
	{
	    fprintf (stderr, "swapon: ");
	    fflush (stderr);
	    perror (SC_LOCKFILE);
	    return -1;
	}
    }

    /*
     * The savecore lockfile contains a list dev_t's for devices from
     * which savecore is currently saving a dump. The list consists of
     * one entry per line in hex (ascii representation).
     */
    while (fgets (line, sizeof(line), fp) != NULL)
    {
	dev = strtol (line, (char **)NULL, HEX);

	/* enlarge the table if out of space */
	if (sc_devs.next == sc_devs.ents)
	{
	    sz = (sc_devs.ents + DT_CHUNK) * sizeof(dev_t);
	    if ((ptr = (dev_t *) realloc (sc_devs.tab, sz)) != NULL)
	    {
		sc_devs.tab   = ptr;
		sc_devs.ents += DT_CHUNK;
	    }	

	    else
	    {
		perror ("swapon");
		(void) fclose(fp);
		return -1;
	    }
	
	}
	
	/* ignore duplicates */
	for (i=0; i < sc_devs.next; i++)
	    if (dev == sc_devs.tab[i])
		break;
	
	if (i == sc_devs.next)
	    sc_devs.tab[sc_devs.next++] = dev;
    }
    (void) fclose(fp);
    return 0;
}


/************************************************************************/
/* override_savecore_lock() - Zero out the given devices entry in the	*/
/* table of savecore devices. Install an exit routine which will 	*/
/* rewrite the savecore lock file.					*/
/************************************************************************/
override_savecore_lock (dev)
  dev_t	dev;
{
    int i;
    static int first_time = 0;
    
    if (first_time == 0)
    {
	atexit (write_savecore_devs);
	first_time++;
    }
    
    for (i=0; i < sc_devs.next; i++)
	if (dev == sc_devs.tab[i])
	    break;
    
    sc_devs.tab[i] = 0;
}

/************************************************************************/
/* savecore_dev() - Returns true if the given device is in use by 	*/
/* savecore and false if it is not.					*/
/************************************************************************/
savecore_dev (dev)
  dev_t	dev;
{
    int i;
    static int first_time = 0;
    
    if (first_time == 0)
    {
	(void) read_savecore_devs();
	first_time++;
    }
    
    for (i=0; i < sc_devs.next; i++)
	if (dev == sc_devs.tab[i])
	    break;
    
    return (i != sc_devs.next);
}

void savecore_inuse_msg (dev)
  char *dev;
{
    fprintf (stderr, "swapon: Device %s is currently in use by savecore(1m).\n", dev);
    fprintf (stderr, "        Use the -u option to forcibly unlock and enable swapping or\n");
    fprintf (stderr, "        wait for savecore to complete and try again.\n");
}


#ifdef DEBUG
swapon(name, min, lim, res, pri)
char  *name;
int   min, lim, res, pri;
{
   printf("swapon would be called as:\n");
   printf("swapon(\"%s\", %d, %d, %d, %d)\n", name, min, lim, res, pri);
   return(0);
}
#endif /* DEBUG */
