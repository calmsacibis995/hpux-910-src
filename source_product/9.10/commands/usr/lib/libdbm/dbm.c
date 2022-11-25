/* @(#) $Revision: 51.3 $    $Date: 87/08/07 10:17:57 $ */

#include	<dbm.h>

#define	NODB	((DBM *)0)

static DBM *cur_db = NODB;

static char no_db[] = "dbm: no open database\n";

dbminit(file)
	char *file;
{
	if (cur_db != NODB)
		dbm_close(cur_db);

	cur_db = dbm_open(file, 2, 0);
	if (cur_db == NODB) {
		cur_db = dbm_open(file, 0, 0);
		if (cur_db == NODB)
			return (-1);
	}
	return (0);
}
/*	Nobody appears to call this function 
long
forder(key)
datum key;
{
	if (cur_db == NODB) {
		printf(no_db);
		return (0L);
	}
	return (dbm_forder(cur_db, key));
}
*/

datum
fetch(key)
datum key;
{
	datum item;

	if (cur_db == NODB) {
		printf(no_db);
		item.dptr = 0;
		return (item);
	}
	return (dbm_fetch(cur_db, key));
}

delete(key)
datum key;
{
	if (cur_db == NODB) {
		printf(no_db);
		return (-1);
	}
	if (dbm_rdonly(cur_db))
		return (-1);
	return (dbm_delete(cur_db, key));
}

store(key, dat)
datum key, dat;
{
	if (cur_db == NODB) {
		printf(no_db);
		return (-1);
	}
	if (dbm_rdonly(cur_db))
		return (-1);

	return (dbm_store(cur_db, key, dat, DBM_REPLACE));
}

datum
firstkey()
{
	datum item;

	if (cur_db == NODB) {
		printf(no_db);
		item.dptr = 0;
		return (item);
	}
	return (dbm_firstkey(cur_db));
}

datum
nextkey(key)
datum key;
{
	datum item;
	datum dbm_do_nextkey();

	if (cur_db == NODB) {
		printf(no_db);
		item.dptr = 0;
		return (item);
	}
	return (dbm_do_nextkey(cur_db, key));
}
void
dbmclose()
{
	if (cur_db != NODB)
		dbm_close(cur_db);
}
