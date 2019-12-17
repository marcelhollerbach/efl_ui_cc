#define EFL_BETA_API_SUPPORT
#define _GNU_SOURCE
#include <dlfcn.h>
#include <ffi.h>
#include <stdio.h>

#include "main.h"
#include <Efl_Ui.h>
#include <Ecore_File.h>
#include <Efl_Ui_Format.h>

typedef struct {
   Eina_Strbuf *typedef_fields;
} Object_Generator_Context;

static Efl_Ui_Widget* _generate_node(Object_Generator_Context *ctx, Outputter_Node *n, Efl_Ui_Widget *parent);

typedef struct {
   Eolian_Type_Builtin_Type type;
   ffi_type                *ffi_type;
} Type_Mapping;

Type_Mapping mapping[] = {
   {EOLIAN_TYPE_BUILTIN_BYTE,  &ffi_type_sint8},
   {EOLIAN_TYPE_BUILTIN_UBYTE, &ffi_type_uint8},
   {EOLIAN_TYPE_BUILTIN_CHAR,  &ffi_type_sint8},
   {EOLIAN_TYPE_BUILTIN_SHORT, &ffi_type_sint16},
   {EOLIAN_TYPE_BUILTIN_USHORT, &ffi_type_uint16},
   {EOLIAN_TYPE_BUILTIN_INT, &ffi_type_sint32},
   {EOLIAN_TYPE_BUILTIN_UINT, &ffi_type_uint32},
   {EOLIAN_TYPE_BUILTIN_LONG, &ffi_type_sint64},
   {EOLIAN_TYPE_BUILTIN_ULONG, &ffi_type_uint64},
   {EOLIAN_TYPE_BUILTIN_LLONG, NULL},
   {EOLIAN_TYPE_BUILTIN_ULLONG, NULL},
   {EOLIAN_TYPE_BUILTIN_INT8, &ffi_type_sint32},
   {EOLIAN_TYPE_BUILTIN_UINT8, &ffi_type_uint32},
   {EOLIAN_TYPE_BUILTIN_INT16, &ffi_type_sint16},
   {EOLIAN_TYPE_BUILTIN_UINT16, &ffi_type_uint16},
   {EOLIAN_TYPE_BUILTIN_INT32, &ffi_type_sint32},
   {EOLIAN_TYPE_BUILTIN_UINT32, &ffi_type_uint32},
   {EOLIAN_TYPE_BUILTIN_INT64, &ffi_type_sint64},
   {EOLIAN_TYPE_BUILTIN_UINT64, &ffi_type_uint64},
   {EOLIAN_TYPE_BUILTIN_INT128, NULL},
   {EOLIAN_TYPE_BUILTIN_UINT128, NULL},
   {EOLIAN_TYPE_BUILTIN_SIZE, NULL},
   {EOLIAN_TYPE_BUILTIN_SSIZE, NULL},
   {EOLIAN_TYPE_BUILTIN_INTPTR, NULL},
   {EOLIAN_TYPE_BUILTIN_UINTPTR, NULL},
   {EOLIAN_TYPE_BUILTIN_PTRDIFF, NULL},
   {EOLIAN_TYPE_BUILTIN_TIME, NULL},
   {EOLIAN_TYPE_BUILTIN_FLOAT, &ffi_type_float},
   {EOLIAN_TYPE_BUILTIN_DOUBLE, &ffi_type_double},
   {EOLIAN_TYPE_BUILTIN_BOOL, &ffi_type_uint8},
   {EOLIAN_TYPE_BUILTIN_SLICE, NULL},
   {EOLIAN_TYPE_BUILTIN_RW_SLICE, NULL},
   {EOLIAN_TYPE_BUILTIN_VOID, NULL},
   {EOLIAN_TYPE_BUILTIN_ACCESSOR, NULL},
   {EOLIAN_TYPE_BUILTIN_ARRAY, NULL},
   {EOLIAN_TYPE_BUILTIN_FUTURE, NULL},
   {EOLIAN_TYPE_BUILTIN_ITERATOR, NULL},
   {EOLIAN_TYPE_BUILTIN_LIST, NULL},
   {EOLIAN_TYPE_BUILTIN_ANY_VALUE, NULL},
   {EOLIAN_TYPE_BUILTIN_ANY_VALUE_REF, NULL},
   {EOLIAN_TYPE_BUILTIN_BINBUF, NULL},
   {EOLIAN_TYPE_BUILTIN_EVENT, NULL},
   {EOLIAN_TYPE_BUILTIN_MSTRING, NULL},
   {EOLIAN_TYPE_BUILTIN_STRING, &ffi_type_pointer},
   {EOLIAN_TYPE_BUILTIN_STRINGSHARE, &ffi_type_pointer},
   {EOLIAN_TYPE_BUILTIN_STRBUF, NULL},
   {EOLIAN_TYPE_BUILTIN_HASH, NULL},
   {EOLIAN_TYPE_BUILTIN_VOID_PTR, NULL},
   {EOLIAN_TYPE_BUILTIN_INVALID, NULL},

};

