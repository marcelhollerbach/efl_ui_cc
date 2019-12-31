#define EFL_BETA_API_SUPPORT 1
#define EFL_UI_WIDGET_PROTECTED
#include "main.h"
#include <Efl_Ui_Format.h>
#include "predictor.h"
#include "abstract_tree_private.h"

#include "reorder_ui.h"
#include "reorder_ui_linear_item.h"
#include "reorder_ui_table_item.h"
#include "reorder_ui_part_item.h"

typedef struct {
  Eina_Promise *ctx;
  Reorder_Ui_Data *ui;
  void (*value_fetch)(Reorder_Ui_Data *ui, Eina_Strbuf *buf);
} Reorder_Context;

static void
_finished_cb(void *data, const Efl_Event *ev)
{
   Reorder_Context *ctx = data;
   Eina_Strbuf *buf = eina_strbuf_new();
   ctx->value_fetch(ctx->ui, buf);
   char *tmp = eina_strbuf_release(buf);
   eina_promise_resolve(ctx->ctx, eina_value_string_init(tmp));
   efl_del(ctx->ui->root);
   free(tmp);
}

static void
_close_cb(void *data, const Efl_Event *ev)
{
   Reorder_Context *ctx = data;

   eina_promise_reject(ctx->ctx, 0);
   efl_del(ctx->ui->root);
}

static void
_del_cb(void *data, const Efl_Event *ev)
{
   Reorder_Context *ctx = data;

   free(ctx->ui);
   free(ctx);
}

static void
_linear_value_fetch(Reorder_Ui_Data *ui, Eina_Strbuf *buf)
{
   for (int i = 0; i < efl_content_count(ui->container); ++i)
   {
      Eo *item = efl_pack_content_get(ui->container, i);
      Eina_Value *v = efl_key_value_get(item, "__reorder_value");
      int d = -1;

      EINA_SAFETY_ON_FALSE_RETURN(eina_value_int_get(v, &d));

      if (i != 0)
        eina_strbuf_append(buf, ";");
      eina_strbuf_append_printf(buf, "%d", d);
   }
}

static inline void
_reeval(Reorder_Ui_Linear_Item_Data *data, Eo *container, int index)
{
   efl_ui_widget_disabled_set(data->up, EINA_FALSE);
   efl_ui_widget_disabled_set(data->down, EINA_FALSE);
   /*if (index == 0)
     efl_ui_widget_disabled_set(data->up, EINA_TRUE);
   if (index == efl_content_count(container) - 1)
     efl_ui_widget_disabled_set(data->down, EINA_TRUE);*/
}

static inline void
_repack(Eo *container, Eo *subobj, int new_index)
{
   efl_pack_unpack(container, subobj);
   efl_pack_at(container, subobj, new_index);
}

static void
_up_cb(void *data, const Efl_Event *ev)
{
   Reorder_Ui_Linear_Item_Data *d = data;
   Eo *container = efl_ui_widget_parent_get(d->root);
   int index = efl_pack_index_get(container, d->root);

   index --;
   if (index < 0 || index >= efl_content_count(container))
     return;
   _repack(container, d->root, index);
   _reeval(d, container, index);
   efl_ui_selectable_selected_set(d->root, EINA_TRUE);
}

static void
_down_cb(void *data, const Efl_Event *ev)
{
   Reorder_Ui_Linear_Item_Data *d = data;
   Eo *container = efl_ui_widget_parent_get(d->root);
   int index = efl_pack_index_get(container, d->root);

   index ++;
   if (index < 0 || index >= efl_content_count(container))
     return;
   _repack(container, d->root, index);
   _reeval(d, container, index);
   efl_ui_selectable_selected_set(d->root, EINA_TRUE);
}

