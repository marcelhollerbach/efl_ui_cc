#include "Efl_Ui_Format.h"
#include "Internal.h"
#include "abstract_tree_private.h"

struct _Outputter_Node {
   const Eolian_Class *klass;
   Eolian_State *s;
   Efl_Ui_Node *node;
   Eina_Array *children;
   Eina_Array *properties;
};

typedef struct {
   Outputter_Property prop;
   Eina_Array *values;
} Inner_Outputter_Property;

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
   node->klass = find_klass(s, content->type);
   node->node = content;
   return node;
}

static void
_outputter_properties_values_fill(Outputter_Node *node, Inner_Outputter_Property *iprop, Efl_Ui_Property *prop)
{
   Eina_Iterator *values;
   Efl_Ui_Property_Value *val;
   iprop->values = eina_array_new(10);
   values = eina_array_iterator_new(prop->value);

   EINA_ITERATOR_FOREACH(values, val)
     {
        Outputter_Property_Value *value = calloc(1, sizeof(Outputter_Property_Value));
        if (val->is_node)
          {
             value->simple = EINA_FALSE;
             value->object = create_outputter_node(node->s, val->node);
          }
        else
          {
             value->simple = EINA_TRUE;
             value->value = val->value;
          }
        eina_array_push(iprop->values, value);
     }
   eina_iterator_free(values);
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
        oprop->prop.property = find_function(node->s, node->klass, prop->key);
        _outputter_properties_values_fill(node, oprop, prop);
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
          }
        else if (node->node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE)
          {
             Efl_Ui_Pack_Table *table = c;
             ochild->table.x = atoi(table->x);
             ochild->table.y = atoi(table->y);
             ochild->table.w = atoi(table->w);
             ochild->table.h = atoi(table->h);
             ochild->child = create_outputter_node(node->s, table->node);
          }
        else if (node->node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK)
          {
             Efl_Ui_Pack_Pack *pack = c;
             ochild->pack.pack = pack->part_name;
             ochild->child = create_outputter_node(node->s, pack->node);
          }

        eina_array_push(node->children, ochild);
     }
   eina_iterator_free(children);
   return eina_array_iterator_new(node->children);
}


Outputter_Node*
outputter_node_init(Eolian_State *s, Efl_Ui* ui, const char **name)
{
   Outputter_Node *n;

   n = create_outputter_node(s, ui->content);

   n->s = s;
   *name = ui->name;

   return n;
}

static void
_outputter_node_free(Outputter_Node *node)
{
   while (eina_array_count(node->children))
     {
        Outputter_Child *child;

        child = eina_array_pop(node->children);
        _outputter_node_free(child->child);
        free(child);
     }
   eina_array_free(node->children);
   while (eina_array_count(node->properties))
     {
        Inner_Outputter_Property *prop = eina_array_pop(node->properties);

        while(eina_array_count(prop->values))
          {
            Outputter_Property_Value *value = eina_array_pop(prop->values);

            if (!value->simple)
              _outputter_node_free(value->object);

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
