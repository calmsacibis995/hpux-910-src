.SOURCE : $(SRCDIR:/^$/./)
.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS	      = decode.c \
		display_msg.c \
		dpp.c \
		process.c \
		store_login.c

HDRS          = $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include \
                decodex.h \
		display_msg.h \
		display_msgx.h \
		processx.h \
		store_loginx.h

OBJS	      = decode.o \
		display_msg.o \
		dpp.o \
		process.o \
		store_login.o

CONFFILES	= dp

CCFLAGS=        -O $(NEWCCFLAGS)
LDFLAGS=        -s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif


all:		build

dpp::           $(SRCS)

build:          $(TARGET:/^$/dpp/)
                echo "Building... $(TARGET)"

clean:		
		$(RM) $(RMFLAGS) $(OBJS) core

clobber:	clean
                $(RM) $(RMFLAGS) $(TARGET)
                $(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

install:
		echo Installing $(TARGET)
		$(INSTALL) $(TARGET)
		echo Installing $(CONFFILES)
		$(INSTALL) $(CONFFILES)

decode.o: 	display_msg.h display_msgx.h \
		$(SRCDIR:/^$/./)/../include/dpp_const.h

display_msg.o:  display_msg.h

dpp.o: 		display_msgx.h processx.h decodex.h store_loginx.h \
		display_msg.h \
		$(SRCDIR:/^$/./)/../include/dpp_const.h 

process.o: 	display_msg.h display_msgx.h \
		$(SRCDIR:/^$/./)/../include/dpp_const.h

store_login.o:  display_msg.h display_msgx.h \
		$(SRCDIR:/^$/./)/../include/dpp_const.h \
		$(SRCDIR:/^$/./)/../include/store_login.h
