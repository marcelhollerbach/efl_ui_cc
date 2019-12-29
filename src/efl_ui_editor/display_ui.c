#define EFL_BETA_API_SUPPORT 1
#define EFL_CANVAS_OBJECT_PROTECTED 1
#include "main.h"
#include "base_ui.h"
#include "abstract_tree_private.h"
#include <Efl_Ui_Format.h>
#include "predictor.h"

#include "abstract_node_ui.h"
#include "property_item_ui.h"

static Efl_Gfx_Entity *sample = NULL;

static Efl_Ui_Win *internal = NULL;

static void
_geom_changed_cb(void *data, const Efl_Event *ev)
{
   Eina_Rect geom = efl_gfx_entity_geometry_get(background);

   geom.x += 20;
   geom.y += 20;
   geom.w -= 40;
   geom.h -= 40;

   efl_gfx_entity_geometry_set(elm_win_inlined_image_object_get(internal), geom);
   efl_gfx_entity_size_set(internal, geom.size);
}

void
display_ui_init(Efl_Ui_Win *win EINA_UNUSED)
{
   internal = elm_win_add(win, "internal", ELM_WIN_INLINED_IMAGE);
   elm_win_alpha_set(internal, EINA_TRUE);
   efl_event_callback_add(background, EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, _geom_changed_cb, NULL);
   efl_event_callback_add(background, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _geom_changed_cb, NULL);
}

void
display_ui_refresh(Efl_Ui *ui)
{
   if (sample)
     efl_del(sample);

   sample = object_generator(internal, editor_state, ui);
   elm_object_tree_focus_allow_set(sample, EINA_FALSE);
   elm_win_resize_object_add(internal, sample);
   _geom_changed_cb(NULL, NULL);
}
