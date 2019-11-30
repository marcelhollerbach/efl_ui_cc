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

static Eina_Bool
eolian_class_isa(const Eolian_Class *a, const Eolian_Class *b)
{
   do
     {
        if (a == b)
          return EINA_TRUE;
        a = eolian_class_parent_get(a);
   } while (a);
   return EINA_FALSE;
}

Eina_Array*
find_all_widgets(Eolian_State *state)
{
   Eina_Array *result;
   Eina_Iterator *it;
   const Eolian_Class *widget;
   Eolian_Class *klass;


   widget = eolian_state_class_by_name_get(state, "Efl.Ui.Widget");
   EINA_SAFETY_ON_NULL_RETURN_VAL(widget, NULL);
   result = eina_array_new(10);
   it = eolian_state_classes_get(state);

   EINA_ITERATOR_FOREACH(it, klass)
     {
        if (eolian_class_type_get(klass) == EOLIAN_CLASS_REGULAR &&
            eolian_class_isa(klass, widget))
          eina_array_push(result, klass);
     }

   return result;
}

Eina_Array*
find_all_properties(Eolian_State *state, const char *klass_name)
{
   const Eolian_Class *klass;
   const Eolian_Class *widget;
   Eina_List *functions = NULL;
   Eina_List *n;
   Eolian_Implement *impl;
   Eina_Array *result;

   result = eina_array_new(10);
   klass = find_klass(state, klass_name);
   widget = eolian_state_class_by_name_get(state, "Efl.Ui.Widget");
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eolian_class_isa(klass, widget), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eolian_class_type_get(klass) == EOLIAN_CLASS_REGULAR, NULL);
   eolian_aux_class_callables_get(klass, &functions, NULL, NULL, NULL);

   EINA_LIST_FOREACH(functions, n, impl)
     {
        Eolian_Function_Type type;
        const Eolian_Function *f;

        f = eolian_implement_function_get(impl, &type);
        if ((type == EOLIAN_PROP_SET || type == EOLIAN_PROPERTY))
          {
             eina_array_push(result, f);
          }
     }
   eina_list_free(functions);
   return result;
}

Eina_Array*
find_all_arguments(Eolian_State *state, const char *klass_name, const char *property)
{
   const Eolian_Class *klass;
   const Eolian_Class *widget;
   const Eolian_Function *f;
   Eina_Iterator *iter;
   Eina_Array *result;
   Eolian_Function_Parameter *p;

   klass = find_klass(state, klass_name);
   widget = eolian_state_class_by_name_get(state, "Efl.Ui.Widget");
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eolian_class_isa(klass, widget), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eolian_class_type_get(klass) == EOLIAN_CLASS_REGULAR, NULL);
   f = find_function(state, klass, property);
   EINA_SAFETY_ON_NULL_RETURN_VAL(f, NULL);
   iter = eolian_property_values_get(f, EOLIAN_PROP_SET);
   result = eina_array_new(10);

   EINA_ITERATOR_FOREACH(iter, p)
     {
         eina_array_push(result, p);
     }
   eina_iterator_free(iter);

   return result;
}
