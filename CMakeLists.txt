cmake_minimum_required(VERSION 3.5)

project(argparse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(examples)

enable_testing()  # ???
SET(BUILD_TESTING ON) # ???

if(BUILD_TESTING)
  find_package(GTest)
  if(GTEST_FOUND)
      add_subdirectory(tests)
  endif(GTEST_FOUND)
endif()



