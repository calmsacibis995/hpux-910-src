/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/initsym.c,v $
 * $Revision: 1.3.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:23:05 $
 */

/*
 * Original version based on: 
 * Revision 63.1  88/05/26  13:37:21  13:37:21  markm
 */

/*
 * Copyright Third Eye Software, 1983.
 * Copyright Hewlett Packard Company 1985.
 *
 * This module is part of the CDB/XDB symbolic debugger.  It is available
 * to Hewlett-Packard Company under an explicit source and binary license
 * agreement.  DO NOT COPY IT WITHOUT PERMISSION FROM AN APPROPRIATE HP
 * SOURCE ADMINISTRATOR.
 */

/*
 * These routines initialize the symbol file (object file), to the extent
 * of checking it out (but not actually reading the symbol table, that's done
 * elsewhere).
 */

#ifdef S200
#ifdef SYSVHDRS
#include <sys/types.h>
#endif
#include <sys/param.h>
#ifdef S200BSD
#include <sys/vmparam.h>
#define USRSTART 0x0
#else	/* not S200BSD */
#include <sys/page.h>
#endif	/* not S200BSD */
#else 	/* not S200 */
#include <sys/types.h>
#endif

#include <sys/stat.h>

#if (!FOCUS && !SPECTRUM)
#include <core.h>
#endif

#define SYS_USHORT 1			/* see basic.h */
#if (S200BSD || SYSVHDRS)
#define SYS_UINT 1                      /* Required for Sys V header files  */
#endif  /* S200BSD */

#ifdef SPECTRUM
#define SYS_UINT 1
#endif

#include "cdb.h"

#ifdef SPECTRUM
#include "somaccess.h"
#ifdef HPE
#include "lst.h"
#include "magic.h"
#include "errnohpe.h"
extern  void    xdbhperr();
int     _mpe_errno;
int     _mpe_intrinsic;
#endif
#endif

long	vmtimeSym;	/* last modification time of vsbSymfile */
int	vmagicSym;	/* magic number of the Sym file		*/
MAPR	vmapText;	/* map information for some versions	*/

#ifdef HPE
ADRT	vipreDp;	/* presumed dp from compiler or linker  */
long	visomloc;	/* start of som header (0 if not lst)   */
struct init_pointer_record init_ptr_rec;
#endif

#ifdef SPECTRUM
struct som_exec_auxhdr vauxhdr; /* aux. header needed here and initcore */
typedef	struct	header EXECR;	/* a.out header record */
typedef	struct	header *pEXECR;
FLAGT vfprntipdifd = 0;
pSUR vrgUnwind;
uint vcbUnwindFirst;
uint viUnwindMax;
pSTUBSUR vrgStubUnwind;
uint vcbStubUnwindFirst;
uint viStubUnwindMax;
FLAGT vfUnwind;
#else
typedef	struct	exec EXECR;	/* a.out header record */
typedef	struct	exec *pEXECR;
#endif

FLAGT vfNoDebugInfo = false;

#ifdef XDB && SPECTRUM
ADRT vTextAdrMin;     /* for assembly mode screen I/O (textmap.b1) */
ADRT vTextAdrMax;     /* for assembly mode screen I/O (textmap.e1) */
long vTextFileStart;  /* for assembly mode screen I/O (textmap.f1) */
#endif

#define	cbEXECR	((long)(sizeof (EXECR)))


/***********************************************************************
 * A G E   F   F N
 *
 * Return the age (last modify time) of a file by file number.
 */

long AgeFFn (fn)
    int		fn;		/* file to age */
{
    struct stat statbuf;	/* for fstat(2) return */

#ifdef INSTR
    vfStatProc[146]++;
#endif

    fstat  (fn, &statbuf);
    return (statbuf.st_mtime);

} /* AgeFFn */


/***********************************************************************
 * I N I T   S Y M F I L E
 *
 * Open, check out, and set values relating to an executable object
 * file containing symbol information:
 */

