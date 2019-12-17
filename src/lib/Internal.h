const Eolian_Class* find_klass(Eolian_State *s, const char *klass);
const Eolian_Function* find_function(Eolian_State *s, const Eolian_Class *klass, const char *prop);
Eina_Array* find_all_widgets(Eolian_State *state);
Eina_Array* find_all_properties(Eolian_State *state, const char *klass_name);
Eina_Array* find_all_arguments(Eolian_State *state, const char *klass_name, const char *property);
enum Efl_Ui_Node_Children_Type fetch_usage(Eolian_State *state, const char *klass);
