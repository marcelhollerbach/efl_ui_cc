#include "Efl_Ui_Format.h"
#include "Internal.h"
#include "abstract_tree_private.h"

static Efl_Ui_Node*
create_node(void)
{
   Efl_Ui_Node *node = calloc(1, sizeof(Efl_Ui_Node));

   node->properties = eina_array_new(10);
   node->children = eina_array_new(10);
   return node;
}

void
node_id_set(Efl_Ui_Node *n, const char *id)
{
   if (n->id) free((char*)n->id);
   n->id = eina_strdup(id);
}

void
node_type_set(Efl_Ui_Node *n, const char *type)
{
   if (n->type) free((char*)n->type);
   n->type = eina_strdup(type);
}

Efl_Ui_Property*
node_property_append(Efl_Ui_Node *n)
{
   Efl_Ui_Property *ret = calloc(1, sizeof(Efl_Ui_Property));

   ret->value = eina_array_new(10);

   eina_array_push(n->properties, ret);

   return ret;
}

void
property_key_set(Efl_Ui_Property *prop, const char *key)
{
   if (prop->key) free((char*) prop->key);
   prop->key = eina_strdup(key);
}

Efl_Ui_Property_Value*
property_value_append(Efl_Ui_Property *prop)
{
   Efl_Ui_Property_Value* ret = calloc(1, sizeof(Efl_Ui_Property_Value));

   eina_array_push(prop->value, ret);

   return ret;
}

Efl_Ui_Node*
property_value_node(Efl_Ui_Property_Value *val)
{
   Efl_Ui_Node *node = create_node();

   val->is_node = EINA_TRUE;
   val->node = node;
   return node;
}

void
property_value_value(Efl_Ui_Property_Value *val, const char *value)
{
   val->is_node = EINA_FALSE;
   if (val->value) free((char*)val->value);
   val->value = eina_strdup(value);
}

Efl_Ui_Pack_Linear*
node_pack_linear_node_append(Efl_Ui_Node *node)
{
   Efl_Ui_Pack_Linear *inner_node;

   node->usage_type = EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR;
   inner_node = calloc(1, sizeof(Efl_Ui_Pack_Linear));
   inner_node->node = create_node();
   eina_array_push(node->children, inner_node);
   return inner_node;
}

Efl_Ui_Pack_Table*
node_pack_table_node_append(Efl_Ui_Node *node)
{
   Efl_Ui_Pack_Table *inner_node;

   node->usage_type = EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE;
   inner_node = calloc(1, sizeof(Efl_Ui_Pack_Table));
   inner_node->node = create_node();
   eina_array_push(node->children, inner_node);

   return inner_node;
}

Efl_Ui_Pack_Pack*
node_pack_node_append(Efl_Ui_Node *node)
{
   Efl_Ui_Pack_Pack *inner_node;
   node->usage_type = EFL_UI_NODE_CHILDREN_TYPE_PACK;

   inner_node = calloc(1, sizeof(Efl_Ui_Pack_Table));
   inner_node->node = create_node();
   eina_array_push(node->children, inner_node);

   return inner_node;
}

Efl_Ui*
efl_ui_new(void)
{
   Efl_Ui *ui = calloc(1, sizeof(Efl_Ui));

   return ui;
}

void
efl_ui_name_set(Efl_Ui *ui, const char *name)
{
   if (ui->name) free((char*)ui->name);
   ui->name = eina_strdup(name);
}

Efl_Ui_Node*
efl_ui_content_get(Efl_Ui *ui)
{
   ui->content = create_node();
   return ui->content;
}

void
efl_ui_node_free(Efl_Ui_Node *node)
{
   if (node->id)
     free((char*)node->id);

   if (node->type)
     free((char*)node->type);

   //handle properties
   {
      Efl_Ui_Property *property;

      while(eina_array_count(node->properties))
        {
           property = eina_array_pop(node->properties);

           if (property->key)
             free((char*)property->key);

           Efl_Ui_Property_Value *value;
           while(eina_array_count(property->value))
             {
                value = eina_array_pop(property->value);
                if (value->is_node)
                  efl_ui_node_free(value->node);
                else if (value->value)
                  free((char*)value->value);
                free(value);
             }
           eina_array_free(property->value);
           free(property);
        }
   }
   eina_array_free(node->properties);

   //handle children
   {
      while (eina_array_count(node->children))
        {
           if (node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR)
             {
                Efl_Ui_Pack_Linear *linear = eina_array_pop(node->children);

                efl_ui_node_free(linear->node);
                free(linear);
             }
           else if (node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE)
             {
                Efl_Ui_Pack_Table *table = eina_array_pop(node->children);
                if (table->x)
                  free((char*)table->x);
                if (table->y)
                  free((char*)table->y);
                if (table->w)
                  free((char*)table->w);
                if (table->h)
                  free((char*)table->h);
                efl_ui_node_free(table->node);
                free(table);
             }
           else if (node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK)
             {
                Efl_Ui_Pack_Pack *pack = eina_array_pop(node->children);
                if (pack->part_name)
                  free((char*)pack->part_name);
                efl_ui_node_free(pack->node);
                free(pack);
             }

        }
      eina_array_free(node->children);
   }
   if (node)
     free(node);
}

void
efl_ui_free(Efl_Ui *ui)
{
   if (ui->name)
     free((char*)ui->name);
   efl_ui_node_free(ui->content);
   free(ui);
}

enum Efl_Ui_Node_Children_Type
node_child_type_get(Efl_Ui_Node *node)
{
   return node->usage_type;
}

const char*
node_id_get(Efl_Ui_Node *n)
{
   return n->id;
}
