#include <Efl_Ui_Format.h>

typedef struct _Efl_Ui_Node Efl_Ui_Node;
typedef struct _Efl_Ui_Property Efl_Ui_Property;
typedef struct _Efl_Ui_Property_Value Efl_Ui_Property_Value;
typedef struct _Efl_Ui_Pack_Table Efl_Ui_Pack_Table;
typedef struct _Efl_Ui_Pack_Linear Efl_Ui_Pack_Linear;
typedef struct _Efl_Ui_Pack_Pack Efl_Ui_Pack_Pack;


Efl_Ui_Property* node_property_append(Efl_Ui_Node *n);
void node_id_set(Efl_Ui_Node *n, const char *id);
void node_type_set(Efl_Ui_Node *n, const char *type);
void property_key_set(Efl_Ui_Property *prop, const char *key);
Efl_Ui_Property_Value* property_value_append(Efl_Ui_Property *prop);
Efl_Ui_Node* property_value_node(Efl_Ui_Property_Value *val);
void property_value_value(Efl_Ui_Property_Value *val, const char *value);
Efl_Ui_Pack_Linear* node_pack_linear_node_append(Efl_Ui_Node *node);
Efl_Ui_Pack_Table* node_pack_table_node_append(Efl_Ui_Node *node);
Efl_Ui_Pack_Pack* node_pack_node_append(Efl_Ui_Node *node);
enum Efl_Ui_Node_Children_Type node_child_type_get(Efl_Ui_Node *node);
enum Efl_Ui_Node_Children_Type node_child_fetch_usage(Eolian_State *state, const char *klass);


void efl_ui_node_free(Efl_Ui_Node *node); //Be carefull does not cleanup in the parent
