#include "main.h"
#define EFL_BETA_API_SUPPORT
#include <Eolian.h>
#include <Eolian_Aux.h>
#include <Eina.h>

struct _Klass_Type {
   const Eolian_Class *klass;
};

static Eolian_State *state;
static Eina_Bool beta_support;

static Eina_Bool
_flush_include_dirs(const void *fdata, void *path, void *data)
{
   eolian_state_directory_add(data, path);

   return EINA_TRUE;
}

void
class_db_init(Eina_Bool support, Eina_Array *include_dirs)
{
   beta_support = support;
   eolian_init();

   state = eolian_state_new();
   if (!eolian_state_system_directory_add(state))
     {
        printf("Error, Adding system directory failed!\n");
        abort();
     }

   eina_array_foreach(include_dirs, _flush_include_dirs, state);
}

Klass_Type*
klass_fetch(const char *klass)
{
   Eina_Strbuf *buf = eina_strbuf_new();
   Klass_Type *t = calloc(1, sizeof(Klass_Type));

   eina_strbuf_append(buf, klass);
   eina_strbuf_tolower(buf);
   eina_strbuf_replace_all(buf, ".", "_");
   eina_strbuf_append(buf, ".eo");

   if (!eolian_state_file_parse(state, eina_strbuf_string_get(buf)))
     {
        printf("Error, file %s could not be parsed for finding %s\n", eina_strbuf_string_get(buf), klass);
        return NULL;
     }

   t->klass = eolian_state_class_by_name_get(state, klass);

   if (!t->klass) return NULL;

   if (eolian_object_is_beta(EOLIAN_OBJECT(t->klass)) && !beta_support)
     return NULL;

   eina_strbuf_free(buf);

   return t;
}

void
klass_free(Klass_Type *t)
{
   free(t);
}

Eina_Bool
klass_property_append(Klass_Type *t, const char *prop, const char *value, Eina_Strbuf *goal)
{
   Eina_List *functions = NULL;
   Eina_List *n;
   Eolian_Implement *impl;

   eolian_aux_class_callables_get(t->klass, &functions, NULL, NULL, NULL);

   EINA_LIST_FOREACH(functions, n, impl)
     {
        Eolian_Function_Type type;
        const Eolian_Function *f;

        f = eolian_implement_function_get(impl, &type);
        if (eina_streq(eolian_function_name_get(f), prop) && (type == EOLIAN_PROP_SET || type == EOLIAN_PROPERTY))
          {
             eina_strbuf_append_printf(goal, "   %s(o, %s);\n", eolian_function_full_c_name_get(f, EOLIAN_PROP_SET), value);
             eina_list_free(functions);
             return EINA_TRUE;
          }
     }
   printf("NOTHING FOR %s could be found for %s\n", prop, eolian_class_name_get(t->klass));
   eina_list_free(functions);
   return EINA_FALSE;
}

void
klass_field_append(Klass_Type *t, const char *field_name, Eina_Strbuf *goal)
{
   eina_strbuf_append_printf(goal, "   %s* %s;\n", eolian_object_c_name_get(EOLIAN_OBJECT(t->klass)), field_name);
}

void
klass_create_instance(Klass_Type *t, const char *field_name, const char *parent, Eina_Strbuf *goal)
{
   if (field_name)
     eina_strbuf_append_printf(goal, "   %s = o = efl_add(%s(), %s);\n", field_name, eolian_class_c_get_function_name_get(t->klass), parent);
   else
     eina_strbuf_append_printf(goal, "   o = efl_add(%s(), %s);\n", eolian_class_c_get_function_name_get(t->klass), parent);
}
