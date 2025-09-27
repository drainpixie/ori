#ifndef ORI_H
#define ORI_H

#define TB_IMPL
#define MAX 512

#define BRANCH "├" // U+251C
#define LEAF "└"   // U+2514

#include "termbox2.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
  char *name;
  char **entries;
  int count;
} Topic;

typedef enum {
    INDEX_INCREMENT, 
    INDEX_DECREMENT, 
    INDEX_DELETE_LINE 
} index_action_t;

static int utf8_decode(const char *s, uint32_t *rune);
static void tb_puts(int x, int y, uint16_t fg, uint16_t bg, const char *str);
static void clean_str(char *str);
static char *read_file(FILE *fp);

static int index_manage(char *topic_name, const char *index_path, 
                       index_action_t action);

static void create_topic(char *topic_name, const char *dir_path,
                         FILE *fp);
static void delete_topic(char *topic_name, const char *dir_path, 
                          const char *index_path);

static void create_entry(char *topic_name, const char *dir_path,
                        const char *index_path);

static void get_input(const char *prompt, char *buffer, int bufsize);
static int display_topic(Topic **topics_ptr, int *n_topics_ptr,
                         const char *dir_path, const char *index_file_path);

static int read_entries(const char *topic_name, int entries,
                        const char *dir_path, char ***out_entries,
                        int *out_count);
static int read_index(FILE *fp, const char *dir_path, Topic **out_topics,
                      int *out_n_topics);

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

static void tb_puts(int x, int y, uint16_t fg, uint16_t bg, const char *str) {
  int cx = x;

  for (int i = 0; str[i];) {
    uint32_t rune;
    int bytes = utf8_decode(str + i, &rune);
    tb_set_cell(cx, y, rune, fg, bg);
    cx++;
    i += bytes;
  }
}

static void clean_str(char *str) {
  if (!str)
    return;

  char *start = str;
  while (*start == ' ' || *start == '\t')
    start++;

  if (start != str)
    memmove(str, start, strlen(start) + 1);

  char *end = str + strlen(str) - 1;
  while (end >= str &&
         (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
    *end-- = '\0';
  }
}

static char *read_file(FILE *fp) {
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

static void get_input(const char *prompt, char *buffer, int bufsize) {
  int pos = 0;
  struct tb_event ev;

  tb_clear();
  tb_puts(0, tb_height() - 1, TB_WHITE, TB_DEFAULT, prompt);
  tb_present();

  while (1) {
    if (tb_poll_event(&ev) == -1)
      continue;

    if (ev.type == TB_EVENT_KEY) {
      if (ev.key == TB_KEY_ENTER) {
        buffer[pos] = '\0';
        break;
      } else if (ev.key == TB_KEY_BACKSPACE || ev.key == TB_KEY_BACKSPACE2) {
        if (pos > 0) {
          pos--;
          buffer[pos] = '\0';
        }
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

static int unlink_cb(const char *fpath, const struct stat *sb,
                     int typeflag, struct FTW *ftwbuf) {
    (void)sb;      
    (void)typeflag; 
    (void)ftwbuf;   
    return remove(fpath);
}

#endif
