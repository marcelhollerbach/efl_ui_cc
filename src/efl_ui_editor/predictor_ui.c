#define EFL_BETA_API_SUPPORT 1
#include "main.h"
#include <Efl_Ui_Format.h>
#include "predictor.h"
#include "property_name_ui.h"

static Eina_Promise*
_promise_remove(Efl_Ui_Widget *popup)
{
   Eina_Promise *ctx = efl_key_data_get(popup, "__promise");
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);
   efl_key_data_set(popup, "__promise", NULL);

   return ctx;
}

static void
_types_hide_cb(void *data, const Efl_Event *ev)
{
   efl_gfx_entity_visible_set(ev->object, EINA_FALSE);
   eina_promise_reject(_promise_remove(ev->object), 0); //FIXME
}

static void
_types_selection_changed_cb(void *data, const Efl_Event *ev)
{
   Efl_Ui_Item *last_selected = efl_ui_selectable_last_selected_get(ev->object);
   Eina_Value resolved_value;

   if (!last_selected) return; //Not interested in unselects

   resolved_value = eina_value_string_init(efl_text_get(last_selected));
   efl_gfx_entity_visible_set(data, EINA_FALSE);
   eina_promise_resolve(_promise_remove(data), resolved_value);
}

Eina_Future*
select_available_types(void)
{
   static Efl_Ui_Popup *popup = NULL;
   Eina_Promise *ctx = efl_loop_promise_new(efl_main_loop_get());

   if (!popup)
     {
        Efl_Ui_List *container;
        Predicted_Class *klass;

        popup = efl_add(EFL_UI_POPUP_CLASS, efl_provider_find(win, EFL_UI_WIN_CLASS));
        efl_event_callback_add(popup, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _types_hide_cb, NULL);

        container = efl_add(EFL_UI_LIST_CLASS, popup);
        efl_gfx_hint_size_min_set(container, EINA_SIZE2D(250, 250));
        efl_content_set(popup, container);
        efl_event_callback_add(container, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, _types_selection_changed_cb, popup);

        klass = get_available_types();

        for (int i = 0; klass[i].klass_name; ++i)
          {
             Efl_Ui_Default_Item *item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, container);
             efl_gfx_hint_size_min_set(item, EINA_SIZE2D(80, 50));
             efl_text_set(item, klass[i].klass_name);
             efl_pack_end(container, item);
          }
        free(klass);
     }
   //bind promise to the popup
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_key_data_get(popup, "__promise") == NULL, NULL);
   efl_key_data_set(popup, "__promise", ctx);

   Eo *container = efl_content_get(popup);
   efl_ui_multi_selectable_all_unselect(container);
   efl_gfx_entity_visible_set(popup, EINA_TRUE);

   return eina_future_new(ctx);
}

static void
_backwall_clicked_cb(void *data, const Efl_Event *ev)
{
   eina_promise_reject(_promise_remove(data), 0);
   efl_del(data);
}

static void
_selection_changed_cb(void *data, const Efl_Event *ev)
{
   Efl_Ui_Item *last_selected = efl_ui_selectable_last_selected_get(ev->object);
   Eina_Value resolved_value;

   if (!last_selected) return; //Not interested in unselects

   resolved_value = eina_value_string_init(efl_text_get(last_selected));
   eina_promise_resolve(_promise_remove(data), resolved_value);
   efl_del(data);
}

Eina_Future*
select_available_properties(Efl_Ui_Node *node)
{
   Predicted_Property *property = get_available_properties(node);
   Property_Name_Ui_Data *data = property_name_ui_gen(win);
   Eina_Promise *ctx = efl_loop_promise_new(efl_main_loop_get());

   for (int i = 0; property[i].name; ++i)
     {
        Efl_Ui_Default_Item *item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, data->list);
        efl_gfx_hint_size_min_set(item, EINA_SIZE2D(80, 50));
        efl_text_set(item, property[i].name);
        efl_pack_end(data->list, item);
     }
   free(property);

   efl_event_callback_add(data->root, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _backwall_clicked_cb, data->root);
   efl_event_callback_add(data->list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, _selection_changed_cb, data->root);
   efl_gfx_hint_size_min_set(data->list, EINA_SIZE2D(250, 250));
   efl_key_data_set(data->root, "__promise", ctx);
   efl_gfx_entity_visible_set(data->root, EINA_TRUE);

   free(data);

   return eina_future_new(ctx);
}
