#ifndef UI_TREE_H
#define UI_TREE_H

#include <abstract_tree.h>

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

#endif
