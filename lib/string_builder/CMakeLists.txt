add_library(stringbuilder string_builder.cpp)

target_link_libraries(stringbuilder PRIVATE liblogs)

target_include_directories(stringbuilder PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR})