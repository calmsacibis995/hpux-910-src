/*

    instr_assert_bwl()

    Process "assert.b", "assert.w", and "assert.l" pseudo-ops.

					assertions_tail
						|
    assertions-->+-----------+    +-----------+ +->+-----------+
		 |   next    |--->|    next   |--->| next = 0  |
		 +-----------+    +-----------+    +-----------+
		 |   name    |    |    name   |    |   name    |
		 +-----------+    +-----------+    +-----------+
		       |		|		|
		       V		V		V
	   "assertion name 1"  "assertion name 2"  "assertion name 3"

*/

#include "fizz.h"

static struct assertion_type {
    struct assertion_type *next;
    char *name;
} *assertions = (struct assertion_type *) 0,
  *assertions_tail = (struct assertion_type *) 0;

void instr_assert_bwl(ptr, size)
char *ptr;
short size;
{
    int c;
    char *name_start, *name_end;
    char *ea, *ea_start, *ea_end;
    struct assertion_type *current_assertion;

    /* must be in verify section */
    if (gSection != VERIFY) {
	error_locate(45);
	return;
    };

    /* get assert name */
    get_next_list_item(ptr, &name_start, &name_end);
    if ((name_start == (char *) 0) || (*name_end != ',')) {
	error_locate(48); /* error if no assert name */
	return;
    };

    /* get assert location */
    get_next_token(name_end + 1, &ea_start, &ea_end);
    if ((ea_start == (char *) 0) || (ea_start > name_end + 1)) {
	error_locate(48); /* error if no assert location */
	return;
    };

    /* check for valid assert name */
    ptr = name_start;
    if (!isalpha(*ptr++)) {
	error_locate(47);
	return;
    };
    while (ptr < name_end)
        if (!(isalnum(c = *ptr++) || (c == '_'))) {
	    error_locate(47);
	    return;
        };

    /* create struct and string to store assert information */
    CREATE_STRUCT(current_assertion, assertion_type);
    CREATE_STRING(current_assertion->name, name_end - name_start + 1);

    /* store information into linked list */
    (void) strncpy(current_assertion->name, name_start, name_end - name_start);
    current_assertion->name[name_end - name_start] = '\0';
    current_assertion->next = (struct assertion_type *) 0;
    if (assertions == (struct assertion_type *) 0)
	assertions = current_assertion;
    else assertions_tail->next = current_assertion;
    assertions_tail = current_assertion;

    /* print code to perform assertion */
    CREATE_STRING(ea, ea_end - ea_start + 1);
    (void) strncpy(ea, ea_start, ea_end - ea_start);
    ea[ea_end - ea_start] = '\0';
    Cprint_assertion_code(size, gAssertionCount, ea);
    free(ea);

    gAssertionCount++;

    /* check for tokens following assert instruction */
    get_next_token(ea_end, &name_start, &name_end);
    if (name_start != (char *) 0) error_locate(*name_start == '#' ? 68 : 48);

    return;
}

/*

    get_assertion_name()

    Given an assertion_number, return the name associated with it.  This
    assumes that a valid number is passed to this routine (0 < index <
    number of assertions.)

*/

char *get_assertion_name(index)
register unsigned long index;
{
    register struct assertion_type *current_assertion;

    current_assertion = assertions;
    while (index--) current_assertion = current_assertion->next;
    return current_assertion->name;
}
