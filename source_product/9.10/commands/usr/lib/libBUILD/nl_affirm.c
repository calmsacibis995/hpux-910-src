/* $Revision: 66.1 $ */
/*
    nl_affirm()

    This routine takes a user input string and sees if it matches 
    YES or NO in a the current locale.  It uses the LC_MESSAGES and
    YESEXPR and NOEXPR portions of the locale if they are available.
    If these are not available, the routine attempts to use regular
    expression strings passed to it (in paramYes and paramNo).  If
    these parameters are NULL, then default C locale regular expressions
    are used.

    The regular expression library routines regcomp (3C) and regexec (3C)
    are used in this routine.  If errors occur during these calls, the 
    return codes from regcomp (3C)/regexec (3C) are passed back to the 
    calling program with no further information.  The assumption is made 
    here that the error values for these two routines are disjoint sets.

    Possible return values from nl_affirm (besides those from regcomp (3C)
    and regexec (3C)) are:
       
       -1:	Could not get a regular expression for NO although one was
		  obtainable for YES.  This occurs if the YES expression
		  can be obtained from the locale but the NO expression
		  cannot, or if the YES expression can be obtained from the
		  parameters but the NO expression cannot.
       
	0:	The user response was NEGATIVE.

	1:	The user response was AFFIRMATIVE.

	2:	The user response was AMBIGUOUS.  This might occur if the
		  regular expressions for YES and NO allow matches to both
		  for some number of characters and the user response is not
		  long enough to resolve the ambiguity.  Unlikely, but you
		  never know.

        3:	The user response was NEITHER affirmative or negative.
    
    This routine remembers "state" to some extent.  If the LC_MESSAGES
    environment variable changes from one call to nl_affirm to the next,
    then the expressions from the previous call are "invalidated" and new
    expressions are obtained and compiled.  Otherwise this work can be
    skipped.  This scenario may be unlikely, but there is at least one
    known program that can execute multiple setlocale calls that might
    change the program locale.

    If the LC_MESSAGES environment variable isn't set, then another method
    has to be used to remember the "state".  In this case, the parameters
    passed for the YES and NO regular expressions are used.  If these are
    different from the last pair passed to nl_affirm, the expressions and
    their compilations are "invalidated" as discussed above.  This case
    should be the exception, not the rule, since this parameter capability
    was provided in order to support pre-LC_MESSAGES implementations.  
    Therefore the assumption was made that the parameters could be saved
    in a static area of a limited size (200 bytes).
*/

#ifdef _NAMESPACE_CLEAN
#define nl_affirm _nl_affirm
#endif

#include <nl_types.h>
#include <langinfo.h>
#include <regex.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#define FALSE	0
#define TRUE	1

#define NEGATIVE	0
#define AFFIRMATIVE	1
#define AMBIGUOUS	2
#define NEITHER		3
#define NO_EXPR		-1

/*  If YESEXPR and NOEXPR are not defined in the NLS include files (langinfo.h
    to be specific), then re-define them as -1.  This will cause nl_langinfo
    to return a pointer to an empty string since the -1 index is out of range.
*/
#ifndef YESEXPR
#   define YESEXPR	-1
#endif /* YESEXPR */

#ifndef NOEXPR
#   define NOEXPR	-1
#endif /* NOEXPR */

#ifdef _NAMESPACE_CLEAN
#undef nl_affirm
#pragma _HP_SECONDARY_DEF _nl_affirm nl_affirm
#define nl_affirm _nl_affirm
#endif

/***************************************************************************/
int nl_affirm (userInput, paramYes, paramNo)
  unsigned char *userInput;
  unsigned char *paramYes;
  unsigned char *paramNo;
