#include "Efl_Ui_Format.h"
#include "Internal.h"
#include "abstract_tree_private.h"

typedef struct {
   Eolian_State *s;
   Eina_Array *messages;
} Validator_Context;

typedef struct {
   const char *placment_context;
   const Efl_Ui_Node *node;
} Validator_Context_Message_Node;

static Eina_Bool validate_node(Validator_Context *ctx, Efl_Ui_Node *node, const char *placment_context);

#define ERROR_OUT(ctx, ...) \
  do { \
    _print_position(ctx); \
    printf(__VA_ARGS__); \
    return EINA_FALSE; \
  } while (0) \

static void
_print_position(const Validator_Context *ctx)
{
   Eina_Iterator *msgs = eina_array_iterator_new(ctx->messages);
   Validator_Context_Message_Node *node;
   Eina_Bool first = EINA_TRUE;

   EINA_ITERATOR_FOREACH(msgs, node)
     {
        const char *opener = "in";

        if (first)
          opener = " In";

        if (node->placment_context && strlen(node->placment_context) > 0)
          printf("%s, ", node->placment_context);
        if (node->node->id)
          printf("%s \"%s\" ", opener, node->node->id);
        else
          printf("%s \"%s\" ", opener, node->node->type);
        first = EINA_FALSE;
     }
   eina_iterator_free(msgs);
   printf(":\n   ");
}

static Eina_Bool
validate_builtin_property_values(const Validator_Context *ctx, const Eolian_Type *type, const Efl_Ui_Property_Value *value)
{
   Eolian_Type_Builtin_Type builtin = eolian_type_builtin_type_get(type);

   if (!value)
     {
        ERROR_OUT(ctx, "Values are NULL");
     }

   if (builtin >= EOLIAN_TYPE_BUILTIN_BYTE && builtin <= EOLIAN_TYPE_BUILTIN_PTRDIFF)
     {
        char *tmp_end;
        if (value->is_node)
          {
            ERROR_OUT(ctx, "Expected a number");
          }
        strtol(value->value, &tmp_end, 10);
        if (errno == EINVAL || errno == ERANGE || tmp_end != value->value + strlen(value->value))
          {
             ERROR_OUT(ctx, "Numeric values must be numbers: '%s'\n", value->value);
          }
     }
   else if (builtin >= EOLIAN_TYPE_BUILTIN_FLOAT && builtin <= EOLIAN_TYPE_BUILTIN_DOUBLE)
     {
        char *tmp_end;
        if (value->is_node)
          {
            ERROR_OUT(ctx, "Expected a float");
          }
        strtof(value->value, &tmp_end);
        if (errno == ERANGE || tmp_end != value->value + strlen(value->value))
          {
             ERROR_OUT(ctx, "FLoating values must be numbers: '%s'\n", value->value);
          }

     }
   else if (builtin == EOLIAN_TYPE_BUILTIN_BOOL)
     {
        if (value->is_node)
          {
            ERROR_OUT(ctx, "Expected true or false");
          }
        if (!eina_streq(value->value, "true") && !eina_streq(value->value, "false"))
          {
             ERROR_OUT(ctx, "Boolean values must be true or false, not '%s'\n", value->value);
          }
     }
   else if (builtin >= EOLIAN_TYPE_BUILTIN_MSTRING && builtin <= EOLIAN_TYPE_BUILTIN_STRINGSHARE)
     {
        if (value->is_node)
          {
            ERROR_OUT(ctx, "Expected a String");
          }
     }
   else
     {
        ERROR_OUT(ctx, "Type %d is not handled yet.\n", builtin);
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

   ERROR_OUT(ctx, "Value %s not within '%s'\n", value->value, eina_strbuf_release(buf));
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
        ERROR_OUT(ctx, "Function %s not found\n", node->key);
     }
   parameters = eolian_property_values_get(f, EOLIAN_PROP_SET);
   int c = 0;
   EINA_ITERATOR_FOREACH(parameters, parameter)
     {
        const Eolian_Type *type = eolian_parameter_type_get(parameter);
        Efl_Ui_Property_Value *value = eina_array_data_get(node->value, c);
        const Eolian_Typedecl *decl = eolian_type_typedecl_get(type);

        if (c >= eina_array_count(node->value))
          {
             eina_iterator_free(parameters);
             ERROR_OUT(ctx, "Too few parameters supplied.\n");
          }
        value = eina_array_data_get(node->value, c);
        fetch_real_typedecl(&decl, &type);

        switch(eolian_type_type_get(type))
          {
             case EOLIAN_TYPE_VOID:
               EINA_LOG_ERR("EOLIAN_TYPE_VOID should not happen as a paramter");
             break;
             case EOLIAN_TYPE_REGULAR:
               {
                  //a regular can be a number or a enum it seems ?
                  if (decl && eolian_typedecl_type_get(decl) == EOLIAN_TYPEDECL_ENUM)
                    {
                       if (!validate_enum_values(ctx, decl, value))
                         {
                            eina_iterator_free(parameters);
                            return EINA_FALSE;
                         }
                    }
                  else
                    {
                       if (!value)
                         {
                            ERROR_OUT(ctx, "Value is NULL");
                         }
                       if (!validate_builtin_property_values(ctx, type, value))
                         {
                            eina_iterator_free(parameters);
                            return EINA_FALSE;
                         }
                    }
               }
             break;
             case EOLIAN_TYPE_CLASS: {
               char buffer[100];
               EINA_SAFETY_ON_FALSE_RETURN_VAL(value->is_node, EINA_FALSE);
               snprintf(buffer, sizeof(buffer), "at property %s", node->key);
               if (!validate_node(ctx, value->node, buffer))
                 {
                    eina_iterator_free(parameters);
                    return EINA_FALSE;
                 }
             }
             break;
             default:
               EINA_LOG_ERR("This is an error");
             break;
          }

        c++;
     }
   eina_iterator_free(parameters);
   if (c != eina_array_count(node->value))
     {
        ERROR_OUT(ctx, "Not enough arguments for property %s", node->key);
     }

   return EINA_TRUE;
}

