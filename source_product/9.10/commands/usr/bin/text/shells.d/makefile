# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/text/shells.d/makefile,v $
# $Revision: 64.2 $

# Common makefile for text subsystem shells

FILES	= mm

# Had to use this control structure so that when on an 800 machine, the extra
# file needed there would get included.
default:
	if hp9000s800; then             \
		files="osdd";           \
	elif hp9000s300; then           \
		files="osdd";           \
	fi;                             \
	$(MAKE) FILES="$(FILES) $$files" all

all:  $(FILES)

mm:  mm.sh
	cp mm.sh mm

osdd : osdd.sh
	cp osdd.sh osdd

clean:
	:

clobber:  clean
	/bin/rm -f mm osdd
