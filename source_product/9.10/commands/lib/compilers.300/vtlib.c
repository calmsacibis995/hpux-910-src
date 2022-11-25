/* file vtlib.c */
/*    SCCS    REV(64.3);       DATE(92/04/03        14:22:39) */
/* KLEENIX_ID @(#)vtlib.c	64.3 91/12/17 */
/* -*-C-*-
********************************************************************************
*
* File:         vtlib.c
* RCS:          $Header: vtlib.c,v 70.3 92/04/03 14:19:19 ssa Exp $
* Description:  Library routines to create VT table -- used by compilers
* Author:       Jim Wichelman, SES
* Created:      Fri Jun 16 12:06:33 1989
* Modified:     Fri Sep  1 17:15:51 1989 (Jim Wichelman) jww@hpfcjww
* Language:     C
* Package:      N/A
* Status:       Experimental (Do Not Distribute)
*
* (c) Copyright 1989, Hewlett-Packard Company, all rights reserved.
*
********************************************************************************
*/
static char rcs_identity[] = "$Header: vtlib.c,v 70.3 92/04/03 14:19:19 ssa Exp $";

/* #define DEBUG */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#ifndef DOMAIN
# include <unistd.h>
#endif

/***************/
/* Local types */
/**************/

typedef struct vt_node_s
{
    long		vt_index;
    struct vt_node_s	*next;
} vt_node_t, *vt_node;

#define MAX_NODES_PER_BLOCK 200

typedef struct vt_node_block_s
{
    struct VT_NODE_BLOCK	*next_block;
    long			node_cnt;
    vt_node_t			nodes[MAX_NODES_PER_BLOCK];
} vt_node_block_t, *vt_node_block;

#define VT_HASH_MAX  809
/* 401 809 1619 3251 6521 13043 */

#define TMPDIR "/tmp"
#define VTNAME "asvt"

#define MAX_STRING_BUFF_SIZE 256

/*****************************/
/* Global (static) variables */
/*****************************/

static int		Initialized      = 0;
static FILE 		*Fd_vt           = NULL;
static char		*Fname           = NULL;
static int		(*Err_proc)()    = NULL;
static vt_node_block_t	First_node_block = {NULL, 0};
static vt_node_block    Node_block_list  = &First_node_block;

static vt_node		Vt_hash_table[VT_HASH_MAX];
static unsigned long    Vt_file_pos = 0;
    
#ifdef DEBUG
static int		Collisions = 0;
#endif

/*****************************************************************************\
* 
* void remove_vt_tempfile()
* 
*    This routine closes any open VT tempfile(s).  It is intended to be used
*    by the error cleanup routines to destroy any tempfiles before exiting.
* 
*    Do not call this after a successful compilation, as the assembler will
*    need this file!
*    
\*****************************************************************************/

void remove_vt_tempfile()
{
    if (Fd_vt) 
    {
	fclose(Fd_vt);
	unlink(Fname);
    }
}


/*****************************************************************************\
* 
*  void register_vt_error_callback( error_routine )
*
*   This routine should be called before any other of these routines.
*   It is used to register an error routine to be called if we encounter
*   some fatal error in these library routines.
*
*   The error routine should like like:
*
*       int error_routine( err_msg )
*	   char  *err_msg;
*   The "err_msg" will be a string describing the problem.  You will
*   probably want to prefix it with your tool name (e.g. "cc: <err msg>")
*   The return value is not looked at.
*
*   If we should return from the "error_routine", an exit(1) will be
*   performed.
*
\*****************************************************************************/

void register_vt_error_callback( error_routine )
	int	(*error_routine)();
{
    Err_proc = error_routine;
}


/*****************************************************************************\
* 
*      static void vt_error( s )
*
\*****************************************************************************/

static void vt_error( s )
    char	*s;
{
    /* If user registered an error routine, call it. */
    /* --------------------------------------------- */
    if (Err_proc) 
	Err_proc( s );
    else
	/* No User error routine.  Just print our own msg */
	/* ---------------------------------------------- */
    {
#ifdef FORT
	fprintf(stderr, "vtlib: Fatal error: %s\n", s);
#else /* not FORT */
	stderr_fprntf( "vtlib: Fatal error: %s\n", s);
#endif /* not FORT */
	remove_vt_tempfile();
    }
    
    exit(1);
}


/*****************************************************************************\
* 
*      static void open_vt_file()
*
\*****************************************************************************/

static void open_vt_file()
{
    extern char *tempnam();
    
    Fname = tempnam(TMPDIR, VTNAME);
    
    if ((Fd_vt = fopen(Fname, "a+")) == NULL)
	vt_error("Unable to open temporary (vt) file");

    /* First byte in VT must be zero.  This is so 0 vt */
    /* ptrs have string length of 0.		       */
    /* ----------------------------------------------- */
    fputc(0, Fd_vt);
    Vt_file_pos = 1;
    
#ifdef IRIF
    ir_vt( "" );
#endif /* IRIF */

    /* NOTE: The fflush here is important, see comments */
    /* in add_to_vt                                     */

    fflush(Fd_vt);
}

/*****************************************************************************\
* 
*      static void init_vt_hash_table()
*
\*****************************************************************************/

static void init_vt_hash_table()
{
    int 	i;
    
    for (i = 0; i < VT_HASH_MAX; i++) Vt_hash_table[i] = NULL;
	
}


/*****************************************************************************\
* 
*      static void initialize_vt_stuff()
*
\*****************************************************************************/

static void initialize_vt_stuff()
{
    open_vt_file();
    init_vt_hash_table();
    Initialized = 1;
}

/*****************************************************************************\
* 
*      static int hash_vt(s)
*
\*****************************************************************************/

# define MAX_CHARS_TO_HASH 64

static int hash_vt(s)
    char *s;
{
    register int i;
    register int hash_value = 0;
    
    for (i = 0; (*s) && (i < MAX_CHARS_TO_HASH); s++, i++)
	hash_value += (*s) << 2;

    return( hash_value % VT_HASH_MAX);
}

/*****************************************************************************\
* 
*  static vt_node new_vt_hash_node(index)
* 
\*****************************************************************************/

static vt_node new_vt_hash_node(index)
    int	index;
{
    vt_node		hash_p;
    vt_node_block	new_block;
    

    if (Node_block_list->node_cnt >= MAX_NODES_PER_BLOCK)
    {
	new_block = (vt_node_block) malloc(sizeof(vt_node_block_t));
	if (!new_block)
	    vt_error("can't malloc space for vt hash table");
	
	new_block->node_cnt = 0;
	new_block->next_block = (struct VT_NODE_BLOCK *)Node_block_list;
	Node_block_list = new_block;
    }
    
#ifdef DEBUG
    if (Vt_hash_table[index]) Collisions++;
#endif

    hash_p = &(Node_block_list->nodes[Node_block_list->node_cnt++]);
    hash_p->next = Vt_hash_table[index];
    Vt_hash_table[index] = hash_p;
    
    return hash_p;
}

/*****************************************************************************\
* 
*  static long write_vt_string(s, len, add_null)
* 
\*****************************************************************************/

static long write_vt_string(s, len, add_null)
    char *s;
    int  len;
    int	 add_null;
    
{
    unsigned long  start_pos = Vt_file_pos;
    
    if (fputs(s, Fd_vt) == EOF)
	vt_error("error writing vt string to vt temp file");
    else
	Vt_file_pos += len;
        
    if (add_null)
    {
	if (fputc(0, Fd_vt) == EOF)
	    vt_error("error writing a NULL byte to vt temp file");
        else
	    Vt_file_pos++;
    }
        
    return (start_pos);
}


/*****************************************************************************\
* 
*  static int same_vt_string(s, node, did_read)
* 
\*****************************************************************************/

static int same_vt_string(s, node, did_read)
    char	*s;
    vt_node	node;
    int		*did_read;
    
{
    static char   s_buffer[MAX_STRING_BUFF_SIZE] = {0};
    static int    last_index = 0;
    int		  i = 0;
    int		  c;
        
    if (last_index != node->vt_index)
    {
	last_index = node->vt_index;

	if (fseek(Fd_vt, node->vt_index, 0) != 0)
	    vt_error("error doing fseek in vt file to compare string");

	while ( 1 )
	{
	    c = getc( Fd_vt );
	    if (c == EOF )
		vt_error("Unexpected EOF doing getc in vt file for same_vt_string");

	    s_buffer[i++] = c;

	    if (c == NULL) break;

	    if (i >= MAX_STRING_BUFF_SIZE)
		vt_error("string too long doing getc in vt file for same_vt_string");
	}

        *did_read = 1;
    }
    
    return (strcmp(s, s_buffer) == 0);
}

/*****************************************************************************\
*
* long add_to_vt( s, check_duplicates, add_null )
* 	char	*s;
* 	int	check_duplicates;
* 	int	add_null;
* 
* This function will add a string to the VT table and return the            
* index (byte offset) of the string within the table.                       
*                                                                           
* Note: there is no initialization routine as initialization is             
* automatically performed the first time this routine is called.            
*                                                                           
* s                -  This is the string to be added to the VT table.       
*                                                                           
* check_duplicates -  A non-zero value will cause the routine to            
*                     check and see if the string is already present in     
*                     the VT.  If so, the index of the already existing     
*                     string will be return.                                
*                                                                           
*                     A zero will cause the string to be added without      
*                     checking for duplicates.                              
*                                                                           
*                     If the length of "s" is greater than 300 characters,  
*                     no checking of duplicates is performed.               
*                                                                           
*                     This value should be zero when adding CONST           
*                     values that are not null-terminated.                  
*                                                                           
*                     It is HIGHLY RECOMMENDED this be set to "true" when   
*                     adding file and function name entries to the XT table..
*                                                                           
*                                                                           
* add_null         -  Add a null to the string if non-zero.  This is the    
*                     usual action (all strings should be null terminated). 
*                     Values used for CONST DNTT records need not be null   
*                     terminated.                                           
*                                                                           
*                     Note, if "check_duplicates" is non-zero, this         
*                     parameter is ignored.                                 
*                                                                           
*                                                                           
* LIMITATIONS:                                                              
*                                                                           
* This routine does not support any kind of alignment of data.  If this is  
* required, a new routine will be provided for that purpose.                
*                                                                           
* This interface does not support adding CONST values containing NULL       
* characters.                                                               
* 
\*****************************************************************************/

long add_to_vt( s, check_duplicates, add_null )
    char	*s;
    int		check_duplicates;
    int		add_null;
    
