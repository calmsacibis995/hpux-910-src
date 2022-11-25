#!/bin/sh
# @(#) $Revision: 70.1 $      
if test -z "$3" -o "$3" = "$1" -o "$3" = "$2"; then
	echo "usage: name1 name2 name3 -- name3 must be different"
	exit
fi
diff -e $1 $2 | (echo H; sed -n -e '
/[ac]$/{
	p
	a\
.mc |
: loop
	n
	/^\.$/b done1
	p
	b loop
: done1
	a\
.mc\
.
	b
}

/d$/{
	s/d/c/p
	a\
.mc *\
.mc\
.
	b
}'; echo '1,$p\nQ') | ed -s $1| sed -n '
	/^\.mc |$/{
		b loop1
	}
	/^\.TS/{
		p
		b loop2
	}
	/^\.T&/{
		p
		b loop2
	}
	p
	d

:loop1
	{
		p
		n
		/^\.mc$/ {
			p
			d
		}
		/^\.TS/{
			i\
.mc
			p
			b loop2
		}
		/^\.T&/{
			i\
.mc
			p
			b loop2
		}
		b loop1
	}
:loop2
	{
		n
		/^\.mc |$/ b loop3
		/^\.mc$/ b loop2
		/.*\. *$/ {
			p
			d
		}
		p
		b loop2
	}
:loop3
	{
		n
		/^\.mc$/ b loop2
		/.*\. *$/ {
			p
			a\
.mc |
			d
		}
		p
		b loop3
	}
' > $3
