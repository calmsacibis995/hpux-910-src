/* @(#) $Revision: 66.9 $ */      

#include "defs"
#include <sys/param.h>      /* for MAXPATHLEN */
#ifdef NLS16
#include <nl_ctype.h>
#endif


unsigned char Makecall;          /* flag which says whether to exec $(MAKE) */
extern unsigned char archmem[];  /* used for DBUG statements only */
extern unsigned char archname[]; /* used for DBUG statements only */


/*
p->done = 0   don't know what to do yet - new NAMEBLOCKS are thus initialized
p->done = 1   file in process of being updated
p->done = 2   file already exists in current state
p->done = 3   file make failed
*/


doname(p, reclevel, tval)
register NAMEBLOCK p;       /*target name to be "make"*/
int reclevel;               /*recursion level*/
TIMETYPE *tval;             /*modification time of target to be
							  passed back to caller*/
/*************************************************************/
/*  THE DONAME FUNCTION IS RECURSIVE in two places,
	once is Section 1, and once in Section 2. 
	See comments below.                                      */
/*************************************************************/
{
   register DEPBLOCK q;     /*used to find all dependents of target*/
   register LINEBLOCK lp;   /*used to search for target's dependents*/
                            /*and through sufflist in outer for loop*/
   int errstat;             /*capture status from recursive and docom calls*/
   int okdel1;
   int didwork;
   TIMETYPE td;          /*calculated modtime of dependent after recursing*/
   TIMETYPE td1;         /*modtime returned from dependent recursive calls*/
   TIMETYPE tdep;        /*calculated modtime of dependent(s)*/
   TIMETYPE ptime;       /*final calculated modtime of target returned in tval*/
   TIMETYPE ptime1;      /*holds modtime of target or present time*/
   DEPBLOCK qtemp;       /*never used*/
   DEPBLOCK suffp;       /*working pointer to actual suffixes in sufflist */ 
                         /*for outer for loop*/
   LINEBLOCK lp1;        /*working pointer to sufflist for inner for loop*/
   DEPBLOCK suffp1;      /*working pointer to actual suffixes in sufflist */
                         /*for inner for loop*/
   NAMEBLOCK p1;         /*pointer to built-in inference rule information*/
   NAMEBLOCK p2;         /*pointer to sourcefile info from hashtable*/
   SHBLOCK implcom;      /*rule strings from built-in rules for this target*/
   SHBLOCK explcom;      /*rule strings from makefile for this target*/
   LINEBLOCK lp2;        /*working pointer to implicit rule's linep's */

   unsigned char sourcename[MAXPATHLEN];  /*source corresponding to target*/
   unsigned char prefix[MAXPATHLEN];  /*target name minus any suffix*/
   unsigned char temp[MAXPATHLEN];    /*holds target name search pattern*/
   unsigned char concsuff[20];     /*name of internal rule being searched for*/

   CHARSTAR pnamep;      /*current suffix value from sufflist in outer for*/
   CHARSTAR p1namep;     /*current suffix value from sufflist in inner for*/
   CHAIN qchain;         /*list of dependents out of date with target*/
   int found;
   int onetime;
   CHARSTAR savenamep = NULL;       /* saves original target name if */
                                    /* it is a library archive name  */

#ifdef   PATH
   char thisname[100];
   register LINEBLOCK pp;
   DEPBLOCK pathp;
#endif   PATH


/***************/
/* BASE CASE 1 */
/***************/
   if(p == 0)      /* NAMEBLOCK parameter empty */ 
   {
      *tval = 0;
      return(0);
   }

   if(IS_ON(DBUG))
   {
      blprt(reclevel);
      printf("doname(%s,%d)\n",p->namep,reclevel);
      fflush(stdout);
   }

/***************/
/* BASE CASE 2 */
/***************/
   if(p->done > 0)  /* if file in processing somehow */
   {
      *tval = p->modtime;   /* return mod time of file */
      return(p->done == 3); /* return true only if file make failed */
                            /* this will set errstat in calling instance */
   }


/*********************************************************************/
/* BODY OF RECURSIVE FUNCTION
	This function has several main parts which are headed by comments
	below.  Recursion takes place in two places: in the dependency
	processing section, and in the suffix search section.            
	
	The basic gist of this function is to:
	1) Determine if the target has any dependencies and process those first,
	   and if they are out of date with respect to the current target
		keeping track of the name in qchain (which will set the value
		of the built-in macro ?)
	2) Obtain the commands from the makefile, if any, associated with
		the current target and either saving them in explcom (:)
		or executing them immediately (::)
	3) Obtain the first implicit rule from sufflist, if any, which
		matches the suffix of the target and has a sourcefile which
		also matches the rule and exists.  Save the commands for this
		implicit rule in implcom.	
	4) Execute the command for that target, under certain conditions.
		The conditions prevent infinite recursion.  Whether or not
		commands are executed, set certain fields in the target
		NAMEBLOCK and return the calculated modification time of
		the current target.  This will be used by the calling instance.

	The recursion is what allows the makefile to be "executed" in
	depth first order.                                               */

/*********************************************************************/



/***********************/
/* Initialization      */
/***********************/
   errstat = 0;
   tdep = 0;
   implcom = 0;
   explcom = 0;
   ptime = exists(p);    /* returns last time modified for target */

   if(reclevel == 0 && IS_ON(DBUG))
   {
      blprt(reclevel);
      printf("TIME(%s)=%ld\n", p->namep, ptime);
   }

   ptime1 = 0;
   didwork = NO;
   p->done = 1;   /* avoid infinite loops */

   qchain = NULL;  /*dependents out of date will accumulate here*/

/********************************************/
/*  Perform runtime dependency translations */
/* Expand on this comment after deciphering */
/* what dynamicdep does                     */
/********************************************/

   if(p->rundep == 0)
   {
      setvar("@", p->namep); /* set current target macro to target name*/
      dynamicdep(p);         /* expand all $'s in dependency lines */
      setvar("@", Nullstr);  /* clear current @ target             */
   }

/*********************************************/
/* Expand any names that have embedded       */
/* metacharacters. Must be done after dynamic*/
/* dependencies because the dyndep symbols   */
/* ($(*D)) may contain shell meta characters.*/
/*********************************************/

   expand(p);

/*******************************************************************/
/*  FIRST SECTION -- RECURSE ON DEPENDENCIES                       */
/*******************************************************************/

/*	The nested for loops search every lineblock  the target has
	and every dependent list in every lineblock for any dependents
	of this target and recurse on each dependent.  The recursion
	returns the modification time of the dependent, which is
	compared against the modification time of the target, and
	if the target was out-of-date with respect to the target,	
	its name is added to a list of names which becomes the value
	of the built-in macro ? for this target.  Any actions needed
	to update the dependents are done in the recursive calls.
	
	The target's separation character is then checked and if
	a double colon line is being processed, the commands
	associated with this target (if any) are immediately executed.
	Otherwise, the commands associated with this target
	(if any) are saved for further use in the third section
	of the code.  Error checking for more than one single colon
	line for this target with instructions is done here.           */
/*******************************************************************/

	if(IS_ON(DBUG))
	{
		blprt(reclevel);
		printf("look for explicit deps. %d \n", reclevel);
	}

	for(lp = p->linep ; lp!=0 ; lp = lp->nextline)  /* for each line   */
	{                                               /* of the nameblock*/
		td = 0;
		for(q = lp->depp ; q!=0 ; q=q->nextdep)
		{
			q->depname->backname = p;    /*for each dependent name*/
			errstat += doname(q->depname, reclevel+1, &td1); /*RECURSION*/
			if(IS_ON(DBUG))
			{
				blprt(reclevel);
				printf("TIME(%s)=%ld\n", q->depname->namep, td1);
			}
			td = max(td1,td);

            /* if dependent was out of date, add it to the list */
			if(ptime <= td1)
				appendq(&qchain, q->depname->namep);
		}

		/* DIFFENTIATE BETWEEN SINGLE AND DOUBLE COLON TARGET LINES */

		if(p->septype == SOMEDEPS)  /*double colon target line*/
		{
			if(lp->shp!=0)
				if( ptime<=td || (ptime==0 && td==0) || lp->depp==0)
				{
					okdel1 = okdel;
					okdel = NO;
					setvar("@", p->namep);
					if(savenamep)
						setvar("%", archmem);
					setvar("?", mkqlist(qchain) );
					qchain = NULL;
					if( IS_OFF(QUEST) )
					{
						ballbat(p, reclevel);
						errstat += docom(lp->shp);  /*DO COMMAND*/
					}
					setvar("@", Nullstr);
					setvar("%", Nullstr);
					okdel = okdel1;
					if( (ptime1 = exists(p)) == 0)
						ptime1 = prestime();  /*add value to global firstpat*/
					didwork = YES;
				}
		}
		else                            /* single colon target line */
		{
			if(lp->shp != 0)
			{
				if(explcom)            /* already got a command for target */
	 			   fprintf(stderr,"Too many command lines for `%s'\n",p->namep);
				else
					explcom = lp->shp;   /*save command from makefile*/
			}
			tdep = max(tdep, td);      /*calculate dependent's modtime*/
		}
	}

/***************************************************************/
/* SECOND SECTION - LOCATE APPROPRIATE INTERNAL RULE           */
/***************************************************************/

/*	If the target is a library archive name, separate the
	libary name and the member name and save the whole name
	for control flow checks later.  The new "target" is just
	the member name.  

	Make two passes through the .SUFFIXES list - the first
	pass succeeds if a suffix in the list matches the suffix
	of the target name.  At this point, if the target was
	an archive library form, the suffix just found is changed
	to a .a.  The main part of the target name (without a suffix)
	is then searched for (as *targetname.*) and all occurences
	of such files found in the appropriate directory for make
	are added to the hashtable.  

	Now the second search through the .SUFFIXES list begins.
	Using the suffix of the target as the second part of
	the string, each suffix from .SUFFIXES is appended as the
	first part of the string, and a built-in rule matching
	this pattern is searched for.  (For instance, if the suffix
	of the target is .o, rules .o.o, .c.o etc are searched for)
	When such a rule is found that has associated instructions
	(suffix rules can be empty) the suffix from the second
	loop is tacked onto the main target name and the hashtable
	is consulted to see if such a file exists.  If it doesn't,
	the code returns to the inner for loop and continues the search.

	The second instance of recursion takes place if a rule
	is found whose second suffix matches the target's and whose
	first suffix matches an existing file.  A sourcefilename
	is then constructed from the target name and the first suffix
	and recursion on this name takes place.  This recursion
	returns the last modtime of the sourcefile and the sourcename
	is added to the "out-of-date-with-respect-to-target" list.
	Internal macros * and < are then set and the suffix rule
	is consulted for its instructions, which are saved in
	"implcom".  

	If the first for loop never succeeds (the target's suffix does
	not match any in the .SUFFIXES list), things are set up
	to institute a search for a single suffix rule instead of a
	double and the code goto's just before the inner for loop.
	
	At this point, implcom may or may not contain instructions
	to execute for this target.                                */
/***************************************************************/

	if(IS_ON(DBUG))
	{
		blprt(reclevel);
		printf("look for implicit rules. %d \n", reclevel);
	}

#ifndef   PATH
	found = 0;    /* will indicate if target's suffix is found in .SUFFIXES */ 
	onetime = 0;
#endif   PATH

	/* if target is an archive library name, save whole name and */
	/* reset p->namep to the member portion only */
	if(any(p->namep, LPAREN))
	{
		savenamep = p->namep;
		p->namep = copys(archmem);
		if(IS_ON(DBUG))
		{
			blprt(reclevel);
			printf("archmem = %s\n", archmem);
		}

		if(IS_ON(DBUG)) 
		{
			blprt(reclevel);
			printf("archname = %s\n", archname);
		}
	}
   else
      savenamep = 0;


#ifdef   PATH

	found = 0;
	thisname[0] = 0;
	for(pp=pathlist ; pp!=0 ; pp = pp->nextline) 
		for(pathp = pp->depp ; pathp!=0 ; pathp = pathp->nextdep)
		{
			onetime = 0;
			if (p->namep[0] == '/' || p->path != 0)
			{
            /* absolute names: no prefix; bail out after first */
				if (thisname[0] != 0)
					goto impldone;
				copstr(thisname, p->namep);
			}
			else
				if (equal(pathp->depname->namep, "."))
					copstr(thisname, p->namep);
				else
				{
					concat(pathp->depname->namep, "/", thisname);
					concat(thisname, p->namep, thisname);
				}
#endif   PATH


	/* sufflist is a global LINEBLOCK linked list which */
	/* holds the list of .SUFFIXES defined in           */
 	/* the rules.c file and/or the makefile. sufflist is*/
	/* filled by the parser                             */

	/* OUTER FOR LOOP - find a match with the target's suffix */

	for(lp=sufflist ; lp!=0 ; lp = lp->nextline)
		for(suffp = lp->depp ; suffp!=0 ; suffp = suffp->nextdep)
		{
			pnamep = suffp->depname->namep;

#ifdef   PATH
			if(suffix(thisname, pnamep , prefix))
#else   PATH
			if(suffix(p->namep , pnamep , prefix))
#endif   PATH
			{
			/* you are here if the suffix in pnamep was found at the end */
			/* of the target name, p->namep.  Prefix will contain*/
			/* the target name minus the suffix                  */ 

				if(IS_ON(DBUG)) 
				{
					blprt(reclevel);
#ifdef   PATH
					printf("right match = %s\n",thisname);
#else   PATH
					printf("right match = %s\n",p->namep);
#endif   PATH
				}

				found = 1;
				if(savenamep)
 					pnamep = (unsigned char *) ".a";

searchdir:

				copstr(temp, prefix);
				addstars(temp);       /* add wild card stars so that */
                                    /*every file with this base name */
	                                /*will be found                  */

			/* srchdir will put temp into the global var
	           "firstpat" if it is not already there, determines what
			    directory to look for the files in temp in, and opens it if
			    necessary, searches the directory for the files, and if it
			    finds any, inserts the full pathnames of the files into
			    "firstname" and the hashtable (via a call to srchname). If
			    the second parameter is not NO, it allocates another
	   			DEPBLOCK, puts the new NAMEBLOCK containing the filename
			    into it, and returns a pointer to the NAMEBLOCK.  If mkchain
			    is NO, a NULL pointer is returned. */

#ifdef   PATH
				srchdir( temp , NO, NULL, 1);
#else   PATH
				srchdir( temp , NO, NULL);
#endif   PATH

				/* INNER FOR LOOP - look for a suffix rule which */
				/* matches the target suffix and has a corresponding */
				/* sourcefile */

				for(lp1 = sufflist ; lp1!=0 ; lp1 = lp1->nextline)
					for(suffp1=lp1->depp ; suffp1!=0 ; suffp1 = suffp1->nextdep)
					{
						p1namep = suffp1->depname->namep;
						concat(p1namep, pnamep, concsuff);
						if( (p1=srchname(concsuff)) == 0)
							continue;      /*no rule with these suffixes*/
						if(p1->linep == 0)
							continue;  /*rule exists but has no instructions*/

						/* you are here if you have found a good */
						/* suffix rule that matches the target's suffix */

						concat(prefix, p1namep, sourcename);
						/* SCCS stuff */
						if(any(p1namep, WIGGLE))
						{
							sourcename[strlen(sourcename) - 1] = CNULL;
							if(!sdot(sourcename))
								trysccs(sourcename);
						}
						if( (p2=srchname(sourcename)) == 0)
							continue;

					/*you are here if source file matching the */
					/*first suffix of the rule exists          */
#ifdef   PATH
						if(equal(sourcename, thisname))
#else   PATH
						if(equal(sourcename, p->namep))
#endif   PATH
							continue;

			/* you are here if the target's suffix, a matching */
			/* rule, and the corresponding sourcefile have all */
			/* been located                                    */

						found = 2;

						if(IS_ON(DBUG))
						{
							blprt(reclevel);
							printf("%s ---%s--- %s\n",
#ifdef   PATH
							sourcename, concsuff, thisname);
#else   PATH
							sourcename, concsuff, p->namep);
#endif   PATH
						}

						p2->backname = p;
						errstat += doname(p2, reclevel+1, &td); /*RECURSE*/
						if(ptime <= td)   /*add sourcename to out-of-date list*/
							appendq(&qchain, p2->namep);

						if(IS_ON(DBUG))
						{
							blprt(reclevel);
							printf("TIME(%s)=%ld\n",p2->namep,td);
						}

						tdep = max(tdep, td);
						setvar("*", prefix);
						setvar("<", sourcename);
						for(lp2=p1->linep ; lp2!=0 ; lp2 = lp2->nextline)
							if(implcom = lp2->shp)
								break;     /* get commands for rule */
										   /* from suffix NAMEBLOCK */
						goto endloop;
					}

				if(onetime == 1)   /* search for single suffix rule */
	                               /* has been done - don't do it again */
					goto endloop;
			}
		}

endloop:

#ifdef   PATH
		if (found == 2) goto impldone;
#endif   PATH


/********************************************************************/
/*  THIS NESTED if IS EXECUTED ONLY IF OUTER FOR LOOP FAILS ON ALL  
	SUFFIXES in the .SUFFIXES list

	Look for a single suffix type rule, but only if the target has
	no dependents and no shell rules associated with it, and if      
	nothing has been done so far.  Previously, `make' would exit with
	'Don't know how to make ...' message.                           */
/********************************************************************/

	if (found == 0)
		if (onetime == 0)
			if (p->linep == 0 || (p->linep->depp == 0 && p->linep->shp == 0))
			{
				onetime = 1;
				if(IS_ON(DBUG))
				{
					blprt(reclevel);
					printf("Looking for Single suffix rule.\n");
				}
#ifdef   PATH
				concat(thisname, "", prefix);
#else   PATH
				concat(p->namep, "", prefix);
#endif   PATH
				pnamep = (unsigned char *) "";
				goto searchdir;
			}

#ifdef   PATH
   }

