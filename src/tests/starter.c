#define EFL_BETA_API_SUPPORT 1
#include <Efl_Ui.h>
#include TEST_FILE

static void
_ticker(void *data, const Efl_Event *ev)
{
   efl_loop_quit(efl_main_loop_get(), EINA_VALUE_EMPTY);
}

EAPI_MAIN void
efl_main(void *data EINA_UNUSED, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win;

   eina_log_abort_on_critical_level_set(3);
   eina_log_abort_on_critical_set(EINA_TRUE);

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
                 efl_text_set(efl_added, "Efl.Ui.Radio example"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE)
                );

   testcase_Data *d = testcase_gen(win);

   efl_content_set(win, d->basic);

   if (getenv("TEST_IN_TREE"))
     efl_add(EFL_LOOP_TIMER_CLASS, win,
             efl_loop_timer_interval_set(efl_added, 3.0),
             efl_event_callback_add(efl_added, EFL_LOOP_TIMER_EVENT_TIMER_TICK, _ticker, NULL));
}
EFL_MAIN()
