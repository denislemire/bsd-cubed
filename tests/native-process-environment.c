/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */
/* Original regression probe for FreeBSD PR 297512. No real environment is read. */
#include <stdio.h>

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/exec.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t expired;
static const char synthetic[] = "BSD3_BOUNDARY_TEST=synthetic";

static void
deadline(int signal_number)
{

	(void)signal_number;
	expired = 1;
}

static int
receive(int fd, char expected)
{
	struct pollfd input = { .fd = fd, .events = POLLIN };
	char value;

	if (poll(&input, 1, 5000) != 1 || expired ||
	    read(fd, &value, 1) != 1 || value != expected)
		return (-1);
	return (0);
}

static void
child_case(int ready, int release, int boundary)
{
	uintptr_t address = 0;
	size_t length = sizeof(address);
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PS_STRINGS, getpid() };
	struct ps_strings *strings;
	char **saved_vector, *vector[2], *mapping;
	int saved_count, result = 1;
	long page = sysconf(_SC_PAGESIZE);

	/* Only this child changes its own ps_strings, while single-threaded. */
	if (signal(SIGALRM, SIG_DFL) == SIG_ERR)
		_exit(1);
	alarm(10);
	if (page < 256 || (size_t)page < sizeof(synthetic) ||
	    sysctl(mib, 4, &address, &length, NULL, 0) != 0 ||
	    length != sizeof(address) || address == 0)
		_exit(1);
	strings = (struct ps_strings *)address;
	mapping = mmap(NULL, (size_t)page * 2, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_ANON, -1, 0);
	if (mapping == MAP_FAILED)
		_exit(1);
	if (boundary && munmap(mapping + page, (size_t)page) != 0)
		_exit(1);
	vector[0] = mapping + page - sizeof(synthetic);
	vector[1] = NULL;
	memcpy(vector[0], synthetic, sizeof(synthetic));
	saved_vector = strings->ps_envstr;
	saved_count = strings->ps_nenvstr;
	strings->ps_envstr = vector;
	strings->ps_nenvstr = 1;
	/* Pipes keep the vector/mapping alive until the parent's one read ends. */
	if (write(ready, "R", 1) == 1 && receive(release, 'D') == 0)
		result = 0;
	strings->ps_envstr = saved_vector;
	strings->ps_nenvstr = saved_count;
	if (munmap(mapping, boundary ? (size_t)page : (size_t)page * 2) != 0)
		result = 1;
	close(ready);
	close(release);
	_exit(result);
}

static int
run_case(int boundary, int legacy)
{
	int ready[2], release[2], mib[4], status, rc, saved_errno;
	int failed = 0;
	pid_t child, waited;
	char output[512];
	size_t length = sizeof(output);
	const char *label = boundary ? "unmapped-boundary" : "mapped-control";

	if (pipe(ready) != 0)
		return (1);
	if (pipe(release) != 0) {
		close(ready[0]);
		close(ready[1]);
		return (1);
	}
	fflush(stdout);
	child = fork();
	if (child == 0) {
		close(ready[0]);
		close(release[1]);
		child_case(ready[1], release[0], boundary);
	}
	close(ready[1]);
	close(release[0]);
	if (child < 0) {
		close(ready[0]);
		close(release[1]);
		return (1);
	}
	if (receive(ready[0], 'R') != 0) {
		fprintf(stderr, "FAIL %s child readiness\n", label);
		failed = 1;
		goto cleanup;
	}
	/* This is the only environment read: our live, pipe-bound synthetic child. */
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_ENV;
	mib[3] = child;
	memset(output, 0, sizeof(output));
	errno = 0;
	rc = sysctl(mib, 4, output, &length, NULL, 0);
	saved_errno = errno;
	printf("%s rc=%d errno=%d bytes=%zu\n", label, rc, saved_errno, length);
	if (boundary && legacy) {
		failed = rc != -1 || saved_errno != ENOMEM;
	} else {
		failed = rc != 0 || length != sizeof(synthetic) ||
		    memcmp(output, synthetic, sizeof(synthetic)) != 0;
	}
	if (write(release[1], "D", 1) != 1)
		failed = 1;
cleanup:
	close(ready[0]);
	close(release[1]);
	if (failed || expired)
		(void)kill(child, SIGKILL);
	do {
		waited = waitpid(child, &status, 0);
		if (waited < 0 && errno == EINTR && expired)
			(void)kill(child, SIGKILL);
	} while (waited < 0 && errno == EINTR);
	if (waited != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
		failed = 1;
	if (expired)
		failed = 1;
	if (failed)
		fprintf(stderr, "FAIL %s result or child cleanup\n", label);
	return (failed);
}

int
main(int argc, char **argv)
{
	struct sigaction action = { .sa_handler = deadline };
	struct sigaction ignore = { .sa_handler = SIG_IGN };
	int legacy = 0;

	if (argc == 2 && strcmp(argv[1], "--expect-legacy") == 0)
		legacy = 1;
	else if (argc != 1 && (argc != 2 || strcmp(argv[1], "--expect-fixed") != 0)) {
		fprintf(stderr, "usage: %s [--expect-fixed|--expect-legacy]\n", argv[0]);
		return (64);
	}
	if (getuid() == 0 || geteuid() == 0) {
		fprintf(stderr, "Ordinary native FreeBSD user required\n");
		return (77);
	}
	sigemptyset(&action.sa_mask);
	sigemptyset(&ignore.sa_mask);
	if (sigaction(SIGALRM, &action, NULL) != 0 ||
	    sigaction(SIGPIPE, &ignore, NULL) != 0)
		return (1);
	alarm(15);
	if (run_case(0, legacy) != 0 || run_case(1, legacy) != 0)
		return (1);
	alarm(0);
	puts(legacy ? "LEGACY BUG REPRODUCED: NOT A QUALIFICATION" :
	    "PASS: mapped control and unmapped environment boundary");
	return (0);
}
#else
int
main(void)
{

	fprintf(stderr, "Native FreeBSD required\n");
	return (77);
}
#endif
