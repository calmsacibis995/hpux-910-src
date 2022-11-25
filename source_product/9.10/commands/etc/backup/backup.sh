#!/bin/sh
# @(#) $Revision: 66.2 $
#
# System delta backup or full archive script.
# This script will also run fsck if requested (with argument "-fsck").
#
# NOTE:  This file (/etc/backup) also exists as /etc/newconfig/backup.
#        When you update your system, you must merge any changes made
#        to the new version with any customization that you have done
#        to this file.
#
# Should only be executed by the superuser (as from cron)!
#
# IF BACKUP IS DONE TO A TRULY RAW DEVICE (i.e., 1/2" mag tape or
# floppy disk instead of tape cartridge), replace these lines in the
# script:
#
#	cpio -ocxa	|
#	tcio -o $dest
#
# with one of these two lines:
#
#	cpio -ocBxa >/dev/rmt/xx    # Mag tape
#	cpio -ocBxa >/dev/rdsk/xx   # Floppy disk
#
# That is, remove the tcio, add a "B" (buffer) option, and send standard
# output to the correct /dev/rmt/xx or /dev/rdsk/xx.  (It is unnecessary
# to use this option with tcio.)
#
# or replace with one of these lines for a faster backup:
#
#	ftio -ocxM /dev/rmt/xx      # cpio-compatible archive
# or
#	ftio -ocx /dev/rmt/xx       # ftio-compatible only archive
#
# (Please refer to the ftio(1) manual page for additional information.)
#
# If the target device is a 9144, better throughput may be obtained
# by specifying a buffer size of 8 KBytes to tcio:
#
# 	find . -print | cpio -ocx | tcio -oS 8 /dev/rct/xx
#

# Set local values:
#
# Note:  backupdirs should be ".", not "/", to backup the whole
#        filesystem.  Since the find takes place at "/", files are
#        then archived with relative paths, not absolute paths.

PATH=/bin:/usr/bin:/etc         # where to look for commands.
TIMEOUT=60			# time to wait for response
backupdirs="."			# directories to backup.
backuplog="/etc/backuplog"	# where to log errors, etc.
archive="/etc/archivedate"	# date of last full archive.
remind="/tmp/changetape"	# reminder file for /etc/profile.
destfile="/etc/backupdest"	# file containing backup destination
fscklog="/dev/lp"		# where to log fsck output.  Fsck
				# output must not be sent to a file on
				# the device being checked.

#
# Check arguments:
#

aflag="-newer $archive"		# default = incremental backup:
aecho=""
Aflag=false			# default = print messages
fflag=false			# default = no fsck.

for arg in $*
do
    case $arg in
    -archive)
	aflag=""	 # all files.
	aecho="archive"
	;;
    -fsck)
	fflag=true
	;;
    -A)
	Aflag=true
	;;
    *)
	echo "usage: $0 [-A] [-archive] [-fsck]"
	exit 1
	;;
    esac
done

if [ X"$aflag" != X -a ! -f $archive ]; then
    echo "Error: you must perform a full archive before an incremental backup"
    exit 1
fi

#
# Determine backup destination
#
if [ -f $destfile ]
then
    dest=`cat $destfile`
else
    dest="/dev/update.src"
fi

#
# You may remove the next 14 lines to use the default device
#
echo "backing up to $dest"
echo "Enter new device name to change the backup destination".
echo
echo "This will timeout in $TIMEOUT seconds."
echo

response=`line -t $TIMEOUT`
echo
if [ -n "$response" ]
then
    echo "$response" > $destfile
    dest="$response"
fi

#
# Check for backup to file:
#
if [ ! -c $dest ]
then
    if [ -b $dest ]
    then
	echo "Warning: you are not backing up to a character special device"
    else
	echo "Warning: you may be backing up to a file"
    fi
fi

#
# Warn all users that a backup is about to begin and then start the
# backup.
#
echo "Starting system backup" | wall

if [ X"$aflag" = X ]	# doing a full archive
then
    # remember the date/time backup started
    touch $archive.x
fi

if $Aflag; # ACLS are quiet
then
    A=A
else
    A=
fi

cd /

echo "s `date` `uname -r` $aecho" >>$backuplog
{
    find $backupdirs $aflag -fsonly hfs -hidden -print	|
    cpio -ocxa$A					|
    tcio -o $dest
} 2>>$backuplog			# log stderr only.

echo "f `date`" >>$backuplog

if [ -f $archive.x ]	# did an archive
then
    mv $archive.x $archive	# save the new archive time
fi

# Finish up after backup:

echo "\007Backup complete at `date`"
touch $remind			# make reminder file.

#
# Optionally start fsck:
#
if $fflag;
then
    echo "File system check starts in 60 seconds.  LOG OFF!" | wall
    sleep 60
    echo Now starting fsck.

    {
	echo "\n\nStarting fsck at `date`"
	uname -a
	fsck -n
	echo "\n\nFinished fsck at `date`\f"
    } >>$fscklog 2>&1	# log all output.

    echo "File system check done.  Safe to resume." | wall
fi

exit	# All done (remainder of script just has comments)


#
# SOME OTHER NICE THINGS YOU MIGHT LIKE TO DO:
#

#
# To save the list of files backed up, insert this line or equivalent
# after the "find", before the "cpio":
#

	tee /etc/backuplist	|
