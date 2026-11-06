/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef __FreeBSD__
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#define _DARWIN_C_SOURCE
#endif
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
#ifdef __FreeBSD__
#include <sys/procctl.h>
#endif
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef BSD3_DINIT
#define BSD3_DINIT "/usr/local/sbin/dinit"
#endif
#ifndef BSD3_DINITCTL
#define BSD3_DINITCTL "/usr/local/sbin/dinitctl"
#endif
static volatile sig_atomic_t requested_signal;

static void
on_signal(int signo)
{
    if (signo != SIGCHLD)
        requested_signal = signo;
}

int
main(int argc, char **argv)
{
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    struct stat st;
    struct sigaction action = {0};
    sigset_t signals, old;
    char *directory, *socket;
    pid_t child, probe;
    struct timespec started, stopping_at = {0}, child_ended = {0}, now;
    int status = 1, probe_status, result = 1, ready[2] = {-1, -1};
    int child_done = 0, probe_done = 0, start_ok = 0, stopping = 0, forced = 0;
    char ready_number[32];

    if (argc != 3 || argv[1][0] != '/' || argv[2][0] == '-' || !*argv[2]) {
        fprintf(stderr, "usage: bsd3-user-session /absolute/services-dir root-service\n");
        return 64;
    }
    if (getuid() == 0 || getuid() != geteuid()) {
        fprintf(stderr, "bsd3-user-session: ordinary user required\n");
        return 77;
    }
    if (!runtime || runtime[0] != '/' || lstat(runtime, &st) < 0 ||
        !S_ISDIR(st.st_mode) || st.st_uid != getuid() || (st.st_mode & 077)) {
        fprintf(stderr, "bsd3-user-session: owned private XDG_RUNTIME_DIR required\n");
        return 78;
    }
    directory = malloc(strlen(runtime) + 32);
    if (!directory)
        return 71;
    sprintf(directory, "%s/bsd3-session.XXXXXX", runtime);
    if (!mkdtemp(directory)) {
        perror("bsd3-user-session: mkdtemp");
        free(directory);
        return 71;
    }
    socket = malloc(strlen(directory) + 9);
    if (!socket) {
        rmdir(directory);
        free(directory);
        return 71;
    }
    sprintf(socket, "%s/control", directory);
    if (setenv("BSD3_SESSION_SOCKET", socket, 1) < 0)
        goto cleanup;
    /* Do not inherit a control connection from another service manager. */
    unsetenv("DINIT_CS_FD");
    unsetenv("DINIT_SOCKET_PATH");
    unsetenv("BSD3_READY_FD");
#ifdef __FreeBSD__
    /* Scope orphan cleanup to descendants of this one login session. */
    if (procctl(P_PID, getpid(), PROC_REAP_ACQUIRE, NULL) < 0) {
        perror("bsd3-user-session: procctl");
        goto cleanup;
    }
#endif
    sigemptyset(&signals);
    sigaddset(&signals, SIGTERM);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGHUP);
    sigaddset(&signals, SIGCHLD);
    sigprocmask(SIG_BLOCK, &signals, &old);
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGCHLD, &action, NULL);
    if (pipe(ready) < 0) {
        perror("bsd3-user-session: readiness pipe");
        goto cleanup;
    }
    snprintf(ready_number, sizeof(ready_number), "%d", ready[1]);
    clock_gettime(CLOCK_MONOTONIC, &started);
    child = fork();
    if (child == 0) {
        close(ready[0]);
        sigprocmask(SIG_SETMASK, &old, NULL);
        execl(BSD3_DINIT, BSD3_DINIT, "--user", "--container",
            "--services-dir", argv[1], "--socket-path", socket,
            "--ready-fd", ready_number, argv[2],
            (char *)NULL);
        perror("bsd3-user-session: dinit");
        _exit(127);
    }
    if (child < 0) {
        perror("bsd3-user-session: fork");
        goto cleanup;
    }
    /* Dinit's own zero exit status does not establish successful service
     * startup. Wait for its control socket, then require an acknowledged root
     * start, including the graph's service-readiness dependencies. */
    probe = fork();
    if (probe == 0) {
        char byte;
        ssize_t count;
        close(ready[1]);
        sigprocmask(SIG_SETMASK, &old, NULL);
        do {
            count = read(ready[0], &byte, 1);
        } while (count < 0 && errno == EINTR);
        close(ready[0]);
        if (count != 1)
            _exit(1);
        execl(BSD3_DINITCTL, BSD3_DINITCTL, "--user", "--quiet",
            "--socket-path", socket, "start", argv[2], (char *)NULL);
        perror("bsd3-user-session: start probe");
        _exit(127);
    }
    close(ready[0]);
    close(ready[1]);
    ready[0] = ready[1] = -1;
    if (probe < 0) {
        perror("bsd3-user-session: fork start probe");
        probe_done = 1;
        requested_signal = SIGTERM;
    }
    while (!child_done || !probe_done) {
        struct timespec delay = {0, 100000000};
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (requested_signal) {
            /* HUP also means end this login; never reload arbitrary services. */
            if (!child_done)
                kill(child, SIGTERM);
            if (!probe_done)
                kill(probe, SIGKILL);
            if (!stopping)
                stopping_at = now;
            stopping = 1;
            requested_signal = 0;
        }
        if (!probe_done && waitpid(probe, &probe_status, WNOHANG) == probe) {
            probe_done = 1;
            start_ok = WIFEXITED(probe_status) && WEXITSTATUS(probe_status) == 0;
            if (!start_ok && !child_done)
                requested_signal = SIGTERM;
        }
        if (!child_done && waitpid(child, &status, WNOHANG) == child) {
            child_done = 1;
            child_ended = now;
        }
        /* Drain a start acknowledgement already queued before a quick logout;
         * never turn an unobserved acknowledgement into a success. */
        if (child_done && !probe_done && now.tv_sec - child_ended.tv_sec >= 5)
            kill(probe, SIGKILL);
        if (!probe_done && now.tv_sec - started.tv_sec >= 120) {
            fprintf(stderr, "bsd3-user-session: graph startup deadline expired\n");
            requested_signal = SIGTERM;
        }
        if (stopping && !child_done && now.tv_sec - stopping_at.tv_sec >= 30) {
            fprintf(stderr, "bsd3-user-session: manager shutdown deadline expired\n");
            kill(child, SIGKILL);
            forced = 1;
        }
        if (!child_done || !probe_done)
            pselect(0, NULL, NULL, NULL, &delay, &old);
    }
    sigprocmask(SIG_SETMASK, &old, NULL);
    if (start_ok && !forced)
        result = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
#ifdef __FreeBSD__
    {
        struct procctl_reaper_kill reap = {0};
        reap.rk_sig = SIGKILL;
        if (procctl(P_PID, getpid(), PROC_REAP_KILL, &reap) < 0 && errno != ESRCH) {
            perror("bsd3-user-session: descendant cleanup");
            result = 1;
        }
        while (waitpid(-1, NULL, 0) > 0 || errno == EINTR)
            ;
    }
#endif
cleanup:
    if (ready[0] >= 0)
        close(ready[0]);
    if (ready[1] >= 0)
        close(ready[1]);
    unlink(socket);
    if (rmdir(directory) < 0) {
        perror("bsd3-user-session: runtime cleanup");
        result = 1;
    }
    free(socket);
    free(directory);
    return result;
}
