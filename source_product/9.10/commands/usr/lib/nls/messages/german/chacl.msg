$set 1
1 Syntax:
2 %s Acldat ...         ?ndere ACLs (Eintr?ge hinzuf?gen oder Modus ?ndern)
3 %s -r Acldat ...      ersetze alte ACLs mit gegebenen ACL zu Dateien
4 %s -d Aclpattdat ...  l?sche angegebe Eintr?ge, falls vorhanden, aus ACLs
5 %s -f Vondat Nachdat  kopiere ACL von vondat nach nachdat; beinhaltet -r
6 %s -[z|Z|F] Datei ...    -z L?schen optionaler Eintr?ge in ACLs
7                             -Z L?schen aller Eintr?ge (Null-Zugriff)
8                             -F ?ndert optionale Eintr?ge in Basiseintr?ge
9 
10 Beispiel: ACL-Operatorformat
11 
12  %s 'ajs.adm=7, @.%%=rw, ggd.%%+w-x, %%.@-rwx, %%.adm+x, %%.%%=0' Dateien...
13 
14 Beispiel: ACL-Kurzformat
15 
16  %s '(ajs.adm,7)(@.%%,rw)(ggd.%%,-w-),(%%.@,0)(%%.adm,rx),(%%.%%,0)' Dateien...
17 
18 %% bedeutet "kein(e) spezifische(r) Anwender oder Gruppe"
19 @ bedeutet "aktuelle(r) Dateieigent?mer oder Gruppe"
20 * bedeutet "beliebige(r) Anwender oder Gruppe, einschlie?lich %%" (nur Muster)
21 In Mustern kann "*" als Platzhalterwert oder f?r fehlende Werte stehen.
30 (kein Fehler)
31 ACL-Eintrag beginnt nicht mit "(" in der Kurzform
32 ACL-Eintrag endet nicht mit ")" in der Kurzform
33 Benutzername endet nicht mit einem Punkt in ACL-Eintrag
34 Gruppename endet nicht korrekt in ACL-Eintrag
35 Benutzername gleich Null in ACL-Eintrag
36 Gruppenname gleich Null in ACL-Eintrag
37 Fehlerhafter Benutzername in ACL-Eintrag
38 Fehlerhafter Gruppenname in ACL-Eintrag
39 Fehlerhafter Modus in ACL-Eintrag
40 Mehr als 16 ACL-Eintr?ge im Eintrag
41 Unbekannter Fehler von strtoacl()
42 Unbekannter Fehler von strtoaclpatt()
50 Lediglich eine Auswahl aus -rdfzZF ist zugelassen
51 Fehlerhafter ACL: %s%s: "%s"
52  (errno = %d)
53 Fehler OneFile (%c)\n
54 Datei "%s": ACL kann nicht angewendet werden auf %s: "%s"
55 Datei "%s"
56 Datei "%s": Kein entsprechender zu l?schender ACL-Eintrag f?r "%s"
