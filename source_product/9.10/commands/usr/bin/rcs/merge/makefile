# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/rcs/merge/makefile,v $
# $Revision: 66.2 $

# Common makefile for merge utility of rcs
# The old make files used to also chmod on merge to +x.
# This will be done by the build process.

default: merge

merge:  merge.sh
	/bin/rm -f merge
	grep -v '^# ' merge.sh > merge

clean:	
	/bin/rm -f *.o core

clobber:	
	/bin/rm -f merge
