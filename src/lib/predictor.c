#include "Efl_Ui_Format.h"
#include "Internal.h"
#include "abstract_tree_private.h"
#include "predictor.h"

static Predicted_Class *all_widgets = NULL;
static Eolian_State *state;

void
predictor_init(Eolian_State *s)
{
   state = s;
   eolian_state_all_eo_files_parse(state);
}

static void
predirect_obj_init(Predicted_Object *obj, const char *doc, Eina_Bool freeable)
{
   obj->documentation = doc;
   obj->internal = (void*)(intptr_t)freeable;
}

const Predicted_Class*
get_available_types(void)
{
   if (!all_widgets)
     {
         Eina_Array *w = find_all_widgets(state);
         all_widgets = calloc(eina_array_count(w) + 1, sizeof(Predicted_Class));
         all_widgets[eina_array_count(w)].klass_name = NULL;
         all_widgets[eina_array_count(w)].obj.documentation = NULL;
         for (int i = 0; i < eina_array_count(w); ++i)
           {
              Eolian_Class *klass = eina_array_data_get(w, i);
              predirect_obj_init(&all_widgets[i].obj, eolian_documentation_description_get(eolian_class_documentation_get(klass)), EINA_FALSE);
              all_widgets[i].klass_name = eolian_class_name_get(klass);
           }
         eina_array_free(w);
     }
   return all_widgets;
}

const Predicted_Property*
get_available_properties(Efl_Ui_Node *node)
{
   Eina_Array *tmp_array = find_all_properties(state, node->type);
   Predicted_Property *result = calloc(eina_array_count(tmp_array) + 1, sizeof(Predicted_Property));
   result[eina_array_count(tmp_array)].name = NULL;
   for (int i = 0; i < eina_array_count(tmp_array); ++i)
     {
        Eolian_Function *f = eina_array_data_get(tmp_array, i);

        predirect_obj_init(&result[i].obj, "TODO", EINA_TRUE);
        result[i].name = eolian_function_name_get(f);
     }
   return result;
}

const Predicted_Property_Details*
get_available_property_details(Efl_Ui_Node *node, const char *property_name)
{
   Eina_Array *tmp_array = find_all_arguments(state, node->type, property_name);
   Predicted_Property_Details *details = calloc(eina_array_count(tmp_array) + 1, sizeof(Predicted_Property_Details));
   details[eina_array_count(tmp_array)].name = NULL;
   for (int i = 0; i < eina_array_count(tmp_array); ++i)
     {
         const Eolian_Function_Parameter *param = eina_array_data_get(tmp_array, i);
         const Eolian_Type *type = eolian_parameter_type_get(param);
         Eolian_Type_Builtin_Type builtin = eolian_type_builtin_type_get(type);
         Eolian_Type_Type ttype = eolian_type_type_get(type);

         predirect_obj_init(&details[i].obj, "TODO", EINA_TRUE);
         if (builtin >= EOLIAN_TYPE_BUILTIN_BYTE && builtin <= EOLIAN_TYPE_BUILTIN_PTRDIFF)
           details[i].type = TYPE_NUMBER;
         else if (builtin >= EOLIAN_TYPE_BUILTIN_MSTRING && builtin <= EOLIAN_TYPE_BUILTIN_STRINGSHARE)
           details[i].type = TYPE_STRING;
         else if (builtin == EOLIAN_TYPE_BUILTIN_BOOL)
           details[i].type = TYPE_BOOL;
         else if (ttype != EOLIAN_TYPE_UNKNOWN_TYPE)
           details[i].type = TYPE_OBJECT;
         details[i].name = eolian_parameter_name_get(param);
     }

   return details;
}