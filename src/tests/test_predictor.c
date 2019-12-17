#include <stdlib.h>
#include <stdint.h>
#include <check.h>
#include <predictor.h>
#include <abstract_tree_private.h>

void setup(void)
{
   eolian_init();
   Eolian_State *state = eolian_state_new();
   eolian_state_system_directory_add(state);
   eolian_bridge_beta_allowed_set(EINA_FALSE);
   predictor_init(state);
}

void teardown(void)
{

}

START_TEST(test_predictor_klasses)
{
   const Predicted_Class *klasses = get_available_types();
   int classes = 0;

   for (int i = 0; klasses[i].klass_name; ++i)
     {
        classes++;
     }
   ck_assert_int_ge(classes, 10);

}
END_TEST

START_TEST(test_predictor_properties)
{
   const Predicted_Property *props;
   int properties;
   Efl_Ui_Node n;

   n.type = "Efl.Ui.Button";
   props = get_available_properties(&n);
   properties = 0;

   for (int i = 0; props[i].name; ++i)
     {
        properties++;
     }
   ck_assert_int_ge(properties, 123);

}
END_TEST

START_TEST(test_predictor_properties_details)
{
   const Predicted_Property_Details *details;
   int details_argument_count;
   Efl_Ui_Node n;

   n.type = "Efl.Ui.Button";
   details = get_available_property_details(&n, "disabled");
   details_argument_count = 0;

   for (int i = 0; details[i].name; ++i)
     {
        details_argument_count++;
        ck_assert_ptr_ne(details[i].type, NULL);
     }
   ck_assert_int_eq(details_argument_count, 1);

}
END_TEST
Suite * predictor_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Test Predictor");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_predictor_klasses);
    tcase_add_test(tc_core, test_predictor_properties);
    tcase_add_test(tc_core, test_predictor_properties_details);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = predictor_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
