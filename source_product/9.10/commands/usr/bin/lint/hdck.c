/* @(#) $Revision: 70.11 $ */

/* Header file checker.
 * Usage:  hdck [-wi...] [-t target_std] [-O origins] [-S targets] [-T file.ln] hdck.db [file.i]
 * Read a cpp'ed file and look for files listed in the hdck database.
 * If file.i is not listed, read from stdin.
 */

#ifndef __LINT__
static char *HPUX_ID = "@(#) Internal $Revision: 70.11 $";
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include "apex.h"
extern char *optarg;
extern int opterr;
extern int optind;
extern int errno;

#ifdef DOMAIN
#define DEFAULT_ORIGIN  2		/* DOMAIN */
extern char *getenv();
#else
#define DEFAULT_ORIGIN	1		/* HPUX */
#endif

#define FALSE 	0
#define TRUE 	1

#define LINE_SIZE 	256
#define BIG_BUF		4096
long target_stds[NUM_STD_WORDS];
long origin = DEFAULT_ORIGIN;
int detail = 255;
short more_detail = 0;
char *target_file = "/usr/apex/lib/targets";
char *origin_file = "/usr/apex/lib/origins";
char *summary_file_name;
struct std_defs *target_list, *origin_list;

static char usrinclude[] = "/usr/include/";
static int usrinclude_len = sizeof(usrinclude) - 1;	/* don't include NULL */
char c_file[LINE_SIZE];
int lineno;
short num_errors = 0;	/* number of non-standard header files detected */
short num_hints = 0;	/* number of header file hints issued */
int header_printed = FALSE; /* whether file name and line of ====== printed */
int check_stds = TRUE;	/* turned off by "-nocheck stds" */
int show_hints = TRUE;	/* turned off by "-noshow hints" */
char *cur_file_name = "";
int cur_line_num = 0;

/* local routines */
static void read_db();
static int name_cmp();
static int check_file();
static char *parse_keyword();
static char *parse_name();
static char *parse_std();
static void parse_comment();
static void print_title();
static char *add_string();
static int pop_level();
static void warn_s();
static void error();
void print_filename();


/* error codes */
#define ERR_MEM_DB	1
#define ERR_MEM_STR	2
#define ERR_NOT_LB	3
#define ERR_NOT_RB	4
#define ERR_NO_APEX	5
#define ERR_BAD_DETAIL	6
#define ERR_CONFIG	7

/* character definitions */
#define NULL_CHAR	'\0'
#define SPACE_CHAR	' '
#define TAB_CHAR	'\t'
#define COMMA_CHAR	','
#define SLASH		'\\'
#define NEWLINE		'\n'

/* tag values */
#define BAD_TAG		0
#define STD		1
#define HINT		2


#define TRUE	1
#define FALSE	0

#define MAX(a,b) ( (a)>(b) ? (a) : (b) )


#ifdef STATS_UDP
/* The following defines should be passed in, the examples below were used
 * in the prototype: -DINST_UDP_PORT=49963 -DINST_UDP_ADDR=0x0f01780d */
/* pick a udp port that is unlikely to be used elsewhere */
#ifndef INST_UDP_PORT
#define INST_UDP_PORT           42963
#endif
#ifndef INST_UDP_ADDR
/* internet address of hpfcpas 15.1.120.15 */
#define INST_UDP_ADDR           0x0f01780f
#endif
#ifndef INST_VERSION
/* compiler version id */
#define INST_VERSION            1
#endif
/* instrumentation for beta releases. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
/* udp packet definition */
struct udp_packet {
    int size;                   /* size of the source file in bytes */
    char name[16];              /* basename of the file */
    unsigned long inode;        /* inode */
    int flag_lint:11;            /* used for lint options */
    int flag_w:1;
    int dummy:16;	/* for S800 alignment */
    int pass:2;         /* lint1 vs. lint2 vs. lintfor1 vs. hdck */
    int version:2;              /* compiler version id */
};
#endif /* STATS_UDP */


main(argc, argv)
int argc;
char *argv[];
{
    int c;
    char *p;
    struct std_defs *stdp;
    char *next_target, *comma_ptr;
    int target_bit;
    FILE *ifile;
    int summary_file;
    int category;
    long dummy[NUM_STD_WORDS];
    int target_given;


	summary_file = -1;
	target_given = FALSE;

	while ((c = getopt(argc, argv, "d:o:t:w:O:S:T:")) != EOF) {
	    switch (c) {
		case 'd':
		    detail = atoi(optarg);
		    break;

		case 'o':
		case 't':
		    /* defer to 2nd pass */
		    break;

		case 'w':
		    p = optarg;
		    while (*p) {
			if (*p == 'i')
			    exit(0);
			else if (*p == 's')
			    check_stds = FALSE;
			else if (*p == 'H')
			    show_hints = FALSE;

			p++;
		    }
		    break;

		case 'O':
		    origin_file = optarg;
		    break;

		case 'S':
		    target_file = optarg;
		    break;
		
		case 'T':
		    summary_file_name = optarg;
		    summary_file = 
			open(summary_file_name, O_RDWR|O_CREAT|O_APPEND, 0644); 
		    break;

	    }
	}

	origin_list = get_names(origin_file);
	if (origin_list==NULL)  {
	    warn_s(ERR_CONFIG, origin_file);
	    exit(-1);
	}
	target_list = get_names(target_file);
	if (target_list==NULL) {
	    warn_s(ERR_CONFIG, target_file);
	    exit(-1);
	}

	std_zero(target_stds);

	/* Repeat scan now that we know standards names; suppress errors */
	optind = 1;
	opterr = 0;
	while ((c = getopt(argc, argv, "d:o:t:w:S:T:O:")) != EOF) {
	    switch (c) {
		 case 'o':
		    if (get_bit(dummy, optarg, origin_list) != -1)
			origin = dummy[0];
		    else {
			fprintf(stderr,
				"Unrecognized origin: %s\n", optarg);
			origin = DEFAULT_ORIGIN;
		    }
		    break;

		case 't':
		    next_target = optarg;
		    target_given = TRUE;
		    while (comma_ptr = strchr(next_target, ',') ) {
			*comma_ptr = '\0';
			if (get_bit(dummy, next_target, target_list) != -1)
			    /* Could print error message here in the case of
			     * unrecognized target names, but that
			     * produces duplicate warnings when lint2 runs.
			     * Continue processing in case other valid
			     * target names are given.
			     */
			    std_or(target_stds, dummy, target_stds);
			next_target = comma_ptr + 1;
		    }
		    /* now get the last name in the option */
		    if (get_bit(dummy, next_target, target_list) != -1)
			std_or(target_stds, dummy, target_stds);
		    break;

		case 'O':
		case 'S':
		case 'T':
		    break;

	    }
	}

#ifdef STATS_UDP
	{
	    /* code to send out a UDP packet with information about
	     * the current compiler.  Requires a server that is
	     * listening for packets.
	     */
	    int s = socket(AF_INET,SOCK_DGRAM,0);
	    struct udp_packet packet;
	    struct sockaddr_in address,myaddress;
	    /* initialize the data */
	    packet.size = 0;
	    packet.inode = 0;
	    packet.pass = 3;
	    if (optind < argc-1)
		(void)strncpy(packet.name,argv[optind+1],16);
	    else
		(void)strncpy(packet.name,"",16);
	    packet.version = INST_VERSION;
	    /* set up the addresses */
	    address.sin_family = AF_INET;
	    address.sin_port = INST_UDP_PORT;
	    address.sin_addr.s_addr = INST_UDP_ADDR;
	    myaddress.sin_family = AF_INET;
	    myaddress.sin_port = 0;
	    myaddress.sin_addr.s_addr = INADDR_ANY;
	    /* try blasting a packet out, no error checking here */
	    bind(s,&myaddress,sizeof(myaddress));
	    sendto(s,&packet,sizeof(packet),0,&address,
		   sizeof(address));
	}
#endif /* STATS_UDP */

	if (!target_given)
	    exit(0);	/* not useful without -t target; e.g. apex -c */

	read_db(argv[optind]);

	if (optind < argc-1) {
	    ifile = fopen(argv[optind+1], "r");
	    if (ifile)
		check_file(ifile);
	} else
	    check_file(stdin);

	/* write the counts to a file that will be concatenated with lint1
	 * output to form the .ln file.  The counts will be totaled by lint2
	 * and printed in the summary section.
	 */
	if (summary_file != -1) {
	    char buf[5];
	    long format_magic;

	    /* REC_FILE_FMT, length=1 byte, version=1. Required first tag
	     * in the new file format. */
	    format_magic = 0x26000101;
	    write(summary_file, &format_magic, 4);

	    buf[0] = REC_HDR_ERR;
	    buf[1] = 0;		/* MSB of count */
	    buf[2] = 2;		/* 2 bytes follow */
	    if (num_errors) {
		memmove(&buf[3], &num_errors, 2);
		write(summary_file, buf, 5);
	    }
	    if (num_hints) {
		buf[0] = REC_HDR_HINT;
		memmove(&buf[3], &num_hints, 2);
		write(summary_file, buf, 5);
	    }
	    if (more_detail) {
		buf[0] = REC_HDR_DETAIL;
		memmove(&buf[3], &more_detail, 2);
		write(summary_file, buf, 5);
	    }
	    close(summary_file);
	}

	/* return value lets lint1 whether or not to print the file title */
	return (header_printed);
}



