/*
 * stringcatalog.c
 *
 */

#include <stdio.h>

#include "stringcatalog.h"

const char *
get_string(int key, const StringEntry *catalog, int catalog_length)
{
	unsigned int i;
	for(i=0; i < catalog_length; i++) {
		if(catalog[i].key == key) return catalog[i].value;
	}

	return NULL;
}

