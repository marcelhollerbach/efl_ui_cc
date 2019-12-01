#include "main.h"
#include <Ecore_File.h>
#include <Efl_Ui_Format.h>
#include <Eolian.h>

typedef struct {
} Json_Context;

static Eina_Strbuf*
_output_node(Json_Context *ctx, Outputter_Node *n, Outputter_Child *thischild)
{
   const char *id;
   Eina_Strbuf *current_object;
   const Eolian_Class *klass;

   id = outputter_node_id_get(n);
   klass = outputter_node_klass_get(n);
   current_object = eina_strbuf_new();
   eina_strbuf_append_printf(current_object, "  {\n");
   eina_strbuf_append_printf(current_object, "  \"type\" : \"%s\",\n", eolian_class_name_get(klass));
   if (id)
     eina_strbuf_append_printf(current_object, "  \"id\" : \"%s\",\n", id);

   if (thischild)
     {
        if (thischild->type == EFL_UI_NODE_CHILDREN_TYPE_PACK)
          {
             eina_strbuf_append_printf(current_object, "  \"part\" : \"%s\",\n", thischild->pack.pack);
          }
        else if (thischild->type == EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE)
          {
             eina_strbuf_append_printf(current_object, "  \"x\" : %d,\n", thischild->table.x);
             eina_strbuf_append_printf(current_object, "  \"y\" : %d,\n", thischild->table.y);
             eina_strbuf_append_printf(current_object, "  \"w\" : %d,\n", thischild->table.w);
             eina_strbuf_append_printf(current_object, "  \"h\" : %d,\n", thischild->table.h);
          }
        else if (thischild->type == EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR)
          {
            //There is nothing to do here
          }

     }

   //handle properties
   Outputter_Property *property;
   Eina_Iterator *properties = outputter_properties_get(n);
   EINA_ITERATOR_FOREACH(properties, property)
     {
        Outputter_Property_Value *value;
        int i = 0;

        eina_strbuf_append_printf(current_object, "  \"%s\" : ", eolian_function_name_get(property->property));

        EINA_ITERATOR_FOREACH(property->values, value)
          {
             if (i > 0)
               eina_strbuf_append(current_object, ", ");
             if (value->simple)
               {
                  eina_strbuf_append(current_object, value->value);
               }
             else
               {
                  Eina_Strbuf *buf = _output_node(ctx, value->object, NULL);
                  eina_strbuf_append_buffer(current_object, buf);
                  eina_strbuf_free(buf);
               }
             i++;
          }
        eina_iterator_free(property->values);
        eina_strbuf_append(current_object, ",\n");
     }
   eina_iterator_free(properties);

   //handle children
   Outputter_Child *child;
   Eina_Iterator *children = outputter_children_get(n);


   if (outputter_node_type_get(n) != EFL_UI_NODE_CHILDREN_TYPE_NOTHING)
     {
        const char *keys[] = {
          "NOTHING",
          "\"pack-linear\"",
          "\"pack-table\"",
          "\"pack-part\"",
        };
        eina_strbuf_append_printf(current_object, "  %s : [\n", keys[outputter_node_type_get(n)]);
        int i = 0;
        EINA_ITERATOR_FOREACH(children, child)
          {
             Eina_Strbuf *buf = _output_node(ctx, child->child, child);
             eina_strbuf_prepend(buf, "  ");
             eina_strbuf_replace_all(buf, "\n", "\n  ");

             if (i > 0)
               eina_strbuf_append(current_object, ", \n");
             eina_strbuf_append_buffer(current_object, buf);
             eina_strbuf_free(buf);
             i++;
         }
       eina_strbuf_append(current_object, "\n  ]\n");
       eina_iterator_free(children);
     }

   eina_strbuf_replace_all(current_object, "\n", "\n  ");
   eina_strbuf_append(current_object, "}");
   return current_object;
}

char*
json_output(const Eolian_State *s, const Efl_Ui *ui)
{
   Eina_Strbuf *type = eina_strbuf_new();
   Outputter_Node *start;
   Json_Context ctx;
   const char *full_case_name;
   char *json_output;
   char *name;

   start = outputter_node_init((Eolian_State*)s, (Efl_Ui*)ui, &full_case_name);
   Eina_Strbuf *main_func = _output_node(&ctx, start, NULL);

   name = eina_strdup(full_case_name);
   eina_str_tolower(&name);
   eina_strbuf_append_printf(type, "{\n");
   eina_strbuf_append_printf(type, "  \"ui-name\" : \"%s\"\n", full_case_name);
   eina_strbuf_append_printf(type, "  \"ui-content\" : ");
   eina_strbuf_replace_all(main_func, "\n", "\n  ");
   eina_strbuf_append_buffer(type, main_func);
   eina_strbuf_append(type, "\n}\n");
   json_output = eina_strdup(eina_strbuf_string_get(type));
   eina_strbuf_free(type);
   eina_strbuf_free(main_func);
   outputter_node_root_free(start);
   free(name);

   return json_output;
}
