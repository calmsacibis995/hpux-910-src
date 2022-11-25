$set 1
1 %s: "%s": UngÏltiger Parameter.\n
2 %s: Sprache %s unbekannt. Standardsprache wird verwendet.\n
3 %s: Berechtigung verweigert. Nicht Superuser.\n
4 %s: Warnung: Kein Vorgang durchgefÏhrt.\n
5 %s: GerÌt %s kann nicht geÎffnet werden.\n
6 %s: Kein "character device" %s.\n
7 %s: Kein SCSI device %s.\n
8 %s: UngÏltiges GerÌte-Argument %s.\n
9 immediate_report setzen
10 immediate_report anfordern
11 %s: UngÏltiges magneto-optisches GerÌt %s fÏr LÎschvorgang.\n
12 %s: UngÏltiges Abschnitt-2-GerÌt %s fÏr LÎschvorgang.\n
13 LÎschen
14 Syntax: %s [-akq] [-m Modus[=Wert]]... [-c Befehl]... GerÌt\n
$
$ L10N Hinweis: Die beiden folgenden Meldungen werden zusammen verwendet.
$		Die gesamte Fehlermeldung hat etwa folgenden Inhalt:
$
$		     scsictl: Device ioctl ("msg") error
$
$	      where "msg" would be a localized message.
$	      Do not localize "ioctl" since it is a Unix terminlolgy.
15 : GerÌt ioctl (
16 ) Fehler
