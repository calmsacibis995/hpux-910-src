# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libcrypt/makefile,v $
# @(#) $Revision: 64.1 $    

LIB=     libcrypt.a

OBJECTS=  crypt.o

default:    $(LIB)

$(LIB):     $(OBJECTS)
	ar  rv  $(LIB)  $(OBJECTS)

clean:
	/bin/rm -f $(OBJECTS)

clobber:    clean
	/bin/rm -f $(LIB)
