add_executable(mathparser main.cpp diff_utils.cpp)

target_link_libraries(mathparser PRIVATE lexer parser treemath liblogs article)

target_include_directories(mathparser PRIVATE
                        ${CMAKE_CURRENT_LIST_DIR})