#ifndef EFL_UI_FORMAT_H
#define EFL_UI_FORMAT_H 1

#include <Eina.h>
#include <Eolian.h>

typedef struct _Efl_Ui Efl_Ui;

Efl_Ui* efl_ui_format_parse(const char *input_content);
Eina_Bool validate(Eolian_State *state, Efl_Ui *ui);
void efl_ui_free(Efl_Ui *ui);

typedef struct _Outputter_Node Outputter_Node;

typedef struct {
  Eina_Bool simple;
  union {
    Outputter_Node *object;
    const char *value;
  };
} Outputter_Property_Value;

typedef struct {
  const Eolian_Function *property;
  Eina_Iterator *values;
} Outputter_Property;

enum Efl_Ui_Node_Children_Type {
  EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR = 1,
  EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE = 2,
  EFL_UI_NODE_CHILDREN_TYPE_PACK = 3,
  EFL_UI_NODE_CHILDREN_TYPE_NOTHING = 0,
};

typedef struct {
  Outputter_Node *child;
  enum Efl_Ui_Node_Children_Type type;
  union {
    struct {
      int x,y,w,h;
    } table;
    struct {

    } linear;
    struct {
      const char *pack;
    } pack;
  };
} Outputter_Child;

Eina_Iterator *outputter_properties_get(Outputter_Node *node);
Eina_Iterator *outputter_children_get(Outputter_Node *node);
const Eolian_Class* outputter_node_klass_get(Outputter_Node *node);
const char* outputter_node_id_get(Outputter_Node *node);
Outputter_Node* outputter_node_init(Eolian_State *s, Efl_Ui* ui, const char **name);
void outputter_node_root_free(Outputter_Node* node);

#endif