{
    int		hash_val;
    vt_node	hash_p;
    int		len;
    
    if (!Initialized) initialize_vt_stuff();

    if (!*s) return 0;
    
    hash_val = hash_vt(s);

    /* According to the people responsible for file operations,     */
    /* one needs to flush when switching file mode operations.      */
    /* So, whenever returning from add_to_vt, Fd_vt must be flushed */
    /* Thereby, guarenteeing a clean file structure when it is      */
    /* called again.                                                */

    len = strlen(s);
    if ( (check_duplicates) && (len < MAX_STRING_BUFF_SIZE) )
    {
	int  did_read = 0;
	
	hash_p = Vt_hash_table[hash_val];
	
	while (hash_p)
	    if (same_vt_string(s, hash_p, &did_read))
	    {
                /* add_to_vt is returning, so flush here */
                /* WORKAROUND for libc bug, clear the    */
                /* _IOREAD bit in the file flag. This is */
                /* a bug and will be fixed for           */ 	
		/* ------------------------------------- */
                if (did_read) {
		    fflush(Fd_vt);
		    Fd_vt->__flag &=  (~ _IOREAD);
		}
		
		return(hash_p->vt_index);
	    }
	    else
		hash_p = hash_p->next;
	
	/* NOTE: vtlib is switching from doing read operations to doing */
	/* write operations, so flush here.                             */
	/* WORKAROUND, again reset the _IOREAD bit in the flag for libc */
	/* bug.                                                         */
	/* ------------------------------------------------------------ */
	if (did_read)
	{
	    fflush(Fd_vt);
	    Fd_vt->__flag &=  (~ _IOREAD);
	}
    }

    /* If we get here, we either did not want to check for duplicates,  */
    /* or we did not find the string in the VT table.  So, create a new	*/
    /* entry in the hash table, and write the string to the VT file.    */
    /* ---------------------------------------------------------------  */
    hash_p = new_vt_hash_node(hash_val);
    hash_p->vt_index = write_vt_string(s, len, add_null);

    /* leaving add_to_vt,  so flush                              */
    /* NOTE: The fflush is also important for piping ccom to the */
    /* assembler, (i.e. the assembler may want to read the last  */
    /* string put into the file).                                */
    /* ---------------------------------------------------------------  */
    fflush(Fd_vt);

#ifdef IRIF
    ir_vt( s );       /* generate VT table entry */
#endif /* IRIF */

    return(hash_p->vt_index);
}

/*****************************************************************************\
* 
* static void remove_data_structures()
*
\*****************************************************************************/

static void remove_data_structures()
{    
    vt_node_block	block;

    while (Node_block_list)
    {
	block = Node_block_list;
	Node_block_list = (vt_node_block)Node_block_list->next_block;

	if (block != &First_node_block) free(block);
    }
}

#ifndef IRIFORT

/*****************************************************************************\
* 
* char *dump_vt()
* 
*  This routine will "dump" the VT table (close the file) built by the     
*  calls to "add_to_vt".                                                   
*                                                                          
*  A new pseudo op (vtfile) will be used to tell the assembler the name of 
*  the temp file where it will find the VT data.  This routine returns a   
*  string containing that pseudo-op with all required parameters.  The     
*  calling compiler must ensure the string gets inserted into the assembler
*  stream.                                                                 
*                                                                          
*  This routine closes the vt tempfile and releases any malloc'ed space    
*  created by these library routines.  Any additional call to "add_to_vt" 
*  is undefined.                                                           
*
\*****************************************************************************/

#define VTFILE_PSEUDO_OP     "vtfile"
#define FILEPSEUDO_OP_FORMAT "\t%s\t\"%s\"\n"

char *dump_vt()
{
    static char 	pseudo_op_buff[40];

    remove_data_structures();
    
    if (fclose(Fd_vt) != 0)
	vt_error("error closing vt temp file");
    
    sprintf(pseudo_op_buff, FILEPSEUDO_OP_FORMAT, VTFILE_PSEUDO_OP, Fname);
    return (pseudo_op_buff);
}

/*****************************************************************************\
* 
*  void dump_vt_to_dot_s( output_func, remove_tempfile )
* 
*  This routine will "dump" the VT table (close the file) built by the     
*  calls to "add_to_vt".                                                   
*                                                                          
*  A new pseudo op (vtfile) will be used to tell the assembler the name of 
*  the temp file where it will find the VT data.  This routine returns a   
*  string containing that pseudo-op with all required parameters.  The     
*  calling compiler must ensure the string gets inserted into the assembler
*  stream.                                                                 
*                                                                          
*  This routine closes the vt tempfile and releases any malloc'ed space    
*  created by these library routines.  Any additional call to "add_to_vt" 
*  is undefined.                                                           
*
\*****************************************************************************/

#define VT_PSEUDO_OP     "\n\tvt\n"
#define VTBYTE_PSEUDO_OP "\tvtbytes\t"
#define BYTES_PER_LINE   16

void dump_vt_to_dot_s( output_func, remove_tempfile )
    void (*output_func)();
    int remove_tempfile;
             
