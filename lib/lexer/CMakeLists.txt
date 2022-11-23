add_library(lexer token_list.cpp lexer.cpp)

target_link_libraries(lexer PUBLIC liblogs liblist)

target_include_directories(lexer PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR})