$set 1
1 usage:
2 %s acl file ...           modify existing ACLs (add entries or modify modes)
3 %s -r acl file ...        replace old ACLs with given ACL on files
4 %s -d aclpatt file ...    delete specified entries, if present, from ACLs
5 %s -f fromfile tofile ... copy ACL from fromfile to tofiles; implies -r
6 %s -[z|Z|F] file ...      -z zap (delete) optional entries in ACLs
7                              -Z zap (delete) all entries (zero access)
8                              -F fold optional entries to base entries
9 
10 Operator form ACL input example:
11 
12     %s 'ajs.adm=7, @.%%=rw, ggd.%%+w-x, %%.@-rwx, %%.adm+x, %%.%%=0' files...
13 
14 Short form ACL input example:
15 
16     %s '(ajs.adm,7)(@.%%,rw)(ggd.%%,-w-),(%%.@,0)(%%.adm,rx),(%%.%%,0)' files...
17 
18 %% means "no specific user or group"
19 @ means "current file owner or group"
20 * means "any user or group, including %%" (patterns only)
21 In patterns "*" can be used for wildcard mode values, or they can be absent.
30 (no error)
31 ACL entry doesn't start with "(" in short form
32 ACL entry doesn't end with ")" in short form
33 user name not terminated with dot in ACL entry
34 group name not terminated correctly in ACL entry
35 user name is null in ACL entry
36 group name is null in ACL entry
37 invalid user name in ACL entry
38 invalid group name in ACL entry
39 invalid mode in ACL entry
40 more than 16 ACL entries at entry
41 unknown error from strtoacl()
42 unknown error from strtoaclpatt()
50 only one of -rdfzZF is allowed at a time
51 invalid ACL: %s%s: "%s"
52  (errno = %d)
53 defect in OneFile (%c)\n
54 file "%s": can't apply ACL: %s: "%s"
55 file "%s"
56 file "%s": no matching ACL entry to delete for "%s"
