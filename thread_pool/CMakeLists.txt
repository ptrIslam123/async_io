cmake_minimum_required(VERSION 3.18)

project(static_thread_pool)

add_library(${PROJECT_NAME} static_thread_pool.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}