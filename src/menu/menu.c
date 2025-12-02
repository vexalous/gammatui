#define _POSIX_C_SOURCE 200809L
#include "proc.h"
#include "ui.h"
#include "utils.h"
#include "settings.h"
#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

extern volatile sig_atomic_t active_child;
extern void shutdown_handler(int sig);

static void config_keys(struct cfg *c) {
    c->key_up = tolower(c->key_up);
    c->key_down = tolower(c->key_down);
    c->key_select = tolower(c->key_select);
    c->key_quit = tolower(c->key_quit);
}

static void setup_win(WINDOW **w) {
    int r, c;
    if (*w) delwin(*w);
    getmaxyx(stdscr, r, c);
    if (!(*w = recreate_window(&r, &c))) {
        *w = newwin(r - 2, c - 2, 1, 1);
        keypad(*w, TRUE);
        wtimeout(*w, -1);
    }
}

static void run(const char *bin, bool *ok, const char *msg, WINDOW **w, struct cfg *c, const char *cp) {
    if (!*ok && !(*ok = is_executable_file(bin))) {
        show_message(*w, "Not found", msg);
        return;
    }

    flushinp();
    def_prog_mode();
    endwin();
    
    int rc = spawn_and_wait(bin);
    nanosleep(&(struct timespec){0, 100000000L}, NULL);
    
    reset_prog_mode();
    refresh();
    
    load_config(c, cp);
    config_keys(c);
    setup_win(w);

    if (rc == 0) {
        *ok = true;
    } else {
        char buf[64];
        if (rc == -1) snprintf(buf, sizeof buf, "Fork failed");
        else if (rc == 127) snprintf(buf, sizeof buf, "Exec failed (127)");
        else if (rc >= 128) snprintf(buf, sizeof buf, "Signal %d", rc - 128);
        else snprintf(buf, sizeof buf, "Exit %d", rc);
        show_message(*w, rc >= 128 ? "Crashed" : "Error", buf);
    }
}

int main(int argc, char **argv) {
    struct cfg c;
    char cp[PATH_MAX], gp[PATH_MAX], sp[PATH_MAX], *a0 = argc > 0 ? argv[0] : NULL;
    
    if (!config_path_for_exe(cp, sizeof cp, a0) || !load_config(&c, cp)) {
        c.key_up = 'w'; c.key_down = 's'; c.key_select = 10; c.key_quit = 'q';
    }
    config_keys(&c);

    bool go = build_gammatui_path(gp, sizeof gp, a0) && is_executable_file(gp);
    bool so = build_settings_path(sp, sizeof sp, a0) && is_executable_file(sp);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    struct sigaction sa = { .sa_handler = shutdown_handler, .sa_flags = SA_RESTART };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    WINDOW *win = NULL;
    setup_win(&win);
    int hl = 0;

    while (true) {
        draw_menu(win, hl, c.key_up, c.key_down, c.key_select, c.key_quit);
        int ch = wgetch(win);
        
        if (ch == KEY_RESIZE) {
            endwin();
            refresh();
            setup_win(&win);
            continue;
        }

        if (ch == ERR) continue;
        ch = tolower(ch);

        if (ch == c.key_quit) break;
        else if (ch == c.key_up) hl = (hl + 2) % 3;
        else if (ch == c.key_down) hl = (hl + 1) % 3;
        else if (ch == c.key_select) {
            if (hl == MI_Quit) break;
            bool g = (hl == MI_Adjustment);
            run(g ? gp : sp, g ? &go : &so, 
                g ? "Missing ../gammatui/gammatui.elf" : "Missing ../settings/brightnesstui.elf", 
                &win, &c, cp);
        }
    }

    if (active_child > 0) terminate_child_group(active_child);
    if (win) delwin(win);
    endwin();
    return 0;
}
