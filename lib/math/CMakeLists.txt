add_library(treemath tree_math.cpp)

target_link_libraries(treemath PUBLIC liblogs parser)

target_include_directories(treemath PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR})