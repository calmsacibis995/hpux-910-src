#!/bin/sh
# @(#) B.08.00 LANGUAGE TOOLS Internal $Revision: 66.3 $
#
# DESCRIPTION
#
#    A script to comment out extraneous non-whitespace characters after #else
#    and #endif in cpp directive lines. If these characters are already
#    properly commented out, then the script tries to detect this condition
#    and leave the line alone. (the criterion for "properly commented out" is
#    slightly crude: if the first token after the else/endif contains a "/*"
#    and the last token contains a "*/", then the line is assumed to be
#    properly commented.
#
# USAGE
#
#    endif <ORIG_FILE > FIXED_FILE
#
# KNOWN DEFICIENCIES
#
#    If the else or endif is followed without spaces by some text, the output
#    will be incorrect (the comment leader will be inserted after the first
#    white space after the endif or else)
#
#    A line of the form "#endif stuff /* comment */ stuff" (with stuff outside
#    the comment delimiters) will confuse the script and cause it to put out
#    syntactically a incorrect line. 
#
#    Neither of these is too harmful, as the first will elicit a warning
#    and the second an error from cpp.
#

awk '/^[ \t]*#[ \t]*endif/ {  # ENDIF\
		if ( $1 == "#endif" ) {\
		   START = 2\
		} else {\
		   START = 3\
		}\
		if ( NF >= START ) {\
		   if ( $START ~ /\/\*/ && $NF ~ /\*\// ) \
			print $0;\
		   else { TOKEN=index($0, $START);\
			print substr($0,1,TOKEN-1) "/* " substr($0,TOKEN) " */"\
                   }\
		} else {\
		    print $0;\
		}\
	     break;\
	     }\
\
/^[ \t]*#[ \t]*else/   {  # ELSE\
		if ( $1 == "#else" ) {\
		   START = 2\
		} else {\
		   START = 3\
		}\
		if ( NF >= START ) {\
		   if ( $START ~ /\/\*/ && $NF ~ /\*\// ) \
			print $0;\
		   else { # non-commented text after else\
			TOKEN=index($0, $START);\
			print substr($0,1,TOKEN-1) "/* " substr($0,TOKEN) " */"\
		   }\
		} else {\
		    print $0;\
		}\
	     break;\
	     }\
# all other lines\
{print $0}'
