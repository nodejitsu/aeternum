#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "options.h"

options_t options_parse(int argc, char *argv[]) {
  assert(argc > 1);

  options_t opts;

  opts.infile = NULL;
  opts.outfile = NULL;
  opts.errfile = NULL;
  opts.pidname = NULL;
  opts.target = NULL;
  opts.json = 0;
  opts.child_args = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    switch((int)argv[i][0]) {
      case '-':
        switch((int)argv[i][1]) {
          case 'i':
            if (argv[i + 1][0] != '-') {
              opts.infile = &argv[i + 1][0];
            }
            break;
          case 'o':
            if (argv[i + 1][0] != '-') {
              opts.outfile = &argv[i + 1][0];
            }
            break;
          case 'e':
            if (argv[i + 1][0] != '-') {
              opts.errfile = &argv[i + 1][0];
            }
            break;
          case 'j':
            opts.json = 1;
            break;
          case 'p':
            if (argv[i + 1][0] != '-') {
              opts.pidname = &argv[i + 1][0];
            }
            break;
          case '-':
            if (argv[i + 1] != NULL) {
              opts.target = &argv[i + 1][0];
              opts.child_args = &argv[i + 1];
              return opts;
            }
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
  return opts;
}

