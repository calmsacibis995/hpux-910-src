$set 1
1 Syntax: %s [-u Benutzer] [-g Gruppe[,Gruppe]...] [-n] Datei ...
2        %s -r [-n] Datei ...
3 -u ermittelt Zugriff f?r angegebenen Benutzer und nicht f?r die effektive ID des Aufrufenden
4 -g ermittelt Zugriff f?r angegebene Gruppen und nicht f?r Gruppen des Aufrufenden
5 -r f?hrt Zugriffspr?fungen f?r die tats?chliche ID des Aufrufenden und nicht mit der effektiven durch
6 -n gibt numerische Werte der Zugriffsrechte (0..7) und keine Zeichenkette aus
7 Benutzer oder Gruppe kann "@" f?r jede der Gruppen-ID's einer jeden Datei sein
15 -u oder -g k?nnen nicht zusammen mit -r angegeben werden
17 Mehr als %d Gruppen sind nicht erlaubt
18 NULL-Benutzer-ID nicht erlaubt
19 NULL-Gruppen-ID nicht erlaubt
20 Ung?ltige Benutzer-ID "%s"
21 Ung?ltige Grppen-ID "%s"
22 Datei "%s"
23 Datei "%s"
30  (errno = %d)
