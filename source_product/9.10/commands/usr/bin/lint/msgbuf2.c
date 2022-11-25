/* @(#) $Revision: 70.4 $ */      
# include	"lerror.h"
struct msg2info msg2info[ NUM2MSGS ] = {
/* [0] */	{ "name used but not defined",	/* name */
	/* name in filename( lineno ) */
		NMFNLN, WUSAGE },

/* [1] */	{ "name defined but never used",/* name */
	/* name in filename( lineno ) */
		NMFNLN, WUSAGE },

/* [2] */	{ "name declared but never used or defined",	/* name */
	/* name in filename( lineno ) */
		NMFNLN, WUDECLARE },

/* [3] */	{ "name multiply declared",	/* name */
		NM2FNLN, WDECLARE },

/* [4] */	{ "value type used inconsistently",	/* name */
		NM2FNLN, WUSAGE },

/* [5] */	{ "value type declared inconsistently",	/* name */
		NM2FNLN, WDECLARE },

/* [6] */	{ "function argument ( number ) used inconsistently",	/* name and number */
	/* function( argnumber )  filename( lineno ) :: filename( lineno ) */
		ND2FNLN, WDECLARE },

/* [7] */	{ "function used with a variable number of arguments",	/* name */
		NM2FNLN, WDECLARE },

/* [8] */	{ "function value type must be declared before use",	/* name */
		NM2FNLN, WDECLARE },

/* [9] */	{ "function value is used, but none returned",	/* name */
		NMONLY, WUSAGE },
/* [10] */	{ "function returns value which is always ignored",	/* name */
		NMONLY, WUSAGE },
/* [11] */	{ "function returns value which is sometimes ignored",	/* name */
		NMONLY, WUSAGE },

/* [12] */	{ "mixed old-style/new-style function declaration",
		NM2FNLN, WPROTO },
#ifdef APEX
/* [13] */      {"standards/portability information ",
		NMFNSTD, WHINTS },
#endif
};
