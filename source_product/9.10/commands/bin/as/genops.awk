# @(#) $Revision: 70.1 $       

# NOTE:  "pairs.dat" must be run through "cpp" FIRST!!!

# Read a data file containing icode and base bit pattern pairs: 

# I_ABCD	0xc100	comments can go here
# I_ADD		0xd000
# ....

# and produce a C file which initializes an array of skeletal op codes,
# indexed by I_xxx value:

# #include "ivalue.h"
# short iopcode[I_LASTOP] = {
# 0xc100, 0xd000, ... }

BEGIN	{inum = 0;
	 printf("#include \"ivalues.h\"\nshort iopcode[I_LASTOP] = {")
	}

# Print all lines with at least 2 fields.  Comments and "ifdefs" have
# already been taken care of by cpp. Don't increment inum for blank lines.

	{ if (NF >= 2) { 
	     if ($1 == "I_LASTOP") exit;
	     if (inum > 0) printf(", ");	# "," before all but first
	     if (inum % 8 == 0)  printf("\n");	# 8 per line
	     inum++;
	     printf("%s",$2);			# the op-code skeleton itself
	     }
	}

END	{ printf("};\n") }


