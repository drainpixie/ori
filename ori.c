// vim: set wrap:

#define TB_IMPL
#include "termbox2.h"

#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX 512

// TODO: make notes fold, unfold with `o`
// TODO: make notes scrollable
// TODO: figure out utf-8, we can't use cool tree unicode.
// TODO: make all strings in the program dynamic, consider using external
// library if 2 much work.

void tb_puts(int x, int y, uintattr_t fg, uintattr_t bg, const char *str) {
  for (int i = 0; str[i]; i++)
    tb_printf(x + i, y, fg, bg, "%c", str[i]);
}

void clean_str(char *str) {
  if (!str)
    return;

  size_t len = strlen(str);
  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' ||
                     str[len - 1] == ' ' || str[len - 1] == '\t')) {
    str[--len] = '\0';
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

int display_topic(char ***topics, int *counts, char **names, int n_topics) {
    int selected = 0;
    struct tb_event ev;

    while (1) {
        tb_clear();
        int row = 1;

        for (int t = 0; t < n_topics; t++) {
            uint16_t fg = (t == selected) ? TB_BLACK : TB_WHITE | TB_BOLD;
            uint16_t bg = (t == selected) ? TB_WHITE : TB_DEFAULT;
            tb_puts(2, row, fg, bg, names[t]);

            char buf[32];
            snprintf(buf, sizeof(buf), " [%d]", counts[t]);
            tb_puts(2 + strlen(names[t]), row, TB_CYAN, bg, buf);

            row++;
        }

        tb_present();

        if (tb_poll_event(&ev) == -1) continue;
        if (ev.type == TB_EVENT_KEY) {
            if (ev.key == TB_KEY_ESC || ev.key == TB_KEY_CTRL_C) break;
            if (ev.key == TB_KEY_ARROW_UP) {
                if (selected > 0) selected--;
            } else if (ev.key == TB_KEY_ARROW_DOWN) {
                if (selected < n_topics - 1) selected++;
            } else if (ev.key == TB_KEY_ENTER) {
                return selected;
            }
        }
    }

    return -1;  
}

int read_entries(const char *topic_name, int entries, const char *dir_path,
                 char ***out_entries, int *out_count) {
  if (!topic_name || !dir_path || entries <= 0)
    return -1;

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

int read_index(FILE *fp, const char *dir_path, char ***out_topic_names,
               char ****out_all_entries, int **out_counts, int *out_n_topics) {
  if (!fp || !dir_path)
    return -1;

  char line[MAX], topic_name[MAX];
  int n_topics = 0, capacity = 8;

  char **topic_names = malloc(capacity * sizeof(char *));
  char ***all_entries = malloc(capacity * sizeof(char **));
  int *counts = malloc(capacity * sizeof(int));

  while (fgets(line, sizeof(line), fp)) {
    clean_str(line);
    if (strlen(line) == 0)
      continue;

    int entries;
    if (sscanf(line, " \"%[^\"]\"\t%d", topic_name, &entries) == 2 &&
        entries > 0) {
      char **topic_entries = NULL;
      int count = 0;

      if (read_entries(topic_name, entries, dir_path, &topic_entries, &count) ==
              0 &&
          count > 0) {
        if (n_topics >= capacity) {
          capacity *= 2;
          topic_names = realloc(topic_names, capacity * sizeof(char *));
          all_entries = realloc(all_entries, capacity * sizeof(char **));
          counts = realloc(counts, capacity * sizeof(int));
        }

        topic_names[n_topics] = strdup(topic_name);
        all_entries[n_topics] = topic_entries;
        counts[n_topics] = count;

        n_topics++;
      }
    }
  }

  *out_topic_names = topic_names;
  *out_all_entries = all_entries;
  *out_counts = counts;
  *out_n_topics = n_topics;

  return 0;
}

int main(int argc, char *argv[]) {
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

  char **topic_names;
  char ***all_entries;
  int *counts;
  int n_topics;

  int ret =
      read_index(fp, dir_path, &topic_names, &all_entries, &counts, &n_topics);
  fclose(fp);

  if (ret != 0) {
    tb_shutdown();
    fprintf(stderr, "error: reading failed\n");
    return ret;
  }

  display_topic(all_entries, counts, topic_names, n_topics);

  for (int i = 0; i < n_topics; i++) {
    free(topic_names[i]);
    for (int j = 0; j < counts[i]; j++)
      free(all_entries[i][j]);
    free(all_entries[i]);
  }
  free(topic_names);
  free(all_entries);
  free(counts);

  tb_shutdown();
  return 0;
}
