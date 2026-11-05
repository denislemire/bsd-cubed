/* SPDX-License-Identifier: BSD-2-Clause */
/* Root-only, opt-in bounded desktop trial guard. Never installed setuid. */
#include <stdio.h>
#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/consio.h>
#include <sys/kbio.h>
#include <sys/procctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

struct console {
	char path[32];
	struct stat identity;
	struct termios term;
	struct vt_mode mode;
	int keyboard, display;
};
static volatile sig_atomic_t interrupted;
static void stop(int sig) { interrupted = sig; }
static double now(void) {
	struct timespec value;
	clock_gettime(CLOCK_MONOTONIC, &value);
	return (value.tv_sec + value.tv_nsec / 1000000000.0);
}
static int mark(int dir, const char *name, const char *text) {
	int fd = openat(dir, name, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
	if (fd < 0)
		return (-1);
	size_t size = strlen(text), done = 0;
	while (done < size) {
		ssize_t count = write(fd, text + done, size - done);
		if (count < 0 && errno == EINTR) continue;
		if (count <= 0) { close(fd); return (-1); }
		done += (size_t)count;
	}
	int result = fsync(fd);
	if (close(fd) != 0) result = -1;
	return (result);
}
static int kill_owned(int sig, unsigned *count) {
	struct procctl_reaper_kill action = { .rk_sig = sig };
	if (procctl(P_PID, 0, PROC_REAP_KILL, &action) != 0 && errno != ESRCH)
		return (-1);
	*count += action.rk_killed;
	return (0);
}
static int same_term(const struct termios *a, const struct termios *b) {
	return (a->c_iflag == b->c_iflag && a->c_oflag == b->c_oflag &&
	    a->c_cflag == b->c_cflag && a->c_lflag == b->c_lflag &&
	    cfgetispeed(a) == cfgetispeed(b) && cfgetospeed(a) == cfgetospeed(b) &&
	    memcmp(a->c_cc, b->c_cc, sizeof(a->c_cc)) == 0);
}

int
main(int argc, char **argv)
{
	struct console saved[32];
	struct sigaction action = { .sa_handler = stop };
	struct stat directory;
	char guest[32], text[512], *end;
	size_t guest_size = sizeof(guest);
	int original = 0, current = 0, total = 0, dir, log, channel[2], status = 0;
	int recovery = 0, failed = 0, deadline_hit = 0, child_done = 0;
	unsigned terminated = 0, killed = 0;
	pid_t child;

	if (argc < 5 || strcmp(argv[3], "--") != 0 || getuid() != 0 || geteuid() != 0)
		return (64);
	errno = 0;
	unsigned long seconds = strtoul(argv[1], &end, 10);
	if (errno || *argv[1] == '\0' || *end != '\0' || seconds < 1 || seconds > 600)
		return (64);
	if (sysctlbyname("kern.vm_guest", guest, &guest_size, NULL, 0) != 0)
		return (1);
	if (strcmp(guest, "kvm") != 0 &&
	    (getenv("BSD3_ALLOW_HARDWARE_TRIAL") == NULL ||
	    strcmp(getenv("BSD3_ALLOW_HARDWARE_TRIAL"), "yes") != 0)) {
		fputs("bsd3-vt-guard: hardware requires an explicitly coordinated trial\n", stderr);
		return (64);
	}
	for (int i = 0; i < 32; i++) {
		struct console *item = &saved[total];
		snprintf(item->path, sizeof(item->path), "/dev/ttyv%c", "0123456789abcdefghijklmnopqrstuv"[i]);
		int fd = open(item->path, O_RDWR | O_NOCTTY | O_CLOEXEC);
		if (fd < 0) {
			if (errno == ENOENT) continue;
			perror("VT baseline open"); return (1);
		}
		int index;
		if (fstat(fd, &item->identity) != 0 || !S_ISCHR(item->identity.st_mode) ||
		    tcgetattr(fd, &item->term) != 0 || ioctl(fd, VT_GETINDEX, &index) != 0 ||
		    index != i + 1 || ioctl(fd, VT_GETMODE, &item->mode) != 0 ||
		    item->mode.mode != VT_AUTO || ioctl(fd, KDGKBMODE, &item->keyboard) != 0 ||
		    ioctl(fd, KDGETMODE, &item->display) != 0) {
			fputs("bsd3-vt-guard: console unavailable or already controlled; refusing trial\n", stderr);
			close(fd); return (1);
		}
		if (i == 0 && ioctl(fd, VT_GETACTIVE, &original) != 0) {
			close(fd); return (1);
		}
		close(fd);
		total++;
	}
	if (total == 0 || strcmp(saved[0].path, "/dev/ttyv0") != 0 || original <= 0)
		return (1);
	if (mkdir(argv[2], 0700) != 0)
		return (1);
	dir = open(argv[2], O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
	if (dir < 0 || fstat(dir, &directory) != 0 || directory.st_uid != 0 ||
	    (directory.st_mode & 0777) != 0700)
		return (1);
	log = openat(dir, "workload.log", O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
	if (log < 0 || pipe(channel) != 0 || procctl(P_PID, 0, PROC_REAP_ACQUIRE, NULL) != 0)
		return (1);
	sigemptyset(&action.sa_mask);
	sigaction(SIGHUP, &action, NULL); sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
	signal(SIGPIPE, SIG_IGN);
	char **command = calloc((size_t)argc + 2, sizeof(char *));
	if (command == NULL) return (1);
	command[0] = "/usr/bin/timeout"; command[1] = "-k"; command[2] = "5";
	command[3] = argv[1];
	for (int i = 4; i < argc; i++) command[i] = argv[i];
	child = fork();
	if (child < 0) return (1);
	if (child == 0) {
		close(channel[1]);
		char ready;
		if (read(channel[0], &ready, 1) != 1) _exit(125);
		close(channel[0]);
		signal(SIGPIPE, SIG_DFL);
		if (setsid() < 0 || dup2(log, 1) < 0 || dup2(log, 2) < 0) _exit(125);
		close(log);
		int null = open("/dev/null", O_RDONLY);
		if (null < 0 || dup2(null, 0) < 0) _exit(125);
		if (null != 0) close(null);
		snprintf(text, sizeof(text), "controller=%ld\n", (long)getpid());
		if (mark(dir, "controller-ready.txt", text) != 0) _exit(125);
		close(dir);
		execv(command[0], command);
		_exit(126);
	}
	close(channel[0]); close(log); free(command);
	snprintf(text, sizeof(text), "guard=%ld\ncontroller=%ld\noriginal_vt=%d\nconsoles=%d\n",
	    (long)getpid(), (long)child, original, total);
	if (mark(dir, "guard-ready.txt", text) != 0 || write(channel[1], "1", 1) != 1)
		failed = 1;
	close(channel[1]);
	double deadline = now() + seconds + 15;
	while (!failed && !interrupted && now() < deadline) {
		pid_t waited = waitpid(child, &status, WNOHANG);
		if (waited == child) { child_done = 1; break; }
		if (waited < 0 && errno != EINTR) { failed = 1; break; }
		usleep(10000);
	}
	if (!child_done && !interrupted && now() >= deadline) deadline_hit = 1;
	if (kill_owned(SIGTERM, &terminated) != 0) failed = 1;
	deadline = now() + 5;
	int escalated = 0;
	for (;;) {
		int one_status;
		pid_t waited = waitpid(-1, &one_status, WNOHANG);
		if (waited == child) { status = one_status; child_done = 1; }
		if (waited < 0 && errno == ECHILD) break;
		if (now() >= deadline) {
			if (escalated) { failed = 1; break; }
			if (kill_owned(SIGKILL, &killed) != 0) failed = 1;
			escalated = 1; deadline = now() + 5;
		}
		if (waited <= 0) usleep(10000);
	}
	int child_exit = child_done && WIFEXITED(status) ? WEXITSTATUS(status) :
	    child_done && WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 125;
	snprintf(text, sizeof(text), "%d\n", child_exit);
	if (mark(dir, "command-exit.txt", text) != 0) failed = 1;
	for (int i = 0; i < total; i++) {
		struct console *item = &saved[i];
		struct stat identity;
		struct termios term;
		struct vt_mode mode;
		int keyboard, display;
		int fd = open(item->path, O_RDWR | O_NOCTTY | O_CLOEXEC);
		if (fd < 0 || fstat(fd, &identity) != 0 || identity.st_rdev != item->identity.st_rdev) {
			recovery = 1; if (fd >= 0) close(fd); continue;
		}
		if (ioctl(fd, VT_SETMODE, &item->mode) != 0) recovery = 1;
		if (ioctl(fd, KDSKBMODE, item->keyboard) != 0) recovery = 1;
		if (ioctl(fd, KDSETMODE, item->display) != 0) recovery = 1;
		if (tcsetattr(fd, TCSANOW, &item->term) != 0) recovery = 1;
		if (fchown(fd, item->identity.st_uid, item->identity.st_gid) != 0) recovery = 1;
		if (fchmod(fd, item->identity.st_mode & 07777) != 0) recovery = 1;
		if (ioctl(fd, VT_GETMODE, &mode) != 0 || mode.mode != item->mode.mode ||
		    ioctl(fd, KDGKBMODE, &keyboard) != 0 || keyboard != item->keyboard ||
		    ioctl(fd, KDGETMODE, &display) != 0 || display != item->display ||
		    tcgetattr(fd, &term) != 0 || !same_term(&term, &item->term) ||
		    fstat(fd, &identity) != 0 || identity.st_uid != item->identity.st_uid ||
		    identity.st_gid != item->identity.st_gid ||
		    (identity.st_mode & 07777) != (item->identity.st_mode & 07777)) recovery = 1;
		close(fd);
	}
	int control = open("/dev/ttyv0", O_RDWR | O_NOCTTY | O_CLOEXEC);
	if (control < 0 || ioctl(control, VT_ACTIVATE, original) != 0) recovery = 1;
	deadline = now() + 3;
	while (now() < deadline) {
		if (ioctl(control, VT_GETACTIVE, &current) == 0 && current == original) break;
		usleep(10000);
	}
	if (current != original) recovery = 1;
	if (control >= 0) close(control);
	snprintf(text, sizeof(text), "recovery=%s\nactive_vt=%d\nexpected_vt=%d\nchild_exit=%d\nrequested_signal=%d\nouter_deadline=%d\nterminated_descendants=%u\nkilled_descendants=%u\n",
	    recovery ? "FAIL" : "PASS", current, original, child_exit, (int)interrupted,
	    deadline_hit, terminated, killed);
	if (mark(dir, "result.txt", text) != 0) failed = 1;
	int result = failed || recovery || child_exit != 0 || interrupted || deadline_hit || terminated || killed;
	if (mark(dir, "complete.txt", result ? "FAIL_OR_ASSISTED\n" : "PASS\n") != 0) result = 1;
	close(dir);
	procctl(P_PID, 0, PROC_REAP_RELEASE, NULL);
	return (result != 0);
}
#else
int main(void) { fputs("bsd3-vt-guard requires FreeBSD\n", stderr); return (69); }
#endif