/***************************************************************************/
{
/*  Space for the yes and no parameters passed into this routine so that
    they can be saved for the next invocation of nl_affirm.
*/
  static unsigned char saveParamYes [200] = "";
  static unsigned char saveParamNo [200] = "";

/*  Pointers for the yes and no expresssions; static so that they can be saved 
    for the next invocation of nl_affirm.
*/
  static unsigned char *yesExprPtr = NULL;
  static unsigned char *noExprPtr = NULL;

/*  Space for the saved value of LC_MESSAGES; static so that it can be saved
    for the next invocation of nl_affirm.  If the saved version is not 
    different from the current version, then the saved values of the 
    expressions as well as the compiled versions are still valid.  Also 
    space for a boolean to decide whether or not we ever saved the LC_MESSAGES
    value.
*/
  static char saveLCMESS [SL_NAME_SIZE];
  static int savedLCM = FALSE;
  char *currLCMESS;

/*  Variables to decide if there is a match with the yes expression, no
    expression, both, or neither.
*/
  short matchY = FALSE;
  short matchN = FALSE;

/*  Space for the compiled regular expressions; static so that it can be
    saved for the next invocation of nl_affirm.
*/
  static regex_t yesRcomp;
  static regex_t noRcomp;

/*  Return values for regcomp and regexec.
*/
  int regcompReturn;
  int regexecReturn;

/*  Variables to decide whether or not the expressions need to be obtained
    and/or compiled; static so the values can be saved for the next invocation
    of nl_affirm.
*/
  static int gotExpr = FALSE;
  static int compiledExpr = FALSE;

/*  Default regular expressions that are only used when the regular expressions
    can't be obtained using nl_langinfo and they haven't been passed into the
    routine via paramYes and paramNo.
*/
  static unsigned char defaultYes [] = "y|ye|yes";
  static unsigned char defaultNo [] = "n|no";

/*  Get the current LC_MESSAGES environment variable.  If there is one, then
    if it is different from the saved value we need to get the expressions
    again and compile them again.  If savedLCM is FALSE, this means that 
    we never saved a value for LC_MESSAGES or that this is the first time
    through the routine.  In this case, just save the value of LC_MESSAGES
    and then set savedLCM to TRUE.

    If savedLCM is TRUE, then we have a saved value which needs to be
    compared to the current value.  If they are the same then the expressions
    are still valid.  However, if they aren't, then we need to get the 
    expressions again and re-compile them.  This is accomplished by setting
    the booleans driving getting and compiling expressions to NULL.  In
    addition, we need to free up the space taken by the previous expressions
    once they were compiled.
*/
  currLCMESS = getenv ("LC_MESSAGES");

  if (currLCMESS != NULL) 
    {
      if (savedLCM == FALSE)
	{
	  (void) strcpy (saveLCMESS, currLCMESS);
	  savedLCM = TRUE;
	}
      
      else
	{
	  if ((strcoll (saveLCMESS, currLCMESS)) != 0)
	    {
	      (void) strcpy (saveLCMESS, currLCMESS);
	      yesExprPtr = NULL;
	      noExprPtr = NULL;
	      gotExpr = FALSE;

	      if (compiledExpr)
	        {
	          regfree (&yesRcomp);
	          regfree (&noRcomp);
	          compiledExpr = FALSE;
	        }
	    }
	}
    }

/*  If there is no LC_MESSAGES environment variable, then the next location
    to check for expressions is in the parameters passed to this routine.
    However, if the parameters change from one call to nl_affirm to the
    next, then they need to be re-compiled.  Although "remembering" via the
    LC_MESSAGES environment variable is preferable, until this is implemented
    the "remembering" has to be done via the parameters.

    So if either of the parameters is different from the saved values, then
    the current expressions and compiled expressions are no longer valid.
    In this case save the new expressions and set the booleans that direct
    them and their compilations to FALSE.  In addition, we need to free up the
    space taken by the previous expressions once they were compiled.
*/
  else
    {

/*  First check to see if the Yes expression is different.  If so, assume
    both saved expressions are invalid.
*/
      if ((strcoll (saveParamYes, paramYes)) != 0)
	{
	  (void) strcpy (saveParamYes, paramYes);
	  (void) strcpy (saveParamNo, paramNo);

	  yesExprPtr = NULL;
	  noExprPtr = NULL;
	  gotExpr = FALSE;

	  if (compiledExpr)
	    {
	      regfree (&yesRcomp);
	      regfree (&noRcomp);
	      compiledExpr = FALSE;
	    }
	}
      
/*  If the Yes expression was the same, check the No expression.  If it is
    different, do the work of getting the expressions and compiling them
    again.  This isn't necessary for the Yes expression, but it isn't very
    costly.
*/
      else if ((strcoll (saveParamNo, paramNo)) != 0)
	{
	  (void) strcpy (saveParamNo, paramNo);

	  yesExprPtr = NULL;
	  noExprPtr = NULL;
	  gotExpr = FALSE;

	  if (compiledExpr)
	    {
	      regfree (&yesRcomp);
	      regfree (&noRcomp);
	      compiledExpr = FALSE;
	    }
	}
    }
  
/*  Get the regular expressions if they haven't been obtained already.
    The first location to look for these is in the locale, via the YESEXPR
    and NOEXPR indicies into the language data area.  If this fails, next
    try the parameters passed to nl_affirm.  Finally, if this fails, go
    for a hard coded C locale definition of the expressions.  In each case,
    if the YES expression can be obtained but the NO expression cannot, 
    treat this as a fatal error for nl_affirm.
*/
  if (gotExpr == FALSE)
    {

/*  1:  Try to get the Yes expression from the locale.
*/
      yesExprPtr = (unsigned char *) nl_langinfo (YESEXPR);

/*  Because of a fluke in nl_langinfo, it returns a POINTER to a NULL
    string, not NULL if no value can be obtained for a locale element.
*/
      if (*yesExprPtr == NULL)
	{

/*  2:  Couldn't get the Yes expression from the locale, so try the parameters
        passed to this routine.
*/
	  if ((paramYes != NULL) && (paramNo != NULL))
	    {
	      yesExprPtr = paramYes;
	      noExprPtr = paramNo;
	      gotExpr = TRUE;
	      
	    }

/*  3:  Couldn't get the Yes expression from the the parameters passed to this 
        routine so use the defaults.
*/
	  else if ((paramYes == NULL) && (paramNo == NULL))
	    {
	      yesExprPtr = defaultYes;
	      noExprPtr = defaultNo;
	      gotExpr = TRUE;
	    }

/*  Could get one of the expressions from the parameters, but not the other.
    Treat this as a fatal error for this routine.  It's MUCH better to get
    them both from the same location than one from one place and one from
    another.
*/
	  else
	    return NO_EXPR;
	}
      
/*  We get through this test if the Yes expression was obtained from the 
    locale.  Otherwise, one of the other paths filled in noExprPtr so that
    it isn't NULL anymore.  If the No expression isn't obtainable from the
    locale, treat this as a fatal error for this routine.  Again, the rationale
    is that both expressions should be obtained from the same location.
*/
      if (noExprPtr == NULL)
	{
          noExprPtr = (unsigned char *) nl_langinfo (NOEXPR);

          if (*noExprPtr == NULL)
            return NO_EXPR;
	}
    }

/*  Compile the regular expressions if they haven't already been compiled.
    If an error value is returned from regcomp (3C), treat this as an error
    to nl_affirm and return the return value.
*/
  if (compiledExpr == FALSE)
    {
      regcompReturn = regcomp (&yesRcomp, yesExprPtr, 
			   REG_EXTENDED | REG_NEWLINE | REG_ICASE | REG_NOSUB);
  
      if (regcompReturn != 0)
        return regcompReturn;
  
      regcompReturn = regcomp (&noRcomp, noExprPtr, 
			    REG_EXTENDED | REG_NEWLINE | REG_ICASE | REG_NOSUB);
  
      if (regcompReturn != 0)
        return regcompReturn;
      
      compiledExpr = TRUE;
    }

/*  Compare the strings.  If an error value which is not REG_NOMATCH is 
    returned from regexec (3C), treat this as an error to nl_affirm and 
    return the return value.

    If a match occurs for the YES expression, set matchY.  If a match occurs
    for the NO expression, set matchN.  This is the most general case for
    the routine; that is, to always check against both the YES expression and
    the NO expression.  The calling program is free to decide whether or not
    to use all of the information returned.  For example, it may decide that
    if AFFIRMATIVE is not returned then it will assume that the user response
    is NEGATIVE, even though nl_affirm returned NEITHER, or AMBIGUOS.  Or the
    calling program could do the opposite.  Whatever the case, this routine
    supplies enough information that the program can take whichever path it
    wants.
*/
  regexecReturn = regexec (&yesRcomp, userInput, (size_t) 0, NULL, 0);

  if ((regexecReturn != 0) && (regexecReturn != REG_NOMATCH))
    return regexecReturn;
  
  if (regexecReturn == 0)
    matchY = TRUE;

  regexecReturn = regexec (&noRcomp, userInput, (size_t) 0, NULL, 0);

  if ((regexecReturn != 0) && (regexecReturn != REG_NOMATCH))
    return regexecReturn;
      
  if (regexecReturn == 0)
    matchN = TRUE;

/*  Return an answer regarding whether the userInput was affirmative, negative,
    both, or neither.
*/
  if ((matchY == TRUE) && (matchN == TRUE))
    return AMBIGUOUS;
  
  if ((matchY == FALSE) && (matchN == FALSE))
    return NEITHER;
  
  if (matchY == TRUE)
    return AFFIRMATIVE;
  
  if (matchN == TRUE)
    return NEGATIVE;
}