Eina_Future*
linear_change_ui(Efl_Ui_Node *node, Eo *anchor_widget)
{
   Reorder_Context *ctx = calloc(1, sizeof(Reorder_Context));
   Eina_Strbuf *buf = eina_strbuf_new();
   ctx->ctx = efl_loop_promise_new(efl_main_loop_get());
   ctx->ui = reorder_ui_gen(anchor_widget);

   efl_text_set(ctx->ui->promotion_text, "Reorder Linear Items");
   efl_gfx_entity_visible_set(ctx->ui->root, EINA_TRUE);
   efl_event_callback_add(ctx->ui->ok, EFL_INPUT_EVENT_CLICKED, _finished_cb, ctx);
   efl_event_callback_add(ctx->ui->root, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _close_cb, ctx);
   efl_event_callback_add(ctx->ui->root, EFL_EVENT_DEL, _del_cb, ctx);
   efl_gfx_hint_size_min_set(ctx->ui->container, EINA_SIZE2D(250, 250));
   ctx->value_fetch = _linear_value_fetch;

   for (unsigned int i = 0; i < eina_array_count(node->children_linear); ++i)
     {
        Efl_Ui_Pack_Linear *n_complex = eina_array_data_get(node->children_linear, i);
        Efl_Ui_Node *n = n_complex->basic.node;
        Reorder_Ui_Linear_Item_Data *data = reorder_ui_linear_item_gen(anchor_widget);
        efl_key_value_set(data->root, "__reorder_value", eina_value_int_new(i));
        efl_key_data_set(data->root, "__efl_ui_value", data);

        //efl_event_callback_add(data->up, desc, cb, data)
        efl_event_callback_add(data->up, EFL_INPUT_EVENT_CLICKED, _up_cb, data);
        efl_event_callback_add(data->down, EFL_INPUT_EVENT_CLICKED, _down_cb, data);

        if (n->id && strlen(n->id) > 0)
          eina_strbuf_append_printf(buf, "%s: ", n->id);
        else
          eina_strbuf_append_printf(buf, "%s: ", n->type);
        eina_strbuf_append_printf(buf, "(%d)", i+1);
        efl_text_set(data->root, eina_strbuf_string_get(buf));
        efl_pack_end(ctx->ui->container, data->root);
        eina_strbuf_reset(buf);
     }
   eina_strbuf_free(buf);

   return eina_future_new(ctx->ctx);
}

static void
_table_value_fetch(Reorder_Ui_Data *ui, Eina_Strbuf *buf)
{
   for (int i = 0; i < efl_content_count(ui->container); ++i)
   {
      Eo *item = efl_pack_content_get(ui->container, i);
      Reorder_Ui_Table_Item_Data *data = efl_key_data_get(item, "__efl_ui_value");

      if (i != 0)
        eina_strbuf_append(buf, ";");
      eina_strbuf_append_printf(buf, "%d:%d:%d:%d", (int)efl_ui_range_value_get(data->x), (int)efl_ui_range_value_get(data->y), (int)efl_ui_range_value_get(data->w), (int)efl_ui_range_value_get(data->h));
   }
}

Eina_Future*
table_change_ui(Efl_Ui_Node *node, Eo *anchor_widget)
{
   Reorder_Context *ctx = calloc(1, sizeof(Reorder_Context));
   Eina_Strbuf *buf = eina_strbuf_new();
   ctx->ctx = efl_loop_promise_new(efl_main_loop_get());
   ctx->ui = reorder_ui_gen(anchor_widget);

   efl_text_set(ctx->ui->promotion_text, "Change Table Items");
   efl_gfx_entity_visible_set(ctx->ui->root, EINA_TRUE);
   efl_event_callback_add(ctx->ui->ok, EFL_INPUT_EVENT_CLICKED, _finished_cb, ctx);
   efl_event_callback_add(ctx->ui->root, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _close_cb, ctx);
   efl_event_callback_add(ctx->ui->root, EFL_EVENT_DEL, _del_cb, ctx);
   efl_gfx_hint_size_min_set(ctx->ui->container, EINA_SIZE2D(250, 250));
   ctx->value_fetch = _table_value_fetch;

   for (unsigned int i = 0; i < eina_array_count(node->children_table); ++i)
     {
        Efl_Ui_Pack_Table *n_complex = eina_array_data_get(node->children_table, i);
        Efl_Ui_Node *n = n_complex->basic.node;
        Reorder_Ui_Table_Item_Data *data = reorder_ui_table_item_gen(anchor_widget);
        efl_key_data_set(data->root, "__efl_ui_value", data);

        if (n->id && strlen(n->id) > 0)
          eina_strbuf_append_printf(buf, "%s: ", n->id);
        else
          eina_strbuf_append_printf(buf, "%s: ", n->type);

        efl_text_set(data->tmp_name, eina_strbuf_string_get(buf));
        efl_ui_range_value_set(data->x, atof(n_complex->x));
        efl_ui_range_value_set(data->y, atof(n_complex->y));
        efl_ui_range_value_set(data->w, atof(n_complex->w));
        efl_ui_range_value_set(data->h, atof(n_complex->h));
        efl_pack_end(ctx->ui->container, data->root);
        eina_strbuf_reset(buf);
     }
   eina_strbuf_free(buf);

   return eina_future_new(ctx->ctx);
}

static void
_part_value_fetch(Reorder_Ui_Data *ui, Eina_Strbuf *buf)
{
   for (int i = 0; i < efl_content_count(ui->container); ++i)
   {
      Eo *item = efl_pack_content_get(ui->container, i);
      Reorder_Ui_Part_Item_Data *data = efl_key_data_get(item, "__efl_ui_value");

      if (i != 0)
        eina_strbuf_append(buf, ";");
      eina_strbuf_append_printf(buf, "%s", efl_text_get(data->part_name));
   }
}

Eina_Future*
part_change_ui(Efl_Ui_Node *node, Eo *anchor_widget)
{
   Reorder_Context *ctx = calloc(1, sizeof(Reorder_Context));
   Eina_Strbuf *buf = eina_strbuf_new();
   ctx->ctx = efl_loop_promise_new(efl_main_loop_get());
   ctx->ui = reorder_ui_gen(anchor_widget);

   efl_text_set(ctx->ui->promotion_text, "Change Part Items");
   efl_gfx_entity_visible_set(ctx->ui->root, EINA_TRUE);
   efl_event_callback_add(ctx->ui->ok, EFL_INPUT_EVENT_CLICKED, _finished_cb, ctx);
   efl_event_callback_add(ctx->ui->root, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED, _close_cb, ctx);
   efl_event_callback_add(ctx->ui->root, EFL_EVENT_DEL, _del_cb, ctx);
   efl_gfx_hint_size_min_set(ctx->ui->container, EINA_SIZE2D(250, 250));
   ctx->value_fetch = _part_value_fetch;

   for (unsigned int i = 0; i < eina_array_count(node->children_part); ++i)
     {
        Efl_Ui_Pack_Pack *n_complex = eina_array_data_get(node->children_part, i);
        Efl_Ui_Node *n = n_complex->basic.node;
        Reorder_Ui_Part_Item_Data *data = reorder_ui_part_item_gen(anchor_widget);
        efl_ui_widget_focus_allow_set(data->root, EINA_FALSE);
        efl_key_data_set(data->root, "__efl_ui_value", data);

        if (n->id && strlen(n->id) > 0)
          eina_strbuf_append_printf(buf, "%s : ", n->id);
        else
          eina_strbuf_append_printf(buf, "%s : ", n->type);

        efl_text_set(data->root, eina_strbuf_string_get(buf));
        efl_text_set(data->part_name, n_complex->part_name);
        efl_pack_end(ctx->ui->container, data->root);
        eina_strbuf_reset(buf);
     }
   eina_strbuf_free(buf);

   efl_ui_focus_util_focus(ctx->ui->container);

   return eina_future_new(ctx->ctx);
}
