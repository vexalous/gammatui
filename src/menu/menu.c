#define _POSIX_C_SOURCE 200809L
#include "proc.h"
#include "ui.h"
#include "utils.h"
#include "settings.h"
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#define KEY_LOWER(k) ((k) >= 'A' && (k) <= 'Z' ? (k) + 32 : (k))

extern volatile sig_atomic_t active_child;
extern void shutdown_handler(int sig);

static void run_tool(const char *path, bool *ok, const char *fail_msg, WINDOW **win, int *r, int *c, struct cfg *cfg, const char *cfgpath, int h, int ku, int kd, int ks, int kq) {
    if (!*ok) *ok = is_executable_file(path);

    if (!*ok) {
        show_message(*win, "Not found", fail_msg);
        return;
    }

    flushinp();
    def_prog_mode();
    endwin();
    int rc = spawn_and_wait(path);
    struct timespec ts = {0, 100000000L};
    nanosleep(&ts, NULL);
    reset_prog_mode();
    refresh();

    load_config(cfg, cfgpath);
    cfg->key_up = KEY_LOWER(cfg->key_up);
    cfg->key_down = KEY_LOWER(cfg->key_down);
    cfg->key_select = KEY_LOWER(cfg->key_select);
    cfg->key_quit = KEY_LOWER(cfg->key_quit);

    if (*win) delwin(*win);
    *win = recreate_window(r, c);
    if (!*win) {
        *win = newwin(*r - 2, *c - 2, 1, 1);
        keypad(*win, TRUE);
        wtimeout(*win, -1);
    }
    draw_menu(*win, h, ku, kd, ks, kq);

    if (rc == 0) {
        *ok = is_executable_file(path);
    } else {
        char buf[128];
        const char *title = (rc >= 128) ? "Crashed" : ((rc == -1 || rc == 127) ? "Error" : "Exited");
        if (rc == -1) snprintf(buf, sizeof buf, "Failed to run (fork/exec error).");
        else if (rc == 127) snprintf(buf, sizeof buf, "Failed to exec (exit 127).");
        else if (rc >= 128) snprintf(buf, sizeof buf, "Terminated by signal %d", rc - 128);
        else snprintf(buf, sizeof buf, "Exited with status %d", rc);
        show_message(*win, title, buf);
    }
}

int main(int argc, char **argv) {
    struct cfg config;
    char cfgpath[PATH_MAX];
    char g_path[PATH_MAX] = {0};
    char s_path[PATH_MAX] = {0};
    char *arg0 = (argc > 0) ? argv[0] : NULL;

    if (!config_path_for_exe(cfgpath, sizeof(cfgpath), arg0) || !load_config(&config, cfgpath)) {
        config.key_up = 'w';
        config.key_down = 's';
        config.key_select = 10;
        config.key_quit = 'q';
    }

    config.key_up = KEY_LOWER(config.key_up);
    config.key_down = KEY_LOWER(config.key_down);
    config.key_select = KEY_LOWER(config.key_select);
    config.key_quit = KEY_LOWER(config.key_quit);

    bool g_ok = build_gammatui_path(g_path, sizeof g_path, arg0) && is_executable_file(g_path);
    bool s_ok = build_settings_path(s_path, sizeof s_path, arg0) && is_executable_file(s_path);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    struct sigaction sh = {0};
    sh.sa_handler = shutdown_handler;
    sigemptyset(&sh.sa_mask);
    sh.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sh, NULL);
    sigaction(SIGTERM, &sh, NULL);

    int rows, cols, highlight = 0;
    getmaxyx(stdscr, rows, cols);
    WINDOW *win = recreate_window(&rows, &cols);
    if (!win) {
        win = newwin(rows - 2, cols - 2, 1, 1);
        keypad(win, TRUE);
        wtimeout(win, -1);
    }

    draw_menu(win, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);

    bool running = true;
    while (running) {
        int ch = wgetch(win);
        if (ch == ERR) continue;

        if (ch == KEY_RESIZE) {
            endwin();
            refresh();
            getmaxyx(stdscr, rows, cols);
            if (win) delwin(win);
            win = recreate_window(&rows, &cols);
            if (!win) {
                win = newwin(rows - 2, cols - 2, 1, 1);
                keypad(win, TRUE);
                wtimeout(win, -1);
            }
            draw_menu(win, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);
            continue;
        }

        ch = KEY_LOWER(ch);

        if (ch == config.key_up) {
            highlight = (highlight + 2) % 3;
        } else if (ch == config.key_down) {
            highlight = (highlight + 1) % 3;
        } else if (ch == config.key_quit) {
            running = false;
        } else if (ch == config.key_select) {
            if (highlight == MI_Adjustment) {
                run_tool(g_path, &g_ok, "Expected ../gammatui/gammatui.elf relative to menu.elf\nPlace gammatui.elf at ../gammatui/gammatui.elf and make it executable.", &win, &rows, &cols, &config, cfgpath, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);
            } else if (highlight == MI_Settings) {
                run_tool(s_path, &s_ok, "Expected ../settings/brightnesstui.elf relative to menu.elf\nPlace brightnesstui.elf at ../settings/brightnesstui.elf and make it executable.", &win, &rows, &cols, &config, cfgpath, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);
            } else if (highlight == MI_Quit) {
                running = false;
            }
        }
        if (running) draw_menu(win, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);
    }

    if (active_child > 0) terminate_child_group(active_child);
    if (win) delwin(win);
    endwin();
    return 0;
}
