# $Source: /misc/source_product/9.10/commands.rcs/lib/libcurses/terminfo/makefile,v $
# @(#) $Revision: 66.2 $

# Common makefile for libcurses/terminfo subdirectory

# 
# Current entries for terminfo library
#
NONHPTERM_SRC =		\
	adds.ti		\
	annarbor.ti	\
	beehive.ti	\
	cdc.ti		\
	concept.ti	\
	datamedia.ti	\
	dec.ti		\
	diablo.ti	\
	general.ti	\
	hardcopy.ti	\
	hazeltine.ti	\
	heath.ti	\
	homebrew.ti	\
	ibm.ti		\
	lsi.ti		\
	microterm.ti	\
	misc.ti		\
	pc.ti		\
	perkinelm.ti	\
	print.ti	\
	tektronix.ti	\
	teleray.ti	\
	teletype.ti	\
	televideo.ti	\
	ti.ti		\
	visual.ti	\
	wyse.ti

HPTERM_SRC =		\
	hp.ti		\
	special.ti	\
	virtual.ti

# Terminfo entry compiler.
TIC = tic -v

# Where to put the compiled terminfo's
LIB = ../lib

#
# Link information is placed in the HPLINKS and NONHPLINKS files
#
HPLINKS    = HPTERM.links
NONHPLINKS = NONHPTERM.links

default: HPTERM.targets NONHPTERM.targets

HPTERM.targets:	HPTERM.src $(LIB)
	TERMINFO=$(LIB) $(TIC) HPTERM.src | awk -F"'" ' \
	  /^link/ {printf "%s %s\n", $$2, $$4 > "$(HPLINKS)"; next} \
	  /^create/ {print $$2 > "$@"; next} \
	  /^mkdir/ {next} \
	  {print $$0}' -

NONHPTERM.targets: NONHPTERM.src $(LIB)
	TERMINFO=$(LIB) $(TIC) NONHPTERM.src | awk -F"'" ' \
	  /^link/ {printf "%s %s\n", $$2, $$4 > "$(NONHPLINKS)"; next} \
	  /^create/ {print $$2 > "$@"; next} \
	  /^mkdir/ {next} \
	  {print $$0}' -

HPTERM.src: header $(HPTERM_SRC) HPTERM.obs
	cat header $(HPTERM_SRC) HPTERM.obs > HPTERM.src

NONHPTERM.src: header $(NONHPTERM_SRC) NONHPTERM.obs
	cat header $(NONHPTERM_SRC) NONHPTERM.obs > NONHPTERM.src

$(LIB):
	mkdir $(LIB) || exit 0
	if [ ! -d $(LIB) ]; then		\
	    echo "Failed to make $(LIB)";	\
	    exit 1;				\
	fi

clean:
	/bin/rm -f \
		HPTERM.src \
		HPTERM.links \
		HPTERM.targets \
		NONHPTERM.src \
		NONHPTERM.links \
		NONHPTERM.targets
	/bin/rm -rf $(HPTERM) $(NONHPTERM)

clobber: clean
	rm -rf $(LIB)
