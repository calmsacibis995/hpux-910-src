/* @(#) $Revision: 70.1 $ */    
#ifndef hpux
#endif

#include "defs"
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <ar.h>
#include <sys/param.h>    	/* for MAXPATHLEN */
#ifdef hp9000s800
#include <lst.h>
#else
#include <ranlib.h>
#endif

#ifdef NLS16
#include <nl_ctype.h>
#endif

/* UNIX DEPENDENT PROCEDURES */

#define equaln    	!strncmp
#define MAXNAMLEN   255   /* this is in sys/dirent.h but its inclusion */
                          /* causes other conflicts, so define it here */
                          /* instead - monitor value of MAXNAMLEN and  */
                          /* change it here if it changes in header    */

         /* these two variables are used in other modules */

unsigned char    archmem[MAXNAMLEN]; /*archive file member name to search for*/
unsigned char    archname[MAXPATHLEN];	/*archive file to be opened*/

static struct ar_hdr arhead;    /* archive file header */
long int arpos, arlen;          /* position and length */

char * malloc();
char * trim();


TIMETYPE    afilescan();         /* defined below */
TIMETYPE    entryscan();         /* defined below */
FILE    	*arfd;



TIMETYPE
exists(pname)
NAMEBLOCK pname;
/*************************************************************/
/* The argument pname is a NAMEBLOCK structure containing    */
/* among other things a filename (target name).  This        */
/* function extracts the filename, determines if the file    */
/* is an archive library member or a regular file, stats the */
/* file, and RETURNS the date of last modification.  If the  */
/* file cannot be found, it will try the alias for that file */
/* also contained in the NAMEBLOCK                           */
/* If the target name is an archive library member construct */
/* a call to lookarch handles the filename and returns the   */
/* last modified time of the member specified                */ 
/*************************************************************/

{
    register CHARSTAR s;         
    struct stat buf;               /* hold result of stat call */
    TIMETYPE lookarch();           /* handle archive targets */
    CHARSTAR filename;             /* target name */

    filename = pname->namep;           /* extract target name */ 

    if(any(filename, LPAREN))          /* archive library member case */
    	return(lookarch(filename));    /* lookarch does the stat here */

    if(stat(filename,&buf) < 0)        /* the filename was not found     */ 
    {                                  /* put stat results into buf      */
    	s = findfl(filename);          /* get path to find file          */
    	if(s != (CHARSTAR )-1)         /* file was found in PATH         */
    	{
    		pname->alias = copys(s);  /*copy full pathname into NAMEBLOCK*/
    		if(stat(pname->alias, &buf) == 0)
    			return(buf.st_mtime); /* return modification time */
    	}
    	return(0);     /* file cannot be found anywhere */ 
    }
    else
    	return(buf.st_mtime);     /* successful stat of regular filename */ 
	                          /* return modification time            */
}


TIMETYPE
prestime()
/***************************/
/* Return system time      */
/***************************/

{
    TIMETYPE t;

    time(&t);
    return(t);
}



FSTATIC unsigned char tempbuf[MAXPATHLEN];


DEPBLOCK
#ifdef    PATH
srchdir(pat, mkchain, nextdbl, ispath)
#else    PATH
srchdir(pat, mkchain, nextdbl)
#endif    PATH
register CHARSTAR pat;    	/* pattern to be matched in directory */
int mkchain;    		/* nonzero if results to be remembered */
DEPBLOCK nextdbl;    	/* final value for chain */
/*******************************************************************/
/*	This function accesses the following global variables:
	firstpat, firstod(main), firstvar, hashtab.
	It does several things:
		1) it puts the parameter pat into firstpat if not already there
		2) it sets local variables dirpref, dirname, and filepat
		   according to whether or not pat has slashes in it
		3) it searches firstod to see if dirname is already open
		   and opens it if it isn't, adding its name to firstod
		4) it searches the opened directory for the file and if 
		   found, concatenates dirpref onto the filename and
		   inserts the filename into firstname and the hashtable
		   (via a call to srchname)
		5) if the mkchain parameter is NO, it allocates another
		   DEPBLOCK, puts the new nameblock containing the
		   filename into it, and hooks it onto nextdbl, which
		   as far as I can tell, is never used again by anything,
		   so why bother.  Either the new nameblock or a null
		   is returned.                                            */
/*******************************************************************/
{
#ifdef    PATH
    int ispath;		/* true when .PATH name is applied to pat */
#endif    PATH
    DIR * dirp;
    int i, nread;
    CHARSTAR dirname;           /* directory part of pat */
    CHARSTAR dirpref;           /* same as dirname? */
    CHARSTAR endir;             /* pointer to last slash in pat */
    CHARSTAR filepat;           /* filename portion of pat only */
    CHARSTAR p;
    unsigned char temp[MAXPATHLEN];
    unsigned char fullname[MAXPATHLEN];
    register CHARSTAR p1;
    register CHARSTAR p2;
    NAMEBLOCK q;
    DEPBLOCK thisdbl;           /* allocate new DEPBLOCK */
    register OPENDIR od;
    int dirofl = 0;             /* maximum open dir count flag */
    static opendirs = 0;        /* keep track of directories opened */
    register PATTERN patp;      /* allocate new PATTERN */

    struct direct *entry;      /* directory entries */


    thisdbl = 0;

	/* only return here if pat is already in firstpat */

    if(mkchain == NO)         
    	for(patp=firstpat ; patp!=0 ; patp = patp->nextpattern)
	    if(equal(pat,patp->patval))
    		return(0);               

	/* either mkchain is not NO, or pat was not in firstpat */
	/* hook pat onto front of firstpat list */

    patp = ALLOC(pattern);
    patp->nextpattern = firstpat;
    firstpat = patp;
    patp->patval = copys(pat);

    endir = 0;

#ifndef NLS16
    for(p=pat; *p!=CNULL; ++p)  /* find last slash in pat, if any */
#else
    /* NLS: advance ptr to first byte of new char, whether
    	or not its the first of 2-byte char's */
    for(p=pat; *p!=CNULL; ADVANCE(p))
#endif
    	if(*p==SLASH)
    	    endir = p;   /* endir points to last slash in pat */

    if(endir==0)         /* there were no slashes in pat */
    {
    	dirname = (unsigned char *) ".";   /*look in current directory*/
    	dirpref = (unsigned char *) "";
    	filepat = pat;
    }
    else          
    {
    	*endir = CNULL;              /* chop pat at last slash */
    	dirpref = concat(pat, "/", temp);  /* replace slash */
    	filepat = endir+1;          /* save rest of pat */
    	dirname = temp;             
    }

    dirp = NULL;

    for(od=firstod ; od!=0; od = od->nextopendir)
    	if(equal(dirname, od->dirn))
    	{
    	    dirp = od->dirp;    /* set directory pointer */
    	    rewinddir(dirp);    /* reset the directory */
    	    break;   
    	}

    if(dirp == NULL)            /* dirname not in open list */
    {
    	dirp = opendir(dirname);   /* so open it */
    	if(++opendirs < MAXODIR)
    	{
    	    od = ALLOC(opendir);
    	    od->nextopendir = firstod;   /* hook dir onto firstod */
    	    firstod = od;
    	    od->dirp = dirp;
    	    od->dirn = copys(dirname);
    	}
    	else
    	    dirofl = 1;          /* maximum number of dirs open */
    }

    if(dirp == NULL)           /* if dirname was not found */
    {
    	fprintf(stderr, "Directory %s: ", dirname);
    	fatal("Cannot open");
    }
    else
		while ((entry = readdir(dirp)) != NULL)
		{
			if(entry->d_ino!= 0)     /* file number */
			{
				p1 = (unsigned char *) entry->d_name;
				p2 = tempbuf;
				while( (*p2++ = *p1++)!=CNULL );  /*copy name into tempbuf*/
				if( amatch(tempbuf,filepat) )
				{
					concat(dirpref,tempbuf,fullname);
					if( (q=srchname(fullname)) ==0)
					{
						q = makename(copys(fullname));
#ifdef    PATH
						q->path = ispath;
#endif    PATH
					}
					if(mkchain)
					{
						thisdbl = ALLOC(depblock);
						thisdbl->nextdep = nextdbl;
						thisdbl->depname = q;
						nextdbl = thisdbl;
					}
				}
			}
		}

	if(endir != 0)
		*endir = SLASH;     /* replace slash taken out of dirp */
	if(dirofl)              /* if open directories max'd out   */
		closedir(dirp);     /* close this one                  */

	return(thisdbl);        /* DEPBLOCK containing ? */
}



amatch(s, p)
register char *s, *p;
{
    if (fnmatch(p,s,0))
    	return(0);
    else
    	return(1);
}

#ifdef NOT_DEFINED	/* the following code is no longer used */

/* stolen from glob through find */

amatch(s, p)
CHARSTAR s, p;
{
    register int cc, scc, k;
    int c, lc;

    scc = *s;
    lc = 077777;
    switch (c = *p)
    {
    case LSQUAR:
    	k = 0;
    	while (cc = *++p)
    	{
    	    switch (cc)
    	    {
    	        case RSQUAR:
    		    if (k)
    			return(amatch(++s, ++p));
    		    else
    			return(0);

    		case MINUS:
    		    k |= lc <= scc & scc <= (cc=p[1]);
    	    }
    	    if(scc==(lc=cc))
    		k++;
    	}
    	return(0);

    case QUESTN:
    caseq:
    	if(scc)
    	    return(amatch(++s, ++p));
    	return(0);
    case STAR:
    	return(umatch(s, ++p));
    case 0:
    	return(!scc);
    }                          /* end switch */

    if(c==scc)
    	goto caseq;
    return(0);
}

umatch(s, p)
register CHARSTAR s, p;
{
    if(*p==0)
    	return(1);
    while(*s)
    	if(amatch(s++,p))
    		return(1);
    return(0);
}

#endif /* NOT_DEFINED, the above code is no longer used */


#ifdef METERFILE
int meteron 0;    /* default: metering off */

meter(file)
CHARSTAR file;
{
    TIMETYPE tvec;
    CHARSTAR p, ctime();
    FILE * mout;
    struct passwd *pwd, *getpwuid();

    if(file==0 || meteron==0)
    	return;

    pwd = getpwuid(getuid());

    time(&tvec);

    if( (mout=fopen(file,"a")) != NULL )
    {
    	p = ctime(&tvec);
    	p[16] = CNULL;
    	fprintf(mout,"User %s, %s\n",pwd->pw_name,p+4);
    	fclose(mout);
    }
}
#endif




TIMETYPE
lookarch(filename)
register CHARSTAR filename;
/*********************************************************/
/* look inside archives for notations a(b) and a((b))    */
/*  a(b)	is file member   b   in archive a            */
/*  a((b))	is entry point   b  in object archive a      */
/*
	This function is called when it has been determined
	that filename contains at least one parenthesis.
	The filename string is parsed into archname and
	archmem, respectively.  archmem is stripped of any
	slashes and a local variable s is assigned the value
	of the actual archive member name.  This value is
	given to a function, la, which along with the flag
	value for whether or not we have an object archive,
	returns the time the archive member was last modified.
	This time is returned by this function.  

	The flag variable objarch is initialized to NO and
	set to YES if filename contains two left parens.
	The name of this variable seems counter-intuitive
	to me;  an object archive member is denoted with
	one set of parens; an entry point is denoted with
	two sets of parens;  it seems that the sense of this
	flag is backwards.  But, that is the way it was
	written.  See the function la for details on how
	the member time is retrieved.                        */
/*********************************************************/
{
    register int i;
    CHARSTAR p, q;
    unsigned char s[255];
    int nc, objarch;
    TIMETYPE la();

#ifndef NLS16
    for(p = filename; *p!= LPAREN ; ++p);   /* find first left paren */
#else
    /* NLS: advance char ptr to next char regardless of size of char */
    for(p = filename; *p!= LPAREN ; ADVANCE(p));
#endif
    i = p - filename;                /* count characters in archname part */
    strncpy(archname, filename, i);  /* extract archive name from filename */
    archname[i] = CNULL;            
    if(archname[0] == CNULL)
    	fatal1("Null archive name `%s'", filename);
#ifndef NLS16
    p++;
#else
    ADVANCE(p);		/* advance to start of next char */
#endif
    if(*p == LPAREN)    /* we have an entry point - found 2 left parens */
    {
    	objarch = YES;
#ifndef NLS16
    	++p;
    	if((q = strchr(p, RPAREN)) == NULL)   /* find right paren ? */
#else
    	ADVANCE(p);
    	if((q = nl_strchr(p, RPAREN)) == NULL)
    	/* use NLS version so as not to redefine 2-byte chars */
#endif
    		q = p + strlen(p);      /* mark end of string */
    	strncpy(s,p,q-p);
    	s[q-p] = CNULL;
    }
    else
    {
    	objarch = NO;     /* regular archive member */
#ifndef NLS16
    	if((q = strchr(p, RPAREN)) == NULL)
#else
    	if((q = nl_strchr(p, RPAREN)) == NULL)
#endif
    		q = p + strlen(p);
    	i = q - p;
    	strncpy(archmem, p, i);
    	archmem[i] = CNULL;
    	nc = 14;
    	if(archmem[0] == CNULL)
    	    fatal1("Null archive member name `%s'", filename);
#ifndef NLS16
    	if(q = strrchr(archmem, SLASH))
#else
    	if(q = nl_strrchr(archmem, SLASH))
#endif
    	    ++q;
    	else
    	    q = archmem;

	/*
	** New ar(1) supports long file names.  The 14 byte restriction
	** is no longer valid.  Just copy the string from q to s since
	** q is NULL terminated.
	*/
    	strcpy(s, q);
    }
    return(la(s, objarch));  /* return time of last modification of s */
}


TIMETYPE
la(am,flag)
register unsigned char *am;
register int flag;
/****************************************************************/
/*	This functions attempts to open the archive "archname" which
	is a global set in lookarch.  If the open fails, a long 0
	is returned.  If the open succeeds, the archive is scanned
	(by entryscan or afilescan) for the archive member name, am,
	and the date of last modification is returned.

	If flag indicates that am is a regular archive member, 
	afilescan is called to retrieve the last mod date.  If it
	indicates that am is an entry point member, entryscan is
	called to retrieve the last mod date.  In either case,
	the date returned is passed back by this function to lookarch.
	clarch() closes the archive file before the date is
	returned.                                                   */ 
/****************************************************************/
{
	TIMETYPE date = 0L;

	if(openarch(archname) == -1)
		return(0L);
	if(flag)
		date = entryscan(am);	/* fatals if cannot find entry */
	else
		date = afilescan(am);
	clarch();
	return(date);
}


TIMETYPE
afilescan(name)    	
unsigned char *name;
/*****************************************************************/
/* 	This function returns the last modified date for name, which
	is a member of a regular object archive file.  It does this
	by calling getarch in a loop, which puts the next member
	entry into this file's global variable arhead.  This entry
	is then checked to see if its name matches the parameter.
	If a match is found, the date from this entry is returned;
	otherwise, a 0 (float) is returned.                          */ 
/*****************************************************************/
{
    char *ntbl = NULL;
    size_t ar_size;
#ifdef NLS16
    int last_stat = ONEBYTE;
#endif
    int len = strlen(name);

    while(getarch())
    {
	/*
	** An archive can contain two special members, the symbol table and
	** the long file name table.  The special members both start with
	** a slash in ar_name[0].  The symbol table member is guaranteed to
	** be the first member in the archive if it exists.  Likewise the
	** long file name table immediately follows the symbol table if
	** it exists.
	**
	** The special member which contains the long name table has the
        ** following format:
        **
        **     +0  +1  +2  +3  +4  +5   +6  +7  +8  +9
        **    ------------------------------------------
        **  0 | t | h | i | s | i | s |  a | v | e | r |
        **    |----------------------------------------|
        ** 10 | y | l | o | n | g | f |  i | l | e | n |
        **    |-----------------------------------------
        ** 20 | a | m | e | . | o | / | \n |
        **    ------------------------------
        **
        ** each entry in the table is followed by one slash and
        ** a new-line character.  The offset of the table begins at zero.
        ** If an archive member name exceeds 15 bytes then the ar_name
        ** entry in the members header does not hold a name, rather the
        ** offset into the string table preceeded by a slash.
        **
        ** ex.  The member name thisisaverylongfilename.o will contain
        **      /0  for the ar_name value.  This value represents the
        **      offset into the string table.
        **
        */
        if (arhead.ar_name[0] == '/')
        {
	    /*
	    ** If the long name table exists then allocate memory for it
	    ** and read it in.  Search through the string table and convert
	    ** all slash's to NULL's so the that strings can be easily
	    ** accessed. 
	    */
            if (arhead.ar_name[1] == '/' &&
	        sscanf(arhead.ar_size, "%ld", &ar_size) == 1 &&
	        (ntbl=(char *)malloc(ar_size)) != NULL &&
                fread(ntbl, sizeof(char), ar_size, arfd) == ar_size)
            {
                char *ptr = ntbl;
                for( ;ptr <= ntbl + ar_size; ptr++)
                {
#ifndef NLS16
                    if (*ptr == '/')
                        *ptr = '\0';
#else
                    /* make sure SLASH is single byte char */
                    last_stat = BYTE_STATUS(*ptr, last_stat);
                    if (*ptr == '/' && last_stat == ONEBYTE)
                        *ptr = '\0';
#endif
                }
            } 
	    /*
	    ** If this member contains a digit following the slash it is
	    ** an index into the long name table rather than the name.
	    */
            else if (isdigit(arhead.ar_name[1]))
            {
               if (ntbl && equaln(ntbl + atol(&arhead.ar_name[1]), name, len))
	       {
		   free(ntbl);
	           return(atol(arhead.ar_date));
	       }
            }
        }
        else if (equaln(trim(arhead.ar_name), name, len) && 
            (len == sizeof arhead.ar_name || arhead.ar_name[len] == '\0'))
        {
		free(ntbl);
    	        return(atol(arhead.ar_date));
        }
    }
    free(ntbl);

    return(0L);
}


/****************************************************************/
/* 	The function entryscan, which retrieves the mod time of
	a kernel entry point from an archive library, is architecture
	dependent, hence the following three definitions for it     */
/****************************************************************/

