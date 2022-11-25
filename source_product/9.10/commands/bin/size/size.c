static char *HPUX_ID = "@(#) $Revision: 66.1 $";

/*
 *	size	[-xod]	<file1> ... <filen>	Print sizes of segments.
 *	-[xod]	print in [hex/octal/decimal] format (default is decimal)
 *
 *	The first version of this command for the series 200 came from
 *	the MIT system, and was part of its 2.0 release.
 *
 *	The main routine was adapted from Bell System V Release 2, and
 *	the options and output have been made compatible with that command.
 *
 *	Support for archives and for the series 500 a.out format were
 *	added from scratch.
 *
 *	The series 200 release 2.0 version supported several undocumented
 *	options, notably -l for a long format output, and -n to force
 *	printing of the filename.  These were removed for the sake of
 *	compatibility, and because they were of minimal value.  The code
 *	is still present, and can be conditionally compiled with the
 *	-DMINUS_L_OPTION flag.  The -l option could possibly be useful
 *	on the series 500 if it were extended to print the size of each
 *	code and data segment.
 */


#include <stdio.h>
#include <a.out.h>
#include <ar.h>

#ifdef hp9000s500
#define N_BADMACH(x)  ((x).a_magic.system_id != HP9000_ID)
#endif

/* global variables */

char *filename;		/* input file name */
FILE *infile;		/* id of input file */
#ifdef MINUS_L_OPTION
char lflag = 0;		/* print long form */
#endif
char nflag = 0;		/* print names of files */
struct exec filhdr;	/* header of input file */

char DECIMAL_FORMAT[]	=	"%ld";
char HEX_FORMAT[]	=	"0x%lx";
char OCTAL_FORMAT[]	=	"0%lo";
char *output_format	=	DECIMAL_FORMAT;


/* Multiply-referenced Error Messages */

char	BAD_READ[] =		"bad read of %s";
char	BAD_SEEK[] =		"bad seek on %s";

#ifdef hp9000s500
char	OUT_OF_MEMORY[] =	"out of memory";
#endif

/*************************************************************************
	main -	process arguments, call major loop and go away
 *************************************************************************/

main(argc, argv)

int	argc;
char	**argv;

{
	/* UNIX FUNCTIONS CALLED */
	extern 		fprintf( ),
			exit( );

	register int		filec;
	register char		**filev;
	register int		flagc;
	register char		**flagv;

	--argc;
	filev = ++argv;
	filec = 0;


	for (flagc = argc, flagv = argv; flagc > 0; --flagc, ++flagv) {

		if (**flagv == '-') {
		while(*++*flagv != '\0') {
			switch (**flagv) {
			case 'o':
				output_format = OCTAL_FORMAT;
				break;

			case 'd':
				output_format = DECIMAL_FORMAT;
				break;

			case 'x':
				output_format = HEX_FORMAT;
				break;

#ifdef MINUS_L_OPTION
			case 'l':
		 		lflag = 1;
				break;

			case 'n':
				nflag = 1;
				break;
#endif
			default:
				error("unknown option \"%c\" ignored",
				      **flagv);
				break;
			}
		}
		} else {
			*filev++ = *flagv;
			++filec;
		}
	}

	if (filec == 0) {
		fprintf(stderr, "usage:  size [-oxd] file ...\n");
		exit(0);
	}

	if ( filec > 1)
		nflag = 1;

	for (filev = argv; filec > 0; --filec, ++filev) {
		filename = *filev;
		if ((infile = fopen(filename, "r")) == NULL)
			error("cannot open file %s", filename);
		else {
			(void) size(0L, filename);
			fclose(infile);
		}
	}
}

/*************************************************************************
	error - type a message on error stream
 *************************************************************************/

/*VARARGS1*/
error(fmt, a1, a2, a3, a4, a5)
char *fmt;
{
	fflush(stdout);
	fprintf(stderr, "size: ");
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
	fprintf(stderr, "\n");
}


#ifdef hp9000s500
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
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* trim -       Removes slash and makes the string asciz. */

