cmake_minimum_required(VERSION 3.14)
project(bmc_clid)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src CLID_SRC)

add_executable(bmc_clid ${CLID_SRC} src/Commandline.cpp inc/CommandLine.hpp)

target_link_libraries(bmc_clid readline)