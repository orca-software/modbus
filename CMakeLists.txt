cmake_minimum_required(VERSION 3.1)
project(modbus)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Boost REQUIRED COMPONENTS system thread coroutine)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads)

include_directories(include)
include_directories(SYSTEM
	${Boost_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}
	src/client.cpp
	src/server.cpp
	src/error.cpp
)

add_executable(${PROJECT_NAME}_test_client
	src/test.cpp
)

set_target_properties(${PROJECT_NAME}_test_client PROPERTIES
  COMPILE_FLAGS "-ggdb -Wall -pedantic"
)

target_link_libraries(${PROJECT_NAME}
	${Boost_LIBRARIES}
	Threads::Threads
)

target_link_libraries(${PROJECT_NAME}_test_client
	${PROJECT_NAME}
	Threads::Threads
)

