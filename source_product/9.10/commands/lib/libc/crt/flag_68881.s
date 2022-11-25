# @(#) $Revision: 30.2 $     
# This file was specifically added so kernel configurations can be done.
# The flag_68881 flag is normally found in crt0.o (or frt0.o), but because 
# the kernel links with neither of these, it will find the flag here.

	data
	global	flag_68881
flag_68881:
	short	0
