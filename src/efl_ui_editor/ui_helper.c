#define EFL_BETA_API_SUPPORT 1

#include "main.h"
#include <Efl_Ui_Format.h>
#include <Efl_Ui.h>

static Efl_Canvas_Animation *fade_animation;

void
ui_helper_init(void)
{
   fade_animation = efl_new(EFL_CANVAS_ANIMATION_SCALE_CLASS);
   efl_animation_duration_set(fade_animation, 0.1);
}

void
animated_visible_set(Eo *obj, Eina_Bool visible)
{
  if (!fade_animation)
    ui_helper_init();

  if (visible)
    {
      efl_animation_scale_set(fade_animation, EINA_VECTOR2(0.0, 0.0), EINA_VECTOR2(1.0, 1.0), obj, EINA_VECTOR2(0.5, 0.5));
      efl_gfx_entity_visible_set(obj, visible);
      efl_canvas_object_animation_start(obj, fade_animation, 1.0, 0.0);
    }
}
