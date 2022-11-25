/* HP-UX_ID: @(#) $Revision: 70.2 $ */
static char Kleen_id[]="@(#) $Revision: 70.2 $";

#define    __hp9000s300   1

#include <stdio.h>
#include <a.out.h>
#include <magic.h>
#include <assert.h>
#if 0
/* This no longer is of any use, since it cannot recreate all the tables
 * which are needed by an object module to be inserted into a shared library.
 */
#define   COUNT_KLUDGE       /* For Counting Dynamic Relocation Entries */
#endif
#if 1
/* This option will allow relocation information to include the symbol
 * name on output.  This was done for fun once, and is here in case anyone
 * wishes to use it.
 */
#define   CAPR_KLUDGE        /* For -R support (symbolic relocation info) */
#endif
/* This define is for the newest object module format.  Since that
 * format is now the format that 8.0 will use, this will probably be
 * unifdef'ed true.
 */
#define   DMODULE            
#define   DBOUND
#define   OFFSET     1
#define   ABSOLUTE   2

#define   MONTH(a)   ((a) ? _month[(a)%12] : "Original")
char *_month[] = { "January", "February", "March", "April", "May", "June",
		   "July", "August", "September", "October", "November", "December" };
#define   YEAR(a)    ((a) ? (a)/12 + 1990 : 0)

FILE *in;
struct exec filhdr;
char * seg_names[] = { "TEXT", "DATA", "BSS", "EXTERN",
					   "PC", "DLT", "PLT" } ;
char * rec_sizes[] = { "BYTE", "WORD", "LONG", "ALIGN" } ;
long eptr;
int hflag;
int tflag;
int dflag;
int mflag;
int sflag;
int Dflag;
int rflag;
int aflag;
int xflag;
#ifdef   COUNT_KLUDGE
#define  COUNT_MAX_SYMBOLS  50000
int Xflag, dyn_count;
char reloc_sym[ COUNT_MAX_SYMBOLS ];
char dlt_required[ COUNT_MAX_SYMBOLS ], plt_required[ COUNT_MAX_SYMBOLS ];
#endif
#ifdef   CAPR_KLUDGE
#define  CAPR_MAX_SYMBOLS   50000
int Rflag;
int sym_num[ CAPR_MAX_SYMBOLS ], sym_count;
char *sym_str;
#endif

main(argc,argv) 
int argc;
char *argv[];
{
        procargs(argc,argv);
#ifdef   COUNT_KLUDGE
		if( Xflag )
			in = fopen(argv[argc-1],"r+");
		else
			in = fopen(argv[argc-1],"r");
#else
        in = fopen(argv[argc-1],"r");
#endif
        if (in == NULL) error('f',"Couldn't open %s for input",argv[argc-1]);

        if (fread(&filhdr,sizeof filhdr,1,in) != 1)
          error('w',"error reading header");
	filhdr.a_spared = 0;
	filhdr.a_spares = 0;

        if (hflag || aflag) header();
        if (tflag || aflag) text();
        if (dflag || aflag) data();
        if (mflag || aflag) modcal();
        if (sflag || aflag) symbols();
        if (Dflag || aflag) debug();
#ifdef  CAPR_KLUDGE
		if (Rflag && !aflag) 
		{
			symbols();       /* To get symbol names */
			rtext();         
			rdata();
		}
		else
#endif
        if (rflag || aflag) { rtext(); rdata(); }
		if (xflag || aflag) header_extensions();
#ifdef   COUNT_KLUDGE
		if (Xflag) 
		{
			rtext();
			rdata();
			filhdr.a_drelocs = dyn_count;

			if( fseek( in, 0L, 0 ) )
				error( 'w', "Can't SEEK" );
			if( fwrite( &filhdr, sizeof( filhdr ), 1, in ) != 1 )
				error( 'w', "error WRITING header" );
			printf( "\n****** Changed a_drelocs to %d ******\n\n", dyn_count );

			symbols();

			fclose( in );
		}
#endif
}

procargs(argc,argv)
int argc;
char **argv;
{
        int       n;

        /* parse command line */
        if (argc == 1) error('f', "Usage: diss [options] filename");
        if (argc == 2) { aflag++; return; }
        for (n = 1; n < argc - 1; n++)
        {
              switch (argv[n][1])
              {
                 case 'h': hflag ++ ; break;
                 case 't': tflag ++ ; break;
                 case 'd': dflag ++ ; break;
                 case 'm': mflag ++ ; break;
                 case 's': sflag ++ ; break;
                 case 'D': Dflag ++ ; break;
                 case 'r': rflag ++ ; break;
                 case 'a': aflag ++ ; break;
				 case 'x': xflag ++ ; break;
#ifdef  COUNT_KLUDGE
				 case 'X': Xflag ++ ; break;
#endif
#ifdef  CAPR_KLUDGE
				 case 'R': Rflag ++ ; break;
#endif
                 default : error('f', "invalid argument:%s ",argv[n]);
              }
        }
        return;
}

header()
{       
	char *str;

	printf( ">>>> Main Header (Offset 0)\n" );

	switch( filhdr.a_magic.system_id )
	{
	  case HP9000_ID:      str = "HP9000_ID";      break;
	  case HP98x6_ID:      str = "HP98x6_ID";      break;
	  case HP9000S200_ID:  str = "HP9000S200_ID";  break;
	  default:             str = "*** UNKNOWN SYSTEM ID ***"; break;
	}
    printf("System id: 0x%x (%s)\n",filhdr.a_magic.system_id, str);

	switch( filhdr.a_magic.file_type )
	{
	  case RELOC_MAGIC:  str = "RELOC_MAGIC";      break;
	  case EXEC_MAGIC:   str = "EXEC_MAGIC";       break;
	  case SHARE_MAGIC:  str = "SHARE_MAGIC";      break;
	  case AR_MAGIC:     str = "?? AR_MAGIC ??";   break;
	  case DEMAND_MAGIC: str = "DEMAND_MAGIC";     break;
	  case SHL_MAGIC:    str = "SHL_MAGIC";        break;
	  case DL_MAGIC:     str = "DL_MAGIC";         break;
	  default:           str = "*** UNKNOWN MAGIC NUMBER ***";  break;
	}
    printf("Magic Number: 0x%x (%s)\n",filhdr.a_magic.file_type, str);

    printf("Stamp: 0x%x\n",filhdr.a_stamp);
	printf("Highwater Mark: %hd (%s %d)\n", filhdr.a_highwater,
              MONTH(filhdr.a_highwater), YEAR(filhdr.a_highwater) );

	printf("Misc: 0x%X",filhdr.a_miscinfo); 
	if( filhdr.a_miscinfo & M_VALID )
	{
		if( filhdr.a_miscinfo & M_FP_SAVE )     printf( ", fp_save" );
		if( filhdr.a_miscinfo & M_INCOMPLETE )  printf( ", incomplete" );
		if( filhdr.a_miscinfo & M_DEBUG )       printf( ", debug" );
		if( filhdr.a_miscinfo & M_PIC )         printf( ", pic" );
		if( filhdr.a_miscinfo & M_DL )          printf( ", dl" );
	}
	printf( ".\n" );

    printf("Text Size: %ld bytes\n",filhdr.a_text);
    printf("Data Size: %ld bytes\n",filhdr.a_data);
    printf("BSS Size: %ld bytes\n",filhdr.a_bss);
    printf("Modcal Size: %ld bytes\n",filhdr.a_pasint);
    printf("Symbol Table Size: %ld bytes\n",filhdr.a_lesyms);
	printf("Symbol Table Supplementary Size: %ld bytes\n", filhdr.a_supsym );
    printf("Spare 'd': %ld bytes\n", filhdr.a_spared );
	printf("Spare 's': %ld bytes\n", filhdr.a_spares );
    printf("Text Relocation Size: %ld bytes\n",filhdr.a_trsize);
    printf("Data Relocation Size: %ld bytes\n",filhdr.a_drsize);
	printf("Dynamic Relocation Count: %ld\n", filhdr.a_drelocs);
    printf("Entry Location: %ld\n",filhdr.a_entry);
	printf("Extension Header: %ld\n",filhdr.a_extension);
}

text()
  {     long i,j,k,text_size;
        char l[16];
        printf("\n>>>> Text Segment\n");
        fseek(in, TEXTPOS, 0);

		text_size = filhdr.a_text;

        if (text_size%4 != 0) error('w',"text size not multiple of 4");
        k = text_size - 16;
        for (i=0; i<k; i+=16)
          {     if (fread(l, 16, 1, in) != 1)
                        error('w',"read error in text section");
                        printf("%6X: ",i);
                        for (j=0;j<16;j++) printf("%2x ",l[j]&0xff);
                        printf("\n");
          }
          k = text_size - i;
          if (k > 0) {
                  if (fread(l, k, 1, in) != 1)
                          error('w',"read error in text section");
                          printf("%6X: ",i);
                          for (j=0;j<k;j++) printf("%2x ",l[j]&0xff);
                          printf("\n");
          }
  }

data()
  {     long i,j,k;
        char l[16];
        printf("\n>>>> Data Segment\n");
        fseek(in, DATAPOS, 0);

        if (filhdr.a_data%4 != 0) error('w',"data size not multiple of 4");
        k = filhdr.a_data - 16;
        for (i=0; i<k; i+=16)
          {     if (fread(l, 16, 1, in) != 1)
                        error('w',"read error in data section");
                        printf("%6X: ",i);
                        for (j=0;j<16;j++) printf("%2x ",l[j]&0xff);
                        printf("\n");
          }
          k = filhdr.a_data-i;
          if (k > 0) {
                  if (fread(l, k, 1, in) != 1)
                          error('w',"read error in data section");
                          printf("%6X: ",i);
                          for (j=0;j<k;j++) printf("%2x ",l[j]&0xff);
                          printf("\n");
          }
  }

