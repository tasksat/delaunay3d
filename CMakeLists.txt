cmake_minimum_required(VERSION 3.20)

project(delauday_pd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tetrahedralize
    main.cpp
    delaunay/builder.cpp
    delaunay/delaunay.cpp
)

target_include_directories(tetrahedralize PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/eigen3
)