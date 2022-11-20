add_library(libparser token_list.cpp)

target_link_libraries(libparser PUBLIC liblogs liblist)

target_include_directories(libparser PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR})