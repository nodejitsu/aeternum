#include <stdlib.h>
#include <stdio.h>
#include "help.h"

char *BANNER[] = {
  // aeternum ascii art
  "              __                                ",
  "  ____ ____  / /____  _________  __  ______ ___ ",
  " / __ `/ _ \\/ __/ _ \\/ ___/ __ \\/ / / / __ `__ \\",
  "/ /_/ /  __/ /_/  __/ /  / / / / /_/ / / / / / /",
  "\\__,_/\\___/\\__/\\___/_/  /_/ /_/\\__,_/_/ /_/ /_/"
};

help_list_t help_list;

void init_help() {
  if (help_list.length > 0) return;

  help_list.commands = (help_t*) malloc(sizeof(help_t*));
}

void add_help(char *name, char *syntax, char *desc, char **text) {
  init_help();

  help_t help;
  help.name = name;
  help.syntax = syntax;
  help.desc = desc;
  help.text = text;

  // Get help text length
  int i = 0;
  while (text[i] != NULL) i++;

  help.length = i;

  help_list.commands[help_list.length++] = help;
}

void print_help() {
  int i;
  print_help_header();
  for (i = 0; i < help_list.length; i++) {
    printf("aeternum %s %s\t\t\t%s\n", help_list.commands[i].name, help_list.commands[i].syntax, help_list.commands[i].desc);
  }
}

void print_help_for(char *name) {
  int i, j;
  for (i = 0; i < help_list.length; i++) {
    if (help_list.commands[i].name == name) {
      print_help_header();
      printf("%s\n", help_list.commands[i].syntax);
      printf("----------------------------------\n");
      for (j = 0; j < help_list.commands[i].length; j++) {
        printf("%s\n", help_list.commands[i].text[j]);
      }
    }
  }
}

void print_help_header() {
  print_header();
  printf("Usage: aeternum [action] [options] -- program\n\n");
}

void print_header() {
  int i = 0, length = sizeof(BANNER) / sizeof(char**);
  for (i = 0; i < length; i++) {
    printf("%s\n", BANNER[i]);
  }
  printf("\n");
}
