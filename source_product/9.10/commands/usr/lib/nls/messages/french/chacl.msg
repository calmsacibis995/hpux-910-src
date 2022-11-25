$set 1
1 syntaxe:
2 %s acl fich ...           modifie ACL existant (ajout d'entr≈es ou\n                             modif de modes)
3 %s -r acl fich ...        remplace anciennes ACL par celles donn≈es\n                             dans fichiers
4 %s -d aclexpr fich ...    supprime les entr≈es sp≈cifi≈es dans les ACL
5 %s -f du_fich au_fich ... copie ACL du fich dans fichiers; -r implicite
6 %s -[z|Z|F] fich ...      -z supprime les entr≈es optionnelles dans les ACL
7                              -Z supprime toutes les entr≈es (aucun acc…s)
8                              -F int…gre les entr≈es optionnelles aux\n                             entr≈es de base
9 
10 Exemple d'entr≈e ACL sous forme d'op≈rateurs:
11 
12   %s 'ajs.adm=7, @.%%=rw, ggd.%%+w-x, %%.@-rwx, %%.adm+x, %%.%%=0' fich...
13 
14 Exemple d'entr≈e ACL sous forme abr≈g≈e:
15 
16   %s '(ajs.adm,7)(@.%%,rw)(ggd.%%,-w-),(%%.@,0)(%%.adm,rx),(%%.%%,0)' fich...
17 
18 %% signifie "aucun utilisateur ou groupe sp≈cifique"
19 @ signifie  "propri≈taire ou groupe du fichier en cours"
20 * signifie "tout utilisateur ou groupe, y compris %%" (expressions seulement)
21 Dans les expressions, "*" peut servir de valeur de remplacement.
30 (aucune erreur)
31 l'entr≈e ACL ne d≈bute pas par "(" dans\nla forme abr≈g≈e
32 l'entr≈e ACL ne se termine pas par ")" dans\nla forme abr≈g≈e
33 le nom de l'utilisateur ne se termine pas par un point\ndans l'entr≈e ACL
34 le nom de groupe ne se termine pas correctement dans\nl'entr≈e ACL
35 le nom de l'utilisateur est nul dans l'entr≈e ACL
36 le nom de groupe est nul dans l'entr≈e ACL
37 nom d'utilisateur non valide dans l'entr≈e ACL
38 nom de groupe non valide dans l'entr≈e ACL
39 mode non valide dans l'entr≈e ACL
40 plus de 16 entr≈es ACL dans la cha—ne
41 erreur inconnue provenant de strtoacl()
42 erreur inconnue provenant de strtoaclpatt()
50 vous ne pouvez entrer qu'une des options -rdfzZF » la fois
51 ACL non valide: %s%s: "%s"
52  (num≈ro de l'erreur = %d)
53 d≈faut dans l'un des fichiers (%c)\n
54 fichier "%s": impossible de mettre en application l'ACL: %s: "%s"
55 fichier "%s"
56 fichier "%s": aucune entr≈e ACL correspondante » supprimer pour "%s"
