#include "main.h"
#define EFL_BETA_API_SUPPORT
#include <Eina.h>
#include <Eolian.h>
#include <unistd.h>
#include <Efl_Ui_Format.h>

const char *input_file = NULL;
const char *output_c_file = NULL;
const char *output_h_file = NULL;

const char* input_content;
const char* c_header_content;
const char* c_file_content;

static Eina_Bool beta_support = EINA_FALSE;

static void
print_help()
{
  printf("[options] <input> <output c file> <output h file>\n");
}

static void
load_content(void)
{
   Eina_File *file;

   file = eina_file_open(input_file, EINA_FALSE);
   input_content = eina_file_map_all(file, EINA_FILE_SEQUENTIAL);
}

static void
dump_output(const char *file, const char *content)
{
   FILE *f;

   f = fopen(file, "w");
   fwrite(content, strlen(content), 1, f);
   fclose(f);
}

static void
output_content(void)
{
   dump_output(output_c_file, c_file_content);
   dump_output(output_h_file, c_header_content);
}

static Eolian_State *state;

static Eina_Bool
_flush_include_dirs(const void *fdata, void *path, void *data)
{
   eolian_state_directory_add(data, path);

   return EINA_TRUE;
}

void
class_db_init(Eina_Bool support, Eina_Array *include_dirs)
{
   beta_support = support;
   eolian_init();

   state = eolian_state_new();
   if (!eolian_state_system_directory_add(state))
     {
        printf("Error, Adding system directory failed!\n");
        abort();
     }

   eina_array_foreach(include_dirs, _flush_include_dirs, state);

   eolian_bridge_beta_allowed_set(beta_support);
}

int main(int argc, char const *argv[])
{
   Efl_Ui *ui;
   Eina_Array *arr = eina_array_new(5);
   eina_init();

   if (argc < 4)
     {
        print_help();
        return -1;
     }
   for (int i = 1; i < argc - 3; ++i)
     {
        if (!strcmp(argv[i], "--beta-support"))
          {
             beta_support = EINA_TRUE;
          }
        else if (!strcmp(argv[i], "-I"))
          {
             if (i + 1 >= argc - 3)
               {
                  print_help();
                  return -1;
               }
             else
               {
                  i++;
                  if (!access(argv[i], F_OK))
                    {
                       eina_array_push(arr, argv[i]);
                    }
                  else
                    {
                       printf("Directory %s cannot be accessed!\n", argv[i]);
                       print_help();
                       return -1;
                    }
               }
          }
        else
          {
            printf("Not understood token : %s\n", argv[i]);
          }
     }

   input_file = argv[argc - 3];
   output_c_file = argv[argc - 2];
   output_h_file = argv[argc - 1];

   if (!input_file || !output_c_file || !output_h_file)
     {
        print_help();
        return -1;
     }

   class_db_init(beta_support, arr);

   load_content();

   ui = efl_ui_format_parse(input_content);

   if (!validate(state, ui))
     exit(-1);

   if (!c_output(state, ui))
     exit(-1);

   efl_ui_free(ui);
   eina_array_free(arr);
   output_content();
   eina_shutdown();
   return 0;
}
