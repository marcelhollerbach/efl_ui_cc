#define EFL_BETA_API_SUPPORT 1
#include <Efl_Ui.h>
#include "simple_button.h"

static void
_clicked_cb(void *data, const Efl_Event *ev)
{
   printf("The button is clicked\n");
}

static void
_quit_clicked_cb(void *data, const Efl_Event *ev)
{
   printf("Goodbye\n");
   efl_loop_quit(efl_main_loop_get(), EINA_VALUE_EMPTY);
}

EAPI_MAIN void
efl_main(void *data EINA_UNUSED, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
                 efl_text_set(efl_added, "Simple Button"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE)
                );


   Simple_Button_Data *pd = simple_button_gen(win);
   efl_event_callback_add(pd->btn, EFL_INPUT_EVENT_CLICKED, _clicked_cb, NULL);
   efl_event_callback_add(pd->quit, EFL_INPUT_EVENT_CLICKED, _quit_clicked_cb, NULL);
   efl_content_set(win, pd->root);
}
EFL_MAIN()
