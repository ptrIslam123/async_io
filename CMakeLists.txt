cmake_minimum_required(VERSION 3.18)

project(async_io)

add_subdirectory(asyncio_driver)
add_subdirectory(ioevent_driver)
add_subdirectory(thread_pool)

add_executable(main main.cpp)

target_link_libraries(main INTERFACE ioevent_driver)
target_link_libraries(main PUBLIC asyncio_driver static_thread_pool)