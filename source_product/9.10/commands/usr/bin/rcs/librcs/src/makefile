# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/rcs/librcs/src/makefile,v $
# $Revision: 66.2 $

# Common makefile for creating the library for rcs

INC     = ../../include
PFLAGS  = -I$(INC)

LIBDIR  = ../../lib
LIBRARY = librcs.a
OBJS    = diagnose.o 	\
 	  diagnose1s.o 	\
	  diagnose2s.o 	\
	  diagnose4s.o 	\
	  error.o 	\
	  error1c.o 	\
	  error1s.o 	\
	  error2s.o 	\
 	  error4s.o 	\
	  execonly.o 	\
	  execstdout.o 	\
	  faterr1c1s.o 	\
	  faterror.o 	\
	  faterror1c.o 	\
	  faterror1s.o 	\
	  faterror2s.o 	\
	  fatserr1s.o 	\
	  fatserror.o 	\
	  fflsbuf.o 	\
	  fputs.o 	\
	  getdirlink.o	\
	  getwd.o 	\
	  local.o 	\
	  maketime.o 	\
	  mvfile.o 	\
	  partime.o 	\
	  pathncat.o 	\
  	  rcsedit.o 	\
	  rcsfcmp.o 	\
	  rcsfnms.o 	\
	  rcsgen.o 	\
	  rcskeep.o 	\
	  rcslex.o 	\
	  rcsrev.o 	\
	  rcssigs.o	\
	  rcssyn.o 	\
	  rcsutil.o 	\
	  serror.o 	\
	  serror1c.o 	\
	  serror1s.o 	\
	  setuser.o 	\
	  strsav.o	\
	  warn.o 	\
	  warn1c.o 	\
	  warn1i.o 	\
	  warn1s.o 	\
	  warn2s.o 	\
	  warn3s.o

default:  all

all:	  $(LIBRARY)

$(LIBRARY): $(OBJS)
	/bin/rm -rf $(LIBRARY) $(LIBDIR);	\
	ar q $(LIBRARY) $(OBJS);		\
	mkdir $(LIBDIR);			\
	ln $(LIBRARY) $(LIBDIR)

clean:
		/bin/rm -f $(OBJS) 

clobber:	clean 
		/bin/rm -rf $(LIBRARY) $(LIBDIR)


execonly.o:     $(INC)/system.h $(INC)/rcsbase.h
execstdout.o:   $(INC)/system.h $(INC)/rcsbase.h
getwd.o:	$(INC)/system.h $(INC)/dir.h $(INC)/rcsbase.h
maketime.o: 	$(INC)/system.h $(INC)/time.h
partime.o: 	$(INC)/time.h
rcsedit.o: 	$(INC)/system.h $(INC)/rcsbase.h
rcsfcmp.o: 	$(INC)/rcsbase.h $(INC)/system.h $(INC)/dir.h
rcsgen.o:	$(INC)/rcsbase.h
rcskeep.o:	$(INC)/system.h $(INC)/rcsbase.h
rcslex.o: 	$(INC)/rcsbase.h
rcsrev.o: 	$(INC)/rcsbase.h
rcssyn.o: 	$(INC)/rcsbase.h
rcsutil.o: 	$(INC)/system.h $(INC)/rcsbase.h $(INC)/bin.h $(INC)/time.h
setuser.o: 	$(INC)/rcsbase.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
