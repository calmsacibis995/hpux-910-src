#  $Source: /misc/source_product/9.10/commands.rcs/usr/bin/lp/fonts/makefile,v $
#  $Revision: 66.1 $

#  makefile for lp/fonts files:
#  lp2l, lp4p, lp4l

CFLAGS = -O
LDFLAGS = -s -Wl,-a,archive

default: all

all: lp2l lp4p lp4l

lp2l : lp2l.src
	cc $(CFLAGS) $(LDFLAGS) -D'FONTSOURCE="lp2l.src"' -o makefont makefont.c
	./makefont
	rm -f makefont

lp4p : lp4p.src
	cc $(CFLAGS) $(LDFLAGS) -D'FONTSOURCE="lp4p.src"' -o makefont makefont.c
	./makefont
	rm -f makefont

lp4l : lp4l.src
	cc $(CFLAGS) $(LDFLAGS) -D'FONTSOURCE="lp4l.src"' -o makefont makefont.c
	./makefont
	rm -f makefont

clean:

clobber:  clean
	/bin/rm -f lp2l lp4p lp4l