{
    register int c,i, eol;
    
    char line_buff[90];
    char byte_buff[10];
    
    /* Clean up our mess.  Also rewind the VT file so we can read it. */
    /* Also write the "vt" pseudo-op out to the .s file.	      */
    /* -------------------------------------------------------------- */
    remove_data_structures();
    rewind(Fd_vt);
    output_func( VT_PSEUDO_OP );

    /* Now fetch a character at a time from the VT file.  We buffer 16 */
    /* characters at a time before we send a "vtbytes xxx,xxx,xxx,..." */
    /* string out to the ".s" file.  The data is formatted in decimal. */
    /* --------------------------------------------------------------- */
    eol = 1;
    while ((c=getc(Fd_vt)) != EOF)
    {
	if (eol)
	{
	    strcpy(line_buff, VTBYTE_PSEUDO_OP);
	    i = 0;
	}
	
	/* About to add the last one for this line?  If so, dont */
	/* add a COMMA after it.				 */
	/* ----------------------------------------------------- */
	eol = (!((++i) % BYTES_PER_LINE));
	sprintf(byte_buff, "%d%s", c, (eol) ? "\n" : ",");
	strcat(line_buff, byte_buff);
	if (eol)
	    output_func( line_buff );
    }
    
    /* If we are in the middle of a line, nuke the trailing comma */
    /* and send the line to the .s file.                          */
    /* ---------------------------------------------------------- */
    if (!eol)
    {
	line_buff[strlen(line_buff) - 1] = '\n';
	output_func( line_buff );
    }
        
    if (fclose(Fd_vt) != 0)
	vt_error("error closing vt temp file");

    if (remove_tempfile)
	unlink(Fname);
}

#else /* IRIFORT */

#define BYTES_PER_LINE   16

void dump_vt_to_ucode( output_func, remove_tempfile )
    void (*output_func)();
    int remove_tempfile;
             
{
    register int c,i, eol;
    
    char line_buff[90];
    
    /* Clean up our mess.  Also rewind the VT file so we can read it. */
    /* -------------------------------------------------------------- */
    remove_data_structures();
    rewind(Fd_vt);

    /* Now fetch a character at a time from the VT file.  We buffer 16 */
    /* characters at a time before we send a "OPTN 31 n 'dddddddddddd' */
    /* --------------------------------------------------------------- */
    eol = 1;
    while ((c=getc(Fd_vt)) != EOF)
    {
	if (eol)
	    i = 0;
	line_buff[i++] = c;
	eol = !(i % BYTES_PER_LINE);
	if (eol)
	    output_func( BYTES_PER_LINE, line_buff );
    }
    
    /* If we are in the middle of a line, send the rest of the    */
    /* data.                                                      */
    /* ---------------------------------------------------------- */
    if (!eol)
    {
	output_func( i, line_buff );
    }
        
    if (fclose(Fd_vt) != 0)
	vt_error("error closing vt temp file");

    if (remove_tempfile)
	unlink(Fname);
}

#endif /* IRIFORT */

#ifdef TEST

/*****************************************************************************\
* 
* main()
*
\*****************************************************************************/

myprintf(s)
    char *s;
    
{
    printf(s);
}

main()
{
    char s[100];
    char d[20];
    int  fold;
    
    
    while (1)
    {
	printf("Enter a string (<string>|DUMP) : ");
	gets(s);
	if (strcmp(s, "DUMP") == 0)
	{
	    printf(dump_vt());
	    break;
	}
	else
	    if (strcmp(s, "DUMP2") == 0)
	    {
		dump_vt_to_dot_s( myprintf, 1 );
		break;
	    }
	
		
	printf("Check for dups (Y/N) [Y] : ");
	gets(d);
	
	fold =( (!*d) || (*d == 'Y') || (*d == 'y') ) ? 1 : 0;
	
	printf("\n...added, VT = %d\n\n", add_to_vt(s, fold, 1));
    }

    printf("\n\nDon't forget to remove: %s\n", Fname);
}    

#endif
