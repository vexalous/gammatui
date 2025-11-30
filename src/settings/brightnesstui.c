#define _POSIX_C_SOURCE 200809L
#include "settings.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stddef.h>

static const char *TITLE = "Gammatui - Settings";
static const char *UI_FIELDS[] = {
    "Output",
    "Gamma Min",
    "Gamma Max",
    "Bright Min",
    "Bright Max",
    "Sel Red",
    "Sel Green",
    "Sel Blue",
    "Unsel Red",
    "Unsel Green",
    "Unsel Blue",
    "Key Up",
    "Key Down",
    "Key Select",
    "Key Quit",
    "Save Config"
};

#define UI_START_ROW 5
#define UI_LABEL_COL 4
#define UI_VALUE_COL 20
#define UI_ROW_STEP 1

enum {
    FI_OUTPUT = 0,
    FI_GAMMA_MIN,
    FI_GAMMA_MAX,
    FI_BRIGHT_MIN,
    FI_BRIGHT_MAX,
    FI_SEL_R,
    FI_SEL_G,
    FI_SEL_B,
    FI_UNSEL_R,
    FI_UNSEL_G,
    FI_UNSEL_B,
    FI_KEY_UP,
    FI_KEY_DOWN,
    FI_KEY_SELECT,
    FI_KEY_QUIT,
    FI_SAVE,
    FI_COUNT
};

static void show_message(WINDOW *w, const char *title, const char *msg) {
    werase(w);
    box(w, 0, 0);
    mvwprintw(w, 1, 2, "%s", title);
    mvwprintw(w, 3, 2, "%s", msg);
    mvwprintw(w, 5, 2, "Press any key to return");
    wrefresh(w);
    wgetch(w);
}

static void string_insert_char(char *str, int pos, int max_len, char ch) {
    int len = (int)strlen(str);
    if (len >= max_len - 1) return;
    for (int i = len; i > pos; i--) str[i] = str[i-1];
    str[pos] = ch;
    str[len + 1] = '\0';
}

static void string_delete_char(char *str, int pos) {
    int len = (int)strlen(str);
    if (pos >= len) return;
    for (int i = pos; i < len; i++) str[i] = str[i+1];
}

static void edit_string(WINDOW *w, int row, int col, char *buf, size_t bufsz) {
    char scratch[OUTPUT_LEN + 1];
    if (bufsz > sizeof(scratch)) bufsz = sizeof(scratch);
    
    memset(scratch, 0, sizeof(scratch));
    int pos = 0;
    int ch;
    bool editing = true;

    curs_set(1);
    noecho();
    keypad(w, TRUE);

    while (editing) {
        mvwprintw(w, row, col, "%-*s", (int)bufsz - 1, " ");
        mvwprintw(w, row, col, "%s", scratch);
        wmove(w, row, col + pos);
        wrefresh(w);

        ch = wgetch(w);
        switch (ch) {
            case '\n':
            case KEY_ENTER:
                snprintf(buf, bufsz, "%s", scratch);
                editing = false;
                break;
            case 27: editing = false; break;
            case KEY_BACKSPACE:
            case 127:
            case 8:
                if (pos > 0) {
                    string_delete_char(scratch, pos - 1);
                    pos--;
                }
                break;
            case KEY_DC:
                if (pos < (int)strlen(scratch)) string_delete_char(scratch, pos);
                break;
            case KEY_LEFT:
                if (pos > 0) pos--;
                break;
            case KEY_RIGHT:
                if (pos < (int)strlen(scratch)) pos++;
                break;
            case KEY_HOME: pos = 0; break;
            case KEY_END: pos = (int)strlen(scratch); break;
            default:
                if (isprint(ch)) {
                    if (strlen(scratch) < bufsz - 1) {
                        string_insert_char(scratch, pos, (int)bufsz, (char)ch);
                        pos++;
                    }
                }
                break;
        }
    }
    curs_set(0);
}

static void edit_double(WINDOW *w, int row, int col, double *val) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%.3f", *val);
    edit_string(w, row, col, tmp, sizeof(tmp));
    char *endptr;
    double v = strtod(tmp, &endptr);
    if (endptr != tmp && *endptr == '\0') *val = v;
}

static void edit_int(WINDOW *w, int row, int col, int *val) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%d", *val);
    edit_string(w, row, col, tmp, sizeof(tmp));
    char *endptr;
    long v = strtol(tmp, &endptr, 10);
    if (endptr != tmp && *endptr == '\0') {
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        *val = (int)v;
    }
}

