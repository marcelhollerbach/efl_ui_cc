#include "Efl_Ui_Format.h"
#include "Internal.h"
#include "abstract_tree_private.h"

typedef void (*Value_Transform)(const Eolian_Type *etype, Eina_Strbuf *buf, const char *value);


static void _free_property_values(Outputter_Property_Value *value);

struct _Outputter_Node {
   Value_Transform value_transform;
   const Eolian_Class *klass;
   Eolian_State *s;
   Efl_Ui_Node *node;
   Eina_Array *children;
   Eina_Array *properties;
};

typedef struct {
   Outputter_Property prop;
   Efl_Ui_Property *real_prop;
   Eina_Array *values;
} Inner_Outputter_Property;

typedef struct {
   Outputter_Property_Value value;
   Efl_Ui_Property_Value *pvalue;
   const Eolian_Type *type;
} Inner_Outputter_Property_Value;

typedef struct {
   Outputter_Struct str;
   Eina_Array *values;
} Inner_Outputter_Struct;

const Eolian_Class*
outputter_node_klass_get(Outputter_Node *node)
{
   return node->klass;
}

const char*
outputter_node_id_get(Outputter_Node *node)
{
   return node->node->id;
}

static Outputter_Node*
create_outputter_node(Eolian_State *s, Efl_Ui_Node *content)
{
   Outputter_Node *node = calloc(1, sizeof(Outputter_Node));
   EINA_SAFETY_ON_NULL_RETURN_VAL(s, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(content, NULL);
   node->klass = find_klass(s, content->type);
   if (!node->klass)
     EINA_SAFETY_ON_NULL_RETURN_VAL(node->klass, NULL);
   node->node = content;
   node->s = s;
   return node;
}

static Inner_Outputter_Property_Value*
_handle_property_value(Eolian_State *s, Value_Transform value_transform, Efl_Ui_Property_Value *val, const char *context_argument, const Eolian_Type *type)
{
   Inner_Outputter_Property_Value *value = calloc(1, sizeof(Inner_Outputter_Property_Value));
   value->pvalue = val;
   value->value.argument = context_argument; //eolian_parameter_name_get(parameter);
   value->value.type = type; //eolian_parameter_type_get(parameter);
   if (val->type == EFL_UI_PROPERTY_VALUE_TYPE_NODE)
     {
        value->value.node_type = EFL_UI_PROPERTY_VALUE_TYPE_NODE;
        value->value.object = create_outputter_node(s, val->node);
        value->value.object->value_transform = value_transform;
     }
   else if (val->type == EFL_UI_PROPERTY_VALUE_TYPE_STRUCT)
     {
        Inner_Outputter_Struct *str = calloc(1, sizeof(Inner_Outputter_Struct));

        str->values = eina_array_new(10);
        str->str.decl = eolian_type_typedecl_get(type);

        {
           Eolian_Struct_Type_Field *field;
           Eina_Iterator *iter = eolian_typedecl_struct_fields_get(str->str.decl);
           int i = 0;

           EINA_ITERATOR_FOREACH(iter, field)
             {
                Efl_Ui_Property_Value *v = eina_array_data_get(val->str->fields, i);
                Inner_Outputter_Property_Value *value = _handle_property_value(s, value_transform, v, eolian_typedecl_struct_field_name_get(field), eolian_typedecl_struct_field_type_get(field));
                eina_array_push(str->values, value);
                i ++;
             }
           eina_iterator_free(iter);
           str->str.values = eina_array_iterator_new(str->values);
        }
        value->value.node_type = EFL_UI_PROPERTY_VALUE_TYPE_STRUCT;
        value->value.str = (Outputter_Struct*)str;
     }
   else if (val->type == EFL_UI_PROPERTY_VALUE_TYPE_VALUE)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        value->value.node_type = EFL_UI_PROPERTY_VALUE_TYPE_VALUE;
        value_transform(value->value.type, buf, val->value);
        if (eina_strbuf_length_get(buf) == 0)
          eina_strbuf_append(buf, val->value);
        value->value.value = eina_strbuf_release(buf);
     }
   return value;
}

static void
_outputter_properties_values_fill(Outputter_Node *node, Inner_Outputter_Property *iprop, Efl_Ui_Property *prop, Eina_Iterator *parameters)
{
   Eolian_Function_Parameter *parameter;
   int i = 0;

   iprop->values = eina_array_new(10);
   EINA_ITERATOR_FOREACH(parameters, parameter)
     {
        Efl_Ui_Property_Value *val = eina_array_data_get(prop->value, i);
        Inner_Outputter_Property_Value *value = _handle_property_value(node->s, node->value_transform, val,
                                                                        eolian_parameter_name_get(parameter),
                                                                        eolian_parameter_type_get(parameter));
        eina_array_push(iprop->values, value);
        i ++;
     }
   eina_iterator_free(parameters);
}

Eina_Iterator*
outputter_properties_get(Outputter_Node *node)
{
   Eina_Iterator *properties;
   Efl_Ui_Property *prop;


   node->properties = eina_array_new(10);
   properties = eina_array_iterator_new(node->node->properties);
   EINA_ITERATOR_FOREACH(properties, prop)
     {
        Inner_Outputter_Property *oprop = calloc(1, sizeof(Inner_Outputter_Property));
        oprop->real_prop = prop;
        oprop->prop.property = find_function(node->s, node->klass, prop->key);
        _outputter_properties_values_fill(node, oprop, prop, eolian_property_values_get(oprop->prop.property, EOLIAN_PROP_SET));
        oprop->prop.values = eina_array_iterator_new(oprop->values);
        eina_array_push(node->properties, oprop);
     }
   eina_iterator_free(properties);
   return eina_array_iterator_new(node->properties);
}

