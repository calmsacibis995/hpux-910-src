$set 1
1 %s: "%s": Ung?ltiger Parameter.\n
2 %s: Sprache %s unbekannt. Standardsprache wird verwendet.\n
3 %s: Berechtigung verweigert. Nicht Superuser.\n
4 %s: Warnung: Kein Vorgang durchgef?hrt.\n
5 %s: Ger?t %s kann nicht ge?ffnet werden.\n
6 %s: Kein "character device" %s.\n
7 %s: Kein SCSI device %s.\n
8 %s: Ung?ltiges Ger?te-Argument %s.\n
9 immediate_report setzen
10 immediate_report anfordern
11 %s: Ung?ltiges magneto-optisches Ger?t %s f?r L?schvorgang.\n
12 %s: Ung?ltiges Abschnitt-2-Ger?t %s f?r L?schvorgang.\n
13 L?schen
14 Syntax: %s [-akq] [-m Modus[=Wert]]... [-c Befehl]... Ger?t\n
$
$ L10N Hinweis: Die beiden folgenden Meldungen werden zusammen verwendet.
$		Die gesamte Fehlermeldung hat etwa folgenden Inhalt:
$
$		     scsictl: Device ioctl ("msg") error
$
$	      where "msg" would be a localized message.
$	      Do not localize "ioctl" since it is a Unix terminlolgy.
15 : Ger?t ioctl (
16 ) Fehler