static ffi_type*
_fetch_type_mapping(Eolian_Type_Builtin_Type type)
{
   for (int i = 0; mapping[i].type; ++i)
     {
        if (mapping[i].type == type)
          {
             EINA_SAFETY_ON_FALSE_RETURN_VAL(mapping[i].ffi_type, NULL);
             return mapping[i].ffi_type;
          }
     }
   EINA_SAFETY_ON_FALSE_RETURN_VAL(EINA_FALSE, NULL);
}

static Eina_Bool
fill_in_data(void *ctx, const Eolian_Type *type, const Outputter_Property_Value *value, void **data)
{
   Eolian_Type_Builtin_Type builtin = eolian_type_builtin_type_get(type);

   if (builtin >= EOLIAN_TYPE_BUILTIN_BYTE && builtin <= EOLIAN_TYPE_BUILTIN_PTRDIFF)
     {
        *data = (void*)(intptr_t)atoi(value->value);
     }
   else if (builtin >= EOLIAN_TYPE_BUILTIN_FLOAT && builtin <= EOLIAN_TYPE_BUILTIN_DOUBLE)
     {
        *data = (void*)(intptr_t)atof(value->value);
     }
   else if (builtin == EOLIAN_TYPE_BUILTIN_BOOL)
     {
        if (eina_streq(value->value, "EINA_TRUE"))
          *data = (void*)(intptr_t)EINA_TRUE;
        else if (eina_streq(value->value, "EINA_FALSE"))
          *data = (void*)(intptr_t)EINA_FALSE;
        else
          {
             printf("Error, no value is not expected (%s)\n", value->value);
             return EINA_FALSE;
          }
     }
   else if (builtin == EOLIAN_TYPE_BUILTIN_MSTRING || builtin == EOLIAN_TYPE_BUILTIN_STRING || builtin == EOLIAN_TYPE_BUILTIN_STRINGSHARE)
     {
        *data = (void*) value->value;
     }
   else
     {
        printf("Type %d is not handled yet.\n", builtin);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static Eina_Bool
_call_property(Object_Generator_Context *ctx, Eo *obj, Outputter_Property *property)
{
   Outputter_Property_Value *value;
   ffi_cif cif;
   ffi_type *types[200];
   void *values[200];
   void *pointers[200];
   int value_count = 0;

   const Eolian_Function *function = property->property;
   const char *full_func_name = eolian_function_full_c_name_get(function,  EOLIAN_PROP_SET);
   void *eo_func = dlsym(RTLD_DEFAULT, full_func_name);

   EINA_SAFETY_ON_NULL_RETURN_VAL(function, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(full_func_name, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(eo_func, EINA_FALSE);

   //every eo function passes the eo object as first argument
   pointers[value_count] = obj;
   values[value_count] = &pointers[value_count];
   types[value_count] = &ffi_type_pointer;
   value_count++;

   EINA_ITERATOR_FOREACH(property->values, value)
     {
        const Eolian_Type *type = value->type;
        const Eolian_Type_Type ttype = eolian_type_type_get(type);

        if (value->simple)
          {
             const Eolian_Typedecl *decl = eolian_type_typedecl_get(type);
             EINA_SAFETY_ON_FALSE_RETURN_VAL(ttype == EOLIAN_TYPE_REGULAR, EINA_FALSE);
             if (decl && eolian_typedecl_type_get(decl) == EOLIAN_TYPEDECL_ENUM)
               {
                  //asserting that enums are getting passed arround as an int
                  const Eolian_Enum_Type_Field *field = eolian_typedecl_enum_field_get(decl, value->value);
                  if (!field)
                    {
                       printf("Error, unable to fetch ptr for %s\n", value->value);
                       return EINA_FALSE;
                    }
                  //fetch value of the field that is used
                  const Eolian_Expression *exp = eolian_typedecl_enum_field_value_get(field, EINA_TRUE);
                  Eolian_Value val = eolian_expression_value_get(exp);
                  int value = val.value.u;

                  types[value_count] = &ffi_type_uint32;
                  pointers[value_count] = (void*)(intptr_t)value;
               }
             else
               {
                  const Eolian_Type_Builtin_Type bt = eolian_type_builtin_type_get(type);

                  types[value_count] = _fetch_type_mapping(bt);
                  fill_in_data(ctx, type, value, &pointers[value_count]);
               }
          }
        else
          {
             //recursive depth for creating the tree
             EINA_SAFETY_ON_FALSE_RETURN_VAL(ttype == EOLIAN_TYPE_CLASS, EINA_FALSE);
             pointers[value_count] = _generate_node(ctx, value->object, obj);
             types[value_count] = &ffi_type_pointer;
          }
        values[value_count] = &pointers[value_count];
        value_count ++;
     }
   eina_iterator_free(property->values);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, value_count, &ffi_type_void, types) == FFI_OK, EINA_FALSE);
   ffi_call(&cif, eo_func, &ffi_type_void, values);

   return EINA_TRUE;
}

static Efl_Ui_Widget*
_generate_node(Object_Generator_Context *ctx, Outputter_Node *n, Efl_Ui_Widget *parent)
{
   const Eolian_Class *klass;
   const Efl_Class* (*klass_func)(void);
   void *obj;

   klass = outputter_node_klass_get(n);
   klass_func = dlsym(RTLD_DEFAULT, eolian_class_c_get_function_name_get(klass));
   obj = efl_add(klass_func(), parent);

   EINA_SAFETY_ON_NULL_RETURN_VAL(klass, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(klass_func, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, NULL);

   //handle properties
   Outputter_Property *property;
   Eina_Iterator *properties = outputter_properties_get(n);
   EINA_ITERATOR_FOREACH(properties, property)
     {
        _call_property(ctx, obj, property);
     }
   eina_iterator_free(properties);

   //handle children
   Outputter_Child *child;
   Eina_Iterator *children = outputter_children_get(n);

   EINA_ITERATOR_FOREACH(children, child)
     {
        void *c = _generate_node(ctx, child->child, obj);
        if (child->type == EFL_UI_NODE_CHILDREN_TYPE_PACK_LINEAR)
          {
             efl_pack_end(obj, c);
          }
        else if (child->type == EFL_UI_NODE_CHILDREN_TYPE_PACK_TABLE)
          {
             efl_pack_table(obj, c, child->table.x, child->table.y, child->table.w, child->table.h);
          }
        else if (child->type == EFL_UI_NODE_CHILDREN_TYPE_PACK)
          {
             efl_content_set(efl_part(obj, child->pack.pack), c);
          }
     }
   eina_iterator_free(children);

   return obj;
}

void
_obj_gen_transform_value_cb(const Eolian_Type *etype, Eina_Strbuf *buf, const char *value)
{
   const Eolian_Type_Type type = eolian_type_type_get(etype);
   EINA_SAFETY_ON_FALSE_RETURN(type == EOLIAN_TYPE_REGULAR);
   const Eolian_Type_Builtin_Type bt = eolian_type_builtin_type_get(etype);

   if (bt == EOLIAN_TYPE_BUILTIN_STRING || bt == EOLIAN_TYPE_BUILTIN_STRINGSHARE || bt == EOLIAN_TYPE_BUILTIN_MSTRING)
      {
         eina_strbuf_append_n(buf, value+1, strlen(value+1) - 1);
      }
}

Efl_Ui_Widget*
object_generator(Efl_Ui_Win *win, const Eolian_State *s, const Efl_Ui *ui)
{
   Object_Generator_Context ctx;
   const char *name;

   Outputter_Node *root = outputter_node_init((Eolian_State*)s, (Efl_Ui*)ui, &name, _obj_gen_transform_value_cb);
   Eo *ret = _generate_node(&ctx, root, win);
   outputter_node_root_free(root);
   return ret;
}
