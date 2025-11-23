#define _XOPEN_SOURCE 700
#include "gammatui.h"
#include "gamma_control.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ncurses.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h>

static struct cfg config;

static void load_app_config(const char *argv0) {
    char cfgpath[PATH_MAX];
    if (config_path_for_exe(cfgpath, sizeof(cfgpath), argv0)) {
        if (!load_config(&config, cfgpath)) {
            config.gamma_min = 0.1;
            config.gamma_max = 10.0;
            config.bright_min = 0.1;
            config.bright_max = 2.0;
            config.selected_color = 7;
            config.unselected_color = 5;
            config.key_up = 'w';
            config.key_down = 's';
            config.key_select = 10;
            config.key_quit = 'q';
        }
    }
}

static void handle_resize(WINDOW **win, int *rows, int *cols) {
    int new_rows, new_cols;
    getmaxyx(stdscr, new_rows, new_cols);
    if (new_rows < 5 || new_cols < 10) return;
    if (new_rows != *rows || new_cols != *cols) {
        *rows = new_rows;
        *cols = new_cols;
        wresize(*win, *rows - 2, *cols - 2);
        mvwin(*win, 1, 1);
        clear();
        refresh();
    }
}

static int normalize_key(int k) {
    if (k >= 'A' && k <= 'Z') return k + 32;
    return k;
}

int main(int argc, char **argv) {
    load_app_config(argc > 0 ? argv[0] : NULL);
    int opt;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };
    const char *forced_output = NULL;
    while ((opt = getopt_long(argc, argv, "ho:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'o':
                forced_output = optarg;
                break;
            case 'h':
                printf("Usage: %s [--output NAME]\n", argv[0]);
                return 0;
            default:
                fprintf(stderr, "Usage: %s [--output NAME]\n", argv[0]);
                return 1;
        }
    }

    if (system("command -v xrandr >/dev/null 2>&1") != 0) {
        fprintf(stderr, "WARNING: xrandr not found. The UI will still run but changes won't apply.\n");
    }

    if (forced_output) {
        snprintf(display_output, sizeof(display_output), "%s", forced_output);
    } else if (!detect_output(display_output, sizeof(display_output))) {
        display_output[0] = '\0';
    }

    double gamma = clamp_double(1.0, config.gamma_min, config.gamma_max);
    double bright = clamp_double(1.0, config.bright_min, config.bright_max);
    int selected = 0;
    int rows, cols;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) init_colors_safe(config.selected_color, config.unselected_color);
    getmaxyx(stdscr, rows, cols);
    if (rows < 5 || cols < 10) {
        endwin();
        fprintf(stderr, "Terminal too small.\n");
        return 1;
    }

    WINDOW *win = newwin(rows - 2, cols - 2, 1, 1);
    keypad(win, TRUE);
    
    draw_ui(win, gamma, bright, selected, rows - 2, cols - 2, 
            config.key_up, config.key_down, config.key_select, config.key_quit);

    int ch;
    bool running = true;
    while (running && (ch = wgetch(win)) != ERR) {
        bool needs_redraw = false;
        bool needs_apply = false;
        
        int check_ch = normalize_key(ch);
        int check_up = normalize_key(config.key_up);
        int check_down = normalize_key(config.key_down);
        int check_sel = normalize_key(config.key_select);
        int check_quit = normalize_key(config.key_quit);

        if (check_ch == check_quit) {
            running = false;
        } else if (check_ch == check_up) {
            selected = 0;
            needs_redraw = true;
        } else if (check_ch == check_down) {
            selected = 1;
            needs_redraw = true;
        } else if (check_ch == check_sel) {
            char buf[32] = {0};
            int h = getmaxy(win);
            curs_set(1);
            echo();
            mvwprintw(win, h - 2, 2, "Set Value: ");
            wrefresh(win);
            wgetnstr(win, buf, sizeof(buf) - 1);
            noecho();
            curs_set(0);
            char *endptr;
            double val = strtod(buf, &endptr);
            if (endptr != buf) {
                if (selected == 0) {
                    if (val >= config.gamma_min && val <= config.gamma_max) {
                        gamma = val;
                        needs_apply = true;
                    }
                } else {
                    if (val >= config.bright_min && val <= config.bright_max) {
                        bright = val;
                        needs_apply = true;
                    }
                }
            }
            needs_redraw = true;
        } else if (ch == 'r' || ch == 'R') {
            gamma = clamp_double(1.0, config.gamma_min, config.gamma_max);
            bright = clamp_double(1.0, config.bright_min, config.bright_max);
            apply_values(gamma, bright);
            needs_redraw = true;
        } else if (ch == KEY_RESIZE) {
            handle_resize(&win, &rows, &cols);
            needs_redraw = true;
        }

        if (needs_apply && debounce_allow()) {
            apply_values(gamma, bright);
        }
        if (needs_redraw) {
            draw_ui(win, gamma, bright, selected, rows - 2, cols - 2,
                    config.key_up, config.key_down, config.key_select, config.key_quit);
        }
    }
    delwin(win);
    endwin();
    return 0;
}
