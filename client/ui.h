#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include "types.h"

void print_header();
void print_menu(ClientState *state);
void clear_screen();
void print_separator();
void print_error(const char *message);
void print_success(const char *message);
void print_info(const char *message);

#endif
