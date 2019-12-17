#ifndef MAIN_H
#define MAIN_H

#include <Efl_Ui.h>
#include <Efl_Ui_Format.h>
#include <Eolian.h>

extern Eolian_State *editor_state;
extern Efl_Ui_Win *win;
extern Efl_Gfx_Entity *background;

Efl_Ui_Widget* object_generator(Efl_Ui_Win *win, const Eolian_State *s, const Efl_Ui *ui);
char*          json_output(const Eolian_State *s, const Efl_Ui *ui);
void           base_ui_init(Efl_Ui_Win *win);
void           base_ui_refresh(Efl_Ui *ui);

//APIs for refreshing the UI representation
void display_ui_init(Efl_Ui_Win *win);
void display_ui_refresh(Efl_Ui *ui);

//Controller APIs
void change_type(Efl_Ui_Node *node, const char *new_type);
void add_child(Efl_Ui_Node *node, const char *klass_name);
void del_child(Efl_Ui_Node *node, Efl_Ui_Node *child);
void add_property(Efl_Ui_Node *node, const char *property_name);
void del_property(Efl_Ui_Node *node, const char *prop_name);
void change_parameter_type(Efl_Ui_Property_Value *value, const char *v);
void change_id(Efl_Ui_Node *node, const char *new_id);
void file_set(const char *file);

//UIs for selecting Types and properties
Eina_Future* select_available_types(void);
Eina_Future* select_available_properties(Efl_Ui_Node *node);

//UI for selecting a parameter value
Eina_Future *change_value(Outputter_Property_Value *value, Eo *anchor_widget);
Eina_Future* change_name(Efl_Ui_Node *node, Eo *anchor_widget);

#endif
