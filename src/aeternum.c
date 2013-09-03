#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#include <uv.h>
#include <saneopt.h>

#include "../options.h"

extern char **environ;

options_t opts;

uv_loop_t *loop;
uv_process_t child_req;
uv_process_options_t options;

static struct timeval start_tv;

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

  if (detach) {
    for (i = 0; i < options.stdio_count; i++) {
      stdio[i].flags = UV_IGNORE;
    }
  }
  else {
    for (i = 0; i < options.stdio_count; i++) {
      stdio[i].flags = UV_INHERIT_FD;
      stdio[i].data.fd = i;
    }
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

  if (opts.pidname != NULL) {
    write_pid_file(child_req.pid, opts.pidname);
  }
}

void spawn_cb(uv_process_t *req, int exit_status, int signal_status) {
  struct timeval tv;
  int ms_up;

  if (opts.min_uptime != 0) {
    gettimeofday(&tv, NULL);
    ms_up = ((tv.tv_sec * 1000) + (tv.tv_usec / 1000)) -
            ((start_tv.tv_sec * 1000) + (start_tv.tv_usec / 1000));

    if (ms_up < opts.min_uptime) {
      fprintf(stderr, "Child exited prematurely with %d, exiting.\n", exit_status);
      exit(2);
    }
  }

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

  uv_process_kill(&child_req, SIGTERM);
  sleep(5); // Just `sleep` here, not doing anything important anyway.
  uv_process_kill(&child_req, SIGKILL);

  if (opts.pidname) {
    cleanup_pid_file(opts.pidname);
  }
  exit(0);
}

void restart_child(int signal_status) {
  fprintf(stderr, "Aeternum received SIGUSR1, killing child.\n");
  uv_process_kill(&child_req, SIGTERM);
  sleep(5); // Just `sleep` here, not doing anything important anyway.
  uv_process_kill(&child_req, SIGKILL);
  // Re-assign signal handler
  signal(SIGUSR1, restart_child);
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
  if (opts.infile != NULL) {
    stdio_redirect(opts.infile, STDIN_FILENO);
  }

  if (opts.outfile != NULL) {
    stdio_redirect(opts.outfile, STDOUT_FILENO);

    if (opts.errfile != NULL) {
      stdio_redirect(opts.errfile, STDERR_FILENO);
    }
    else {
      stdio_redirect(opts.outfile, STDERR_FILENO);
    }
  }
}

int main(int argc, char *argv[]) {
  int r;
  saneopt_t* opt;
  char** args;
  char* min_uptime;

  signal(SIGHUP, SIG_IGN);
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGUSR1, restart_child);

  loop = uv_default_loop();

  if (argc == 1) {
    printf("Usage: aeternum [action] [options] -- program\n");
    return 1;
  }

  opt = saneopt_init(argc - 1, argv + 1);
  args = saneopt_arguments(opt);

  if (strcmp(args[0], "start") == 0) {
    argv[1] = "run";

    opts.infile = NULL;
    opts.outfile = NULL;
    opts.errfile = NULL;
    opts.pidname = NULL;
    opts.min_uptime = 0;
    opts.target = argv[0];
    opts.json = 0;
    opts.child_args = argv;

    spawn_child(1);
    printf("%d\n", child_req.pid);
    return 0;
  }

  opts.infile = saneopt_get(opt, "i");
  opts.outfile = saneopt_get(opt, "o");
  opts.errfile = saneopt_get(opt, "e");
  opts.pidname = saneopt_get(opt, "p");
  opts.json = (saneopt_get(opt, "j") != NULL) ? 1 : 0;
  opts.target = args[1];
  opts.child_args = &args[1];

  min_uptime = saneopt_get(opt, "min-uptime");
  if (min_uptime != NULL) {
    sscanf(min_uptime, "%d", &opts.min_uptime);
    gettimeofday(&start_tv, NULL);

  }
  else {
    opts.min_uptime = 0;
  }

  configure_stdio();

  spawn_child(0);

  r = uv_run(loop, UV_RUN_DEFAULT);

  return r;
}
