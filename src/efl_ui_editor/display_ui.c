#define EFL_BETA_API_SUPPORT 1
#define EFL_CANVAS_OBJECT_PROTECTED 1
#include "main.h"
#include "base_ui.h"
#include "abstract_tree_private.h"
#include <Efl_Ui_Format.h>
#include "predictor.h"

#include "abstract_node_ui.h"
#include "property_item_ui.h"
#include "new_entry_item_ui.h"

static Efl_Gfx_Entity *sample = NULL;

static void
_geom_changed_cb(void *data, const Efl_Event *ev)
{
   Eina_Rect geom = efl_gfx_entity_geometry_get(background);

   geom.x += 20;
   geom.y += 20;
   geom.w -= 40;
   geom.h -= 40;

   efl_gfx_entity_geometry_set(sample, geom);
}

void
display_ui_init(Efl_Ui_Win *win EINA_UNUSED)
{
   efl_event_callback_add(background, EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, _geom_changed_cb, NULL);
   efl_event_callback_add(background, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _geom_changed_cb, NULL);
}

void
display_ui_refresh(Efl_Ui *ui)
{
   if (sample)
     efl_del(sample);

   sample = object_generator(win, editor_state, ui);

   efl_canvas_group_member_add(efl_canvas_object_render_parent_get(background), sample);
   _geom_changed_cb(NULL, NULL);
}
