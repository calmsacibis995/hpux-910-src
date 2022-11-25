/* $Revision: 72.1 $ */

/*
 * (c) Copyright 1989,1990,1991,1992 Hewlett-Packard Company, all rights reserved.
 *
 * #   #  ##  ##### ####
 * ##  # #  #   #   #
 * # # # #  #   #   ####
 * #  ## #  #   #   #
 * #   #  ##    #   ####
 *
 * Please read the README file with this source code. It contains important
 * information on conventions which must be followed to make sure that this
 * code keeps working on all the platforms and in all the environments in
 * which it is used.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "support.h"

#ifdef hpe
#include <fcntl.h>
#define CCE 2
#endif

char date_string[12], time_string[9];


/*
 * This routine returns a hash number of int range based upon the given
 * string.  It uses up to the first 10 characters in string.
 */
int string_hash(string, length)
register char *string;
int length;
{
	register int hash_num = 0;

	if(length > 10)
		length = 10;
	while(length--)
	{
		hash_num = (hash_num<<1)+*string;
		string++;
	}
	return hash_num;
}


/*
 * This routine compares the two ids up to the first non-identifier
 * character.  It returns TRUE if they are the same, FALSE otherwise.
 */
boolean id_cmp(id1, id2)
register char *id1, *id2;
{
	while(*id1 == *id2 && is_id(*id1))
	{
		id1++;
		id2++;
	}
	if(is_id(*id1) || is_id(*id2))
		return FALSE;
	else
		return TRUE;
}


/*
 * This routine mallocs the various static buffers used by cpp.ansi
 * The default buffer size is 8188, so that
 *   2 * buffersize + sizeof(ChunkHeader) = 16384
 * It can be modified with the -B option.
 */
int buffersize = DEFAULT_BUFFER_SIZE;
extern char *shift_buffer;
extern char *replace_buffer;
extern char *return_line;
extern char *in_line;
#ifdef OPT_INCLUDE
extern char *condition_buffer;
#endif
extern char *original_line;
extern char *param_buffer;
extern char *result_buffer;
extern int result_size;
extern char *input_buffer;
extern char *tree_buffer;
extern int tree_buffer_size;

void init_buffers()
{
  if(!shift_buffer &&
     (shift_buffer = malloc(buffersize)) == NULL)
    error("Out of dynamic memory");
  if(!replace_buffer &&
     (replace_buffer = malloc(2*buffersize)) == NULL)
    error("Out of dynamic memory");
  if(!return_line &&
     (return_line = malloc(2*buffersize)) == NULL)
    error("Out of dynamic memory");
  if(!in_line &&
     (in_line = malloc(buffersize)) == NULL)
    error("Out of dynamic memory");
#ifdef OPT_INCLUDE
  if(!condition_buffer &&
     (condition_buffer = malloc(2*buffersize)) == NULL)
    error("Out of dynamic memory");
#endif
  if(!original_line &&
     (original_line = malloc(buffersize)) == NULL)
    error("Out of dynamic memory");
  if(!param_buffer &&
     (param_buffer = malloc(2*buffersize)) == NULL)
    error("Out of dynamic memory");

  if(!result_buffer &&
     (result_buffer = malloc(buffersize)) == NULL)
    error("Out of dynamic memory");
  result_size = buffersize-1;

  if(!input_buffer &&
     (input_buffer = malloc(2*buffersize)) == NULL)
    error("Out of dynamic memory");

  if(!tree_buffer &&
     (tree_buffer = malloc(buffersize)) == NULL)
    error("Out of dynamic memory");
  tree_buffer_size = buffersize;
}

/*
 * Free up all memory in use.
 */

void finish_cpp()
{
  if (shift_buffer)
    free (shift_buffer), shift_buffer = NULL;
  if (replace_buffer)
    free (replace_buffer), replace_buffer = NULL;
  if (return_line)
    free (return_line), return_line = NULL;
  if (in_line)
    free (in_line), in_line = NULL;
#ifdef OPT_INCLUDE
  if (condition_buffer)
    free (condition_buffer), condition_buffer = NULL;
#endif
  if (original_line)
    free (original_line), original_line = NULL;
  if (param_buffer)
    free (param_buffer), param_buffer = NULL;
  if (result_buffer)
    free (result_buffer), result_buffer = NULL;
  if (input_buffer)
    free (input_buffer), input_buffer = NULL;
  if (tree_buffer)
    free (tree_buffer), tree_buffer = NULL;

  area_dealloc (temp_area_ptr, 1);
  area_dealloc (perm_area_ptr, 1);
}

/* Memory is allocated from the system in large chunks, then dished out in
 * small pieces. Each chunk has a header which contains a pointer to the next
 * chunk and the size of the chunk; each chunk is a default size unless the
 * allocation request is very large, then it's the size of the request. This
 * is the structure of the chunk header. */
typedef struct chunkheader ChunkHeader;
struct chunkheader
{
  ChunkHeader *nextChunk;
  long	       chunkSize;
};

/* This is the default chunk size, including the size of the header.
 * Since the largest allocation request seems to be 2*buffersize
 * make MEMORYCHUNK always accomodate it. The default buffersize is arranged
 * so that MEMORYCHUNK is a power of 2. */
#define MEMORYCHUNK (2*buffersize + sizeof(ChunkHeader))

/* ALIGN must be a power of 2 and is the alignment boundary. */
#define ALIGN 4
#define align(n) ((-(long)(n)) & (ALIGN-1))

/* Two separate allocation areas are maintained - the temp area is deallocated
 * after each output line is assembled, and so should only be used for data
 * which can be disposed of after each line. The perm area is deallocated after
 * the entire main source file is processed. This is the information maintained
 * for each allocation area. */
typedef struct allocarea
{
  ChunkHeader  *allocList;	/* list of in-use memory chunks */
  ChunkHeader  *freeList;	/* list of available memory chunks */
  char	       *curChunk;	/* pointer into chunk being used for allocation */
  long		curSize;	/* amount available in current chunk */
#ifdef MEMORY_DEBUG
  long		totalRequested;
  long		maxRequested;
  long		totalAllocated;
#endif
} AllocArea;

static AllocArea temp_area;
static AllocArea perm_area;

/* Pointers to the areas are made visible for use by calling routines,
 * but their type is opaque. Macros defined in support.h reference them
 * to allocate temporary or permanent memory. */
AllocArea *temp_area_ptr = &temp_area;
AllocArea *perm_area_ptr = &perm_area;

/*
 * The allocate procedure. It is not referenced directly by callers, but via
 * a macro which specifies which area to allocate from.
 */
char *area_alloc(n, area)
     register long       n;
     register AllocArea *area;
{
  register char         *temp;
  
  /* Make n the value of the passed parameter rounded up to an alignment boundary. */
  n += align(n);

  /* See if the current chunk of memory is big enough to provide n bytes. */
  if (n > area->curSize)
    {
      /* It isn't. If there are chunks on the free list, and the first
       * one is big enough, use it. Otherwise, allocate a new one.
       * (We don't bother searching the free list for a big enough chunk,
       * since huge allocation requests are expected to be very rare.) */
      register ChunkHeader *chunkPtr;

      if (area->freeList &&
	  n <= area->freeList->chunkSize - sizeof(ChunkHeader))
	{
	  chunkPtr = area->freeList;
	  area->freeList = chunkPtr->nextChunk;
	}
      else
	{
	  long chunkSize = ((n > MEMORYCHUNK - sizeof(ChunkHeader))
			     ? n + sizeof(ChunkHeader)
			     : MEMORYCHUNK);
	  chunkPtr = (ChunkHeader *) malloc (chunkSize);
	  if (!chunkPtr)
	    fatal_cpp_error("Out of dynamic memory");

	  chunkPtr->chunkSize = chunkSize;
#ifdef MEMORY_DEBUG
	  area->totalAllocated += chunkSize;
#endif
	}

      /* Add new chunk to alloc list and set up current chunk info. */
      chunkPtr->nextChunk = area->allocList;
      area->allocList     = chunkPtr;
      area->curChunk      = (char*) chunkPtr + sizeof(ChunkHeader);
      area->curSize       = chunkPtr->chunkSize - sizeof(ChunkHeader);
    }

  /* Return the allocation request. */
  temp = area->curChunk;
  area->curChunk += n;
  area->curSize  -= n;

#ifdef MEMORY_DEBUG
  area->totalRequested += n;
  if (area->totalRequested > area->maxRequested)
    area->maxRequested = area->totalRequested;
#endif

  return temp;
}


/*
 * This routine returns a pointer to 'n' bytes of dynamic storage, the first
 * portion of which has the same contents as what 'ptr' points to.
 * No attempt is made to expand the space which 'ptr' points to since
 * branch analysis showed that this case never occurs.
 */
char *area_realloc(ptr, n, area)
register char      *ptr;
register long       n;
register AllocArea *area;
{
  register char *new_mem, *new_ptr;
  
  n += align(n);
  new_ptr = new_mem = area_alloc(n, area);
  
  /* 920206 vasta: this is a bad thing to do - most likely `n' is larger than
     the originally allocated size of *ptr, so we're going to reference memory
     beyond what was allocated. However, since we don't know the size of *ptr,
     there's nothing much we can do (but cross our fingers!) */
  while(n > 0)
    {
      *new_ptr++ = *ptr++;
      n--;
    }
  return new_mem;
}


/*
 * This routine deallocates all the memory for a particular area. If `do_free' != 0
 * then the memory is actually freed, otherwise it is just returned to the free list
 * for the area.
 */
void area_dealloc(area, do_free)
register AllocArea *area;
int do_free;
{
  register ChunkHeader *chunkPtr;
  
  if (do_free)
    {
      register ChunkHeader *nextChunk;
      
      for (chunkPtr = area->allocList; chunkPtr; chunkPtr = chunkPtr->nextChunk)
	{
	  nextChunk = chunkPtr->nextChunk;
	  free (chunkPtr);
	  chunkPtr = nextChunk;
	}

      for (chunkPtr = area->freeList; chunkPtr; chunkPtr = chunkPtr->nextChunk)
	{
	  nextChunk = chunkPtr->nextChunk;
	  free (chunkPtr);
	  chunkPtr = nextChunk;
	}

      area->allocList = area->freeList = NULL;
      area->curChunk  = 0;
      area->curSize   = 0;
#ifdef MEMORY_DEBUG
      area->totalAllocated = 0;
#endif
    }
  else
    {
      /* When combining the two lists, the alloc list is traversed rather than
	 the free list because it will tend to be shorter and will definitely be
	 proportional to the amount of space allocated since the last deallocate. */
      if(area->allocList == 0)
	return;
      if(area->freeList != 0)
	{
	  chunkPtr = area->allocList;
	  while(chunkPtr->nextChunk != 0)
	    chunkPtr = chunkPtr->nextChunk;
	  chunkPtr->nextChunk = area->freeList;
	}
      area->freeList  = area->allocList;
      area->allocList = 0;
      area->curChunk  = 0;
      area->curSize   = 0;
#ifdef MEMORY_DEBUG
      area->totalRequested = 0;
#endif
    }
}

#ifdef MEMORY_DEBUG
void dump_chunks(chunkPtr)
ChunkHeader *chunkPtr;
{
  while (chunkPtr)
    {
      printf ("%8x: %6d -> %8x\n", chunkPtr, chunkPtr->chunkSize,
	      chunkPtr->nextChunk);
      chunkPtr = chunkPtr->nextChunk;
    }
}
#endif

