/* @(#) $Revision: 70.8 $ */

/*
 wordexp --  performs word expansions and places the list of expanded words 
             into the structure pointed to by pwordexp.
 wordfree -- frees any memory associated with pwordexp from a previous call 
	     to wordexp().
 External Dependency: 
   1. /bin/echo: -q option to add backslash before each backslash and 
		 each <blank> that occurs within an argument
   3. /bin/posix/sh: exit with the following status:
		     3 - when one of the unquoted characters |,&,;,<,>,(,),{,} 
			 appears in its parameters
                     2 - shell syntax error
*/

#ifdef _NAMESPACE_CLEAN
#define wordexp _wordexp
#define wordfree _wordfree
#define close   _close
#define execlp  _execlp
#define fcntl   _fcntl
#define fflush  _fflush
#define freopen _freopen
#define getenv  _getenv
#define pipe    _pipe
#define read    _read
#define strcat  _strcat
#define strcpy  _strcpy
#define strdup  _strdup
#define strlen  _strlen
#define fork   _fork
#define waitpid _waitpid
#   ifdef _ANSIC_CLEAN
#	define malloc _malloc
#	define free _free
#   endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <wordexp.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#define _WRDE_SHELL_BADVAL 1		/* these are shell exit status */
#define _WRDE_SHELL_SYNTAX 2
#define _WRDE_SHELL_BADCHAR 3

#define _WRDE_CHILD_EXIT 4		/* if our child exit return this */

#define _WRDE_ECHO_CMD "/bin/echo -q "
#define _WRDE_SHELL_PATH "/bin/posix/sh"
#define _WRDE_SHELL_NAME "sh"


typedef struct listnode {
	struct listnode *next;
	char *buf;
} LISTNODE;

typedef struct list {
	int count;			/* how many list nodes are there */
	LISTNODE *first, *last;
} LIST;

static LIST *put();
static char *delete();
static int  parseWord();
static LIST *makeList();
static int  putWord();
static void disposeWordList();
static char *getword();

static int wrdeAppend();

#ifdef _NAMESPACE_CLEAN
#undef wordexp
#pragma _HP_SECONDARY_DEF _wordexp wordexp
#define wordexp _wordexp
#endif /* _NAMESPACE_CLEAN */
int wordexp(words, pwordexp, flags)
const char *words;
wordexp_t *pwordexp;
int flags;
{
	int pipefd[2];
	pid_t pid;

	if  (flags & WRDE_APPEND) 
	{
		int i;
		wordexp_t wordAppend;

		wordAppend.we_offs=0;
		if (i=wordexp(words, &wordAppend, flags&(~(WRDE_DOOFFS|WRDE_APPEND))))
	   		return(i);
		return(wrdeAppend(pwordexp, &wordAppend, flags));
	}

	if (flags & WRDE_REUSE)
	{
		wordfree(pwordexp);
		return(wordexp(words, pwordexp, flags&(~WRDE_REUSE)));
	}
	

	/*
	   preprocess words for tasks which posix shell won't do
	   1. if WRDE_NOCMD is set and there is a command substitution
	      in words, return WRDE_CMDSUB
	   2. if one of the following characters: |, &, ;, >, <, (, ), {, },
	      appear in an inappropriate context
	      i.e |, &, ;, >, <, (, ), {, } not within command substitution
		  or {, } not within variable expansion
        */
	{
	int ret;
	char *end;
	end=words+strlen(words);
	if (ret=parseWord(words, end, flags)) {
		if ((flags & WRDE_NOCMD) && ret == WRDE_CMDSUB)
			return WRDE_CMDSUB;
		return WRDE_BADCHAR;
	}
	}

	if (pipe(pipefd) == -1)
		return WRDE_INTERNAL;
	switch(pid=fork()) {
	case -1:
		return WRDE_INTERNAL;
	case 0: {
		char *shell_opt;
		char command[_POSIX_ARG_MAX];
		char *ifs_pointer=getenv("IFS");

		command[0]='\0';
		/* 
		  IFS is not inherited by sub-shell; therefore, we must
		  explicitly export it
		*/
		if (ifs_pointer != NULL)
		{
			(void) strcat(command, "export IFS=\"");
			(void) strcat(command, ifs_pointer);
			(void) strcat(command, "\"; ");
		}
		(void) strcat(command, _WRDE_ECHO_CMD);
		(void) strcat(command, words);
		close(0);
		close(1);
		if ((flags & WRDE_SHOWERR) == 0)        /* show error to stderr */
			freopen("/dev/null", "w", stderr);
		else
			fflush(stderr);
		close(pipefd[0]);
		if (fcntl(pipefd[1], F_DUPFD, 1) != 1)
			_exit(_WRDE_CHILD_EXIT);
		shell_opt=(flags&WRDE_UNDEF?"-cu":"-c"); /* shell returns error with -u */
		execlp(_WRDE_SHELL_PATH, _WRDE_SHELL_NAME, shell_opt, command, (char *) 0);
		_exit(_WRDE_CHILD_EXIT);
		}
	default: 
		{
		LIST *list;
		int status;
		close(pipefd[1]);
		list=makeList(pipefd[0]);
		close(pipefd[0]);
		waitpid(pid, &status, 0);
		if (list == NULL)
			return WRDE_NOSPACE;
		if (status == -1 || !WIFEXITED(status)) {
			disposeWordList(list);
			return WRDE_INTERNAL;
		}
		switch (WEXITSTATUS(status)) {
		case _WRDE_SHELL_BADVAL:
			disposeWordList(list);
			return WRDE_BADVAL;
		case _WRDE_SHELL_SYNTAX:	
			/* must scan for badchar because our shell
			doesn't differeniate badchar and wrong syntax */
			disposeWordList(list);
			return WRDE_SYNTAX;
		case _WRDE_SHELL_BADCHAR:
			disposeWordList(list);
			return WRDE_BADCHAR;
		case _WRDE_CHILD_EXIT:
			return WRDE_INTERNAL;
		default:
			break;
		}
		return(putWord(list, pwordexp, flags));
		}
	}
}

#ifdef _NAMESPACE_CLEAN
#undef wordfree
#pragma _HP_SECONDARY_DEF _wordfree wordfree
#define wordfree _wordfree
#endif /* _NAMESPACE_CLEAN */
void wordfree(pwordexp)
wordexp_t *pwordexp;
{
	int i, j;

	for (i=0; pwordexp->we_wordv[i]==NULL ; i++)  /* seek passed offset */
		;
	for (j=0; j < pwordexp->we_wordc; j++)        /* free each word     */
		free(pwordexp->we_wordv[i+j]);	
	free(pwordexp->we_wordv);                     /* free the word list */
}

/*
  makeList -- creates a list of expanded words via file descriptor, fd
*/
static LIST *makeList(fd)
int fd;
{
	char *word;
	LIST *list;
	
	if ((list=put(NULL, NULL)) == NULL) 		/* get a empty list */
		return NULL;
	while ((word=getword(fd)) != NULL) 
		if (put(list, word) == NULL) {
			disposeWordList(list);
			return NULL;
		}
	return list;
}

/*
   match -- if ch is found between start and end return offset from start
	    else return 0;
*/
static int match(start, end, ch)
char *start, *end;
char ch;
{
	char *ptr;
	for (ptr=start+1; ptr < end; ptr++) {
		if (*ptr == ch)
			return (ptr-start);
	}
	return 0;
}

/*
	parseWord -- look for bad characters and command substition
*/
static int parseWord(words, end, flags)
char *words, *end;
int flags;
{
	int i;
	char prev;
	char *tmp;
	char *ptr;

	for (prev=' ', ptr=words; ptr < end; prev=*ptr, ptr++)
	{
		switch(*ptr) {
		case '\'':
			/* 
			   single quote string is taken literally;
			   therefore, we don't need to do this recursively
                        */
			ptr+=match(ptr, end, '\'');
			break;
		case '\\':
			/* 
			   if we have odd number of slashes
			      we should skip the next escaped character
			   else
			      process as normal
			*/
			for (i=-1, tmp=ptr+1; tmp < end; tmp++) 
				if (*tmp != '\\') {
					i=(((tmp-ptr) % 2)?1:-1);
					break;
				}
			ptr=tmp+i;	
			break;
		case '"':
			/*
			   parse embedded string recursively because
			   $, `, and \ have special meaning
			   i.e the string "abc $(pwd) def" contains
			       a command substitution while
			       'abc $(pwd) def' does not
			*/
			if (i=match(ptr, end, '"'))
				if (flags & WRDE_NOCMD)
					if (parseWord(ptr+1, ptr+i, flags) == WRDE_CMDSUB)
						return WRDE_CMDSUB;
			ptr+=i;
			break;
		case '(':
			if (prev == '$' && (i=match(ptr, end, ')'))) {
				if (flags & WRDE_NOCMD)
					return WRDE_CMDSUB;
				ptr+=i;
				break;
			}
			return WRDE_BADCHAR;
		case '{':
			if (prev == '$' && (i=match(ptr, end, '}'))) 
			{
				ptr+=i;
				break;
			}
			return WRDE_BADCHAR;
		case '`':
			if (i=match(ptr, end, '`')) {
				if (flags & WRDE_NOCMD)
					return WRDE_CMDSUB;
				ptr+=i;
			}
			break;
		case ')':
		case '}':
		case '|':
		case '&':
		case ';':
		case '<':
		case '>':
			return WRDE_BADCHAR;
		default:
			break;
		}
	}
	return 0;
}

