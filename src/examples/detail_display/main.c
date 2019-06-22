#define EFL_BETA_API_SUPPORT 1
#include <Efl_Ui.h>
#include "detail_display.h"

const char* BYTE_ENDINGS[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", NULL};

static void
_emous_util_size_convert(char *buf, size_t s, int bytes)
{
   float niceval = bytes;
   int i = 0;

   for (i = 0; BYTE_ENDINGS[i + 1]; i++)
     {
        if (((float)(niceval / 1024.0f)) < 1.0f)
          break;
        niceval /= (float) 1024.0f;
     }
   snprintf(buf, s, "%'.2f %s", niceval, BYTE_ENDINGS[i]);
}

static void
_generate_permissions_str(char *buf, size_t s, int mode)
{
   char d,ur,uw,ux,gr,gw,gx,or,ow,ox;

   d = ur = uw = ux = gr = gw = gx = or = ow = ox = '-';

   if (S_ISDIR(mode)) d = 'd';
   if (mode & S_IRUSR) ur = 'r';
   if (mode & S_IWUSR) uw = 'w';
   if (mode & S_IXUSR) ux = 'x';
   if (mode & S_IRGRP) gr = 'r';
   if (mode & S_IWGRP) gw = 'w';
   if (mode & S_IXGRP) gx = 'x';
   if (mode & S_IROTH) or = 'r';
   if (mode & S_IWOTH) ow = 'w';
   if (mode & S_IXOTH) ox = 'x';

   snprintf(buf, s, "%c%c%c%c%c%c%c%c%c%c\n", d, ur, uw, ux, gr, gw, gx, or, ow, ox);
}

EAPI_MAIN void
efl_main(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Eina_Accessor *acc = efl_core_command_line_command_access(ev->object);
   struct stat st;
   const char *c;
   char buf[PATH_MAX];
   Eo *win;

   if (!eina_accessor_data_get(acc, 1, (void**)&c))
     {
        printf("Please pass a file to this binary\n");
        exit(-1);
     }

   if (stat(c, &st))
     {
        perror("Failed to open file");
        exit(-1);
     }

   efreet_init();

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
                 efl_text_set(efl_added, "File Detail"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE)
                );


   Detail_Editor_Data *pd = detail_editor_gen(win);
   efl_content_set(win, pd->root_tbl);
   eina_accessor_free(acc);

   //===========
   //User
   //===========
   struct passwd *pw;

   pw = getpwuid(st.st_uid);
   if (pw)
     snprintf(buf, sizeof(buf), "%s", pw->pw_name);
   else
     snprintf(buf, sizeof(buf), "User not found");
   efl_text_set(pd->owner, buf);

   //===========
   //Group
   //===========
   struct group *gr;

   gr = getgrgid(st.st_gid);
   if (gr)
     snprintf(buf, sizeof(buf), "%s", gr->gr_name);
   else
     snprintf(buf, sizeof(buf), "Group not found");
   efl_text_set(pd->group, buf);

   //===========
   //Permissions
   //===========
   _generate_permissions_str(buf, sizeof(buf), st.st_mode);
   efl_text_set(pd->permissions, buf);

   //===========
   //Modify
   //===========
   snprintf(buf, sizeof(buf), "%s", ctime(&st.st_mtime));
   efl_text_set(pd->modify, buf);

   //===========
   //Changed
   //===========
   snprintf(buf, sizeof(buf), "%s", ctime(&st.st_ctime));
   efl_text_set(pd->change, buf);

   //===========
   //Accessed
   //===========
   snprintf(buf, sizeof(buf), "%s", ctime(&st.st_atime));
   efl_text_set(pd->access, buf);

   //===========
   //Size
   //===========
   _emous_util_size_convert(buf, sizeof(buf), st.st_size);
   efl_text_set(pd->size, buf);

   //===========
   //Icon
   //===========

   const char *path = efreet_mime_type_icon_get(efreet_mime_fallback_type_get(c), "Numix", 20);
   if (path)
     {
        efl_file_set(pd->icon, path);
        efl_file_load(pd->icon);
     }

   efreet_shutdown();
}
EFL_MAIN()
