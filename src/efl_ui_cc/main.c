#include "main.h"
#define EFL_BETA_API_SUPPORT
#include <Eina.h>
#include <Eolian.h>
#include <unistd.h>

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

int main(int argc, char const *argv[])
{
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
        if (!strcmp(argv[i], "-I"))
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
   parse();

   output_content();
   eina_shutdown();
   return 0;
}
