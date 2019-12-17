#include "Efl_Ui_Format.h"
#include "Internal.h"
#include "abstract_tree_private.h"

struct _Outputter_Node {
   void (*value_transform)(const Eolian_Type *etype, Eina_Strbuf *buf, const char *value);
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
   node->klass = find_klass(s, content->type);
   if (!node->klass)
     EINA_SAFETY_ON_NULL_RETURN_VAL(node->klass, NULL);
   node->node = content;
   node->s = s;
   return node;
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
        Inner_Outputter_Property_Value *value = calloc(1, sizeof(Inner_Outputter_Property_Value));
        value->pvalue = val;
        value->value.argument = eolian_parameter_name_get(parameter);
        value->value.type = eolian_parameter_type_get(parameter);
        if (val->is_node)
          {
             value->value.simple = EINA_FALSE;
             value->value.object = create_outputter_node(node->s, val->node);
             value->value.object->value_transform = node->value_transform;
          }
        else
          {
             Eina_Strbuf *buf = eina_strbuf_new();
             value->value.simple = EINA_TRUE;

             //value->value.value = _fetch_real_value(parameter, val->value);
             node->value_transform(value->value.type, buf, val->value);
             if (eina_strbuf_length_get(buf) == 0)
               eina_strbuf_append(buf, val->value);
             value->value.value = eina_strbuf_release(buf);
          }
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

Eina_Iterator*
outputter_children_get(Outputter_Node *node)
{
   Eina_Iterator *children;
   void *c;

   node->children = eina_array_new(10);
   children = eina_array_iterator_new(node->node->children);
   EINA_ITERATOR_FOREACH(children, c)
     {
        Outputter_Child *ochild = calloc(1, sizeof(Outputter_Child));
        ochild->type = node->node->usage_type;
        if (node->node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR)
          {
            Efl_Ui_Pack_Linear *linear = c;

            ochild->child = create_outputter_node(node->s,linear->node);
            ochild->child->value_transform = node->value_transform;
          }
        else if (node->node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE)
          {
             Efl_Ui_Pack_Table *table = c;
             ochild->table.x = atoi(table->x);
             ochild->table.y = atoi(table->y);
             ochild->table.w = atoi(table->w);
             ochild->table.h = atoi(table->h);
             ochild->child = create_outputter_node(node->s, table->node);
             ochild->child->value_transform = node->value_transform;
          }
        else if (node->node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK)
          {
             Efl_Ui_Pack_Pack *pack = c;
             ochild->pack.pack = pack->part_name;
             ochild->child = create_outputter_node(node->s, pack->node);
             ochild->child->value_transform = node->value_transform;
          }

        eina_array_push(node->children, ochild);
     }
   eina_iterator_free(children);
   return eina_array_iterator_new(node->children);
}

enum Efl_Ui_Node_Children_Type
outputter_node_type_get(Outputter_Node *node)
{
   return node->node->usage_type;
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

            if (!value->simple)
              {
                 _outputter_node_free(value->object);
              }
            else
              {
                 if (value->value)
                   free((char*)value->value);
              }
            free(value);
          }
        eina_array_free(prop->values);
        free(prop);
     }
   eina_array_free(node->properties);
   free(node);
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
