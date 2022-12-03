#ifndef ARTICLE_BUILDER
#define ARTICLE_BUILDER

#include "string_builder.h"
#include "text_lines.h"

enum article_state
{
    ARTC_NEW,
    ARTC_TITLE,
    ARTC_STARTED,
    ARTC_ENDED,
    ARTC_DELETED
};

struct article_builder
{
    article_state state;
    TextLines starters;
    TextLines transitions;
    TextLines placeholders;
    string_builder text;
    const char* preamble;
};

void article_ctor(article_builder* article);
void article_use_starters(article_builder* article, const char* filename);
void article_use_transitions(article_builder* article, const char* filename);
void article_use_placeholders(article_builder* article, const char* filename);
void article_use_preamble(article_builder* article, const char* filename);
void article_dtor(article_builder* article);
void article_add_title(article_builder* article, const char* title, const char* author);
void article_start(article_builder* article);
void article_end(article_builder* article);
void article_add_abstract(article_builder* article, const char* abstract);
void article_add_section(article_builder* article, const char* title);
void article_add_starter(article_builder* article);
void article_add_transition(article_builder* article);
void article_add_placeholder(article_builder* article);
void article_build(article_builder* article, const char* output_dir);

#endif