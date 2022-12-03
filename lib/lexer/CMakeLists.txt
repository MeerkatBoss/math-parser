add_library(lexer token_array.cpp lexer.cpp)

target_link_libraries(lexer PUBLIC liblogs dynamicarray)

target_include_directories(lexer PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR})