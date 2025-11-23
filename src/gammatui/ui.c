#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include "gammatui.h"

enum {
    COLOR_PAIR_DEFAULT = 1,
    COLOR_PAIR_TITLE,
    COLOR_PAIR_HEADER,
    COLOR_PAIR_LABEL,
    COLOR_PAIR_VALUE,
    COLOR_PAIR_VALUE_SELECTED,
    COLOR_PAIR_BAR_FILL,
    COLOR_PAIR_BAR_EMPTY,
    COLOR_PAIR_HELP
};

void init_colors_safe(int sel, int unsel) {
    if (!has_colors()) return;
    start_color();
    use_default_colors();
    init_pair(COLOR_PAIR_DEFAULT, COLOR_WHITE, -1);
    init_pair(COLOR_PAIR_TITLE, COLOR_CYAN, -1);
    init_pair(COLOR_PAIR_HEADER, COLOR_YELLOW, -1);
    
    init_pair(COLOR_PAIR_LABEL, (short)unsel, -1);
    init_pair(COLOR_PAIR_VALUE, (short)unsel, -1);
    init_pair(COLOR_PAIR_VALUE_SELECTED, (short)sel, -1);
    
    init_pair(COLOR_PAIR_BAR_FILL, COLOR_GREEN, -1);
    init_pair(COLOR_PAIR_BAR_EMPTY, COLOR_WHITE, -1);
    init_pair(COLOR_PAIR_HELP, COLOR_BLUE, -1);
}

void draw_rounded_border(WINDOW *w) {
    wborder(w, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
}

void draw_ui(WINDOW *win, double gamma, double bright, int selected, int rows, int cols,
             int k_up, int k_down, int k_sel, int k_quit) {
    werase(win);
    draw_rounded_border(win);

    const char *title = "Gamma & Brightness Control";
    int title_x = (cols - (int)strlen(title)) / 2;
    wattron(win, COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
    mvwprintw(win, 1, title_x, "%s", title);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);

    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER));
    char output_buf[256];
    snprintf(output_buf, sizeof(output_buf), "Display Output: %s", display_output[0] ? display_output : "(not detected)");
    mvwprintw(win, 2, (cols - (int)strlen(output_buf)) / 2, "%s", output_buf);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER));

    mvwaddch(win, 3, 0, ACS_LTEE);
    mvwhline(win, 3, 1, ACS_HLINE, cols - 2);
    mvwaddch(win, 3, cols - 1, ACS_RTEE);

    int left_margin = 4;
    int control_y_start = 5;

    int gamma_y = control_y_start;
    wattron(win, COLOR_PAIR(selected == 0 ? COLOR_PAIR_VALUE_SELECTED : COLOR_PAIR_LABEL) | A_BOLD);
    mvwprintw(win, gamma_y, left_margin, "Gamma");
    wattroff(win, COLOR_PAIR(selected == 0 ? COLOR_PAIR_VALUE_SELECTED : COLOR_PAIR_LABEL) | A_BOLD);
    char gamma_val_str[10];
    snprintf(gamma_val_str, sizeof(gamma_val_str), "%.2f", gamma);
    wattron(win, COLOR_PAIR(selected == 0 ? COLOR_PAIR_VALUE_SELECTED : COLOR_PAIR_VALUE) | A_BOLD);
    mvwprintw(win, gamma_y, cols - left_margin - (int)strlen(gamma_val_str), "%s", gamma_val_str);
    wattroff(win, COLOR_PAIR(selected == 0 ? COLOR_PAIR_VALUE_SELECTED : COLOR_PAIR_VALUE) | A_BOLD);

    int bright_y = gamma_y + 2;
    wattron(win, COLOR_PAIR(selected == 1 ? COLOR_PAIR_VALUE_SELECTED : COLOR_PAIR_LABEL) | A_BOLD);
    mvwprintw(win, bright_y, left_margin, "Brightness");
    wattroff(win, COLOR_PAIR(selected == 1 ? COLOR_PAIR_VALUE_SELECTED : COLOR_PAIR_LABEL) | A_BOLD);
    char bright_val_str[10];
    snprintf(bright_val_str, sizeof(bright_val_str), "%.2f", bright);
    wattron(win, COLOR_PAIR(selected == 1 ? COLOR_PAIR_VALUE_SELECTED : COLOR_PAIR_VALUE) | A_BOLD);
    mvwprintw(win, bright_y, cols - left_margin - (int)strlen(bright_val_str), "%s", bright_val_str);
    wattroff(win, COLOR_PAIR(selected == 1 ? COLOR_PAIR_VALUE_SELECTED : COLOR_PAIR_VALUE) | A_BOLD);

    mvwaddch(win, rows - 3, 0, ACS_LTEE);
    mvwhline(win, rows - 3, 1, ACS_HLINE, cols - 2);
    mvwaddch(win, rows - 3, cols - 1, ACS_RTEE);

    const char *kn_up = keyname(k_up); if(!kn_up) kn_up="?";
    const char *kn_dn = keyname(k_down); if(!kn_dn) kn_dn="?";
    const char *kn_sl = keyname(k_sel); if(!kn_sl) kn_sl="?";
    const char *kn_qt = keyname(k_quit); if(!kn_qt) kn_qt="?";

    char help_text[256];
    snprintf(help_text, sizeof(help_text), 
             "[%s/%s] Select [%s] Set Value [R] Reset [%s] Quit", 
             kn_up, kn_dn, kn_sl, kn_qt);

    int help_x = (cols - (int)strlen(help_text)) / 2;
    if(help_x < 1) help_x = 1;

    wattron(win, COLOR_PAIR(COLOR_PAIR_HELP));
    mvwprintw(win, rows - 2, help_x, "%s", help_text);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HELP));

    wnoutrefresh(win);
    doupdate();
}
