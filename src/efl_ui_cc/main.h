#ifndef MAIN_H
#define MAIN_H

#include <Eina.h>

extern const char* input_content;
extern const char* c_header_content;
extern const char* c_file_content;
extern const char *output_h_file;

typedef struct _Klass_Type Klass_Type;

void class_db_init(Eina_Bool support, Eina_Array *include_dirs);
void parse(void);

Klass_Type* klass_fetch(const char *klass);
void klass_free(Klass_Type *t);
Eina_Bool klass_property_append(Klass_Type *t, const char *prop, const char *value, Eina_Strbuf *goal);
void klass_field_append(Klass_Type *t, const char *field_name, Eina_Strbuf *goal);
void klass_create_instance(Klass_Type *t, const char *field_name, const char *parent, Eina_Strbuf *goal);
#endif
