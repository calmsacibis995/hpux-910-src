$set 1
1 usage: %s [-u user] [-g group[,group]...] [-n] file ...
2        %s -r [-n] file ...
3 -u determine access for given user instead of caller's effective ID
4 -g determine access for given groups instead of caller's groups
5 -r do access checks using caller's real IDs instead of effective
6 -n print numeric value of access rights (0..7) instead of string
7 user or group can be "@" for each file's group ID in turn
15 can't give -u or -g along with -r
17 more than %d groups not allowed
18 null user ID not allowed
19 null group ID not allowed
20 invalid user ID "%s"
21 invalid group ID "%s"
22 file "%s"
23 file "%s"
30  (errno = %d)
