/* @(#) $Revision: 66.2 $ */   

/****************************************************************************

	TRIX DEBUGGER - setup routines for a.out and core files

****************************************************************************
*/

#include "defs.h"
#include <sys/stat.h>

MAP		txtmap;
MAP		datmap;
int		wtflag;
int		fcor;
int		fsym;
long		maxfile;
long		mainsval;
long		txtsiz;
long		datsiz;
long		datbas;
long		stksiz;
STRING		errflg;
short		magic;
long		symbas;
long		entrypt;
int		signum = 0;
STRING		symfil = "a.out";
STRING		corfil = "core";
BHDR 		bhdr;
BKPTR		bkpthead;
char		coremapped;
PROCEXEC	*cexec;
PROC_REGS	*cregs;
COREMAP		*cmaps;
TDHDR		PData;
TDHDR		PText;
TDHDR		PStack;

#define BHDRS	sizeof(struct exec)

extern char *malloc();
extern void free();

setbout()				/* setup a.out file */
{
	if (*symfil == '-') fsym = -1;
	else {
		fsym = open(symfil, wtflag);
		if ((fsym < 0) && wtflag) fsym = creat(symfil);
		printf("executable file = %s", symfil);
		if (fsym < 0) printf(" - cannot open or create"); 
	}

	txtmap.ufd = fsym;		/* user may exchange files */

	if (read(fsym, &bhdr, BHDRS) == BHDRS)
	{
		magic = bhdr.a_magic.file_type;
		if (N_BADMAG(bhdr) || 
		    bhdr.a_magic.system_id == HP98x6_ID)
		{
			magic = 0;
			printf(" - warning: not in executable format");
		}
		else
		{
			/* treat relocatable files like unshared */
			if (magic == RELOC_MAGIC) 
			{
				printf(" - warning: relocatable file");
				/* restored later */
				magic = EXEC_MAGIC;
			}

			txtsiz = bhdr.a_text;
			datsiz = bhdr.a_data;
			entrypt = bhdr.a_entry;
			symbas = LESYM_OFFSET(bhdr);
			txtmap.b1 = USRSTART;
			txtmap.e1 = txtmap.b1+txtsiz+(magic==EXEC_MAGIC ? datsiz : 0);
			txtmap.f1 = TEXT_OFFSET(bhdr);
			if (magic == SHARE_MAGIC || magic == DEMAND_MAGIC)
			    datbas = EXEC_ALIGN(txtsiz);
			else datbas = txtmap.b1;
			txtmap.b2 = datbas;
			txtmap.e2 = datbas+datsiz+(magic==EXEC_MAGIC ? txtsiz : 0);
			txtmap.f2 = (magic == EXEC_MAGIC ? BHDRS : DATA_OFFSET(bhdr));

			/* read in symbol table from a.out file */
			setupsym(bhdr.a_lesyms);
			symset();
		}
	}
	if (magic == 0) txtmap.e1 = maxfile;

	if (bhdr.a_magic.file_type == RELOC_MAGIC) magic = RELOC_MAGIC;

	printf("\n");
}


releasecoremap()
{ COREMAP *tmap;
	if (cregs) free (cregs);		/* Free up any memory    */
	if (cexec) free (cexec);		/* previously malloc'ed. */
	while (cmaps) {
	   tmap = cmaps;			/* Walk down the list    */
	   cmaps = cmaps->next_map;		/* giving it back.	 */
	   free (tmap);
	}
	cregs = (PROC_REGS *) 0;		/* Zero the pointers to   */
	cexec = (PROCEXEC *) 0;			/* memory that was free'd */
	cmaps = (COREMAP *) 0;	  /* Must be zero already, but what the hey. */
}


int is_mem_or_kmem(fd)			/* SMT - for FSDdt03559 */
  int fd;
  {  struct stat st;
     fstat(fd, &st);
     return S_ISCHR(st.st_mode) && (major(st.st_rdev) == 3);
  }


