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
static Object_Hash_Tuple tuple = {NULL, NULL};
static Efl_Ui_Widget *current_widget = NULL;
static Efl_Ui_Win *internal = NULL;
static Efl_Animation *animation;

static void
_geom_changed_cb(void *data, const Efl_Event *ev)
{
   Eina_Rect geom = efl_gfx_entity_geometry_get(background);
   efl_gfx_entity_geometry_set(elm_win_inlined_image_object_get(internal), geom);
   efl_gfx_entity_size_set(internal, geom.size);
}

void
display_ui_init(Efl_Ui_Win *win EINA_UNUSED)
{
   Efl_Animation *up, *down;
   internal = elm_win_add(win, "internal", ELM_WIN_INLINED_IMAGE);
   elm_win_alpha_set(internal, EINA_TRUE);
   efl_event_callback_add(background, EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, _geom_changed_cb, NULL);
   efl_event_callback_add(background, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _geom_changed_cb, NULL);

   up = efl_new(EFL_CANVAS_ANIMATION_TRANSLATE_CLASS, NULL);
   efl_animation_translate_set(up, EINA_POSITION2D(0, 0), EINA_POSITION2D(0, -45));

   down = efl_new(EFL_CANVAS_ANIMATION_TRANSLATE_CLASS, NULL);
   efl_animation_translate_set(down, EINA_POSITION2D(0, -45), EINA_POSITION2D(0, 0));
   efl_animation_interpolator_set(down, efl_new(EFL_BOUNCE_INTERPOLATOR_CLASS, efl_bounce_interpolator_bounces_set(efl_added, 3)));

   animation = efl_new(EFL_CANVAS_ANIMATION_GROUP_SEQUENTIAL_CLASS);
   efl_animation_group_animation_add(animation, up);
   efl_animation_group_animation_add(animation, down);
   efl_animation_duration_set(animation, 1.5);
}


void
display_ui_refresh(Efl_Ui *ui)
{
   if (sample)
     efl_del(sample);

   if (tuple.node_widget)
     eina_hash_free(tuple.node_widget);
   if (tuple.widget_node)
     eina_hash_free(tuple.widget_node);

   sample = object_generator(internal, editor_state, ui, &tuple);
   elm_object_tree_focus_allow_set(sample, EINA_FALSE);
   elm_win_resize_object_add(internal, sample);
   _geom_changed_cb(NULL, NULL);
}

void
highlight_node(Efl_Ui_Node *node)
{
   Efl_Ui_Widget *widget = eina_hash_find(tuple.node_widget, &node);

   if (widget == current_widget) return;

   efl_canvas_object_animation_start(widget, animation, 1.0, 0.0);
}