modcal()
{       long i,j,k;
        char l[16];
        printf("\n>>>> Modcal Section\n");
        fseek(in, MODCALPOS, 0);

        k = filhdr.a_pasint - 16;
        for (i=0; i<k; i+=16)
          {     if (fread(l, 16, 1, in) != 1)
                        error('w',"read error in modcal section");
                        for (j=0;j<16;j++) printf("%c",l[j]&0xff);
/*                        printf("\n"); */
          }
          k = filhdr.a_pasint-i;
          if (k > 0) {
                  if (fread(l, k, 1, in) != 1)
                          error('w',"read error in modcal section");
                          for (j=0;j<k;j++) printf("%c",l[j]&0xff);
/*                          printf("\n"); */
          }
  }

symbols()
  {     short count = 0;
        long i;
#ifdef  COUNT_KLUDGE
		long j;
#endif
        struct nlist_ sym;
        char s[SYMLENGTH+1];
        char type;
        char alch;
		char *malloc();
		struct supsym_entry *supptr, *suptable = NULL;
		
		if( filhdr.a_supsym > 0 && filhdr.a_spared == 0 )
		{
			suptable = (struct supsym_entry *) malloc( filhdr.a_supsym );
			fseek( in, SUPSYMPOS, 0 );
			if( fread( suptable, filhdr.a_supsym, 1, in ) != 1 )
			{
				error( 'w', "read error on supp symbol table" );
				free( suptable );
				suptable = NULL;
			}
		}
		supptr = suptable;

#ifdef  CAPR_KLUDGE
	if( !Rflag )
	{
#endif
        printf("\n>>>> Symbol Table\n");
		if( suptable != NULL )
			printf("          Value    Type  PIC    Size    Name\n");
		else
			printf("          Value    Type  PIC   Name\n" );
#ifdef  CAPR_KLUDGE
	 }
#endif
        fseek(in, LESYMPOS, 0);

#ifdef  CAPR_KLUDGE
		if( Rflag && (sym_str = (char *)malloc( filhdr.a_lesyms )) == NULL )
			error( 'f', "can't allocate memory for string table" );
#endif

        for (i=0; i < filhdr.a_lesyms; i+=sizeof sym)
          {   
#ifdef  COUNT_KLUDGE
			  j = ftell( in );
#endif
			  if (fread(&sym,sizeof sym,1,in) != 1)
                  error('w',"read error in symbol table section");
                if (fread(s, sym.n_length, 1, in) != 1)
                  error('w',"read error in string table section");
                s[sym.n_length] = '\0';
#ifdef  CAPR_KLUDGE
			  if( Rflag )
			  {
				  assert( sym_count < CAPR_MAX_SYMBOLS );
				  strcpy( sym_str + (sym_num[ sym_count++ ] = i), s );
			  }
#endif
                i+=sym.n_length;

                
                switch(sym.n_type & 017)
                {
                case UNDEF:     type = 'U'; break;
                case ABS:       type = 'A'; break;
                case TEXT:      type = 'T'; break;
                case DATA:      type = 'D'; break;
                case BSS:       type = 'B'; break;
                default:
               error('w', "unrecognized type 0%o on symbol %s", sym.n_type, s);
                break;
                }
                if ((sym.n_type & EXTERN) == 0)
                        type |= 040;
                if (sym.n_type & ALIGN)
                        alch = 'L';
                else    alch = ' ';
                if (sym.n_type & EXTERN2)
                        alch = 'S';
#ifdef   COUNT_KLUDGE
			  if( Xflag )
			  {
				char change = 0;

				if( reloc_sym[ count ] != sym.n_dreloc )
				{
					sym.n_dreloc = reloc_sym[ count ];
					change = 1;
				}
			    if( dlt_required[ count ] != sym.n_dlt )
				{
					sym.n_dlt = dlt_required[ count ];
					change = 1;
				}
			    if( plt_required[ count ] != sym.n_plt )
				{
					sym.n_plt = plt_required[ count ];
					change = 1;
				}
			    if( change )
				{
					if( fseek( in, j, 0 ) )
						error( 'f', "can't seek backward for symbol entry" );
					if( fwrite( &sym, sizeof sym, 1, in ) != 1 )
						error( 'f', "can't write symbol table entry" );
					if( fseek( in, (long)sym.n_length, 1 ) )
						error( 'f', "can't seek to same place" );
				}
			  }
#endif
#ifdef   CAPR_KLUDGE
			  if( !Rflag )
#endif
			  if( suptable != NULL )
                printf("%5d:  0x%08X  %c%c   %c%c%c   %5ld%c   %s\n",
					   count++, (long)sym.n_value,type, alch,
					   sym.n_plt ? 'P' : '.',
                       sym.n_dlt ? 'D' : '.',
					   sym.n_dreloc ? 'R' : '.',
					   suptable ? (supptr++)->a.size : -1,  
					   sym.n_list ? ' ': '*',
					   s);
			  else
                 printf("%5d:  0x%08X  %c%c   %c%c%c   %s\n",
					   count++, (long)sym.n_value,type, alch,
					   sym.n_plt ? 'P' : '.',
                       sym.n_dlt ? 'D' : '.',
					   sym.n_dreloc ? 'R' : '.',
					   s);
          }

		if( suptable != NULL )
			free( suptable );
  }

