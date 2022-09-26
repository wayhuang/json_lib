#ifndef __JSON_PARSER__
#define __JSON_PARSER__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

enum json_type {
	STRING = 0,
	OBJECT,
	ARRAY,
	MISC
};

typedef struct {
	char *p;
	size_t len;
} buf_t;

TAILQ_HEAD(json_list, _json_data);

typedef struct _json_data {
	TAILQ_ENTRY(_json_data) next;
	enum json_type type;
	char *buf;
	buf_t name;
	buf_t value;
	struct json_list head;
} json_data;

void print_buf(buf_t *buf);
json_data *json_data_get_by_name(json_data *item, const char *name);
json_data *json_data_get_by_index(json_data *item, int idx);
int json_data_to_long(json_data *item, long *val);
int json_data_to_ulong(json_data *item, unsigned long *val);
int json_data_to_string(json_data *item, char *str, size_t size);
json_data *json_data_from_string(const char *str);
json_data *json_data_from_file(const char *file);
json_data *json_data_get(json_data *obj);
void json_data_free(json_data *obj);

#endif /* __JSON_PARSER__ */
