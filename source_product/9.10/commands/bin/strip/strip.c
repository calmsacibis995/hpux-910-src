static char *HPUX_ID = "@(#) $Revision: 70.2 $";
#include <stdio.h>
#include <a.out.h>
#include <ar.h>
#include <signal.h>
#include <ranlib.h>
#include <errno.h>
#define AROFF 48
#define LEOFF sizeof(struct ar_hdr) + 36
#define SSOFF sizeof(struct ar_hdr) + 52

/*
 *	strip	<file1> { <file2> ... }
 *
 *	If the named file is 
 *      an executable object file : strip removes the symbol table
 *                                  and debug information(same as -s of ld).
 *	a relocatable file        : strip removes local symbols and debug
 *                                  information, updating relocation records.
 *      an archive file           : strip handles each a.out file in the
 *                                  archive like a relocatable file.
 *                                  It also removes the table of contents.
 */


/* type and structure definitions */

typedef struct name_seg *nlink;

struct name_seg
{
	char *nname;	/* pointer to name */
	nlink nnext;	/* next name in list */
};

struct str_info {
	int ar_size_off;
	int lesyms_off;
	int supsyms_off;
	int new_ar_size;
	int new_lesyms;
	int new_supsize;
	struct str_info *str_next;
} *str_head, *str_last, *get_str();


/* global variables */

int argnum;		/* number of current argument being processed */
nlink namelist;		/* list of filenames to process */
char *cmdname;		/* name of this command, used in error etc */
char *name;		/* source file name */
FILE *file;		/* id of source file */
char tmpname[20];	/* actual name of temporary file */
FILE *tempfile;
struct exec filhdr;	/* header of input file */
struct exec newhdr;	/* header of output file */
struct ar_hdr arp;	/* header of archive element */
MAGIC fmagic;
int curpos;		/* current position in archive file */
int in_odd, out_odd;	/* 0 if # of bytes is even, 1 if odd */
int *symnum;		/* new symbol table number of symbols */
char *symeol;		/* is symbol the end of a supsym list */
int   eolchange;	/* do any eol flags need to change?   */
long int arsize;
int tcount;
unsigned char arch_flag;/* flag for archive file */
unsigned char r_flag;	/* flag for relocatable file */
struct header_extension exthdr;
int error_occurred;	/* so we can return non-zero if any errors occur */
int signum[] = {SIGHUP, SIGINT, SIGQUIT, SIGTERM, 0};

/* Files */

char *tmpproto = "/tmp/stmXXXXXX";	/* prototype tmp file name */


/* Internal functions */

char *nextname();
int interrupt();


/* Error Messages */

char *e1 = "filename required following -%c option";
char *e2 = "unrecognized option: %c";
char *e3 = "file %s not found";
char *e4 = "file %s format error, unrecognized magic number 0x%x";
char *e5 = "file %s format error, improper system id 0x%x";
char *e6 = "file %s already stripped";
char *e9 = "file %s format error, unexpected eof";
char *e10 = "write error on temporary file %s";
char *e11 = "could not open file %s for writing, a copy is available as %s";
char *e12 = "could not open temporary file %s for reading";
char *e13 = "read error on temporary file %s";
char *e14 = "could not open temporary file %s for writing";
char *e15 = "file %s is in old archive format, use 'arcv' to convert";
char *e16 = "%s: Operation not supported on socket";

/*************************************************************************
	main -	process arguments, call major loop and go away
 *************************************************************************/

main(argc, argv)
int argc;
char *argv[];
{
	register long n;
	register int c;
	char arcmag[SARMAG+1];

	for (c = 0; signum[c]; c++)
		if (signal(signum[c], SIG_IGN) != SIG_IGN)
			signal(signum[c], interrupt);

	procargs(argc, argv);
	while((name = nextname()) != NULL)
	{
		arch_flag = 0;
		r_flag = 0;

		if ((file = fopen(name, "r")) == NULL)
		{
			if (errno == EOPNOTSUPP)
				error(e16, name);
			else
				error(e3, name);
			continue;
		}
		if (fread(arcmag, SARMAG, 1, file) < 1)
		{
			error(e9, name);
			cleanup();
			continue;
		}
		if (strncmp(arcmag, ARMAG, SARMAG) == 0) arch_flag++;
		else 
		{
		     fseek(file, 0L ,0);
		     if (fread(&fmagic, sizeof(fmagic), 1, file) < 1)
		     {
			error(e9, name);
			cleanup();
			continue;
		     }
		     if (fmagic.system_id != HP98x6_ID
                         && fmagic.system_id != HP9000S200_ID)
		     {	error(e5, name, fmagic.system_id);
			cleanup();
			continue;
		     }
		     switch(fmagic.file_type)
		     {
		     case AR_MAGIC: error(e15, name);
				    cleanup();
				    continue;
		     case RELOC_MAGIC: r_flag++;
		     case DL_MAGIC:
		     case SHL_MAGIC:
		     case EXEC_MAGIC:
                     case DEMAND_MAGIC:
		     case SHARE_MAGIC:	break;
		     default:	error(e4, name, fmagic.file_type);
				cleanup();
				continue;
		     }
		}
#ifndef nocpy
		strcpy(tmpname, tmpproto);
		mktemp(tmpname);
#ifdef	PRESERVE
		signal(SIGINT, interrupt);
#endif	/* PRESERVE */
#endif
#ifdef nocpy
		strcpy(tmpname,"temp");
#endif
		if ((tempfile = fopen(tmpname, "w+")) == NULL)
		{
			error(e14, tmpname);
			cleanup();
			continue;
		}

		if (arch_flag) strip_archive();
		else if (r_flag)
		     {
			if (strip_reloc() == -1)
			{
				cleanup();
				continue;
			}
		     }
		else 
			if (strip_object() == -1)
			{
				cleanup();
				continue;
			}
 
		fclose(file);
		file = NULL;
		fclose(tempfile);
		tempfile = NULL;

#ifndef nocpy
#ifdef	PRESERVE
		signal(SIGINT, SIG_IGN);	/* push on thru */
#endif	/* PRESERVE */
		if ((file = fopen(name, "w")) == NULL)
		{
			error(e11, name, tmpname);
			continue;
		}
		if ((tempfile = fopen(tmpname, "r")) == NULL)
		{
			error(e12, tmpname);
			continue;
		}
		while ((c = getc(tempfile)) != EOF)
		{
			putc(c, file);
			if (ferror(file))
			{
				error(e10, name);
				goto bad;
			}
		}
		if (ferror(tempfile))
		{
			error(e13, tmpname);
			continue;
		}
		fclose(tempfile);
		unlink(tmpname);

	bad:
#ifdef	PRESERVE
		signal(SIGINT, SIG_DFL);
#endif	/* PRESERVE */
		if (file != NULL) fclose(file);
		if (tempfile != NULL) fclose(tempfile);
#endif
	}
	exit(error_occurred);
}

/***********************************************************************
	strip_object--remove symbol table and debug information
	from an executable a.out file
***********************************************************************/

strip_object()
{
	register int n,c;

	/*
	 * NOTE the following assumption about a.outs:
	 * there are currently only 2 types of extension
	 * headers: shared lib and debug.  If both ext
	 * headers exist, the shared lib ext header will
	 * come first, the debug ext header second.  The
	 * shared lib ext header is in the text segment
	 * while the debug header comes after the modcal
	 * info section.  Strip truncates the file after
	 * the text segment thus removing the debug info,
	 * the debug ext header and, in the future, any
	 * other new type of ext header.  Therefore when
	 * any new type of ext header is added, this code
	 * should be reviewed based on whether or not this
	 * new ext header is to be removed by strip.  Any
	 * new ext header that is to be stripped should
	 * be after the shared lib ext header in the ext
	 * header linked list.  If the new ext header is
	 * to be left intact by strip then the code that
	 * only copies the file up to the end of text must
	 * be modified.
	 */


	fseek(file,0,0);
	fread(&filhdr, sizeof(filhdr), 1, file);
	newhdr = filhdr;
	if (filhdr.a_lesyms == 0)
	{	error(e6, name);
		--error_occurred;
		return -1;
	}
	newhdr.a_drsize = newhdr.a_trsize = newhdr.a_lesyms = 0;
	newhdr.a_spared = newhdr.a_spares = newhdr.a_supsym = 0;

	newhdr.a_miscinfo &= ~M_DEBUG;

	if (newhdr.a_extension >= MODCALPOS) {
		/*
		 * The first extension header is after
		 * the text, so it must be a debug ext
		 * header, not a shared lib ext header.
		 * we are stripping debug info, so that
		 * leaves no ext headers
		 */
		newhdr.a_extension = 0;
	}

	if (fwrite(&newhdr, sizeof(filhdr), 1, tempfile) < 1)
	{
		error(e10, tmpname);
		return -1;
	}
	fseek(file, TEXTPOS, 0);
	fseek(tempfile, TEXTPOS, 0);
	for (n = MODCALPOS - (TEXTPOS); --n >= 0;)
	{
		if ((c = getc(file)) == EOF)
		{
			error(e9, name);
			return -1;
		}
		putc(c, tempfile);
		if (ferror(tempfile))
		{
			error(e10, tmpname);
			return -1;
		}
	}

	/* 
	 * if a_extension is non-zero then there is
	 * a shared lib ext header.  Any other ext
	 * headers (if they exist) have been deleted so 
	 * we must nil out the "next pointer" in the
	 * shared lib ext header
	 */
	if (newhdr.a_extension != 0) {
		fseek(tempfile, filhdr.a_extension, 0);
		fread(&exthdr, sizeof(exthdr), 1, tempfile);
		exthdr.e_extension = 0;
		fseek(tempfile, filhdr.a_extension, 0);
		fwrite(&exthdr, sizeof(exthdr), 1, tempfile);
	}

	return 0;
}

/***********************************************************************
	strip_reloc--remove local symbols and debug information
	from a relocatable a.out file
***********************************************************************/

strip_reloc()
{
	register int n,c;
	int old_debug = 0;

	fseek(file,0,0);
	fread(&filhdr, sizeof(filhdr), 1, file);
	newhdr = filhdr;
	if (filhdr.a_lesyms == 0)
	{	error(e6, name);
		--error_occurred;
		return -1;
	}
	newhdr.a_spared = newhdr.a_spares = newhdr.a_extension = 0;

	newhdr.a_miscinfo &= ~M_DEBUG;

	if (filhdr.a_spared || filhdr.a_spares) {
		old_debug = 1;
		newhdr.a_supsym = 0;
	}

        fseek(tempfile, sizeof(filhdr), 0);

	for (n = filhdr.a_text + filhdr.a_data + filhdr.a_pasint; --n >= 0;)
	{
		if ((c = getc(file)) == EOF)
		{
			error(e9, name);
			return -1;
		}
		putc(c, tempfile);
		if (ferror(tempfile))
		{
			error(e10, tmpname);
			return -1;
		}
	}

	newhdr.a_lesyms -= stripsym();

	if (filhdr.a_supsym && !old_debug) {
		newhdr.a_supsym = stripsupsym();
	}

	if (old_debug) {
		fseek(file, RTEXTPOS + filhdr.a_spared + filhdr.a_spares, 0);
	}
	else {
		fseek(file, RTEXTPOS, 0);
	}
	copy_rinfo();
	fseek(tempfile, 0, 0);
	if (fwrite(&newhdr, sizeof(filhdr), 1, tempfile) < 1)
	{
		error(e10, tmpname);
		return -1;
	}

	return 0;
}

/***********************************************************************
	STRIP_ARCHIVE--tasks performed to strip archive:
		remove non-global symbols from symbol table in each 
			a.out section
		change ar_size in each archive header
		change lesyms in each a.out header
		change symbol number in each relocation section
		update date of each archive element
***********************************************************************/

strip_archive()
{
	register struct str_info *str_ptr;
	int striplen,i;
	int stripsuplen;
	char buf[11];
	long int dbg_size;
	int  new_debug;
	int  old_debug;

	str_head = NULL;
	str_last = NULL;
	curpos = SARMAG;
	if (fwrite(ARMAG, SARMAG, 1, tempfile) < 1)
	{
		error(e10, tmpname);
		return -1;
	}
	tcount = SARMAG;

	/* get the first archive header, if its the directory, skip it */

	fread(&arp,sizeof arp,1,file);
	fseek(file, SARMAG, 0);
	if (strncmp(arp.ar_name,DIRNAME,strlen(DIRNAME)) == 0)
	{
		arsize = atol(arp.ar_size);
		fseek(file, SARMAG + sizeof(arp) + ((arsize + 1) & ~1), 0);
	}

	while (nextel())
	{
	    /* read a.out header and write it out */

	    fread(&filhdr, sizeof (filhdr), 1, file);
	    tcount += sizeof filhdr;
	    if (N_BADMAG(filhdr))  /* not a.out-- just copy it */
	    {	fseek(file, -sizeof(filhdr), 1);
#ifdef bill
fprintf(stderr,"copying non a.out file, size is %d\n",arsize);
#endif
		copy(name,file,tempfile,(arsize + 1) & ~1);
		curpos = curpos + sizeof(arp) + ((arsize + 1) & ~1);
		continue;
	    }

	    new_debug = old_debug = 0;
            if (filhdr.a_miscinfo & M_DEBUG) {
		new_debug = 1;
		fseek(file, filhdr.a_extension - sizeof(filhdr), 1);
		fread(&exthdr, sizeof(exthdr), 1, file);
	        dbg_size = exthdr.e_spec.debug_header.header_size + 
			   exthdr.e_spec.debug_header.gntt_size + 
			   exthdr.e_spec.debug_header.lntt_size + 
			   exthdr.e_spec.debug_header.slt_size + 
			   exthdr.e_spec.debug_header.vt_size + 
			   exthdr.e_spec.debug_header.xt_size + 
			   sizeof(exthdr) +
			   filhdr.a_extension -
			   (RDATAPOS + filhdr.a_drsize);
		fseek(
		    file, 
		    -(filhdr.a_extension + sizeof(exthdr) - sizeof(filhdr)), 
		    1
		);
		filhdr.a_miscinfo &= ~M_DEBUG;
	    }
	    else if (filhdr.a_spared || filhdr.a_spares) {
		old_debug = 1;
	    	dbg_size = filhdr.a_spared + 
			   filhdr.a_spares + 
			   filhdr.a_supsym;
		filhdr.a_supsym = 0;
	    }
	    else {
	    	dbg_size = 0;
	    }
	    filhdr.a_spared = filhdr.a_spares = filhdr.a_extension = 0;
	    if (fwrite(&filhdr, sizeof(filhdr), 1, tempfile) < 1)
	    {
		error(e10, tmpname);
		return -1;
	    }

	    /* copy out the text, data, and pascal interface sections of a.out */

	    copy(name,file,tempfile,filhdr.a_text+filhdr.a_data+
			filhdr.a_pasint);
	    
	    /* strip the symbol table of non-global symbols */

	    striplen = stripsym();

	    if (filhdr.a_supsym && !old_debug) {
		stripsuplen = stripsupsym();
	    }
	    else {
		stripsuplen = filhdr.a_supsym;
	    }

#ifdef bill
fprintf(stderr,"amount stripped is %d\n",striplen);
#endif

	    if (old_debug) {
		fseek(file, dbg_size, 1);
	    }

	    /* copy and alter the relocation information */

	    copy_rinfo();

	    if (new_debug) {
		fseek(file, dbg_size, 1);
	    }

	    /* allocate a new str_info element and add it to list */

	    str_ptr = (struct str_info *)malloc(sizeof(struct str_info));
	    str_ptr->ar_size_off = curpos + AROFF;
	    str_ptr->lesyms_off = curpos + LEOFF;
	    str_ptr->supsyms_off = curpos + SSOFF;
	    str_ptr->new_lesyms = filhdr.a_lesyms - striplen;
	    str_ptr->new_supsize = stripsuplen;
	    str_ptr->new_ar_size = arsize - 
				   striplen - 
				   dbg_size -
				   (filhdr.a_supsym - stripsuplen);
	    add_str(str_ptr);

	    /* update curpos */

	    out_odd = str_ptr->new_ar_size & 1;
	    curpos += sizeof(arp) + str_ptr->new_ar_size + out_odd;

	    /* if in element is odd, advance one byte, if out element
	       is odd write one byte */

	    if (in_odd) fseek(file,1,1);
	    if (out_odd) fwrite(&out_odd,1,1,tempfile);

	}
	    /* now go thru list */

	rewind(tempfile);
        while (str_ptr = get_str())
        {	
	    fseek(tempfile,str_ptr->ar_size_off,0);
	    sprintf(buf, "%-10ld", str_ptr->new_ar_size);
	    fwrite(buf,10,1,tempfile);

	    fseek(tempfile,str_ptr->lesyms_off,0);
	    i = str_ptr->new_lesyms;
	    fwrite(&i,4,1,tempfile);

	    fseek(tempfile,str_ptr->supsyms_off,0);
	    i = str_ptr->new_supsize;
	    fwrite(&i,4,1,tempfile);
	    free(str_ptr);

#ifdef bill
fprintf(stderr,"new ar_size is %d\n",str_ptr->new_ar_size);
#endif

	}
}

/***********************************************************************
	NEXTEL--read next archive header from file, change the date,
	and write it back out.  if EOF return 0 else return 1
***********************************************************************/

nextel()
{
	long time();
	char buf[13];

	if (fread(&arp,sizeof arp,1,file) != 1) return 0;
	tcount += sizeof arp;
	arsize = atol(arp.ar_size);
	sprintf(buf, "%-12ld", time(0));
	strncpy(arp.ar_date, buf, 12);

	/* if the size is odd it will be necessary to read one more byte
	   at the end of the archive element */

	in_odd = arsize & 1;
	if (fwrite(&arp, sizeof(arp), 1, tempfile) < 1)
	{
		error(e10, tmpname);
		return -1;
	}

#ifdef bill
fprintf(stderr,"nextel: name is %s, size is %d\n",arp.ar_name,arsize);
#endif

	return 1;
}

/***********************************************************************
	STRIPSYM--strip symbol table. Remove all non-global symbols and
	return the number of bytes removed.
***********************************************************************/

stripsym()
{
	struct nlist_ sym;
	int pos, striplen = 0, newnum = 0;
	register int i,symcount = 0;
	char *sp,symbuf[SYMLENGTH +1], c;
	int maxsyms;

	maxsyms = filhdr.a_lesyms / sizeof(sym);
	symnum = (int *) malloc(maxsyms * sizeof(int));
	symeol = (char *) malloc(maxsyms * sizeof(char));
	for(pos=0;pos<filhdr.a_lesyms;pos+=sizeof(sym)+sym.n_length)
	{	fread(&sym,sizeof(sym),1,file);	/* read nlist_ structure */
		tcount += sizeof sym;

		/* get symbol name from file */

		for(sp = symbuf,i=sym.n_length;i>0;i--)
		{	if (((c=getc(file)) == EOF) || (c==0))
			{
				error(e9, name);
				return;
			}
			*sp++ = c;
		}
		*sp = 0;
		tcount += strlen(symbuf);

#ifdef junk
fprintf(stderr,"symbol is %s\n",symbuf);
#endif

		/* if symbol is global write it out */

		if ((sym.n_type&N_EXT) || sym.n_dlt)
		{	fwrite(&sym,sizeof(sym),1,tempfile);
			fputs(symbuf,tempfile);
			symnum[symcount] = newnum;
			newnum++;
		}
		else
		{ 	striplen += sizeof(sym) + strlen(symbuf);
			symnum[symcount] = -1;
		}
		symeol[symcount] = sym.n_list;
		symcount++;
	}

#ifdef bill
for(i=0;i<=symcount;i++)
fprintf(stderr,"symbol %d becomes %d\n",i,symnum[i]);
#endif

	return striplen;
}

/***********************************************************************
	STRIPSUPSYM--strip sup symbol table. Remove all supsym entries 
	whose sym entry was removed
***********************************************************************/

stripsupsym()
{
	register struct supsym_entry *sup;
	register int i,j;
	int          supnum;
	int          supsize;
	int         *backptr;

	eolchange = 0;

	supnum = filhdr.a_supsym / sizeof(struct supsym_entry);

	supsize = 0;

	sup = (struct supsym_entry *) malloc(filhdr.a_supsym);
	fread(sup, filhdr.a_supsym, 1, file);

        backptr = (int *) malloc(supnum * sizeof(int));
	for (i = 0; i < supnum; i++) {
		backptr[i] = -1;
	}

	for (i = 0; i < supnum; i++) {
		if (!symeol[i]) {
			backptr[sup[i].a.next] = i;
		}
	}

	for (i = 0; i < supnum; i++) {
		if ((symnum[i] == -1) && (backptr[i] != -1)) {
			j = backptr[i];
			sup[j].a.next = sup[i].a.next;
			if (symeol[i]) {
			   symeol[j] = 1;
			   eolchange = 1;
			}
			else {
			   backptr[sup[i].a.next] = j;
			}
		}
	}

	if (eolchange) {
		redosymeol();
	}

	for (i = 0; i < supnum; i++) {
		if (symnum[i] != -1) {
			supsize += sizeof(struct supsym_entry);
			fwrite(&sup[i],sizeof(struct supsym_entry),1,tempfile);
		}
	}

	return(supsize);
}

/***********************************************************************
	REDOSYMEOL--go over the new symbol table and reset the eol
	field base on the results of stripsypsym()
***********************************************************************/

redosymeol()
{
	struct nlist_ sym;
	int pos;
	int oldpos = -1;

	fseek(tempfile,-newhdr.a_lesyms,1);

	for(pos=0;pos<newhdr.a_lesyms;pos+=sizeof(sym)+sym.n_length)
	{
		for (oldpos++; symnum[oldpos] == -1; oldpos++);
		fread(&sym,sizeof(sym),1,tempfile);
		sym.n_list = symeol[oldpos];
		fseek(tempfile,-sizeof(sym),1);
		fwrite(&sym,sizeof(sym),1,tempfile);
		fseek(tempfile,sym.n_length,1);
	}
}

/***********************************************************************
	COPY_RINFO--copy relocation information and update the
	symbolnum entry
***********************************************************************/

copy_rinfo()
{	register int i;
	struct r_info rinfo;

	for(i=0; i < (filhdr.a_trsize+filhdr.a_drsize) / sizeof(rinfo); i++)
	{	fread(&rinfo,sizeof(rinfo),1,file);

#ifdef bill
fprintf(stderr,"copy_rinfo: symbolnum is %d",rinfo.r_symbolnum);
#endif

		if ((rinfo.r_segment == REXT) ||
		    (rinfo.r_segment == RDLT) ||
		    (rinfo.r_segment == RPLT) ||
		    (rinfo.r_segment == RPC))
			rinfo.r_symbolnum = symnum[rinfo.r_symbolnum];
		else
			rinfo.r_symbolnum = 0;
		fwrite(&rinfo,sizeof(rinfo),1,tempfile);
	}
}

