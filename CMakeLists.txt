cmake_minimum_required(VERSION 3.20)

project(usbnotify LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

add_executable(usbnotify usbnotify.cpp)

target_link_libraries(usbnotify PRIVATE
  PkgConfig::SYSTEMD)

target_include_directories(usbnotify PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")


target_compile_options(usbnotify PRIVATE -Wall -Wextra -Wpedantic)