static int normalize_key(int k) {
    if (k >= 'A' && k <= 'Z') return k + 32;
    return k;
}

static const char* get_action_name_for_key(const struct cfg *c, int key) {
    int nk = normalize_key(key);
    if (nk == normalize_key(c->key_up)) return "Move Up";
    if (nk == normalize_key(c->key_down)) return "Move Down";
    if (nk == normalize_key(c->key_select)) return "Select";
    if (nk == normalize_key(c->key_quit)) return "Quit";
    return NULL;
}

static void edit_key(WINDOW *w, int row, int col, int *val, const struct cfg *full_cfg, int current_field) {
    curs_set(1);
    mvwprintw(w, row, col, "Press New Key...");
    wrefresh(w);
    int ch = wgetch(w);
    
    if (ch != ERR && ch != 27) {
        const char *conflict = get_action_name_for_key(full_cfg, ch);
        bool self_collision = false;
        if (conflict) {
            if (current_field == FI_KEY_UP && strcmp(conflict, "Move Up") == 0) self_collision = true;
            else if (current_field == FI_KEY_DOWN && strcmp(conflict, "Move Down") == 0) self_collision = true;
            else if (current_field == FI_KEY_SELECT && strcmp(conflict, "Select") == 0) self_collision = true;
            else if (current_field == FI_KEY_QUIT && strcmp(conflict, "Quit") == 0) self_collision = true;
        }

        if (conflict && !self_collision) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Key already set to: %s", conflict);
            show_message(w, "Error", buf);
        } else {
            *val = ch;
        }
    }
    curs_set(0);
}

static void draw_form(WINDOW *w, const struct cfg *c, int highlight) {
    werase(w);
    box(w, 0, 0);
    mvwprintw(w, 1, 2, "%s", TITLE);
    
    const char *k_u = keyname(c->key_up); if(!k_u) k_u="?";
    const char *k_d = keyname(c->key_down); if(!k_d) k_d="?";
    const char *k_s = keyname(c->key_select); if(!k_s) k_s="?";
    const char *k_q = keyname(c->key_quit); if(!k_q) k_q="?";

    char help[256];
    snprintf(help, sizeof(help), "Move:%s/%s | %s:Edit | %s:Quit", k_u, k_d, k_s, k_q);
    mvwprintw(w, 2, 2, "%s", help);

    for (int i = 0; i < FI_COUNT; ++i) {
        int current_row = UI_START_ROW + i * UI_ROW_STEP;
        if (i == highlight) wattron(w, A_REVERSE);
        
        if (i == FI_SAVE) mvwprintw(w, current_row, UI_LABEL_COL, "[ %s ]", UI_FIELDS[i]);
        else mvwprintw(w, current_row, UI_LABEL_COL, "%s:", UI_FIELDS[i]);
        
        wattroff(w, A_REVERSE);

        if (i == FI_OUTPUT) mvwprintw(w, current_row, UI_VALUE_COL, "%s", c->output[0] ? c->output : "(empty)");
        else if (i == FI_GAMMA_MIN) mvwprintw(w, current_row, UI_VALUE_COL, "%.3f", c->gamma_min);
        else if (i == FI_GAMMA_MAX) mvwprintw(w, current_row, UI_VALUE_COL, "%.3f", c->gamma_max);
        else if (i == FI_BRIGHT_MIN) mvwprintw(w, current_row, UI_VALUE_COL, "%.3f", c->bright_min);
        else if (i == FI_BRIGHT_MAX) mvwprintw(w, current_row, UI_VALUE_COL, "%.3f", c->bright_max);
        else if (i == FI_SEL_R) mvwprintw(w, current_row, UI_VALUE_COL, "%d", c->sel_r);
        else if (i == FI_SEL_G) mvwprintw(w, current_row, UI_VALUE_COL, "%d", c->sel_g);
        else if (i == FI_SEL_B) mvwprintw(w, current_row, UI_VALUE_COL, "%d", c->sel_b);
        else if (i == FI_UNSEL_R) mvwprintw(w, current_row, UI_VALUE_COL, "%d", c->unsel_r);
        else if (i == FI_UNSEL_G) mvwprintw(w, current_row, UI_VALUE_COL, "%d", c->unsel_g);
        else if (i == FI_UNSEL_B) mvwprintw(w, current_row, UI_VALUE_COL, "%d", c->unsel_b);
        else if (i == FI_KEY_UP) mvwprintw(w, current_row, UI_VALUE_COL, "%s", keyname(c->key_up));
        else if (i == FI_KEY_DOWN) mvwprintw(w, current_row, UI_VALUE_COL, "%s", keyname(c->key_down));
        else if (i == FI_KEY_SELECT) mvwprintw(w, current_row, UI_VALUE_COL, "%s", keyname(c->key_select));
        else if (i == FI_KEY_QUIT) mvwprintw(w, current_row, UI_VALUE_COL, "%s", keyname(c->key_quit));
    }
    wrefresh(w);
}