void InitSymfile (sbFile)
    char	*sbFile;	/* name of file to init */
{
    long	cbData;		/* size of data area	*/
    long	cbText;		/* size of text area	*/
    EXECR	execSym;	/* start of object file */

#if (FOCUS || S200)
   long		residue = 0;	/* bytes of partial records in sections */
#endif
#ifdef S200BSD
   long         NBPG;           /* bytes per page */
   long         PGSHIFT;        /* log2 (NBPG) */
#endif

#ifdef SPECTRUM
   struct PXDB_header pxdbh;
   status	stat;
   long_ptr	ofile_ptr;
   struct subspace_dictionary_record subsp;
   int 		i,j,waitstat;

#ifdef HPE
   int          nb, pid, dir_size, som_count;
   char         infostring[BUFSIZ];
   struct som_entry *som_dir;
   struct lst_header lst_hdr;
   MAGIC file_id;
#endif

#define loadable_defined   0
#define unloadable_defined 3
#define acb                0x2c

#endif

#ifdef INSTR
    vfStatProc[147]++;
#endif

/*
 * OPEN SYMFILE and check magic number:
 */

    if ((vfnSym = open (sbFile, O_RDONLY)) < 0)		/* can't open */
    {
#ifdef HPE
        int hpe_status;

        hpe_status = ((_mpe_errno << 16) & (~0xffff));
        hpe_status |= 0x8f;
        xdbhperr (hpe_status);
#else
        perror (sbFile);
#endif
	exit (1);
    }

    vmtimeSym	= AgeFFn (vfnSym);
    vmapText.fn = vfnSym;

#ifdef HPE
    nb = sizeof(file_id);

    if (read (vfnSym, &file_id, nb) != nb) {
	sprintf (vsbMsgBuf, (nl_msg(524, "Can't read header of %s (IE524)")), 
                 sbFile);
	Panic (vsbMsgBuf);
    }

    if (FBadMagic (file_id)) {
	sprintf (vsbMsgBuf, (nl_msg(375, "Bad magic number %d (UE414)")),
		 execSym.a_magic);
	Panic (vsbMsgBuf);
    }

    vmagicSym = file_id.file_type;

    lseek(vfnSym, 0, 0);				/* rewind */

    visomloc    = 0;					/* unless EXECLIB */

    if (vmagicSym == EXECLIBMAGIC) {			/* lst header */
	nb = sizeof(lst_hdr);
	if (read(vfnSym, &lst_hdr, nb) != nb) {
	   sprintf (vsbMsgBuf, (nl_msg(524, "Can't read header of %s (IE524)")),
                    sbFile);
	   Panic (vsbMsgBuf);
	}
	som_count = lst_hdr.module_count;
	if (som_count != 1) {
	   sprintf (vsbMsgBuf, (nl_msg(525, "More than one som in %s (IE525)")),
                    sbFile);
	   Panic (vsbMsgBuf);				/* "Multiple soms" */
        }

	dir_size = sizeof(struct som_entry) * som_count;
	som_dir = (struct som_entry *) Malloc(dir_size, true);
	lseek(vfnSym, (long) lst_hdr.dir_loc, 0);
	read(vfnSym, som_dir, dir_size);
        
	visomloc = som_dir[0].location;			/* only 1 som */
	lseek(vfnSym, visomloc, 0);			/* find som header */
    }		

    nb = sizeof(EXECR);
    if (read(vfnSym, &execSym, nb) != nb) {		/* "Bad header" */
	sprintf (vsbMsgBuf, (nl_msg(524, "Can't read header of %s (IE524)")),
                 sbFile);
	Panic (vsbMsgBuf);
    }

#else /* HP-UX */

    if (read (vfnSym, &execSym, cbEXECR) != cbEXECR)
    {
	sprintf (vsbMsgBuf, (nl_msg(524, "Can't read header of %s (IE524)")), sbFile);
	Panic (vsbMsgBuf);
    }

    if (FBadMagic (execSym))
    {
#ifdef S200
	sprintf (vsbMsgBuf, (nl_msg(375, "Bad magic number %X (UE414)")),
		 (execSym.a_magic.system_id << 16) | execSym.a_magic.file_type);
#else
	sprintf (vsbMsgBuf, (nl_msg(375, "Bad magic number %X (UE414)")),
		 execSym.a_magic);
#endif
	Panic (vsbMsgBuf);
    }

#if (S200 || FOCUS)
    vmagicSym = execSym.a_magic.file_type;
#else
    vmagicSym = execSym.a_magic;
#endif

#ifdef FOCUS
    if (vmagicSym == SHARE_MAGIC)
    {
	printf (
    (nl_msg(138, "WARNING:  Code is SHARED; you can't set breakpoints so you can't run.\n")));
    }

#endif

#endif /* HP-UX */
/*
 * There is no problem setting breakpoints in shared text code in S200.
 * The user cannot write to it but ptrace can.  The text portion is then
 * marked busy and another user is not allowed to run it.
 */

/*
 * SET VALUES based on object file header:
 */


#ifdef S200	/* should do some of the nice checks in FOCUS, below */
    vcbLstFirst = LESYM_OFFSET (execSym);		/* start of LST  */
    vilstMax	= execSym.a_lesyms;			/* size	 of LST  */
    vcbSymFirst = DNTT_OFFSET (execSym);		/* start of DNTT */
    visymMax	= execSym.a_dnttsize / cbSYMR;		/* size  of DNTT */
    residue    += execSym.a_dnttsize % cbSYMR;		/* partial rec?  */
    vcbSltFirst = SLT_OFFSET (execSym);			/* start of SLT  */
    visltMax	= execSym.a_sltsize / cbSLTR;		/* size  of SLT  */
    residue    += execSym.a_sltsize % cbSLTR;		/* partial rec?  */
    vcbSbFirst	= VT_OFFSET (execSym);			/* start of VT   */
    vissMax	= execSym.a_vtsize;			/* size  of VT   */
#endif /* S200 */

#ifdef FOCUS
    vcbLstFirst = execSym.a_link_st.fs_offset;		/* start of LST	 */
    vilstMax	= execSym.a_link_st.fs_size / cbLSTR;	/* size	 of LST	 */
    residue    += execSym.a_link_st.fs_size % cbLSTR;	/* partial rec?	 */
    vcbNpFirst	= execSym.a_name_pool.fs_offset;	/* start of NP	 */
    vinpMax	= execSym.a_name_pool.fs_size;		/* size	 of NP	 */
    vcbSymFirst = execSym.a_debug_st.fs_offset;		/* start of DNTT */
    visymMax	= execSym.a_debug_st.fs_size / cbSYMR;	/* size	 of DNTT */
    residue    += execSym.a_debug_st.fs_size % cbSYMR;	/* partial rec?	 */
    /* visymMax is in DNTT blocks, not symbols */
    vcbSltFirst = execSym.a_line_info.fs_offset;	/* start of SLT	 */
    visltMax	= execSym.a_line_info.fs_size / cbSLTR; /* size	 of SLT	 */
    residue    += execSym.a_line_info.fs_size % cbSLTR; /* partial rec?	 */
    vcbSbFirst	= execSym.a_debug_np.fs_offset;		/* start of VT	 */
    vissMax	= execSym.a_debug_np.fs_size;		/* size	 of VT	 */
#endif  /* FOCUS */

#ifdef (XDB && !SPECTRUM)
    if (vcbSymFirst == 0) {
        vfNoDebugInfo = 1;
        vcbSymFirst = 0;
        visymMax = 0;
        vcbSltFirst = 0;
        visltMax = 0;
        vcbSbFirst = 0;
        vissMax = 0;
    }
#endif

#if (FOCUS || S200)
    if (vilstMax <= 0)
	UError (
	(nl_msg(202, "No linker symbol table.  Try linking without -s.")));

    if (residue) {
	UError (
	(nl_msg(203, "One or more symbol table sections not a whole number of records")));
    }
#endif

#ifdef FOCUS
    /* this supports a kludge; see sym.h */
    if (vissMax > bitsIssMax)				/* uh oh, too big */
    {
	sprintf (vsbMsgBuf, (nl_msg(261, "Value table too big in InitSymfile() (%d)")),
		 vissMax);
	Panic (vsbMsgBuf);
    }

#endif /* FOCUS */

#ifdef SPECTRUM

    vcbLstFirst = execSym.symbol_location;		/* start of LST	 */
    vilstMax	= execSym.symbol_total;			/* size	 of LST	 */
    vcbNpFirst	= execSym.symbol_strings_location;	/* start of NP	 */
    vinpMax	= execSym.symbol_strings_size;		/* size	 of NP	 */

#ifdef HPE
    vipreDp     = execSym.presumed_dp;			/* ~ 0x40000000	  */
    vcbLstFirst += visomloc;				/* add som offset */
    vcbNpFirst  += visomloc;				/* add som offset */
#endif
    vrglst      = (LSTR *) malloc(vilstMax * cbLSTR);
    vrgnp       = (char *) malloc(vinpMax);
    lseek(vfnSym, (long) vcbLstFirst, 0);
    read(vfnSym, vrglst, cbLSTR * vilstMax);
    lseek(vfnSym, (long) vcbNpFirst, 0);
    read(vfnSym, vrgnp, vinpMax);

Retry:
   ofile_ptr.hi32.fptr = fdopen(vfnSym, "r");

#ifdef HPE
   fseek(ofile_ptr.hi32.fptr, visomloc, 0);  
#else
   rewind(ofile_ptr.hi32.fptr);  
#endif

   space_nmfdr(&stat,&ofile_ptr,"$DEBUG$",unloadable_defined,&i,&ofile_ptr);
   if (stat.error) 
   {
      vfNoDebugInfo = 1;
      vcbSymFirst = 0;
      visymMax = 0;
      vcbSltFirst = 0;
      visltMax = 0;
      vcbSbFirst = 0;
      vissMax = 0;
      goto GetAuxHdr;
   }

   /* find pxdb header subspace */

#ifdef HPE
   fseek(ofile_ptr.hi32.fptr, visomloc, 0);  
#else
   rewind(ofile_ptr.hi32.fptr);  
#endif

   subsp_nmfdr(&stat,&ofile_ptr,"$HEADER$",0,i,&j,&ofile_ptr);
   fseek(ofile_ptr.hi32.fptr,(long) ofile_ptr.offset, 0);
   fread( (char *) &subsp, sizeof(subsp), 1, ofile_ptr.hi32.fptr);
#ifdef HPE
   vipxdbheader = visomloc + subsp.file_loc_init_value;
#else
   vipxdbheader = subsp.file_loc_init_value;
#endif

   /* call PXDB if needed */
   lseek(vfnSym, vipxdbheader, 0);
   read(vfnSym, &pxdbh, sizeof(struct PXDB_header));
   if (!pxdbh.pxdbed) {
#ifdef HPE
       char *pc1;
       char *pc2;

       if (vfprntipdifd) 				/* already tried this */
           Panic((nl_msg(579,"pxdb failed (IE579)")));

       fclose(ofile_ptr.hi32.fptr);
       close(vfnSym);

       strcpy(infostring, ";INFO=\"");
       strcat(infostring, vsbSymfile);
       strcat(infostring, "\"");
       if ((pid = launch("pxdb.pub.sys", infostring, TRUE)) < 0)
          perror((nl_msg(523,"Create Process failed (IE523)")));
       
       if ((pc1= strchr(vsbSymfile,'/')) == NULL) {
           printf("Invoking PXDB on %s:\n", vsbSymfile);
       }
       else {
           pc2 = malloc(strlen(vsbSymfile));
           strncpy(pc2,vsbSymfile,pc1 - vsbSymfile);
           pc2[pc1 - vsbSymfile] = NULL;
           printf("Invoking PXDB on %s:\n", pc2);
       }
       fflush(stdout);

       if (execute(pid, 0) < 0)
          perror((nl_msg(508,"Activate failed (IE508)")));
       
       else {
          wait(&waitstat);
          if (!waitstat) {
             if ((vfnSym = open(vsbSymfile, O_RDONLY)) < 0) {
                perror(vsbSymfile);
                exit(1);
             }
             vfprntipdifd = 1;
             goto Retry;
          }
          else {
             Panic((nl_msg(579,"pxdb failed (IE579)")));
          }
       }
   }

#else /* HP-UX */

       fclose(ofile_ptr.hi32.fptr);
       close(vfnSym);
       if (fork() == 0) {
          execl("/usr/bin/pxdb","pxdb",vsbSymfile,0);
          perror((nl_msg(578,"exec failed (IE578)")));
       }
       else {
          wait(&waitstat);
          if (!waitstat) {
             if ((vfnSym = open(vsbSymfile, O_RDONLY)) < 0) {
                perror(vsbSymfile);
                exit(1);
             }
             vfprntipdifd = 1;
             goto Retry;
          }
          else {
             Panic((nl_msg(579,"pxdb failed (IE579)")));
          }
       }
   }
#endif /* HP-UX */
   
   /* get file modified time from pxdb header (if it is there) */
    if (pxdbh.bighdr) {
       vmtimeSym = pxdbh.time;
    }

#ifndef OLDSYMTAB
   /*
    * check to see if a.out is compatible with static analysis
    * info version of xdb
    */
    if (!pxdbh.sa_header) {
       printf ( "Incompatible debug information\n");
       printf ( "Please relink your program (and use new pxdb)\n");
    }
#endif

   /* find pxdb quick look up tables in gntt subspace */

#ifdef HPE
   fseek(ofile_ptr.hi32.fptr, visomloc, 0);  
#else
   rewind(ofile_ptr.hi32.fptr);  
#endif

   subsp_nmfdr(&stat,&ofile_ptr,"$GNTT$",0,i,&j,&ofile_ptr);
   fseek(ofile_ptr.hi32.fptr,(long) ofile_ptr.offset, 0);
   fread( (char *) &subsp, sizeof(subsp), 1, ofile_ptr.hi32.fptr);

#ifdef HPE
   vipdfdmd = visomloc + subsp.file_loc_init_value;
#else
   vipdfdmd = subsp.file_loc_init_value;
#endif



   /* find dntt in lntt subspace, and size in blocks */

#ifdef HPE
   fseek(ofile_ptr.hi32.fptr, visomloc, 0);  
#else
   rewind(ofile_ptr.hi32.fptr);  
#endif
   subsp_nmfdr(&stat,&ofile_ptr,"$LNTT$",0,i,&j,&ofile_ptr);
   fseek(ofile_ptr.hi32.fptr,(long) ofile_ptr.offset, 0);
   fread( (char *) &subsp, sizeof(subsp), 1, ofile_ptr.hi32.fptr);
#ifdef HPE
   vcbSymFirst = visomloc + subsp.file_loc_init_value;
#else
   vcbSymFirst = subsp.file_loc_init_value;
#endif
   visymMax = subsp.initialization_length / cbSYMR;
  


   /* find slt subspace, and size in blocks */

#ifdef HPE
   fseek(ofile_ptr.hi32.fptr, visomloc, 0);  
#else
   rewind(ofile_ptr.hi32.fptr);  
#endif

   subsp_nmfdr(&stat,&ofile_ptr,"$SLT$",0,i,&j,&ofile_ptr);
   fseek(ofile_ptr.hi32.fptr,(long) ofile_ptr.offset, 0);
   fread( (char *) &subsp, sizeof(subsp), 1, ofile_ptr.hi32.fptr);
#ifdef HPE
   vcbSltFirst = visomloc + subsp.file_loc_init_value;
#else
   vcbSltFirst = subsp.file_loc_init_value;
