add_executable(parser main.cpp)

target_include_directories(parser PRIVATE
                        ${CMAKE_CURRENT_LIST_DIR})