enum Efl_Ui_Node_Children_Type
outputter_node_available_types_get(Outputter_Node *node)
{
   enum Efl_Ui_Node_Children_Type type = 0;
   if (eina_array_count(node->node->children_part))
     type |= EFL_UI_NODE_CHILDREN_TYPE_PACK;
   if (eina_array_count(node->node->children_linear))
     type |= EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR;
   if (eina_array_count(node->node->children_table))
     type |= EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE;

   return type;
}

Eina_Iterator*
outputter_children_get(Outputter_Node *node, enum Efl_Ui_Node_Children_Type preference)
{
   Eina_Iterator *children;
   void *c;

   node->children = eina_array_new(10);
   if (preference & EFL_UI_NODE_CHILDREN_TYPE_PACK)
     {
        children = eina_array_iterator_new(node->node->children_part);
        EINA_ITERATOR_FOREACH(children, c)
          {
             Outputter_Child *ochild = calloc(1, sizeof(Outputter_Child));

             Efl_Ui_Pack_Pack *pack = c;
             ochild->pack.pack = pack->part_name;
             ochild->type = EFL_UI_NODE_CHILDREN_TYPE_PACK;
             ochild->child = create_outputter_node(node->s, pack->basic.node);
             ochild->child->value_transform = node->value_transform;

             eina_array_push(node->children, ochild);
         }
       eina_iterator_free(children);
     }

   if (preference & EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE)
     {
        children = eina_array_iterator_new(node->node->children_table);
        EINA_ITERATOR_FOREACH(children, c)
          {
             Outputter_Child *ochild = calloc(1, sizeof(Outputter_Child));

             Efl_Ui_Pack_Table *table = c;
             ochild->type = EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE;
             ochild->table.x = atoi(table->x);
             ochild->table.y = atoi(table->y);
             ochild->table.w = atoi(table->w);
             ochild->table.h = atoi(table->h);
             ochild->child = create_outputter_node(node->s, table->basic.node);
             ochild->child->value_transform = node->value_transform;

             eina_array_push(node->children, ochild);
         }
       eina_iterator_free(children);
     }

   if (preference & EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR)
     {
        children = eina_array_iterator_new(node->node->children_linear);
        EINA_ITERATOR_FOREACH(children, c)
          {
             Outputter_Child *ochild = calloc(1, sizeof(Outputter_Child));

             Efl_Ui_Pack_Linear *linear = c;
             ochild->type = EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR;
             ochild->child = create_outputter_node(node->s,linear->basic.node);
             ochild->child->value_transform = node->value_transform;

             eina_array_push(node->children, ochild);
          }
       eina_iterator_free(children);
     }

   return eina_array_iterator_new(node->children);
}

Outputter_Node*
outputter_node_init(Eolian_State *s, Efl_Ui* ui, const char **name, void (*value_transform)(const Eolian_Type *etype, Eina_Strbuf *buf, const char *value))
{
   Outputter_Node *n;

   EINA_SAFETY_ON_NULL_RETURN_VAL(s, NULL);

   n = create_outputter_node(s, ui->content);
   n->value_transform = value_transform;
   n->s = s;
   *name = ui->name;

   return n;
}

static void
_outputter_struct_free(Inner_Outputter_Struct *str)
{
   while (eina_array_count(str->values) > 0)
     {
        Outputter_Property_Value *value = eina_array_pop(str->values);
        _free_property_values(value);
     }
   eina_array_free(str->values);
   free(str);
}

static void
_outputter_node_free(Outputter_Node *node)
{
   while (node->children && eina_array_count(node->children))
     {
        Outputter_Child *child;

        child = eina_array_pop(node->children);
        _outputter_node_free(child->child);
        free(child);
     }
   eina_array_free(node->children);
   while (node->properties && eina_array_count(node->properties))
     {
        Inner_Outputter_Property *prop = eina_array_pop(node->properties);

        while(eina_array_count(prop->values))
          {
            Outputter_Property_Value *value = eina_array_pop(prop->values);
            _free_property_values(value);
          }
        eina_array_free(prop->values);
        free(prop);
     }
   eina_array_free(node->properties);
   free(node);
}

static void
_free_property_values(Outputter_Property_Value *value)
{
   if (value->node_type == EFL_UI_PROPERTY_VALUE_TYPE_NODE)
     {
        _outputter_node_free(value->object);
     }
   else if (value->node_type == EFL_UI_PROPERTY_VALUE_TYPE_STRUCT)
     {
        _outputter_struct_free((Inner_Outputter_Struct*)value->str);
     }
   else if (value->node_type == EFL_UI_PROPERTY_VALUE_TYPE_VALUE)
     {
        if (value->value)
          free((char*)value->value);
     }
   free(value);
}

void
outputter_node_root_free(Outputter_Node* node)
{
   _outputter_node_free(node);
}

Efl_Ui_Node*
outputter_node_get(Outputter_Node *node)
{
   return node->node;
}

Efl_Ui_Property*
outputter_property_property_get(Outputter_Property *prop)
{
   return ((Inner_Outputter_Property*)prop)->real_prop;
}

Efl_Ui_Property_Value*
outputter_property_value_value_get(Outputter_Property_Value *val)
{
   return ((Inner_Outputter_Property_Value*)val)->pvalue;
}

enum Efl_Ui_Node_Children_Type
outputter_node_possible_types_get(Outputter_Node *node)
{
   return fetch_usage(node->s, node->klass);
}
