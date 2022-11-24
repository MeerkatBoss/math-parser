add_library(parser ast.cpp parser.cpp)

target_link_libraries(parser PUBLIC liblogs lexer)

target_include_directories(parser PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR})