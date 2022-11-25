
shared: archive profile archive_pic
	cd gen; \
	make shared

archive:
	( mkdir ../tmp >/dev/null 2>&1 || exit 0 ); \
	touch ../tmp/OK.rel80;

profile:
	( mkdir ../tmp_p >/dev/null 2>&1 || exit 0 ); \
	touch ../tmp_p/OK.rel80;

archive_pic:
	( mkdir ../tmp_pic >/dev/null 2>&1 || exit 0 ); \
	touch ../tmp_pic/OK.rel80;

clean:
	cd gen; make clean

clobber:
	@echo "Clobbering is done by the top-level libc makefile"
