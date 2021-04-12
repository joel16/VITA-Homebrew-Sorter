/*
 * Copyright © 2015 Sergi Granell (xerpi)
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <psp2/types.h>
#include "draw.h"

void console_init();
void console_exit();
void console_reset();
void console_putc(char c);
void console_print(const char *s);
void console_printf(const char *s, ...);
void console_set_color(uint32_t color);
int console_get_y();
void console_set_y(int new_y);
void console_set_top_margin(int new_top_margin);

#define printf(...) console_printf(__VA_ARGS__)

#endif
