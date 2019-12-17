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

Eina_Future*
select_avaible_value(Outputter_Property_Value *value, Eo *anchor_widget)
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
   else if (decl && eolian_typedecl_type_get(decl) == EOLIAN_TYPEDECL_ENUM)
     {
        printf("NOT IMPLEMENTED YET\n");
        return NULL;
     }

   efl_ui_popup_anchor_set(selection->popup, anchor_widget);
   efl_event_callback_add(selection->set, EFL_INPUT_EVENT_CLICKED, _set_clicked_cb, selection);
   efl_gfx_entity_visible_set(selection->popup, EINA_TRUE);

   return eina_future_new(selection->ctx);
}
