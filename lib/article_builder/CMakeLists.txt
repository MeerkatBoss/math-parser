add_library(article article_builder.cpp)

target_link_libraries(article PUBLIC stringbuilder liblogs libinput)

target_include_directories(article PUBLIC
                        ${CMAKE_CURRENT_LIST_DIR})