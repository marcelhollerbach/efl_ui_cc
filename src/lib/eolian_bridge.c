#include "Efl_Ui_Format.h"
#include "Internal.h"
#define EFL_BETA_API_SUPPORT
#include <Eolian_Aux.h>

const Eolian_Class*
find_klass(Eolian_State *s, const char *klass)
{
   Eina_Strbuf *buf = eina_strbuf_new();

   eina_strbuf_append(buf, klass);
   eina_strbuf_tolower(buf);
   eina_strbuf_replace_all(buf, ".", "_");
   eina_strbuf_append(buf, ".eo");

   if (!eolian_state_file_parse(s, eina_strbuf_string_get(buf)))
     {
        printf("Error, file %s could not be parsed for finding %s\n", eina_strbuf_string_get(buf), klass);
        return NULL;
     }
   eina_strbuf_free(buf);

   return eolian_state_class_by_name_get(s, klass);
}

const Eolian_Function*
find_function(Eolian_State *s, const Eolian_Class *klass, const char *prop)
{
   Eina_List *functions = NULL;
   Eina_List *n;
   Eolian_Implement *impl;

   eolian_aux_class_callables_get(klass, &functions, NULL, NULL, NULL);

   EINA_LIST_FOREACH(functions, n, impl)
     {
        Eolian_Function_Type type;
        const Eolian_Function *f;

        f = eolian_implement_function_get(impl, &type);
        if (eina_streq(eolian_function_name_get(f), prop) && (type == EOLIAN_PROP_SET || type == EOLIAN_PROPERTY))
          {
             return f;
          }
     }
   eina_list_free(functions);
   return NULL;
}
