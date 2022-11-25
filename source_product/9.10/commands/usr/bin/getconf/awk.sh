#!/bin/sh
# @(#) $Revision: 66.4 $

exit 0

# Generate a local header file from the existing /usr/include/sys.

header_name="getconf.h"			# name of the file to create

#awk </usr/include/sys/unistd.h '
sed -e 's/^# *define/#define/' unistd.h | # allow for '# define'
awk '
BEGIN {
    print "/*";
    print " * This automatically generated table of parameters is used";
    print " * for calls to confstr, pathconf and sysconf functions.";
    print " */";

    print "#define	FT_CONFSTR	1	/* Function Types */";
    print "#define	FT_PATHCONF	2";
    print "#define	FT_SYSCONF	3";

    print "struct parmareter_table		/* parameter table */ ";
    print "{";
    print "	char	*pt_name;		/* parameter name */";
    print "	int 	pt_code;		/* parameter code */";
    print "	int 	pt_function_type;	/* value of signal */";
    print "} parm_table[] = {";
}
/define/ {
    parm_code=0;
    if ($1 == "#define")			#/* allow for "#define" */
    {
        parm_code=2;
	if (substr($2,1,4) == "_CS_")
		func_type="FT_CONFSTR";
	else if (substr($2,1,4) == "_PC_")
		func_type="FT_PATHCONF";
	else if (substr($2,1,4) == "_SC_")
		func_type="FT_SYSCONF";
	else
		parm_code=0;
    }

    if (parm_code != 0 && \
	substr($parm_code,1,4) != "SIG_")
    {
	#name_str = sprintf "\"%s\",", substr($parm_code,4);
	#name_str = substr($parm_code,2);
	name_str = sprintf "\"%s\",", substr($parm_code,5);
	code_str = sprintf "%s,", substr($parm_code,1);
	printf "\t{%-23s %-23s %s},\n", name_str, code_str, func_type;
#	if ((n = index($0, "/*")) != 0)
#	    printf " %s\n", substr($0, n);
#	else
#	    print "";
    }
}
END {
    printf "\t{ %s, %s, %s}			/* Terminates table */\n", \
	"(char *)0", 0, 0;
    print "};"
}' > $header_name
