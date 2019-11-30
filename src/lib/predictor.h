#include <Efl_Ui_Format.h>

typedef struct _Predicted_Object                Predicted_Object;
typedef struct _Predicted_Class                 Predicted_Class;
typedef struct _Predicted_Property              Predicted_Property;
typedef struct _Predicted_Property_Details      Predicted_Property_Details;
typedef enum   _Predicted_Property_Details_Type Predicted_Property_Details_Type;

struct _Predicted_Object {
   const char *documentation;
   void *internal;
};

struct _Predicted_Class {
   Predicted_Object obj;
   const char *klass_name;
};

struct _Predicted_Property {
   Predicted_Object obj;
   const char *name;
};

enum _Predicted_Property_Details_Type {
  TYPE_BOOL = 4,
  TYPE_NUMBER = 3,
  TYPE_OBJECT = 2,
  TYPE_STRING = 1,

  TYPE_WHAT_THE_FUCK = 0
};

struct _Predicted_Property_Details {
   Predicted_Object obj;
   Predicted_Property_Details_Type type;
   const char *name;
};

void predictor_init(Eolian_State *s);

const Predicted_Class* get_available_types(void);
const Predicted_Property* get_available_properties(Efl_Ui_Node *node);
const Predicted_Property_Details* get_available_property_details(Efl_Ui_Node *node, const char *property_name);