impldone:
#endif   PATH


/**************************************************************/
/*  THIRD SECTION -- POSSIBLY DO COMMAND AND SET TARGET VALUES*/
/**************************************************************/


/*
	Commands are done only if no recursive calls reported
	errors, and the modification time of any of the dependents
	was more recent that that of the target, or if the
	modtime of all dependents and the target are 0, indicating
	that (what?).  If these conditions are met, built-in
	macros are set in preparation for executing command lines
	which might refer to them.  Don't know what ballbat does
	yet.  The execution sequence for the target is one of (in order):
	explicit commands from makefile, implicit commands from 
	suffix rule, .DEFAULT action, or the infamous error message.  */ 



	if(errstat==0 && (ptime<=tdep || (ptime==0 && tdep==0) ) )
	{
		if(savenamep)                /* set up built-in macro values */
		{                            /* for this target              */
			setvar("@", archname);
			setvar("%", archmem);
		}
		else
			setvar("@", p->namep);
		setvar("?", mkqlist(qchain) );
		ballbat(p, reclevel);

		if(explcom)                 /* execute makefile rule */
			errstat += docom(explcom);
		else
			if(implcom)             /* or execute built in rule */
				errstat += docom(implcom);
			else
				if( (p->septype != SOMEDEPS && IS_OFF(MH_DEP)) ||
					(p->septype == 0        && IS_ON(MH_DEP) )    )

/* OLD WAY OF DOING TEST is                           */
/*      else if(p->septype == 0)                      */
/* notice above, a flag has been put in to get the    */
/* murray hill version.  The flag is "-b".            */
/* This cryptic comment refers to the above if statement */
/* whose function in life is to prevent further action   */
/* in the case of a double colon line (I think)          */

					if(p1=srchname(".DEFAULT"))  /*execute default rule*/
					{
						if(IS_ON(DBUG))
						{
							blprt(reclevel);
							printf("look for DEFAULT rule. %d \n", reclevel);
						}
						setvar("<", p->namep);
						for(lp2=p1->linep ; lp2!=0 ; lp2 = lp2->nextline)
							if(implcom = lp2->shp)
								errstat += docom(implcom);
					}
					else                   /*NO INSTRUCTIONS FOR THIS TARGET*/
						if(IS_OFF(GET) || !get(p->namep, NOCD, 0) )
							fatal1(" Don't know how to make %s", p->namep);

		setvar("@", Nullstr);
		setvar("%", Nullstr);
		if((ptime = exists(p)) == 0)
		{
			ptime = prestime();
#ifndef   PATH
			didwork = YES;
#endif   PATH
		}
	}
	else                              /* don't do any commands */
		if(errstat!=0 && reclevel==0)
			printf("`%s' not remade because of errors\n", p->namep);
		else
			if(IS_OFF(QUEST) && reclevel==0  &&  didwork==NO)
				printf("`%s' is up to date.\n", p->namep);

	/* the following will be done for every call to doname */  

	if(IS_ON(QUEST) && reclevel==0)
		exit(ndocoms>0 ? -1 : 0);
	p->done = (errstat ? 3 : 2);
	ptime = max(ptime1, ptime);
	p->modtime = ptime;
	*tval = ptime;
	setvar("<", Nullstr);
	setvar("*", Nullstr);
	return(errstat);
}



/******************************/
/* FUNCTIONS USED BY DONAME   */
/******************************/

docom(q)
SHBLOCK q;   /* linked list of strings to be executed */
/***************************************************************/
/*	This function processes each string in the linked list q  
	either until one of the lines causes terminates with an
	error or until all lines have been processed.  The global
	variable "ndocoms" is incremented here.  After checking
	various flags (noex, touch, etc) each line is inspected
	in turn to see if it begins with one of "-", "-@", "@-",
	"@", or "+" and the appropriate flags are reset.  

	"-" means ignore errors from this line
	"@" means suppress output of the command line before exec'ing
	"+" means override the -n (no print) option for this line 

	The ignore "-" and "@" flag values are given as arguments
	to docom1, which actually causes the string s to be executed
	(in yet one more level of indirection).  

	In the loop which executes each command line, a test is made
	for the result of each execution.  (Fill this in when you
	know what docom1 returns).

	If all the command lines are executed successfully, the loop
	terminates and this functions returns a 0.                 */
/***************************************************************/
{
	CHARSTAR s;
	int status;
	int ign;          /* -i option or "-"command indicator */
	int nopr;         /*to print or not to print commands before exec'ing*/
	unsigned char string[OUTMAX]; /*local buffer to manipulate command strings*/

	++ndocoms;
	if(IS_ON(QUEST))
		return(0);

	if(IS_ON(TOUCH))              /* either just touch files */
	{
		s = varptr("@")->varval;   /* get value of current target */
		if(IS_OFF(SIL))
			printf("touch(%s)\n", s);
		if(IS_OFF(NOEX))
			touch(1,s);
	}
   else                         /* or really do the command(s) */ 
      for( status = 0; q!=0 ; q = q->nextsh )   /* for each string, unless */
      {                                         /* one of them craps out */

	      /* Allow recursive makes to execute only if the NOEX flag set */
		if ( ((sindex(q->shbp, "$(MAKE)") != -1)   ||
		      (sindex(q->shbp, "${MAKE}") != -1))
				&& IS_ON(NOEX)
           )
			Makecall = YES;
		else
			Makecall = NO;

		/* Expand macros in command string */
		subst(q->shbp,string,&string[OUTMAX]); 

		ign = IS_ON(IGNERR) ? YES : NO;
		nopr = NO;           /*default is to print commands before exec'ing*/

      /* check beginning of command string for "-", "@", or "+" signs  */
      /* PLUS feature added for POSIX conformance for 8.0              */  

#ifndef NLS16
		for(s = string ; *s==MINUS || *s==AT || *s==PLUS ; ++s)
#else
		for(s = string ; *s==MINUS || *s==AT || *s==PLUS ; ADVANCE(s))
#endif
			if(*s == MINUS)
				ign = YES;
			else
				if(*s == PLUS)
					Makecall = YES;
				else                   /* AT */
					nopr = YES;

		if( docom1(s, ign, nopr) && !ign)   /*individually exec each string*/
			if(IS_ON(KEEPGO))
				return(1);     
			else
				fatal(0);
   }
   return(0);
}



