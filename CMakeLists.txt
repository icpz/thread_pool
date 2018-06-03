cmake_minimum_required(VERSION 3.2)
project(thread_pool)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/thread_pool.h DESTINATION include)

