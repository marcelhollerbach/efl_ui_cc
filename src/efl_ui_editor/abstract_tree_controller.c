#include <main.h>
#include <Efl_Ui_Format.h>
#include <abstract_tree_private.h>
#include "Internal.h"
#include "predictor.h"

static Efl_Ui *ui_tree;
static Eina_Stringshare *path = NULL;

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

   EINA_SAFETY_ON_NULL_RETURN(klass);

   free((char*)node->type);
   node_type_set(node, type);
   int shifter = 0;

   for (int i = 0; eina_array_count(node->properties); ++i)
     {
        Efl_Ui_Property *prop = eina_array_data_get(node->properties, i);

        if (!find_function(editor_state, klass, prop->key))
          shifter ++;
        if (shifter)
          {
             Efl_Ui_Property *propn = eina_array_data_get(node->properties, i+shifter);
             eina_array_data_set(node->properties, i, propn);
          }
     }
    node->properties->count -= shifter;

    //FIXME remove children if needed
    node->usage_type = fetch_usage(editor_state, type);

    EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
    propagate_tree_change();
}

void
add_child(Efl_Ui_Node *node, const char *klass_name)
{
   Efl_Ui_Node *new_node;
   const Eolian_Class *klass = find_klass(editor_state, klass_name);

   EINA_SAFETY_ON_NULL_RETURN(klass);

   switch(node_child_type_get(node))
     {
        case EFL_UI_NODE_CHILDREN_TYPE_PACK: {
          Efl_Ui_Pack_Pack *pack = node_pack_node_append(node);
          new_node = pack->node;
        }
        break;
        case EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR: {
          Efl_Ui_Pack_Linear *linear = node_pack_linear_node_append(node);
          new_node = linear->node;
        }
        break;
        case EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE: {
          Efl_Ui_Pack_Table *table = node_pack_table_node_append(node);
          new_node = table->node;
        }
        break;
        default:
          EINA_LOG_ERR("This is not supposed to happen");
          return;
        break;
     }
   node_type_set(new_node, klass_name);
   new_node->usage_type = fetch_usage(editor_state, klass_name);
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

void
del_child(Efl_Ui_Node *node, Efl_Ui_Node *child)
{
   int shifter = 0;
   void *to_free1 = NULL, *to_free2 = NULL;
   for (int i = 0; i < eina_array_count(node->children); ++i)
     {
        Efl_Ui_Node *n;
        void *f;
        switch(node->usage_type)
          {
             case EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR: {
                Efl_Ui_Pack_Linear *linear = f = eina_array_data_get(node->children, i);
                n = linear->node;
             }
             break;
             case EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE: {
                Efl_Ui_Pack_Table *table = f = eina_array_data_get(node->children, i);
                n = table->node;
             }
             break;
             case EFL_UI_NODE_CHILDREN_TYPE_PACK: {
                Efl_Ui_Pack_Pack *pack = f = eina_array_data_get(node->children, i);
                n = pack->node;
             }
             default: /*WAD*/
             break;
          }
        if (n == child)
          {
             to_free1 = n;
             to_free2 = f;
             shifter ++;
          }
        if (shifter)
          {
             void *n = NULL;

             if (i + shifter < eina_array_count(node->children))
               n = eina_array_data_get(node->children, i + shifter);

             eina_array_data_set(node->children, i, n);
          }
     }
   node->children->count -= shifter;
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
   efl_ui_node_free(to_free1);
   free(to_free2);
}

void
add_property(Efl_Ui_Node *node, const char *prop_name)
{
   Efl_Ui_Property *property;
   const Predicted_Property_Details *details;

   property = node_property_append(node);
   property_key_set(property, prop_name);

   details = get_available_property_details(node, prop_name);

   for (int i = 0; details[i].name; ++i)
     {
        Efl_Ui_Property_Value *value = property_value_append(property);
        const Eolian_Type *etype = details[i].type;
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
     }
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

void
del_property(Efl_Ui_Node *node, const char *prop_name)
{
   int shifter = 0;
   Efl_Ui_Property *p;
   for (int i = 0; i < eina_array_count(node->properties); ++i)
     {
        Efl_Ui_Property *prop = eina_array_data_get(node->properties, i);

        if (!strcmp(prop->key, prop_name))
          {
             p = prop;
             shifter ++;
          }

        if (shifter)
          {
             void *n = NULL;

             if (i + shifter < eina_array_count(node->properties))
               n = eina_array_data_get(node->properties, i + shifter);

             eina_array_data_set(node->properties, i, n);
          }
     }
   node->properties->count -= shifter;
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
   free(p);
}

void
change_parameter_type(Efl_Ui_Property_Value *value, const char *v)
{
   if (value->value)
     free((char*)value->value);
   property_value_value(value, v);
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   propagate_tree_change();
}

static void
load_content(void)
{
   Eina_File *file;
   void *input_content;

   file = eina_file_open(path, EINA_FALSE);
   input_content = eina_file_map_all(file, EINA_FILE_SEQUENTIAL);
   ui_tree  = efl_ui_format_parse(input_content);
   if (!ui_tree) //empty file
     {
        Efl_Ui_Node *content;
        ui_tree = efl_ui_new();
        content = efl_ui_content_get(ui_tree);
        efl_ui_name_set(ui_tree, "<Insert Name Here>");
        node_type_set(content, "Efl.Ui.Button");
        content->usage_type = EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR;
     }
   EINA_SAFETY_ON_FALSE_RETURN(validate(editor_state, ui_tree));
   base_ui_refresh(ui_tree);
   display_ui_refresh(ui_tree);
}

void
file_set(const char *file)
{
   eina_stringshare_replace(&path, file);
   load_content();
}
