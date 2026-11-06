/* SPDX-License-Identifier: BSD-2-Clause
 * Original BSD3 interface to an explicitly launched Dinit user instance.
 */
#define _POSIX_C_SOURCE 200809L
#include <bsd3/user-session.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef BSD3_DINITCTL
#define BSD3_DINITCTL "/usr/local/sbin/dinitctl"
#endif
extern char **environ;

int
bsd3_session_ready(void)
{
    const char *text = getenv("BSD3_READY_FD");
    char *end;
    long number;
    struct stat st;
    sigset_t blocked, old, pending;
    int error, ignored, had_pending;
    ssize_t result;

    if (getuid() == 0 || getuid() != geteuid())
        return -EPERM;
    if (!text || !*text)
        return -ENOTCONN;
    errno = 0;
    number = strtol(text, &end, 10);
    if (errno || *end || number < 3 || number > INT_MAX)
        return -EINVAL;
    if (fstat((int)number, &st) < 0)
        return -errno;
    if (!S_ISFIFO(st.st_mode))
        return -EINVAL;
    /* Do not change the process-wide SIGPIPE disposition in a library. */
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGPIPE);
    error = pthread_sigmask(SIG_BLOCK, &blocked, &old);
    if (error)
        return -error;
    sigpending(&pending);
    had_pending = sigismember(&pending, SIGPIPE);
    do {
        result = write((int)number, "\n", 1);
    } while (result < 0 && errno == EINTR);
    error = result == 1 ? 0 : errno;
    if (error == EPIPE && !had_pending) {
        sigpending(&pending);
        if (sigismember(&pending, SIGPIPE))
            (void)sigwait(&blocked, &ignored);
    }
    (void)pthread_sigmask(SIG_SETMASK, &old, NULL);
    if (!error) {
        close((int)number);
        unsetenv("BSD3_READY_FD");
    }
    return -error;
}

static int
valid_name(const char *name)
{
    const unsigned char *p = (const unsigned char *)name;
    if (!p || !(*p == '_' || (*p >= 'A' && *p <= 'Z') ||
        (*p >= 'a' && *p <= 'z')))
        return 0;
    for (++p; *p; ++p)
        if (!(*p == '_' || (*p >= 'A' && *p <= 'Z') ||
            (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')))
            return 0;
    /* The control channel cannot be redirected by an environment update. */
    return strcmp(name, "BSD3_SESSION_SOCKET") != 0 &&
        strcmp(name, "BSD3_READY_FD") != 0 &&
        strncmp(name, "DINIT_", 6) != 0;
}

int
bsd3_session_setenv(const char *name, const char *value)
{
    const char *socket = getenv("BSD3_SESSION_SOCKET");
    struct stat st;
    char *directory, *slash, *entry;
    char *argv[8];
    posix_spawn_file_actions_t actions;
    struct timespec delay = {0, 25000000}, start, now;
    pid_t child;
    int error, status;

    if (getuid() == 0 || getuid() != geteuid())
        return -EPERM;
    if (!valid_name(name) || strlen(name) > 255)
        return -EINVAL;
    /* Dinit 0.19.4's SETENV packet has a 1024-byte transport limit.
     * Leave room for its command/length header and assignment separator. */
    if (value && strlen(name) + strlen(value) + 1 > 1020)
        return -E2BIG;
    if (!socket || socket[0] != '/')
        return -ENOTCONN;
    directory = strdup(socket);
    if (!directory)
        return -ENOMEM;
    slash = strrchr(directory, '/');
    *slash = '\0';
    error = 0;
    if (lstat(directory, &st) < 0)
        error = errno;
    else if (!S_ISDIR(st.st_mode) || st.st_uid != getuid() ||
        (st.st_mode & 077) != 0)
        error = EPERM;
    else if (lstat(socket, &st) < 0)
        error = errno;
    else if (!S_ISSOCK(st.st_mode) || st.st_uid != getuid())
        error = EPERM;
    free(directory);
    if (error)
        return -error;
    entry = malloc(strlen(name) + (value ? strlen(value) + 1 : 0) + 1);
    if (!entry)
        return -ENOMEM;
    strcpy(entry, name);
    if (value) {
        strcat(entry, "=");
        strcat(entry, value);
    }
    argv[0] = BSD3_DINITCTL;
    argv[1] = "--user";
    argv[2] = "--quiet";
    argv[3] = "--socket-path";
    argv[4] = (char *)socket;
    argv[5] = value ? "setenv" : "unsetenv";
    argv[6] = entry;
    argv[7] = NULL;
    error = posix_spawn_file_actions_init(&actions);
    if (!error) {
        error = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO,
            "/dev/null", O_WRONLY, 0);
        if (!error)
            error = posix_spawn(&child, BSD3_DINITCTL, &actions, NULL, argv, environ);
        posix_spawn_file_actions_destroy(&actions);
    }
    free(entry);
    if (error)
        return -error;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (;;) {
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child)
            return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -EIO;
        if (result < 0 && errno != EINTR)
            return -errno;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec - start.tv_sec >= 5)
            break;
        nanosleep(&delay, NULL);
    }
    kill(child, SIGKILL);
    while (waitpid(child, &status, 0) < 0 && errno == EINTR)
        ;
    return -ETIMEDOUT;
}
