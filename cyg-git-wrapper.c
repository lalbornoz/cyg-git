#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cygwin.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define GIT_STDOUT_BUFFER_WINDOW    1024

static bool check_path_conversion_out(int argc, char **argv);
static void construct_argv_new(const char *argv0, int argc, char **argv, char ***pargv_new);
static int err(int eval, const char *fmt, ...);
static void paths_convert_in(int argc, char **argv);

static bool check_path_conversion_out(int argc, char **argv)
{
    if ((argc == 3)
    &&  (strcmp(argv[1], "rev-parse") == 0)
    &&  (strcmp(argv[2], "--show-toplevel") == 0))
        return true;
    else
        return false;
}

static void construct_argv_new(const char *argv0, int argc, char **argv, char ***pargv_new)
{
    size_t argc_new = 0;
    char *arg, **argv_new = NULL;

    if (!(argv_new = malloc(2 * sizeof(*argv_new))))
        err(EXIT_FAILURE, "malloc");
    else {
        argv_new[0] = (char *)argv0, argv_new[1] = NULL, argc_new = 2;
        for (int n = 1; n < argc; n++) {
            if (!(argv_new = realloc(argv_new, (argc_new + 1) * sizeof(*argv_new))))
                err(EXIT_FAILURE, "realloc");
            else if (!(arg = strdup(argv[n])))
                err(EXIT_FAILURE, "strdup");
            else
                argv_new[argc_new - 1] = arg, argv_new[argc_new] = NULL, argc_new++;
        }
        *pargv_new = argv_new;
    }
}

static int err(int eval, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s", strerror(errno));
    return eval;
}

static void paths_convert_in(int argc, char **argv)
{
    bool foundfl = false;
    ssize_t nconv;
    char *path_buf = NULL;

    if ((argc > 2) && (strcmp(argv[1], "add") == 0)) {
        for (int narg = 2; narg < argc; narg++) {
            if (foundfl) {
                if ((nconv = cygwin_conv_path(CCP_ABSOLUTE | CCP_WIN_A_TO_POSIX, argv[narg], NULL, 0)) < 0)
                    err(EXIT_FAILURE, "cygwin_conv_path");
                else if (!(path_buf = malloc(nconv)))
                    err(EXIT_FAILURE, "malloc");
                else if ((nconv = cygwin_conv_path(CCP_ABSOLUTE | CCP_WIN_A_TO_POSIX, argv[narg], path_buf, nconv)) < 0)
                    err(EXIT_FAILURE, "cygwin_conv_path");
                else
                    argv[narg] = path_buf;
            } else if (strcmp(argv[narg], "--") == 0) {
                foundfl = true;
            }
        }
    }
}

int main(int argc, char **argv)
{
    char **argv_new = NULL;
    bool convertfl;
    ssize_t nconv, nread;
    pid_t pid;
    int pid_status = EXIT_SUCCESS;
    char *pipe_buf = NULL;
    size_t pipe_buf_len = 0, pipe_buf_size = 0;
    char *pipe_buf_win = NULL;
    int pipe_fds[2];

    paths_convert_in(argc, argv);
    convertfl = check_path_conversion_out(argc, argv);
    construct_argv_new(GIT_FNAME_POSIX, argc, argv, &argv_new);
    if (!(pipe_buf = malloc(pipe_buf_size = GIT_STDOUT_BUFFER_WINDOW)))
        return err(EXIT_FAILURE, "malloc");
    else if (pipe(&pipe_fds[0]) < 0)
        return err(EXIT_FAILURE, "pipe");
    else switch (pid = fork()) {
    case 0:
        close(fileno(stdout)); close(pipe_fds[0]); dup2(pipe_fds[1], fileno(stdout));
        return execvp(argv_new[0], (char * const*)argv_new), EXIT_FAILURE;
    case -1:
        return err(EXIT_FAILURE, "fork"); break;
    default:
        close(pipe_fds[1]);
        while ((nread = read(pipe_fds[0], pipe_buf + pipe_buf_len, pipe_buf_size - pipe_buf_len - 1)) > 0) {
            pipe_buf_len += nread;
            if ((pipe_buf_size - pipe_buf_len - 1) == 0) {
                if (!(pipe_buf = realloc(pipe_buf, pipe_buf_size + GIT_STDOUT_BUFFER_WINDOW)))
                    return err(EXIT_FAILURE, "realloc");
                else
                    pipe_buf_size += GIT_STDOUT_BUFFER_WINDOW;
            }
        }
        if (nread < 0)
            return err(EXIT_FAILURE, "read");
        else {
            if (convertfl) {
                pipe_buf[pipe_buf_len] = '\0';
                if ((nconv = cygwin_conv_path(CCP_ABSOLUTE | CCP_POSIX_TO_WIN_A, pipe_buf, NULL, 0)) < 0)
                    return err(EXIT_FAILURE, "cygwin_conv_path");
                else if (!(pipe_buf_win = malloc(nconv)))
                    return err(EXIT_FAILURE, "malloc");
                else if ((nconv = cygwin_conv_path(CCP_ABSOLUTE | CCP_POSIX_TO_WIN_A, pipe_buf, pipe_buf_win, nconv)) < 0)
                    return err(EXIT_FAILURE, "cygwin_conv_path");
                else
                    printf("%s", pipe_buf_win);
            } else
                write(fileno(stdout), pipe_buf, pipe_buf_len);
            if (waitpid(pid, &pid_status, 0) < 0)
                return err(EXIT_FAILURE, "waitpid");
            else
                return WEXITSTATUS(pid_status);
        }
    }
}

/*
 * vim:expandtab sw=4 ts=4
 */
