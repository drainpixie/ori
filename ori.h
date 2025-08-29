#ifndef ORI_H
#define ORI_H

#define MAX 512

#define BRANCH "├"     // U+251C
#define LEAF "└"       // U+2514

typedef struct {
	char *name;
	char **entries;
	int count;
} Topic;

static int utf8_decode(const char *s, uint32_t *rune);
void tb_puts(int x, int y, uint16_t fg, uint16_t bg, const char *str);
void clean_str(char *str);
char *read_file(FILE *fp);
int display_topic(Topic **topics_ptr, int *n_topics_ptr, const char *dir_path, const char *index_file_path);
void create_topic(const char *topic_name, const char *dir_path, FILE *fp);
void get_input(const char *prompt, char *buffer, int bufsize);
int read_entries(const char *topic_name, int entries, const char *dir_path, char ***out_entries, int *out_count);
int read_index(FILE *fp, const char *dir_path, Topic **out_topics, int *out_n_topics);

#endif
