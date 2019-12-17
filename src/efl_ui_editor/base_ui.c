#define EFL_BETA_API_SUPPORT 1
#include "main.h"
#include <Efl_Ui_Format.h>
#include "predictor.h"

#include "base_ui.h"
#include "abstract_node_ui.h"
#include "property_item_ui.h"
#include "new_entry_item_ui.h"
#include "children_item_ui.h"

typedef struct _Local_Stack Local_Stack;

struct _Local_Stack {
   Outputter_Node *onode;
   Efl_Ui_Node *tnode;
   Abstract_Node_Ui_Data *data;
   Local_Stack *next, *parent;
   Outputter_Node *onode_replacement;
};

static Local_Stack *highest = NULL, *root;
static Eina_Array *update_stack;
static Base_Ui_Data *base_ui;

Efl_Gfx_Entity *background;

static void push_ui_node(Outputter_Node *node, Eina_Bool back_support);


static void
_local_stack_update(Outputter_Node *node, Efl_Ui_Node *n)
{
   Local_Stack *stack = root;

   EINA_SAFETY_ON_NULL_RETURN(update_stack);

   for (stack = root; stack; stack = stack->next)
     {
       if (stack->tnode == n)
         {
            stack->onode_replacement = node;
            eina_array_push(update_stack, stack);
         }
     }
}
static void
_push_node_cb(void *data, const Efl_Event *ev)
{
   push_ui_node(data, EINA_TRUE);
}

static void
_back_cb(void *data, const Efl_Event *ev)
{
   efl_ui_spotlight_pop(base_ui->prop_stack, EINA_TRUE);
}

static Eina_Value
_new_value_cb(void *data, const Eina_Value value, const Eina_Future *f)
{
   if (eina_value_type_get(&value) != EINA_VALUE_TYPE_ERROR)
     {
         const char *name;
         EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_string_get(&value, &name), eina_value_error_init(eina_error_find("Failed to find type")));
         change_parameter_type(data, name);
     }
   return EINA_VALUE_EMPTY;
}

static void
_argument_selected_cb(void *data, const Efl_Event *ev)
{
   Outputter_Property_Value *value = data;

   if (value->simple)
     {
        Eina_Future* new_value = select_avaible_value(value, ev->object);
        Efl_Ui_Property_Value *v = outputter_property_value_value_get(value);
        eina_future_then(new_value, _new_value_cb, v);
     }
   else
     {
        push_ui_node(value->object, EINA_TRUE);
     }
}

static void
_fill_property_values(Efl_Ui_Group_Item *item, Outputter_Property *property)
{
   Outputter_Property_Value *value;
   Eina_Strbuf *displayed_value;
   Eina_Iterator *values = property->values;

   displayed_value = eina_strbuf_new();

   EINA_ITERATOR_FOREACH(values, value)
     {
        Eo *o;

        eina_strbuf_append(displayed_value, value->argument);
        eina_strbuf_append(displayed_value, " : ");
        if (value->simple)
          eina_strbuf_append_printf(displayed_value, "\"%s\"", value->value);
        else
          eina_strbuf_append(displayed_value, "Object");

        o = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, item);
        efl_text_set(o, eina_strbuf_string_get(displayed_value));
        efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, _argument_selected_cb, value);
        efl_pack_end(item, o);

        eina_strbuf_reset(displayed_value);
        //if this property is a object, update its stack
        if (!value->simple)
          _local_stack_update(value->object, outputter_node_get(value->object));
     }
   eina_strbuf_free(displayed_value);
   eina_iterator_free(property->values);
}

static Eina_Value
_add_new_property_cb(void *data, const Eina_Value value, const Eina_Future *f)
{
   Local_Stack *stack = data;
  if (eina_value_type_get(&value) != EINA_VALUE_TYPE_ERROR)
    {
        const char *name;
        EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_string_get(&value, &name), eina_value_error_init(eina_error_find("Failed to find type")));
        add_property(stack->tnode, name);
    }
  return EINA_VALUE_EMPTY;
}

static void
_new_property_create_cb(void *data, const Efl_Event *ev)
{
   Local_Stack *stack = data;
   Eina_Future *f = select_available_properties(stack->tnode);

   eina_future_then(f, _add_new_property_cb, stack);
}

static void
_prop_delete_cb(void *data, const Efl_Event *ev)
{
   del_property(data, efl_key_data_get(ev->object, "__name"));
}

static void
properties_flush(Local_Stack *data)
{
   Eina_Iterator *properties = outputter_properties_get(data->onode);
   Outputter_Property *property;

   efl_pack_clear(data->data->properties);

   EINA_ITERATOR_FOREACH(properties, property)
     {
        Property_List_Item_Data *item_data;
        const char *prop_name = eolian_object_name_get((const Eolian_Object*)property->property);

        item_data = property_list_item_gen(data->data->properties);
        efl_event_callback_add(item_data->delete, EFL_INPUT_EVENT_CLICKED, _prop_delete_cb, data->tnode);
        efl_key_data_set(item_data->delete, "__name", prop_name);
        efl_text_set(item_data->root, prop_name);
        efl_pack_end(data->data->properties, item_data->root);
        _fill_property_values(item_data->root, property);
        free(item_data);
     }
   eina_iterator_free(properties);
   // new property item
   New_Entry_Item_Data *item_data = new_entry_item_gen(data->data->properties);
   efl_event_callback_add(item_data->new, EFL_INPUT_EVENT_CLICKED, _new_property_create_cb, data);
   efl_pack_end(data->data->properties, item_data->root);
   free(item_data);
}

static Eina_Value
_add_new_child_cb(void *data, const Eina_Value value, const Eina_Future *f)
{
   Local_Stack *stack = data;
  if (eina_value_type_get(&value) != EINA_VALUE_TYPE_ERROR)
    {
        const char *name;
        EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_string_get(&value, &name), eina_value_error_init(eina_error_find("Failed to find type")));
        add_child(stack->tnode, name);
    }
  return EINA_VALUE_EMPTY;
}

static void
_new_child_create_cb(void *data, const Efl_Event *ev)
{
   Local_Stack *stack = data;
   Eina_Future *f = select_available_types();

   eina_future_then(f, _add_new_child_cb, stack);
}


static void
_delete_cb(void *data, const Efl_Event *ev)
{
   Efl_Ui_Node *node = outputter_node_get(efl_key_data_get(ev->object, "__child"));

   del_child(data, node);
}

static void
children_flush(Local_Stack *data)
{
   Eina_Iterator *children = outputter_children_get(data->onode);
   Outputter_Child *child;
   Eina_Strbuf *text = eina_strbuf_new();
   int i = 0;

   efl_pack_clear(data->data->children);

   EINA_ITERATOR_FOREACH(children, child)
     {
        Children_List_Item_Data *pd;
        const char *id = outputter_node_id_get(child->child);
        const Eolian_Class *klass = outputter_node_klass_get(child->child);
        _local_stack_update(child->child, outputter_node_get(child->child));

        pd = children_list_item_gen(data->data->properties);
        if (id)
          eina_strbuf_append_printf(text, "%s: ", id);
        else
          eina_strbuf_append_printf(text, "%s: ", eolian_class_name_get(klass));
        switch (child->type)
          {
             case EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR: {
               i++;
               eina_strbuf_append_printf(text, "(%d)", i);
             }
             break;
             case EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE: {
               eina_strbuf_append_printf(text, "(%d,%d,%d,%d)",
                child->table.x, child->table.y,
                child->table.w, child->table.h);
             }
             break;
             case EFL_UI_NODE_CHILDREN_TYPE_PACK: {
               eina_strbuf_append_printf(text, "(%s)", child->pack.pack);
             }
             break;
             default:
             break;
          }
        efl_text_set(pd->root, eina_strbuf_string_get(text));
        efl_pack_end(data->data->children, pd->root);
        efl_event_callback_add(pd->root, EFL_INPUT_EVENT_CLICKED, _push_node_cb, child->child);
        efl_event_callback_add(pd->delete, EFL_INPUT_EVENT_CLICKED, _delete_cb, data->tnode);
        efl_key_data_set(pd->delete, "__child", child->child);
        //FIXME delete
        eina_strbuf_reset(text);
     }
   eina_strbuf_free(text);
   eina_iterator_free(children);

   // new property item
   New_Entry_Item_Data *new_item = new_entry_item_gen(data->data->properties);
   efl_event_callback_add(new_item->new, EFL_INPUT_EVENT_CLICKED, _new_child_create_cb, data);
   efl_pack_end(data->data->children, new_item->root);

   if (node_child_type_get(data->tnode) == EFL_UI_NODE_CHILDREN_TYPE_NOTHING)
     efl_ui_widget_disabled_set(new_item->new, EINA_TRUE);

   free(new_item);
}

static Eina_Value
_selected_type_cb(void *data, const Eina_Value value, const Eina_Future *dead_future)
{
   Local_Stack *stack = data;
   if (eina_value_type_get(&value) != EINA_VALUE_TYPE_ERROR)
     {
        const char *name;
        EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_string_get(&value, &name), eina_value_error_init(eina_error_find("Failed to find type")));
        change_type(stack->tnode,  name);
     }

   return EINA_VALUE_EMPTY;
}

static void
_type_clicked_cb(void *data, const Efl_Event *ev)
{
   Eina_Future *soon_value = select_available_types();

   eina_future_then(soon_value, _selected_type_cb, data);
}

static void
ui_node_flush(Local_Stack *data)
{
   const Eolian_Class* klass = outputter_node_klass_get(data->onode);

   efl_text_set(data->data->id_text, outputter_node_id_get(data->onode));
   efl_text_set(data->data->type_text, eolian_object_name_get((const Eolian_Object*)klass));
   properties_flush(data);
   children_flush(data);
}