setcor()
{ int mce;
	releasecoremap;
	coremapped = 0;
	datmap.b1 = datmap.b2 = 0;	/* Initialize the default maps. */
	datmap.e1 = datmap.e2 = maxfile;
	datmap.f1 = datmap.f2 = 0;
	if (*corfil == '-') fcor = -1;
	else if ((fcor = open(corfil, wtflag)) == -1)
		printf("cannot open %s\n", corfil);
	else printf("core file = %s\n", corfil);
	if (fcor == -1) return;
	datmap.ufd = fcor;		/* user map exchange files */
	if (magic == RELOC_MAGIC)
		printf ("- core file cannot be for relocatable file\n");
	else if ( is_mem_or_kmem(fcor) )
		printf(" - warning: core file is mem or kmem\n");
	else
	{	signum = 0;	/* why this */
		if ((mce = makecoremap(fcor)) == -1)
			printf("- bad format in core file\n");
		if (!mce) coremapped = 1;
		if ((magic == 0) ||
	    	    (cexec && magic != cexec->ux_mag &&
		     cexec->ux_mag != 0))
			printf(" - warning: bad magic number\n");
		if (cregs) readregs();
		else printf("- no registers in core file\n");
		if (coremapped)
		{	if (PText.cnt) txtsiz = PText.siz;
			if (PData.cnt)
			{	datsiz = PData.siz;
				datbas = PData.bas;
			}
			if (PStack.cnt)
				stksiz = PStack.siz;
		}
		if (!coremapped || !PData.cnt)  /* what use this ?? */
			datbas = (magic == SHARE_MAGIC) ? txtmap.b2 : USRSTART + txtsiz;
	}
}


makecoremap(cf)
int cf;
{ COREHEAD chtmp;
  int res_val, hdr_count=0, filepos;
  COREMAP *thismap, *lastmap;

	lseek (cf,0L,SEEK_SET);	
	chtmp.len = 0;

	while ((res_val = next_ch (&chtmp,cf,&filepos)) > 0)
	{	++hdr_count;	/* got one */

		if (((char *)thismap = malloc(sizeof(COREMAP))) == 0)
		{	fprintf(stderr, "out of memory\n");
			return -2;
		}			/* build map entry for it */
		if (!cmaps) cmaps = thismap;
		else lastmap->next_map = thismap;
		thismap->type	= chtmp.type;
		thismap->b	= (long) chtmp.addr;
		thismap->e	= thismap->b + chtmp.len;
		thismap->f	= filepos;
		thismap->next_map = 0;
		lastmap = thismap;
				
		switch (chtmp.type)	/* do some special cases */
		{
		  case CORE_PROC:	/* Copy registers if present. */
			if (chtmp.len != sizeof(PROC_REGS))
				printf("- registers bad in core file\n");
			else if (((char *)cregs = malloc(sizeof(PROC_REGS))) == 0)
			{	fprintf(stderr, "out of memory\n");
				return -2;
			}
			read (cf,cregs,sizeof(PROC_REGS));
			chtmp.len = 0;
			break;

		  case CORE_TEXT:	/* info about text segments */
			PText.cnt++;
			PText.siz += chtmp.len;
			if ((PText.cnt == 1) ||
			   ((unsigned int)(chtmp.addr) < PText.bas))
				PText.bas = (unsigned int)(chtmp.addr);
			break;

		  case CORE_DATA:	/* info about data segments */
			PData.cnt++;
			PData.siz += chtmp.len;
			if ((PData.cnt == 1) ||
			   ((unsigned int)(chtmp.addr) < PData.bas))
				PData.bas = (unsigned int)(chtmp.addr);
			break;

		  case CORE_STACK:	/* info about stack segments */
			PStack.cnt++;
			PStack.siz += chtmp.len;
			if ((PStack.cnt == 1) ||
			   ((unsigned int)(chtmp.addr) < PStack.bas))
				PStack.bas = (unsigned int)(chtmp.addr);
			break;

		  case CORE_EXEC:	/* Copy exec area if present. */
			if (chtmp.len != sizeof(PROCEXEC))
				printf("- exec area bad in core file\n");
			else if (((char *)cexec = malloc(sizeof(PROCEXEC))) == 0)
			{	fprintf(stderr, "out of memory\n");
				return -2;
			}
			read (cf,cexec,sizeof(PROCEXEC));
			chtmp.len = 0;
			break;

		  default:;
		}
	}

	if (res_val || !hdr_count)    /* must end on eof && must not be empty */
	{ 	releasecoremap();
		return -1;	/* clean up and report failure */
	}

	return 0;		/* Indicate success. */
}

next_ch(last_ch,cf,curpos)	/* This routine reads a corehead into */
COREHEAD *last_ch;		/* the buffer last_ch, updates curpos */
int cf;				/* for use in a coremap, and reports: */
int *curpos;			/*   eof, unexpected end, or success. */
{ int inc,tp;

	*curpos = lseek(cf,last_ch->len,SEEK_CUR);
	inc = read(cf,last_ch,sizeof(COREHEAD));
	if (!inc) return 0;				/* end of file ok */
	if (inc != sizeof(COREHEAD)) return(-1);	/* unexpected end */
	*curpos += inc;
	for (tp = last_ch->type, inc = 0; tp; tp >>= 1) inc += tp & 0x1;
	if (inc != 1) return(-1);			/* bad type in hdr */
	return 1;					/* success */
}

