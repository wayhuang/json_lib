#include <fcntl.h>
#include <unistd.h>

#include "json.h"

static char *parse_value(char *begin, char *end, size_t *offset, size_t *len,
	enum json_type *type);
static char *parse_string(char *begin, char *end);
static json_data *json_data_from_buf(buf_t *buf);

static int is_blank(char c)
{
	if ((c == ' ') || (c == '\t'))
		return 1;
	return 0;
}

static int is_endofline(char c)
{
	if ((c == '\n') || (c == '\r'))
		return 1;
	return 0;
}


static json_data *json_data_alloc()
{
	json_data *d;

	d = (json_data *)malloc(sizeof(*d));
	if (!d) {
		perror("malloc json error");
		return NULL;
	}

	d->buf = NULL;
	d->name.p = NULL;
	d->name.len = 0;
	TAILQ_INIT(&d->head);

	return d;
}

void json_data_free(json_data *d)
{
	json_data *p, *tmp;

	if (d) {
		TAILQ_FOREACH_SAFE(p, &d->head, next, tmp) {
			TAILQ_REMOVE(&d->head, p, next);
			json_data_free(p);
		}
		if (d->buf)
			free(d->buf);
		free(d);
	}
}

void print_buf(buf_t *buf)
{
	size_t i;
	char *p;

	if (!buf)
		return;

	p = buf->p;
	if (p)
		for (i = 0; i < buf->len; i++)
			printf("%c", *p++);
	else
		printf("(null)");

	printf("\n");
}

static int json_parse_object(json_data *d)
{
	char *begin = d->value.p + 1;
	char *end = d->value.p + d->value.len;
	char *p = begin;
	buf_t name;
	size_t offset;
	size_t len;
	enum json_type type;
	json_data *e;
	int completed = 0;
	int ret = 0;

	while (p < end) {
		if (is_blank(*p) || is_endofline(*p)) {
			p++;
			continue;
		}
		switch (*p) {
		case '\"':
			begin = p;
			p = parse_string(begin, end);
			if (p) {
				name.p = begin;
				name.len = p - begin + 1;
			}
			break;
		case ':':
			begin = p + 1;
			p = parse_value(begin, end, &offset, &len, &type);
			if (p && (len > 0)) {
				e = json_data_alloc();
				if (e) {
					e->type = type;
					e->name.p = name.p;
					e->name.len = name.len;
					e->value.p = begin + offset;
					e->value.len = len;
					TAILQ_INSERT_TAIL(&d->head, e, next);
				}
			}
			break;
		case '}':
			completed = 1;
			break;
		default:
			break;
		}
		if (!p || completed)
			break;
		p++;
	}

	return ret;
}

static int json_parse_array(json_data *d)
{
	char *begin = d->value.p;
	char *end = d->value.p + d->value.len;
	char *p = begin;
	size_t offset;
	size_t len;
	enum json_type type;
	json_data *e;
	int completed = 0;
	int ret = 0;

	while (p < end) {
		if (is_blank(*p) || is_endofline(*p)) {
			p++;
			continue;
		}
		switch (*p) {
		case '[':
		case ',':
			begin = p + 1;
			p = parse_value(begin, end, &offset, &len, &type);
			if (p && (len > 0)) {
				e = json_data_alloc();
				if (e) {
					e->type = type;
					e->value.p = begin + offset;
					e->value.len = len;
					TAILQ_INSERT_TAIL(&d->head, e, next);
				}
			}
			break;
		case ']':
			completed = 1;
			break;
		default:
			break;
		}
		if (!p || completed)
			break;
		p++;
	}

	return ret;
}

json_data *json_data_get_by_name(json_data *d, const char *name)
{
	json_data *p = NULL;

	if (!d || !name)
		return NULL;

	if (d->type != OBJECT) {
		printf("json data is not object [type: %d]\n", d->type);
		return NULL;
	}

	if (TAILQ_EMPTY(&d->head)) {
		if (json_parse_object(d))
			return NULL;
	}

	if (!TAILQ_EMPTY(&d->head)) {
		TAILQ_FOREACH(p, &d->head, next) {
			if (!strncmp(p->name.p+1, name, p->name.len-2))
				break;
		}
	}

	return p;
}

