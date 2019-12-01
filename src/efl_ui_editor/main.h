#ifndef MAIN_H
#define MAIN_H

#include <Efl_Ui.h>
#include <Efl_Ui_Format.h>
#include <Eolian.h>

extern Eolian_State *editor_state;

Efl_Ui_Widget* object_generator(Efl_Ui_Win *win, const Eolian_State *s, const Efl_Ui *ui);
char*          json_output(const Eolian_State *s, const Efl_Ui *ui);

#endif
