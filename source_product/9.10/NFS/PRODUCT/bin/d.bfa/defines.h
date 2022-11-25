
#if defined(MSDOS)
#define FAR far
#else
#define FAR
#endif

#if defined(UNIX)
#define YYLMAX 32768
#else
#define YYLMAX 8196
#endif

#define DEFAULT_STACK_MAX	128
#define	DEFAULT_STREAM_LEN	16384
#define DEFAULT_ELSE_LEN	2048
#define MAX_FUNCTION_ID		128

#define SPECIAL_CHAR		'\001'
#define	MAXFILES		128
#define MAXPATH			512
#define MAXMAINS		256
#define MAXLINE			1024
#define MAXID			32

#define	BFADBASE		"bfadbase"
#define BFATEMP			"bfatmp"
#define BFA_ERROR		1

#if defined(MSDOS)
#define	FILELEN			8
#endif

#if defined(UNIX)
#define FILELEN			12
#endif

#define STATE_L			1
#define STATE_I			2
#define STATE_D			3
#define STATE_P			4

#define CURL	1
#define	SEMI	2
#define	IGNORE	3	
#define DO_SEMI	4
#define DO_CURL	5

#define _ENTRY     1
#define _FOR       2
#define _WHILE     3
#define _DO        4
#define _CASE      5
#define _DEFAULT   6
#define _IF        7
#define _ELSE      8
#define _ELSEIF    9
#define _SWITCH   10
#define _LABEL    11
#define _FALLTHRU 12

#define STACK_ELEMENT struct stack_element 
struct stack_element {
	int	line;
	int	kind;
	int	branch_type;
};



#if defined(BFA_KERNEL)
/* Declarations needed for the Kernel Version of BFA               */

#define MAX_BRANCHES  8192
#define MAX_ENTRIES     32

#define ENTRY struct entry
struct entry {
   char   database[MAXPATH];
   char   file[MAXPATH];
   long   offset;
   int    length;
};

#endif



#if defined(TSR)
/* Declarations needed for the TSR Version of BFA			*/

#define MAX_BRANCHES  		4096
#define MAX_ENTRIES     	  32
#define INTERRUPT_VECTOR	0xfe


#define MEM_MODE	1
#define FILE_MODE	2
#define DATA_MODE	3
#define COUNT_MODE	4


#define MEM_REQUEST struct mem_request
struct mem_request {
	char far *database;
	char far *file;
	int	 branches;
	long far *data;
};

#define DATA_REQUEST struct data_request
struct data_request {
	char far *data;
};

#define COUNT_REQUEST struct count_request
struct count_request {
	int	data;
};


#define ENTRY struct entry
struct entry {
   char   database[MAXPATH];
   char   file[MAXPATH];
   long   offset;
   int    length;
};

#endif



