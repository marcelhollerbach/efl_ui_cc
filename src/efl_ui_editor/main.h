#ifndef MAIN_H
#define MAIN_H

#include <Efl_Ui.h>
#include <Efl_Ui_Format.h>
#include <Eolian.h>

extern Eolian_State *editor_state;
extern Efl_Ui_Win *win;
extern Efl_Gfx_Entity *background;
extern Efl_Ui *ui_tree;

typedef struct {
   Eina_Hash *node_widget, *widget_node;
} Object_Hash_Tuple;

Efl_Ui_Widget* object_generator(Efl_Ui_Win *win, const Eolian_State *s, const Efl_Ui *ui, Object_Hash_Tuple *tuple);
char*          json_output(const Eolian_State *s, const Efl_Ui *ui);
void           base_ui_init(Efl_Ui_Win *win);
void           base_ui_refresh(Efl_Ui *ui);
void           highlight_node(Efl_Ui_Node *node);

//APIs for refreshing the UI representation
void display_ui_init(Efl_Ui_Win *win);
void display_ui_refresh(Efl_Ui *ui);

//Controller APIs
void change_type(Efl_Ui_Node *node, const char *new_type);
void add_child(Efl_Ui_Node *node, const char *klass_name, enum Efl_Ui_Node_Children_Type type);
void del_child(Efl_Ui_Node *node, Efl_Ui_Node *child);
void add_property(Efl_Ui_Node *node, const char *property_name);
void del_property(Efl_Ui_Node *node, const char *prop_name);
void change_parameter_type(Efl_Ui_Property_Value *value, const char *v);
void change_id(Efl_Ui_Node *node, const char *new_id);
void file_set(const char *input_file, const char *output_file);
void safe_file(void);
void change_linear_details(Efl_Ui_Node *n, const char *new_details);
void change_table_details(Efl_Ui_Node *n, const char *new_details);
void change_part_details(Efl_Ui_Node *n, const char *new_details);

//UIs for selecting Types and properties
Eina_Future* select_available_types(void);
Eina_Future* select_available_properties(Efl_Ui_Node *node);

//UI for selecting a parameter value
Eina_Future *change_value(Outputter_Property_Value *value, Eo *anchor_widget);
Eina_Future* change_name(Efl_Ui_Node *node, Eo *anchor_widget);

Eina_Future* linear_change_ui(Efl_Ui_Node *node, Eo *anchor_widget);
Eina_Future* table_change_ui(Efl_Ui_Node *node, Eo *anchor_widget);
Eina_Future* part_change_ui(Efl_Ui_Node *node, Eo *anchor_widget);

#endif