docom1(comstring, nohalt, noprint)
register CHARSTAR comstring;
int nohalt, noprint;
/**************************************************************/
/* more processing of command lines and the actual call which */
/* processes them                                             */
/**************************************************************/
{
	register int status;

	if(comstring[0] == '\0') return(0);  /*empty command line*/

	/*************************/
	/* PRINT OUT THE COMMAND */
	/*************************/

	if(IS_OFF(SIL) && (!noprint || IS_ON(NOEX)) )
	{

		CHARSTAR p1, ps;               /* local if variables */
		CHARSTAR pmt = prompt;     

		ps = p1 = comstring;
		while(1)
		{
			while(*p1 && *p1 != NEWLINE) p1++;
			if(*p1)
			{
				*p1 = 0;
				printf("%s%s\n", pmt, ps);
				*p1 = NEWLINE;
				ps = p1 + 1;
				p1 = ps;
			}
			else
			{
				printf("%s%s\n", pmt, ps);
				break;
			}
      }

      fflush(stdout);
   }

	/*********************************************************/
	/* ACTUALLY DO THE COMMAND AND CHECK THE RETURNED STATUS */
	/*********************************************************/

	if( status = dosys(comstring, nohalt) )
	{
		if( status>>8 )
#ifdef hp9000s500
			printf("*** Error exit code %d", status>>8 );
		else   /* mask high (i.e. "core dump") bit and print */
			printf("*** Termination signal %d", status );
#else
			printf("*** Error code %d", status>>8 );
		else
			printf("*** Termination code %d", status );
#endif

    /*********rewrite above calls to use new macros instead of */
    /* all the shifting junk                                   */

		if(nohalt)
			printf(" (ignored)\n");
		else
			printf("\n");
		fflush(stdout);
	}

	return(status);
}



