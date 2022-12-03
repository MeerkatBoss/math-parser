add_library(dynamicarray INTERFACE)

target_link_libraries(dynamicarray INTERFACE liblogs)

target_include_directories(dynamicarray INTERFACE
                        ${CMAKE_CURRENT_LIST_DIR})