/*
 * stringcatalog.h
 *
 */

#ifndef STRINGCATALOG_H
#define STRINGCATALOG_H

typedef struct {
	int key;
	char *value;
} StringEntry;

const char *get_string(int key, const StringEntry *catalog, int catalog_length);

#endif /* STRINGCATALOG_H */