#endif
   visltMax = subsp.initialization_length / cbSLTR;


   
   /* find vt subspace, and size in bytes  */

#ifdef HPE
   fseek(ofile_ptr.hi32.fptr, visomloc, 0);  
#else
   rewind(ofile_ptr.hi32.fptr);  
#endif
   subsp_nmfdr(&stat,&ofile_ptr,"$VT$",0,i,&j,&ofile_ptr);
   fseek(ofile_ptr.hi32.fptr,(long) ofile_ptr.offset, 0);
   fread( (char *) &subsp, sizeof(subsp), 1, ofile_ptr.hi32.fptr);
#ifdef HPE
   vcbSbFirst = visomloc + subsp.file_loc_init_value;
#else
   vcbSbFirst = subsp.file_loc_init_value;
#endif
   vissMax = subsp.initialization_length;


   /* Read in aux header (or for hpe, make aux header) */

#ifdef HPE
GetAuxHdr:
   if (! visomloc) {     /* som file (uses HP-UX auxiliary header) */

      fseek(ofile_ptr.hi32.fptr, (long) execSym.aux_header_location, 0);
      fread((char *) &vauxhdr,sizeof(vauxhdr),1,ofile_ptr.hi32.fptr);
   }
   else {		/* lst file (uses initization records) */
      int i,j; 
      int rsize;
      long code_offset, code_length;
      long loc, min_loc, max_loc;

      j       = execSym.init_array_total;
      loc     = execSym.init_array_location;
      rsize   = sizeof(init_ptr_rec);
      min_loc = 0;
      max_loc = 0;

      for (i=0; i < j; i++) {
         fseek (ofile_ptr.hi32.fptr, (long) (visomloc + loc + i*rsize), 0);
         fread((char *) &init_ptr_rec,rsize,1,ofile_ptr.hi32.fptr);
         
	 if (init_ptr_rec.has_data) {	/* contains code */

	     code_offset = visomloc + init_ptr_rec.file_loc_init_value;
	     code_length = init_ptr_rec.initialization_length;
             
	     if (!min_loc) 
	        min_loc = code_offset;
	     else
	        min_loc = Min (min_loc, code_offset);

	     if (!max_loc) 
	        max_loc = code_offset + code_length;
	     else
	        max_loc = Max (max_loc, code_offset + code_length);
         } 
      }
      vauxhdr.exec_tmem = min_loc;
      vauxhdr.exec_tfile = min_loc;
      vauxhdr.exec_tsize = max_loc - min_loc;
   }
#else /* HP-UX */
GetAuxHdr:
   (void) fseek(ofile_ptr.hi32.fptr,(long)execSym.aux_header_location,0);
   fread((char *) &vauxhdr,sizeof(vauxhdr),1,ofile_ptr.hi32.fptr);
#endif /* HP-UX */


   /* find unwind subspace, and size in bytes  */

   vcbUnwindFirst = AdrFLabel("$UNWIND_START");
   vcbStubUnwindFirst = AdrFLabel("$UNWIND_END");
   if (vcbUnwindFirst == adrNil) {
      vfUnwind = 0;
   }
   else {
      vfUnwind = 1;
      viUnwindMax = vcbStubUnwindFirst - vcbUnwindFirst;
      vcbUnwindFirst += vauxhdr.exec_tfile - vauxhdr.exec_tmem;
      vrgUnwind = (pSUR) Malloc(viUnwindMax, true);
      fseek(ofile_ptr.hi32.fptr,vcbUnwindFirst, 0);
      fread( (char *) vrgUnwind, viUnwindMax, 1, ofile_ptr.hi32.fptr);
      viUnwindMax /= cbSUR;

      viStubUnwindMax = AdrFLabel("$RECOVER_START") - vcbStubUnwindFirst;
      vcbStubUnwindFirst += vauxhdr.exec_tfile - vauxhdr.exec_tmem;
      vrgStubUnwind = (pSTUBSUR) Malloc(viStubUnwindMax, true);
      fseek(ofile_ptr.hi32.fptr,vcbStubUnwindFirst, 0);
      fread( (char *) vrgStubUnwind, viStubUnwindMax, 1, ofile_ptr.hi32.fptr);
      viStubUnwindMax /= cbSTUBSUR;
   }
