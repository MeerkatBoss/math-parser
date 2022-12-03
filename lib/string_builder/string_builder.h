#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stdio.h>

struct string_builder
{
    char* data;
    size_t capacity;
    size_t size;
};

/* TODO: docs */

void string_builder_ctor(string_builder* builder, const char* str = NULL);
void string_builder_copy(string_builder* dest, const string_builder* src);
void string_builder_dtor(string_builder* builder);
void string_builder_append(string_builder* builder, char c);
void string_builder_append(string_builder* builder, const char* str);
void string_builder_append_format(string_builder* builder, const char* format, ...)
    __attribute__((format(printf, 2, 3)));
char* string_builder_get_string(const string_builder* builder);
void string_builder_print(const string_builder* builder, FILE* stream);

#endif