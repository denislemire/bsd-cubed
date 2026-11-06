/* SPDX-License-Identifier: BSD-2-Clause */
#define _POSIX_C_SOURCE 200809L
#include <bsd3/user-session.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void
mark(const char *event)
{
    const char *path = getenv("TEST_EVENTS");
    FILE *file = path ? fopen(path, "a") : NULL;
    if (!file)
        exit(90);
    fprintf(file, "%s\n", event);
    fclose(file);
}

int
main(int argc, char **argv)
{
    int error;
    if (argc < 2)
        return 64;
    if (!strcmp(argv[1], "setenv")) {
        if (argc < 3)
            return 64;
        error = bsd3_session_setenv(argv[2], argc > 3 ? argv[3] : NULL);
        printf("%d\n", error);
        return error ? 1 : 0;
    }
    if (!strcmp(argv[1], "ready") || !strcmp(argv[1], "ready-ignored")) {
        if (!strcmp(argv[1], "ready-ignored"))
            signal(SIGPIPE, SIG_IGN);
        error = bsd3_session_ready();
        printf("%d\n", error);
        return error ? 1 : 0;
    }
    if (!strcmp(argv[1], "dependency")) {
        struct timespec delay = {0, 300000000};
        mark("dependency-starting");
        nanosleep(&delay, NULL);
        mark("dependency-ready");
        if (bsd3_session_ready() < 0)
            return 91;
    } else if (!strcmp(argv[1], "dependent")) {
        mark("dependent-started");
    } else if (!strcmp(argv[1], "environment")) {
        const char *value = getenv("BSD3_TEST_VALUE");
        mark(value ? value : "unset");
        return 0;
    } else if (!strcmp(argv[1], "crash")) {
        mark("crashed");
        return 42;
    } else if (!strcmp(argv[1], "stubborn")) {
        signal(SIGTERM, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        mark("stubborn-started");
        if (bsd3_session_ready() < 0)
            return 91;
    } else if (!strcmp(argv[1], "orphan")) {
        pid_t child = fork();
        if (child < 0)
            return 92;
        if (child == 0) {
            char event[64];
            if (setsid() < 0)
                return 93;
            snprintf(event, sizeof(event), "orphan-pid=%ld", (long)getpid());
            mark(event);
        } else if (bsd3_session_ready() < 0) {
            return 91;
        }
    } else if (!strcmp(argv[1], "never-ready")) {
        mark("never-ready-started");
    } else {
        return 64;
    }
    for (;;)
        pause();
}
