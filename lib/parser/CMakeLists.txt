add_library(parser ast.cpp)

target_link_libraries(parser PRIVATE liblogs lexer)

target_include_directories(parser PUBLIC
                    ${CMAKE_CURRENT_LIST_DIR})