debug()
{
        if (Dflag)
        printf("\nDebug info not yet implemented in diss, use dsymtab\n");
        return;
}

rtext()
  {     long i;
#ifdef  CAPR_KLUDGE
        int print_flag = 0;
#endif
        struct r_info reloc;
        printf("\n>>>> Text Relocation Commands\n");
#ifdef  CAPR_KLUDGE
	if( Rflag )
		printf("Address  Symbolnum  Segment  Length  Symbol\n");
	else
#endif
        printf("Address  Symbolnum  Segment  Length\n");
        fseek(in, RTEXTPOS + filhdr.a_spared + filhdr.a_spares, 0);

        for (i=0; i<filhdr.a_trsize; i+=sizeof reloc)
          {     if (fread(&reloc,sizeof reloc,1,in) != 1)
                  error('w',"error reading text relocation commands");

                  printf("%7X ", reloc.r_address);
                  if (reloc.r_segment == REXT ||
					  reloc.r_segment == RPC  ||
					  reloc.r_segment == RPLT ||
					  reloc.r_segment == RDLT ) 
                  {
                     printf(" %6d    ", reloc.r_symbolnum);
#ifdef   CAPR_KLUDGE
                     print_flag = 1;
#endif
				  }
                  else
                     printf("    -*-    ");
                  if (reloc.r_segment == RNOOP)
                     printf(" %7s","NOOP");
                  else
                     printf(" %7s", seg_names[reloc.r_segment] );
#ifdef  COUNT_KLUDGE
				if( Xflag )
				{
					assert( reloc.r_symbolnum < COUNT_MAX_SYMBOLS );
					if( reloc.r_segment < 4 )
						dyn_count++;
					if( reloc.r_segment == REXT )
						reloc_sym[ reloc.r_symbolnum ] = 1;
					if( reloc.r_segment == RPLT )
						plt_required[ reloc.r_symbolnum ] = 1;
					if( reloc.r_segment == RDLT )
						dlt_required[ reloc.r_symbolnum ] = 1;
				}
#endif
#ifdef  CAPR_KLUDGE
				if( Rflag && print_flag )
				{
				  printf(" %6s   %s\n", rec_sizes[reloc.r_length], 
						               sym_str + sym_num[reloc.r_symbolnum] );
				  print_flag = 0;
			    }
				else
#endif
                  printf(" %6s\n", rec_sizes[reloc.r_length] );

          }
  }

rdata()
  {     long i;
#ifdef   CAPR_KLUDGE
		int print_flag = 0;
#endif

        struct r_info reloc;
        printf("\n>>>> Data Relocation Commands\n");
#ifdef  CAPR_KLUDGE
	if( Rflag )
		printf("Address  Symbolnum  Segment  Length  Symbol\n");
	else
#endif
        printf("Address  Symbolnum  Segment  Length\n");
        fseek(in, RDATAPOS + filhdr.a_spared + filhdr.a_spares, 0);

        for (i=0; i<filhdr.a_drsize; i+=sizeof reloc)
          {     if (fread(&reloc,sizeof reloc,1,in) != 1)
                  error('w',"error reading data relocation commands");
                
                  printf("%7X ", reloc.r_address);
                  if (reloc.r_segment == REXT ||
					  reloc.r_segment == RPC  ||
					  reloc.r_segment == RPLT ||
					  reloc.r_segment == RDLT )
				  {
                     printf(" %6d    ", reloc.r_symbolnum);
#ifdef  CAPR_KLUDGE
					 print_flag = 1;
#endif
				  }
                  else
                     printf("    -*-    " );
                  if (reloc.r_segment == RNOOP)
                     printf(" %7s","NOOP");
                  else
                     printf(" %7s", seg_names[reloc.r_segment] );
#ifdef  COUNT_KLUDGE
				if( Xflag )
				{
					assert( reloc.r_symbolnum < COUNT_MAX_SYMBOLS );
					if( reloc.r_segment < 4 )
						dyn_count++;
					if( reloc.r_segment == REXT )
						reloc_sym[ reloc.r_symbolnum ] = 1;
					if( reloc.r_segment == RPLT )
						plt_required[ reloc.r_symbolnum ] = 1;
					if( reloc.r_segment == RDLT )
						dlt_required[ reloc.r_symbolnum ] = 1;
				}
#endif
#ifdef  CAPR_KLUDGE
				if( Rflag && print_flag )
				{
				  printf(" %6s   %s\n", rec_sizes[reloc.r_length], 
						               sym_str + sym_num[reloc.r_symbolnum] );
				  print_flag = 0;
			    }
				else
#endif
                  printf(" %6s\n", rec_sizes[reloc.r_length] );

          }
  }