static Eina_Bool
validate_klass(Validator_Context *ctx, const char *klass_name, const Eolian_Class *klass)
{
   if (!klass)
     {
        ERROR_OUT(ctx, "type %s cannot be found", klass_name);
     }
   return EINA_TRUE;
}

static Eina_Bool
validate_part_name(Validator_Context *ctx, Efl_Ui_Pack_Pack *pack, Eina_Hash *part_names)
{
   if (!pack->part_name)
     {
        ERROR_OUT(ctx, "Part name required");
     }
   if (eina_hash_find(part_names, &pack->part_name))
     {
        ERROR_OUT(ctx, "Part name %s is duplicated", pack->part_name);
     }
   eina_hash_add(part_names, &pack->part_name, pack);
   return EINA_TRUE;
}

static Eina_Bool
validate_node(Validator_Context *ctx, Efl_Ui_Node *node, const char *placment_context)
{
   Eina_Iterator *it;
   Efl_Ui_Property *property;
   const Eolian_Class *klass;
   Validator_Context_Message_Node ctx_node;

   ctx_node.placment_context = placment_context;
   ctx_node.node = node;

   eina_array_push(ctx->messages, &ctx_node);

   //validate type here
   klass = find_klass(ctx->s, node->type);
   if (!validate_klass(ctx, node->type, klass))
     {
        eina_array_pop(ctx->messages);
        return EINA_FALSE;
     }

   it = eina_array_iterator_new(node->properties);

   EINA_ITERATOR_FOREACH(it, property)
     {
        if (!validate_property(ctx, klass, property))
          {
             eina_iterator_free(it);
             eina_array_pop(ctx->messages);
             return EINA_FALSE;
          }
     }

   eina_iterator_free(it);

   //validate children
   if (eina_array_count(node->children_linear))
     {
        Efl_Ui_Pack_Linear *linear;
        Eina_Iterator *it = eina_array_iterator_new(node->children_linear);
        int i = 1;

        EINA_ITERATOR_FOREACH(it, linear)
          {
             char buffer[100];

             snprintf(buffer, sizeof(buffer), "at %d. place", i);
             if (!validate_node(ctx, linear->basic.node, buffer))
               {
                  eina_iterator_free(it);
                  eina_array_pop(ctx->messages);
                  return EINA_FALSE;
               }
             i ++;
          }
        eina_iterator_free(it);
     }
   if (eina_array_count(node->children_table))
     {
        Efl_Ui_Pack_Table *table;
        Eina_Iterator *it = eina_array_iterator_new(node->children_table);

        EINA_ITERATOR_FOREACH(it, table)
          {
             char buffer[100];

             snprintf(buffer, sizeof(buffer), "at position (%s, %s, %s, %s)", table->x, table->y, table->w, table->h);
             if (!validate_node(ctx, table->basic.node, buffer))
               {
                  eina_iterator_free(it);
                  eina_array_pop(ctx->messages);
                  return EINA_FALSE;
               }
          }
        eina_iterator_free(it);
     }
   if (eina_array_count(node->children_part))
     {
        Efl_Ui_Pack_Pack *pack;
        Eina_Iterator *it = eina_array_iterator_new(node->children_table);
        Eina_Hash *part_name = eina_hash_string_small_new(NULL);

        EINA_ITERATOR_FOREACH(it, pack)
          {
             char buffer[100];

             if (!validate_part_name(ctx, pack, part_name))
               {
                  eina_hash_free(part_name);
                  eina_iterator_free(it);
                  eina_array_pop(ctx->messages);
                  return EINA_FALSE;
               }
             snprintf(buffer, sizeof(buffer), "at part %s", pack->part_name);
             if (!validate_node(ctx, pack->basic.node, buffer))
               {
                  eina_hash_free(part_name);
                  eina_iterator_free(it);
                  eina_array_pop(ctx->messages);
                  return EINA_FALSE;
               }

          }
        eina_hash_free(part_name);
        eina_iterator_free(it);
     }
   eina_array_pop(ctx->messages);
   return EINA_TRUE;
}

Eina_Bool
validate(Eolian_State *state, Efl_Ui *ui)
{
   Validator_Context ctx = {state};

   ctx.messages = eina_array_new(10);
   if (!ui)
     return EINA_TRUE;

   if (ui->content)
     {
        Eina_Bool v = validate_node(&ctx, ui->content, "");
        EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_array_count(ctx.messages) == 0, EINA_FALSE);
        eina_array_free(ctx.messages);
        return v;
     }
   else
     return EINA_TRUE;
}
