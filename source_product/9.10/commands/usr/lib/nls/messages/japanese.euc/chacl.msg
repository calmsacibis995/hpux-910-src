$set 1
1 使用法:
2 %s acl file ...           存在している ACL を修正します。(エントリを追加するかモードを修正)
3 %s -r acl file ...        ファイル上の 古い ACL を置き換えます。
4 %s -d aclpatt file ...    指定されたエントリがある場合には ACL から削除します。
5 %s -f fromfile tofile ... fromfile から tofiles へ ACL をコピーします。-r を暗黙裡に指定します。
6 %s -[z|Z|F] file ...         -z ACL のオプショナル・エントリを zap (削除) します。
7                              -Z ACL のすべてのエントリを zap (削除) します。(０をアクセス)
8                              -F fold オプショナル・エントリをベース・エントリに組み込みます。
9 
10 ACL 入力の演算子・フォームの例:
11 
12     %s 'ajs.adm=7, @.%%=rw, ggd.%%+w-x, %%.@-rwx, %%.adm+x, %%.%%=0' files...
13 
14 ACL 入力のショート・フォームの例:
15 
16     %s '(ajs.adm,7)(@.%%,rw)(ggd.%%,-w-),(%%.@,0)(%%.adm,rx),(%%.%%,0)' files...
17 
18 %% は特定のユーザやグループがないことを意味します。
19 @ は現在のファイルのオーナやグループを意味します。
20 * は %% を含む任意のユーザやグループを意味します。(パターンのみ)
21 パターンで "*" はワイルドカード・モードの値か何もないものとして使われます。
30 (エラーはありません)
31 ショート・フォームの ACL エントリは "(" では始まりません。
32 ショート・フォームの ACL エントリは ")" では終わりません。
33 ACL エントリのユーザ名がドットでターミネイトされません。
34 ACL エントリのグループ名が正しくターミネイトされません。
35 ACL エントリのユーザ名が null です。
36 ACL エントリのグループ名が null です。
37 ACL エントリのユーザ名が正しくありません。
38 ACL エントリのグループ名が正しくありません。
39 ACL エントリのモードが正しくありません。
40 17 以上の ACL エントリが入力されました。
41 strtoacl() で未定義のエラーが発生しました。
42 strtoaclpatt() で未定義のエラーが発生しました。
50 -rdfzZF の１つだけが使用できます。
51 ACLが正しくありません: %s%s: "%s"
52  (errno = %d)
53 OneFile (%c) でエラーがあります。\n
54 ファイル "%s": ACL を使えません: %s: "%s"
55 ファイル "%s"
56 ファイル "%s": "%s" のために削除する ACL エントリに一致するものがありません。