/*******************************************************************
 *
 *		Routines to read in the database
 *
 *******************************************************************/

/********************  Database Globals ****************************/

#define DB_INIT_SIZE  	512
#define DB_INC_SIZE	512
#define STRING_INIT_SIZE  1024
#define STRING_INCR  512
struct hd_db {char *name; 
	      long origin; 
	      long stds[NUM_STD_WORDS]; 
	      unsigned char min, max;	/* detail level */
	      short tag;	/* STD or HINT */
	      char *comment;};
struct hd_db *db;
int db_max;
char *strings, *next_string;
int cur_str_blk_sz;

static void read_db(config_file)
char *config_file;
{
    char line[BIG_BUF];
    char db_file[LINE_SIZE];
    int str_len;
    FILE *db_fd, *file_list;
    char *start;
    int db_size;


	db = (struct hd_db *)malloc(DB_INIT_SIZE * sizeof(struct hd_db) );
	if (db == NULL) {
#ifdef BBA_COMPILE
#pragma     BBA_IGNORE
#endif
	    error(ERR_MEM_DB);		/* "out of memory (db)" */
	    exit(-1);
	}
	db_max = 0;
	db_size = DB_INIT_SIZE;
	strings = (char *)malloc(STRING_INIT_SIZE);
	if (strings == NULL) {
#ifdef BBA_COMPILE
#pragma     BBA_IGNORE
#endif
	    error(ERR_MEM_STR);		/* "out of memory (strings)" */
	    exit(-1);
	}
	next_string = strings;
	cur_str_blk_sz = STRING_INIT_SIZE;

	file_list = fopen(config_file, "r");
	if (file_list == NULL)
	    exit(-1);
	while (fgets(db_file, LINE_SIZE, file_list) != NULL ) {
	    str_len = strlen(db_file);
	    db_file[str_len-1] = NULL_CHAR;
	    if (str_len <= 2)
		continue;
	    start = db_file;
	    while (*start==SPACE_CHAR || *start==TAB_CHAR)
		start++;
	    if (*start == NULL_CHAR || *start == '#')
		continue;
	    db_fd = fopen(start, "r");
	    cur_file_name = start;
	    cur_line_num = 1;
	    if (db_fd == NULL)
		continue;
	    while (fgets(line, BIG_BUF, db_fd) != NULL) {
		if (db_max >= db_size) {
		    db_size += DB_INC_SIZE;
		    db = (struct hd_db *)realloc(db, db_size*sizeof(struct hd_db) );
		    if (db == NULL) {
			error(ERR_MEM_DB);		/* "out of memory (db)" */
			exit(-1);
		    }
		}
		str_len = strlen(line);
		if (str_len > 2) {
		    /* replace the terminating newline with a NULL */
		    line[str_len-1] = NULL_CHAR;
		    start = line;
		    start = parse_keyword(start);
		    start = parse_name(start);
		    start = parse_std(start);
		    parse_comment(start, line, db_fd);
		    if (start != NULL)
			db_max++;
		}
		cur_line_num++;
	    }
	    fclose(db_fd);
	}

	qsort((void *)db, db_max, sizeof(struct hd_db), name_cmp);

}


