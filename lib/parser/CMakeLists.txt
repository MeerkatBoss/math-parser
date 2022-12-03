add_library(parser ast.cpp parser.cpp var_name_array.cpp)

target_link_libraries(parser PUBLIC liblogs lexer mathutils dynamicarray stringbuilder)

target_include_directories(parser PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR})