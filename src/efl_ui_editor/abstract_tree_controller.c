#include <main.h>
#include <abstract_tree_private.h>
#include "Internal.h"
#include "predictor.h"

static Efl_Ui *ui_tree;
static Eina_Stringshare *input = NULL, *output = NULL;

static void
propagate_tree_change(void)
{
   base_ui_refresh(ui_tree);
   display_ui_refresh(ui_tree);
}

void
change_type(Efl_Ui_Node *node, const char *type)
{
   const Eolian_Class *klass = find_klass(editor_state, type);
   Eina_Array *stack = eina_array_new(10);
   enum Efl_Ui_Node_Children_Type available = fetch_usage(editor_state, klass);

   EINA_SAFETY_ON_NULL_RETURN(klass);

   node_type_set(node, type);

   for (int i = 0; i < eina_array_count(node->properties); ++i)
     {
        Efl_Ui_Property *prop = eina_array_data_get(node->properties, i);

        if (!find_function(editor_state, klass, prop->key))
          eina_array_push(stack, prop->key);
     }

   while(eina_array_count(stack) > 0)
     node_property_remove(node, eina_array_pop(stack));

   if ((available & EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR) == 0)
     node_delete_children(node, EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR);
   if ((available & EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE) == 0)
     node_delete_children(node, EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE);
   if ((available & EFL_UI_NODE_CHILDREN_TYPE_PACK) == 0)
     node_delete_children(node, EFL_UI_NODE_CHILDREN_TYPE_PACK);

   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

void
add_child(Efl_Ui_Node *node, const char *klass_name, enum Efl_Ui_Node_Children_Type type)
{
   Efl_Ui_Node *new_node;
   const Eolian_Class *klass = find_klass(editor_state, klass_name);

   EINA_SAFETY_ON_NULL_RETURN(klass);

   switch(type)
     {
        case EFL_UI_NODE_CHILDREN_TYPE_PACK: {
          Efl_Ui_Pack_Pack *pack = node_pack_node_append(node);
          new_node = pack->basic.node;
        }
        break;
        case EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR: {
          Efl_Ui_Pack_Linear *linear = node_pack_linear_node_append(node);
          new_node = linear->basic.node;
        }
        break;
        case EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE: {
          Efl_Ui_Pack_Table *table = node_pack_table_node_append(node);
          new_node = table->basic.node;
        }
        break;
        default:
          EINA_LOG_ERR("This is not supposed to happen");
          return;
        break;
     }
   node_type_set(new_node, klass_name);
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

void
del_child(Efl_Ui_Node *node, Efl_Ui_Node *child)
{
   node_child_remove(node, child);
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

static void
_insert_values(const Eolian_Type *etype, Efl_Ui_Property_Value *value)
{
   const Eolian_Type_Builtin_Type btype = eolian_type_builtin_type_get(etype);
   const Eolian_Class *klass = eolian_type_class_get(etype);
   const Eolian_Typedecl *decl = eolian_type_typedecl_get(etype);

   if (klass)
     {
        Efl_Ui_Node *node = property_value_node(value);
        node_type_set(node, "Efl.Ui.Button");
     }
   else if (btype >= EOLIAN_TYPE_BUILTIN_BYTE && btype <= EOLIAN_TYPE_BUILTIN_UINT128)
     {
        property_value_value(value, "0");
     }
   else if (btype >= EOLIAN_TYPE_BUILTIN_FLOAT && btype <= EOLIAN_TYPE_BUILTIN_DOUBLE)
     {
        property_value_value(value, "0.0");
     }
   else if (btype == EOLIAN_TYPE_BUILTIN_BOOL)
     {
        property_value_value(value, "false");
     }
   else if (btype == EOLIAN_TYPE_BUILTIN_MSTRING || btype == EOLIAN_TYPE_BUILTIN_STRING || btype == EOLIAN_TYPE_BUILTIN_STRINGSHARE)
     {
        property_value_value(value, "<Empty>");
     }
   else if (decl && eolian_typedecl_type_get(decl) == EOLIAN_TYPEDECL_ENUM)
     {
        Eolian_Enum_Type_Field *field = NULL;
        Eina_Iterator *fields = eolian_typedecl_enum_fields_get(decl);
        EINA_SAFETY_ON_FALSE_RETURN(eina_iterator_next(fields, (void**) &field));
        property_value_value(value, eolian_typedecl_enum_field_name_get(field));
     }
   else if (decl && eolian_typedecl_type_get(decl) == EOLIAN_TYPEDECL_STRUCT)
     {
        Eina_Iterator *fields = eolian_typedecl_struct_fields_get(decl);
        Eolian_Struct_Type_Field *field;

        Efl_Ui_Struct *str = property_value_struct(value);

        EINA_ITERATOR_FOREACH(fields, field)
          {
             Efl_Ui_Property_Value *val = property_struct_value_append(str);
             _insert_values(eolian_typedecl_struct_field_type_get(field), val);
          }
        eina_iterator_free(fields);
     }
}

void
add_property(Efl_Ui_Node *node, const char *prop_name)
{
   Efl_Ui_Property *property;
   Predicted_Property_Details *details;

   property = node_property_append(node);
   property_key_set(property, prop_name);

   details = get_available_property_details(node, prop_name);

   for (int i = 0; details[i].name; ++i)
     {
        Efl_Ui_Property_Value *value = property_value_append(property);
        const Eolian_Type *etype = details[i].type;

        _insert_values(etype, value);
     }
   free(details);
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

void
del_property(Efl_Ui_Node *node, const char *prop_name)
{
   node_property_remove(node, prop_name);
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

void
change_id(Efl_Ui_Node *node, const char *new_id)
{
   node_id_set(node, new_id);
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

void
change_parameter_type(Efl_Ui_Property_Value *value, const char *v)
{
   property_value_value(value, v);
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

static void
load_content(void)
{
   Eina_File *file;
   void *input_content;

   file = eina_file_open(input, EINA_FALSE);
   input_content = eina_file_map_all(file, EINA_FILE_SEQUENTIAL);
   ui_tree  = efl_ui_format_parse(input_content);
   if (!ui_tree) //empty file
     {
        Efl_Ui_Node *content;
        ui_tree = efl_ui_new();
        content = efl_ui_content_get(ui_tree);
        efl_ui_name_set(ui_tree, "<Insert Name Here>");
        node_type_set(content, "Efl.Ui.Button");
     }
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   base_ui_refresh(ui_tree);
   display_ui_refresh(ui_tree);
}

void
file_set(const char *input_file, const char *output_file)
{
   eina_stringshare_replace(&input, input_file);
   eina_stringshare_replace(&output, output_file);
   load_content();
}

void
safe_file(void)
{
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   char *json = json_output(editor_state, ui_tree);
   EINA_SAFETY_ON_NULL_RETURN(json);

   FILE *f = fopen(output, "w+");
   if (!f)
     {
        printf("Failed to open file at %s\n", output);
        return;
     }
   if (!fwrite(json, strlen(json), 1, f))
     {
        printf("Failed to write file: %s\n", strerror(ferror(f)));
        return;
     }
   fclose(f);

   printf("File stored in %s\n", output);
}
