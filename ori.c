#define TB_IMPL

#include "termbox2.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>

#define MAX 512

#define SUCCESS 0
#define ERROR_ARGS -1
#define ERROR_FILE -2
#define ERROR_MKDIR -3

void clean_str(char *str)
{
	size_t len;
	if (!str) return;

	len = strlen(str);
	while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r' || str[len-1] == ' ' || str[len-1] == '\t')) {
		str[--len] = '\0';
	}
}

char *read_file(FILE *fp)
{
	size_t capacity = 1024;
	size_t size = 0;
	char *content;
	char *new_content;
	int c;

	if (!fp) return NULL;
	content = malloc(capacity);
	if (!content) return NULL;

	while ((c = fgetc(fp)) != EOF) {
		if (size >= capacity - 1) {
			capacity *= 2;
			new_content = realloc(content, capacity);

			if (!new_content) {
				free(content);
				return NULL;
			}

			content = new_content;
		}

		content[size++] = (char)c;
	}

	content[size] = '\0';
	return content;
}

int create_topic(const char *name, const char *dir_path) {
	char topic_dir[PATH_MAX];
	int ret;

	if (!name || !dir_path) {
		fprintf(stderr, "error: invalid parameters\n");
		return ERROR_ARGS;
	}

	ret = snprintf(topic_dir, sizeof(topic_dir), "%s/%s", dir_path, name);

	if (ret >= PATH_MAX) {
		fprintf(stderr, "error: path too long for topic '%s'\n", name);
		return ERROR_ARGS;
	}

	if (mkdir(topic_dir, 0755) != 0) {
		if (errno == EEXIST) {
			printf("info: directory '%s' already exists\n", topic_dir);
			return SUCCESS;
		} else {
			perror("error: directory creation failed");
			return ERROR_MKDIR;
		}
	}

	printf("info: created directory: %s\n", topic_dir);
	return SUCCESS;
}

int read_entries(const char *topic_name, int num_entries, const char *dir_path)
{
	char entry_path[PATH_MAX];
	int ret;
	FILE *entry_fp;
	char *content;
	int i;

	if (!topic_name || !dir_path || num_entries < 0) {
		fprintf(stderr, "error: invalid parameters\n");
		return ERROR_ARGS;
	}

	printf("topic: %s (%d entries)\n", topic_name, num_entries);

	for (i = 1; i <= num_entries; i++) {
		ret = snprintf(entry_path, sizeof(entry_path), "%s/%s/%d.txt", 
		    dir_path, topic_name, i);

		if (ret >= PATH_MAX) {
			fprintf(stderr, "error: path too long for entry %d of topic '%s'\n", 
			    i, topic_name);
			continue;
		}

		entry_fp = fopen(entry_path, "r");
		if (!entry_fp) {
			fprintf(stderr, "warn: could not open entry %d (%s): %s\n", i, entry_path, strerror(errno));
			continue;
		}

		printf("  entry %d (%s):\n", i, entry_path);


		content = read_file(entry_fp);
		if (content) {
			printf("    %s", content);
			free(content);
		} else {
			fprintf(stderr, "warn: failed to read content from %s\n", entry_path);
		}

		fclose(entry_fp);
		printf("\n");


		return SUCCESS;
	}
}

int read_index(FILE *fp, const char *dir_path)
{
	char line[MAX];
	char topic_name[MAX];

	int entries;
	int processed = 0;

	if (!fp || !dir_path) {
		fprintf(stderr, "error: invalid parameters to process_index_file\n");
		return ERROR_ARGS;
	}

	while (fgets(line, sizeof(line), fp)) {
		clean_str(line);
		if (strlen(line) == 0) continue;

		if (sscanf(line, " \"%[^\"]\"\t%d", topic_name, &entries) == 2) {
			printf("reading topic: %s\n", topic_name);

			if (entries < 0) {
				fprintf(stderr, "warn: invalid entry count %d for topic '%s', skipping\n", 
				    entries, topic_name);
				continue;
			}

			if (entries > 0)
				read_entries(topic_name, entries, dir_path);

			create_topic(topic_name, dir_path);

			processed++;
		} else {
			fprintf(stderr, "warn: could not parse line: '%s'\n", line);
		}
	}

	printf("read %d topics\n", processed);
	return SUCCESS;
}

int main(int argc, char *argv[])
{
	char file_path[PATH_MAX];
	char *dir_path;
	FILE *fp;
	int ret;

	if (argc < 2) {
		printf("usage: %s <index_file>\n", argv[0]);
		printf("  index_file: path to the index file containing topic information\n");
		return ERROR_ARGS;
	}

	fp = fopen(argv[1], "a+");
	if (!fp) {
		fprintf(stderr, "error: cannot open file '%s': %s\n", argv[1], strerror(errno));
		return ERROR_FILE;
	}

	strncpy(file_path, argv[1], sizeof(file_path) - 1);
	file_path[sizeof(file_path) - 1] = '\0';

	dir_path = dirname(file_path);
	if (!dir_path) {
		fprintf(stderr, "error: could not determine directory path\n");
		fclose(fp);

		return ERROR_ARGS;
	}

	printf("using directory: %s\n", dir_path);

	rewind(fp);
	ret = read_index(fp, dir_path);
	fclose(fp);

	if (ret != SUCCESS) {
		fprintf(stderr, "error: reading failed with code %d\n", ret);
		return ret;
	}

	return SUCCESS;
}