json_data *json_data_get_by_index(json_data *d, int idx)
{
	json_data *p = NULL;
	int i = 0;

	if (!d || (index < 0))
		return NULL;

	if (d->type != ARRAY) {
		printf("json data is not array\n");
		return NULL;
	}

	if (TAILQ_EMPTY(&d->head)) {
		if (json_parse_array(d))
			return NULL;
	}

	if (!TAILQ_EMPTY(&d->head)) {
		TAILQ_FOREACH(p, &d->head, next) {
			if (i++ == idx)
				break;
		}
	}

	return p;
}

static int buf_to_bool(buf_t *buf, int *val)
{
	char *p = buf->p;
	size_t len = buf->len;
	int ret = 0;

	switch (len) {
	case 2:
		if (!strncmp(p, "no", len))
			*val = 0;
		else
			ret = -1;
	case 3:
		if (!strncmp(p, "yes", len))
			*val = 1;
		else
			ret = -1;
	case 4:
		if (!strncmp(p, "null", len))
			*val = 0;
		else if (!strncmp(p, "true", len))
			*val = 1;
		else
			ret = -1;
	case 5:
		if (!strncmp(p, "false", len))
			*val = 0;
		else
			ret = -1;
	default:
		ret = -1;
		break;
	}

	return ret;
}

int json_data_to_ulong(json_data *d, unsigned long *val)
{
	char *p;
	size_t len;
	char *endptr = NULL;
	char number[128];
	int ival;
	int ret = 0;

	if (!d || !val)
		return -1;

	if (d->type != MISC) {
		printf("json data cannot be convert to number\n");
		return -1;
	}

	p = d->value.p;
	len = d->value.len;
	if ((*p >= '0') && (*p <= '9')) {
		if (len < 128) {
			memcpy(number, p, len);
			number[len] = '\0';
			*val = strtoul(number, &endptr, 0);
			if (endptr != &number[len])
				ret = -1;
		} else {
			ret = -1;
		}
	} else {
		ret = buf_to_bool(&d->value, &ival);
		*val = (unsigned long)ival;
	}

	return ret;
}

int json_data_to_long(json_data *d, long *val)
{
	char *p;
	size_t len;
	char *endptr = NULL;
	char number[128];
	int ival;
	int ret = 0;

	if (!d || !val)
		return -1;

	if (d->type != MISC) {
		printf("json data cannot be convert to number\n");
		return -1;
	}

	p = d->value.p;
	len = d->value.len;
	if (((*p == '-') && (*(p+1) >= '0') && (*(p+1) <= '9')) ||
		((*p >= '0') && (*p <= '9'))) {
		if (len < 128) {
			memcpy(number, p, len);
			number[len] = '\0';
			*val = strtol(number, &endptr, 0);
			if (endptr != &number[len])
				ret = -1;
		} else {
			ret = -1;
		}
	} else {
		ret = buf_to_bool(&d->value, &ival);
		*val = (long)ival;
	}

	return ret;
}

int json_data_to_string(json_data *d, char *str, size_t size)
{
	size_t len;

	if (!d || !str)
		return -1;

	if (d->type != STRING) {
		printf("json data is not string\n");
		return -1;
	}

	if (d->value.len < 2) {
		printf("incomplete string\n");
		return -1;
	}

	len = d->value.len - 2;
	if (size <= len) {
		printf("space is not enough to contain string (%zu bytes)\n", len);
		return -1;
	}

	if (len > 0)
		memcpy(str, d->value.p+1, len);
	str[len] = '\0';

	return 0;
}

static int parse_head(char *begin, char *end, size_t *offset,
	enum json_type *type)
{
	char *p = begin;
	int ret = 0;

	while (p < end) {
		if (!is_blank(*p) && !is_endofline(*p))
			break;
		p++;
	}

	if (p < end) {
		if (offset)
			*offset = p - begin;
		if (type) {
			switch (*p) {
			case '{':
				*type = OBJECT;
				break;
			case '[':
				*type = ARRAY;
				break;
			case '\"':
				*type = STRING;
				break;
			default:
				*type = MISC;
				break;
			}
		}
	} else {
		ret = -1;
	}

	return ret;
}

/* parse buffer begin with '{' */
static char *parse_object(char *begin, char *end)
{
	char *p = begin + 1;
	int completed = 0;

	while (p < end) {
		if (is_blank(*p) || is_endofline(*p)) {
			p++;
			continue;
		}
		switch (*p) {
		case '\"':
			begin = p;
			p = parse_string(begin, end);
			if (p == (begin + 1)) {
				printf("'name' is empty\n");
				p = NULL;
			}
			break;
		case ':':
			begin = p + 1;
			p = parse_value(begin, end, NULL, NULL, NULL);
			break;
		case '}':
			completed = 1;
			break;
		default:
			break;
		}
		if (!p || completed)
			break;
		p++;
	}

	if (!completed)
		printf("'}' is missing\n");

	return p;
}

