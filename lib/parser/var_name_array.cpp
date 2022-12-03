#include "var_name_array.h"

#define ARRAY_ELEMENT var_name

#include "dynamic_array_impl.h"

int array_try_find_variable(const dynamic_array(var_name)* array, const char* var, size_t* var_id)
{
    for (size_t i = 0; i < array->size; i++)
        if (strcmp(*array_get_element(array, i), var) == 0)
        {
            *var_id = i;
            return 1;
        }
    return 0;
}