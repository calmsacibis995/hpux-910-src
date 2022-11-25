# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/mkmf/mkmf/makefile,v $
# $Revision: 66.1 $

# Common makefile for building mkmf command

PFLAGS	      = -I..

HDRS	      = Mkmf.h 		\
		RELEASE.h 	\
		defaultrul.h 	\
		defaultsfx.h 	\
		dir.h 		\
		dlist.h 	\
		getarg.h 	\
		hash.h 		\
		macro.h 	\
		null.h 		\
		path.h 		\
		rule.h 		\
		slist.h 	\
		suffix.h 	\
		system.h 	\
		target.h 	\
		yesno.h

OBJS	      = Mkmf.o		\
		badopt.o	\
		buildlist.o	\
		depend.o	\
		dlist.o		\
		editmf.o	\
		getcwp.o	\
		getproject.o	\
		hash.o		\
		iolin.o		\
		macro.o		\
		misc.o		\
		mustfopen.o	\
		optpath.o	\
		pathcat.o	\
		pathhead.o	\
		pperror.o	\
		rule.o		\
		slappend.o	\
		slinit.o	\
		slsort.o	\
		slvect.o	\
		slvtol.o	\
		strpcpy.o	\
		strsav.o	\
		suffix.o	\
		usage.o		\
		warn.o		\
		warn2.o		\
		warns.o

PROGRAM	      = mkmf

SRCS	      = $(OBJS:.o=.c)

default:	$(PROGRAM)

$(PROGRAM):     $(OBJS) 
		$(CC) $(LDFLAGS) $(OBJS) -o $(PROGRAM)

clean:		
		/bin/rm -f $(OBJS) core

clobber:
		/bin/rm -f $(OBJS) $(PROGRAM) core

index:		
		ctags -wx $(HDRS) $(SRCS)

tags:           
		$(HDRS) $(SRCS); ctags $(HDRS) $(SRCS)

$(OBJS): 	$(HDRS)

# Redefined the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