/* The sorting routine used by qsort */
static int name_cmp(one, two)
struct hd_db *one, *two;
{
	return strcmp(one->name, two->name);
}




/* Accept a pointer to the beginning of the database line.
 * If the line is blank, or the first non-white-space character is #,
 * return NULL.  Otherwise, look for the keywords APEX STD or APEX HINT.
 * Accept:
 *      <APEX keywords> <header name>      [<stdlist>][min-max]   <comment>
 * ^
 * Return:
 *      <APEX keywords> <header name>      [<stdlist>][min-max]   <comment>
 *                     ^
 */

static char *parse_keyword(start)
char *start;
{
    char *line;

	line = start;

	while (*start==SPACE_CHAR || *start==TAB_CHAR)
	    start++;
	if (*start=='#' || *start==NULL_CHAR)
	    return NULL;	/* blank, or 1st char a '#' - ignore comment */

	/* Scan the keyword "APEX" */
	if ( strncmp(start, "APEX", 4) ) {
	    warn_s(ERR_NO_APEX, line);
	    return NULL;
	}
	start += 4;

	while (*start==SPACE_CHAR || *start==TAB_CHAR)
	    start++;

	/* Scan the keyword "STD" or "HINT" */
	if ( strncmp(start, "STD", 3) == 0 ) {
	    start += 3;
	    db[db_max].tag = STD;
	} else if ( strncmp(start, "HINT", 4) ==0 )  {
	    start += 4;
	    db[db_max].tag = HINT;
	} else {
	    warn_s(ERR_NO_APEX, line);
	    db[db_max].tag = BAD_TAG;
	    return NULL;
	}

	return start;

}

/* Accept a pointer to the beginning of the header name.  Save the name in
 * the string pool, and a pointer to it in the db entry.
 * Accept:
 *      <APEX keywords> <header name>      [<stdlist>][min-max]    <comment>
 * 		       ^
 * Return:
 *      <APEX keywords> <header name>      [<stdlist>][min-max]    <comment>
 *                   		     ^
 */

static char *parse_name(start)
char *start;
{
    char *end;
    int i;

	if (start == NULL)
	    return NULL;
	while (*start==SPACE_CHAR || *start==TAB_CHAR)
	    start++;
	end = start;

	/* Scan the header file name */
	while (*end != SPACE_CHAR && *end != TAB_CHAR && 
		*end != NULL_CHAR && *end != '[' )
	    end++;
	if ( !strncmp(start, usrinclude, usrinclude_len) ) {
		/* save effort in library, and space here by stripping
		 * common include path prefix.
		 */
		start += usrinclude_len;
	}
	db[db_max].name = add_string(start, end-start);

	/* set defaults in case of later parsing errors */
	for (i=0; i<NUM_STD_WORDS; i++)
	    db[db_max].stds[i] = 0;

	db[db_max].origin = 0;
	db[db_max].min = 1;
	db[db_max].max = 255;
	return end;

}



/* Parse the list of comma-separated standards names between square brackets.
 * Set the bitfield of applicable standards in the database entry.
 * Accept:
 *      <APEX keywords> <header name>      [<stdlist>][min-max]    <comment>
 * 		     		     ^
 * Return:
 *      <APEX keywords> <header name>      [<stdlist>][min-max]    <comment>
 *				    			       ^
 */

