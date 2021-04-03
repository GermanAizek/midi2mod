/*
 * stringcatalog.c
 *
 */

#include <stdio.h>

#include "stringcatalog.h"

const char *
get_string(int key, const StringEntry *catalog, size_t catalog_length)
{
	size_t i;
	for (i = 0; i < catalog_length; i++) {
		if (catalog[i].key == key) {
			return catalog[i].value;
		}
	}

	return NULL;
}

