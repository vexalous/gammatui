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

extern volatile sig_atomic_t active_child;
extern void shutdown_handler(int sig);

static int normalize_key(int k) {
    if (k >= 'A' && k <= 'Z') return k + 32;
    return k;
}

int main(int argc, char **argv) {
    struct cfg config;
    char cfgpath[PATH_MAX];
    bool config_loaded = false;
    if (config_path_for_exe(cfgpath, sizeof(cfgpath), (argc > 0) ? argv[0] : NULL)) {
        config_loaded = load_config(&config, cfgpath);
    }
    if (!config_loaded) {
        config.key_up = 'w';
        config.key_down = 's';
        config.key_select = 10;
        config.key_quit = 'q';
    }

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

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    WINDOW *win = recreate_window(&rows, &cols);
    if (!win) {
        win = newwin(rows - 2, cols - 2, 1, 1);
        keypad(win, TRUE);
        wtimeout(win, -1);
    }

    int highlight = 0;
    char gammatui_path[PATH_MAX] = {0};
    bool gammatui_built = build_gammatui_path(gammatui_path, sizeof gammatui_path, (argc > 0) ? argv[0] : NULL);
    bool gammatui_ok = gammatui_built && is_executable_file(gammatui_path);

    char settings_path[PATH_MAX] = {0};
    bool settings_built = build_settings_path(settings_path, sizeof settings_path, (argc > 0) ? argv[0] : NULL);
    bool settings_ok = settings_built && is_executable_file(settings_path);

    draw_menu(win, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);
    bool running = true;
    while (running) {
        int ch = wgetch(win);
        
        int check_ch = normalize_key(ch);
        int check_up = normalize_key(config.key_up);
        int check_down = normalize_key(config.key_down);
        int check_sel = normalize_key(config.key_select);
        int check_quit = normalize_key(config.key_quit);

        if (ch == KEY_RESIZE) {
            endwin();
            refresh();
            getmaxyx(stdscr, rows, cols);
            if (win) delwin(win);
            win = recreate_window(&rows, &cols);
            draw_menu(win, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);
            continue;
        }

        if (ch == ERR) continue;

        if (check_ch == check_up)
            highlight = (highlight + 3 - 1) % 3;
        else if (check_ch == check_down)
            highlight = (highlight + 1) % 3;
        else if (check_ch == check_quit)
            running = false;
        else if (check_ch == check_sel) {
            if (highlight == MI_Adjustment) {
                if (!gammatui_ok) gammatui_ok = is_executable_file(gammatui_path);

                if (!gammatui_ok) {
                    show_message(win, "Not found",
                        "Expected ../gammatui/gammatui.elf relative to menu.elf\nPlace gammatui.elf at ../gammatui/gammatui.elf and make it executable.");
                } else {
                    flushinp();
                    def_prog_mode();
                    endwin();
                    int rc = spawn_and_wait(gammatui_path);
                    struct timespec ts = {0, 100000000L};
                    nanosleep(&ts, NULL);
                    reset_prog_mode();
                    refresh();
                    
                    load_config(&config, cfgpath);
                    
                    if (win) delwin(win);
                    win = recreate_window(&rows, &cols);
                    draw_menu(win, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);

                    if (rc == 0)
                        gammatui_ok = is_executable_file(gammatui_path);
                    else if (rc == -1)
                        show_message(win, "Error", "Failed to run adjustment (fork/exec error).");
                    else if (rc == 127)
                        show_message(win, "Error", "Adjustment failed to exec (exit 127).");
                    else if (rc >= 128) {
                        char buf[128];
                        snprintf(buf, sizeof buf, "Adjustment terminated by signal %d", rc - 128);
                        show_message(win, "Crashed", buf);
                    } else {
                        char buf[128];
                        snprintf(buf, sizeof buf, "Adjustment exited with status %d", rc);
                        show_message(win, "Exited", buf);
                    }
                }
            } else if (highlight == MI_Settings) {
                if (!settings_ok) settings_ok = is_executable_file(settings_path);

                if (!settings_ok) {
                    show_message(win, "Not found",
                        "Expected ../settings/brightnesstui.elf relative to menu.elf\nPlace brightnesstui.elf at ../settings/brightnesstui.elf and make it executable.");
                } else {
                    flushinp();
                    def_prog_mode();
                    endwin();
                    int rc = spawn_and_wait(settings_path);
                    struct timespec ts = {0, 100000000L};
                    nanosleep(&ts, NULL);
                    reset_prog_mode();
                    refresh();
                    
                    if (load_config(&config, cfgpath)) {
                    }

                    if (win) delwin(win);
                    win = recreate_window(&rows, &cols);
                    draw_menu(win, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);

                    if (rc == 0)
                        settings_ok = is_executable_file(settings_path);
                    else if (rc == -1)
                        show_message(win, "Error", "Failed to run settings (fork/exec error).");
                    else if (rc == 127)
                        show_message(win, "Error", "Settings failed to exec (exit 127).");
                    else if (rc >= 128) {
                        char buf[128];
                        snprintf(buf, sizeof buf, "Settings terminated by signal %d", rc - 128);
                        show_message(win, "Crashed", buf);
                    } else {
                        char buf[128];
                        snprintf(buf, sizeof buf, "Settings exited with status %d", rc);
                        show_message(win, "Exited", buf);
                    }
                }
            } else if (highlight == MI_Quit) {
                running = false;
            }
        }
        if (running) draw_menu(win, highlight, config.key_up, config.key_down, config.key_select, config.key_quit);
    }
    if (active_child > 0)
        terminate_child_group(active_child);
    if (win)
        delwin(win);
    endwin();
    return 0;
}
