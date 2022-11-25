$set 1
1 %s: "%s": Invalid parameter.\n
2 %s: Language %s unknown.  Proceeding with the default language.\n
3 %s: Permission denied. Not super-user.\n
4 %s: Warning: no operation performed.\n
5 %s: Can't open device %s.\n
6 %s: Not a character device %s.\n
7 %s: Not a SCSI device %s.\n
8 %s: Invalid device argument %s.\n
9 set immediate_report
10 get immediate_report
11 %s: Invalid magneto-optical device %s for erase.\n
12 %s: Invalid section-2 device %s for erase.\n
13 erase
14 Usage: %s [-akq] [-m mode[=value]]... [-c command]... device\n
$
$ L10N note:  The following 2 messages are used together.  The entire error 
$             message should look somthing like this:
$
$	             scsictl: Device ioctl ("msg") error
$
$             where "msg" would be a localized message.
$             Do not localize "ioctl" since it is a Unix terminlolgy.
15 : Device ioctl (
16 ) error
