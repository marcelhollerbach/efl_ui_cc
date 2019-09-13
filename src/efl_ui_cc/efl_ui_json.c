#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Eina.h>
#include <Ecore_File.h>

#include "json.h"
#include "main.h"

typedef struct {
   Eina_Strbuf *global_typedefs;
   Eina_Strbuf *func_calls;
   int depth;
   int last_pos;
} Parser_Scope;

typedef Eina_Bool (*Custom_Handler)(void *data, const char *key, const char *value);

static Eina_Bool parse_ui_element(Parser_Scope *scope, Custom_Handler custom_handler, void *data);

static jsmn_parser parser;
static jsmntok_t *tokens;
static int tokens_current_pos = 0;
static int tokens_max_pos = 128;

static Parser_Scope*
parser_scope_new(int last_pos)
{
   Parser_Scope *ret = calloc(1, sizeof(Parser_Scope));
   ret->global_typedefs = eina_strbuf_new();
   ret->func_calls = eina_strbuf_new();
   ret->depth = 0;
   ret->last_pos = last_pos;

   eina_strbuf_append(ret->func_calls, "{\n");
   eina_strbuf_append(ret->func_calls, "   Eo *o = parent;\n");

   return ret;
}

static Parser_Scope*
parser_scope_inherit(Parser_Scope *scope, int last_pos)
{
   Parser_Scope *ret = calloc(1, sizeof(Parser_Scope));
   ret->global_typedefs = scope->global_typedefs;
   ret->func_calls = eina_strbuf_new();
   ret->depth = scope->depth + 1;
   ret->last_pos = last_pos;

   eina_strbuf_append(ret->func_calls, "{\n");
   eina_strbuf_append(ret->func_calls, "   Eo *o = parent;\n");

   return ret;
}

static void
parser_scope_free(Parser_Scope *scope)
{
   eina_strbuf_free(scope->func_calls);
   free(scope);
}

static void
parser_scope_merge(Eina_Strbuf *goal, Parser_Scope *gone)
{
   eina_strbuf_replace_all(gone->func_calls, "\n", "\n   ");
   eina_strbuf_prepend(gone->func_calls, "   ");

   eina_strbuf_append(gone->func_calls, "}\n");
   eina_strbuf_append_buffer(goal, gone->func_calls);
   parser_scope_free(gone);
}

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
     return T_POP_SCOPE;
   tokens_current_pos ++;
   return T_OK;
}

static Token_Result
next_pair(char **key, char **value, jsmntype_t *type, int *return_pos, int max_pos)
{
   jsmntype_t tmpt;
   int tmp_returnpos;
   Token_Result res;

   res = next_token(key, &tmpt, &tmp_returnpos, max_pos);
   if (res < 0)
     return res;
   next_token(value, type, return_pos, max_pos);
   if (res < 0)
     return res;
   return T_OK;
}

static Eina_Bool
parse_parameter_array(Parser_Scope *scope, Eina_Strbuf *buf, int return_pos)
{
   int last_pos = 0;
   char *value;
   jsmntype_t type;

   while(1)
     {
        if (next_token(&value, &type, &last_pos, return_pos) < 0)
          break;

        if (eina_strbuf_length_get(buf) != 0)
          eina_strbuf_append(buf, ", ");

        if (type == JSMN_OBJECT)
          {
             Parser_Scope *cs;

             cs = parser_scope_inherit(scope, last_pos);
             if (!parse_ui_element(cs, NULL, NULL))
              return EINA_FALSE;
             parser_scope_merge(scope->func_calls, cs);
             eina_strbuf_append(buf, "child");
          }
        else if (type == JSMN_STRING)
          {
             Eina_Strbuf *tmp = eina_strbuf_new();

             eina_strbuf_append_printf(tmp, "\"%s\"", value);
             eina_strbuf_append(buf, eina_strbuf_release(tmp));
          }
        else if (type == JSMN_PRIMITIVE)
          {
             eina_strbuf_append(buf, value);
          }
        else if (type == JSMN_ARRAY)
          {
             printf("Error, Array within Array\n");
          }
        free(value);
        value = NULL;
     }

   return EINA_TRUE;
}