/**********************************************************************
	ADD_STR--add str_info structure to list
**********************************************************************/

add_str(p)
register struct str_info *p;
{
	if (str_head == NULL)
	{	str_head = p;
		str_last = p;
	}
	else
	{	str_last->str_next = p;
		str_last = p;
	}
	p->str_next = NULL;
}

/***********************************************************************
	GET_STR--get str_info structure from list
***********************************************************************/

struct str_info *
get_str()
{
	register struct str_info *p;

	p = str_head;
	if (p) str_head = p->str_next;
	return p;
}

/***********************************************************************
	COPY--copy from input to output
***********************************************************************/

copy(name, fr, to, size)
char *name;
FILE *fr, *to;
long size;
{
	register s, n;
	char buf[512];

#ifdef junk
fprintf(stderr,"copy: size is %d\n",size);
#endif

	tcount += size;
	while(size != 0) {
		s = 512;
		if(size < 512)
			s = (int)size;
		n = fread(buf, 1, s, fr);
		if(n != s) {
			error(e9, name);
			return(1);
		}
		n = fwrite(buf, 1, s, to);
		if(n != s) {
		        error(e10, tmpname);
			return(1);
		}
		size -= s;
	}
	fflush(to);
	return(0);
}

/*************************************************************************
	procargs - process command arguments
 *************************************************************************/

procargs(argc, argv)
int argc;
char *argv[];
{
	cmdname = argv[0];
	for (argnum = 1; argnum < argc; argnum++) {
		if (argv[argnum][0] == '-' )
			procflags(&argv[argnum][1], argc, argv);
		else newname(argv[argnum]);
	}
}


/*************************************************************************
	procflags - process flags
 *************************************************************************/

procflags(flagptr, argc, argv)
char *flagptr;
int argc;
char *argv[];
{
	char c;
	while (c = *flagptr++) switch(c)
	{
	default:	error(e2, c);
	}
}

/*************************************************************************
	error - type a message on error stream
 *************************************************************************/

/*VARARGS1*/
error(fmt, a1, a2, a3, a4, a5)
char *fmt;
{
	++error_occurred;
	fprintf(stderr, "%s: ", cmdname);
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
	fprintf(stderr, "\n");
}


/*************************************************************************
	fatal - type an error message and abort
 *************************************************************************/

/*VARARGS1*/
fatal(fmt, a1, a2, a3, a4, a5)
char *fmt;
{
	error(fmt, a1, a2, a3, a4, a5);
	exit(1);
}
/*************************************************************************
  newname -	Attach a new name to the list of names in name list.
 *************************************************************************/

newname(name)
char *name;
{	nlink np1, np2;
	np1 = (nlink)malloc(sizeof(*np1));
	np1->nname = name;
	np1->nnext = NULL;
	if (namelist == NULL) namelist = np1;
	else
	{	np2 = namelist;
		while(np2->nnext != NULL) np2 = np2->nnext;
		np2->nnext = np1;
	}
}


/*************************************************************************
  nextname - 	Return the next name from the list of names being processed.
 *************************************************************************/

char *nextname()
{
	nlink np;
	if (namelist == NULL) return(NULL);
	np = namelist;
	namelist = np->nnext;
	return(np->nname);
}
/*************************************************************************
 cleanup -	Remove open files.
 *************************************************************************/

cleanup()
{
	if (tempfile != NULL) 
	{	unlink(tmpname);
		fclose(tempfile);
		tempfile = NULL;
	}
	fclose(file);
	file = NULL;
}


/*************************************************************************
 interrupt -	Interrupt signal handler.
 *************************************************************************/

interrupt(sig)
{
	int i;

	/* ignore signals */
	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_IGN);

	cleanup();

	/* set signals to default values */
	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_DFL); 

	kill(getpid(),sig);
}
