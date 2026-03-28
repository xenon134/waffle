cmake_minimum_required(VERSION 3.16)
project(waffle VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

set(CMAKE_AUTOMOC ON)

add_executable(waffle main.cpp)
add_executable(copyimage copyimage.cpp)

target_link_libraries(waffle PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(copyimage PRIVATE gdiplus ole32)
