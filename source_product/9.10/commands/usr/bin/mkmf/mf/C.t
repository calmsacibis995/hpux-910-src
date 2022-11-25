CFLAGS	      =

LD	      = cc

LDFLAGS	      =

LIBS	      =

SHELL	      = /bin/sh

OBJS	      =

PROGRAM	      = a.out

all:		$(PROGRAM)

$(PROGRAM):     $(OBJS) $(LIBS)
		@$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(PROGRAM)