/* parse buffer begin with '[' */
static char *parse_array(char *begin, char *end)
{
	char *p = begin;
	int completed = 0;

	while (p < end) {
		if (is_blank(*p) || is_endofline(*p)) {
			p++;
			continue;
		}
		switch (*p) {
		case '[':
		case ',':
			begin = p + 1;
			p = parse_value(begin, end, NULL, NULL, NULL);
			break;
		case ']':
			completed = 1;
			break;
		default:
			break;
		}
		if (!p || completed)
			break;
		p++;
	}

	if (!completed)
		printf("']' is missing\n");

	return p;
}

/* parse buffer begin with '"' */
static char *parse_string(char *begin, char *end)
{
	char *p = begin + 1;
	int completed = 0;

	while (p < end) {
		if (is_blank(*p) || is_endofline(*p)) {
			p++;
			continue;
		}
		switch (*p) {
		case '"':
			completed = 1;
			break;
		default:
			break;
		}
		if (completed)
			break;
		p++;
	}

	if (!completed)
		printf("'\"' is missing\n");

	return p;
}

/* buffer begin without '{', '[' and '"' */
static char *parse_misc(char *begin, char *end)
{
	char *p = begin;
	int completed = 0;
	int pads = 0;

	while (p < end) {
		if (is_blank(*p) || is_endofline(*p)) {
			pads++;
			p++;
			continue;
		}
		switch (*p) {
		case '}':
		case ']':
		case ',':
			completed = 1;
			break;
		default:
			pads = 0;
			break;
		}
		if (completed)
			break;
		p++;
	}

	return p - pads - 1;
}

static char *parse_value(char *begin, char *end, size_t *offset, size_t *len,
	enum json_type *type)
{
	char *p;
	buf_t buf;
	size_t o;
	enum json_type t;

	if (parse_head(begin, end, &o, &t))
		return NULL;
	begin += o;

	switch (t) {
	case OBJECT:
		p = parse_object(begin, end);
		break;
	case ARRAY:
		p = parse_array(begin, end);
		break;
	case STRING:
		p = parse_string(begin, end);
		break;
	default:
		p = parse_misc(begin, end);
		break;
	}

	if (offset)
		*offset = o;
	if (len && p)
		*len = p - begin + 1;
	if (type)
		*type = t;

	return p;
}

static json_data *json_data_from_buf(buf_t *buf)
{
	char *end;
	size_t offset;
	size_t len;
	enum json_type type;
	json_data *d;

	end = parse_value(buf->p, buf->p + buf->len, &offset, &len, &type);
	if (!end || (len == 0))
		return NULL;

	d = json_data_alloc();
	if (d) {
		d->type = type;
		d->value.p = buf->p + offset;
		d->value.len = len;
	}

	return d;
}

json_data *json_data_from_string(const char *str)
{
	buf_t b;
	json_data *d;

	if (!str) {
		printf("string is not specified\n");
		return NULL;
	}

	b.len = strlen(str);
	if (b.len == 0) {
		printf("string is empty\n");
		return NULL;
	}

	b.p = (char *)malloc(b.len);
	if (!b.p) {
		perror("malloc buf error");
		return NULL;
	}

	memcpy(b.p, str, b.len);

	d = json_data_from_buf(&b);
	if (d)
		d->buf = b.p;

	return d;
}

json_data *json_data_from_file(const char *file)
{
	int fd;
	ssize_t n;
	buf_t b;
	json_data *d = NULL;

	if (!file) {
		printf("file is not specified\n");
		return NULL;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		perror("open error");
		return NULL;
	}

	b.len = lseek(fd, 0, SEEK_END);
	if (!b.len) {
		printf("file '%s' is empty\n", file);
		goto end;
	}

	b.p = (char *)malloc(b.len);
	if (!b.p) {
		perror("malloc buf error");
		goto end;
	}

	lseek(fd, 0, SEEK_SET);
	n = read(fd, b.p, b.len);
	if (n < 0) {
		perror("read error");
		free(b.p);
		goto end;
	}
	if (n != (ssize_t)b.len) {
		printf("read %zd bytes data which is less than file size %zu",
			n, b.len);
		free(b.p);
		goto end;
	}

	d = json_data_from_buf(&b);
	if (d)
		d->buf = b.p;

end:
	close(fd);
	return d;
}
