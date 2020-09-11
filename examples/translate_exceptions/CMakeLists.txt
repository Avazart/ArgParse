cmake_minimum_required(VERSION 3.5)

project(translate_exceptions LANGUAGES CXX)

aux_source_directory(. SRC_LIST)

add_executable(${PROJECT_NAME}  ${SRC_LIST})