int main(int argc, char **argv) {
    char cfgpath[PATH_MAX];
    const char *exe_name = (argc > 0) ? argv[0] : "gammatui";
    if (!config_path_for_exe(cfgpath, sizeof(cfgpath), exe_name)) return 1;

    struct cfg cur;
    if (!load_config(&cur, cfgpath)) {
        config_set_defaults(&cur);
        save_config(&cur, cfgpath);
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    WINDOW *win = newwin(rows - 2, cols - 2, 1, 1);
    keypad(win, TRUE);

    int highlight = 0;
    bool running = true;
    bool dirty = true;
    while (running) {
        if (dirty) {
            draw_form(win, &cur, highlight);
            dirty = false;
        }
        int ch = wgetch(win);

        int check_ch = normalize_key(ch);
        int check_up = normalize_key(cur.key_up);
        int check_down = normalize_key(cur.key_down);
        int check_sel = normalize_key(cur.key_select);
        int check_quit = normalize_key(cur.key_quit);

        if (check_ch == check_up) {
             if (highlight > 0) { highlight--; dirty = true; }
        } else if (check_ch == check_down) {
             if (highlight < FI_COUNT - 1) { highlight++; dirty = true; }
        } else if (check_ch == check_quit) {
            running = false;
        } else if (ch == KEY_RESIZE) {
            getmaxyx(stdscr, rows, cols);
            wresize(win, rows - 2, cols - 2);
            mvwin(win, 1, 1);
            dirty = true;
        } else if (check_ch == check_sel || ch == KEY_ENTER) {
            if (check_ch != check_sel) continue;

            int row = UI_START_ROW + highlight * UI_ROW_STEP;
            if (highlight == FI_SAVE) {
                if (save_config(&cur, cfgpath)) show_message(win, "Saved", "Configuration saved successfully.");
                else show_message(win, "Error", "Failed to save configuration.");
            } else if (highlight == FI_OUTPUT) edit_string(win, row, UI_VALUE_COL, cur.output, sizeof(cur.output));
            else if (highlight == FI_GAMMA_MIN) edit_double(win, row, UI_VALUE_COL, &cur.gamma_min);
            else if (highlight == FI_GAMMA_MAX) edit_double(win, row, UI_VALUE_COL, &cur.gamma_max);
            else if (highlight == FI_BRIGHT_MIN) edit_double(win, row, UI_VALUE_COL, &cur.bright_min);
            else if (highlight == FI_BRIGHT_MAX) edit_double(win, row, UI_VALUE_COL, &cur.bright_max);
            else if (highlight == FI_SEL_R) edit_int(win, row, UI_VALUE_COL, &cur.sel_r);
            else if (highlight == FI_SEL_G) edit_int(win, row, UI_VALUE_COL, &cur.sel_g);
            else if (highlight == FI_SEL_B) edit_int(win, row, UI_VALUE_COL, &cur.sel_b);
            else if (highlight == FI_UNSEL_R) edit_int(win, row, UI_VALUE_COL, &cur.unsel_r);
            else if (highlight == FI_UNSEL_G) edit_int(win, row, UI_VALUE_COL, &cur.unsel_g);
            else if (highlight == FI_UNSEL_B) edit_int(win, row, UI_VALUE_COL, &cur.unsel_b);
            else if (highlight == FI_KEY_UP) edit_key(win, row, UI_VALUE_COL, &cur.key_up, &cur, FI_KEY_UP);
            else if (highlight == FI_KEY_DOWN) edit_key(win, row, UI_VALUE_COL, &cur.key_down, &cur, FI_KEY_DOWN);
            else if (highlight == FI_KEY_SELECT) edit_key(win, row, UI_VALUE_COL, &cur.key_select, &cur, FI_KEY_SELECT);
            else if (highlight == FI_KEY_QUIT) edit_key(win, row, UI_VALUE_COL, &cur.key_quit, &cur, FI_KEY_QUIT);
            dirty = true;
        }
    }
    delwin(win);
    endwin();
    return 0;
