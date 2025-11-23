#define _XOPEN_SOURCE 700
#include "gammatui.h"
#include "gamma_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ncurses.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h>

static double config_gamma_min = 0.1;
static double config_gamma_max = 10.0;
static double config_bright_min = 0.1;
static double config_bright_max = 2.0;
static int config_sel_color = 7;
static int config_unsel_color = 5;

static void load_config_limits(const char *argv0) {
    char exe_path[PATH_MAX] = {0};
#if defined(linux)
    ssize_t n = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
    if (n > 0) exe_path[n] = '\0';
#endif
    if (exe_path[0] == '\0' && argv0) {
        if (realpath(argv0, exe_path) == NULL) {
            strncpy(exe_path, argv0, sizeof(exe_path)-1);
        }
    }
    char *dir = dirname(exe_path);
    char config_path[PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s/../settings/config.json", dir);

    FILE *f = fopen(config_path, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *key = line;
        while (isspace((unsigned char)*key)) key++;

        if (strncmp(key, "\"gamma_min\"", 11) == 0) {
            char *p = strchr(key, ':');
            if (p) config_gamma_min = strtod(p + 1, NULL);
        } else if (strncmp(key, "\"gamma_max\"", 11) == 0) {
            char *p = strchr(key, ':');
            if (p) config_gamma_max = strtod(p + 1, NULL);
        } else if (strncmp(key, "\"brightness_min\"", 16) == 0 || strncmp(key, "\"bright_min\"", 12) == 0) {
            char *p = strchr(key, ':');
            if (p) config_bright_min = strtod(p + 1, NULL);
        } else if (strncmp(key, "\"brightness_max\"", 16) == 0 || strncmp(key, "\"bright_max\"", 12) == 0) {
            char *p = strchr(key, ':');
            if (p) config_bright_max = strtod(p + 1, NULL);
        } else if (strncmp(key, "\"selected_color\"", 16) == 0) {
            char *p = strchr(key, ':');
            if (p) config_sel_color = (int)strtol(p + 1, NULL, 10);
        } else if (strncmp(key, "\"unselected_color\"", 18) == 0) {
            char *p = strchr(key, ':');
            if (p) config_unsel_color = (int)strtol(p + 1, NULL, 10);
        }
    }
    fclose(f);
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

int main(int argc, char **argv) {
    load_config_limits(argc > 0 ? argv[0] : NULL);
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

    double gamma = clamp_double(1.0, config_gamma_min, config_gamma_max);
    double bright = clamp_double(1.0, config_bright_min, config_bright_max);
    int selected = 0;
    int rows, cols;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) init_colors_safe(config_sel_color, config_unsel_color);
    getmaxyx(stdscr, rows, cols);
    if (rows < 5 || cols < 10) {
        endwin();
        fprintf(stderr, "Terminal too small.\n");
        return 1;
    }

    WINDOW *win = newwin(rows - 2, cols - 2, 1, 1);
    nodelay(stdscr, FALSE);
    draw_ui(win, gamma, bright, selected, rows - 2, cols - 2);

    int ch;
    bool running = true;
    while (running && (ch = getch()) != ERR) {
        bool needs_redraw = false;
        bool needs_apply = false;
        switch (ch) {
            case 'q':
            case 'Q':
                running = false;
                break;
            case 'w':
            case 'W':
            case 's':
            case 'S':
                selected = 1 - selected;
                needs_redraw = true;
                break;
            case '/': {
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
                        if (val >= config_gamma_min && val <= config_gamma_max) {
                            gamma = val;
                            needs_apply = true;
                        }
                    } else {
                        if (val >= config_bright_min && val <= config_bright_max) {
                            bright = val;
                            needs_apply = true;
                        }
                    }
                }
                needs_redraw = true;
                break;
            }
            case 'r':
            case 'R':
                gamma = clamp_double(1.0, config_gamma_min, config_gamma_max);
                bright = clamp_double(1.0, config_bright_min, config_bright_max);
                apply_values(gamma, bright);
                needs_redraw = true;
                break;
            case KEY_RESIZE:
                handle_resize(&win, &rows, &cols);
                needs_redraw = true;
                break;
        }

        if (needs_apply && debounce_allow()) {
            apply_values(gamma, bright);
        }
        if (needs_redraw) {
            draw_ui(win, gamma, bright, selected, rows - 2, cols - 2);
        }
    }
    delwin(win);
    endwin();
    return 0;
}
