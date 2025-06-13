cmake_minimum_required(VERSION 3.16)
project(obs-dynamic-path-plugin)

set(CMAKE_CXX_STANDARD 17)

find_package(obs REQUIRED)

add_library(${PROJECT_NAME} MODULE
    obs-dynamic-recording-path-plugin.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    libobs
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${OBS_INCLUDE_DIR}
)
