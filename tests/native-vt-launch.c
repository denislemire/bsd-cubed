/* SPDX-License-Identifier: BSD-2-Clause */
/* Disposable KVM only: attach an unused real VT, drop privilege, restore it. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/consio.h>
#include <sys/kbio.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <ttyent.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t interrupted;
static void stop(int sig) { interrupted = sig; }
static double now(void) {
	struct timespec value;
	clock_gettime(CLOCK_MONOTONIC, &value);
	return value.tv_sec + value.tv_nsec / 1000000000.0;
}

int
main(int argc, char **argv)
{
	struct passwd *user;
	struct ttyent *target;
	struct stat saved;
	struct termios term;
	struct vt_mode mode;
	struct sigaction action = { .sa_handler = stop };
	struct pollfd input;
	char guest[32], line[4096], buffer[512], original_text[16];
	size_t guest_size = sizeof(guest), used = 0;
	int fd, control, original, current, kbd, display, channel[2], status = 0;
	int recovery_failed = 0;
	pid_t child;
	int attached = 0, paused = 0, resumed = 0, exited = 0, failed = 0;
	double deadline;

	if (argc != 4 || geteuid() != 0 || getenv("BSD3_DISPOSABLE_VM_TEST") == NULL ||
	    strcmp(getenv("BSD3_DISPOSABLE_VM_TEST"), "yes") != 0)
		return (64);
	if (sysctlbyname("kern.vm_guest", guest, &guest_size, NULL, 0) != 0 ||
	    strcmp(guest, "kvm") != 0)
		return (64);
	struct rlimit no_core = { 0, 0 };
	if (setrlimit(RLIMIT_CORE, &no_core) != 0)
		return (1);
	user = getpwnam(argv[1]);
	if (user == NULL || user->pw_uid == 0)
		return (64);
	target = getttynam("ttyv8");
	if (target == NULL || (target->ty_status & TTY_ON) != 0)
		return (64);
	endttyent();
	control = open("/dev/ttyv0", O_RDWR | O_NOCTTY | O_CLOEXEC);
	fd = open("/dev/ttyv8", O_RDWR | O_NOCTTY | O_CLOEXEC);
	if (control < 0 || fd < 0 || fstat(fd, &saved) != 0 || tcgetattr(fd, &term) != 0 ||
	    ioctl(control, VT_GETACTIVE, &original) != 0 || original <= 0 || original == 9 ||
	    ioctl(fd, VT_GETMODE, &mode) != 0 || mode.mode != VT_AUTO ||
	    ioctl(fd, KDGKBMODE, &kbd) != 0 || ioctl(fd, KDGETMODE, &display) != 0 ||
	    pipe(channel) != 0) {
		perror("VT baseline");
		return (1);
	}
	printf("VT baseline: active=%d; unused target=9\n", original);
	fflush(stdout);
	snprintf(original_text, sizeof(original_text), "%d", original);
	sigemptyset(&action.sa_mask);
	for (int i = 0; i < 4; i++) {
		int signals[] = { SIGHUP, SIGINT, SIGTERM, SIGALRM };
		sigaction(signals[i], &action, NULL);
	}
	child = fork();
	if (child < 0)
		return (1);
	if (child == 0) {
		close(channel[0]);
		close(control);
		if (setsid() < 0 || ioctl(fd, TIOCSCTTY, 0) != 0 ||
		    dup2(fd, STDIN_FILENO) < 0 || dup2(channel[1], STDOUT_FILENO) < 0 ||
		    dup2(channel[1], STDERR_FILENO) < 0)
			_exit(125);
		close(fd);
		close(channel[1]);
		if (setlogin(user->pw_name) != 0 || initgroups(user->pw_name, user->pw_gid) != 0 ||
		    setgid(user->pw_gid) != 0 || setuid(user->pw_uid) != 0)
			_exit(125);
		if (chdir(user->pw_dir) != 0)
			_exit(125);
		setenv("HOME", user->pw_dir, 1);
		setenv("BSD3_ORIGINAL_VT", original_text, 1);
		unsetenv("LD_PRELOAD");
		unsetenv("LD_LIBRARY_PATH");
		puts("BSD3VT:ATTACHED");
		fflush(stdout);
		execl(argv[2], argv[2], "--console", argv[3], (char *)NULL);
		_exit(126);
	}
	close(channel[1]);
	input = (struct pollfd){ .fd = channel[0], .events = POLLIN };
	deadline = now() + 35;
	while (!interrupted && now() < deadline) {
		if (poll(&input, 1, 100) > 0) {
			ssize_t count = read(channel[0], buffer, sizeof(buffer));
			if (count == 0) {
				if (!exited && waitpid(child, &status, WNOHANG) == child)
					exited = 1;
				if (exited)
					break;
				usleep(10000);
				continue;
			}
			if (count < 0) {
				if (errno != EINTR)
					failed = 1;
				break;
			}
			fwrite(buffer, 1, (size_t)count, stdout);
			fflush(stdout);
			for (ssize_t i = 0; i < count; i++) {
				if (buffer[i] == '\n') {
					line[used] = '\0';
					if (strcmp(line, "BSD3VT:ATTACHED") == 0) {
						attached = 1;
						if (ioctl(control, VT_ACTIVATE, 9) != 0)
							failed = 1;
					}
					if (strcmp(line, "BSD3VT:PAUSED") == 0)
						paused = 1;
					if (strcmp(line, "BSD3VT:RESUMED") == 0)
						resumed = 1;
					used = 0;
				} else if (used < sizeof(line) - 1)
					line[used++] = buffer[i];
			}
		}
		if (paused == 1 && ioctl(control, VT_GETACTIVE, &current) == 0 && current == original) {
			if (ioctl(control, VT_ACTIVATE, 9) != 0)
				failed = 1;
			paused = 2;
		}
		if (!exited && waitpid(child, &status, WNOHANG) == child)
			exited = 1;
		if (failed)
			break;
	}
	if (!exited) {
		kill(-child, SIGTERM);
		kill(child, SIGTERM);
		double stop_deadline = now() + 2;
		while (now() < stop_deadline) {
			if (waitpid(child, &status, WNOHANG) == child) { exited = 1; break; }
			usleep(10000);
		}
		if (!exited) {
			kill(-child, SIGKILL);
			kill(child, SIGKILL);
			waitpid(child, &status, 0);
		}
		failed = 1;
	}
	/* Session teardown can revoke its tty handles; reopen independent controls. */
	close(fd);
	close(control);
	control = open("/dev/ttyv0", O_RDWR | O_NOCTTY | O_CLOEXEC);
	fd = open("/dev/ttyv8", O_RDWR | O_NOCTTY | O_CLOEXEC);
	struct stat after;
	if (fd < 0 || fstat(fd, &after) != 0 || after.st_rdev != saved.st_rdev)
		recovery_failed = 1;
	if (ioctl(fd, VT_SETMODE, &mode) != 0) { perror("restore VT mode"); recovery_failed = 1; }
	if (ioctl(fd, KDSKBMODE, kbd) != 0) { perror("restore keyboard"); recovery_failed = 1; }
	if (ioctl(fd, KDSETMODE, display) != 0) { perror("restore display"); recovery_failed = 1; }
	if (tcsetattr(fd, TCSANOW, &term) != 0) { perror("restore termios"); recovery_failed = 1; }
	if (ioctl(control, VT_ACTIVATE, original) != 0) { perror("restore active VT"); recovery_failed = 1; }
	deadline = now() + 3;
	current = 0;
	while (now() < deadline) {
		if (ioctl(control, VT_GETACTIVE, &current) == 0 && current == original)
			break;
		usleep(10000);
	}
	if (fchown(fd, saved.st_uid, saved.st_gid) != 0) recovery_failed = 1;
	if (fchmod(fd, saved.st_mode & 07777) != 0) recovery_failed = 1;
	struct vt_mode restored_mode;
	int restored_kbd, restored_display;
	if (ioctl(fd, VT_GETMODE, &restored_mode) != 0 || restored_mode.mode != mode.mode ||
	    ioctl(fd, KDGKBMODE, &restored_kbd) != 0 || restored_kbd != kbd ||
	    ioctl(fd, KDGETMODE, &restored_display) != 0 || restored_display != display ||
	    fstat(fd, &after) != 0 || after.st_uid != saved.st_uid || after.st_gid != saved.st_gid ||
	    (after.st_mode & 07777) != (saved.st_mode & 07777) || current != original)
		recovery_failed = 1;
	close(channel[0]);
	close(fd);
	close(control);
	if (!attached || paused != 2 || !resumed || current != original || interrupted ||
	    !WIFEXITED(status) || WEXITSTATUS(status) != 0)
		failed = 1;
	printf("VT recovery: active=%d expected=%d recovery=%s scenario=%s\n",
	    current, original, recovery_failed ? "FAIL" : "PASS", failed ? "FAIL" : "PASS");
	return (failed || recovery_failed);
}
