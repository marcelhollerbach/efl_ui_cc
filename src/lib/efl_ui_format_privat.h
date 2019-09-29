#include <Efl_Ui_Format.h>

typedef struct _Efl_Ui_Node Efl_Ui_Node;
typedef struct _Efl_Ui_Property Efl_Ui_Property;
typedef struct _Efl_Ui_Property_Value Efl_Ui_Property_Value;
typedef struct _Efl_Ui_Pack_Table Efl_Ui_Pack_Table;
typedef struct _Efl_Ui_Pack_Linear Efl_Ui_Pack_Linear;
typedef struct _Efl_Ui_Pack_Pack Efl_Ui_Pack_Pack;


struct _Efl_Ui_Node {
   const char *id;   //Id of the name of this node
   const char *type; //String of the Eolian type
   Eina_Array *properties; //array of Node_Properties
   enum Efl_Ui_Node_Children_Type usage_type;
   Eina_Array *children;
};

struct _Efl_Ui_Property {
   const char *key;   //the key this property is used with
   Eina_Array *value; //array of Efl_Ui_Property_Value
};

struct _Efl_Ui_Property_Value {
   Eina_Bool is_node;
   union {
      Efl_Ui_Node *node;
      const char *value;
   };
};

struct _Efl_Ui_Pack_Table {
   Efl_Ui_Node *node;
   const char *x;
   const char *y;
   const char *w;
   const char *h;
};

struct _Efl_Ui_Pack_Linear {
   Efl_Ui_Node *node;
};

struct _Efl_Ui_Pack_Pack {
   Efl_Ui_Node *node;
   const char *part_name;
};

struct _Efl_Ui {
   const char *name;
   Efl_Ui_Node *content;
};

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
Efl_Ui* efl_ui_new(void);
void efl_ui_name_set(Efl_Ui *ui, const char *name);
Efl_Ui_Node* efl_ui_content_get(Efl_Ui *ui);

const Eolian_Class* find_klass(Eolian_State *s, const char *klass);
const Eolian_Function* find_function(Eolian_State *s, const Eolian_Class *klass, const char *prop);
