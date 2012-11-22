#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "uv.h"
#include "options.h"

#ifdef __WIN32
#include <shellapi.h>
#include <Winerror.h>
#else
extern char **environ;
#endif
static char *pidfile;

static char *outfile;
static char *errfile;

options_t opts;

uv_loop_t *loop;
uv_process_t child_req;
uv_process_options_t options;

void spawn_cb(uv_process_t*, int, int);
void configure_stdio();

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
  if (errno != 0) fprintf(stderr, "%s\n", strerror(errno));
  if (fd != -1) close(fd);
}

void cleanup_pid_file(char *pidname) {
  if (unlink(pidname) == -1) {
    fprintf(stderr, "Could not remove %s: %s\n", pidname, strerror(errno));
  }
  free(pidname);
}

void spawn_child(int detach) {
  uv_stdio_container_t stdio[3];
  int i;

  options.stdio_count = 3;

  for (i = 0; i < options.stdio_count; i++) {
    stdio[i].flags = UV_INHERIT_FD;
    stdio[i].data.fd = i;
  }

  options.file = opts.child_args[0];
  options.args = opts.child_args;
  options.stdio = stdio;
  options.env = environ;
  options.exit_cb = detach ? NULL : spawn_cb;
  if (detach) options.flags = UV_PROCESS_DETACHED;

  if (uv_spawn(loop, &child_req, options)) {
    fprintf(stderr, "Error %s\n", uv_err_name(uv_last_error(loop)));
    fprintf(stderr, "%s\n", uv_strerror(uv_last_error(loop)));
  }

  if (detach) {
    uv_unref((uv_handle_t*)&child_req);
    return;
  }

  write_pid_file(child_req.pid, pidfile);
}

void spawn_cb(uv_process_t *req, int exit_status, int signal_status) {
  if (signal_status) {
    fprintf(stderr, "Child killed by signal %d, restarting.\n", signal_status);
  }
  else {
    fprintf(stderr, "Child exited with %d, restarting.\n", exit_status);
  }
  spawn_child(0);
}

void handle_signal(int signal_status) {
  fprintf(stderr, "Aeternum kill by signal %d, exiting.\n", signal_status);
  uv_process_kill(&child_req, SIGKILL);
  cleanup_pid_file(pidfile);
}

int stdio_redirect(char *dest, int fd) {
  int out;

  if (dest == NULL) {
    dest = "/dev/null";
  }
  out = open(dest, O_WRONLY | O_APPEND | O_CREAT, 0660);
  if (out == -1) {
    close(out);
    perror("stdio_redirect");
    return -1;
  }
  else if (dup2(out, fd) == -1) {
    close(out);
    perror("stdio_redirect");
    return -1;
  }
  return out;
}

void configure_stdio() {
  outfile = opts.outfile;
  errfile = opts.errfile;
  stdio_redirect(opts.infile, STDIN_FILENO);
  stdio_redirect(opts.outfile, STDOUT_FILENO);
  if (opts.errfile != NULL) {
    stdio_redirect(opts.errfile, STDERR_FILENO);
  }
  else {
    stdio_redirect(opts.outfile, STDERR_FILENO);
  }
}

void set_pidfile_path(char *pidname) {
#ifdef _WIN32
  WCHAR homedir[MAX_PATH];
  if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path)) {
    perror("SHGetFolderPathW");
    return;
  }
#else
  const char *homedir = getenv("HOME");
  if (!homedir) {
    struct passwd *pw = getpwuid(getuid());
    homedir = pw->pw_dir;
  }
#endif
  char *basepath = malloc(strlen(homedir) + 12);
  struct stat has_dir;

  sprintf(basepath, "%s/.aeternum/", homedir);

  if (lstat(basepath, &has_dir) == -1) {
    if (errno == ENOENT) {
      if (mkdir(basepath, 0755) == -1) {
        perror("mkdir");
      }
    }
    else {
      return;
    }
  }

  pidfile = malloc(snprintf(NULL, 0, "%s%s", basepath, pidname) + 1);
  sprintf(pidfile, "%s%s", basepath, pidname);
  free(basepath); 
}

int main(int argc, char *argv[]) {
  int r;

  if (signal (SIGINT, handle_signal) == SIG_IGN)
    signal (SIGINT, SIG_IGN);
  if (signal (SIGHUP, handle_signal) == SIG_IGN)
    signal (SIGHUP, SIG_IGN);
  if (signal (SIGTERM, handle_signal) == SIG_IGN)
    signal (SIGTERM, SIG_IGN);

  loop = uv_default_loop();
  if (strcmp(argv[1], "start") == 0) {
    argv[1] = "run";

    opts.infile = NULL;
    opts.outfile = NULL;
    opts.errfile = NULL;
    opts.pidname = NULL;
    opts.target = argv[0];
    opts.json = 0;
    opts.child_args = argv;

    spawn_child(1);
    printf("%d\n", child_req.pid);
    return 0;
  }

  if (strcmp(argv[1], "run") == 0) {
    argv[1] = argv[0];
    argv = &argv[1];
    argc--;
  }
  opts = options_parse(argc, argv);

  if (opts.pidname != NULL) {
    if (strcspn(opts.pidname, "/") < strlen(opts.pidname))
      pidfile = strdup(opts.pidname);
    else set_pidfile_path(opts.pidname);
  }
  else {
    set_pidfile_path(opts.target);
  }

  configure_stdio();

  spawn_child(0);

  r = uv_run(loop);
  free(pidfile);
  return r;
}
