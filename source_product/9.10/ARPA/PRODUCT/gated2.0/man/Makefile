MANROOT	= /usr/local/man
MAN5DIR	= man5
MAN5EXT = 5
MAN8DIR = man8
MAN8EXT = 8
MANMODE	= 644
TROFF   = ptroff
FILES	= gated.man gated-config.man ripquery.man

all:	help

help:
	@echo "'make install' to install in ${MANROOT}/{${MAN5DIR},${MAN8DIR}}/{${FILES}}.{${MAN5EXT},${MAN8EXT}}"
	@echo "'make print' to format {${FILES}} with ${TROFF}"

install:	${FILES}
	install -c -m ${MANMODE} gated.man ${MANROOT}/${MAN8DIR}/gated.${MAN8EXT}
	install -c -m ${MANMODE} ripquery.man ${MANROOT}/${MAN8DIR}/ripquery.${MAN8EXT}
	install -c -m ${MANMODE} gated-config.man ${MANROOT}/${MAN5DIR}/gated-config.${MAN5EXT}

print:	${FILES}
	@for f in ${FILES}; do ${TROFF} -man $$f; done	

clean: