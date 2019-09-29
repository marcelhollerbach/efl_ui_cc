#ifndef MAIN_H
#define MAIN_H

#include <Eina.h>
#include <Efl_Ui_Format.h>

extern const char* input_content;
extern const char* c_header_content;
extern const char* c_file_content;
extern const char *output_h_file;

Eina_Bool c_output(Eolian_State *s, Efl_Ui *ui);
#endif
