# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libV3/makefile,v $
# $Revision: 66.1.1.2 $

# Common makefile for libV3

M4=m4

AS300FLG	=
AS300FLG_P	=
PFLAGS	=  -p
DFLAGS  =  -DFLOAT
M4FLAGS	=
M4FLG_P	=  -DPROFILE
CPPFLAGS=  -P
SEDFLAGS=  -e '/^[       ]*$$/d' -e 's/\\,/,/' -e 's/^\./  \./' -e 's/; _/;/' -e 'y/; /\n /' -e 's/ /     /'
PREFIX  =  /lib/pcc_prefix.s
OBJECTS =  hpux_rel.o setjmp.o sighold.o sigignore.o sigpause.o \
 	   sigrelse.o sigset.o
OBJECTS_P = $(OBJECTS:.o=_p.o)
LIB	=  libV3.a
LIB_P	=  libV3_p.a


default:	$(LIB)

$(LIB):     $(OBJECTS) $(OBJECTS_P)
	/bin/rm -f $(LIB) $(LIB_P)
	ar q $(LIB) `lorder $(OBJECTS) | tsort`
	ar q $(LIB_P) `lorder $(OBJECTS_P) | tsort`

.c_p.o:
	$(CC) $(CFLAGS) $(PFLAGS) $(DFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -c $<

.s.o:
	if hp9000s300; then					\
		$(M4) $(M4FLAGS) $< | as $(AS300FLG) -o $@;	\
	elif hp9000s800; then					\
		$(CC) -E $(CPPFLAGS) -DAS -DLOCORE $< | sed $(SEDFLAGS)	\
			| cat $(PREFIX) - | as -o $@;\
	fi;

.s_p.o:
	if hp9000s300; then					\
		$(M4) $(M4FLG_P) $< | as $(AS300FLG_P) -o $@;	\
	elif hp9000s800; then					\
		$(CC) -E $(CPPFLAGS) -DAS -DLOCORE -DPROF $< | sed $(SEDFLAGS)\
			| cat $(PREFIX) - | as -o $@;\
	fi;

clean:
	/bin/rm -f $(OBJECTS) $(OBJECTS_P)

clobber:    clean
	/bin/rm -f $(LIB) $(LIB_P)

.SUFFIXES: _p.o
# copy /usr/lib/libV3 to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libV3
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp libV3.a ../../../../Filesets.sl/BUILDENV/usr/lib/libV3.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/libV3.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libV3 to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libV3
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libV3.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libV3.a
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libV3.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