set_time()
{
	register struct tm *current_time;
	long seconds;
	static char month_name[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	/* Get the current time. */
	seconds = time(0);
	current_time = localtime(&seconds);

	/* Create the date string for the __DATE__ macro. */
	sprintf(date_string, "%.3s %2d %d", month_name[current_time->tm_mon],
		current_time->tm_mday, 1900+current_time->tm_year);

	/* Create the time string for the __TIME__ macro. */
	time_string[0] = '0'+current_time->tm_hour/10;
	time_string[1] = '0'+current_time->tm_hour%10;
	time_string[2] = ':';
	time_string[3] = '0'+current_time->tm_min/10;
	time_string[4] = '0'+current_time->tm_min%10;
	time_string[5] = ':';
	time_string[6] = '0'+current_time->tm_sec/10;
	time_string[7] = '0'+current_time->tm_sec%10;
	time_string[8] = '\0';
}

static char *make_date_string(file_time)
register struct tm *file_time;
{
	register char *filedate;

	filedate = perm_alloc(21);
	strftime(filedate, 21, ",%Y,%m,%d,%H,%M,%S", file_time);
	filedate[20] = '\0';
	return filedate;
}

#ifdef hpe
   #pragma intrinsic FFILEINFO
   #pragma intrinsic ALMANAC
#endif

char *get_file_date(filename)
char *filename;
{
	struct stat buf, *bufp = &buf;
	register struct tm *file_time;

#ifdef hpe
        int file_num;
        int fdes;
        int last_mod_time = 0;
        unsigned short int last_mod_date = 0;
        char date_and_time[27];
        struct tm temp_file_time;
        struct tm break_time_date();
#endif


#ifndef hpe
        if(stat(filename, bufp) != 0)
        {
                error("Unable to stat %s", filename);
                return NULL;
        }
        file_time = localtime(&bufp->st_mtime);
#else
        fdes = open(filename, O_RDONLY);
        if (fdes == -1)
        {
                error("Unable to open %s", filename);
                return NULL;
        }
/*                                                        */
/* call intrinsic FFILEINFO to obtain info about the file */
/*                                                        */
        file_num = _mpe_fileno(fdes);
        FFILEINFO(file_num,52,&last_mod_time,
                           53,&last_mod_date);
        if (ccode() != CCE)
        {
                error("FFILEINFO intrinsic call for %s", filename);
                error(" not granted\n");
                return NULL;
        }
        else
        {
           temp_file_time = break_time_date(last_mod_time, last_mod_date);
           mktime(&temp_file_time);
           file_time = &temp_file_time;
        }
#endif     /* #ifndef hpe */

	return make_date_string(file_time);
}

char *get_current_date()
{
	register struct tm *current_time;
	long seconds;

	/* Get the current time. */
	seconds = time(0);
	current_time = localtime(&seconds);
	return make_date_string(current_time);
}


/*
 * This routine skips over the number starting at 'line' it returns a pointer
 * to the character following the number.  Here, a number is an ANSI pp-number
 * which is a digit followed by digits, letters, underscores, periods, e+, e-,
 * E+, or E-.
 */
char *skip_number(line)
register char *line;
{
	char *ch_mark = line;
	while(isalnum(*line) || *line == '_' || *line == '.')
	{
		if((*line == 'e' || *line == 'E') && (*(line+1) == '+' || *(line+1) == '-'))
			line++;
		line++;
	}
	/* DTS # CLLbs00249. klee 12/08/92
	 * Issue a warning if detect invalid preprocessing number such as 0xe+1.
	 */
	if (line > ch_mark && (strncmp(ch_mark,"0x",2) == 0 ||
		               strncmp(ch_mark,"0X",2) == 0))
	{
		register char *p = ch_mark+2;
		char *message;
		int len;
		while(isxdigit(*p))
			p++;
		if ((*(p-1) == 'e' || *(p-1) == 'E') && 
		    (*p == '+' || *p == '-'))
		{
			len = line-ch_mark;
			message = perm_alloc(len+1);
			strncpy(message, ch_mark, len);
			warning("Invalid preprocessing number '%s'",message);
		}
	}
	/* end DTS # CLLbs00249 */
	return line;
}

/*
 * Same as skip_number except it allows embedded NL_MARK's within the number.
 */
char *skip_def_number(line)
register char *line;
{
	while(isalnum(*line) || *line=='_' || *line=='.' || *line==NL_MARK)
	{
		if((*line == 'e' || *line == 'E'))
		{
			line++;
			while(*line==NL_MARK)
				line++;
			if (*line == '+' || *line == '-')
				line++;
		}
		else
			line++;
	}
	return line;
}

#ifdef hpe

/* This function breaks the time_parm (CLOCK format returned by FFILEINFO)
 * and date_parm (CALENDAR format returned by FFILEINFO) in order to build
 * a struct of type tm.
 */
struct tm break_time_date (time_parm, date_parm)
int time_parm;
unsigned short date_parm;
{
   unsigned short date_error[2];
   short month_num = 0;
   short day_num = 0;
   struct tm ret;

   union { int u_time;
           struct clock { unsigned int hour:8;
                          unsigned int minute:8;
                          unsigned int second:8;
                          unsigned int tenths:8;
                        } clock;
         } time_union;
   union { unsigned short u_date;
           struct calendar { unsigned int year:7;
                             unsigned int day:9;
                           } calendar;
         } date_union;

   ALMANAC(date_parm, &date_error,, &month_num, &day_num);

   time_union.u_time = time_parm;
   date_union.u_date = date_parm;
   ret.tm_sec = time_union.clock.second;
   ret.tm_min = time_union.clock.minute;
   ret.tm_hour = time_union.clock.hour;
   ret.tm_mday = day_num;
   ret.tm_mon = month_num-1;
   ret.tm_year = date_union.calendar.year;
   ret.tm_wday = -1;
   ret.tm_yday = -1;
   ret.tm_isdst = -1;
   return ret;
}
#endif /* #ifdef hpe */
