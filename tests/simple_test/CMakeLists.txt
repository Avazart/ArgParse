cmake_minimum_required(VERSION 3.5)

project(simple_test LANGUAGES CXX)

aux_source_directory(. SRC_LIST)

add_executable(${PROJECT_NAME}  ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} GTest::GTest GTest::Main)
include_directories(SYSTEM ${GTEST_INCLUDE_DIR})




