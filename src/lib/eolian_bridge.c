#include "Efl_Ui_Format.h"
#include "Internal.h"
#define EFL_BETA_API_SUPPORT
#include <Eolian_Aux.h>

static Eina_Bool _beta_allowed = EINA_FALSE;
static Eina_Hash *klass_hash = NULL;

void
eolian_bridge_beta_allowed_set(Eina_Bool beta_allowed)
{
   _beta_allowed = beta_allowed;
}

static Eina_Bool
is_allowed(const Eolian_Object *obj)
{
   if (!_beta_allowed)
     {
        return !eolian_object_is_beta(obj);
     }
   return EINA_TRUE;
}

const Eolian_Class*
find_klass(Eolian_State *s, const char *klass)
{
   Eina_Strbuf *buf;
   const Eolian_Class *k;

   if (!klass_hash)
     klass_hash = eina_hash_string_small_new(NULL);

   k = eina_hash_find(klass_hash, &klass);
   if (k) return k;

   buf = eina_strbuf_new();
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

   k = eolian_state_class_by_name_get(s, klass);
   eina_hash_add(klass_hash, &klass, k);
   return k;
}

const Eolian_Function*
find_function(Eolian_State *s, const Eolian_Class *klass, const char *prop)
{
   Eina_List *functions = NULL;
   Eina_List *n;
   Eolian_Implement *impl;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(is_allowed(EOLIAN_OBJECT(klass)), NULL);

   eolian_aux_class_callables_get(klass, &functions, NULL, NULL, NULL);

   EINA_LIST_FOREACH(functions, n, impl)
     {
        Eolian_Function_Type type;
        const Eolian_Function *f;

        EINA_SAFETY_ON_FALSE_RETURN_VAL(is_allowed(EOLIAN_OBJECT(impl)), NULL);

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
   const Eolian_Class *c;
   do
     {
        if (a == b)
          return EINA_TRUE;
        Eina_Iterator *iter;

        iter = eolian_class_extensions_get(a);
        EINA_ITERATOR_FOREACH(iter, c)
          {
             if (eolian_class_isa(c, b))
               {
                  eina_iterator_free(iter);
                  return EINA_TRUE;
               }
          }
        eina_iterator_free(iter);

        a = eolian_class_parent_get(a);
   } while (a);
   return EINA_FALSE;
}

Eina_Array*
find_all_widgets(Eolian_State *state)
{
   Eina_Array *result;
   Eina_Iterator *it;
   const Eolian_Class *widget, *win;
   Eolian_Class *klass;

   widget = eolian_state_class_by_name_get(state, "Efl.Ui.Widget");
   win = eolian_state_class_by_name_get(state, "Efl.Ui.Win");
   EINA_SAFETY_ON_NULL_RETURN_VAL(widget, NULL);
   result = eina_array_new(10);
   it = eolian_state_classes_get(state);

   EINA_ITERATOR_FOREACH(it, klass)
     {
        if (eolian_class_isa(klass, win)) continue;
        if (eolian_class_type_get(klass) == EOLIAN_CLASS_REGULAR &&
            eolian_class_isa(klass, widget) &&
            is_allowed(EOLIAN_OBJECT(klass)))
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
   EINA_SAFETY_ON_FALSE_RETURN_VAL(is_allowed(EOLIAN_OBJECT(klass)), NULL);
   eolian_aux_class_callables_get(klass, &functions, NULL, NULL, NULL);

   EINA_LIST_FOREACH(functions, n, impl)
     {
        Eolian_Function_Type type;
        const Eolian_Function *f;

        if (!is_allowed(EOLIAN_OBJECT(impl)))
          continue;

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

   widget = eolian_state_class_by_name_get(state, "Efl.Ui.Widget");
   klass = find_klass(state, klass_name);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eolian_class_isa(klass, widget), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eolian_class_type_get(klass) == EOLIAN_CLASS_REGULAR, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(is_allowed(EOLIAN_OBJECT(klass)), NULL);
   f = find_function(state, klass, property);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(is_allowed(EOLIAN_OBJECT(f)), NULL);
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

enum Efl_Ui_Node_Children_Type
fetch_usage(Eolian_State *state, const char *klass)
{
   const Eolian_Class *eolian_klass = find_klass(state, klass);
   const Eolian_Class *pack_linear = find_klass(state, "Efl.Pack_Linear");
   const Eolian_Class *pack_table = find_klass(state, "Efl.Pack_Table");
   const Eolian_Class *pack_part = find_klass(state, "Efl.Part");

   if (eolian_class_isa(eolian_klass, pack_linear))
     return EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR;

   if (eolian_class_isa(eolian_klass, pack_table))
     return EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE;

   if (eolian_class_isa(eolian_klass, pack_part))
     return EFL_UI_NODE_CHILDREN_TYPE_PACK;

   return EFL_UI_NODE_CHILDREN_TYPE_NOTHING;
}
