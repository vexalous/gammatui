#ifndef GAMMATUI_H
#define GAMMATUI_H

#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stddef.h>
#include <ncurses.h>

int detect_output(char *outbuf, size_t outlen);
void xr_call_async(const char *output, const char *opt, const char *val);
int debounce_allow(void);
double clamp_double(double v, double lo, double hi);

void init_colors_safe(int sr, int sg, int sb, int ur, int ug, int ub);
void draw_bar(WINDOW *w, int y, int x, int width, double value, double lo, double hi);
void draw_rounded_border(WINDOW *w);

void draw_ui(WINDOW *win, double gamma, double bright, int selected, int rows, int cols, 
             int k_up, int k_down, int k_sel, int k_quit);

void apply_values(double gamma, double bright);
void revert_values(void);

extern char display_output[128];
