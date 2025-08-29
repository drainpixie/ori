// vim: set wrap:

#define TB_IMPL
#include "termbox2.h"

#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ori.h"

// TODO: make notes scrollable
// TODO: make all strings in the program dynamic, consider using external
// library if 2 much work.
// TODO: maybe comment stuff a bit or clean up somehow, im getting bit lost in certain parts
// TODO: move more stuff to header(?)
// TODO: add creation of entries, add deletion of topics and entries

static int utf8_decode(const char *s, uint32_t *rune) {
  unsigned char c = s[0];

  if (c < 0x80) {
    *rune = c;
    return 1;
  } else if ((c & 0xE0) == 0xC0) {
    *rune = ((c & 0x1F) << 6) | (s[1] & 0x3F);
    return 2;
  } else if ((c & 0xF0) == 0xE0) {
    *rune = ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    return 3;
  } else if ((c & 0xF8) == 0xF0) {
    *rune = ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) |
            (s[3] & 0x3F);
    return 4;
  }

  return 1;
}

void tb_puts(int x, int y, uint16_t fg, uint16_t bg, const char *str) {
  int cx = x;

  for (int i = 0; str[i];) {
    uint32_t rune;
    int bytes = utf8_decode(str + i, &rune);
    tb_set_cell(cx, y, rune, fg, bg);
    cx++;
    i += bytes;
  }
}

void clean_str(char *str) {
    if (!str) return;

    char *start = str;
    while (*start == ' ' || *start == '\t') start++;

    if (start != str)
        memmove(str, start, strlen(start) + 1);

    char *end = str + strlen(str) - 1;
    while (end >= str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end-- = '\0';
    }
}

char *read_file(FILE *fp) {
  if (!fp)
    return NULL;

  size_t capacity = 1024, size = 0;
  char *content = malloc(capacity);
  if (!content)
    return NULL;

  int c;
  while ((c = fgetc(fp)) != EOF) {
    if (size >= capacity - 1) {
      capacity *= 2;
      char *tmp = realloc(content, capacity);
      if (!tmp) {
        free(content);
        return NULL;
      }
      content = tmp;
    }
    content[size++] = (char)c;
  }
  content[size] = '\0';
  return content;
}

void get_input(const char *prompt, char *buffer, int bufsize) {
    int pos = 0;
    struct tb_event ev;

    tb_clear();
    tb_puts(0, tb_height() - 1, TB_WHITE, TB_DEFAULT, prompt);
    tb_present();

    while (1) {
        if (tb_poll_event(&ev) == -1) continue;

        if (ev.type == TB_EVENT_KEY) {
            if (ev.key == TB_KEY_ENTER) {
                buffer[pos] = '\0';
                break;
            } else if (ev.key == TB_KEY_BACKSPACE || ev.key == TB_KEY_BACKSPACE2) {
                if (pos > 0) pos--;
            } else if (ev.ch && pos < bufsize - 1) {
                buffer[pos++] = (char)ev.ch;
            }
        }

        tb_clear();
        tb_puts(0, tb_height() - 1, TB_WHITE, TB_DEFAULT, prompt);
        tb_puts(strlen(prompt), tb_height() - 1, TB_WHITE, TB_DEFAULT, buffer);
        tb_present();
    }
}

void create_topic(const char *topic_name, const char *dir_path, FILE *fp) {
    fprintf(fp, "\"%s\"\t0\n", topic_name);
    fflush(fp);

    char tdir[MAX];
    snprintf(tdir, sizeof(tdir), "%s/%s", dir_path, topic_name);

    if (mkdir(tdir, 0755) != 0)
        if (errno != EEXIST)
            fprintf(stderr, "Failed to create dir %s\n", tdir);
}

