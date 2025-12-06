#define _POSIX_C_SOURCE 200809L
#include "proc.h"
#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

volatile sig_atomic_t resized = 0;
volatile pid_t active_child = 0;

void sigwinch_handler(int s) { (void)s; resized = 1; }

void terminate_child_group(pid_t p) {
    if (p <= 0) return;
    kill(-p, SIGTERM);
    if (waitpid(p, 0, WNOHANG) != p) {
        nanosleep(&(struct timespec){0, 200000000}, 0);
        if (waitpid(p, 0, WNOHANG) != p) {
            kill(-p, SIGKILL);
            waitpid(p, 0, 0);
        }
    }
    while (waitpid(-1, 0, WNOHANG) > 0);
}

__attribute__((noreturn)) void shutdown_handler(int s) {
    if (active_child > 0) terminate_child_group(active_child);
    endwin();
    _exit(128 + (s & 0x7f));
}

int spawn_and_wait(const char *b) {
    pid_t p = fork();
    if (p < 0) return -1;
    if (!p) {
#ifdef __linux__
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (getppid() == 1) _exit(127);
#endif
        if (setsid() < 0) _exit(127);
        int f = open("/dev/tty", O_RDWR);
        if (f >= 0) {
            dup2(f, 0); dup2(f, 1); dup2(f, 2);
            if (f > 2) close(f);
        }
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        execl(b, b, (char *)0);
        _exit(127);
    }
    active_child = p;
    int s;
    while (waitpid(p, &s, 0) < 0 && errno == EINTR);
    active_child = 0;
    while (waitpid(-1, 0, WNOHANG) > 0);
    return WIFEXITED(s) ? WEXITSTATUS(s) : (WIFSIGNALED(s) ? 128 + WTERMSIG(s) : -1);
}
