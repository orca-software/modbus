cmake_minimum_required(VERSION 3.1)
project(modbus)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS system thread coroutine)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads)

include_directories(include)

include_directories(SYSTEM
	${Boost_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}_test
	test/test.cpp
)

set_target_properties(${PROJECT_NAME}_test PROPERTIES
  COMPILE_FLAGS "-ggdb -Wall -pedantic"
)

target_link_libraries(${PROJECT_NAME}_test
  ${Boost_LIBRARIES}
  Threads::Threads
)

