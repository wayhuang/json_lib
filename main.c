#include <getopt.h>
#include <inttypes.h>

#include "json.h"

int main(int argc, char *argv[])
{
	struct option longopts[] = {
		{"file", required_argument, 0, 'f'},
		{"string", required_argument, 0, 's'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	const char *file = "test.json";
	const char *str = NULL;
	json_data *d, *e, *g, *p;
	int help = 0;
	int i, ret;
	unsigned long val;
	char sval[64];

	while ((ret = getopt_long(argc, argv, "f:s:h", longopts, NULL)) != -1) {
		if (optarg && (*optarg == '='))
			optarg++;
		switch (ret) {
		case 'f':
			file = optarg;
			break;
		case 's':
			str = optarg;
			break;
		case 'h':
			help = 1;
			break;
		case '?':  /* unknown option */
		case ':':  /* no argument with option */
			printf("%c\n", ret);
			break;
		default:
			break;
		}
	}

	if (help) {
		printf("Options:\n");
		printf("\t--file,-f\t[FILE]\n");
		printf("\t--string,-s\t[STR]\n");
		return 0;
	}

	if (str) {
		d = json_data_from_string(str);
		if (!d) {
			printf("create json data from string failed\n");
			return -1;
		}
	} else {
		d = json_data_from_file(file);
		if (!d) {
			printf("create json data from file '%s' failed\n", file);
			return -1;
		}
	}

	if (!strcmp(file, "test.json")) {
		e = json_data_get_by_name(d, "version");
		if (!e) {
			printf("version not found\n");
		} else {
			if (e->type == MISC) {
				if (!json_data_to_ulong(e, &val))
					printf("version: %"PRIu64"\n", val);
			} else
				printf("version type is %d\n", e->type);
		}
		e = json_data_get_by_name(d, "queues_per_sp");
		if (!e) {
			printf("queues_per_sp not found\n");
		} else {
			if (e->type == MISC) {
				if (!json_data_to_ulong(e, &val))
					printf("queues_per_sp: %"PRIu64"\n", val);
			} else
				printf("queues_per_sp type is %d\n", e->type);
		}
		e = json_data_get_by_name(d, "service_core_en");
		if (!e) {
			printf("service_core_en not found\n");
		} else {
			if (e->type == MISC) {
				if (!json_data_to_ulong(e, &val))
					printf("service_core_en: %"PRIu64"\n", val);
			} else
				printf("service_core_en type is %d\n", e->type);
		}
	
		g = json_data_get_by_name(d, "flow_match");
		if (!g) {
			printf("flow_match not found\n");
		} else {
			if (g->type == ARRAY) {
				for (i = 0; i < 4; i++) {
					p = json_data_get_by_index(g, i);
					if (p) {
						if (p->type == OBJECT) {
							e = json_data_get_by_name(p, "ip_ttl_en");
							if (e) {
								if (e->type == MISC) {
									if (!json_data_to_ulong(e, &val))
										printf("%d: ip_ttl_en: %"PRIu64"\n", i, val);
									else
										printf("%d: ip_ttl_en: covert failed\n", i);
								} else
									printf("ip_ttl_en type is %d\n", e->type);
							} else {
								printf("ip_ttl_en not found\n");
							}
						} else if (p->type == MISC) {
							if (!json_data_to_ulong(p, &val))
								printf("%d: %"PRIu64"\n", i, val);
							else
								printf("%d: covert failed\n", i);
						} else if (p->type == STRING) {
							if (!json_data_to_string(p, sval, sizeof(sval)))
								printf("%d: %s\n", i, sval);
						} else {
							printf("array data type is %d\n", p->type);
						}
					} else {
						printf("%d: out of array\n", i);
						continue;
					}
				}
			} else {
				printf("flow_match type is %d\n", g->type);
			}
		}
	} else if (!strcmp(file, "tcam.json")) {
		for (i = 0; i < 9; i++) {
			snprintf(sval, sizeof(sval), "%d", i);
			e = json_data_get_by_name(d, sval);
			if (!e) {
				printf("\"%s\": not found\n", sval);
			} else {
				if (e->type == OBJECT) {
					p = json_data_get_by_name(e, "key0");
					if (p->type == MISC) {
						if (!json_data_to_ulong(p, &val))
							printf("key0: %"PRIu64"\n", val);
					} else {
						printf("key0 type is %d\n", p->type);
					}
					p = json_data_get_by_name(e, "key1");
					if (p->type == MISC) {
						if (!json_data_to_ulong(p, &val))
							printf("key1: %"PRIu64"\n", val);
					} else {
						printf("key1 type is %d\n", p->type);
					}
					p = json_data_get_by_name(e, "msk0");
					if (p->type == MISC) {
						if (!json_data_to_ulong(p, &val))
							printf("msk0: %"PRIu64"\n", val);
					} else {
						printf("msk0 type is %d\n", p->type);
					}
					p = json_data_get_by_name(e, "msk1");
					if (p->type == MISC) {
						if (!json_data_to_ulong(p, &val))
							printf("msk1: %"PRIu64"\n", val);
					} else {
						printf("msk1 type is %d\n", p->type);
					}
					p = json_data_get_by_name(e, "rst0");
					if (p->type == MISC) {
						if (!json_data_to_ulong(p, &val))
							printf("rst0: %"PRIu64"\n", val);
					} else {
						printf("rst0 type is %d\n", p->type);
					}
					p = json_data_get_by_name(e, "rst1");
					if (p->type == MISC) {
						if (!json_data_to_ulong(p, &val))
							printf("rst1: %"PRIu64"\n", val);
					} else {
						printf("rst1 type is %d\n", p->type);
					}
				} else
					printf("version type is %d\n", e->type);
			}
		}
	}
	json_data_free(d);

	return 0;
}
