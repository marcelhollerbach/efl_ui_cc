#include <stdlib.h>
#include <stdint.h>
#include <check.h>
#include <Eolian.h>
#include <main.h>
#include <Efl_Ui_Format.h>
#include <predictor.h>
#include <abstract_tree_private.h>

//symbols needed to compile abstract_tree_controller

Eolian_State *editor_state;

void
base_ui_refresh(Efl_Ui *ui)
{

}

void
display_ui_refresh(Efl_Ui *ui)
{

}

char*
json_output(const Eolian_State *s, const Efl_Ui *ui)
{
   return "";
}

Efl_Ui *ui_tree;
static Efl_Ui_Node *content;

//test suite content
void setup(void)
{
   ui_tree = efl_ui_new();
   content = efl_ui_content_get(ui_tree);
   efl_ui_name_set(ui_tree, "<Insert Name Here>");
   node_type_set(content, "Efl.Ui.Button");
}

void teardown(void)
{

}

START_TEST(test_change_type)
{
   //first add a property that gets removed
   add_property(content, "text");
   change_type(content, "Efl.Ui.Box");
   add_property(content, "orientation");
   change_type(content, "Efl.Ui.Button");
}
END_TEST

START_TEST(test_add_linear_child)
{
   //first add a property that gets removed
   change_type(content, "Efl.Ui.Box");
   add_child(content, "Efl.Ui.Button", EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR);
}
END_TEST

START_TEST(test_add_table_child)
{
   //first add a property that gets removed
   change_type(content, "Efl.Ui.Table");
   add_child(content, "Efl.Ui.Button", EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE);
}
END_TEST

START_TEST(test_add_part_child)
{
   //first add a property that gets removed
   add_child(content, "Efl.Ui.Button", EFL_UI_NODE_CHILDREN_TYPE_PACK);
}
END_TEST

START_TEST(test_del_child)
{
   //first add a property that gets removed
   Efl_Ui_Pack_Pack *node = node_pack_node_append(content);

   if (!validate(editor_state, ui_tree))
     abort();

   del_child(content, node->basic.node);
}
END_TEST

START_TEST(set_all_properties_on_all_types_true)
{
   //first add a property that gets removed
   const Predicted_Class *klasses = get_available_types();
   eolian_bridge_beta_allowed_set(EINA_TRUE);

   for (int i = 0; klasses[i].klass_name; ++i)
     {
        change_type(content, klasses[i].klass_name);
        const Predicted_Property *properties;
        properties = get_available_properties(content);

        for (int f = 0; properties[f].name; ++f)
          {
             add_property(content, properties[f].name);
          }

        for (int f = 0; properties[f].name; ++f)
          {
             del_property(content, properties[f].name);
          }
        free((void*)properties);
     }
}
END_TEST

START_TEST(set_all_properties_on_all_types_false)
{
   //first add a property that gets removed
   Eina_Hash *string_already_checked = eina_hash_string_small_new(NULL);
   const Predicted_Class *klasses = get_available_types();
   eolian_bridge_beta_allowed_set(EINA_FALSE);

   for (int i = 0; klasses[i].klass_name; ++i)
     {
        change_type(content, klasses[i].klass_name);
        const Predicted_Property *properties;
        properties = get_available_properties(content);

        for (int f = 0; properties[f].name; ++f)
          {
             if (eina_hash_find(string_already_checked, properties[f].name))
               continue;
             add_property(content, properties[f].name);
          }

        for (int f = 0; properties[f].name; ++f)
          {
             if (eina_hash_find(string_already_checked, properties[f].name))
               continue;
             del_property(content, properties[f].name);
             eina_hash_add(string_already_checked, properties[f].name, content);
          }
        free((void*)properties);
     }
}
END_TEST

Suite * predictor_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Test Abstract Controller");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_change_type);
    tcase_add_test(tc_core, test_add_linear_child);
    tcase_add_test(tc_core, test_add_table_child);
    tcase_add_test(tc_core, test_add_part_child);
    tcase_add_test(tc_core, test_del_child);
    tcase_add_test(tc_core, set_all_properties_on_all_types_true);
    tcase_add_test(tc_core, set_all_properties_on_all_types_false);
    tcase_set_timeout(tc_core, 100.0);

    tcase_add_checked_fixture(tc_core, setup, teardown);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

   eolian_init();
   editor_state = eolian_state_new();
   eolian_state_system_directory_add(editor_state);
   eolian_bridge_beta_allowed_set(EINA_FALSE);

   eina_log_abort_on_critical_level_set(3);
   eina_log_abort_on_critical_set(EINA_TRUE);

   predictor_init(editor_state);

    s = predictor_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
