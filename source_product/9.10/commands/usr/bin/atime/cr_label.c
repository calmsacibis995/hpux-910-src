/*

    create_label()

    Create a unique label number.

*/

#include "fizz.h"

unsigned long create_label()
{
    static unsigned long label_number = 1;

    return label_number++;
}