/*
  wrdeAppend -- append w2 to w1
*/
static int wrdeAppend(w1, w2, flags)
wordexp_t *w1, *w2;
int flags;
{
	int i, j;
	int offset;
	char **wordv;
	
	offset=(flags&WRDE_DOOFFS?w1->we_offs:0) + w1->we_wordc;
	if ((wordv=(char **) 
		malloc((offset+w2->we_wordc+1) * sizeof(char **)))==NULL)
	{
	   wordfree(w2);
	   return WRDE_NOSPACE;
	}
	for (i=0; i < offset; i++)
	   wordv[i]=w1->we_wordv[i];
	for (j=0; j<w2->we_wordc; j++)
	   wordv[i++]=w2->we_wordv[j];
	wordv[i]=NULL;
	free(w2->we_wordv);
	w1->we_wordc += w2->we_wordc;
	free(w1->we_wordv);
	w1->we_wordv=wordv;
	return 0;
}

/*
  putWord -- real word expansion routine
	LIST list: contains a list of expanded words
  	wordexp_t *pwordexp: the expanded word data structure
  	int flags:           flags for wordexp
  Return Value: If successfully put words return 0, 
		else return one of the following non-zero values:
		WRDE_NOSPACE: out of memory
		
*/
static int putWord(list, pwordexp, flags)
LIST *list;
wordexp_t *pwordexp;
int flags;
{
	int i,j,offset;
	offset=flags&WRDE_DOOFFS?pwordexp->we_offs:0;
	if ((pwordexp->we_wordv= (char **) malloc((offset+list->count+1) 
	   * sizeof(char **))) == NULL) {
		disposeWordList(list);
		return WRDE_NOSPACE;
	}
	for (i=0; i<offset; i++)
		pwordexp->we_wordv[i]=NULL;
	for (j=list->count; i-offset<j; i++)
	{
		pwordexp->we_wordv[i]=delete(list);
	}
	pwordexp->we_wordc=i-offset;
	pwordexp->we_wordv[i]=NULL;
	return 0;			 
}

/*
  getword -- read a word from input file fd.
             A word/field is separated by an unescaped blank; two unescaped
             blanks together represent a null field in between.
             The routine returns a pointer to a word or NULL (if it
             reaches the end of file or an input error occurs)
*/
static char *getword(fd)
FILE *fd;
{
	char ch;
	char buf[_POSIX_ARG_MAX];
	char *w=buf;
	int esq=0;
	int rt;
	
	while ((rt=read(fd, &ch, 1)) > 0) {
		if ((ch == ' ' || ch == '\\') && esq ) 
		{
			esq=0;
			w--;
		}
		else if (ch == '\\') 
			esq=1;
		else if (ch == ' ')
			break;
		else
			esq=0;
		*w++=ch;
	}
	*w='\0';
	/* if (not a null string) or (just read a blank) */
	if (strlen(buf) || (rt > 0 && ch == ' '))
		return((char *)strdup(buf));
	else 
		return NULL;
}

/*
  disposeWordList -- free the entire list of words
*/
static void disposeWordList(l)
LIST *l;
{
	char *word;
	if (l != NULL) {
		while ((word=delete(l)) != NULL)
			free(word);
	}
}

/* 
   put(l,w) - put word, w into the end of list, l
	      if l==NULL create a new list
*/
static LIST *put(l, w)
LIST *l;
char *w;
{
	if (l == NULL) {
		if ((l= (LIST *) malloc(sizeof(LIST))) == NULL)
			return NULL;
		l->count=0;
		l->first=l->last=NULL;
		return l;
	}	
	if (l->first== NULL) {
		if ((l->first = (LISTNODE *) malloc(sizeof(LISTNODE))) == NULL)
			return NULL;
		l->last=l->first;			
	}
	else {
		if ((l->last->next = (LISTNODE *) malloc(sizeof(LISTNODE))) == NULL)
			return NULL;
		l->last=l->last->next;
	}
	l->last->next=NULL;	
	l->last->buf=w;
	l->count++;
	return l;
}

/*
   delete(l) -- delete the first word in list, l
*/
static char *delete(l)
LIST *l;
{
	char *s;
	LISTNODE *ptr;

	if (l != NULL && l->first != NULL) {
		ptr=l->first;
		s=l->first->buf;
		l->first=l->first->next;
		free(ptr);
		l->count--;
		if (l->first == NULL)
			free(l);
		return(s);	
	}
	return NULL;
}