#ifdef hp9000s800
TIMETYPE
entryscan(name)
char *name;
{
    int i, offset;
    char *tabstr;
    struct lst_symbol_record *tab, *tp;
    struct som_entry *somdir;
    struct lst_header lst_header;
    char *ntbl=NULL;
    size_t ar_size;

#ifdef NLS16
    int last_stat = ONEBYTE;
#endif
    getarch();

    /* make sure archive has a table of contents entry */
    if (strncmp(arhead.ar_name, DIRNAME, AR_NAME_LEN) != 0)
        fatal1("Library %s has no table of contents",archname);

    /* read in the LST header */
    offset = ftell(arfd);
    /*
    ** read in long file name table.  This special member follows
    ** the archive table of contents if members with long names are
    ** contained within the archive.
    ** 
    ** See comments for afilescan() for details regarding the format
    ** of the long file name table.
    */
    if (fseek(arfd, arpos, 0) == 0 &&
        fread(&arhead, sizeof(struct ar_hdr), 1, arfd) == 1 &&
	arhead.ar_name[0] == '/' && arhead.ar_name[1] == '/' &&
	sscanf(arhead.ar_size, "%ld", &ar_size) == 1 &&
	(ntbl = (char *)malloc(atol(ar_size))) != NULL &&
	fread(ntbl, sizeof(char), ar_size, arfd) == ar_size)
    {
	char *ptr = ntbl;
	for( ;ptr <= ntbl + ar_size; ptr++)
	{
#ifndef NLS16
	    if (*ptr == '/')
                *ptr = '\0';
#else
    	    /* make sure SLASH is single byte char */
    	    last_stat = BYTE_STATUS(*ptr, last_stat);
    	    if (*ptr == '/' && last_stat == ONEBYTE)
	        *ptr = '\0';
#endif
	}
    }

    fseek(arfd, offset, 0);

    if (fread((char *)&lst_header, sizeof(lst_header), 1, arfd) != 1)
        fatal1("read error in %s", archname);

    /* read in LST string area */
    tabstr = (char *)malloc(lst_header.string_size);
    if (tabstr == NULL)
        fatal1("no memory for library %s table of contents string table",
    	    archname);
    fseek(arfd, offset+lst_header.string_loc, 0);
    if (fread(tabstr, lst_header.string_size, 1, arfd) != 1)
        fatal1("read error in %s", archname);

    /* read in SOM directory */
    somdir = (struct som_entry *)malloc(lst_header.module_count *
    	sizeof(struct som_entry));
    if (somdir == NULL)
        fatal1("no memory for library %s directory", archname);
    fseek(arfd, offset+lst_header.dir_loc, 0);
    if (fread(somdir, sizeof(struct som_entry),
    	  lst_header.module_count, arfd) != lst_header.module_count)
        fatal1("read error in %s", archname);

    /* read in LST symbol directory */
    tab = (struct lst_symbol_record *)malloc(lst_header.export_count *
    	sizeof(struct lst_symbol_record));
    if (tab == NULL)
        fatal1("no memory for library %s symbol table", archname);
    fseek(arfd, offset+lst_header.export_loc, 0);
    if (fread(tab, sizeof(struct lst_symbol_record),
    	  lst_header.export_count, arfd) != lst_header.export_count)
        fatal1("read error in %s", archname);

    /* now search for the name */
    tp = tab;
    for (i = 0; i < lst_header.export_count; i++)
    {
       if (strcmp(name, tabstr+tp->name.n_strx) == 0)
       {
    	 fseek(arfd, somdir[tp->som_index].location - sizeof(struct ar_hdr), 0);
         fread(&arhead, sizeof(struct ar_hdr), 1, arfd);
         if (arhead.ar_name[0] == '/' && isdigit(arhead.ar_name[1]) && ntbl)
               strcpy(archmem, ntbl + atol(&arhead.ar_name[1]));
         else
         {
    	 	for(i = 0; i < 16; i++)
    	 	{
    	     		archmem[i] = arhead.ar_name[i];
#ifndef NLS16
    	     		if (archmem[i] == '/') archmem[i] = 0;
#else
    			/* make sure SLASH is single byte char */
    	     		last_stat = BYTE_STATUS( archmem[i], last_stat);
    	     		if (archmem[i] == '/' && last_stat == ONEBYTE)
    				archmem[i] = 0;
#endif
    		}
	}
	free(ntbl);
    	free(tab);
    	free(somdir);
    	free(tabstr);
    	return(atol(arhead.ar_date));
      }
      tp++;
    }

    free(ntbl);
    free(tabstr);
    free(somdir);
    free(tab);
    fatal1("cannot find symbol %s in archive %s", name, archname);
}
#endif hp9000s800

# ifdef hp9000s500
TIMETYPE
entryscan(name)    	/* return date of member containing global var named */
unsigned char *name;
{

    unsigned char *ran_name_pool;	/* pointer to name pool of ranlib
    				   table of contents */
    struct rl_ref *ran_table;	/* pointer to ranlib table of
    				   contents */
    struct rl_hdr ran_hdr;		/* ranlib header */
    int num_read;		/* number of bytes read in */
    int nsym;
    int i;
    unsigned char * t;

#ifdef NLS16
    int last_stat = ONEBYTE;
#endif

    getarch();
    
    /* make sure archive has a table of contents entry */
    if (strncmp(arhead.ar_name, RL_INDEX, AR_NAME_LEN) != 0)
    	fatal1("Library %s has no table of contents",archname);

    /* read in the ranlib header */
    num_read = fread((unsigned char *) &ran_hdr, 1, sizeof(ran_hdr), arfd);
    if (num_read != sizeof(ran_hdr))
    	fatal1("read error in %s", archname);

          /* read in ranlib table of contents */
    ran_table = (struct rl_ref *) malloc(ran_hdr.rl_tclen);
    if (ran_table == NULL)
    	fatal1("no memory for library %s table of contents", archname);
    fseek(arfd, ran_hdr.rl_tcbas, 0);
    num_read = fread((unsigned char *) ran_table, sizeof(struct rl_ref),
       ran_hdr.rl_tclen/sizeof(struct rl_ref), arfd);
    if (num_read != (ran_hdr.rl_tclen / sizeof(struct rl_ref)))
    	fatal1("read error in %s", archname);

    /* read in ranlib name table */
    ran_name_pool = malloc(ran_hdr.rl_nmlen);
    if (ran_name_pool == NULL)
    	fatal1("no memory for library %s table of contents name pool", archname);
    fseek(arfd,ran_hdr.rl_nmbas, 0);
    num_read=fread(ran_name_pool, sizeof(unsigned char), ran_hdr.rl_nmlen,arfd);
    if (num_read != ran_hdr.rl_nmlen)
    	fatal1("read error in %s", archname);

    nsym = ran_hdr.rl_tclen / sizeof(struct rl_ref);
    for(i = 0; i<nsym ; ++i)
    {
       t = (unsigned char *)(ran_name_pool + ran_table[i].name_pos);
       if (strcmp(name, t) == 0)
       {

    	fseek(arfd, ran_table[i].lib_pos, 0); 
    	fread(&arhead, sizeof(struct ar_hdr), 1, arfd);

    	for(i = 0; i < 16; i++)
    	{
    		archmem[i] = arhead.ar_name[i];
#ifndef NLS16
    		if (archmem[i] == '/') archmem[i] = 0;
#else
    		/* make sure SLASH is single byte char */
    		last_stat = BYTE_STATUS( archmem[i], last_stat);
    		if (archmem[i] == '/' && last_stat == ONEBYTE)
    			archmem[i] = 0;
#endif
    }
    	free(ran_table);
    	free(ran_name_pool);
    	return(atol(arhead.ar_date));
    	}
    	}

    free(ran_table);
    free(ran_name_pool);
    fatal1("cannot find symbol %s in archive %s", name, archname);

}

#endif

#ifdef hp9000s200

TIMETYPE
entryscan(name)    
unsigned char *name;
{

    unsigned char *tabstr;	/* pointer to name pool of ranlib table
    			   of contents */
    struct ranlib *tab;	/* pointer to ranlib table of contents */
    int rasciisize; 		/* size of string table */
    short rantnum; 			/* size of ranlib array */
    int num_read;		/* number of bytes read in */
    int i;			/* loop index */
    unsigned char * t;

#ifdef NLS16
    int last_stat = ONEBYTE;
#endif

    getarch();

    /* make sure archive has a table of contents entry */
    if (strncmp(arhead.ar_name, DIRNAME, AR_NAME_LEN) != 0)
    	fatal1("Library %s has no table of contents",archname);

    /* read in the ranlib header */
    num_read = fread((unsigned char *) &rantnum, 1, sizeof(rantnum), arfd);
    if (num_read != sizeof(rantnum))
    	fatal1("read error in %s", archname);

    num_read = fread((unsigned char *) &rasciisize, 1, sizeof(rasciisize), arfd);
    if (num_read != sizeof(rasciisize))
    	fatal1("read error in %s", archname);

    /* read in ranlib string table */
    tabstr = (unsigned char *) malloc(rasciisize);
    if (tabstr == NULL)
    	fatal1("no memory for library %s table of contents string table", archname);
    num_read = fread((unsigned char *) tabstr, 1, rasciisize, arfd);
    if (num_read != rasciisize)
    	fatal1("read error in %s", archname);

    /* read in ranlib table of contents */
    tab = (struct ranlib *) malloc(rantnum * sizeof(struct ranlib));
    if (tab == NULL)
    	fatal1("no memory for library %s table of contents", archname);
    num_read=fread((unsigned char *) tab, sizeof(struct ranlib), rantnum, arfd);
    if (num_read != rantnum) 
    	fatal1("read error in %s", archname);

    for (i = 0;  i < rantnum; i++) 
    {
       t = (unsigned char *) (tabstr + tab[i].ran_un.ran_strx);
       if (strcmp(name, t) == 0)
       {

    	fseek(arfd, tab[i].ran_off, 0); 
    	fread(&arhead, sizeof(struct ar_hdr), 1, arfd);

    	for(i = 0; i < 16; i++)
    	{
    				archmem[i] = arhead.ar_name[i];
#ifndef NLS16
    		if (archmem[i] == '/') archmem[i] = 0;
#else
    		/* make sure SLASH is single byte char */
    		last_stat = BYTE_STATUS( archmem[i], last_stat);
    		if (archmem[i] == '/' && last_stat == ONEBYTE)
    			archmem[i] = 0;
#endif
    		}
    	free(tab);
    	free(tabstr);
    	return(atol(arhead.ar_date));
    	}
    }

    free(tab);
    free(tabstr);
    fatal1("cannot find symbol %s in archive %s", name, archname);

}

