cmake_minimum_required(VERSION 3.18)

project(ioevent_driver)

add_library(${PROJECT_NAME} INTERFACE ioevent_driver.h ioevent_poll.h)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}