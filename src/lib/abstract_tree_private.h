#ifndef UI_TREE_H
#define UI_TREE_H

#include <Efl_Ui_Format.h>
#include <abstract_tree.h>


struct _Efl_Ui_Node {
   const char *id;   //Id of the name of this node
   const char *type; //String of the Eolian type
   Eina_Array *properties; //array of Node_Properties
   Eina_Array *children_linear; //array of Efl_Ui_Pack_Linear
   Eina_Array *children_table; //array of Efl_Ui_Pack_Table
   Eina_Array *children_part; //array of Efl_Ui_Pack_Pack
};

struct _Efl_Ui_Property {
   const char *key;   //the key this property is used with
   Eina_Array *value; //array of Efl_Ui_Property_Value
};

struct _Efl_Ui_Property_Value {
   enum Efl_Ui_Property_Value_Type type;
   union {
      Efl_Ui_Node *node;
      Efl_Ui_Struct *str;
      const char *value;
   };
};

struct _Efl_Ui_Pack_Basic {
   Efl_Ui_Node *node;
};

struct _Efl_Ui_Pack_Table {
   Efl_Ui_Pack_Basic basic;
   const char *x;
   const char *y;
   const char *w;
   const char *h;
};

struct _Efl_Ui_Pack_Linear {
   Efl_Ui_Pack_Basic basic;
};

struct _Efl_Ui_Pack_Pack {
   Efl_Ui_Pack_Basic basic;
   const char *part_name;
};

struct _Efl_Ui {
   const char *name;
   Efl_Ui_Node *content;
};

struct _Efl_Ui_Struct {
   Eina_Array *fields; //these are property values
};

#endif
