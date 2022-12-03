#ifndef VAR_NAME_ARRAY
#define VAR_NAME_ARRAY

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef char* var_name;

#define ARRAY_ELEMENT var_name

inline void copy_element(ARRAY_ELEMENT* dest, const ARRAY_ELEMENT* src) { *dest = strdup(*src); }
inline void delete_element(ARRAY_ELEMENT* element) { free(*element); *element = NULL; }

#include "dynamic_array.h"

/**
 * @brief Attempt to find variable by its name
 * 
 * @param[in] array Array, containing variable name
 * @param[in] var Variable name
 * @param[out] var_id Variable id
 * @return 0 if variable with given name does not, non-zero otherwise
 */
int array_try_find_variable(const dynamic_array(var_name)* array, const char* var, size_t* var_id);

#undef ARRAY_ELEMENT

#endif