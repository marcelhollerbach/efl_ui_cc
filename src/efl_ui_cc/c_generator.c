#include "main.h"
#include <Ecore_File.h>
#include <Efl_Ui_Format.h>

typedef struct {
   Eina_Strbuf *typedef_fields;
} Generator_Context;

static Eina_Strbuf*
_output_node(Generator_Context *ctx, Outputter_Node *n)
{
   const char *id;
   Eina_Strbuf *func_calls;
   const Eolian_Class *klass;

   klass = outputter_node_klass_get(n);
   func_calls = eina_strbuf_new();
   eina_strbuf_append(func_calls, "   {\n");
   eina_strbuf_append(func_calls, "   Eo *o;\n");
   eina_strbuf_append(func_calls, "   ");

   id = outputter_node_id_get(n);
   if (id)
     {
        eina_strbuf_append_printf(ctx->typedef_fields, "   %s* %s;\n", eolian_object_c_name_get(EOLIAN_OBJECT(klass)), id);
        eina_strbuf_append_printf(func_calls, "ctx->%s = ", id);
     }
   eina_strbuf_append_printf(func_calls, "o = efl_add(%s(), %s);\n", eolian_class_c_get_function_name_get(klass), "parent");

   //handle properties
   Outputter_Property *property;
   Eina_Iterator *properties = outputter_properties_get(n);
   EINA_ITERATOR_FOREACH(properties, property)
     {
        Outputter_Property_Value *value;
        Eina_Strbuf *params = eina_strbuf_new();
        eina_strbuf_append(params, "o");
        int i = 0;

        EINA_ITERATOR_FOREACH(property->values, value)
          {
             eina_strbuf_append(params, ", ");
             if (value->simple)
               {
                  eina_strbuf_append(params, value->value);
               }
             else
               {
                  const char *func_name = eolian_object_name_get(EOLIAN_OBJECT(property->property));
                  Eina_Strbuf *buf = _output_node(ctx, value->object);
                  eina_strbuf_append_buffer(func_calls, buf);
                  eina_strbuf_append_printf(func_calls, "   Eo *%s%d = child;\n", func_name, i);
                  eina_strbuf_append_printf(params, "%s%d", func_name, i);
                  eina_strbuf_free(buf);
               }
             i++;
          }
        eina_iterator_free(property->values);
        eina_strbuf_append_printf(func_calls, "   %s(%s);\n", eolian_function_full_c_name_get(property->property, EOLIAN_PROP_SET), eina_strbuf_string_get(params));
        eina_strbuf_free(params);
     }
   eina_iterator_free(properties);

   //handle children
   Outputter_Child *child;
   Eina_Iterator *children = outputter_children_get(n);

   EINA_ITERATOR_FOREACH(children, child)
     {
        Eina_Strbuf *buf = _output_node(ctx, child->child);
        eina_strbuf_append_buffer(func_calls, buf);
        if (child->type == EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR)
          {
             eina_strbuf_append(func_calls, "   efl_pack_end(o, child);\n");
          }
        else if (child->type == EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE)
          {
             eina_strbuf_append_printf(func_calls, "   efl_pack_table(o, child, %d, %d, %d, %d);\n", child->table.x, child->table.y, child->table.w, child->table.h);
          }
        else if (child->type == EFL_UI_NODE_CHILDREN_TYPE_PACK)
          {
             eina_strbuf_append_printf(func_calls, "   efl_content_set(efl_part(o, \"%s\"), child);\n", child->pack.pack);
          }

        eina_strbuf_free(buf);
     }
   eina_iterator_free(children);
   eina_strbuf_append(func_calls, "   child = o;\n");
   eina_strbuf_replace_all(func_calls, "\n", "\n   ");
   eina_strbuf_append(func_calls, "}\n");
   return func_calls;
}

static void
dump_c_file(Eina_Strbuf *func_calls, const char *name, Eina_Strbuf *type)
{
   Eina_Strbuf *buf = eina_strbuf_new();

   eina_strbuf_append(buf, "#define EFL_BETA_API_SUPPORT 1\n");
   eina_strbuf_append(buf, "#include <Efl_Ui.h>\n");
   eina_strbuf_append_printf(buf, "#include \"%s\"\n", ecore_file_file_get(output_h_file));
   eina_strbuf_append_buffer(buf, type);
   eina_strbuf_append(buf, "*\n");
   eina_strbuf_append(buf, name);
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
dump_h_file(Eina_Strbuf *typedef_fields, const char *name, Eina_Strbuf *type)
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
   eina_strbuf_append(buf, name);
   eina_strbuf_append(buf, "_gen(Eo *parent);\n");

   c_header_content = eina_strbuf_release(buf);
}

void
_cgen_transform_value_cb(const Eolian_Type *etype, Eina_Strbuf *buf, const char *value)
{
   const Eolian_Type_Type type = eolian_type_type_get(etype);
   EINA_SAFETY_ON_FALSE_RETURN(type == EOLIAN_TYPE_REGULAR);
   const Eolian_Typedecl *decl = eolian_type_typedecl_get(etype);

   if (decl && eolian_typedecl_type_get(decl) == EOLIAN_TYPEDECL_ENUM)
     {
        const Eolian_Enum_Type_Field *field = eolian_typedecl_enum_field_get(decl, value);
        EINA_SAFETY_ON_NULL_RETURN(field);

        eina_strbuf_append_printf(buf, "%s", eolian_typedecl_enum_field_c_constant_get(field));
     }
   else
     {
         const Eolian_Type_Builtin_Type bt = eolian_type_builtin_type_get(etype);

         if (bt == EOLIAN_TYPE_BUILTIN_BOOL)
           {
              if (eina_streq(value, "true"))
                eina_strbuf_append(buf, "EINA_TRUE");
              else
                eina_strbuf_append(buf, "EINA_FALSE");
           }
     }
}

Eina_Bool
c_output(Eolian_State *s, Efl_Ui *ui)
{
   Eina_Strbuf *typedef_fields = eina_strbuf_new();
   Eina_Strbuf *type = eina_strbuf_new();
   Outputter_Node *start;
   Generator_Context ctx;
   const char *full_case_name;
   char *name;

   ctx.typedef_fields = typedef_fields;

   eina_strbuf_append(typedef_fields, "typedef struct {\n");

   start = outputter_node_init(s, ui, &full_case_name, _cgen_transform_value_cb);
   name = eina_strdup(full_case_name);
   eina_str_tolower(&name);
   eina_strbuf_append(type, full_case_name);
   eina_strbuf_append(type, "_Data");

   Eina_Strbuf *main_func = _output_node(&ctx, start);

   eina_strbuf_append_printf(typedef_fields, "} %s;", eina_strbuf_string_get(type));

   dump_h_file(typedef_fields, name, type);
   dump_c_file(main_func, name, type);

   eina_strbuf_free(type);
   eina_strbuf_free(typedef_fields);
   eina_strbuf_free(main_func);

   outputter_node_root_free(start);

   free(name);

   return EINA_TRUE;
}
