/*
	This is a new output routine for the second pass of spellprog.
	What it does is perform the equivalent of:

	   cat incorrect_spellings | sort | comm -23 - local_spellings |
	   tee $HFILE
	   who am i >> $HFILE
	
	The routine init_comm() should be called with the filename
	of the local spelling file before any output.

	For each line of output from spellprog, the routine spell_out()
	should be called.

	When all output from spell is completed, the routine sort_and_dump()
	is called to sort the output, and filter it with the local file
	(if it exists).  This outputs the data to both stdout and the
	history file and will also do a "who am i" to the history file.

	Edgar Circenis
	UDL Commands
	9/7/90
*/
#include	<stdio.h>
#include	<pwd.h>

#define WORD_LENGTH 256
#define BSIZE 4096
static char	*default_hist_file_name="/usr/lib/spell/spellhist";
static char	local_spelling[WORD_LENGTH];
static FILE	*local_file,*hist_file;
static char	*hist_file_name;
static int	no_hist_file;
static int	local_spelling_exists;
static int	local_eof;

static char		*buffer;
static unsigned int	buffer_offset;
static unsigned int	buffer_size;

static char		**pointer_buffer;
static unsigned int	pointer_buffer_offset;
static unsigned int	pointer_buffer_size;

void init_comm(local_file_name)
char *local_file_name;
{
	local_spelling_exists=0;
	local_eof=0;
	if ((local_file=fopen(local_file_name,"r")) == NULL)
		exit(2);
	no_hist_file=0;
	hist_file_name=(char *)getenv("H_SPELL");
	if (hist_file_name==0)
		hist_file_name=default_hist_file_name;

	if (*hist_file_name==0)
		no_hist_file=1;
	else if ((hist_file=fopen(hist_file_name,"a+")) == NULL)
		no_hist_file=1;

	buffer=(char *)malloc(BSIZE);
	if (!buffer) exit(2);
	buffer_size=BSIZE;
	buffer_offset=0;

	pointer_buffer=(char **)malloc(BSIZE*4);
	if (!pointer_buffer) exit(2);
	pointer_buffer_size=BSIZE;
	pointer_buffer_offset=0;
}

void spell_out(word)
char *word;
{
	int len;

	len=strlen(word)+1;
	if (buffer_offset+len > buffer_size) {
		buffer=(char *)malloc(BSIZE);
		if (!buffer) exit(2);
		buffer_size=BSIZE;
		buffer_offset=0;
	}
	strcpy(&buffer[buffer_offset],word);
	if (pointer_buffer_offset >= pointer_buffer_size) {
		pointer_buffer_size+=BSIZE;
		pointer_buffer=(char **)realloc(pointer_buffer,pointer_buffer_size*4);
		if (!pointer_buffer) exit(2);
	}
	pointer_buffer[pointer_buffer_offset++] = &buffer[buffer_offset];
	buffer_offset+=len;
}

static int scmp(a,b)
char **a,**b;
{
	return(strcoll(*a,*b));
}

void sort_and_dump()
{
	int i;

	qsort(pointer_buffer,pointer_buffer_offset,sizeof(char *),scmp);
	for (i=0;i<pointer_buffer_offset;i++)
		comm_out(pointer_buffer[i]);
	who_am_i();
}

static comm_out(output_spelling)
char *output_spelling;
{
	int i;

	if (local_eof) {
		output_word(output_spelling);
		return;
	}
	if (!local_spelling_exists) {
		if (read_spelling(local_file,local_spelling) == -1) {
			output_word(output_spelling);
			local_eof=1;
			return;
		}
	}
	if ((i=strcoll(output_spelling,local_spelling)) < 0) {
		output_word(output_spelling);
		return;
	} else if (i==0) {
		local_spelling_exists=0;
	} else /* i>0 */ {
		while (1) {
			if (read_spelling(local_file,local_spelling) == -1) {
				output_word(output_spelling);
				local_eof=1;
				return;
			}
			if ((i=strcoll(output_spelling,local_spelling))>0) {
				continue;
			} else if (i<0) {
				output_word(output_spelling);
				return;
			} else /* i==0 */ {
				local_spelling_exists=0;
				return;
			}
		}
	}
}


static output_word(word)
char *word;
{
	printf("%s\n",word);
	if (!no_hist_file)
		fprintf(hist_file,"%s\n",word);
}


static int read_spelling(file,buf)
FILE *file;
char *buf;
{
	register int i,c;

	local_spelling_exists=1;
	i = 0;
	*buf='\0';
	while((c=getc(file)) != EOF) {
		*buf = c;
		if (c == '\n' || i > WORD_LENGTH-2) {
			*buf = '\0';
			return(0);
		}
		i++;
		buf++;
	}
	if (i) return 0;
	else return -1;
}


char *my_cuserid(s)
char *s;
{
	char *p;
	struct passwd *pw,*getpwuid();
	char *getlogin();

	p = getlogin();
	if (p != NULL)
		return (char *)strcpy(s, p);
	pw = getpwuid(getuid());
	_endpwent();
	if (pw != NULL)
		return (char *)strcpy(s, pw->pw_name);
	*s = '\0';
	return (NULL);
}


who_am_i()
{
	char *tty,*tstr,name[256];
	char *nl_ascxtime(),*ttyname();
	long t;

	if ((tty = ttyname(fileno(stdin))) == NULL &&
		(tty = ttyname(fileno(stdout))) == NULL &&
		(tty = ttyname(fileno(stderr))) == NULL)
		tty="/dev/unknown";
	tty+=sizeof("/dev/")-1;
	my_cuserid(name);
	t=time(0);
	tstr=nl_ascxtime(localtime(&t),"%b %d %H:%M");
	if (!no_hist_file)
		fprintf(hist_file,"%-10s %-12s %s\n",name,tty,tstr);
}


