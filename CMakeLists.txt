cmake_minimum_required(VERSION 3.21)
project(TestTask3)

set(CMAKE_CXX_STANDARD 14)

include_directories(/opt/curlcpp/include)
link_directories(/opt/curlcpp/build/src)

add_executable(http_getter http_getter.cpp)
target_link_libraries(http_getter PRIVATE -lcurlcpp -lcurl)

add_executable(program main.cpp json.hpp)

