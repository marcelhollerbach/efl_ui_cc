#define EFL_BETA_API_SUPPORT 1
#include <Efl.h>
#include <Efl_Ui.h>
#include <Eolian.h>
#include "efl_ui_grid_view.eo.h"
#include "main.h"
#include "base_ui.h"
#include "predictor.h"

Eolian_State *editor_state = NULL;
Eina_Bool beta_support = EINA_FALSE;
Eina_Bool test_mode = EINA_FALSE;
Eina_Bool format_mode = EINA_FALSE;
const char *output_file = NULL;
Efl_Ui_Win *win;

static void
_exit_cb(void *data, const Efl_Event *ev)
{
   if (getenv("TEST_IN_TREE"))
     efl_loop_quit(efl_main_loop_get(), eina_value_int_init(0));
}

EAPI_MAIN void
efl_main(void *data EINA_UNUSED, const Efl_Event *ev EINA_UNUSED)
{
   Eina_Accessor *cma;
   const char *argument, *input_file = NULL;
   int c = 1;

   eolian_init();
   editor_state = eolian_state_new();
   cma = efl_core_command_line_command_access(efl_main_loop_get());
   EINA_ACCESSOR_FOREACH(cma, c, argument)
     {
        if (c == 0) continue;
        if (!strcmp(argument, "--beta-support"))
          {
             beta_support = EINA_TRUE;
          }
        else if (!strcmp(argument, "--test-mode"))
          {
             test_mode = EINA_TRUE;
          }
        else if (!strcmp(argument, "--format"))
          {
             format_mode = EINA_TRUE;
          }
        else if (!strcmp(argument, "-I"))
          {
             const char *include_dir = NULL;

             c++;
             eina_accessor_data_get(cma, c, (void**)&include_dir);

             if (!access(include_dir, F_OK))
               {
                  eolian_state_directory_add(editor_state, include_dir);
               }
             else
               {
                  printf("Directory %s cannot be accessed!\n", include_dir);
                  abort();
               }
          }
        else if (!strcmp(argument, "-o"))
          {
             c++;
             eina_accessor_data_get(cma, c, (void**)&output_file);
          }
        else if (!input_file)
          {
             if (!access(argument, F_OK))
               {
                 input_file = argument;
               }
             else
               {
                  FILE *fptr = fopen(argument, "wb");
                  fwrite("\n", 1, 1, fptr);
                  fclose(fptr);

                  if (!access(argument, F_OK))
                    {
                       input_file = argument;
                    }
                  else
                    {
                       printf("File %s cannot be accessed\n", input_file);
                       abort();
                    }
               }
          }
        else
          {
             printf("Not understood token : %s\n", argument);
             abort();
          }
     }
   eina_accessor_free(cma);
   if (!input_file)
     {
        printf("No file given\n");
        abort();
     }
   eolian_bridge_beta_allowed_set(beta_support);
   if (!eolian_state_system_directory_add(editor_state))
     {
        printf("Error, Adding system directory failed!\n");
        abort();
     }
   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
                 efl_text_set(efl_added, "EFL UI Editor"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE)
                );
   efl_ui_win_exit_on_close_set(win, eina_value_int_new(1));
   if (test_mode)
     {
        eina_log_abort_on_critical_level_set(3);
        eina_log_abort_on_critical_set(EINA_TRUE);
        efl_event_callback_add(win, EFL_CANVAS_SCENE_EVENT_RENDER_POST, _exit_cb, NULL);
     }
   else if (format_mode)
     {
        base_ui_init(win);
        display_ui_init(win);
        file_set(input_file, output_file ? output_file : input_file);
        safe_file();
        efl_loop_quit(efl_main_loop_get(), eina_value_int_init(0));
     }
   else
     {
        predictor_init(editor_state);
        base_ui_init(win);
        display_ui_init(win);
        file_set(input_file, output_file ? output_file : input_file);
        efl_gfx_entity_size_set(win, EINA_SIZE2D(600, 400));
     }
}
EFL_MAIN()
