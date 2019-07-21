#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static int err_windows(int eval, const char *fmt, ...);
static int execvp_windows(const char *argv0, int argc, char **argv);
static void strcatf(char **buf, size_t *buf_size, const char *restrict fmt, ...);

static int err_windows(int eval, const char *fmt, ...)
{
    va_list ap;
    char *errorMessage = NULL;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                  GetLastError(), 0, (LPTSTR)&errorMessage, 0, NULL);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s", errorMessage ? errorMessage : "(error retrieving error message)\n");
    return eval;
}

static int execvp_windows(const char *argv0, int argc, char **argv)
{
    char *CommandLine = NULL;
    size_t CommandLine_size = 0;
    DWORD ExitCode;
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFO StartupInfo;

    strcatf(&CommandLine, &CommandLine_size, "\"%s\"", argv0);
    for (int narg = 1; narg < argc; narg++)
        strcatf(&CommandLine, &CommandLine_size, " \"%s\"", argv[narg]);
    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    if (!CreateProcess(argv0, CommandLine, NULL, NULL, TRUE, 0,
                       NULL, NULL, &StartupInfo, &ProcessInformation))
        return err_windows(EXIT_FAILURE, "CreateProcess");
    else {
        if (WaitForSingleObject(ProcessInformation.hThread, INFINITE) == WAIT_FAILED)
            return err_windows(EXIT_FAILURE, "WaitForSingleObject");
        else if (!GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode))
            return err_windows(EXIT_FAILURE, "GetExitCodeProcess");
        else
            return (int)ExitCode;
    }
}

static void strcatf(char **buf, size_t *buf_size, const char *restrict fmt, ...)
{
    va_list ap;
    size_t buf_len;
    char *buf_new;
    size_t buf_new_size;
    int nprinted;

    va_start(ap, fmt);
    nprinted = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (nprinted < 0) {
        fprintf(stderr, "vsnprintf: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (nprinted > 0) {
        buf_len = *buf ? strlen(*buf) : 0;
        if ((buf_new_size = buf_len + nprinted + 1) >= *buf_size) {
            if (!(buf_new = realloc(*buf, buf_new_size))) {
                fprintf(stderr, "realloc: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            } else
                *buf = buf_new, *buf_size = buf_new_size;
        }
        va_start(ap, fmt);
        nprinted = vsnprintf(*buf + buf_len, *buf_size - buf_len, fmt, ap);
        va_end(ap);
        if (nprinted < 0) {
            fprintf(stderr, "snprintf: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv)
{
    return execvp_windows(CYG_GIT_WRAPPER_FNAME_WINDOWS, argc, argv);
}

/*
 * vim:expandtab sw=4 ts=4
 */