header_extensions()
{
  struct header_extension ehdr;
  char bad = 0;
  long offset = 0;

  if( filhdr.a_magic.file_type == SHL_MAGIC ||
	  filhdr.a_magic.file_type == DL_MAGIC )
  {
	  offset = filhdr.a_entry;
  }

  for( eptr = filhdr.a_extension; eptr && !bad; eptr = ehdr.e_extension )
  {
	printf( "\n>>>> Extension Header (Offset %ld) --\n", eptr );
	fseek( in, eptr, 0 );
	if( fread( &ehdr, sizeof( struct header_extension ), 1, in ) != 1 )
		error( 'w', "read error on extension header" );
	printf( "Extension Magic Number: 0x%hx ", ehdr.e_header );
	switch( ehdr.e_header )
	{
	  case DL_HEADER:  printf( "(Dynamic Loader Information)\n" ); break;
	  case DEBUG_HEADER: printf( "(Debug Information)\n" ); break;
	  default:         printf( "(*** Unknown Type ***)\n" ); break;
	}
	printf( "Version: %hd\n", ehdr.e_version );
	printf( "Aggregate Size: %ld\n", ehdr.e_size );
	
	switch( ehdr.e_header )
	{
	  case DL_HEADER:
	    {
			struct _dl_header *dl;
			struct dynamic dy;
			long size, dlt_size, plt_size, global_dlt;
			char *string, *malloc(), *str1;
			typedef struct dlt_entry DLT;
			typedef struct plt_entry PLT;
			DLT *dlt;
			PLT *plt;
			long i;
			
			dl = &(ehdr.e_spec.dl_header);

			printf( "-- Dynamic Loader Information:\n" );
			printf( "Zero Field: %ld\n", dl->zero );
			if( dl->zero != 0 )
				error( 'w', "dl_header 'zero' != 0" );
			printf( "Offset of 'struct dynamic': %ld\n", dl->dynamic );
            printf( "Offset of Shared Library List: %ld\n", dl->shlt );
			printf( "Offset of Import List: %ld\n", dl->import );
			printf( "Offset of Export List: %ld\n", dl->export );
			printf( "Offset of Shlib Export List: %ld\n", dl->shl_export );
			printf( "Offset of Export Hash Table: %ld\n", dl->hash );
			printf( "Offset of String Table: %ld\n", dl->string );
			printf( "Offset of Module Import Lists: %ld\n", dl->module );
#ifdef	DBOUND
			printf("Offset of Dmodule Table: %ld\n",dl->dmodule);
#endif
			printf( "Offset of Dynamic Relocation Records: %ld\n", 
				                                            dl->dreloc );

			/* Do 'struct dynamic' information */
			fseek( in, PTR( dl->dynamic, OFFSET ), 0 );
			if( fread( &dy, sizeof( struct dynamic ), 1, in ) != 1 )
				error( 'w', "Trouble reading 'struct dynamic'" );
			printf( "-- Struct Dynamic Information (Address %ld):\n", 
				                                            dl->dynamic );
			printf( "Offset of Text Section: %ld\n", dy.text );
			printf( "Offset of Data Section: %ld\n", dy.data );
			printf( "Offset of BSS Section: %ld\n", dy.bss );
#ifdef  DMODULE
			printf( "Offset of DModule Entries: %ld\n", dy.dmodule );
#endif
			printf( "Offset of DLT Entries: %ld\n", dy.dlt );
			printf( "Offset of Anonymous DLT Entries: %ld\n", dy.a_dlt );
			printf( "Offset of PLT Entries: %ld\n", dy.plt );
#ifdef  DMODULE
			printf( "Offset of Bound Array: %ld\n", dy.bound );
#endif
			switch( dy.bor.opcode )
			{
			  case BRA: str1 = "bra"; break;
			  case BSR: str1 = "BSR"; break;
			  default:  str1 = "unkown"; break;
			}
	printf( "BOR: Opcode = 0x%02x (%s), Size = 0x%02x, Offset = 0x%08lx\n",
				   dy.bor.opcode, str1, dy.bor.size, dy.bor.displacement );
			printf( "Status word: %hd\n", dy.status );
			printf( "Elaborator: 0x%08X\n", dy.elaborator );
			printf( "Initializer: 0x%08X\n", dy.initializer );
			printf("Dld flags: 0x%08X\n",dy.dld_flags);
			printf("Dld hook: 0x%08X\n",dy.dld_hook);
			printf("Dld list: 0x%08X\n",dy.dld_list);
			printf("Shlib path: 0x%08X\n",dy.shlib_path);
			if( ((long)dy.dl_header) + TEXTPOS != eptr )
				printf( "**** (struct dynamic).dl_header != dl_header\n" );
		
		    /* Read the string table into memory */
			size = dl->module - dl->string;
		    if( (string = malloc( size * sizeof( char ))) == NULL )
			   error( 'w', "Unable to malloc() dl string table" );
			fseek( in, dl->string + eptr, 0 );
		    if( fread( string, sizeof( char ), size, in ) != size )
			   error( 'w', "Trouble reading DL string table" );

			printf("Embedded path: <%s>\n",dy.embed_path?string+dy.embed_path:"");

			/* Read the DLT entries into memory */
			dlt_size = ((long)dy.plt - (long)dy.dlt) / sizeof( DLT );
			global_dlt = ((long)dy.a_dlt - (long)dy.dlt) / sizeof( DLT );
			if( (dlt = (DLT *) malloc( dlt_size * sizeof( DLT ) )) == NULL )
				error( 'w', "Unable to malloc() dlt table" );
			fseek( in, PTR(dy.dlt, ABSOLUTE), 0 );
			if( fread( dlt, sizeof( DLT ), dlt_size, in ) != dlt_size )
				error( 'w', "Trouble reading dlt table" );
			
			/* Read the PLT entries into memory */
			plt_size = (PTR(dl->dynamic,OFFSET) - PTR((long)dy.plt,ABSOLUTE))
				       / sizeof( PLT );
			if( (plt = (PLT *) malloc( plt_size * sizeof( PLT ) )) == NULL )
				error( 'w', "Unable to malloc plt table" );
			fseek( in, PTR(dy.plt, ABSOLUTE), 0 );
			if( fread( plt, sizeof( PLT ), plt_size, in ) != plt_size )
				error( 'w', "Trouble reading plt table" );

			/* Interpret the Tables */
			shared_library_list( string, dl->shlt, dl->import );
			import_list( string, dl->import, dl->export,
						 dlt, dlt_size, global_dlt, plt, plt_size );
#ifdef	DBOUND
			module_list( string, dl->module, dl->dmodule );
#else
			module_list( string, dl->module, dl->dreloc );
#endif
#ifdef  DMODULE
#ifdef	DBOUND
			if (dl->dreloc > dl->dmodule)
				dmodule_list(dl->dmodule);
#else
			dmodule_list( dy.dmodule );
#endif
#endif
			export_list( string, dl->export, dl->shl_export, dl->hash );
			export_hash_table( dl->hash, dl->string );
			dynamic_relocation( dl->dreloc ); 

			free( string );
			free( dlt );
			free( plt );
		}
		break;
	  case DEBUG_HEADER:
		printf("'diss' currently has no support for Debug Headers.\n" );
		printf("Please try something like 'dsymtab'.\n" );
		break;
	  default:
		printf("-- Unkown Extension Header!\n" );
		bad = 1;
		break;
	}
  }
}