char * trim(s)
char *s;
{
	register int i;
	for(i = 0; i < 16; i++)
	   if (s[i] == '/') s[i] = '\0';
	return(s);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*************************************************************************
  size - 	Print segment sizes.
		In case of an archive, calls itself recursively for each
		element.  Doesn't allow nested  archives  since System V
		doesn't.  Ignores  non-magic  archive  elements,  unlike
		System V which halts  processing  of the entire  archive
		(that would be unacceptable for ranlib'd archives).  The
		offset  parameter  is the offset of the current  file or
		archive  element  from the start of the file; it is zero
		unless processing an archive element.
 *************************************************************************/

size(offset, current_name)
register long offset;
char *current_name;
{
	register long text_size, data_size, bss_size, t;
	char arcmag[SARMAG+1];

#if (MAGIC_OFFSET != 0L)
	if (fseek(infile, offset + MAGIC_OFFSET, 0) != 0)
	{
		error(BAD_SEEK, filename);
		return (0);
	}
#endif

	if (fread(&filhdr, sizeof (MAGIC), 1, infile) != 1)
	{
		error(BAD_READ, filename);
		return(0);
	}

	switch((int)filhdr.a_magic.file_type)
	{
	case EXEC_MAGIC:
	case SHARE_MAGIC:
	case RELOC_MAGIC:
#ifdef hp9000s200
        case DEMAND_MAGIC:
#endif
	case SHL_MAGIC:
	case DL_MAGIC:
                if (N_BADMACH(filhdr))
		{
			/* it is arguable whether we should ignore     */
			/* this error in an archive element - we won't */
			error("%s: bad system id 0x%x",
			      filename, filhdr.a_magic.system_id);
			return(0);
		}	

#if (MAGIC_OFFSET == 0L)
		if (fread(((char *)&filhdr) + sizeof (MAGIC),
			  (sizeof filhdr) - (sizeof (MAGIC)), 1, infile) != 1)
		{
			error(BAD_READ, filename);
			return(0);
		}
#else
		if (fseek(infile, offset, 0) != 0)
		{
			error(BAD_SEEK, filename);
			return (0);
		}

		if (fread((char *)&filhdr, sizeof filhdr, 1, infile) != 1)
		{
			error(BAD_READ, filename);
			return(0);
		}
#endif
		break;

	case AR_MAGIC:
		error("%s is in old archive format, use 'arcv'",filename);
		return (0);

	default:
	        fseek(infile, offset, 0);
	        fread(arcmag, SARMAG, 1, infile);
	        if (strncmp(arcmag, ARMAG, SARMAG) == 0)
	        {
		     /* If this is already a recursive call, fall through */
		     /* to default (error) case.        		     */
		     if (offset == 0L)
		     {
		         register long archive_offset;
		         struct ar_hdr archive_hdr;
			 register long arsize;
			 char null_terminated_name[AR_NAME_LEN];
        
                         archive_offset = MAGIC_OFFSET + SARMAG;
		         while (1)
			 {
		            if (fread(&archive_hdr, sizeof archive_hdr, 1,
					   infile) != 1)
				return(1);	/* assume end of file */

			    archive_offset += sizeof archive_hdr;
        
		            strncpy (null_terminated_name,
			             trim(archive_hdr.ar_name), AR_NAME_LEN);

			    /* ignore any entry which is too small */
			    arsize = atol(archive_hdr.ar_size);
			    if (arsize >= sizeof filhdr &&
			      	  !size (archive_offset, null_terminated_name))
				return(0);

			    /* offset always padded to even # of bytes */
	         	    archive_offset += arsize + (arsize & 01);
        
		 	    if (fseek(infile, archive_offset, 0) != 0)
			    {
			        error(BAD_SEEK, filename);
			        return (0);
		            }
		         }
			 /*NOTREACHED*/
		     }
                }

		if (offset == 0L)
		{
			error("%s: unrecognized magic number 0x%x",
			      filename, filhdr.a_magic.file_type);
			return(0);
		}
		else return(1);  /* ignore non-magic archive entry */
        }

#ifdef hp9000s200
	text_size = filhdr.a_text;
	data_size = filhdr.a_data;
	bss_size = filhdr.a_bss;
#endif

#ifdef hp9000s500
	if (!compute_s500_sizes (offset, &text_size, &data_size, &bss_size))
		return(0);
#endif

	t = text_size + data_size + bss_size;
	if (nflag || offset != 0L)
	{
		if (nflag && offset != 0L)
			printf("%s[%s]: ", filename, current_name);
			else printf("%s: ", current_name);
#ifdef MINUS_L_OPTION
		if (lflag)
			printf("\n");
#endif
	}
#ifdef MINUS_L_OPTION
	if (!lflag)
	{
#endif
		printf(output_format, text_size);
		printf(" + ");
		printf(output_format, data_size);
		printf(" + ");
		printf(output_format, bss_size);
		printf(" = ");
		printf(output_format, t);
		putchar('\n');
#ifdef MINUS_L_OPTION
	}
	else
	{
		printf("magic:\t0x%X\n", filhdr.a_magic.file_type);
#ifdef hp9000s500
		printf("entry:\t%03X %05X\n", 
			filhdr.a_entry.ent_segment,
			filhdr.a_entry.ent_stt >> 2);
#else
		printf("entry:\t");
		printf(output_format, filhdr.a_entry);
		putchar('\n');
#endif
		printf("tsize:\t");
		printf(output_format, text_size);
		putchar('\n');
		printf("dsize:\t");
		printf(output_format, data_size);
		putchar('\n');
		printf("bsize:\t");
		printf(output_format, bss_size);
		putchar('\n');
	}
#endif

	return(1);
}

#ifdef hp9000s500

/*************************************************************************
	compute_s500_sizes - compute total text, data, and bss (zero-pad)
			     sizes for all segments in s500 a.out file.
 *************************************************************************/

int	compute_s500_sizes (offset, text_size_ptr, data_size_ptr, bss_size_ptr)
long	offset, *text_size_ptr, *data_size_ptr, *bss_size_ptr;
{
	int		this_seg, num_segs;
	CODESEG_ENTRY	*cst;
	DATASEG_ENTRY	*dst;
	int		this_read, total_read;
	long		text_size = 0, data_size = 0, bss_size = 0;
	char		*sbrk();


	if (fseek (infile, offset + filhdr.a_cst.fs_offset, 0) != 0)
	{
	   error (BAD_SEEK, filename);
	   return (0);
	}
	
	cst = (CODESEG_ENTRY *) sbrk (filhdr.a_cst.fs_size);

	if ((int)cst == -1)
	   fatal (OUT_OF_MEMORY);

	for (total_read = 0;
	     total_read < filhdr.a_cst.fs_size;
	     total_read += this_read)
	{
	    this_read = fread ((char *)(cst) + total_read, 1,
			       filhdr.a_cst.fs_size - total_read, infile);
	    if (this_read <= 0)
	    {
		error (BAD_READ, filename);
		(void) sbrk (-filhdr.a_cst.fs_size);
		return (0);
	    }
	}

	num_segs = filhdr.a_cst.fs_size / sizeof (CODESEG_ENTRY);

	for (this_seg = 0; this_seg < num_segs; this_seg++)
	    text_size += cst[this_seg].seg_size;

	(void) sbrk (-filhdr.a_cst.fs_size);

	if (fseek (infile, offset + filhdr.a_dst.fs_offset, 0) != 0)
	{
	   error (BAD_SEEK, filename);
	   return (0);
	}
	
	dst = (DATASEG_ENTRY *) sbrk (filhdr.a_dst.fs_size);

	if ((int)dst == -1)
	   fatal (OUT_OF_MEMORY);

	for (total_read = 0;
	     total_read < filhdr.a_dst.fs_size;
	     total_read += this_read)
	{
	    this_read = fread ((char *)(dst) + total_read, 1,
			       filhdr.a_dst.fs_size - total_read, infile);
	    if (this_read <= 0)
	    {
		error (BAD_READ, filename);
		(void) sbrk (-filhdr.a_dst.fs_size);
		return (0);
	    }
	}

	num_segs = filhdr.a_dst.fs_size / sizeof (DATASEG_ENTRY);

	for (this_seg = 0; this_seg < num_segs; this_seg++)
	{
	    data_size += dst[this_seg].seg_size;
	    bss_size += dst[this_seg].seg_zero_pad;
	}

	(void) sbrk (-filhdr.a_dst.fs_size);

	*text_size_ptr = text_size;
	*data_size_ptr = data_size;
	*bss_size_ptr = bss_size;

	return (1);
}

#endif
