#ifndef _help_h
#define _help_h

#define VERSION "0.1.4"

typedef struct help_s {
  char *name;
  char *syntax;
  char *desc;
  char **text;
  int length;
} help_t;

typedef struct help_list_s {
  int length;
  help_t *commands;
} help_list_t;

void add_help(char *name, char *syntax, char *desc, char **text);
void print_help_for(char *name);
void print_help();
void print_help_header();
void print_header();

#endif