static Eina_Bool
parse_pack_linear(Parser_Scope *scope, Eina_Strbuf *api_calls, int return_pos)
{
   char *value;
   jsmntype_t type;
   int last_pos;

   while(1)
     {
        Parser_Scope *cs;
        if (next_token(&value, &type, &last_pos, return_pos) < 0 )
          break;

        if (type != JSMN_OBJECT)
          {
             printf("Error, Object expected, got %d, content %s\n", type, value);
             return EINA_FALSE;
          }

        cs = parser_scope_inherit(scope, last_pos);
        if (!parse_ui_element(cs, NULL, NULL))
          return EINA_FALSE;
        parser_scope_merge(api_calls, cs);
        eina_strbuf_append(api_calls, "   efl_pack_end(o, child);\n");

        free(value);
        value = NULL;
     }
   return EINA_TRUE;
}

typedef struct {
   const char *x;
   const char *y;
   const char *w;
   const char *h;
} Context_Table;

static Eina_Bool
handler_context_table(void *data, const char *key, const char *value)
{
   Context_Table *ctx = data;
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
parse_pack_table(Parser_Scope *scope, Eina_Strbuf *api_calls, int return_pos)
{
   char *value;
   jsmntype_t type;
   int last_pos;

   while(1)
     {
        Parser_Scope *cs;
        Context_Table table = { 0 };

        if (next_token(&value, &type, &last_pos, return_pos) < 0 )
          break;

        if (type != JSMN_OBJECT)
          {
             printf("Error, Object expected, got %d, content %s\n", type, value);
             return EINA_FALSE;
          }
        if (!table.w)
          table.w = "1";
        if (!table.h)
          table.h = "1";

        cs = parser_scope_inherit(scope, last_pos);
        if (!parse_ui_element(cs, handler_context_table, &table))
          return EINA_FALSE;
        parser_scope_merge(api_calls, cs);
        eina_strbuf_append_printf(api_calls, "   efl_pack_table(o, child, %s, %s, %s, %s);\n", table.x, table.y, table.w, table.h);

        free(value);
        value = NULL;
     }
   return EINA_TRUE;
}

typedef struct {
   const char *part;
} Context_Part;

static Eina_Bool
handler_context_part(void *data, const char *key, const char *value)
{
   Context_Part *ctx = data;
   if (eina_streq(key, "part"))
     {
        ctx->part = eina_strdup(value);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Eina_Bool
parse_part(Parser_Scope *scope, Eina_Strbuf *api_calls, int return_pos)
{
   char *value;
   jsmntype_t type;
   int last_pos;

   while(1)
     {
        Parser_Scope *cs;
        Context_Part part;

        if (next_token(&value, &type, &last_pos, return_pos) < 0)
          break;

        if (type != JSMN_OBJECT)
          {
             printf("Error, Object expected, got %d, content %s\n", type, value);
             return EINA_FALSE;
          }

        cs = parser_scope_inherit(scope, last_pos);
        if (!parse_ui_element(cs, handler_context_part, &part))
          return EINA_FALSE;

        parser_scope_merge(api_calls, cs);
        if (!part.part)
          {
            printf("Error, part not found\n");
            return EINA_FALSE;
          }
        eina_strbuf_append_printf(api_calls, "   efl_content_set(efl_part(o, \"%s\"), child);\n", part.part);
        free(value);
        value = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
parse_ui_element(Parser_Scope *scope, Custom_Handler custom_handler, void *data)
{
   Eina_Strbuf *api_calls;
   Klass_Type *t;
   char *id = NULL;
   char *ctx_id = NULL;
   int last_pos = 0;
   char *key, *value;
   jsmntype_t type;

   api_calls = eina_strbuf_new();

   next_pair(&key, &value, &type, &last_pos, scope->last_pos);

   //ensure that the first key is always the type
   if (!eina_streq(key, "type"))
     {
        printf("Error: First token has to be \"type\", got %s\n", key);
        return EINA_FALSE;
     }
   else
     {
        t = klass_fetch(value);
        if (!t)
          {
             printf("Class %s could not be found.\n", value);
             return EINA_FALSE;
          }
     }

   while(1)
     {
        if (next_pair(&key, &value, &type, &last_pos, scope->last_pos) < 0)
          break;

        if (custom_handler && custom_handler(data, key, value))
          goto end;

        //reserved names that are carrying information only for efl-ui
        if (eina_streq(key, "id"))
          {
             Eina_Strbuf *ctx_id_buf = eina_strbuf_new();
             id = eina_strdup(value);
             eina_strbuf_append_printf(ctx_id_buf, "%s->%s", "ctx", id);
             ctx_id = eina_strbuf_release(ctx_id_buf);

             goto end;
          }
        else if (eina_streq(key, "pack-linear"))
          {
             //its just a array, walk it down, and pack it
             if (type != JSMN_ARRAY)
               {
                  printf("Error, pack-linear must have an array as content\n");
                  return EINA_FALSE;
               }
             if (!parse_pack_linear(scope, api_calls, last_pos))
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
             if (!parse_pack_table(scope, api_calls, last_pos))
               return EINA_FALSE;
          }
        else if (eina_streq(key, "pack-part"))
          {
             //require part names
             if (!parse_part(scope, api_calls, last_pos))
               return EINA_FALSE;
          }
        else

        //handling general properties as normal API calls
        if (type == JSMN_OBJECT)
          {
             Parser_Scope *cs = parser_scope_inherit(scope, last_pos);

             //this must be a property with a Efl_Canvas_Object
             if (!parse_ui_element(cs, NULL, NULL))
              return EINA_FALSE;
             //by this time, the subobject is in "child"
             //now try to apply the API with child
             parser_scope_merge(api_calls, cs);
             if (!klass_property_append(t, key, "child", api_calls))
               return EINA_FALSE;
          }
        else if (type == JSMN_STRING)
          {
             Eina_Strbuf *tmp = eina_strbuf_new();

             eina_strbuf_append_printf(tmp, "\"%s\"", value);
             if (!klass_property_append(t, key, eina_strbuf_release(tmp), api_calls))
               return EINA_FALSE;
          }
        else if (type == JSMN_PRIMITIVE)
          {
             if (!klass_property_append(t, key, value, api_calls))
               return EINA_FALSE;
          }
        else if (type == JSMN_ARRAY)
          {
             Eina_Strbuf *buf = eina_strbuf_new();

             parse_parameter_array(scope, buf, last_pos);
             if (!klass_property_append(t, key, eina_strbuf_release(buf), api_calls))
               return EINA_FALSE;
          }
end:
        free(key);
        key = NULL;

        free(value);
        value = NULL;
     }
   eina_strbuf_append(api_calls, "   child = o;\n");

   //a id means we do want to be public with this field
   if (id)
     {
        klass_field_append(t, id, scope->global_typedefs);
        free(id);
     }

   //first creation line, then API calls
   klass_create_instance(t, ctx_id, "o", scope->func_calls);
   eina_strbuf_append_buffer(scope->func_calls, api_calls);
   klass_free(t);
   eina_strbuf_free(api_calls);
   free(ctx_id);

   return EINA_TRUE;
}

static Eina_Bool
parse_ui_description(Eina_Strbuf *func_calls, Eina_Strbuf *typedef_fields, int return_pos, Eina_Strbuf *ui_type, Eina_Strbuf *name)
{
   int last_pos = 0;

   eina_strbuf_append(typedef_fields, "typedef struct {\n");

   while(1)
     {
        char *key, *value;
        jsmntype_t type;

        if (next_pair(&key, &value, &type, &last_pos, return_pos) < 0)
          break;

        if (eina_streq(key, "ui-name") && type == JSMN_STRING)
          {
             eina_strbuf_append_printf(ui_type, "%s_Data", value);
             eina_strbuf_append(name, value);
             eina_strbuf_tolower(name);
          }
        else if (eina_streq(key, "ui-content") && type == JSMN_OBJECT)
          {
             Parser_Scope *scope = parser_scope_new(last_pos);
             if (!eina_strbuf_length_get(name))
               {
                  printf("Error, \"ui-name\" must be set before \"id-content\" can be used\n");
                  return EINA_FALSE;
               }

             if (!parse_ui_element(scope, NULL, NULL))
               {
                  printf("Error, parsing failed\n");
                  return EINA_FALSE;
               }
             eina_strbuf_append_buffer(typedef_fields, scope->global_typedefs);
             eina_strbuf_free(scope->global_typedefs);
             parser_scope_merge(func_calls, scope);
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
   eina_strbuf_append_printf(typedef_fields, "} ");
   eina_strbuf_append_buffer(typedef_fields, ui_type);
   eina_strbuf_append_printf(typedef_fields, ";\n");
   return EINA_TRUE;
}

typedef struct {
   Eina_Strbuf *func_calls, *typedef_fields;
   int obj_depth;
} Parser_Context;

static void
dump_c_file(Eina_Strbuf *func_calls, Eina_Strbuf *name, Eina_Strbuf *type)
{
   Eina_Strbuf *buf = eina_strbuf_new();

   eina_strbuf_append(buf, "#define EFL_BETA_API_SUPPORT 1\n");
   eina_strbuf_append(buf, "#include <Efl_Ui.h>\n");
   eina_strbuf_append_printf(buf, "#include \"%s\"\n", ecore_file_file_get(output_h_file));
   eina_strbuf_append_buffer(buf, type);
   eina_strbuf_append(buf, "*\n");
   eina_strbuf_append_buffer(buf, name);
   eina_strbuf_append(buf, "_gen(Eo *parent)\n");
   eina_strbuf_append(buf, "{\n");
   eina_strbuf_append(buf, "   Efl_Ui_Widget *child;\n\n");
   eina_strbuf_append_printf(buf, "   %s *ctx = calloc(1, sizeof(%s));\n\n", eina_strbuf_string_get(type), eina_strbuf_string_get(type));
   eina_strbuf_append_buffer(buf, func_calls);
   eina_strbuf_append(buf, "   (void)child;\n");
   eina_strbuf_append(buf, "   return ctx;\n");
   eina_strbuf_append(buf, "}\n");

   c_file_content = eina_strbuf_release(buf);
}

static void
dump_h_file(Eina_Strbuf *typedef_fields, Eina_Strbuf *name, Eina_Strbuf *type)
{
   Eina_Strbuf *buf = eina_strbuf_new();

   eina_strbuf_append(buf, "#define EFL_BETA_API_SUPPORT 1\n");
   eina_strbuf_append(buf, "#include <Elementary.h>\n\n");
   eina_strbuf_append(buf, "#include <Efl_Ui.h>\n\n");
   eina_strbuf_append(buf, "#define false 0\n\n");
   eina_strbuf_append(buf, "#define true !false\n\n");
   eina_strbuf_append_buffer(buf, typedef_fields);
   eina_strbuf_append(buf, "\n");
   eina_strbuf_append_buffer(buf, type);
   eina_strbuf_append(buf, "* ");
   eina_strbuf_append_buffer(buf, name);
   eina_strbuf_append(buf, "_gen(Eo *parent);\n");

   c_header_content = eina_strbuf_release(buf);
}

void
parse(void)
{
   Eina_Strbuf *func_calls, *typedef_fields, *type;
   Eina_Strbuf *name;
   int r = -1;

   name = eina_strbuf_new();
   func_calls = eina_strbuf_new();
   typedef_fields = eina_strbuf_new();
   type = eina_strbuf_new();

   jsmn_init(&parser);

   while (r < 0)
     {
        tokens = realloc(tokens, tokens_max_pos * sizeof(jsmntok_t));
        r = jsmn_parse(&parser, input_content, strlen(input_content), tokens, tokens_max_pos);
        if (r == JSMN_ERROR_INVAL || r == JSMN_ERROR_PART)
          {
            printf("Parsing error! %s\n", input_content + parser.pos);
            return;
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

   //eat the upper structure
   tokens_current_pos++;

   if (!parse_ui_description(func_calls, typedef_fields, strlen(input_content), type, name))
     {
        printf("Parsing failed\n");
        exit(-1);
     }

   //finalize file contents
   dump_c_file(func_calls, name, type);
   dump_h_file(typedef_fields, name, type);

   eina_strbuf_free(name);
   eina_strbuf_free(func_calls);
   eina_strbuf_free(typedef_fields);
   eina_strbuf_free(type);
   free(tokens);
}