static char *parse_std(start)
char *start;
{
    char *end;
    long stds[NUM_STD_WORDS];
    long tmp[NUM_STD_WORDS];
    int str_len;
    struct std_defs *stdp;
    int i;
    char save_char;
    int bit_num;
    int origin_tmp, target_bit;

	if (start == NULL)
	    return NULL;
	std_zero(stds);

	/* Scan the standards list */
	while (*start == SPACE_CHAR || *start == TAB_CHAR)
	    start++;
	if (*start != '[') {
	    /* Database format error: Expected [ for entry %s */
	    warn_s(ERR_NOT_LB, db[db_max].name);	
	    return NULL;
	}
	start++;
	while (*start == SPACE_CHAR || *start == TAB_CHAR)
	    start++;
	end = start;
	while (*end != SPACE_CHAR && *end != TAB_CHAR && 
	    *end != NULL_CHAR && *end != ']' && *end != COMMA_CHAR &&
	    *end != '-')
	    end++;
	save_char = *end;
	*end = NULL_CHAR;
	if ( *start == '*' ) {
	    set_bit(tmp, ALL_STD_BITS);
	} else {
	    if (get_std_num(tmp, start, origin_list) == -1)
		origin_tmp = DEFAULT_ORIGIN;
	    else
		origin_tmp = tmp[0];
	    if (get_std_num(tmp, start, target_list) == -1)
		std_zero(tmp);
	}
	*end = save_char;

	start = end;
	while (*start == SPACE_CHAR || *start == TAB_CHAR)
	    start++;
	if ( *start == '-' && *(start+1) == '>' ) {
	    db[db_max].origin = origin_tmp;
	    start += 2;
	} else {
	    std_cpy(stds, tmp);
	    if ( *start == COMMA_CHAR )
		start++;	/* step over separator */
	}
	while (*start == SPACE_CHAR || *start == TAB_CHAR)
	    start++;

	while ( *start!=NULL_CHAR && *start!=']' ) {  
	/* for each additional standard */
	    end = start;
	    while (*end != SPACE_CHAR && *end != TAB_CHAR && 
		    *end != NULL_CHAR && *end != ']' && *end != COMMA_CHAR)
		end++;
	    save_char = *end;
	    *end = NULL_CHAR;
	    if (*start == '*') 
		set_bit(stds, ALL_STD_BITS);
	    else if (get_std_num(tmp, start, target_list) != -1) 
		std_or(tmp, stds, stds);
	    *end = save_char;
	    start = end;
	    if ( *start == COMMA_CHAR )
		start++;	/* step over separator */
	    while (*start == SPACE_CHAR || *start == TAB_CHAR)
		start++;
	} 

	if (*start == ']') {
	    start++;
	    std_cpy(db[db_max].stds, stds);
	} else {
	    warn_s(ERR_NOT_RB, db[db_max].name);	/* expected ']' */
	}

	/* Look for optional detail level specification */
	while (*start == SPACE_CHAR || *start == TAB_CHAR)
	    start++;
	if (*start == '[') {
	    start++;
	    while (*start == SPACE_CHAR || *start == TAB_CHAR)
		start++;
	    if ( isdigit(*start) )
		db[db_max].min = *start - '0';
	    else
		warn_s(ERR_BAD_DETAIL, db[db_max].name);
	    start++;
	    while (*start == SPACE_CHAR || *start == TAB_CHAR)
		start++;
	    if ( *start == '-' ) {
		start++;
		while (*start == SPACE_CHAR || *start == TAB_CHAR)
		    start++;
		if ( isdigit(*start) )
		    db[db_max].max = *start - '0';
		else
		    warn_s(ERR_BAD_DETAIL, db[db_max].name);
		start++;
		if (db[db_max].max < db[db_max].min)
		    warn_s(ERR_BAD_DETAIL, db[db_max].name);
		while (*start == SPACE_CHAR || *start == TAB_CHAR)
		    start++;
	    }

	    if ( *start == ']' )
		start++;
	    else
		warn_s(ERR_BAD_DETAIL, db[db_max].name);

	}

	return start;
}



/* Find an optional comment field.  Search for the first non-blank character
 * from the passed pointer.  Copy any comments to the end-of-line into the
 * string pool and set the database entry to point to it.
 * Accept:
 *      <APEX keywords> <header name>      [<stdlist>][min-max]     <comment>
 *				      ^
 */
