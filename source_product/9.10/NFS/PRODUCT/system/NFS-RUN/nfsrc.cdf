#!/bin/sh

if [ $1 != "standalone" ] ; then

    if [ ! -f /etc/netnfsrc -o ! -H /etc/netnfsrc ] ; then
	/usr/bin/makecdf -c localroot /etc/netnfsrc
	/usr/bin/makecdf -f /etc/newconfig/netnfsrc -c remoteroot /etc/netnfsrc
    fi

    if [ ! -f /etc/netnfsrc.OLD -o ! -H /etc/netnfsrc.OLD ] ; then
	/usr/bin/makecdf -c localroot /etc/netnfsrc.OLD
    fi

    if [ -f /etc/exports -a ! -H /etc/exports ] ; then
	/usr/bin/makecdf -c localroot /etc/exports
    fi

    if [ ! -f /etc/xtab -o ! -H /etc/xtab ] ; then
	/usr/bin/makecdf -c localroot /etc/xtab
    fi
fi
