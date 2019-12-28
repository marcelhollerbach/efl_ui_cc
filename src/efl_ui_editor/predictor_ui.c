#define EFL_BETA_API_SUPPORT 1
#include "main.h"
#include <Efl_Ui_Format.h>
#include "predictor.h"
#include "predictor_ui.h"
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

static void
_text_changed_cb(void *data, const Efl_Event *ev)
{
   Predictor_Ui_Data *d = data;

   const char *text = efl_text_get(ev->object);

   efl_pack_clear(d->container);
   Predicted_Class *klass = get_available_types();
   for (int i = 0; klass[i].klass_name; ++i)
     {
        if (text)
          {
             char *atext = eina_strdup(text);
             char *klass_name = eina_strdup(klass[i].klass_name);

             eina_str_tolower(&atext);
             eina_str_tolower(&klass_name);

             if (strstr(klass_name, atext) == NULL)
               continue;
          }

        Efl_Ui_Default_Item *item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, d->container);
        efl_gfx_hint_size_min_set(item, EINA_SIZE2D(80, 50));
        efl_text_set(item, klass[i].klass_name);
        efl_pack_end(d->container, item);
     }
}

Eina_Future*
select_available_types(void)
{
   static Predictor_Ui_Data *data;
   Eina_Promise *ctx = efl_loop_promise_new(efl_main_loop_get());

   if (!data)
     {
        Predicted_Class *klass;

        data = predictor_ui_gen(win);
        efl_gfx_hint_size_min_set(data->container, EINA_SIZE2D(250, 250));
        efl_event_callback_add(data->root, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _types_hide_cb, NULL);
        efl_event_callback_add(data->container, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, _types_selection_changed_cb, data->root);
        efl_event_callback_add(data->search_text, EFL_TEXT_INTERACTIVE_EVENT_CHANGED_USER, _text_changed_cb, data);
        klass = get_available_types();
        for (int i = 0; klass[i].klass_name; ++i)
          {
             Efl_Ui_Default_Item *item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, data->container);
             efl_gfx_hint_size_min_set(item, EINA_SIZE2D(80, 50));
             efl_text_set(item, klass[i].klass_name);
             efl_pack_end(data->container, item);
          }
     }
   //bind promise to the popup
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_key_data_get(data->root, "__promise") == NULL, NULL);
   efl_key_data_set(data->root, "__promise", ctx);

   efl_ui_multi_selectable_all_unselect(data->container);
   efl_gfx_entity_visible_set(data->root, EINA_TRUE);
   efl_text_set(data->search_text, "");
   Efl_Event ev = {data->search_text, NULL, NULL};
   _text_changed_cb(data, &ev);
   efl_ui_focus_util_focus(data->search_text);

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

static void
_properties_changed_cb(void *data, const Efl_Event *ev)
{
   Property_Name_Ui_Data *d = data;
   const char *text = efl_text_get(ev->object);

   efl_pack_clear(d->container);
   Predicted_Property *property = get_available_properties(efl_key_data_get(ev->object, "__node"));
   for (int i = 0; property[i].name; ++i)
     {
        if (text)
          {
             char *atext = eina_strdup(text);
             char *klass_name = eina_strdup(property[i].name);

             eina_str_tolower(&atext);
             eina_str_tolower(&klass_name);

             if (strstr(klass_name, atext) == NULL)
               continue;
          }
        Efl_Ui_Default_Item *item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, d->container);
        efl_gfx_hint_size_min_set(item, EINA_SIZE2D(80, 50));
        efl_text_set(item, property[i].name);
        efl_pack_end(d->container, item);
     }
   free(property);

}

Eina_Future*
select_available_properties(Efl_Ui_Node *node)
{
   Predicted_Property *property = get_available_properties(node);
   Property_Name_Ui_Data *data = property_name_ui_gen(win);
   Eina_Promise *ctx = efl_loop_promise_new(efl_main_loop_get());

   for (int i = 0; property[i].name; ++i)
     {
        Efl_Ui_Default_Item *item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, data->container);
        efl_gfx_hint_size_min_set(item, EINA_SIZE2D(80, 50));
        efl_text_set(item, property[i].name);
        efl_pack_end(data->container, item);
     }
   free(property);

   efl_key_data_set(data->search_text, "__node", node);

   efl_event_callback_add(data->root, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _backwall_clicked_cb, data->root);
   efl_event_callback_add(data->container, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, _selection_changed_cb, data->root);
   efl_event_callback_add(data->search_text, EFL_TEXT_INTERACTIVE_EVENT_CHANGED_USER, _properties_changed_cb, data);
   efl_event_callback_add(data->root, EFL_EVENT_DEL, (Efl_Event_Cb)free, data);
   efl_gfx_hint_size_min_set(data->container, EINA_SIZE2D(250, 250));
   efl_key_data_set(data->root, "__promise", ctx);
   efl_gfx_entity_visible_set(data->root, EINA_TRUE);
   efl_ui_focus_util_focus(data->search_text);

   return eina_future_new(ctx);
}