int display_topic(Topic **topics_ptr, int *n_topics_ptr, const char *dir_path, const char *index_file_path) {
    int selected = 0;
    int n_topics = *n_topics_ptr;
    Topic *topics = *topics_ptr;

    int *folded = calloc(n_topics, sizeof(int));
    if (!folded) return -1;
    memset(folded, 1, n_topics * sizeof(int));

    struct tb_event ev;

    FILE *index_fp = fopen(index_file_path, "a+");
    if (!index_fp) {
        tb_shutdown();
        fprintf(stderr, "Cannot open index file\n");
        free(folded);
        return -1;
    }

    while (1) {
        tb_clear();
        int row = 1, indent = 2;

        for (int t = 0; t < n_topics; t++) {
            uint16_t fg = (t == selected) ? TB_BLACK : TB_WHITE;
            uint16_t bg = (t == selected) ? TB_WHITE : TB_DEFAULT;

            tb_puts(indent, row, fg | TB_BOLD, bg, topics[t].name);

            char buf[MAX];
            snprintf(buf, sizeof(buf), " %d", topics[t].count);
            tb_puts(indent + strlen(topics[t].name), row, fg, bg, buf);

            row++;

            if (!folded[t]) {
                for (int i = 0; i < topics[t].count; i++) {
                    const char *prefix = (i == topics[t].count - 1) ? LEAF : BRANCH;
                    clean_str(topics[t].entries[i]);
                    tb_puts(indent, row, TB_WHITE | TB_BOLD, TB_DEFAULT, prefix);
                    tb_puts(indent + 2, row++, TB_WHITE, TB_DEFAULT, topics[t].entries[i]);
                }
            }
        }

        tb_present();

        if (tb_poll_event(&ev) == -1) continue;

        if (ev.type == TB_EVENT_KEY) {
            if (ev.key == TB_KEY_ESC || ev.ch == 'q' || ev.key == TB_KEY_CTRL_C)
                break;
            else if (ev.key == TB_KEY_ARROW_UP || ev.ch == 'k') {
                if (selected > 0) selected--;
            } else if (ev.key == TB_KEY_ARROW_DOWN || ev.ch == 'j') {
                if (selected < n_topics - 1) selected++;
            } else if (ev.key == TB_KEY_ENTER || ev.ch == ' ' || ev.ch == 'o') {
                folded[selected] = !folded[selected];
            } else if (ev.ch == 'c') {
                // move this to a function                
                char new_topic[MAX] = {0};
                get_input("New topic: ", new_topic, sizeof(new_topic));

                if (strlen(new_topic) > 0) {
                    create_topic(new_topic, dir_path, index_fp);

                    for (int i = 0; i < n_topics; i++) {
                        free(topics[i].name);
                        
                        for (int j = 0; j < topics[i].count; j++)
                            free(topics[i].entries[j]);
                        
                        free(topics[i].entries);
                    }
                    free(topics);
                    free(folded);

                    rewind(index_fp);
                    read_index(index_fp, dir_path, &topics, &n_topics);
                    folded = calloc(n_topics, sizeof(int));
                    memset(folded, 1, n_topics * sizeof(int));
                    selected = 0;
                }
            }
        }
    }

    fclose(index_fp);
    free(folded);
    *topics_ptr = topics;
    *n_topics_ptr = n_topics;

    return 0;
}

int read_entries(const char *topic_name, int entries, const char *dir_path,
                 char ***out_entries, int *out_count) {
  if (!topic_name || !dir_path || entries < 0)
    return -1;
  
  if (entries == 0) {
    *out_entries = NULL;
    *out_count = 0;
    return 0;
  }

  char entry_path[PATH_MAX];
  char **all_entries = malloc(entries * sizeof(char *));
  if (!all_entries)
    return -1;

  int count = 0;

  for (int i = 1; i <= entries; i++) {
    int ret = snprintf(entry_path, sizeof(entry_path), "%s/%s/%d.txt", dir_path,
                       topic_name, i);
    if (ret >= PATH_MAX)
      continue;

    FILE *entry_fp = fopen(entry_path, "r");
    if (!entry_fp)
      continue;

    char *content = read_file(entry_fp);
    fclose(entry_fp);

    if (content)
      all_entries[count++] = content;
  }

  *out_entries = all_entries;
  *out_count = count;

  return 0;
}

int read_index(FILE *fp, const char *dir_path, Topic **out_topics, int *out_n_topics) {
  if (!fp || !dir_path)
    return -1;

  char line[MAX], topic_name[MAX];
  int n_topics = 0, capacity = 8;

  Topic *topics = malloc(capacity * sizeof(Topic));

  if (!topics)
	  return -1;

  while (fgets(line, sizeof(line), fp)) {
    clean_str(line);
    if (strlen(line) == 0)
        continue;

    int entries;
    if (sscanf(line, " \"%[^\"]\"\t%d", topic_name, &entries) == 2) {
        char **topic_entries = NULL;
        int count = 0;

        read_entries(topic_name, entries, dir_path, &topic_entries, &count);

        if (n_topics >= capacity) {
            capacity *= 2;
            topics = realloc(topics, capacity * sizeof(Topic));
        }

        topics[n_topics].name = strdup(topic_name);
        topics[n_topics].entries = topic_entries;
        topics[n_topics].count = count;
        n_topics++;
    }
}

 
  *out_topics = topics;
  *out_n_topics = n_topics;

  return 0;
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "C.UTF-8");

  if (argc < 2) {
    fprintf(stderr, "usage: %s <index_file>\n", argv[0]);
    return -1;
  }

  FILE *fp = fopen(argv[1], "r");
  if (!fp) {
    fprintf(stderr, "error: cannot open file '%s': %s\n", argv[1],
            strerror(errno));
    return -1;
  }

  char file_path[PATH_MAX];
  strncpy(file_path, argv[1], sizeof(file_path) - 1);
  file_path[sizeof(file_path) - 1] = '\0';

  char *dir_path = dirname(file_path);
  if (!dir_path) {
    fprintf(stderr, "error: could not determine directory path\n");
    fclose(fp);
    return -1;
  }

  if (tb_init() != 0)
    return -1;

  rewind(fp);

  Topic *topics;
  int n_topics;

  int ret = read_index(fp, dir_path, &topics, &n_topics);
  fclose(fp);

  if (ret != 0) {
    tb_shutdown();
    fprintf(stderr, "error: reading failed\n");
    return ret;
  }

  display_topic(&topics, &n_topics, dir_path, argv[1]);

  for (int i = 0; i < n_topics; i++) {
    free(topics[i].name);
    for (int j = 0; j < topics[i].count; j++)
        free(topics[i].entries[j]);
    free(topics[i].entries);
  }
  free(topics);

  tb_shutdown();
}