static void parse_comment(start, line, db_fd)
char *start;
char *line;
FILE *db_fd;
{
char extra_buf[BIG_BUF];

    int len;
    int str_len;
    char *next_line_start;

	db[db_max].comment = NULL;

	if (start == NULL)
	    return;

	/* Begin comment at first non-white character.  An escaped newline
	 * permits the comment to begin on the line following the APEX keyword.
	 */
	while (start && 
		(*start == SPACE_CHAR || 
		 *start == TAB_CHAR || 
		 *start == SLASH) )  {
	    if (*start == SLASH && *(start+1)==NULL_CHAR) {
		start = fgets(line, BIG_BUF, db_fd);
		str_len = strlen(line) - 1;
		if (str_len >= 0)
		    line[str_len] = NULL_CHAR;		/* replace NEWLINE */
		cur_line_num++;
	    } else
		start++;
	}

	/* Concatenate lines that end with a SLASH */
	next_line_start = start; 
	str_len = strlen(next_line_start);
	while ( next_line_start && *(next_line_start+str_len-1) == SLASH ) {
	    *(next_line_start+str_len-1) = NEWLINE;	/* replace SLASH */
	    next_line_start += str_len;
	    if (fgets(extra_buf, BIG_BUF, db_fd)) {
		cur_line_num++;
		str_len = strlen(extra_buf) - 1;
		if (str_len > 0)
		    extra_buf[str_len] = NULL_CHAR;	/* remove newline */
		/* if there's room, copy into main "line" buf */
		if (next_line_start - line + str_len < BIG_BUF) {
		    strcpy(next_line_start, extra_buf);
		} else { /* discard remaining lines */
		    while ( next_line_start && 
				*(next_line_start+str_len-1) == SLASH ) {
			fgets(extra_buf, BIG_BUF, db_fd);
			str_len = strlen(extra_buf) - 1;
			cur_line_num++;
		    }
		}
	    } else
		next_line_start = NULL;
	} 
	
	if (*start != NULL_CHAR) { 
	    len = strlen(start); 
	    db[db_max].comment = add_string(start, len);
	}

}


/* Add a string to the string pool, appending a null terminator  */
static char *add_string(s, len)
char *s;
int len;
{
    char *temp;

	if ( (next_string - strings + len + 1) > cur_str_blk_sz) {
	    cur_str_blk_sz = MAX(STRING_INIT_SIZE, (len+STRING_INCR) );
	    strings = (char *)malloc(cur_str_blk_sz);
	    if (strings == NULL) {
#ifdef BBA_COMPILE
#pragma 	BBA_IGNORE
#endif
		error(ERR_MEM_STR);	/* "out of memory (strings)" */
		exit(-1);
	    }
	    next_string = strings;
	}
	strncpy(next_string, s, len);
	next_string[len] = NULL_CHAR;
	temp = next_string;
	next_string += (len+1);
	return temp;

}


/*******************************************************************
 *
 *		Routines to check the cpp'ed file
 *
 *******************************************************************/

static int check_file(ifile)
FILE *ifile;
{
    int db_ptr;
    char line[LINE_SIZE];
    char current_file[LINE_SIZE];
    char *start, *stop, *node;
    int base_file;
    int temp_line;
    int cur_line;

	cur_line = 1;
	base_file = TRUE;
	/* assume the first line is # 1 "file.c"  */
	if (fgets(line, LINE_SIZE, ifile) != NULL ) {
	    if (line[0] == '#') {
		start = line + 1;
		while (*start==SPACE_CHAR || *start==TAB_CHAR ) 
		    start++;
		lineno = atoi(start);
		while (isdigit(*start) || *start==SPACE_CHAR || 
					*start==TAB_CHAR ) 
		    start++;
		if ( *start == '"' ) {
		    start++;
		    stop = strrchr(start, '"' );
		    *stop = NULL_CHAR;
		    strcpy(c_file, start);
		    strcpy(current_file, c_file);
		}
	    }
	}

	while (fgets(line, LINE_SIZE, ifile) != NULL) {
	    start = line;
	    if ( *start == '#' ) {
		start++;
		while (*start==SPACE_CHAR || *start==TAB_CHAR ) 
		    start++;
		temp_line = atoi(start);
		while (isdigit(*start) || *start==SPACE_CHAR || 
					*start==TAB_CHAR ) 
		    start++;
		if (*start == '"') {
		    start++;
		    stop =  strrchr(start, '"');
		    if (stop != NULL) {
			*stop = NULL_CHAR;
			if ( !strcmp(start, c_file) ) {
			    base_file = TRUE;
			    strcpy(current_file, start);
			} else { /* included file */
			    /* ignore //node prefix */
			    base_file = FALSE;
			    if (*start=='/' && *(start+1)=='/' && 
					(node=strchr(start+2, '/')) )
				    start = node;
			    if ( !strcmp(current_file, c_file) )
				db_ptr = check_name(start, NULL, lineno-1);
			    else
				db_ptr = check_name(start, current_file, cur_line-1);
			    if (db_ptr != -1) {
				/* ignore inclusions by this header */
				temp_line = pop_level(ifile, current_file);  
				if ( !strcmp(current_file, c_file) ) {
				    base_file = TRUE;
				}
			    } else {
				strcpy(current_file, start);
			    }
			}
		    }
		}
		cur_line = temp_line - 1;
		if (base_file)
		    lineno = temp_line -1;
	    } 
	cur_line++;
	if (base_file)
	    lineno++;
	}
}


