#define EFL_BETA_API_SUPPORT 1
#include "main.h"
#include <Efl_Ui_Format.h>
#include "predictor.h"

#include "value_change_bool_ui.h"
#include "value_change_range_ui.h"
#include "value_change_text_ui.h"

typedef struct _Value_Selection Value_Selection;

struct _Value_Selection {
   Eo *popup, *set;
   Eo *selector;
   void (*get_value)(Value_Selection *v, Eina_Strbuf *buf);
   Eina_Promise *ctx;
};

static void
_fetch_range_cb(Value_Selection *v, Eina_Strbuf *buf)
{
   eina_strbuf_append_printf(buf, "%f", efl_ui_range_value_get(v->selector));
}

static void
_fetch_text_cb(Value_Selection *v, Eina_Strbuf *buf)
{
   eina_strbuf_append_printf(buf, "\"%s\"", efl_text_get(v->selector));
}

static void
_fetch_enum_range_cb(Value_Selection *v, Eina_Strbuf *buf)
{
   Eina_Accessor *acc;
   Efl_Ui_Format_Value *fv;

   acc = efl_ui_format_values_get(v->selector);
   eina_accessor_data_get(acc, efl_ui_range_value_get(v->selector), (void*) &fv);
   eina_strbuf_append_printf(buf, "%s", fv->text);
}

static void
_fetch_bool_cb(Value_Selection *v, Eina_Strbuf *buf)
{
   if (!efl_ui_radio_group_selected_value_get(v->selector))
     eina_strbuf_append(buf, "false");
   else
     eina_strbuf_append(buf, "true");
}

static void
_set_clicked_cb(void *data, const Efl_Event *ev)
{
   Value_Selection *v = data;
   Eina_Strbuf *buf = eina_strbuf_new();

   v->get_value(v, buf);
   eina_promise_resolve(v->ctx, eina_value_string_init(eina_strbuf_string_get(buf)));
   eina_strbuf_free(buf);
   efl_del(v->popup);
   free(v);
}

static void
_close_cb(void *data, const Efl_Event *ev)
{
   Value_Selection *v = data;

   eina_promise_reject(v->ctx, 0); //FIXME
   efl_del(v->popup);
   free(v);
}

Eina_Future*
change_value(Outputter_Property_Value *value, Eo *anchor_widget)
{
   Value_Selection *selection = calloc(sizeof(Value_Selection), 1);

   selection->ctx = efl_loop_promise_new(efl_main_loop_get());
   const Eolian_Type *etype = value->type;
   const Eolian_Type_Builtin_Type btype = eolian_type_builtin_type_get(etype);
   const Eolian_Typedecl *decl = eolian_type_typedecl_get(etype);

   if (btype >= EOLIAN_TYPE_BUILTIN_BYTE && btype <= EOLIAN_TYPE_BUILTIN_UINT128)
     {
        Value_Change_Range_Ui_Data *data = value_change_range_ui_gen(anchor_widget);
        selection->popup = data->root;
        selection->set = data->ok;
        selection->selector = data->range_selector;
        selection->get_value = _fetch_range_cb;
        efl_ui_range_value_set(data->range_selector, atoi(value->value));
     }
   else if (btype >= EOLIAN_TYPE_BUILTIN_FLOAT && btype <= EOLIAN_TYPE_BUILTIN_DOUBLE)
     {
        Value_Change_Range_Ui_Data *data = value_change_range_ui_gen(anchor_widget);
        selection->popup = data->root;
        selection->set = data->ok;
        selection->selector = data->range_selector;
        selection->get_value = _fetch_range_cb;
        efl_ui_range_value_set(data->range_selector, atof(value->value));
     }
   else if (btype == EOLIAN_TYPE_BUILTIN_BOOL)
     {
        Value_Change_Bool_Ui_Data *data = value_change_bool_ui_gen(anchor_widget);
        selection->popup = data->root;
        selection->set = data->ok;
        selection->selector = data->boolean_selector;
        selection->get_value = _fetch_bool_cb;
        efl_ui_radio_group_selected_value_set(data->boolean_selector, eina_streq(value->value, "true"));
     }
   else if (btype == EOLIAN_TYPE_BUILTIN_MSTRING || btype == EOLIAN_TYPE_BUILTIN_STRING || btype == EOLIAN_TYPE_BUILTIN_STRINGSHARE)
     {
        Value_Change_Text_Ui_Data *data = value_change_text_ui_gen(anchor_widget);
        selection->popup = data->root;
        selection->set = data->ok;
        selection->selector = data->text_selector;
        selection->get_value = _fetch_text_cb;
        efl_text_set(data->text_selector, value->value);
        efl_ui_focus_util_focus(data->text_selector);
        efl_text_interactive_all_select(data->text_selector);
     }
   else if (decl && eolian_typedecl_type_get(decl) == EOLIAN_TYPEDECL_ENUM)
     {
        Eina_List *enum_fields = NULL;
        Eolian_Enum_Type_Field *field;
        Eina_Iterator *fields = eolian_typedecl_enum_fields_get(decl);
        int state = 0, later_index = 0;

        EINA_ITERATOR_FOREACH(fields, field)
          {
             const char *name = eolian_typedecl_enum_field_name_get(field);
             Efl_Ui_Format_Value *v = alloca(sizeof(Efl_Ui_Format_Value));

             if (eina_streq(value->value, name))
               later_index = state;

             v->value = state;
             v->text = name;

             enum_fields = eina_list_append(enum_fields, v);
             state ++;
          }
        eina_iterator_free(fields);

        Value_Change_Range_Ui_Data *data = value_change_range_ui_gen(anchor_widget);

        selection->popup = data->root;
        selection->set = data->ok;
        selection->selector = data->range_selector;
        selection->get_value = _fetch_enum_range_cb;
        efl_ui_range_limits_set(selection->selector, 0, state - 1);
        efl_ui_format_values_set(selection->selector, eina_list_accessor_new(enum_fields));
        efl_ui_range_value_set(data->range_selector, later_index);
     }

   efl_ui_popup_anchor_set(selection->popup, anchor_widget);
   efl_event_callback_add(selection->set, EFL_INPUT_EVENT_CLICKED, _set_clicked_cb, selection);
   efl_event_callback_add(selection->popup, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _close_cb, selection);
   efl_gfx_entity_visible_set(selection->popup, EINA_TRUE);

   return eina_future_new(selection->ctx);
}

static void
_name_setting_cb(void *data, const Efl_Event *ev)
{
   Value_Selection *s = data;
   const char *text;

   text = efl_text_get(s->selector);
   eina_promise_resolve(s->ctx, eina_value_string_init(text));
   efl_del(s->popup);
   free(s);
}

Eina_Future*
change_name(Efl_Ui_Node *node, Eo *anchor_widget)
{
   Value_Selection *s = calloc(1, sizeof(Value_Selection));
   Value_Change_Text_Ui_Data *data = value_change_text_ui_gen(win);
   const char *id = node_id_get(node);
   s->ctx = efl_loop_promise_new(efl_main_loop_get());
   s->selector = data->text_selector;
   s->popup = data->root;

   efl_ui_popup_anchor_set(data->root, anchor_widget);
   efl_event_callback_add(data->ok, EFL_INPUT_EVENT_CLICKED, _name_setting_cb, s);
   efl_event_callback_add(s->popup, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _close_cb, s);
   efl_gfx_entity_visible_set(data->root, EINA_TRUE);
   efl_text_set(s->selector, id);

   efl_ui_focus_util_focus(s->selector);
   efl_text_interactive_all_select(s->selector);

   return eina_future_new(s->ctx);
}
