#include "efl_ui_format_privat.h"

typedef struct {
   Eolian_State *s;
} Validator_Context;

static Eina_Bool
validate_property(Validator_Context *ctx, const Eolian_Class *klass, Efl_Ui_Property *node)
{
   const Eolian_Function *f;

   f = find_function(ctx->s, klass, node->key);
   if (!f)
     {
        printf("Function %s not found\n", node->key);
        return EINA_FALSE;
     }
   /* FIXME validate the property values here */
   return EINA_TRUE;
}

static Eina_Bool
validate_node(Validator_Context *ctx, Efl_Ui_Node *node)
{
   Eina_Iterator *it;
   Efl_Ui_Property *property;
   const Eolian_Class *klass;

   //validate type here
   klass = find_klass(ctx->s, node->type);
   if (!klass)
     {
        printf("Failed to find klass\n");
        return EINA_FALSE;
     }

   it = eina_array_iterator_new(node->properties);

   EINA_ITERATOR_FOREACH(it, property)
     {
        if (!validate_property(ctx, klass, property))
          return EINA_FALSE;
     }

   eina_iterator_free(it);

   //validate children

   it = eina_array_iterator_new(node->children);

   if (node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR)
     {
        Efl_Ui_Pack_Linear *linear;

        EINA_ITERATOR_FOREACH(it, linear)
          {
             if (!validate_node(ctx, linear->node))
               return EINA_FALSE;
          }
     }
   else if (node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE)
     {
        Efl_Ui_Pack_Table *table;

        EINA_ITERATOR_FOREACH(it, table)
          {
             if (!validate_node(ctx, table->node))
               return EINA_FALSE;
          }
     }
   else if (node->usage_type == EFL_UI_NODE_CHILDREN_TYPE_PACK)
     {
        Efl_Ui_Pack_Pack *pack;

        EINA_ITERATOR_FOREACH(it, pack)
          {
             if (!validate_node(ctx, pack->node))
               return EINA_FALSE;
             if (!pack->part_name)
               {
                  printf("Part name required\n");
                  return EINA_FALSE;
               }
          }
     }
   else
     {
        if (eina_array_count(node->children))
          {
             printf("No children allowed here\n");
             return EINA_FALSE;
          }
     }

   eina_iterator_free(it);
   return EINA_TRUE;
}

Eina_Bool
validate(Eolian_State *state, Efl_Ui *ui)
{
   Validator_Context ctx = {state};
   return validate_node(&ctx, ui->content);
}