shared_library_list( string, start, end )
char *string;
long start, end;
{
	struct shl_entry sl;
	int k, i = 0;

	fseek( in, start + eptr, 0 );

	printf( "\n>> Shared Library List\n" );
	if( end != start )
		printf( "Index  Binding    Finder           HiHOH   Library\n" );

	for( k = (end-start) / sizeof(struct shl_entry); k; k-- )
	{
		printf( "%5d  ", i++ );
		fread( &sl, sizeof( struct shl_entry ), 1, in );
		if (sl.bind & BIND_IMMEDIATE)
			printf( "Immediate%c ",(sl.bind&BIND_NONFATAL)?'N':' ');
		else if (sl.bind & BIND_DEFERRED)
			printf( "Deferred   " );
		else
			printf( "**** Unknown Type of SHL Binding, " );
		switch( sl.load )
		{
		  case LOAD_LIB:		printf( "'-l' Specified   " );		break;
		  case LOAD_PATH:		printf( "Path Name Given  " );		break;
		  case LOAD_IMPLICIT:	printf( "Load Implicit    " );		break;
		  default:
			printf( "**** Unknown form of Library Specification, " );
			break;
		}
		printf( "%5d   %s\n", sl.highwater, string + sl.name );
	}
}

import_list( string, start, end, dlt, dlt_size, global_dlt, plt, plt_size )
char *string;
long start, end, dlt_size, plt_size, global_dlt;
struct dlt_entry *dlt;
struct plt_entry *plt;
{
	struct import_entry ie;
	int k, count = 0, cnt;
	char *str1, *str2;
	char *type_str();

	fseek( in, start + eptr, 0 );
	
	printf( "\n>> Import List\n" );
	
	cnt = ((end-start) / sizeof(struct import_entry)) - global_dlt - plt_size;

