/* @(#) $Revision: 66.1 $ */

/*
 * funcs.c -- some of the functions for find(1)
 */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#if (defined(DUX) || defined(DISKLESS)) && defined(CNODE_DEV)
#include <cluster.h>
#endif /* (DUX || DISKLESS) && CNODE_DEV */

extern struct stat Statb;

/*
 * Hashing stuff for nouser and nogroup
 */
#define HASHSIZE 71

/*
 * Single entry
 */
struct hash_entry
{
    long key;
    long value;
    struct hash_entry *next;
};

/*
 * Table descriptor
 */
struct hash_table
{
    int size;
    struct hash_entry **buckets;
};

void
hash_add(table, key, value)
struct hash_table *table;
long key;
long value;
{
    int slot = key % table->size;
    struct hash_entry *ptr;

    /*
     * Allocate the table if not allocated already
     */
    if (table->buckets == (struct hash_entry **)0)
    {
	table->buckets =
	    (struct hash_entry **)calloc(sizeof (struct hash_entry *),
					 table->size);
    }

    /*
     * Allocate a new entry and add it.
     */
    ptr = (struct hash_entry *)malloc(sizeof (struct hash_entry));
    ptr->key = key;
    ptr->value = value;
    ptr->next = table->buckets[slot];
    table->buckets[slot] = ptr;
}

static struct hash_entry *
hash_lookup(table, key)
struct hash_table *table;
long key;
{
    int slot;
    struct hash_entry *ptr;

    if (table->buckets == (struct hash_entry **)0)
	return (struct hash_entry *)0;

    slot = key % table->size;
    for (ptr = table->buckets[slot]; ptr; ptr = ptr->next)
	if (ptr->key == key)
	    return ptr;
    return (struct hash_entry *)0;
}

/*
 * nouser() -- return TRUE if the current file belongs to a user
 *             who isn't listed in the password database.
 */
nouser()
{
    static uid_t user = (uid_t)-1;
    static int   found;
    static struct hash_table users = { HASHSIZE, 0};
    struct hash_entry *ptr;

    /*
     * We cache the last value checked
     */
    if (Statb.st_uid == user)
	return !found;
    
    user = Statb.st_uid;
    if ((ptr = hash_lookup(&users, user)) != (struct hash_entry *)0)
	return !(found = ptr->value);

    found = (getpwuid(user) != (struct passwd *)0);
    hash_add(&users, user, found);
    return !found;
}

/*
 * nogroup() -- return TRUE if the current file belongs to a group
 *              that isn't listed in the group database.
 */
nogroup()
{
    static gid_t group = (gid_t)-1;
    static int   found;
    static struct hash_table groups = { HASHSIZE, 0};
    struct hash_entry *ptr;

    /*
     * We cache the last value checked
     */
    if (Statb.st_gid == group)
	return !found;
    
    group = Statb.st_gid;
    if ((ptr = hash_lookup(&groups, group)) != (struct hash_entry *)0)
	return !(found = ptr->value);

    found = (getgrgid(group) != (struct group *)0);
    hash_add(&groups, group, found);
    return !found;
}

#if (defined(DUX) || defined(DISKLESS)) && defined(CNODE_DEV)
/*
 * nodevcid() -- return TRUE if the current file is a device file
 *		 who's cnode id is not listed in /etc/clusterconf.
 */
nodevcid()
{
    static cnode_t cnode = (cnode_t)-1;
    static int     found;
    static struct hash_table cnodes = { HASHSIZE, 0};
    struct hash_entry *ptr;

    /*
     * If it isn't a block or character special file, return FALSE
     */
    if (!S_ISCHR(Statb.st_mode) && !S_ISBLK(Statb.st_mode))
	return 0;

    /*
     * We cache the last value checked
     */
    if (Statb.st_rcnode == cnode)
	return !found;
    
    cnode = Statb.st_rcnode;
    if ((ptr = hash_lookup(&cnodes, cnode)) != (struct hash_entry *)0)
	return !(found = ptr->value);

    found = (getcccid(cnode) != (struct cct_entry *)0);
    hash_add(&cnodes, cnode, found);
    return !found;
}
#endif /* (DUX || DISKLESS) && CNODE_DEV */
