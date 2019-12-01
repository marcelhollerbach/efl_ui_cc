#define EFL_BETA_API_SUPPORT 1
#include <Efl.h>
#include <Efl_Ui.h>
#include <Eolian.h>
#include "efl_ui_grid_view.eo.h"
#include "main.h"

Eolian_State *editor_state = NULL;
Eina_Bool beta_support = EINA_FALSE;
const char *input_file = NULL;
Efl_Ui *ui_tree = NULL;

static void
load_content(void)
{
   Eina_File *file;
   void *input_content;

   file = eina_file_open(input_file, EINA_FALSE);
   input_content = eina_file_map_all(file, EINA_FILE_SEQUENTIAL);
   ui_tree  = efl_ui_format_parse(input_content);
   json_output(editor_state, ui_tree);
}

EAPI_MAIN void
efl_main(void *data EINA_UNUSED, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Win *win;
   Eina_Accessor *cma;
   const char *argument;
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
        else if (!strcmp(argument, "-I"))
          {
             if (!access(argument, F_OK))
               {
                  eolian_state_directory_add(editor_state, argument);
               }
             else
               {
                  printf("Directory %s cannot be accessed!\n", argument);
                  abort();
               }
          }
        else if (!input_file)
          {
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
        else
          {
             printf("Not understood token : %s\n", argument);
             abort();
          }
     }
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

   load_content();
   efl_content_set(win, object_generator(win, editor_state, ui_tree));
}
EFL_MAIN()
