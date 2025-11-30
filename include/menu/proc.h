#ifndef MENU_PROC_H
#define MENU_PROC_H
#include <sys/types.h>
#include <signal.h>

extern volatile sig_atomic_t resized;
extern volatile pid_t active_child;

void sigwinch_handler(int sig);
void shutdown_handler(int sig);
int spawn_and_wait(const char *binpath);
void terminate_child_group(pid_t childpid);
