#include "Efl_Ui_Format.h"
#include "Internal.h"
#include "abstract_tree_private.h"

typedef struct {
   Eolian_State *s;
} Validator_Context;


static Eina_Bool validate_node(Validator_Context *ctx, Efl_Ui_Node *node);

static Eina_Bool
validate_builtin_property_values(const Validator_Context *ctx, const Eolian_Type *type, const Efl_Ui_Property_Value *value)
{
   Eolian_Type_Builtin_Type builtin = eolian_type_builtin_type_get(type);

   if (builtin >= EOLIAN_TYPE_BUILTIN_BYTE && builtin <= EOLIAN_TYPE_BUILTIN_PTRDIFF)
     {
        EINA_SAFETY_ON_TRUE_RETURN_VAL(value->is_node, EINA_FALSE);
        char *tmp_end;
        strtol(value->value, &tmp_end, 10);
        if (errno == EINVAL || errno == ERANGE || tmp_end != value->value + strlen(value->value))
          {
             printf("Numeric values must be numbers: '%s'\n", value->value);
             return EINA_FALSE;
          }
     }
   else if (builtin >= EOLIAN_TYPE_BUILTIN_FLOAT && builtin <= EOLIAN_TYPE_BUILTIN_DOUBLE)
     {
        EINA_SAFETY_ON_TRUE_RETURN_VAL(value->is_node, EINA_FALSE);
        char *tmp_end;
        strtof(value->value, &tmp_end);
        if (errno == ERANGE || tmp_end != value->value + strlen(value->value))
          {
             printf("FLoating values must be numbers: '%s'\n", value->value);
             return EINA_FALSE;
          }
     }
   else if (builtin == EOLIAN_TYPE_BUILTIN_BOOL)
     {
        EINA_SAFETY_ON_TRUE_RETURN_VAL(value->is_node, EINA_FALSE);
        if (!eina_streq(value->value, "true") && !eina_streq(value->value, "false"))
          {
             printf("Boolean values must be true or false, not '%s'\n", value->value);
             return EINA_FALSE;
          }
     }
   else if (builtin >= EOLIAN_TYPE_BUILTIN_MSTRING && builtin <= EOLIAN_TYPE_BUILTIN_STRINGSHARE)
     {
        EINA_SAFETY_ON_TRUE_RETURN_VAL(value->is_node, EINA_FALSE);
     }
   else
     {
        printf("Type %d is not handled yet.\n", builtin);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static Eina_Bool
validate_enum_values(const Validator_Context *ctx, const Eolian_Typedecl *type, const Efl_Ui_Property_Value *value)
{
   Eina_Iterator *iter = eolian_typedecl_enum_fields_get(type);
   Eolian_Enum_Type_Field *v;
   Eina_Strbuf *buf = eina_strbuf_new();

   EINA_ITERATOR_FOREACH(iter, v)
     {
        const char *possible_value = eolian_typedecl_enum_field_name_get(v);

        eina_strbuf_append_printf(buf, "%s, ", possible_value);

        if (!eina_streq(value->value, possible_value))
          continue;

        eina_strbuf_free(buf);
        eina_iterator_free(iter);
        return EINA_TRUE;
     }

   printf("Value %s not within '%s'\n", value->value, eina_strbuf_release(buf));

   eina_iterator_free(iter);
   return EINA_FALSE;
}

static Eina_Bool
validate_property(Validator_Context *ctx, const Eolian_Class *klass, Efl_Ui_Property *node)
{
   const Eolian_Function *f;
   Eolian_Function_Parameter *parameter;
   Eina_Iterator *parameters;

   f = find_function(ctx->s, klass, node->key);
   if (!f)
     {
        printf("Function %s not found\n", node->key);
        return EINA_FALSE;
     }
   parameters = eolian_property_values_get(f, EOLIAN_PROP_SET);
   int c = 0;
   EINA_ITERATOR_FOREACH(parameters, parameter)
     {
        const Eolian_Type *type = eolian_parameter_type_get(parameter);
        Efl_Ui_Property_Value *value = eina_array_data_get(node->value, c);

        switch(eolian_type_type_get(type))
          {
             case EOLIAN_TYPE_VOID:
               EINA_LOG_ERR("EOLIAN_TYPE_VOID should not happen as a paramter");
             break;
             case EOLIAN_TYPE_REGULAR:
               {
                  //a regular can be a number or a enum it seems ?
                  const Eolian_Typedecl *decl = eolian_type_typedecl_get(type);
                  if (decl && eolian_typedecl_type_get(decl) == EOLIAN_TYPEDECL_ENUM)
                    {
                       if (!validate_enum_values(ctx, decl, value))
                         return EINA_FALSE;
                    }
                  else
                    {
                       if (!validate_builtin_property_values(ctx, type, value))
                         {
                            printf("Error, in line %s\n", node->key);
                            return EINA_FALSE;
                         }
                    }
               }
             break;
             case EOLIAN_TYPE_CLASS:
               EINA_SAFETY_ON_FALSE_RETURN_VAL(value->is_node, EINA_FALSE);
               if (!validate_node(ctx, value->node))
                 return EINA_FALSE;
             break;
             default:
               EINA_LOG_ERR("This is an error");
             break;
          }

        c++;
     }
   eina_iterator_free(parameters);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(c == eina_array_count(node->value), EINA_FALSE);

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
