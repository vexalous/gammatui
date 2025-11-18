#define _POSIX_C_SOURCE 200809L
#include "gammatui.h"
#include "gamma_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ncurses.h>

#define GAMMA_STEP 0.02
#define BRIGHT_STEP 0.01
#define GAMMA_MIN 0.5
#define GAMMA_MAX 3.0
#define BRIGHT_MIN 0.1
#define BRIGHT_MAX 2.0

static void handle_resize(WINDOW **win, int *rows, int *cols) {
    int new_rows, new_cols;
    getmaxyx(stdscr, new_rows, new_cols);
    
    if (new_rows != *rows || new_cols != *cols) {
        *rows = new_rows;
        *cols = new_cols;
        wresize(*win, *rows - 2, *cols - 2);
        mvwin(*win, 1, 1);
        clear(); 
        refresh();
    }
}

static void update_settings(int selected, double *gamma, double *bright, int direction) {
    if (selected == 0) {
        *gamma = clamp_double(*gamma + (direction * GAMMA_STEP), GAMMA_MIN, GAMMA_MAX);
    } else {
        *bright = clamp_double(*bright + (direction * BRIGHT_STEP), BRIGHT_MIN, BRIGHT_MAX);
    }
}

int main(int argc, char **argv) {
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

    double gamma = 1.0;
    double bright = 1.0;
    int selected = 0; 
    int rows, cols;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) init_colors_safe();

    getmaxyx(stdscr, rows, cols);
    WINDOW *win = newwin(rows - 2, cols - 2, 1, 1);
    nodelay(stdscr, FALSE);

    draw_ui(win, gamma, bright, selected, rows - 2, cols - 2);

    int ch;
    bool running = true;

    while (running && (ch = getch())) {
        bool needs_redraw = false;
        bool needs_apply = false;

        switch (ch) {
            case 'q':
            case 'Q':
                running = false;
                break;

            case KEY_UP:
            case KEY_DOWN:
                selected = 1 - selected;
                needs_redraw = true;
                break;

            case KEY_LEFT:
                update_settings(selected, &gamma, &bright, -1);
                needs_apply = true;
                needs_redraw = true;
                break;

            case KEY_RIGHT:
                update_settings(selected, &gamma, &bright, 1);
                needs_apply = true;
                needs_redraw = true;
                break;

            case 'r':
            case 'R':
                revert_values();
                gamma = 1.0;
                bright = 1.0;
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
