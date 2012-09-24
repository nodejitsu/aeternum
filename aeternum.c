#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "uv.h"
#include "options.h"

static int child_argc = 0;
static char **child_argv;
static char *pidfile;

uv_loop_t *loop;
uv_process_t child_req;
uv_process_options_t options;

void spawn_cb(uv_process_t*, int, int);

void write_pid_file(int pid, char *pidname) {
  char buf[33];
  int fd = -1, n = 0;

  fd = open(pidname, O_WRONLY | O_TRUNC | O_CREAT, 0660);
  if (fd == -1)
    goto cleanup;

  if ((n = snprintf(buf, sizeof buf, "%d", pid)) < 0)
    goto cleanup;
  if (write(fd, buf, n) < 0)
    goto cleanup;

cleanup:
  perror("write_pid_file");
  if (fd != -1) close(fd);
}

void spawn_child(int argc, char *args[]) {
  uv_stdio_container_t stdio[3];
  int i;

  options.stdio_count = 3;

  // TODO: Make sure this isn't stupid.
  for (i = 0; i < options.stdio_count; i++) {
    stdio[i].flags = UV_INHERIT_FD;
    stdio[i].data.fd = i;
  }

  options.file = strdup(args[0]);
  options.args = args;
  options.stdio = stdio;
  options.exit_cb = spawn_cb;

  if (uv_spawn(loop, &child_req, options)) {
    fprintf(stderr, "Error %s\n", uv_err_name(uv_last_error(loop)));
    fprintf(stderr, "%s\n", uv_strerror(uv_last_error(loop)));
  }

  write_pid_file(child_req.pid, pidfile);
}

void spawn_cb(uv_process_t *req, int exit_status, int signal_status) {
  char *signame;
  // This is shitty.  Make smarter.
  if (signal_status) {
#ifdef __sun
    signame = sig2str(signal_status);
#else
    signame = strdup(sys_signame[signal_status]);
#endif
    if (strcmp("int", signame) == 0 ||
        strcmp("term", signame) == 0 ||
        strcmp("hup", signame) == 0) {
      fprintf(stderr, "Got sig%s, exiting.\n", signame);
      uv_close((uv_handle_t*)req, NULL);
      free(child_argv);
      free(signame);
      // TODO: Investigate why this is necessary.
      exit(0);
    }
    else {
      fprintf(stderr, "Killed by sig%s, restarting.\n", signame);
    }
  }
  else {
    fprintf(stderr, "Exit %d, signal %d.\n", exit_status, signal_status);
  }
  spawn_child(child_argc, child_argv);
}

// TODO: Replace this with libuv fs ops for portability
void stdio_redirect(char *dest, int fd) {
  int out;

  if (dest == NULL) {
    dest = "/dev/null";
  }
  out = open(dest, O_WRONLY | O_APPEND | O_CREAT, 0660);
  if (out == -1) goto cleanup;
  else if (dup2(out, fd) == -1) goto cleanup;

cleanup:
  if (out != -1) close(out);
  perror("stdio_redirect");
}

void set_pidfile_path(char *pidname) {
  char *homedir = getenv("HOME");
  char *basepath = malloc(strlen(homedir) + 12);
  struct stat has_dir;

  sprintf(basepath, "%s/.aeternum/", homedir);

  if (lstat(basepath, &has_dir) == -1) {
    if (errno == ENOENT) {
      if (mkdir(basepath, 0755) == -1) {
        perror("mkdir");
        exit(1);
      }
    }
  }

  pidfile = malloc(snprintf(NULL, 0, "%s%s", basepath, pidname) + 1);
  sprintf(pidfile, "%s%s", basepath, pidname); 
}

int main(int argc, char *argv[]) {
  int i = 0, r;

  loop = uv_default_loop();
  options_t opts = options_parse(argc, argv);

  if (opts.pidname != NULL) {

    if (strcspn(opts.pidname, "/") < strlen(opts.pidname))
      pidfile = strdup(opts.pidname);
    else set_pidfile_path(opts.pidname);
  }
  else
    set_pidfile_path(opts.target);

  child_argv = malloc(sizeof(char*) * argc);

  do {
    child_argv[i] = strdup(opts.child_args[i]);
    i++;
  } while (opts.child_args[i] != NULL);

  child_argv[i] = NULL;

  stdio_redirect(opts.infile, fileno(stdin));
  stdio_redirect(opts.outfile, fileno(stdout));
  if (opts.errfile != NULL) {
    stdio_redirect(opts.errfile, fileno(stderr));
  }
  else {
    stdio_redirect(opts.outfile, fileno(stderr));
  }

  spawn_child(child_argc, child_argv);

  r = uv_run(loop);
  free(child_argv);
  return r;
}