expand(p)
NAMEBLOCK p;
/*****************************************************************/
/*   If there are any Shell meta characters in the name,         */
/*   search the directory, and if the search finds something     */
/*   replace the dependency in "p"'s dependency chain. srchdir   */
/*   produces a DEPBLOCK chain whose last member has a null      */
/*   nextdep pointer or the NULL pointer if it finds nothing.    */
/*   The loops below do the following: for each dep in each line */
/*   if the dep->depname has a shell metacharacter in it and     */
/*   if srchdir succeeds, replace the dep with the new one       */
/*   created by srchdir. The Nextdep variable is to skip over    */
/*   the new stuff inserted into the chain.                      */
/*   (These are old comments - I will change/add to them when    */
/*    I more fully understand                                    */
/*****************************************************************/
{
   register DEPBLOCK db;
   register DEPBLOCK Nextdep;
   register CHARSTAR s;
   register DEPBLOCK srchdb;
   register LINEBLOCK lp;



   for(lp = p->linep ; lp!=0 ; lp = lp->nextline)
      for(db=lp->depp ; db!=0 ; db=Nextdep )
      {
         Nextdep = db->nextdep;
         if(any( (s=db->depname->namep), STAR) ||
            any(s, QUESTN) || any(s, LSQUAR) )
#ifdef   PATH
            if( srchdb = srchdir(s , YES, NULL, 0) )
#else   PATH
            if( srchdb = srchdir(s , YES, NULL) )
#endif   PATH
               dbreplace(p, db, srchdb);
      }
}

