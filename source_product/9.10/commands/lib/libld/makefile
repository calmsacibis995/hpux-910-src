# $Source: /misc/source_product/9.10/commands.rcs/lib/libld/makefile,v $
# $Revision: 64.4 $

# Common makefile for libld

AR=/bin/ar

LIB=libld.a

FLAGS_800=-DPORTAR

COMOBJS = sputl.o sgetl.o hpux_rel.o

OBJS800 = 		\
	allocldptr.o	\
	freeldptr.o	\
	ldaclose.o	\
	ldahread.o	\
	ldaopen.o	\
	ldclose.o	\
	lddef.h		\
	ldfhread.o	\
	ldgetname.o	\
	ldlread.o	\
	ldlseek.o	\
	ldnlseek.o	\
	ldnrseek.o	\
	ldnshread.o	\
	ldnsseek.o	\
	ldohseek.o	\
	ldopen.o	\
	ldrseek.o	\
	ldshread.o	\
	ldsseek.o	\
	ldtbindex.o	\
	ldtbread.o	\
	ldtbseek.o	\
	vldldptr.o

default:
	if hp9000s300; then				\
		objs="$(COMOBJS)";			\
	elif hp9000s800; then				\
		objs="$(COMOBJS) $(OBJS800)";		\
		flags="$(FLAGS_800)";			\
	fi;						\
	$(MAKE) OBJS="$$objs" FLAGS="$$flags" $(LIB)

$(LIB): $(OBJS)
	/bin/rm -f libld.a
	$(AR) q libld.a `lorder $(OBJS) | tsort`

clean:
	/bin/rm -f $(COMOBJS) $(OBJS800)

clobber:	clean
	/bin/rm -f libld.a

#
# Re-define .c.o rule so compilation will use FLAGS as well as CFLAGS
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<
# copy /lib/libld to the . relative fileset PROG-AUX
# change mode, owner, and group on /lib/libld
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/PROG-AUX/lib
	! cp libld.a ../../../Filesets.sl/PROG-AUX/lib/libld.a
	! chmog 0444 bin bin ../../../Filesets.sl/PROG-AUX/lib/libld.a
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/lib
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
