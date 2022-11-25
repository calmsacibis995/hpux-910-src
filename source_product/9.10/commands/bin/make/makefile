# $Source: /misc/source_product/9.10/commands.rcs/bin/make/makefile,v $
# $Revision: 64.3.1.4 $

# Common makefile for the make utility.
# The rules.c file can be modified locally for people who still like
#	things like fortran.

# Define a temporary 7.0 flag for compiling in the VFORK changes
# in dosys and misc.  Take this out for 8.0, as the VFORK change
# will be rendered ineffective with the new kernel (copy on write)

FLAGS = -DVFORK

# Define usual objects

OBJECTS = main.o doname.o misc.o files.o rules.o \
	  dosys.o gram.o dyndep.o prtmem.o nl_strchr.o
RELEASE = hpux_rel.o

default:  make

make:  $(OBJECTS) $(RELEASE)
	$(CC) -o make $(LDFLAGS) $(OBJECTS) $(RELEASE)

$(OBJECTS):  defs

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f make

# temporarily for 7.0, redefine the .c.o rule to include the -DVFORK
# in the compilation of the .c files

.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $*.c

# copy /bin/make to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/make
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	make
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp make ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/make
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/make to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/make
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp make ../../../Filesets.sl/UX-CORE/bin/make
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/make
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
