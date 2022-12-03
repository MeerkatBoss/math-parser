#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "logger.h"

#include "string_builder.h"

static const size_t DEFAULT_CAP = 128;

static inline size_t get_min_cap(size_t size)
{
    size_t res = DEFAULT_CAP;
    while (res <= size)
        res *= 2;
    return res;
}

static void ensure_fit(string_builder *builder, size_t new_size);

void string_builder_ctor(string_builder *builder, const char* str)
{
    LOG_ASSERT(builder != NULL, return);

    size_t cap = DEFAULT_CAP;

    if (str) cap = get_min_cap(strlen(str));

    *builder = {
        .data = (char*) calloc(cap, sizeof(char)),
        .capacity = cap,
        .size = 0
    };

    if (str)
    {
        size_t len = strlen(str);
        strncpy(builder->data, str, len);
        builder->size = len;
    }
}

void string_builder_copy(string_builder *dest, const string_builder *src)
{
    LOG_ASSERT(dest != NULL, return);
    LOG_ASSERT(src != NULL, return);
    LOG_ASSERT(dest->data == NULL, return);

    dest->capacity = src->capacity;
    dest->size = src->capacity;
    dest->data = (char*) calloc(dest->capacity, sizeof(char));

    strncpy(dest->data, src->data, dest->size);
}

void string_builder_dtor(string_builder *builder)
{
    LOG_ASSERT(builder != NULL, return);

    free(builder->data);
    *builder = {};
    return;
}

void string_builder_append(string_builder *builder, char c)
{
    LOG_ASSERT(builder != NULL, return);

    builder->data[builder->size] = c;
    builder->size++;
    ensure_fit(builder, builder->size);
}

void string_builder_append(string_builder *builder, const char *str)
{
    LOG_ASSERT(builder != NULL, return);
    LOG_ASSERT(str != NULL, return);

    size_t len = strlen(str);
    ensure_fit(builder, builder->size + len);

    strncpy(builder->data + builder->size, str, len);
    builder->size += len;
}

void string_builder_append_format(string_builder *builder, const char *format, ...)
{
    LOG_ASSERT(builder != NULL, return);
    LOG_ASSERT(format != NULL, return);

    va_list args = {};
    va_start(args, format);

    int len = vsnprintf(NULL, 0, format, args);

    va_end(args);

    ensure_fit(builder, builder->size + len);

    va_start(args, format);

    vsprintf(builder->data + builder->size, format, args);
    builder->size += len;

    va_end(args);
}

char *string_builder_get_string(const string_builder *builder)
{
    return strndup(builder->data, builder->size);
}

void string_builder_print(const string_builder *builder, FILE *stream)
{
    int fd = fileno(stream);
    string_builder_write(builder, fd);
}

void string_builder_write(const string_builder *builder, int fd)
{
    write(fd, builder->data, builder->size * sizeof(char));
}

void ensure_fit(string_builder *builder, size_t new_size)
{
    size_t new_cap = get_min_cap(new_size);
    if (new_cap > builder->capacity)
    {
        builder->capacity = new_cap;
        builder->data = (char*) reallocarray(builder->data, builder->capacity, sizeof(char));
    }
}
