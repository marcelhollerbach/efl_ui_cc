#ifndef EFL_UI_FORMAT_H
#define EFL_UI_FORMAT_H 1

typedef struct _Efl_Ui Efl_Ui;

#include <Eina.h>
#include <Eolian.h>
#include <abstract_tree.h>


Efl_Ui* efl_ui_format_parse(const char *input_content);
Eina_Bool validate(Eolian_State *state, Efl_Ui *ui);
void efl_ui_free(Efl_Ui *ui);

typedef struct _Outputter_Node Outputter_Node;
typedef struct _Outputter_Struct Outputter_Struct;

typedef struct {
  const Eolian_Type *type;
  enum Efl_Ui_Property_Value_Type node_type;
  union {
    Outputter_Node *object;
    Outputter_Struct *str;
    const char *value;
  };
  const char *argument;
} Outputter_Property_Value;

struct _Outputter_Struct {
  const Eolian_Typedecl *decl;
  Eina_Iterator *values;
};

typedef struct {
  const Eolian_Function *property;
  Eina_Iterator *values;
} Outputter_Property;

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

void eolian_bridge_beta_allowed_set(Eina_Bool beta_awareness);

Eina_Iterator *outputter_properties_get(Outputter_Node *node);
Eina_Iterator *outputter_children_get(Outputter_Node *node, enum Efl_Ui_Node_Children_Type preference);
const Eolian_Class* outputter_node_klass_get(Outputter_Node *node);
const char* outputter_node_id_get(Outputter_Node *node);
Outputter_Node* outputter_node_init(Eolian_State *s, Efl_Ui* ui, const char **name, void (*value_transform)(const Eolian_Type *etype, Eina_Strbuf *buf, const char *value));
void outputter_node_root_free(Outputter_Node* node);
enum Efl_Ui_Node_Children_Type outputter_node_available_types_get(Outputter_Node *node);
enum Efl_Ui_Node_Children_Type outputter_node_possible_types_get(Outputter_Node *node);

Efl_Ui_Node *outputter_node_get(Outputter_Node *node);
Efl_Ui_Property* outputter_property_property_get(Outputter_Property *prop);
Efl_Ui_Property_Value* outputter_property_value_value_get(Outputter_Property_Value *val);

Efl_Ui* efl_ui_new(void);
void efl_ui_name_set(Efl_Ui *ui, const char *name);
Efl_Ui_Node* efl_ui_content_get(Efl_Ui *ui);

void fetch_real_typedecl(const Eolian_Typedecl **decl, const Eolian_Type **type);

#endif
