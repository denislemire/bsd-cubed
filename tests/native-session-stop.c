/* SPDX-License-Identifier: BSD-2-Clause */
/* Termination regression: an owned descendant escapes the process group. */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
int main(void) {
	int channel[2];
	char ready;
	if (getuid() == 0 || getppid() <= 1 || pipe(channel) != 0) return (64);
	pid_t detached = fork();
	if (detached < 0) return (1);
	if (detached == 0) {
		close(channel[0]);
		if (setsid() < 0) _exit(1);
		signal(SIGTERM, SIG_IGN); signal(SIGHUP, SIG_IGN); signal(SIGINT, SIG_IGN);
		if (write(channel[1], "1", 1) != 1) _exit(1);
		close(channel[1]);
		for (;;) pause();
	}
	close(channel[1]);
	if (read(channel[0], &ready, 1) != 1) return (1);
	close(channel[0]);
	printf("BSD3_STOP:detached=%ld\n", (long)detached); fflush(stdout);
	if (kill(getppid(), SIGTERM) != 0) return (1);
	for (;;) pause();
}
