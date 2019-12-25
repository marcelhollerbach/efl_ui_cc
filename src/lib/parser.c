#include "Efl_Ui_Format.h"
#include "Internal.h"
#include "abstract_tree_private.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Eina.h>

#include "json.h"


typedef Eina_Bool (*Custom_Handler)(void *data, const char *key, const char *value);

static Eina_Bool parse_ui_element(Efl_Ui_Node *node, int last_pos, Custom_Handler custom_handler, void *data);

static jsmn_parser parser;
static jsmntok_t *tokens;
static int tokens_current_pos = 0;
static int tokens_max_pos = 128;
static const char *input_content;

typedef enum _token_result {
   T_EOF = -2,
   T_POP_SCOPE = -1,
   T_OK = 0,
} Token_Result;

static Token_Result
next_token(char **value, jsmntype_t *type, int *return_pos, int max_pos)
{
   if (tokens_current_pos >= tokens_max_pos)
     {
        return T_EOF;
     }

   *value = eina_strndup(input_content + tokens[tokens_current_pos].start, tokens[tokens_current_pos].end - tokens[tokens_current_pos].start);
   *type = tokens[tokens_current_pos].type;
   *return_pos = tokens[tokens_current_pos].end;
   if (*return_pos >= max_pos)
     {
        free(*value);
        *value = NULL;
        return T_POP_SCOPE;
     }
   tokens_current_pos ++;
   return T_OK;
}

static Token_Result
next_pair(char **key, char **value, jsmntype_t *type, int *return_pos, int max_pos)
{
   jsmntype_t tmpt;
   int tmp_returnpos;
   Token_Result res;

   *key = NULL;
   *value = NULL;

   res = next_token(key, &tmpt, &tmp_returnpos, max_pos);
   if (res < 0)
     return res;
   res = next_token(value, type, return_pos, max_pos);
   if (res < 0)
     return res;

   return T_OK;
}

static inline Eina_Bool
handle_property_value(Efl_Ui_Property_Value *v, char *value, jsmntype_t type, int last_pos)
{
   if (type == JSMN_OBJECT)
     {
        Efl_Ui_Node *n;
        n = property_value_node(v);
        if (!parse_ui_element(n, last_pos, NULL, NULL))
          return EINA_FALSE;
     }
   else if (type == JSMN_STRING)
     {
        Eina_Strbuf *string = eina_strbuf_new();
        eina_strbuf_append_printf(string, "\"%s\"", value);
        property_value_value(v, eina_strbuf_string_get(string));
        eina_strbuf_free(string);
     }
   else if (type == JSMN_PRIMITIVE)
     {
        property_value_value(v, value);
     }
   else if (type == JSMN_ARRAY)
     {
        printf("TODO\n");
     }
   else
     {
        printf("Error, Unexpected type\n");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static Eina_Bool
parse_parameter_array(Efl_Ui_Property *prop, int return_pos)
{
   int last_pos = 0;
   char *value;
   jsmntype_t type;

   while(1)
     {
        Efl_Ui_Property_Value *v;

        if (next_token(&value, &type, &last_pos, return_pos) < 0)
          break;

        v = property_value_append(prop);

        if (!handle_property_value(v, value, type, last_pos))
          return EINA_FALSE;

        free(value);
        value = NULL;
     }

   return EINA_TRUE;
}

static Eina_Bool
parse_pack_linear(Efl_Ui_Node *n, int return_pos)
{
   char *value;
   jsmntype_t type;
   int last_pos;

   while(1)
     {
        Efl_Ui_Pack_Linear *next;
        if (next_token(&value, &type, &last_pos, return_pos) < 0 )
          break;

        if (type != JSMN_OBJECT)
          {
             printf("Error, Object expected, got %d, content %s\n", type, value);
             return EINA_FALSE;
          }
        next = node_pack_linear_node_append(n);

        if (!parse_ui_element(next->basic.node, last_pos, NULL, NULL))
          return EINA_FALSE;

        free(value);
        value = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
handler_context_table(void *data, const char *key, const char *value)
{
   Efl_Ui_Pack_Table *ctx = data;
   if (eina_streq(key, "x"))
     {
        ctx->x = eina_strdup(value);
        return EINA_TRUE;
     }
   else if (eina_streq(key, "y"))
     {
        ctx->y = eina_strdup(value);
        return EINA_TRUE;
     }
   else if (eina_streq(key, "w"))
     {
        ctx->w = eina_strdup(value);
        return EINA_TRUE;
     }
   else if (eina_streq(key, "h"))
     {
        ctx->h = eina_strdup(value);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Eina_Bool
parse_pack_table(Efl_Ui_Node *n, int return_pos)
{
   char *value;
   jsmntype_t type;
   int last_pos;

   while(1)
     {
        Efl_Ui_Pack_Table *next;

        if (next_token(&value, &type, &last_pos, return_pos) < 0 )
          break;

        if (type != JSMN_OBJECT)
          {
             printf("Error, Object expected, got %d, content %s\n", type, value);
             return EINA_FALSE;
          }
        next = node_pack_table_node_append(n);


        if (!parse_ui_element(next->basic.node, last_pos, handler_context_table, next))
          return EINA_FALSE;

        if (!next->w)
          next->w = eina_strdup("0");

        if (!next->h)
          next->h = eina_strdup("0");

        free(value);
        value = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
handler_context_part(void *data, const char *key, const char *value)
{
   Efl_Ui_Pack_Pack *ctx = data;
   if (eina_streq(key, "part"))
     {
        ctx->part_name = eina_strdup(value);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Eina_Bool
parse_part(Efl_Ui_Node *n, int return_pos)
{
   char *value;
   jsmntype_t type;
   int last_pos;

   while(1)
     {
        Efl_Ui_Pack_Pack *next;

        if (next_token(&value, &type, &last_pos, return_pos) < 0)
          break;

        if (type != JSMN_OBJECT)
          {
             printf("Error, Object expected, got %d, content %s\n", type, value);
             return EINA_FALSE;
          }
        next = node_pack_node_append(n);

        if (!parse_ui_element(next->basic.node, last_pos, handler_context_part, next))
          return EINA_FALSE;

        free(value);
        value = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
parse_ui_element(Efl_Ui_Node *node, int return_pos, Custom_Handler custom_handler, void *data)
{
   int last_pos = 0;
   char *key, *value;
   jsmntype_t type;

   if (next_pair(&key, &value, &type, &last_pos, return_pos) < 0)
     {
        printf("Error: First token has to be \"type\" but it couldnt be fetched\n");
        return EINA_FALSE;
     }

   //ensure that the first key is always the type
   if (!eina_streq(key, "type"))
     {
        printf("Error: First token has to be \"type\", got %s\n", key);
        return EINA_FALSE;
     }
   else
     {
        node_type_set(node, value);
        free(key);
        key = NULL;

        free(value);
        value = NULL;
     }

   while(1)
     {
        if (next_pair(&key, &value, &type, &last_pos, return_pos) < 0)
          {
             key = NULL;
             value = NULL;
             break;
          }

        if (custom_handler && custom_handler(data, key, value))
          goto end;

        //reserved names that are carrying information only for efl-ui
        if (eina_streq(key, "id"))
          {
            node_id_set(node, value);
          }
        else if (eina_streq(key, "pack-linear"))
          {
             //its just a array, walk it down, and pack it
             if (type != JSMN_ARRAY)
               {
                  printf("Error, pack-linear must have an array as content\n");
                  return EINA_FALSE;
               }
             if (!parse_pack_linear(node, last_pos))
               return EINA_FALSE;
          }
        else if (eina_streq(key, "pack-table"))
          {
             //require x / y
             if (type != JSMN_ARRAY)
               {
                  printf("Error, pack-linear must have an array as content\n");
                  return EINA_FALSE;
               }
             if (!parse_pack_table(node, last_pos))
               return EINA_FALSE;
          }
        else if (eina_streq(key, "pack-part"))
          {
             //require part names
             if (!parse_part(node, last_pos))
               return EINA_FALSE;
          }
        else
          {
             Efl_Ui_Property *property = node_property_append(node);
             property_key_set(property, key);
             if (type == JSMN_ARRAY)
               {
                  if (!parse_parameter_array(property, last_pos))
                    return EINA_FALSE;
               }
             else
               {
                  Efl_Ui_Property_Value *v = property_value_append(property);
                  handle_property_value(v, value, type, last_pos);
               }
          }

end:
        free(key);
        key = NULL;

        free(value);
        value = NULL;
     }

   return EINA_TRUE;
}

static Eina_Bool
parse_ui_description(Efl_Ui *ui, int return_pos)
{
   int last_pos = 0;
   Eina_Bool name = EINA_FALSE;

   while(1)
     {
        char *key, *value;
        jsmntype_t type;

        if (next_pair(&key, &value, &type, &last_pos, return_pos) < 0)
          break;

        if (eina_streq(key, "ui-name") && type == JSMN_STRING)
          {
             efl_ui_name_set(ui, value);
             name = EINA_TRUE;
          }
        else if (eina_streq(key, "ui-content") && type == JSMN_OBJECT)
          {
             if (!name)
               {
                  printf("Error, \"ui-name\" must be set before \"id-content\" can be used\n");
                  return EINA_FALSE;
               }

             if (!parse_ui_element(efl_ui_content_get(ui), last_pos, NULL, NULL))
               {
                  printf("Error, parsing failed\n");
                  return EINA_FALSE;
               }
          }
        else
          {
             printf("Error, unhandled case %s %s\n", value, key);
             return EINA_FALSE;
          }
        free(key);
        key = NULL;
        free(value);
        value = NULL;
     }
   return EINA_TRUE;
}

typedef struct {
   Eina_Strbuf *func_calls, *typedef_fields;
   int obj_depth;
} Parser_Context;

Efl_Ui*
efl_ui_format_parse(const char *c)
{
   int r = JSMN_ERROR_NOMEM;
   jsmn_init(&parser);

   input_content = eina_strdup(c);

   while (r < 0)
     {
        tokens = realloc(tokens, tokens_max_pos * sizeof(jsmntok_t));
        r = jsmn_parse(&parser, input_content, strlen(input_content), tokens, tokens_max_pos);
        if (r == JSMN_ERROR_INVAL || r == JSMN_ERROR_PART)
          {
            printf("Parsing error! %s\n", input_content + parser.pos);
            return NULL;
          }
        else if (r == JSMN_ERROR_NOMEM)
          {
            tokens_max_pos *= 2;
          }
        else
          {
            tokens_max_pos = r;
          }
     }
   if (tokens_max_pos)
     {
        //eat the upper structure
        tokens_current_pos++;
        Efl_Ui *ui = efl_ui_new();

        if (!parse_ui_description(ui, strlen(input_content)))
          {
             printf("Parsing failed\n");
             return NULL;
          }
        return ui;
     }
   else
     {
        return NULL;
     }
}