static void
_invalidate_cb(void *data, const Efl_Event *ev)
{
   Local_Stack *stack = data, *lower, *higher;

   if (highest == data)
     highest = stack->parent;
   if (root == data)
     root = stack->next;

   lower = stack->parent;
   higher = stack->next;

   if (lower)
     lower->next = higher;
   if (higher)
     higher->parent = lower;

   free(stack->data);
   free(stack);
}


static void
push_ui_node(Outputter_Node *node, Eina_Bool back_support)
{
   Local_Stack *stack = calloc(sizeof(Local_Stack), 1);

   update_stack = eina_array_new(10);
   stack->data = abstract_node_ui_gen(base_ui->prop_stack);
   stack->tnode = outputter_node_get(node);
   stack->onode = node;
   if (!back_support)
     efl_ui_widget_disabled_set(stack->data->back, !back_support);

   efl_event_callback_add(stack->data->back, EFL_INPUT_EVENT_CLICKED, _back_cb, NULL);
   //change of type
   efl_event_callback_add(stack->data->type, EFL_INPUT_EVENT_CLICKED, _change_node_type_cb, stack);
   efl_event_callback_add(stack->data->id, EFL_INPUT_EVENT_CLICKED, _change_id_cb, stack);
   efl_event_callback_add(stack->data->root, EFL_EVENT_INVALIDATE, _invalidate_cb, stack);
   ui_node_flush(stack);
   efl_ui_spotlight_push(base_ui->prop_stack, stack->data->root);

   //set correct stack pointer
   if (!highest && !root)
     highest = root = stack;
   else
     {
        highest->next = stack;
        stack->parent = highest;
        highest = stack;
     }
   EINA_SAFETY_ON_FALSE_RETURN(eina_array_count(update_stack) == 0);
   eina_array_free(update_stack);
   update_stack = NULL;
}

void
_base_ui_transform_value_cb(const Eolian_Type *etype, Eina_Strbuf *buf, const char *value)
{
   const Eolian_Type_Type type = eolian_type_type_get(etype);
   EINA_SAFETY_ON_FALSE_RETURN(type == EOLIAN_TYPE_REGULAR);
   const Eolian_Type_Builtin_Type bt = eolian_type_builtin_type_get(etype);

   if (bt == EOLIAN_TYPE_BUILTIN_STRING || bt == EOLIAN_TYPE_BUILTIN_STRINGSHARE || bt == EOLIAN_TYPE_BUILTIN_MSTRING)
      {
         eina_strbuf_append_n(buf, value+1, strlen(value+1) - 1);
      }
}

void
base_ui_refresh(Efl_Ui *ui)
{
   const char *name;
   Outputter_Node *oroot = outputter_node_init(editor_state, ui, &name, _base_ui_transform_value_cb);
   efl_text_set(base_ui->ui_name, name);

   if (!root && !highest)
     {
        push_ui_node(oroot, EINA_FALSE);
        EINA_SAFETY_ON_FALSE_RETURN(root && root->onode == oroot);
     }
   else
     {
        //this code first starts to update the root node which is visible
        //then the update_array gets filled when children are discovered that are also visualized.
        //this again fills the array until there is no more child that is already shown.
        int correctness_counter = 1;
        Outputter_Node *former_root = root->onode;
        Outputter_Node *n = oroot;
        Efl_Ui_Node *old_node;
        update_stack = eina_array_new(10);

        old_node = outputter_node_get(n);
        EINA_SAFETY_ON_FALSE_RETURN(root->tnode == old_node);
        root->onode = n;
        ui_node_flush(root);

        //the update code in the flushing fills the new node in onode_replacement
        //we fill it in the correct field here and update
        while(eina_array_count(update_stack) > 0)
          {
             Local_Stack *stack = eina_array_pop(update_stack);
             stack->onode = stack->onode_replacement;
             stack->onode_replacement = NULL;
             ui_node_flush(stack);
             correctness_counter ++;
          }

        //ensure that every item in the stack got updated as expected
        for (Local_Stack *stack = root; stack; stack = stack->next)
          {
             EINA_SAFETY_ON_FALSE_RETURN(!stack->onode_replacement);
             correctness_counter --;
          }

        eina_array_free(update_stack);
        update_stack = NULL;

        EINA_SAFETY_ON_FALSE_RETURN(correctness_counter == 0);

        //we can fill the outputter node tree from before
        outputter_node_root_free(former_root);
     }

}

static void
_win_gone_cb(void *data, const Efl_Event *ev)
{
   Outputter_Node *former_root = root->onode;
   efl_del(base_ui->root);
   outputter_node_root_free(former_root);
   free(base_ui);
}

void
base_ui_init(Efl_Ui_Win *win)
{
   base_ui = base_ui_gen(win);
   efl_content_set(win, base_ui->root);
   efl_event_callback_add(win, EFL_EVENT_INVALIDATE, _win_gone_cb, NULL);
   background = base_ui->background;
}
