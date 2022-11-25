/* Internal RCS $Header: funcs.h,v 70.1 91/11/13 14:42:58 ssa Exp $ */

/* deb.c */
int fix_slts ( int sloc );
int fix_dntts ( long sloc , long globals );

/* emes.c */

/* hpux_rel.c */

/* main.c */
int main ( int ac , char **av );
void delexit ( int );
int procargs ( void );
int get_next_arg ( int flag );
int newarg ( char *name );
int newhide ( char *name );
int newundef ( char *name );
int newlibdir ( char *name );
int plus_procflags ( char *a );
int minus_procflags ( char *a );
long atox ( char *s );
#if   CHKMEM
char *ld_realloc ( void *a , long b , char *f , long l );
char *ld_calloc ( long a , long b , char *f , long l );
char *ld_malloc ( long a , char *f , long l );
#else
char *ld_realloc ( void *a, long b );
char *ld_calloc ( long a, long b );
char *ld_malloc ( long a );
#endif

/* middle.c */
int middle ( void );
int common ( void );
int sym2 ( struct symbol *sp );
int ldrsym ( int asp , long val , int type );
int findal1 ( unsigned int aligntype );
int adjust_sizes ( int final );
int setupout ( void );
#ifdef	FTN_IO
int ftn_io (char *);
#endif

/* pass1.c */
int load1arg ( char *cp , int libtype , int bind );
char *trim ( char *s );
int step ( void );
int ldrand ( void );
int load1 ( int sloc , int libflg );
int sized_data ( struct symbol *sp , long size , long offset );
int sym1 ( void );
#ifdef	MISC_OPTS
int check_ylist (const char * name, int type, int value);
#endif

/* pass2.c */
int load2arg ( char *cp , int libtype );
int load2 ( long sloc );
int load2td ( FILE *outf , FILE *outrf , long txtstart , long txtsize , long rsize , struct r_info *relptr , long start_pos );
long relext ( struct r_info *r );
long relcmd ( long current , long position , int size , long offs , FILE *outf , unsigned int stype , int no_align_inc );
int align_inc ( unsigned long value , unsigned int stype );
int finishout ( void );

/* shlib.c */
char *stralloc ( int bytes );
int DltEnter ( struct symbol *sp , int seg , long val , int highwater );
int PltEnter ( struct symbol *sp , int shl );
int DltSet ( int dlti , int val , int seg );
int DltReloc ( void );
int scan_shlib_imports ( void );
int AddShlib ( char *name , int minus_l , int bind );
int enter_shlib_symbol ( long i , long slot );
int enter_other_shlib_names ( struct export_entry *ee , struct shl_export_entry *see , struct symbol *sp , unsigned flags1 , unsigned flags2, int linked );
int fuzzy_define ( struct export_entry *ee , struct shl_export_entry *see, long slot );
int real_define ( struct export_entry *ee , struct shl_export_entry *see, long slot );
long foffset( long off );
int FillInShlibStuff ( void );
int WriteShlibText ( void );
int rewrite_exec_header ( void );
int WriteDynReloc ( void );
int WriteShlibData ( void );
int shlib_tables ( void );
int write_fuzzy_data ( void );
int enter_string_table( struct symbol *sp );

/* sym.c */
int getfile ( char *cp , int libtype );
int lookup ( void );
int slookup ( char *s );
int enter ( int slot );
int expand ( void );
int symreloc ( void );
int readhdr ( long pos );
int readdebugexthdr ( long pos );
int error ();
int fatal ();
int bletch ();
int dcopy ( FILE *in , FILE *out , long count );
int zout ( FILE *stream , int count );
int dread ( void *pos , int size , int count , FILE *file );
int acopy ( char *sp );
char *asciz ( char *sp , int l );
int make_prime ( int num );
int dump_sym_tab ( void );
int dump_sym ( struct symbol *p );
char *sym_stralloc( int size );

/* export.c */
int ExpLookup ( char *namep , int highwater , struct export_entry **sympp , char *stringt , struct export_entry *expt , struct hash_entry *hasht , int hashsize );
int ExpEnter ( struct symbol *sp , int seg , long val , int highwater , long size , long next_symbol , long mod_imports , long next_module );
int ExpHash ( void );
int hash ( char *sym , int buckets );
int hash ( char *sym , int buckets );
int ExpSet ( int expi , int val , int size , int seg );
int ExpReloc ( void );
