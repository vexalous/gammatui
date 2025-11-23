#ifndef MENU_UI_H
#define MENU_UI_H
#include <ncurses.h>

extern const char *MENU_TITLE;
extern const char *menu_items[];
enum { MI_Adjustment, MI_Settings, MI_Quit };

void draw_menu(WINDOW *w, int highlight, int k_up, int k_down, int k_sel, int k_quit);
void show_message(WINDOW *w,const char *title,const char *msg);
WINDOW *recreate_window(int *rows,int *cols);

#endif