#endif /* SPECTRUM */

#if (!(FOCUS || SPECTRUM))

/*
 * We don't set cbData and cbText for FOCUS, but they aren't used anyway.
 */
    cbData   = execSym.a_data;
    cbText   = execSym.a_text;

#endif /* not FOCUS || SPECTRUM */

#if (!FOCUS)
/*
 * FOCUS does not need vmap*, and has already set the other values.
 * The sym file uses section 1 ONLY; data is in child or core file.
 */

#ifdef SPECTRUM
/* vauxhdr was read in where the stack unwind table was initialiazed */
    vmapText.b1 = vauxhdr.exec_tmem;
    vmapText.e1 = vauxhdr.exec_tmem + vauxhdr.exec_tsize;
    vmapText.f1 = vauxhdr.exec_tfile;
    vmapText.b2 = vmapText.b1;
    vmapText.e2 = vmapText.e1;
    vmapText.f2 = vmapText.f1;
    vTextAdrMin = vmapText.b1;
    vTextAdrMax = vmapText.e1;
    vTextFileStart = vmapText.f1;
#else /* not SPECTRUM */
#ifdef S200
#ifdef S200BSD
    if (vfUMM)		/* UMM */
    {
	NBPG = NBPG_UMM;
	PGSHIFT = PGSHIFT_UMM;
    }
    else	/* WOPR -- 310 or 320 */
    {
	NBPG = NBPG_WOPR;
	PGSHIFT = PGSHIFT_WOPR;
    }
    vmapText.f1 = TEXT_OFFSET(execSym);   /* standard offset		     */
    vmapText.b1 = USRSTART;		  /* section 1 always includes adr 0 */
    vmapText.e1 = vmapText.b1 + cbText + (vmagicSym==EXEC_MAGIC ? cbData : 0);
    if (vmagicSym == EXEC_MAGIC)
    {
	vmapText.b2 = vmapText.b1;
	vmapText.e2 = vmapText.e1;
	vmapText.f2 = vmapText.f1;
    }
    else
    {
	vmapText.b2 = btoc(cbText) * NBPG;
        vmapText.e2 = vmapText.b2 + cbData;
        vmapText.f2 = TEXT_OFFSET(execSym) + cbText;
    }
#else	/* not S200BSD */
    vmapText.f1 = TEXT_OFFSET(execSym);   /* standard offset		     */
    vmapText.b1 = USRSTART;		  /* section 1 always includes adr 0 */
    vmapText.e1 = vmapText.b1 + cbText + (vmagicSym==EXEC_MAGIC ? cbData : 0);
    if (vmagicSym == SHARE_MAGIC)
	vmapText.b2 = DATA_ADDR(btoc(cbText));
    else
	vmapText.b2 = vmapText.b1;
    vmapText.e2 = vmapText.b2 + cbData + (vmagicSym==EXEC_MAGIC ? cbText : 0);
    vmapText.f2 = TEXT_OFFSET(execSym) + (vmagicSym==EXEC_MAGIC ? 0 : cbText);
#endif	/* not S200BSD */
#endif /* S200 */
#endif /* not SPECTRUM */
#endif /* not FOCUS */

    return;

} /* InitSymfile */

