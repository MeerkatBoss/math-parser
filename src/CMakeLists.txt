add_executable(mathparser main.cpp)

target_link_libraries(mathparser PRIVATE libparser liblogs)

target_include_directories(mathparser PRIVATE
                        ${CMAKE_CURRENT_LIST_DIR})