#endif

openarch(f)
register CHARSTAR f;
/**********************************************************/
/*	Open archive file named by parameter f.
	
	A stat determines if the file exists.  If it does,
	its size is saved in arlen.  The file is then opened
	for reading.  One item of data SARMAG bytes long is
	read into arcmag and this string is compared to the
	constant ARMAG.  SARMAG and ARMAG are defined in
	/usr/include/ar.h.  If this match fails, the file
	opened is not an archive file.  A global (local to
	files.c) variable arpos records that we are SARMAG
	bytes into the file, and the function returns a 0,
	for success.

	This function will return a -1 (failure) if the
	archive file f cannot be stat'd or cannot be opened
	for reading.  If it fails the archive test, a fatal
	error message is output and make stops.               */
/**********************************************************/

{
    unsigned char arcmag[SARMAG+1];
    struct stat buf;

    if(stat(f, &buf) == -1)
    	return(-1);
    arlen = buf.st_size;

    arfd = fopen(f, "r");
    if(arfd == NULL)
    	return(-1);
    fread(arcmag, SARMAG, 1, arfd);
    if(strncmp(arcmag, ARMAG, SARMAG))
    	fatal1("%s is not an archive", f);

    arpos = SARMAG;
    return(0);
}

clarch()
/******************************************************/
/* Close the archive file in the global variable arfd */
/******************************************************/
{
    if(arfd != NULL)
    	fclose(arfd);
}

getarch()
/********************************************************/
/* 	Get the next archive member entry from the open 
	archive file specified in arfd, using the current
	position in the file specified by the global variable
	arpos.  Put the entry information into the global
	variable arhead and reset arpos to point to the
	next entry                                          */
/********************************************************/
{
    if(arpos >= arlen)
    	return(0);
    fseek(arfd, arpos, 0);
    fread(&arhead, sizeof(struct ar_hdr), 1, arfd);
    arpos += sizeof(struct ar_hdr);
    arpos += (atol(arhead.ar_size) + 1 ) & ~1L;
    return(1);
}

isdir(p)
unsigned char *p;
/********************************************************************/
/*    Used when unlinking files. If file cannot be stat'ed or it is */
/*    a directory, then do not remove it.                           */
/********************************************************************/
{
    struct stat statbuf;

    if(stat(p, &statbuf) == -1)
    	return(1);		/* no stat, no remove */
    if((statbuf.st_mode&S_IFMT) == S_IFDIR)
    	return(1);
    return(0);
}


char * trim(s)
char *s;
/***************************************************/
/* Removes slash and makes the string asciz.       */
/***************************************************/
{
    register int i;

#ifdef NLS16
    register int last_stat = ONEBYTE;
#endif

    for(i = 0; i < 16; i++)
#ifndef NLS16
       if (s[i] ==  '/') s[i] =  '\0';
#else
    {
       /* make sure SLASH is single byte char */
       last_stat = BYTE_STATUS(s[i], last_stat);
       if (s[i] ==  '/' && last_stat == ONEBYTE)
    	s[i] =  '\0';
    }
#endif
    return(s);
}