	if( dlt_size )
	{
	   printf("Index  Library  Type  DLT:Address                    Symbol\n");
		for( k = 0; dlt_size && k < global_dlt; dlt_size--, k++ )
		{
			if( fread( &ie, sizeof( struct import_entry ), 1, in ) != 1 )
				error( 'w', "Can't read import list (A)" );
			if (ie.type == TYPE_INTRA)
				printf( "%5d  %7d  %s   0x%08x                    <Ox%08X>\n",count++,ie.shl,type_str(ie.type),dlt[k].address,ie.name);
			else
			{
				printf( "%5d  %7d  %s   0x%08x                    %s\n", 
				   count++, ie.shl, 
				   type_str(ie.type), dlt[ k ].address, string + ie.name );
			}
		}
		for( ; dlt_size; dlt_size--, k++ )
		{
			printf( "        anonymous dlt  0x%08x\n", dlt[ k ].address );
		}
	}
	if( plt_size )
	{
	   printf("Index  Library  Type   PLT:Offset  PLT:Op  PLT:Size  Symbol\n");
		for( k = 0; plt_size; plt_size--, k++ )
		{
			if( end != start )
				if( fread( &ie, sizeof( struct import_entry ), 1, in ) != 1 )
					error( 'w', "Can't read import list (B)" );
			switch( plt[ k ].opcode )
			{
			  case BRA:  str1 = "bra";   break;
			  case BSR:  str1 = "bsr";   break;
			  default:   str1 = "*ERR*"; break;
			}
			switch( plt[ k ].size )
			{
			  case LONG: str2 = "long";  break;
			  default:   str2 = "*ERR*"; break;
			}
			if( end != start )
			{
				if (ie.type == TYPE_INTRA)
					printf( "%5d  %7d  %s   0x%08x   %s     %s     <0x%08X>\n", count++, ie.shl, type_str(ie.type), plt[ k ].displacement, str1, str2, ie.name );
				else
					printf( "%5d  %7d  %s   0x%08x   %s     %s     %s\n", count++, ie.shl, type_str(ie.type), plt[ k ].displacement, str1, str2, string + ie.name );
			}
			else
				printf( "        anonymous plt  0x%08x  %s     %s\n",
					   plt[k].displacement, str1, str2 );
		}
	}
	if( cnt > 0 )
	{
	   printf("Index  Library  Type                                 Symbol\n");
		for( k = cnt; k; k-- )
		{
			fread( &ie, sizeof( struct import_entry ), 1, in );
			if (ie.type == TYPE_INTRA)
				printf( "%5d  %7d  %s                                 <0x%08X>\n", count++, ie.shl, type_str(ie.type), ie.name);
			else
				printf( "%5d  %7d  %s                                 %s\n",
				   count++, ie.shl, 
				   type_str(ie.type), string+ie.name);
		}
	}
}

module_list( string, start, end )
char *string;
long start, end;
{
	struct module_entry me;
	int k, j = 0, flag = 1;

	fseek( in, start + eptr, 0 );

	printf( "\n>> Module Import Lists" );

	for( k = (end - start) / sizeof( struct module_entry ); k; k--, j++ )
	{
		if( flag )
		{
			printf( "\nModule %5d:", j );
			flag = 0;
		}
		fread( &me, sizeof( struct module_entry ), 1, in );
		printf( " %4d%c", me.import & MODULE_IMPORT_MASK,
			              (me.import & MODULE_DLT_FLAG ? '*' : ' ') );
		if( me.import & MODULE_END_FLAG )
			flag = 1;
	}

	putchar( '\n' );
}

#ifdef  DMODULE
dmodule_list( start )
long start;
{
	struct dmodule_entry dm;
	int i = 0;

	printf( "\n>> Dmodule List\n" );
	printf( "Index   Imports   Dreloc    A_Dlt     Flags\n" );
	
	fseek( in, PTR(start,ABSOLUTE), 0 );

	do
	{
		fread( &dm, sizeof( struct dmodule_entry ), 1, in ); 
		printf( "%5d  %8d   %6d   %6d   %7d\n", i++, 
			    dm.module_imports, dm.dreloc, dm.a_dlt, dm.flags );
	}
	while( dm.flags != DM_END );
}
#endif
	
export_list( string, start, extra, end )
char *string;
long start, extra, end;
{
	struct export_entry ee;
	struct shl_export_entry *see_table, *see;
	int k, i = 0;
	char *type_str();
	
	printf( "\n>> Export List\n" );

	if( extra != end )
	{
		see = see_table = (struct shl_export_entry *) malloc( end - extra );
		assert( see_table != NULL );
		fseek( in, eptr + extra, 0 );
		fread( see_table, end - extra, 1, in );
	}

	fseek( in, start + eptr, 0 );
	
	k = (extra - start) / sizeof( struct export_entry );
	if( k != 0 )
	{
		printf( "Index Next  TypeS  Offset     HiHOH  Size" );

		if( extra != end )
			printf( "  Drel SyNxt Imprt ExNxt  Name\n" );
		else
			printf( "  Name\n" );

        while( k-- )
		{
			fread( &ee, sizeof( struct export_entry ), 1, in );
			printf( "%5d %4d  %s%c  0x%08x %5d %5d ", 
				    i++, ee.next_export, type_str(ee.type),
				    ee.type & TYPE_EXTERN2 ? 'S' : '.', 
				    ee.value, ee.highwater, ee.size );
			if( extra != end )
			{
				printf( "%5d %5d %5d %5d  %s\n", see->dreloc, see->next_symbol,
					    see->dmodule, see->next_module, 
					    string + ee.name );
				see++;
			}
			else
				printf( " %s\n", string + ee.name );
		}
	}

	if( extra != end )
		free( see_table );
}