dbreplace(np, odb, ndb)
register NAMEBLOCK np;
register DEPBLOCK odb, ndb;
/*********************************************************************/
/* Replace the odb depblock in np's dependency list with the         */
/* dependency chain defined by ndb. This is just a linked list insert*/
/* problem. dbreplace assumes the last "nextdep" pointer in          */
/* "ndb" is null. (old comments)                                     */
/*********************************************************************/
{
   register LINEBLOCK lp;
   register DEPBLOCK  db;
   register DEPBLOCK  enddb;

   for(enddb = ndb; enddb->nextdep; enddb = enddb->nextdep);

   for(lp = np->linep; lp; lp = lp->nextline)
      if(lp->depp == odb)
      {
         enddb->nextdep   = lp->depp->nextdep;
         lp->depp   = ndb;
         return;
      }
      else
      {
         for(db = lp->depp; db; db = db->nextdep)
            if(db->nextdep == odb)
            {
               enddb->nextdep   = odb->nextdep;
               db->nextdep   = ndb;
               return;
            }
      }
}


#define NPREDS 50

ballbat(np, reclevel)
NAMEBLOCK np;
/***************************************************************/
/* isn't this wonderfully mnemonic?                            */
/*	This function is called by doname (twice) just after the
	built-in macros (%,@,<,?) are set and just before docom
	is invoked with the appropriate command strings for this
	target.  np is the target NAMEBLOCK from doname, and
	reclevel is the recursion level count from doname.
	reclevel is never accessed here, so I don't know why
	it is passed in.

	The purpose of this function is to check that the target
	about to be recursed on is neither too deep in a nested
	definition in the makefile (limit is 50 recursive calls)
	or is not part of a circular definition in the makefile.
	I.E. if target1 depends on target2 and target2 depends
	on target3 and target3 depends on target1, this circular
	and endless recursion will be nipped in the bud right here.
	Every target up to the detection of the circularity is
	executed, but infinite recursion is prevented.
	How this is accomplished is not clear to me yet.

	A side effect of this function is the accumulation of all
	target names in the recursive chain from this target back to
	the original target which set off the first recursion.  These
	names accumulate in the macro value for "!", which looks
	like a ballbat if you are a lousy hitter and have a perverted
	sense of programming humor.  This must have something to do
	with how the circular recursion is nipped in the bud, but
	neither the macro value for ! nor the variable ballb is accessed
	anywhere else in any code, so I don't see why this is being used 
	at all.  

	Not sure why ! is chosen as the variable in which to
	accumulate this data, unless whatever it inherited from
	the shell is never needed by make, and so it is a safe
	place to accumulate information.

	The error messages to stderr have been changed from the
	original, singularly unhelpful comments which were originally
	provided.  "predecessor circle" is not going to mean much
	to a user of make!	
/***************************************************************/
{
	static unsigned char ballb[1024];  /*persistent variable*/
	register CHARSTAR p;               /*working pointer into ballb*/
	register NAMEBLOCK npp;            /*working pointer to np*/
	register int i;                 
	VARBLOCK vp;                    /*working pointer into firstvar*/
	int npreds=0;                   /*counter for number of backname pointers*/
	                                /*from this target*/ 
	NAMEBLOCK circles[NPREDS];      /*array of targets to be checked*/


	/* I assume this "if" passes only the first time this function is  */
	/* called.  But what if "!" had something in it from the shell     */
	/* when make was called?                                         */
	/* The purpose of this "if" is not clear to me                     */
 
	if( *((vp=varptr("!"))->varval) == 0)
		vp->varval = ballb;

	p = ballb;                    /*set working pointer to ballb*/
	p = copstr(p, varptr("<")->varval);  /*copy current targets out of date*/
	p = copstr(p, " ");


	/* follow backname pointers from current target to first target */
	/* which kicked off this recursive chain                        */

	for(npp = np; npp; npp = npp->backname) 
	{
		for(i = 0; i < npreds; i++)
		{
			if(npp == circles[i])    /*back target already found once*/
			{
				fprintf(stderr,"circular definition STOPPED at %s\n",np->namep);
				ballb[0] = CNULL;
				return;
			}
		}
		circles[npreds++] = npp;   /*new back target name - add to array*/
		if(npreds >= NPREDS)       /*check recursion level*/
		{
			fprintf(stderr, "%d levels of recursion stopped at %s\n", NPREDS, np->namep);
			ballb[0] = CNULL;
			return;
		}
		p = copstr(p, npp->namep); /*copy target name into ballb*/
		p = copstr(p, " ");
	}
}



blprt(n)
register int n;
/**********************************************************/
/* PRINT n BLANKS WHERE n IS THE CURRENT RECURSION LEVEL  */
/* Used for debugging purposes only                       */
/**********************************************************/
{
   while(n--)
      printf("   ");
}
