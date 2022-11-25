# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/text/mmchek/makefile,v $
# $Revision: 64.2 $

# Makefile for mmchek, used by 300 only

OFILES  = chekl.o chekmain.o
LIBS	= -ll -lPW

default:    mmchek

mmchek: $(OFILES)
	$(CC) $(LDFLAGS) -o mmchek $(OFILES) $(LIBS)

clean:
	/bin/rm -f $(OFILES)

clobber:  clean
	/bin/rm -f mmchek
