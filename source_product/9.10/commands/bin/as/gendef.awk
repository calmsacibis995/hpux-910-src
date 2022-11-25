# @(#) $Revision: 70.1 $       

# NOTE:  "pairs.dat" must be run through "cpp" FIRST!!!

# Read a data file containing icode and base bit pattern pairs: 

# I_ABCD	0xc100	comments can go here
# I_ADD		0xd000
# ....

# and produce a header file containing a sequential list of I_code defines:

# # define I_ABCD	0
# # define I_ADD	1
# ....

BEGIN	{inum = 0}

# Print all lines with at least 2 fields.  Comments and "ifdefs" have
# already been taken care of by cpp.

	{ if (NF >= 2)
		printf("# define %s  \t %d\n", $1, inum++) 
	}

