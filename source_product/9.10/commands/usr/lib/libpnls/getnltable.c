


#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <langinfo.h>
#include <nlinfo.h>
#include <nl_types.h>
#include <nl_ctype.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif


/*
 *	get collate table for a language
 *
 *	1. open the collate8 file.
 *	2. get the starting address and the length of the collate table.
 *	3. allocate memory to hold the collate table.
 *	4. get the collate table.
 *	5. close the collate8 file.
 *
 *	NOTE:
 *		this routine is responsible for allocating the memory for the
 *		collate table. this was done because of two reasons.
 *
 *		1. the size of the collate table may vary.
 *		2. unless the user actually needs this information,
 *		   the space will be conserved.
 *
 *	layout for class three language collating sequence table
 *
 *		0. Table length
 *		1. Language ID
 *		2. Language Class (= 3)
 *		3. Pointer to sequence table (= 11)
 *		4. Length of sequence table (= 256)
 *		5. Pointer to 2:1 mapping table
 *		6. Length of 2:1 mapping table
 *		7. Pointer to 1:2 mapping table
 *		8. Length of 1:2 mapping table
 *		9. lowest char/highest char
 *		10. reserved
 *		11-? The rest of the file
 *	
 */
extern int _nl_errno;
getcollate(langName, pLength, pBuff, err)
char	*langName;
short	*pLength;
char    **pBuff;
short   err[];
{
	int	 fd;
	short	 col_length;
	char     *malloc(), *gBuff;
	long	 lseek();
	void	 free() ;


	err [0] = err [1] = 0;
        col_length = 512;  /* default length for collation tables */

	if ((gBuff =  malloc((unsigned int)64)) == NULL) {
	     err[0] = E_SYSTEMERR;
	     return;
	}

        if (strncmp (langName,"n-computer",10) == 0) {

		/* 
		 *  nlinfo shouldn't call this routine 
		 *  when language is n-computer.
		 *
		 *  The following code was added to allow
		 *  users to access the collation table
		 *  faster than by using nlinfo 11 or 27
		 *
		 */

		gBuff [0] = '\0';
		gBuff [1] = '\3';    /* Length of the collating table */
		gBuff [2] = '\0';
		gBuff [3] = '\0';    /* Language of the talbe */
		gBuff [4] = '\0';
		gBuff [5] = '\1';    /* Collation class (binary) */
	        *pBuff = gBuff ;
	        *pLength = (unsigned short) 3;
	        return;
        }

	sprintf(gBuff, "/usr/lib/nls/%s/collate8", langName);
	if ((fd = open(gBuff, O_RDONLY)) == -1){
		err[0] = E_LNOTCONFIG;
		return;
	}


	/*
	 * get the the length of the collate table
	 */

	if (read(fd, (char *)&col_length,  (unsigned)sizeof(short)) < 
	     (unsigned)sizeof(short) || lseek(fd, 0, 0) == -1L) {
		err[0] = E_SYSTEMERR;
		return;
	}


	/*
	 * allocate enough space for the collate table 
	 *	and read it into the buffer
	 */
	if (gBuff != NULL) free (gBuff);
	gBuff = malloc(2*col_length);

	if (gBuff == NULL) {
		err[0] = E_SYSTEMERR;
		return;
	}
	if ((read(fd, gBuff, (2* (unsigned int)col_length)) == -1 ) ||
	    (close(fd) == -1)) {
		err[0] = E_SYSTEMERR;
		return;
	}

	*pBuff = gBuff;
	*pLength = (unsigned short) col_length;
	
	return;
}




/* 
 *	get the chardef, upshift and downshift tables 
 *
 *	NOTE:
 *		unlike the collate table, this routine does not have to
 *		open the environment file to get its' information. on the
 *		contrary, if we were to call nl_init() to get this info,
 *		we would be adding additional i/o if we were going from one
 *		language to another.
 */
gettables(n)
struct l_info *n;
{
	int	byteValue;
	char	*chardefs,
		*upshift,
		*downshift;

	chardefs = n->char_set_definition;
	upshift = n->upshift;
	downshift = n->downshift;

	for (byteValue = 0; byteValue < SIZE_CHARSET; byteValue++){
		*upshift++   = toupper(byteValue);
		*downshift++ = tolower(byteValue);
		if (n->char_size == TWOBYTECHAR  &&  FIRSTof2(byteValue))
			*chardefs++ = NLI_FIRSTOF2;
                 /*
                 *  WARNING
                 *
                 *  kludge to force katakana to be upper case alpha
                 */

                else if ((n->langid ==221) &&
                            (byteValue > 160) && (byteValue < 224))
                                          *chardefs++ = NLI_UPPER;

		else if (isdigit(byteValue))
			*chardefs++ = NLI_NUMERIC;
		else if (islower(byteValue))
			*chardefs++ = NLI_LOWER;
		else if (isupper(byteValue))
			*chardefs++ = NLI_UPPER;
		else if (isgraph(byteValue))
			*chardefs++ = NLI_SPECIAL;
		else if (iscntrl(byteValue))
			*chardefs++ = NLI_CONTROL;
		else
			*chardefs++ = NLI_GRAPH;
	}
	/*
	 *  WARNING
	 *
	 *  kludge to force the escape character meaning as NLI_CONTROL
	 */
	*(n->char_set_definition + 32) = NLI_CONTROL;
}




/*
 *	get the language id
 *	this routine first checks to see if the user is asking for the langid
 *	by supplying the name or the ascii equivalent to the langid.
 */
getlangid(langName, langid)
char	*langName;
short	*langid; 
{
	char	*w, buff[17];
	char    *idtolang ();
	int	checkByName;

	checkByName = FALSE;
	
	/*
	 *	build a null terminated string with what the user supplied.
	 *	also check if user supplied a name or a number
	 */


	w = buff;
	while ((w != buff + sizeof(buff) - 1) && ((*w = *langName++) != ' ')
	                                      &&  (*w != '\0')) {

		if (!isdigit(*w++))
			checkByName = TRUE;
        }
	*w = '\0';


	_nl_errno = 0;
	if (checkByName == TRUE) *langid = (short)langtoid(buff);
	else {
	      if ((*langid = (short)atoi(buff)) != 0)  {
	          /* now check if the language is configured   */
	          /* note that n-computer is always configured */
                  if (!strcmp (*idtolang(*langid),""))
		      _nl_errno = E_LNOTCONFIG;

              }
        }
}


/*
 *	get the languages supported
 *
 *	WARNING: this routine is Unix specific
 */
getlangs(sBuff, err)
short         **sBuff;
short unsigned 	err[];
{
	FILE	*fp, *fopen(), *popen();
	int	languages;
	short	*langBuff;
	char	*calloc();




	/*
	 *	get the number of languages supported
	 */
	if ((fp = popen("wc -l /usr/lib/nls/config", "r")) == NULL  ||
			fscanf(fp, "%d%*s", &languages) == EOF){
		err[0] = E_SYSTEMERR;
		return;
	}
	pclose(fp);






	/*
	 *	allocate enough space for the array
	 */
	if ( (langBuff = (short *)calloc((unsigned)(languages + 1), 
				(unsigned)sizeof(short))) == (short *)NULL){
		err[0] = E_SYSTEMERR;
		return;
	}





	langBuff[0] = 0;

	if ((fp = fopen("/usr/lib/nls/config", "r")) == NULL){
		free (langBuff);
		err[0] = E_SYSTEMERR;
		return;
	}


	while (fscanf(fp, "%hd%*s", &langBuff[langBuff[0] + 1]) != EOF)
		if (++langBuff[0] == languages)
			break;
	fclose(fp);



	*sBuff = langBuff;

	return;
}

