#include <stdlib.h>
#include <time.h>

#include "logger.h"

#include "article_builder.h"

static inline int rand_init(void) { srand(time(NULL)); return 1; }

static int rand_initialized_ = rand_init();

static inline size_t my_rand(void) {return ((size_t)rand() << 16) + (size_t)rand(); }

void article_ctor(article_builder *article)
{
    LOG_ASSERT(article, return);

    *article = {.state = ARTC_NEW};
    string_builder_ctor(&article->text, "\\documentclass{article}\n");
}

void article_use_starters(article_builder *article, const char *filename)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(filename, return);

    article->starters = read_file(filename);
}

void article_use_transitions(article_builder *article, const char *filename)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(filename, return);

    article->transitions = read_file(filename);
}

void article_use_placeholders(article_builder *article, const char *filename)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(filename, return);

    article->placeholders = read_file(filename);
}

void article_use_preamble(article_builder *article, const char *filename)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(filename, return);
    LOG_ASSERT(article->state == ARTC_NEW, return);

    string_builder_append_format(&article->text, "\\usepackage{preamble}\n");
    article->preamble = filename;
}

void article_dtor(article_builder *article)
{
    LOG_ASSERT(article, return);

    string_builder_dtor(&article->text);
    if (article->starters    .text != NULL) dispose_lines(&article->starters);
    if (article->transitions .text != NULL) dispose_lines(&article->transitions);
    if (article->placeholders.text != NULL) dispose_lines(&article->placeholders);

    *article = {.state = ARTC_DELETED};
}

void article_add_title(article_builder *article, const char *title, const char* author)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(title, return);
    LOG_ASSERT(article->state == ARTC_NEW, return);

    string_builder_append_format(&article->text, "\\title{%s}\n"
                                                 "\\author{%s}\n",
                                                 title, author);
    article->state = ARTC_TITLE;
}

void article_start(article_builder *article)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(article->state == ARTC_NEW, return);

    string_builder_append(&article->text,
                        "\\begin{document}\n"
                        "\\maketitle\n");
    
    article->state = ARTC_STARTED;
}

void article_end(article_builder *article)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(article->state == ARTC_STARTED, return);

    string_builder_append(&article->text, "\\end{document}\n");

    article->state = ARTC_ENDED;
}

void article_add_abstract(article_builder *article, const char *abstract)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(abstract, return);
    LOG_ASSERT(article->state == ARTC_STARTED, return);

    string_builder_append_format(&article->text, "\\begin{abstract}\n"
                                                 "%s\n"
                                                 "\\end{abstract}\n", abstract);
}

void article_add_section(article_builder *article, const char *title)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(title, return);
    LOG_ASSERT(article->state == ARTC_STARTED, return);

    string_builder_append_format(&article->text, "\\newpage\n"
                                                 "\\section{%s}\n", title);
}

void article_add_starter(article_builder *article)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(article->state == ARTC_STARTED, return);

    if (article->starters.text == NULL)
    {
        string_builder_append(&article->text, "Now look at this:\n");
        return;
    }

    string_builder_append_format(&article->text, "\n%s\n",
                article->starters.lines[my_rand() % article->starters.line_count].line);
}

void article_add_transition(article_builder *article)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(article->state == ARTC_STARTED, return);

    if (article->transitions.text == NULL)
    {
        string_builder_append(&article->text, "As you can see,");
        return;
    }

    string_builder_append_format(&article->text, "%s ",
                article->transitions.lines[my_rand() % article->transitions.line_count].line);
}

void article_add_placeholder(article_builder *article)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(article->state == ARTC_STARTED, return);

    if (article->placeholders.text == NULL)
    {
        string_builder_append(&article->text, "is easy to see.\n\n");
        return;
    }

    string_builder_append_format(&article->text, "%s\n",
                article->placeholders.lines[my_rand() % article->placeholders.line_count].line);
}

void article_build(article_builder *article, const char *output_dir)
{
    LOG_ASSERT(article, return);
    LOG_ASSERT(output_dir, return);
    LOG_ASSERT(article->state == ARTC_ENDED, return);

    FILE* output = fopen("article.tex", "w+");
    string_builder_print(&article->text, output);
    fclose(output);

    FILE* shell = popen("sh", "w");

    if (article->preamble)
        fprintf(shell, "cp -T %s %s/preamble.sty\n", article->preamble, output_dir);
    fprintf(shell, "mv article.tex %s\n", output_dir);
    fprintf(shell, "cd %s && pdflatex -shell-escape article.tex", output_dir);

    pclose(shell);
}