/* print <name.h> or "name.h" 
 *  use the <> version if the name begins with /usr/include, or begins
 *  with a character other than "."
 */
void print_h_name(name)
char *name;
{
	if ( !strncmp(name, usrinclude, usrinclude_len) )
	    printf("<%s>", name+usrinclude_len);
	else
	    printf("\"%s\"", name);
}


/* check_name looks up the header file name in the database and prints
 * the appropriate standards and portability information
 */
int check_name(pathname, file, line)
char *pathname, *file;
int line;
{
int index, index_start, i;
long this_std[NUM_STD_WORDS], any_std[NUM_STD_WORDS];
int std_flag, hint_flag;
char *start;
char *name;

	if (!strncmp(pathname, usrinclude, usrinclude_len) )
	    name = pathname + usrinclude_len;
	else
	    name = pathname;

	std_flag = FALSE;
	hint_flag = FALSE;
	std_zero(this_std);

	index = search(name);
	if (index == -1)
	    return -1;

	index_start = index;
	do {
	    if ( db[index].origin==0 || db[index].origin & origin ) {
		if (db[index].tag == STD) {
		    if ( !std_overlap(db[index].stds, target_stds) ) {
			std_or(db[index].stds, this_std, this_std);
			std_flag = TRUE;
		    }
		}
	    }
	    index++;
	} while ( index < db_max && !strcmp(name, db[index].name) );

	/* print standards violations first */
	if (std_flag && check_stds) {
	    print_filename(pathname, file, line);
	    printf("\tnot specified by ");
	    std_print(target_stds, target_list);
	    set_bit(any_std, ALL_STD_BITS);
	    if (std_overlap(any_std, this_std) ) {
		printf(";  (see ");
		std_print(this_std, target_list);
		printf(")\n");
	    } else
		printf("\n");
	    num_errors++;
	}


	if (show_hints) {
	    index = index_start;
	    /* Now repeat the scan to print applicable hints */
	    do {
		if ( db[index].origin==0 || db[index].origin & origin ) {
		    if (db[index].tag == HINT) {
			if ( std_overlap(db[index].stds, target_stds) && 
				    detail < db[index].min  ) {
			    if (db[index].min > more_detail)
				    more_detail = db[index].min;
			} else if ( std_overlap(db[index].stds, target_stds) && 
				    detail >= db[index].min  &&
				    detail <= db[index].max)	 {
			    if (!hint_flag && !std_flag)
				print_filename(pathname, file, line);
			    hint_flag = TRUE;
			    start = db[index].comment;
			    putchar(TAB_CHAR);
			    while (*start != NULL_CHAR) {
				putchar(*start);
				/* make sure there is a least one tab indentation */
				if (*start == NEWLINE && *(start+1) != TAB_CHAR ) {
				    putchar(TAB_CHAR);
				} 
				start++;
			    }
			    putchar(NEWLINE);
			}
		    }
		}
		index++;
	    } while ( index < db_max && !strcmp(name, db[index].name) );

	    if (hint_flag)
		num_hints++;
	}

	return index;

}


