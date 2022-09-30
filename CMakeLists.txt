cmake_minimum_required(VERSION 3.8)

project("LameCC")

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_BUILD_TYPE Debug)
include_directories("third-party")
aux_source_directory(. DIR_SRCS)
add_executable(LameCC ${DIR_SRCS})