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

static int
_compare_available_types(const void *a, const void *b)
{
   const Predicted_Class *a_klass = a;
   const Predicted_Class *b_klass = b;

   return strcmp(a_klass->klass_name, b_klass->klass_name);
}

Predicted_Class*
get_available_types(void)
{
   if (!all_widgets)
     {
         Eina_Array *w = find_all_widgets(state);
         EINA_SAFETY_ON_NULL_RETURN_VAL(w, NULL);
         all_widgets = calloc(eina_array_count(w) + 1, sizeof(Predicted_Class));
         all_widgets[eina_array_count(w)].klass_name = NULL;
         all_widgets[eina_array_count(w)].obj.documentation = NULL;
         for (int i = 0; i < eina_array_count(w); ++i)
           {
              Eolian_Class *klass = eina_array_data_get(w, i);
              predirect_obj_init(&all_widgets[i].obj, eolian_documentation_description_get(eolian_class_documentation_get(klass)), EINA_FALSE);
              all_widgets[i].klass_name = eolian_class_name_get(klass);
           }
         qsort(all_widgets, eina_array_count(w), sizeof(Predicted_Class), _compare_available_types);
         eina_array_free(w);
     }
   return all_widgets;
}

Predicted_Property*
get_available_properties(Efl_Ui_Node *node)
{
   Eina_Array *tmp_array = find_all_properties(state, node->type);
   EINA_SAFETY_ON_NULL_RETURN_VAL(tmp_array, NULL);
   Predicted_Property *result = calloc(eina_array_count(tmp_array) + 1, sizeof(Predicted_Property));
   result[eina_array_count(tmp_array)].name = NULL;
   for (int i = 0; i < eina_array_count(tmp_array); ++i)
     {
        Eolian_Function *f = eina_array_data_get(tmp_array, i);

        predirect_obj_init(&result[i].obj, "TODO", EINA_TRUE);
        result[i].name = eolian_function_name_get(f);
     }
   eina_array_free(tmp_array);
   return result;
}

Predicted_Property_Details*
get_available_property_details(Efl_Ui_Node *node, const char *property_name)
{
   Eina_Array *tmp_array = find_all_arguments(state, node->type, property_name);
   EINA_SAFETY_ON_NULL_RETURN_VAL(tmp_array, NULL);
   Predicted_Property_Details *details = calloc(eina_array_count(tmp_array) + 1, sizeof(Predicted_Property_Details));
   details[eina_array_count(tmp_array)].name = NULL;
   for (int i = 0; i < eina_array_count(tmp_array); ++i)
     {
         const Eolian_Function_Parameter *param = eina_array_data_get(tmp_array, i);
         const Eolian_Type *type = eolian_parameter_type_get(param);

         predirect_obj_init(&details[i].obj, "TODO", EINA_TRUE);
         details[i].type = type;
         details[i].name = eolian_parameter_name_get(param);
     }
   eina_array_free(tmp_array);
   return details;
}
