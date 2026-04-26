#ifndef MENU_H
#define MENU_H

#include <stddef.h>

void outputMenu(void);
int readMenuChoice(int *choice);
int readChoiceInput(const char *prompt, int *choice);
int readTextInput(const char *prompt, char *buffer, size_t size);
void showMenuInputFormatError(void);
void dispatchMenuChoice(int choice);

#endif