void print_filename(name, file, line)
char *name;	/* name of the included file */
char *file;	/* name of including file, if not the base file */
int line;	/* line of including file, if not lineno */
{
	print_title();
	if (file) {
	    printf("(->) %s(%d) ", file, line);
	} else {
	    printf("(%d) ", line);
	}
	print_h_name(name);
	printf("\n");

}


/* pop_level scans the input until the pre-processor include level has
 * returned to the user code.  This is so that a compliant system header
 * is free to include any system-dependent headers it wishes without
 * making these visible in the database.  This routine reads lines until
 * one is found that matches "# <line> includer", where includer is the
 * name of the file that included a recognized header file.
 * Return the line number to re-synchronize lineno.
 */
static int pop_level(ifile, includer)
FILE *ifile;
char *includer;
{
    char line[LINE_SIZE];
    char *start, *stop, *node;
    int temp_line;

	temp_line = 0;
	while (fgets(line, LINE_SIZE, ifile) != NULL) {
	    if (line[0] == '#') {
		start = line+1;
		while (*start==SPACE_CHAR || *start==TAB_CHAR ) 
		    start++;
		temp_line = atoi(start);
		start = strchr(start, '"');
		if (start) {
		    start++;
		    stop = strrchr(line, '"');
		    *stop = NULL_CHAR;
		    /* strip //node prefix */
		    if (*start=='/' && *(start+1)=='/' &&
			    (node=strchr(start+2, '/')) )
			start = node;
		    if ( !strcmp(start, includer) )
			return temp_line;
		}
	    }
	}
	return temp_line;

}



/* return the first index in the database that matches "name" */
static int search(name)
char *name;
{
    int lower;
    int mid;
    int upper;
    int match;
    int result;		/* of the string comparison */

	lower = 0;  upper = db_max-1;
	mid = (lower+upper)/2;
	match = FALSE;

	while (upper >= lower) {
	    result = strcmp(name, db[mid].name); 
	    if (result==0) {
		match = TRUE;
		break;
	    } else if (result < 0) 
		upper = mid - 1;
	    else
		lower = mid + 1;
	    
	    mid = (lower+upper)/2;
	}

	if (!match)
	    return -1;

	while (mid > 0) {
	    if (strcmp(name, db[mid-1].name) != 0)
		return mid;
	    mid--;
	}

	return mid;

}



static void print_title()
{
	if (!header_printed) {
	    printf("%s\n", c_file);
	    printf("==============\n");
	    header_printed = TRUE;
	}
}



#ifdef BBA_COMPILE
#pragma BBA_IGNORE
#endif
static void error(err_number)
int err_number;
{
    fprintf(stderr, "Error (header file check): ");

    switch (err_number) {
	case ERR_MEM_DB:
	    fprintf(stderr, "out of memory (strings)");
	    break;

	case ERR_MEM_STR:
	    fprintf(stderr, "out of memory (strings)");
	    break;

	default:
	    fprintf(stderr, "internal error %d", err_number);
	    break;

    }

    fprintf(stderr, "\n");
    exit(-1);

}


static void warn_s(err_number, mess)
int err_number;
char *mess;
{
    fprintf(stderr, "Warning (header file check %s(%d)): ", 
				cur_file_name, cur_line_num);

    switch (err_number) {
	case ERR_NO_APEX:
	    fprintf(stderr, "Unrecognized database entry (%s)", mess);
	    break;

	case ERR_NOT_LB:
	    fprintf(stderr, "Database format error: Expected [ for entry %s",
			mess);
	    break;

	case ERR_NOT_RB:
	    fprintf(stderr, "Database format error: Expected ] for entry %s",
			mess);
	    break;

	case ERR_BAD_DETAIL:
	    fprintf(stderr, "Database format error: detail level for entry %s",
			mess);
	    break;

	case ERR_CONFIG:
	    fprintf(stderr, "Cannot read configuration file %s", mess);
	    break;

	default:
#ifdef BBA_COMPILE
#pragma BBA_IGNORE
#endif
	    fprintf(stderr, "internal error %d", err_number);
	    exit(-1);		/* we're lost */
	    break;
    }

    fprintf(stderr, "\n");

}