char *type_str( type )
short type;
{
	switch( type & TYPE_MASK )
	{
	  case TYPE_UNDEFINED:     return( "udef" );   break;
	  case TYPE_DATA:          return( "data" );   break;
	  case TYPE_PROCEDURE:     return( "proc" );   break;
	  case TYPE_COMMON:        return( "comm" );   break;
      case TYPE_BSS:           return( "bss " );   break;
	  case TYPE_CDATA:         return( "cdat" );   break;
	  case TYPE_ABSOLUTE:      return( "abs " );   break;
	  case TYPE_SHL_PROC:      return( "Sprc" );   break;
	  case TYPE_INTRA:         return( "Intr" );   break;
	  default:                 return( "*ERR" );   break;
	}
}

export_hash_table( start, end )
long start, end;
{
	struct hash_entry he;
	int k, count;

	fseek( in, start + eptr, 0 );
	
	printf( "\n>> Export Hash Table\n" );

	for( count = 0, k = (end - start) / sizeof( struct hash_entry ); k; k-- )
	{
		if( fread( &he, sizeof( struct hash_entry ), 1, in ) != 1 )
			error( 'w', "Unable to read Export Hash Table" );
		printf( "%8d: %8d\n", count++, he.symbol );
	}
}

dynamic_relocation( start )
long start;
{
	struct relocation_entry re;
	int flag = 0;
	int index = 0;

	fseek( in, start + eptr, 0 );

	printf( "\n>> Dynamic Relocation Information\n" );

	while( 1 )
	{
		if( fread( &re, sizeof( struct relocation_entry ), 1, in ) != 1 )
			error( 'w', "Unable to read Dynamic Relocation Entry" );

		/* Watch for the end marker */
		if( re.type == DR_END )
			break;
		else if( !flag )
		{
			printf( "Index  Address     Importnum   Type        Length\n" );
			flag = 1;
		}
		
		printf( "%5d  0x%08x       %4d   ", index++, re.address, re.symbol );
		switch( re.type )
		{
		  /* Should never see this case: */
		  case DR_END:	        printf( "** end **     " );    break;
		  case DR_NOOP:		printf( "  noop        " );    break;
		  case DR_EXT:	        printf( "  extern      " );    break;
		  case DR_FILE:	        printf( "  file        " );    break;
		  case DR_PROPAGATE:    printf( "propagate     " );    break;
		  case DR_COPY:         printf( "  copy        " );    break;
		  case DR_INVOKE:       printf( "  invoke      " );    break;
		  default:
			printf( "**** Unkown file relocation type " );
			break;
		}
		switch( re.length )
		{
		  case 0:         printf( "    " );   break;
		  case DR_BYTE:   printf( "byte" );   break;
		  case DR_WORD:   printf( "word" );   break;
		  case DR_LONG:   printf( "long" );   break;
		  default:
			printf( "**** Unkown relocation length" );
			break;
		}
		putchar( '\n' );
	}
}

long PTR(off, flag)
long off;
int flag;
{
	switch( filhdr.a_magic.file_type )
	{
	  case SHARE_MAGIC:
		off = (off - EXEC_ALIGN( filhdr.a_text )) + filhdr.a_text + TEXTPOS;
		break;
	  case DEMAND_MAGIC:
		off += TEXTPOS;
		break;
	  case SHL_MAGIC:
	  case DL_MAGIC:
		if( flag == ABSOLUTE )
			off = (off - filhdr.a_entry) + TEXTPOS;
		else
			off = off + TEXTPOS;
		break;
	  case RELOC_MAGIC:
	  case EXEC_MAGIC:
		printf( "reloc and exec not supported for dl information\n" );
		break;
	  default:
		printf( "help!" );
		break;
	}

	return( off );
}


/* VARAGRS 2 */
error(t,s,a,b,c,d,e,f,g,h,i,j) char t; char *s;
  {     fprintf(stderr,"diss: ");
        fprintf(stderr,s,a,b,c,d,e,f,g,h,i,j);
        fprintf(stderr,"\n");
        switch (t)
          { case 'w':   return;
            case 'f':   exit(1);
            case 'a':   abort();
            default:    error('w',"Illegal error type: '%c'",t);
          }
  }


